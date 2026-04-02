// PB3D - 3D rendering layer using glTF models via cgltf
// Sits between PBOGLES and PBGfx in the inheritance chain

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "PB3D.h"
#include "3rdparty/cgltf.h"
#include "3rdparty/linmath.h"
#include "3rdparty/stb_image.h"
#include <algorithm>
#include <climits>
#include <cstring>
#include <cmath>
#include <random>
#include <set>

// ============================================================================
// 3D Shader Sources (GLSL ES 3.0)
// ============================================================================

const char* PB3D::vertexShader3DSource = R"(#version 300 es
    precision mediump float;
    in vec3 aPosition;
    in vec3 aNormal;
    in vec2 aTexCoord;
    uniform mat4 uMVP;
    uniform mat4 uModel;
    out vec2 vTexCoord;
    out vec3 vNormal;
    out vec3 vWorldPos;
    void main() {
        gl_Position = uMVP * vec4(aPosition, 1.0);
        vWorldPos = (uModel * vec4(aPosition, 1.0)).xyz;
        vNormal = mat3(uModel) * aNormal;
        vTexCoord = aTexCoord;
    }
)";

const char* PB3D::fragmentShader3DSource = R"(#version 300 es
    precision mediump float;
    in vec2 vTexCoord;
    in vec3 vNormal;
    in vec3 vWorldPos;
    uniform sampler2D uTexture;
    uniform vec3 uLightDir;
    uniform vec3 uLightColor;
    uniform vec3 uAmbientColor;
    uniform vec3 uCameraEye;
    uniform float uAlpha;
    out vec4 fragColor;
    void main() {
        vec4 texColor = texture(uTexture, vTexCoord);
        vec3 norm = normalize(vNormal);
        vec3 lightDir = normalize(uLightDir);
        float diffuse = max(dot(norm, lightDir), 0.0);
        // Blinn-Phong specular highlight
        vec3 viewDir = normalize(uCameraEye - vWorldPos);
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfDir), 0.0), 32.0);
        vec3 finalColor = texColor.rgb * (uAmbientColor + diffuse * uLightColor)
                        + spec * 0.1 * uLightColor;
        fragColor = vec4(finalColor, texColor.a * uAlpha);
    }
)";

// Skinned-mesh vertex shader: identical to the static shader but transforms
// positions and normals through the weighted sum of up to 4 bone matrices.
// Vertex layout (16 floats): pos3 norm3 uv2 joints4 weights4
const char* PB3D::vertexShader3DSkinnedSource = R"(#version 300 es
    precision mediump float;
    in vec3 aPosition;
    in vec3 aNormal;
    in vec2 aTexCoord;
    in vec4 aJoints;   // bone indices (stored as float, cast to int in shader)
    in vec4 aWeights;  // blend weights (sum to 1.0)
    uniform mat4 uMVP;
    uniform mat4 uModel;
    uniform mat4 uBones[1024];
    out vec2 vTexCoord;
    out vec3 vNormal;
    out vec3 vWorldPos;
    void main() {
        mat4 skinMat = aWeights.x * uBones[int(aJoints.x)]
                     + aWeights.y * uBones[int(aJoints.y)]
                     + aWeights.z * uBones[int(aJoints.z)]
                     + aWeights.w * uBones[int(aJoints.w)];
        vec4 skinnedPos = skinMat * vec4(aPosition, 1.0);
        gl_Position = uMVP * skinnedPos;
        vWorldPos   = (uModel * skinnedPos).xyz;
        mat3 skinNorm = mat3(skinMat);
        vNormal   = normalize(skinNorm * aNormal);
        vTexCoord = aTexCoord;
    }
)";

// ============================================================================
// Constructor / Destructor
// ============================================================================

PB3D::PB3D() {
    m_next3dModelId = 1;
    m_next3dInstanceId = 1;

    m_sceneDirty = true;
    m_skinnedShaderActive = false;

    // Default camera: eye straight back on Z axis so Z=0 maps to screen surface.
    // FOV=45, aspect handled at render time. eyeZ=8 gives a comfortable frustum size.
    m_camera = {0.0f, 0.0f, 8.0f,   0.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f,   45.0f,   0.1f, 100.0f};

    // Default light: direction from upper-right in front of camera (0.5, 1.0, 1.0)
    // so that the front face (normal +Z) and top face (normal +Y) receive diffuse light.
    // Slightly warm light color, slightly cool ambient for natural sky/fill contrast.
    m_light = {0.5f, 1.0f, 1.0f,   1.0f, 0.95f, 0.85f,   0.15f, 0.15f, 0.2f};

    memset(m_viewMatrix, 0, sizeof(m_viewMatrix));
    memset(m_projMatrix, 0, sizeof(m_projMatrix));
}

PB3D::~PB3D() {
    // Clean up all 3D models (release GPU resources via PBOGLES)
    for (auto& pair : m_3dModelList) {
        for (auto& mesh : pair.second.meshes) {
            ogl3dDestroyMesh(mesh.vao, mesh.vboVertices, mesh.eboIndices);
        }
        for (unsigned int texId : pair.second.ownedTextures) {
            ogl3dDestroyTexture(texId);
        }
    }
    ogl3dDestroyShader();
    ogl3dDestroySkinnedShader();
}

// ============================================================================
// Console output
// ============================================================================

void PB3D::pb3dSendConsole(const std::string& msg, bool debug) {
    (void)debug;  // standalone path has no suppress mechanism; always print
    std::cout << msg << std::endl;
}

// ============================================================================
// Initialization
// ============================================================================

bool PB3D::pb3dInit() {
    // Compile and link the static 3D shader (for non-skinned models)
    if (!ogl3dInitShader(vertexShader3DSource, fragmentShader3DSource)) {
        pb3dSendConsole("PB3D: Failed to create 3D shader program");
        return false;
    }
    // Compile and link the skinned 3D shader (for models with bone skeletons)
    // Fragment shader is shared with the static path.
    if (!ogl3dInitSkinnedShader(vertexShader3DSkinnedSource, fragmentShader3DSource)) {
        pb3dSendConsole("PB3D: WARNING - Failed to create skinned 3D shader; skeleton animation disabled");
        // Non-fatal: models without skins still work
    }

    // Query the hardware uniform vector budget and warn if PB3D_MAX_BONES may exceed it.
    // Each mat4 bone costs 4 vectors; Pi 5 reports GL_MAX_VERTEX_UNIFORM_VECTORS = 4096,
    // which supports up to 4096 / 4 = 1024 bones matching PB3D_MAX_BONES.
    GLint maxUniformVectors = 0;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &maxUniformVectors);
    pb3dSendConsole("PB3D: GL_MAX_VERTEX_UNIFORM_VECTORS = " + std::to_string(maxUniformVectors)
                    + "  (PB3D_MAX_BONES = " + std::to_string(PB3D_MAX_BONES) + ")", true);
    if (PB3D_MAX_BONES * 4 > maxUniformVectors) {
        pb3dSendConsole("PB3D: WARNING - PB3D_MAX_BONES (" + std::to_string(PB3D_MAX_BONES)
                        + ") exceeds GPU uniform budget! Max safe bone count on this GPU: "
                        + std::to_string(maxUniformVectors / 4)
                        + ". Skinned meshes with more bones will render incorrectly.");
    }

    return true;
}

// ============================================================================
// Model Loading (glTF via cgltf)
// ============================================================================

// Helper: read accessor data into a float buffer
static void cgltf_accessor_read_float_buffer(const cgltf_accessor* accessor, float* outBuffer, size_t floatsPerElement) {
    for (cgltf_size i = 0; i < accessor->count; i++) {
        cgltf_accessor_read_float(accessor, i, outBuffer + i * floatsPerElement, floatsPerElement);
    }
}

unsigned int PB3D::pb3dLoadModel(const char* glbFilePath, bool forceStatic) {
    cgltf_options options = {};
    cgltf_data* data = nullptr;

    cgltf_result result = cgltf_parse_file(&options, glbFilePath, &data);
    if (result != cgltf_result_success) {
        pb3dSendConsole("PB3D: Failed to parse glTF file: " + std::string(glbFilePath)
                        + " (cgltf error " + std::to_string((int)result) + ")");
        return 0;
    }

    result = cgltf_load_buffers(&options, data, glbFilePath);
    if (result != cgltf_result_success) {
        pb3dSendConsole("PB3D: Failed to load glTF buffers: " + std::string(glbFilePath)
                        + " (cgltf error " + std::to_string((int)result) + ")");
        cgltf_free(data);
        return 0;
    }

    st3DModel model;
    model.name = glbFilePath;
    model.isLoaded = true;
    model.hasSkeleton = false;
    model.normScale  = 1.0f;
    model.normCX = model.normCY = model.normCZ = 0.0f;

    // --- Pass 1: compute the combined bounding box over ALL triangle primitives ---
    // Uses accessor-level min/max when available (O(1) per accessor); falls back
    // to scanning vertices when those fields are absent.
    float globalMinX = 1e30f, globalMaxX = -1e30f;
    float globalMinY = 1e30f, globalMaxY = -1e30f;
    float globalMinZ = 1e30f, globalMaxZ = -1e30f;
    for (cgltf_size mi = 0; mi < data->meshes_count; mi++) {
        for (cgltf_size pi = 0; pi < data->meshes[mi].primitives_count; pi++) {
            cgltf_primitive* prim2 = &data->meshes[mi].primitives[pi];
            if (prim2->type != cgltf_primitive_type_triangles) continue;
            for (cgltf_size ai = 0; ai < prim2->attributes_count; ai++) {
                if (prim2->attributes[ai].type != cgltf_attribute_type_position) continue;
                const cgltf_accessor* acc = prim2->attributes[ai].data;
                if (!acc) break;
                if (acc->has_min && acc->has_max) {
                    if ((float)acc->min[0] < globalMinX) globalMinX = (float)acc->min[0];
                    if ((float)acc->max[0] > globalMaxX) globalMaxX = (float)acc->max[0];
                    if ((float)acc->min[1] < globalMinY) globalMinY = (float)acc->min[1];
                    if ((float)acc->max[1] > globalMaxY) globalMaxY = (float)acc->max[1];
                    if ((float)acc->min[2] < globalMinZ) globalMinZ = (float)acc->min[2];
                    if ((float)acc->max[2] > globalMaxZ) globalMaxZ = (float)acc->max[2];
                } else {
                    for (cgltf_size vi = 0; vi < acc->count; vi++) {
                        float p[3];
                        cgltf_accessor_read_float(acc, vi, p, 3);
                        if (p[0] < globalMinX) globalMinX = p[0];
                        if (p[0] > globalMaxX) globalMaxX = p[0];
                        if (p[1] < globalMinY) globalMinY = p[1];
                        if (p[1] > globalMaxY) globalMaxY = p[1];
                        if (p[2] < globalMinZ) globalMinZ = p[2];
                        if (p[2] > globalMaxZ) globalMaxZ = p[2];
                    }
                }
                break;  // only POSITION attribute needed per primitive
            }
        }
    }
    // Guard against empty/degenerate model
    if (globalMinX > globalMaxX) { globalMinX = -1.0f; globalMaxX = 1.0f; }
    if (globalMinY > globalMaxY) { globalMinY = -1.0f; globalMaxY = 1.0f; }
    if (globalMinZ > globalMaxZ) { globalMinZ = -1.0f; globalMaxZ = 1.0f; }
    float normCX = (globalMinX + globalMaxX) * 0.5f;
    float normCY = (globalMinY + globalMaxY) * 0.5f;
    float normCZ = (globalMinZ + globalMaxZ) * 0.5f;
    float normExtX = (globalMaxX - globalMinX) * 0.5f;
    float normExtY = (globalMaxY - globalMinY) * 0.5f;
    float normExtZ = (globalMaxZ - globalMinZ) * 0.5f;
    float maxGlobalExt = normExtX;
    if (normExtY > maxGlobalExt) maxGlobalExt = normExtY;
    if (normExtZ > maxGlobalExt) maxGlobalExt = normExtZ;
    if (maxGlobalExt < 1e-6f) maxGlobalExt = 1.0f;
    float normScale = 1.0f / maxGlobalExt;

    // Store normalization data in the model for the skinned-render MVP correction
    model.normScale = normScale;
    model.normCX    = normCX;
    model.normCY    = normCY;
    model.normCZ    = normCZ;

    // Local texture deduplication cache (cgltf_image* → GPU texture handle).
    // Keyed on image pointer for identity comparison within this load session.
    // nullptr key is reserved for the shared 1×1 white fallback texture.
    std::map<const cgltf_image*, unsigned int> localTexCache;

    // -----------------------------------------------------------------------
    // Multi-skin joint unification: scan nodes to find which skin each mesh
    // uses, then build a unified node→globalBoneIndex map, deduplicating joints
    // that appear in more than one skin.  VBO joint indices are remapped from
    // skin-local to global during Pass 2.  The maps are also used during
    // skeleton/animation loading so animation channels targeting any skin's
    // joints are all correctly captured.
    // -----------------------------------------------------------------------
    std::map<const cgltf_mesh*, const cgltf_skin*> meshToSkin;
    for (cgltf_size ni = 0; ni < data->nodes_count; ni++) {
        const cgltf_node* nd = &data->nodes[ni];
        if (nd->mesh && nd->skin) meshToSkin[nd->mesh] = nd->skin;
    }
    // For each skin, build a local-index→globalBoneIndex vector, deduplicating
    // by cgltf_node* so joints shared across skins occupy a single global slot.
    std::map<const cgltf_node*, int>              nodeToGlobalBone;
    std::map<const cgltf_skin*, std::vector<int>> skinLocalToGlobal;
    for (cgltf_size si = 0; si < data->skins_count; si++) {
        const cgltf_skin* skin = &data->skins[si];
        std::vector<int>& l2g = skinLocalToGlobal[skin];
        l2g.resize(skin->joints_count, 0);
        for (cgltf_size ji = 0; ji < skin->joints_count; ji++) {
            const cgltf_node* jn = skin->joints[ji];
            auto existing = nodeToGlobalBone.find(jn);
            if (existing != nodeToGlobalBone.end()) {
                l2g[ji] = existing->second;  // shared joint — reuse existing global slot
            } else {
                int globalIdx = (int)nodeToGlobalBone.size();
                if (globalIdx < PB3D_MAX_BONES) {
                    nodeToGlobalBone[jn] = globalIdx;
                    l2g[ji] = globalIdx;
                } else {
                    l2g[ji] = PB3D_MAX_BONES - 1;  // overflow: clamp to last slot
                }
            }
        }
    }

    // --- Pass 2: build GPU resources for each triangle primitive ---
    for (cgltf_size mi = 0; mi < data->meshes_count; mi++) {
        cgltf_mesh* mesh = &data->meshes[mi];

        for (cgltf_size pi = 0; pi < mesh->primitives_count; pi++) {
            cgltf_primitive* prim = &mesh->primitives[pi];
            if (prim->type != cgltf_primitive_type_triangles) continue;

            // Find POSITION, NORMAL, TEXCOORD_0, JOINTS_0, WEIGHTS_0 accessors
            const cgltf_accessor* posAccessor    = nullptr;
            const cgltf_accessor* normAccessor   = nullptr;
            const cgltf_accessor* texAccessor    = nullptr;
            const cgltf_accessor* jointsAccessor = nullptr;
            const cgltf_accessor* weightsAccessor= nullptr;

            for (cgltf_size ai = 0; ai < prim->attributes_count; ai++) {
                if (prim->attributes[ai].type == cgltf_attribute_type_position)
                    posAccessor = prim->attributes[ai].data;
                else if (prim->attributes[ai].type == cgltf_attribute_type_normal)
                    normAccessor = prim->attributes[ai].data;
                else if (prim->attributes[ai].type == cgltf_attribute_type_texcoord)
                    texAccessor = prim->attributes[ai].data;
                else if (prim->attributes[ai].type == cgltf_attribute_type_joints)
                    jointsAccessor = prim->attributes[ai].data;
                else if (prim->attributes[ai].type == cgltf_attribute_type_weights)
                    weightsAccessor = prim->attributes[ai].data;
            }

            if (!posAccessor) continue;

            cgltf_size vertexCount = posAccessor->count;

            // Read position data
            std::vector<float> positions(vertexCount * 3, 0.0f);
            cgltf_accessor_read_float_buffer(posAccessor, positions.data(), 3);

            // Read normal data
            std::vector<float> normals(vertexCount * 3, 0.0f);
            if (normAccessor) {
                cgltf_accessor_read_float_buffer(normAccessor, normals.data(), 3);
                // Diagnostic: log a few normals to verify they vary per vertex

            } else {
                // No normals in the file — compute flat (face) normals from triangle geometry.
                // Each triangle's 3 vertices get the same normal = cross(e1, e2).
                // This is far better than a single global (0,1,0) because different faces
                // point in different directions, giving correct per-face diffuse shading.
                pb3dSendConsole("PB3D: WARNING - no normals in model '" + std::string(glbFilePath)
                                + "' (mesh=" + std::to_string(mi)
                                + " prim=" + std::to_string(pi)
                                + "), computing flat face normals");
                if (prim->indices && prim->indices->count >= 3) {
                    cgltf_size triCount = prim->indices->count / 3;
                    bool warnedBadIndex = false;
                    for (cgltf_size t = 0; t < triCount; t++) {
                        unsigned int i0 = (unsigned int)cgltf_accessor_read_index(prim->indices, t*3+0);
                        unsigned int i1 = (unsigned int)cgltf_accessor_read_index(prim->indices, t*3+1);
                        unsigned int i2 = (unsigned int)cgltf_accessor_read_index(prim->indices, t*3+2);
                        // Guard against malformed index data
                        if (i0 >= vertexCount || i1 >= vertexCount || i2 >= vertexCount) {
                            if (!warnedBadIndex) {
                                pb3dSendConsole("PB3D: WARNING - malformed index in '" + std::string(glbFilePath)
                                                + "' (mesh=" + std::to_string(mi)
                                                + " prim=" + std::to_string(pi) + "), skipping bad triangle(s)");
                                warnedBadIndex = true;
                            }
                            continue;
                        }
                        // Edge vectors
                        float e1x = positions[i1*3+0] - positions[i0*3+0];
                        float e1y = positions[i1*3+1] - positions[i0*3+1];
                        float e1z = positions[i1*3+2] - positions[i0*3+2];
                        float e2x = positions[i2*3+0] - positions[i0*3+0];
                        float e2y = positions[i2*3+1] - positions[i0*3+1];
                        float e2z = positions[i2*3+2] - positions[i0*3+2];
                        // Cross product
                        float nx = e1y*e2z - e1z*e2y;
                        float ny = e1z*e2x - e1x*e2z;
                        float nz = e1x*e2y - e1y*e2x;
                        // Normalize
                        float len = sqrtf(nx*nx + ny*ny + nz*nz);
                        if (len > 1e-8f) { nx /= len; ny /= len; nz /= len; }
                        // Assign to all 3 vertices (flat shading per triangle face)
                        for (int vi = 0; vi < 3; vi++) {
                            unsigned int idx = (vi==0)?i0:(vi==1)?i1:i2;
                            normals[idx*3+0] = nx;
                            normals[idx*3+1] = ny;
                            normals[idx*3+2] = nz;
                        }
                    }
                } else {
                    // No index buffer — compute per triangle from sequential vertices
                    for (cgltf_size v = 0; v + 2 < vertexCount; v += 3) {
                        float e1x = positions[(v+1)*3+0] - positions[v*3+0];
                        float e1y = positions[(v+1)*3+1] - positions[v*3+1];
                        float e1z = positions[(v+1)*3+2] - positions[v*3+2];
                        float e2x = positions[(v+2)*3+0] - positions[v*3+0];
                        float e2y = positions[(v+2)*3+1] - positions[v*3+1];
                        float e2z = positions[(v+2)*3+2] - positions[v*3+2];
                        float nx = e1y*e2z - e1z*e2y;
                        float ny = e1z*e2x - e1x*e2z;
                        float nz = e1x*e2y - e1y*e2x;
                        float len = sqrtf(nx*nx + ny*ny + nz*nz);
                        if (len > 1e-8f) { nx /= len; ny /= len; nz /= len; }
                        normals[v*3+0]=nx; normals[v*3+1]=ny; normals[v*3+2]=nz;
                        normals[(v+1)*3+0]=nx; normals[(v+1)*3+1]=ny; normals[(v+1)*3+2]=nz;
                        normals[(v+2)*3+0]=nx; normals[(v+2)*3+1]=ny; normals[(v+2)*3+2]=nz;
                    }
                }
            }

            // Read texcoord data (or default to 0,0)
            std::vector<float> texcoords(vertexCount * 2, 0.0f);
            if (texAccessor) {
                cgltf_accessor_read_float_buffer(texAccessor, texcoords.data(), 2);
            }

            // Read JOINTS_0 and WEIGHTS_0 for skinned meshes
            // forceStatic overrides skin data so all primitives use the static 8-float path.
            bool hasSkinData = !forceStatic && (jointsAccessor != nullptr && weightsAccessor != nullptr);
            std::vector<float> joints(vertexCount * 4, 0.0f);
            std::vector<float> weights(vertexCount * 4, 0.0f);
            if (hasSkinData) {
                // Joints may be UNSIGNED_BYTE or UNSIGNED_SHORT — cgltf_accessor_read_float normalizes
                cgltf_accessor_read_float_buffer(jointsAccessor, joints.data(), 4);
                cgltf_accessor_read_float_buffer(weightsAccessor, weights.data(), 4);
                // Remap skin-local joint indices to unified global bone indices.
                // glTF joint indices are relative to the skin the node uses; we unify
                // all skins into one bone array, so each mesh's indices must be
                // translated to their correct global slot.
                const cgltf_skin* primSkin = nullptr;
                {
                    auto skinIt = meshToSkin.find(mesh);
                    if (skinIt != meshToSkin.end()) primSkin = skinIt->second;
                }
                const std::vector<int>* l2g = nullptr;
                if (primSkin) {
                    auto l2gIt = skinLocalToGlobal.find(primSkin);
                    if (l2gIt != skinLocalToGlobal.end()) l2g = &l2gIt->second;
                }
                for (cgltf_size v = 0; v < vertexCount; v++) {
                    for (int c = 0; c < 4; c++) {
                        int localIdx = (int)joints[v*4+c];
                        int globalIdx = (l2g && localIdx < (int)l2g->size())
                                        ? (*l2g)[localIdx] : localIdx;
                        if (globalIdx < 0 || globalIdx >= PB3D_MAX_BONES)
                            globalIdx = PB3D_MAX_BONES - 1;
                        joints[v*4+c] = (float)globalIdx;
                    }
                }
            }

            // Skip unskinned primitives in a model that has skeleton/skin data.
            // Such primitives sit at their static model-space position regardless of
            // bone animation — at rest they may hide inside the body, but once bones
            // move they pop out as floating artifacts (e.g. white glass panel).
            // Mesh nodes that genuinely have no skin reference and need to render
            // as static props should be in a separate model file.
            // Use forceStatic=true to load everything without animation.
            if (!forceStatic && !hasSkinData && data->skins_count > 0) {
                pb3dSendConsole("PB3D: WARNING - Mesh[" + std::to_string(mi) + "] Prim[" + std::to_string(pi) + "]"
                    + " '" + (mesh->name ? std::string(mesh->name) : "?") + "'"
                    + " has no skin weights in a skinned model - skipped");
                continue;
            }

            // Vertex positions are stored as raw model-space coordinates.
            // The model matrix at render time folds in normScale and normCenter
            // so IBMs work correctly in the skinned path.

            // Create GPU resources via PBOGLES (no direct GL calls in PB3D)
            st3DMesh gpuMesh = {};
            gpuMesh.isSkinned         = hasSkinData;
            gpuMesh.needsBlend        = false;
            gpuMesh.materialBaseAlpha = 1.0f;
            if (prim->material) {
                cgltf_alpha_mode am = prim->material->alpha_mode;
                gpuMesh.needsBlend = (am == cgltf_alpha_mode_blend ||
                                      am == cgltf_alpha_mode_mask);
                if (prim->material->has_pbr_metallic_roughness) {
                    // Read base_color_factor alpha — often < 1 for transparent/glass materials
                    gpuMesh.materialBaseAlpha =
                        prim->material->pbr_metallic_roughness.base_color_factor[3];
                }
            }

            if (hasSkinData) {
                // Skinned layout: [posX, posY, posZ, normX, normY, normZ, u, v, j0, j1, j2, j3, w0, w1, w2, w3]
                std::vector<float> interleavedData(vertexCount * 16);
                for (cgltf_size v = 0; v < vertexCount; v++) {
                    interleavedData[v*16+ 0] = positions[v*3+0];
                    interleavedData[v*16+ 1] = positions[v*3+1];
                    interleavedData[v*16+ 2] = positions[v*3+2];
                    interleavedData[v*16+ 3] = normals[v*3+0];
                    interleavedData[v*16+ 4] = normals[v*3+1];
                    interleavedData[v*16+ 5] = normals[v*3+2];
                    interleavedData[v*16+ 6] = texcoords[v*2+0];
                    interleavedData[v*16+ 7] = texcoords[v*2+1];
                    interleavedData[v*16+ 8] = joints[v*4+0];
                    interleavedData[v*16+ 9] = joints[v*4+1];
                    interleavedData[v*16+10] = joints[v*4+2];
                    interleavedData[v*16+11] = joints[v*4+3];
                    interleavedData[v*16+12] = weights[v*4+0];
                    interleavedData[v*16+13] = weights[v*4+1];
                    interleavedData[v*16+14] = weights[v*4+2];
                    interleavedData[v*16+15] = weights[v*4+3];
                }

                // Read index data
                std::vector<unsigned int> indices;
                if (prim->indices) {
                    indices.resize(prim->indices->count);
                    for (cgltf_size ii = 0; ii < prim->indices->count; ii++)
                        indices[ii] = (unsigned int)cgltf_accessor_read_index(prim->indices, ii);
                } else {
                    indices.resize(vertexCount);
                    for (cgltf_size ii = 0; ii < vertexCount; ii++) indices[ii] = (unsigned int)ii;
                }

                if (!ogl3dCreateSkinnedMesh(interleavedData.data(), interleavedData.size(),
                                       indices.data(), indices.size(),
                                       gpuMesh.vao, gpuMesh.vboVertices, gpuMesh.eboIndices)) {
                    pb3dSendConsole("PB3D: ERROR - GPU skinned mesh creation failed (out of GPU memory?)");
                    continue;
                }
                gpuMesh.indexCount = (unsigned int)indices.size();
            } else {
                // Static layout: [posX, posY, posZ, normX, normY, normZ, u, v]
                std::vector<float> interleavedData(vertexCount * 8);
                for (cgltf_size v = 0; v < vertexCount; v++) {
                    interleavedData[v*8+0] = positions[v*3+0];
                    interleavedData[v*8+1] = positions[v*3+1];
                    interleavedData[v*8+2] = positions[v*3+2];
                    interleavedData[v*8+3] = normals[v*3+0];
                    interleavedData[v*8+4] = normals[v*3+1];
                    interleavedData[v*8+5] = normals[v*3+2];
                    interleavedData[v*8+6] = texcoords[v*2+0];
                    interleavedData[v*8+7] = texcoords[v*2+1];
                }

                // Read index data
                std::vector<unsigned int> indices;
                if (prim->indices) {
                    indices.resize(prim->indices->count);
                    for (cgltf_size ii = 0; ii < prim->indices->count; ii++)
                        indices[ii] = (unsigned int)cgltf_accessor_read_index(prim->indices, ii);
                } else {
                    indices.resize(vertexCount);
                    for (cgltf_size ii = 0; ii < vertexCount; ii++) indices[ii] = (unsigned int)ii;
                }

                if (!ogl3dCreateMesh(interleavedData.data(), interleavedData.size(),
                                indices.data(), indices.size(),
                                gpuMesh.vao, gpuMesh.vboVertices, gpuMesh.eboIndices)) {
                    pb3dSendConsole("PB3D: ERROR - GPU mesh creation failed (out of GPU memory?)");
                    continue;
                }
                gpuMesh.indexCount = (unsigned int)indices.size();
            }

            // Load texture from material — deduplicate via localTexCache so primitives
            // sharing the same cgltf_image get the same GPU texture handle.
            gpuMesh.textureId = 0;
            if (prim->material && prim->material->has_pbr_metallic_roughness) {
                cgltf_texture* tex = prim->material->pbr_metallic_roughness.base_color_texture.texture;
                if (tex && tex->image) {
                    cgltf_image* img = tex->image;
                    auto cacheIt = localTexCache.find(img);
                    if (cacheIt != localTexCache.end()) {
                        // Reuse already-uploaded texture
                        gpuMesh.textureId = cacheIt->second;
                    } else if (img->buffer_view && img->buffer_view->buffer) {
                        // Embedded image in GLB binary buffer — upload once
                        const uint8_t* imgData = (const uint8_t*)img->buffer_view->buffer->data + img->buffer_view->offset;
                        int imgSize = (int)img->buffer_view->size;
                        int texW, texH, texC;
                        unsigned char* pixels = stbi_load_from_memory(imgData, imgSize, &texW, &texH, &texC, STBI_rgb_alpha);
                        if (pixels) {
                            unsigned int texId = ogl3dCreateTexture(pixels, texW, texH);
                            stbi_image_free(pixels);
                            gpuMesh.textureId = texId;
                            localTexCache[img] = texId;
                            model.ownedTextures.insert(texId);
                        }
                    }
                }
            }

            // Fallback: shared 1×1 white texture (cached as nullptr key)
            if (gpuMesh.textureId == 0) {
                auto fbIt = localTexCache.find(nullptr);
                if (fbIt != localTexCache.end()) {
                    gpuMesh.textureId = fbIt->second;
                } else {
                    unsigned int fallbackTex = ogl3dCreateFallbackTexture();
                    gpuMesh.textureId = fallbackTex;
                    localTexCache[nullptr] = fallbackTex;
                    model.ownedTextures.insert(fallbackTex);
                }
            }

            // Diagnostic: confirm each primitive loaded and surface any material issues.
            {
                // True when the material has a PBR base-color texture that was actually loaded.
                bool hasRealTex = (prim->material
                                && prim->material->has_pbr_metallic_roughness
                                && prim->material->pbr_metallic_roughness.base_color_texture.texture != nullptr
                                && prim->material->pbr_metallic_roughness.base_color_texture.texture->image != nullptr);
                std::string matName = (prim->material && prim->material->name)
                                    ? std::string(prim->material->name) : "(no material)";
                // Note when the material base_color_factor carries an alpha the shader doesn't apply.
                // Alpha transparency must come from the texture's alpha channel.
                if (prim->material && prim->material->has_pbr_metallic_roughness) {
                    const float* bcf = prim->material->pbr_metallic_roughness.base_color_factor;
                    if (bcf[3] < 0.99f) {
                        pb3dSendConsole("PB3D:   NOTE - Mesh[" + std::to_string(mi) + "] mat '" + matName
                                        + "' base_color_factor alpha=" + std::to_string(bcf[3])
                                        + " (not applied by shader; use texture alpha)", true);
                    }
                }
                pb3dSendConsole("PB3D:   Mesh[" + std::to_string(mi) + "] Prim[" + std::to_string(pi) + "]"
                                + " '" + (mesh->name ? std::string(mesh->name) : "?") + "'"
                                + " v=" + std::to_string(vertexCount)
                                + " idx=" + std::to_string(gpuMesh.indexCount)
                                + (gpuMesh.isSkinned ? " [SKINNED]" : " [STATIC]")
                                + " tex=" + std::to_string(gpuMesh.textureId) + (hasRealTex ? "" : " [FALLBACK]")
                                + " mat='" + matName + "'"
                                + (gpuMesh.needsBlend ? " [BLEND]" : " [OPAQUE]"), true);
                // Always-visible warning: skinned primitive with no real texture will render as solid white.
                // This identifies the source mesh for "white poly" artifacts in animated models.
                if (gpuMesh.isSkinned && !hasRealTex) {
                    pb3dSendConsole("PB3D: WARNING - Mesh[" + std::to_string(mi) + "] Prim[" + std::to_string(pi) + "]"
                                    + " '" + (mesh->name ? std::string(mesh->name) : "?") + "'"
                                    + " is SKINNED with no texture (mat='" + matName + "',"
                                    + " v=" + std::to_string(vertexCount) + ") - will render white");
                }
            }

            model.meshes.push_back(gpuMesh);
        }
    }

    // Sort mesh list so opaque primitives render before transparent ones.
    // Transparent geometry must composite over fully-rendered opaque surfaces;
    // drawing it first produces incorrect blending against whatever is already
    // in the framebuffer rather than against the finished opaque geometry.
    std::stable_sort(model.meshes.begin(), model.meshes.end(),
        [](const st3DMesh& a, const st3DMesh& b) {
            bool aBlend = a.needsBlend || (a.materialBaseAlpha < 0.999f);
            bool bBlend = b.needsBlend || (b.materialBaseAlpha < 0.999f);
            return !aBlend && bBlend;  // opaque (false) sorts before transparent (true)
        });

    pb3dSendConsole("PB3D: '" + std::string(glbFilePath) + "' loaded: "
                    + std::to_string(model.meshes.size()) + " mesh primitive(s), "
                    + std::to_string(model.ownedTextures.size()) + " unique texture(s)", true);

    // -----------------------------------------------------------------------
    // Skeleton loading: unify ALL skins into a single bone array using the
    // nodeToGlobalBone map built in the pre-pass above.  Animation channels
    // from all clips are mapped via the same unified index so every skin's
    // joints are animated correctly.
    //
    // New approach (generic glTF spec-compliant):
    //  1. IBMs are used AS-IS from the file (no correction applied).
    //  2. Per-skin meshNodeGlobalInv = inv(meshNode world transform) is stored
    //     and multiplied into the final skinning matrix at animation time.
    //  3. Non-joint ancestor nodes that have animation channels are promoted
    //     to "virtual bones" so their animation propagates to children.
    //  4. parentOffsetMatrix captures intermediate non-joint nodes between
    //     a bone and its parent in the unified hierarchy (walked fully to
    //     scene root — no early stop at non-unit-scale nodes).
    // -----------------------------------------------------------------------
    if (data->skins_count > 0) {
        // --- Step 1: Compute meshNodeGlobalInv for each skin ---
        model.skeleton.skinCount = (int)data->skins_count;
        if (model.skeleton.skinCount > PB3D_MAX_SKINS)
            model.skeleton.skinCount = PB3D_MAX_SKINS;
        for (int si2 = 0; si2 < PB3D_MAX_SKINS; si2++) {
            float* m = model.skeleton.meshNodeGlobalInv[si2];
            memset(m, 0, 64);
            m[0] = m[5] = m[10] = m[15] = 1.0f;  // default: identity
        }
        for (cgltf_size si = 0; si < data->skins_count && (int)si < PB3D_MAX_SKINS; si++) {
            const cgltf_skin* skin = &data->skins[si];
            // Find the mesh node that references this skin
            for (cgltf_size ni = 0; ni < data->nodes_count; ni++) {
                const cgltf_node* nd = &data->nodes[ni];
                if (nd->skin != skin || !nd->mesh) continue;
                float meshGlobal[16];
                cgltf_node_transform_world(nd, meshGlobal);
                if (!pb3dMat4InvertAffine(meshGlobal, model.skeleton.meshNodeGlobalInv[si])) {
                    pb3dSendConsole("PB3D: WARNING — Skin[" + std::to_string(si)
                        + "] mesh node '" + std::string(nd->name ? nd->name : "?")
                        + "' has singular world transform; using identity");
                }
                break;  // first mesh node for this skin
            }
        }

        // --- Step 2: Pre-scan animations for non-joint ancestor nodes (virtual bones) ---
        // Build a set of all nodes that ARE ancestors of any joint and that have
        // animation channels, then add them to nodeToGlobalBone as virtual bones.
        {
            // Collect all joint nodes for fast ancestry checking
            std::set<const cgltf_node*> jointNodes;
            for (auto& kv : nodeToGlobalBone) jointNodes.insert(kv.first);

            // Collect all animated nodes (nodes targeted by any animation channel)
            std::set<const cgltf_node*> animatedNodes;
            for (cgltf_size ai = 0; ai < data->animations_count; ai++) {
                const cgltf_animation* anim = &data->animations[ai];
                for (cgltf_size ci = 0; ci < anim->channels_count; ci++) {
                    if (anim->channels[ci].target_node)
                        animatedNodes.insert(anim->channels[ci].target_node);
                }
            }

            // For each animated node not already in nodeToGlobalBone, check if it's
            // an ancestor of any joint.  An ancestor is found by walking UP from every
            // joint — but it's more efficient to walk UP from the animated node and
            // see if any joint is a descendant.  However, glTF only stores parent
            // pointers, so we check by walking up from each joint to see if the
            // animated node appears in its ancestor chain.
            std::set<const cgltf_node*> virtualBoneNodes;
            for (const cgltf_node* animNode : animatedNodes) {
                if (nodeToGlobalBone.count(animNode)) continue;  // already a joint
                // Check: is animNode an ancestor of any joint?
                bool isAncestor = false;
                for (const cgltf_node* jn : jointNodes) {
                    const cgltf_node* cur = jn->parent;
                    while (cur != nullptr) {
                        if (cur == animNode) { isAncestor = true; break; }
                        cur = cur->parent;
                    }
                    if (isAncestor) break;
                }
                if (isAncestor) virtualBoneNodes.insert(animNode);
            }

            // Add virtual bones to nodeToGlobalBone
            for (const cgltf_node* vbn : virtualBoneNodes) {
                int globalIdx = (int)nodeToGlobalBone.size();
                if (globalIdx >= PB3D_MAX_BONES) {
                    pb3dSendConsole("PB3D: WARNING — virtual bone '"
                        + std::string(vbn->name ? vbn->name : "?")
                        + "' exceeds PB3D_MAX_BONES, skipping");
                    continue;
                }
                nodeToGlobalBone[vbn] = globalIdx;
                pb3dSendConsole("PB3D: Virtual bone[" + std::to_string(globalIdx) + "] '"
                    + std::string(vbn->name ? vbn->name : "?") + "' (animated ancestor)", true);
            }
        }

        int totalBones = (int)nodeToGlobalBone.size();
        if (totalBones > PB3D_MAX_BONES) totalBones = PB3D_MAX_BONES;
        pb3dSendConsole("PB3D: Loading " + std::to_string(data->skins_count) + " skin(s), "
                        + std::to_string(totalBones) + " bones (incl. virtual) from: "
                        + std::string(glbFilePath), true);

        model.skeleton.bones.resize(totalBones);

        // --- Step 3: Fill bone data from each skin's joints ---
        for (cgltf_size si = 0; si < data->skins_count; si++) {
            const cgltf_skin* skin = &data->skins[si];
            const std::vector<int>& l2g = skinLocalToGlobal.at(skin);

            // Read inverse bind matrices for this skin (used AS-IS, no correction)
            std::vector<float> ibmBuffer;
            if (skin->inverse_bind_matrices) {
                cgltf_size ibmCount = skin->inverse_bind_matrices->count;
                ibmBuffer.resize(ibmCount * 16, 0.0f);
                for (cgltf_size ji = 0; ji < ibmCount; ji++) {
                    cgltf_accessor_read_float(skin->inverse_bind_matrices, ji,
                                              ibmBuffer.data() + ji * 16, 16);
                }
            }

            for (cgltf_size ji = 0; ji < skin->joints_count; ji++) {
                int globalIdx = l2g[ji];
                if (globalIdx < 0 || globalIdx >= totalBones) continue;

                st3DBone& bone = model.skeleton.bones[globalIdx];
                if (!bone.name.empty()) continue;  // already filled by an earlier skin (shared joint)

                const cgltf_node* jn = skin->joints[ji];
                bone.name = jn->name ? jn->name : ("bone_" + std::to_string(globalIdx));
                bone.skinIndex = (int)si;

                // Parent index — look up through the unified map (includes virtual bones)
                bone.parentIndex = -1;
                if (jn->parent) {
                    auto parentIt = nodeToGlobalBone.find(jn->parent);
                    if (parentIt != nodeToGlobalBone.end())
                        bone.parentIndex = parentIt->second;
                }

                // parentOffsetMatrix: compose the LOCAL TRS of any non-joint nodes that
                // sit between this bone and its nearest registered ancestor (joint or
                // virtual bone) in the unified hierarchy.
                //
                // Walk up from the glTF parent node.  For each node:
                //  - If it's in nodeToGlobalBone: stop, re-parent to it.
                //  - Otherwise: prepend its local transform into chainMat and continue up.
                //  - Stop if we hit a node with non-unit scale (exporter scene-root with
                //    unit-conversion scale like 0.0075).  IBMs are computed by the exporter
                //    WITHOUT this scale, so we must not include it.
                {
                    memset(bone.parentOffsetMatrix, 0, 64);
                    bone.parentOffsetMatrix[0]  = 1.0f;
                    bone.parentOffsetMatrix[5]  = 1.0f;
                    bone.parentOffsetMatrix[10] = 1.0f;
                    bone.parentOffsetMatrix[15] = 1.0f;

                    if (bone.parentIndex < 0 && jn->parent != nullptr) {
                        const cgltf_node* cur = jn->parent;
                        int jointAncIdx = -1;
                        float chainMat[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
                        float tmp[16];
                        while (cur != nullptr) {
                            auto it = nodeToGlobalBone.find(cur);
                            if (it != nodeToGlobalBone.end()) {
                                jointAncIdx = it->second;
                                break;
                            }
                            // Stop at non-unit-scale nodes (exporter scene root)
                            bool hasNonUnitScale = cur->has_scale &&
                                (fabsf(cur->scale[0] - 1.0f) > 0.001f ||
                                 fabsf(cur->scale[1] - 1.0f) > 0.001f ||
                                 fabsf(cur->scale[2] - 1.0f) > 0.001f);
                            if (hasNonUnitScale) break;
                            // Prepend this node's local transform: chainMat = curLocal × chainMat
                            cgltf_node_transform_local(cur, tmp);
                            float newChain[16];
                            pb3dMat4Mul(tmp, chainMat, newChain);
                            memcpy(chainMat, newChain, 64);
                            cur = cur->parent;
                        }
                        if (jointAncIdx >= 0) bone.parentIndex = jointAncIdx;
                        memcpy(bone.parentOffsetMatrix, chainMat, 64);
                    }
                }

                // Inverse bind matrix (used AS-IS from glTF)
                if (!ibmBuffer.empty() && ji < ibmBuffer.size() / 16) {
                    memcpy(bone.inverseBindMatrix, ibmBuffer.data() + ji * 16, 64);
                } else {
                    memset(bone.inverseBindMatrix, 0, 64);
                    bone.inverseBindMatrix[0]  = 1.0f;
                    bone.inverseBindMatrix[5]  = 1.0f;
                    bone.inverseBindMatrix[10] = 1.0f;
                    bone.inverseBindMatrix[15] = 1.0f;
                }

                // Rest pose TRS from glTF node
                if (jn->has_matrix && !jn->has_translation && !jn->has_rotation && !jn->has_scale) {
                    // Matrix-only node: decompose into TRS for animation system
                    float localMat[16];
                    cgltf_node_transform_local(jn, localMat);
                    // Extract translation from column 3
                    bone.restTranslation[0] = localMat[12];
                    bone.restTranslation[1] = localMat[13];
                    bone.restTranslation[2] = localMat[14];
                    // Extract scale from column magnitudes
                    bone.restScale[0] = sqrtf(localMat[0]*localMat[0] + localMat[1]*localMat[1] + localMat[2]*localMat[2]);
                    bone.restScale[1] = sqrtf(localMat[4]*localMat[4] + localMat[5]*localMat[5] + localMat[6]*localMat[6]);
                    bone.restScale[2] = sqrtf(localMat[8]*localMat[8] + localMat[9]*localMat[9] + localMat[10]*localMat[10]);
                    // Extract rotation: normalize the 3x3 columns, then convert to quaternion
                    float is0 = (bone.restScale[0] > 1e-8f) ? 1.0f/bone.restScale[0] : 0.0f;
                    float is1 = (bone.restScale[1] > 1e-8f) ? 1.0f/bone.restScale[1] : 0.0f;
                    float is2 = (bone.restScale[2] > 1e-8f) ? 1.0f/bone.restScale[2] : 0.0f;
                    float r00=localMat[0]*is0, r10=localMat[1]*is0, r20=localMat[2]*is0;
                    float r01=localMat[4]*is1, r11=localMat[5]*is1, r21=localMat[6]*is1;
                    float r02=localMat[8]*is2, r12=localMat[9]*is2, r22=localMat[10]*is2;
                    float trace = r00 + r11 + r22;
                    if (trace > 0.0f) {
                        float s = sqrtf(trace + 1.0f) * 2.0f;
                        bone.restRotation[3] = 0.25f * s;
                        bone.restRotation[0] = (r21 - r12) / s;
                        bone.restRotation[1] = (r02 - r20) / s;
                        bone.restRotation[2] = (r10 - r01) / s;
                    } else if (r00 > r11 && r00 > r22) {
                        float s = sqrtf(1.0f + r00 - r11 - r22) * 2.0f;
                        bone.restRotation[3] = (r21 - r12) / s;
                        bone.restRotation[0] = 0.25f * s;
                        bone.restRotation[1] = (r01 + r10) / s;
                        bone.restRotation[2] = (r02 + r20) / s;
                    } else if (r11 > r22) {
                        float s = sqrtf(1.0f + r11 - r00 - r22) * 2.0f;
                        bone.restRotation[3] = (r02 - r20) / s;
                        bone.restRotation[0] = (r01 + r10) / s;
                        bone.restRotation[1] = 0.25f * s;
                        bone.restRotation[2] = (r12 + r21) / s;
                    } else {
                        float s = sqrtf(1.0f + r22 - r00 - r11) * 2.0f;
                        bone.restRotation[3] = (r10 - r01) / s;
                        bone.restRotation[0] = (r02 + r20) / s;
                        bone.restRotation[1] = (r12 + r21) / s;
                        bone.restRotation[2] = 0.25f * s;
                    }
                } else {
                    // Standard TRS extraction
                    if (jn->has_translation) {
                        bone.restTranslation[0] = jn->translation[0];
                        bone.restTranslation[1] = jn->translation[1];
                        bone.restTranslation[2] = jn->translation[2];
                    } else {
                        bone.restTranslation[0] = bone.restTranslation[1] = bone.restTranslation[2] = 0.0f;
                    }
                    if (jn->has_rotation) {
                        bone.restRotation[0] = jn->rotation[0];
                        bone.restRotation[1] = jn->rotation[1];
                        bone.restRotation[2] = jn->rotation[2];
                        bone.restRotation[3] = jn->rotation[3];
                    } else {
                        bone.restRotation[0] = bone.restRotation[1] = bone.restRotation[2] = 0.0f;
                        bone.restRotation[3] = 1.0f;
                    }
                    if (jn->has_scale) {
                        bone.restScale[0] = jn->scale[0];
                        bone.restScale[1] = jn->scale[1];
                        bone.restScale[2] = jn->scale[2];
                    } else {
                        bone.restScale[0] = bone.restScale[1] = bone.restScale[2] = 1.0f;
                    }
                }
            }
        }

        // --- Step 4: Fill virtual bone data ---
        // Virtual bones are in nodeToGlobalBone but not in any skin's joints array.
        // They have globalIdx >= original joint count.
        for (auto& kv : nodeToGlobalBone) {
            const cgltf_node* vbn = kv.first;
            int globalIdx = kv.second;
            if (globalIdx < 0 || globalIdx >= totalBones) continue;
            st3DBone& bone = model.skeleton.bones[globalIdx];
            if (!bone.name.empty()) continue;  // already filled (was a real joint)

            bone.name = vbn->name ? vbn->name : ("vbone_" + std::to_string(globalIdx));
            bone.skinIndex = -1;  // not associated with any skin

            // Identity IBM (virtual bones don't receive vertex weights)
            memset(bone.inverseBindMatrix, 0, 64);
            bone.inverseBindMatrix[0]  = 1.0f;
            bone.inverseBindMatrix[5]  = 1.0f;
            bone.inverseBindMatrix[10] = 1.0f;
            bone.inverseBindMatrix[15] = 1.0f;

            // Parent index — walk up to find nearest registered ancestor
            bone.parentIndex = -1;
            {
                memset(bone.parentOffsetMatrix, 0, 64);
                bone.parentOffsetMatrix[0]  = 1.0f;
                bone.parentOffsetMatrix[5]  = 1.0f;
                bone.parentOffsetMatrix[10] = 1.0f;
                bone.parentOffsetMatrix[15] = 1.0f;

                if (vbn->parent != nullptr) {
                    auto directParent = nodeToGlobalBone.find(vbn->parent);
                    if (directParent != nodeToGlobalBone.end()) {
                        bone.parentIndex = directParent->second;
                    } else {
                        // Walk up collecting non-registered locals
                        const cgltf_node* cur = vbn->parent;
                        float chainMat[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
                        float tmp[16];
                        while (cur != nullptr) {
                            auto it = nodeToGlobalBone.find(cur);
                            if (it != nodeToGlobalBone.end()) {
                                bone.parentIndex = it->second;
                                break;
                            }
                            // Stop at non-unit-scale nodes (exporter scene root)
                            bool hasNonUnitScale = cur->has_scale &&
                                (fabsf(cur->scale[0] - 1.0f) > 0.001f ||
                                 fabsf(cur->scale[1] - 1.0f) > 0.001f ||
                                 fabsf(cur->scale[2] - 1.0f) > 0.001f);
                            if (hasNonUnitScale) break;
                            cgltf_node_transform_local(cur, tmp);
                            float newChain[16];
                            pb3dMat4Mul(tmp, chainMat, newChain);
                            memcpy(chainMat, newChain, 64);
                            cur = cur->parent;
                        }
                        memcpy(bone.parentOffsetMatrix, chainMat, 64);
                    }
                }
            }

            // Rest pose TRS
            if (vbn->has_translation) {
                bone.restTranslation[0] = vbn->translation[0];
                bone.restTranslation[1] = vbn->translation[1];
                bone.restTranslation[2] = vbn->translation[2];
            } else {
                bone.restTranslation[0] = bone.restTranslation[1] = bone.restTranslation[2] = 0.0f;
            }
            if (vbn->has_rotation) {
                bone.restRotation[0] = vbn->rotation[0];
                bone.restRotation[1] = vbn->rotation[1];
                bone.restRotation[2] = vbn->rotation[2];
                bone.restRotation[3] = vbn->rotation[3];
            } else {
                bone.restRotation[0] = bone.restRotation[1] = bone.restRotation[2] = 0.0f;
                bone.restRotation[3] = 1.0f;
            }
            if (vbn->has_scale) {
                bone.restScale[0] = vbn->scale[0];
                bone.restScale[1] = vbn->scale[1];
                bone.restScale[2] = vbn->scale[2];
            } else {
                bone.restScale[0] = bone.restScale[1] = bone.restScale[2] = 1.0f;
            }
        }

        // Load animation clips — nodeToGlobalBone now includes virtual bones,
        // so channels targeting non-joint ancestor nodes are captured.
        for (cgltf_size ai = 0; ai < data->animations_count; ai++) {
            const cgltf_animation* anim = &data->animations[ai];
            st3DAnimClip clip;
            clip.name     = anim->name ? anim->name : ("clip_" + std::to_string(ai));
            clip.duration = 0.0f;

            for (cgltf_size ci = 0; ci < anim->channels_count; ci++) {
                const cgltf_animation_channel* ch   = &anim->channels[ci];
                const cgltf_animation_sampler*  smp = ch->sampler;
                if (!ch->target_node || !smp || !smp->input || !smp->output) continue;

                auto jointIt = nodeToGlobalBone.find(ch->target_node);
                if (jointIt == nodeToGlobalBone.end()) continue;

                int boneIdx = jointIt->second;
                if (boneIdx >= PB3D_MAX_BONES) continue;

                st3DAnimChannel channel;
                channel.boneIndex = boneIdx;

                switch (ch->target_path) {
                    case cgltf_animation_path_type_translation:
                        channel.type = ANIM_CHANNEL_TRANSLATION; break;
                    case cgltf_animation_path_type_rotation:
                        channel.type = ANIM_CHANNEL_ROTATION;    break;
                    case cgltf_animation_path_type_scale:
                        channel.type = ANIM_CHANNEL_SCALE;       break;
                    default: continue;
                }

                switch (smp->interpolation) {
                    case cgltf_interpolation_type_step:
                        channel.interpolation = ANIM_INTERP_STEP;        break;
                    case cgltf_interpolation_type_cubic_spline:
                        channel.interpolation = ANIM_INTERP_CUBICSPLINE; break;
                    default:
                        channel.interpolation = ANIM_INTERP_LINEAR;      break;
                }

                cgltf_size kfCount = smp->input->count;
                int valComponents  = (channel.type == ANIM_CHANNEL_ROTATION) ? 4 : 3;
                int stride = (channel.interpolation == ANIM_INTERP_CUBICSPLINE) ? 3 : 1;

                for (cgltf_size ki = 0; ki < kfCount; ki++) {
                    st3DAnimKeyframe kf;
                    cgltf_accessor_read_float(smp->input, ki, &kf.time, 1);
                    if (kf.time > clip.duration) clip.duration = kf.time;

                    cgltf_size outIdx = (channel.interpolation == ANIM_INTERP_CUBICSPLINE)
                                       ? ((cgltf_size)stride * ki + 1) : ki;
                    kf.value[3] = 0.0f;
                    cgltf_accessor_read_float(smp->output, outIdx,
                                              kf.value, (cgltf_size)valComponents);
                    channel.keyframes.push_back(kf);
                }

                if (!channel.keyframes.empty()) {
                    clip.channels.push_back(channel);
                }
            }

            if (!clip.channels.empty()) {
                model.skeleton.clips.push_back(clip);
            }
        }

        // Bind-pose verification: for each bone compute
        //   skinMatrix = meshNodeGlobalInv[skinIdx] × worldMatrix(restTRS) × IBM
        // and verify it approximates identity.  Virtual bones (skinIndex=-1) are
        // skipped since they have identity IBMs and no skin association.
        {
            const st3DSkeleton& vskel = model.skeleton;
            int nbv = (int)vskel.bones.size();
            if (nbv > 0 && nbv <= PB3D_MAX_BONES) {
                float vlocal[PB3D_MAX_BONES * 16];
                float vworld[PB3D_MAX_BONES * 16];
                for (int bi = 0; bi < nbv; bi++) {
                    const st3DBone& b = vskel.bones[bi];
                    pb3dMat4FromTRS(b.restTranslation, b.restRotation, b.restScale,
                                    vlocal + bi * 16);
                }
                // Topo sort (same logic as pb3dComputeBoneMatrices)
                int  vord[PB3D_MAX_BONES];  int vcnt = 0;
                bool vvis[PB3D_MAX_BONES] = {};
                bool vstk[PB3D_MAX_BONES] = {};
                int  vds[PB3D_MAX_BONES];   int vdt = 0;
                for (int root = 0; root < nbv; root++) {
                    if (vvis[root]) continue;
                    vds[vdt++] = root;  vstk[root] = true;
                    while (vdt > 0) {
                        int bi  = vds[vdt-1];
                        int par = vskel.bones[bi].parentIndex;
                        if (par >= 0 && par < nbv && !vvis[par] && !vstk[par]) {
                            vds[vdt++] = par;  vstk[par] = true;
                        } else {
                            vdt--;
                            if (!vvis[bi]) { vvis[bi] = true; vord[vcnt++] = bi; }
                        }
                    }
                }
                float vtmp[16];
                for (int ti = 0; ti < vcnt; ti++) {
                    int bi  = vord[ti];
                    int par = vskel.bones[bi].parentIndex;
                    if (par < 0 || par >= nbv) {
                        pb3dMat4Mul(vskel.bones[bi].parentOffsetMatrix,
                                    vlocal + bi*16, vworld + bi*16);
                    } else {
                        pb3dMat4Mul(vworld + par*16,
                                    vskel.bones[bi].parentOffsetMatrix, vtmp);
                        pb3dMat4Mul(vtmp, vlocal + bi*16, vworld + bi*16);
                    }
                }
                int badCnt = 0;
                int checkedCnt = 0;
                for (int bi = 0; bi < nbv; bi++) {
                    // Skip virtual bones (skinIndex == -1) — they have identity IBMs
                    if (vskel.bones[bi].skinIndex < 0) continue;
                    checkedCnt++;
                    // skinMatrix = meshNodeGlobalInv × worldMatrix × IBM
                    int si = vskel.bones[bi].skinIndex;
                    float tmp1[16], sm[16];
                    pb3dMat4Mul(vworld + bi*16, vskel.bones[bi].inverseBindMatrix, tmp1);
                    if (si >= 0 && si < vskel.skinCount) {
                        pb3dMat4Mul(vskel.meshNodeGlobalInv[si], tmp1, sm);
                    } else {
                        memcpy(sm, tmp1, 64);
                    }
                    bool ok = fabsf(sm[0]-1.f)<0.02f && fabsf(sm[5]-1.f)<0.02f &&
                              fabsf(sm[10]-1.f)<0.02f && fabsf(sm[15]-1.f)<0.02f &&
                              fabsf(sm[12])<0.02f && fabsf(sm[13])<0.02f && fabsf(sm[14])<0.02f;
                    if (!ok) {
                        badCnt++;
                        if (badCnt <= 8) {
                            pb3dSendConsole("PB3D BINDWARN bone[" + std::to_string(bi) + "] \""
                                + vskel.bones[bi].name + "\" diag=("
                                + std::to_string(sm[0]).substr(0,6) + ","
                                + std::to_string(sm[5]).substr(0,6) + ","
                                + std::to_string(sm[10]).substr(0,6) + ") t=("
                                + std::to_string(sm[12]).substr(0,5) + ","
                                + std::to_string(sm[13]).substr(0,5) + ","
                                + std::to_string(sm[14]).substr(0,5) + ")", true);
                        }
                    }
                }
                if (badCnt > 0)
                    pb3dSendConsole("PB3D: " + std::to_string(badCnt) + "/"
                        + std::to_string(checkedCnt) + " bones: non-identity bind-pose skinMatrix", true);
                else
                    pb3dSendConsole("PB3D: bind-pose verification OK ("
                        + std::to_string(checkedCnt) + " bones, "
                        + std::to_string(nbv - checkedCnt) + " virtual)", true);

            }
        }

        model.hasSkeleton = !model.skeleton.bones.empty();
        if (model.hasSkeleton) {
            pb3dSendConsole("PB3D: Skeleton loaded: "
                            + std::to_string(model.skeleton.bones.size()) + " bones (unified from "
                            + std::to_string(data->skins_count) + " skin(s)), "
                            + std::to_string(model.skeleton.clips.size()) + " clips from: "
                            + std::string(glbFilePath), true);
        }
    }

    cgltf_free(data);

    if (model.meshes.empty()) {
        pb3dSendConsole("PB3D: No meshes found in: " + std::string(glbFilePath));
        return 0;
    }

    unsigned int modelId = m_next3dModelId++;
    m_3dModelList[modelId] = model;

    // Reset 2D texture cache: texture uploads during loading are outside
    // PBOGLES's m_lastTextureId tracking, which would cause 2D sprites to
    // silently skip their bind and render with the wrong texture.
    oglResetTextureCache();

    return modelId;
}

bool PB3D::pb3dUnloadModel(unsigned int modelId) {
    auto it = m_3dModelList.find(modelId);
    if (it == m_3dModelList.end()) return false;

    for (auto& mesh : it->second.meshes) {
        ogl3dDestroyMesh(mesh.vao, mesh.vboVertices, mesh.eboIndices);
    }
    // Delete each unique texture exactly once (set deduplicates shared textures)
    for (unsigned int texId : it->second.ownedTextures) {
        ogl3dDestroyTexture(texId);
    }

    m_3dModelList.erase(it);

    // Remove any instances that were referencing this model to prevent stale modelId references
    for (auto instIt = m_3dInstanceList.begin(); instIt != m_3dInstanceList.end(); ) {
        if (instIt->second.modelId == modelId) {
            m_3dAnimateList.erase(instIt->first); // safe no-op if no animation entry exists
            instIt = m_3dInstanceList.erase(instIt);
        } else {
            ++instIt;
        }
    }
    return true;
}

// ============================================================================
// Instance Management
// ============================================================================

unsigned int PB3D::pb3dCreateInstance(unsigned int modelId) {
    if (m_3dModelList.find(modelId) == m_3dModelList.end()) return 0;

    st3DInstance instance = {};
    instance.modelId = modelId;
    instance.posX = 0.0f; instance.posY = 0.0f; instance.posZ = 0.0f;
    instance.rotX = 0.0f; instance.rotY = 0.0f; instance.rotZ = 0.0f;
    instance.scale = 1.0f;
    instance.alpha = 1.0f;
    instance.visible = true;
    instance.hasPixelAnchor = false;
    instance.anchorPixelX = 0.0f; instance.anchorPixelY = 0.0f;
    instance.anchorBaseX  = 0.0f; instance.anchorBaseY  = 0.0f;

    // Initialize skeleton animation state
    instance.skelState.clipIndex       = -1;
    instance.skelState.currentTime     = 0.0f;
    instance.skelState.looping         = false;
    instance.skelState.isPlaying       = false;
    instance.skelState.lastUpdateTick  = 0;
    // Only allocate bone matrix storage for skinned models.
    const st3DModel& model = m_3dModelList.at(modelId);
    if (model.hasSkeleton) {
        // Initialize ALL bone matrix slots to identity (pass-through).
        // Zero matrices would collapse all weighted vertices to the origin.
        instance.skelState.boneMatrices.assign(PB3D_MAX_BONES * 16, 0.0f);
        for (int bi = 0; bi < PB3D_MAX_BONES; bi++) {
            float* m = instance.skelState.boneMatrices.data() + bi * 16;
            m[0] = m[5] = m[10] = m[15] = 1.0f;
        }
    }

    unsigned int instanceId = m_next3dInstanceId++;
    m_3dInstanceList[instanceId] = instance;
    return instanceId;
}

bool PB3D::pb3dDestroyInstance(unsigned int instanceId) {
    auto it = m_3dInstanceList.find(instanceId);
    if (it == m_3dInstanceList.end()) return false;
    m_3dInstanceList.erase(it);
    m_3dAnimateList.erase(instanceId);  // remove stale animation so it isn't processed next frame
    return true;
}

// --- Internal world-unit position setter (private, used by animation system) ---
void PB3D::pb3dSetInstancePosition(unsigned int instanceId, float x, float y, float z) {
    auto it = m_3dInstanceList.find(instanceId);
    if (it != m_3dInstanceList.end()) {
        it->second.posX = x; it->second.posY = y; it->second.posZ = z;
    }
}

void PB3D::pb3dSetInstanceRotation(unsigned int instanceId, float rx, float ry, float rz) {
    auto it = m_3dInstanceList.find(instanceId);
    if (it != m_3dInstanceList.end()) {
        it->second.rotX = rx; it->second.rotY = ry; it->second.rotZ = rz;
    }
}

void PB3D::pb3dSetInstanceScale(unsigned int instanceId, float scale) {
    auto it = m_3dInstanceList.find(instanceId);
    if (it != m_3dInstanceList.end()) {
        it->second.scale = scale;
    }
}

void PB3D::pb3dSetInstanceAlpha(unsigned int instanceId, float alpha) {
    auto it = m_3dInstanceList.find(instanceId);
    if (it != m_3dInstanceList.end()) {
        it->second.alpha = alpha;
    }
}

void PB3D::pb3dSetInstanceVisible(unsigned int instanceId, bool visible) {
    auto it = m_3dInstanceList.find(instanceId);
    if (it != m_3dInstanceList.end()) {
        it->second.visible = visible;
    }
}

bool PB3D::pb3dGetInstanceVisible(unsigned int instanceId) const {
    auto it = m_3dInstanceList.find(instanceId);
    if (it == m_3dInstanceList.end()) return false;
    return it->second.visible;
}

// --- Public pixel-space position setter ---
void PB3D::pb3dSetInstancePositionPx(unsigned int instanceId, float pixelX, float pixelY) {
    pb3dSetInstancePositionPxImpl(instanceId, pixelX, pixelY, 0.0f);
}
void PB3D::pb3dSetInstancePositionPx(unsigned int instanceId, float pixelX, float pixelY, float depthZ) {
    pb3dSetInstancePositionPxImpl(instanceId, pixelX, pixelY, depthZ);
}
void PB3D::pb3dSetInstancePositionPxImpl(unsigned int instanceId, float pixelX, float pixelY, float depthZ) {
    auto it = m_3dInstanceList.find(instanceId);
    if (it == m_3dInstanceList.end()) return;

    float wx, wy;
    pb3dPixelToWorld(pixelX, pixelY, depthZ, wx, wy);
    it->second.posX = wx; it->second.posY = wy; it->second.posZ = depthZ;

    // Store pixel anchor: base world X/Y at Z=0 used as the reference for
    // per-frame Z-depth correction so Z animation doesn't drift laterally.
    float baseX, baseY;
    pb3dPixelToWorld(pixelX, pixelY, 0.0f, baseX, baseY);
    it->second.hasPixelAnchor = true;
    it->second.anchorPixelX  = pixelX;
    it->second.anchorPixelY  = pixelY;
    it->second.anchorBaseX   = baseX;
    it->second.anchorBaseY   = baseY;
}

// --- Simplified lighting controls (public) ---
void PB3D::pb3dSetLightDirection(float x, float y, float z) {
    m_light.dirX = x; m_light.dirY = y; m_light.dirZ = z;
    m_sceneDirty = true;
}
void PB3D::pb3dSetLightColor(float r, float g, float b) {
    m_light.r = r; m_light.g = g; m_light.b = b;
    m_sceneDirty = true;
}
void PB3D::pb3dSetLightAmbient(float r, float g, float b) {
    m_light.ambientR = r; m_light.ambientG = g; m_light.ambientB = b;
    m_sceneDirty = true;
}

// --- Internal camera setter (private) ---
void PB3D::pb3dSetCamera(st3DCamera camera) {
    m_camera = camera;
    m_sceneDirty = true;
}

// --- Internal pixel-to-world conversion (private) ---
void PB3D::pb3dPixelToWorld(float pixelX, float pixelY, float depthZ, float& outX, float& outY) {
    // mat4x4_perspective takes a VERTICAL FOV, so tan(vfov/2) * dist gives the
    // half-HEIGHT of the frustum at that depth plane.
    // Half-WIDTH = half-height * aspect (wider than tall for 16:9 screen).
    float distToPlane = m_camera.eyeZ - depthZ;
    float aspect = (float)oglGetScreenWidth() / (float)oglGetScreenHeight();
    float halfH  = tanf(m_camera.fov * 0.5f * 3.14159265f / 180.0f) * distToPlane;
    float halfW  = halfH * aspect;   // horizontal half-extent

    float ndcX =  pixelX / (float)oglGetScreenWidth()  * 2.0f - 1.0f;
    float ndcY =  1.0f - pixelY / (float)oglGetScreenHeight() * 2.0f;

    outX = ndcX * halfW;
    outY = ndcY * halfH;
}

// ============================================================================
// Rendering
// ============================================================================

void PB3D::pb3dBegin() {
    ogl3dBeginPass();
    m_skinnedShaderActive = false;

    // Re-upload light uniforms and recompute view/projection matrices only when
    // the scene has changed (camera or lighting).  Neither changes at runtime in
    // normal usage, so this eliminates constant trig work every frame.
    if (m_sceneDirty) {
        // Static shader scene uniforms
        ogl3dSetSceneUniforms(
            m_light.dirX,     m_light.dirY,     m_light.dirZ,
            m_light.r,        m_light.g,        m_light.b,
            m_light.ambientR, m_light.ambientG, m_light.ambientB,
            m_camera.eyeX,    m_camera.eyeY,    m_camera.eyeZ);

        // Skinned shader scene uniforms (switch, upload, switch back)
        ogl3dActivateSkinnedShader();
        ogl3dSetSkinnedSceneUniforms(
            m_light.dirX,     m_light.dirY,     m_light.dirZ,
            m_light.r,        m_light.g,        m_light.b,
            m_light.ambientR, m_light.ambientG, m_light.ambientB,
            m_camera.eyeX,    m_camera.eyeY,    m_camera.eyeZ);
        ogl3dActivateStaticShader();

        // Compute view matrix using linmath.h
        vec3 eye = {m_camera.eyeX, m_camera.eyeY, m_camera.eyeZ};
        vec3 center = {m_camera.lookX, m_camera.lookY, m_camera.lookZ};
        vec3 up = {m_camera.upX, m_camera.upY, m_camera.upZ};

        mat4x4 view;
        mat4x4_look_at(view, eye, center, up);
        memcpy(m_viewMatrix, view, sizeof(m_viewMatrix));

        // Compute projection matrix
        float aspect = (float)oglGetScreenWidth() / (float)oglGetScreenHeight();
        float fovRad = m_camera.fov * 3.14159265f / 180.0f;

        mat4x4 proj;
        mat4x4_perspective(proj, fovRad, aspect, m_camera.nearPlane, m_camera.farPlane);
        memcpy(m_projMatrix, proj, sizeof(m_projMatrix));

        m_sceneDirty = false;
    }
}

void PB3D::pb3dEnd() {
    // Ensure static shader is restored before going back to 2D
    if (m_skinnedShaderActive) {
        ogl3dActivateStaticShader();
        m_skinnedShaderActive = false;
    }
    oglRestore2DState();
}

void PB3D::pb3dRenderInstance(unsigned int instanceId) {
    auto instIt = m_3dInstanceList.find(instanceId);
    if (instIt == m_3dInstanceList.end()) return;

    st3DInstance& inst = instIt->second;
    if (!inst.visible) return;

    auto modelIt = m_3dModelList.find(inst.modelId);
    if (modelIt == m_3dModelList.end()) return;

    // Pixel anchor: compute Z-depth perspective correction into local render
    // position — do NOT mutate inst.posX/Y, which would compound each frame.
    // Delta = worldXY_at_currentZ - worldXY_at_Z0 keeps the object at the same
    // screen pixel as Z animates, while preserving any XY animation (die 4 jitter).
    float renderX = inst.posX;
    float renderY = inst.posY;
    if (inst.hasPixelAnchor) {
        float wxZ, wyZ;
        pb3dPixelToWorld(inst.anchorPixelX, inst.anchorPixelY, inst.posZ, wxZ, wyZ);
        renderX += (wxZ - inst.anchorBaseX);
        renderY += (wyZ - inst.anchorBaseY);
    }

    // Build model matrix.
    // Vertex positions in VBOs are raw model-space coordinates (not normalized).
    // We fold the normalization (center + scale) into the matrix chain so that
    // the model displays at a consistent unit size regardless of original authoring
    // scale, and so that IBMs work correctly in the skinned path:
    //   world = T * R * S_user * S_norm * translate(-center) * P_original
    const st3DModel& model3d = modelIt->second;

    mat4x4 model, identMat;
    mat4x4_identity(identMat);

    // Translation (world position)
    mat4x4 translateMat;
    mat4x4_translate(translateMat, renderX, renderY, inst.posZ);

    // Rotation Y
    mat4x4 rotY;
    mat4x4_rotate_Y(rotY, identMat, inst.rotY * 3.14159265f / 180.0f);

    // Rotation X
    mat4x4 rotX;
    mat4x4_rotate_X(rotX, identMat, inst.rotX * 3.14159265f / 180.0f);

    // Rotation Z
    mat4x4 rotZ;
    mat4x4_rotate_Z(rotZ, identMat, inst.rotZ * 3.14159265f / 180.0f);

    // Combined scale = user scale * model normalization scale
    float effectiveScale = inst.scale * model3d.normScale;
    mat4x4 scaleMat;
    mat4x4_identity(scaleMat);
    scaleMat[0][0] = effectiveScale;
    scaleMat[1][1] = effectiveScale;
    scaleMat[2][2] = effectiveScale;

    // Pre-center translate: moves model origin to its bounding-box center before scale/rotate
    mat4x4 centerMat;
    mat4x4_translate(centerMat, -model3d.normCX, -model3d.normCY, -model3d.normCZ);

    // Combine: model = T * rotYXZ * scale * preCenterTranslate
    mat4x4 rotYX, rotYXZ, rotYXZS, rotYXZSC;
    mat4x4_mul(rotYX,    rotY,     rotX);
    mat4x4_mul(rotYXZ,   rotYX,    rotZ);
    mat4x4_mul(rotYXZS,  rotYXZ,   scaleMat);
    mat4x4_mul(rotYXZSC, rotYXZS,  centerMat);
    mat4x4_mul(model,    translateMat, rotYXZSC);

    // Compute MVP = projection * view * model
    mat4x4 view, proj, viewModel, mvp;
    memcpy(view, m_viewMatrix, sizeof(view));
    memcpy(proj, m_projMatrix, sizeof(proj));
    mat4x4_mul(viewModel, view, model);
    mat4x4_mul(mvp, proj, viewModel);

    // Set per-instance uniforms and transparency blend state
    //
    // A skinned mesh VAO is built with attribute pointers keyed to the SKINNED
    // shader's attribute locations (m_3dSk_PosAttrib etc.).  If we switch to the
    // static shader for the same VAO, the static shader's attribute locations may
    // differ, causing the GPU to misread vertex data entirely.
    //
    // Rule: a mesh is drawn with the skinned shader whenever it was uploaded as a
    // skinned VAO (mesh.isSkinned == true), regardless of whether an animation clip
    // is currently playing.  When no clip is active, identity bone matrices are used,
    // which trivially produces skinnedPos == rawPos (weights sum to 1 × identity).
    //
    // needSkinned only gates BONE MATRIX COMPUTATION, not shader selection.
    bool hasActiveClip = model3d.hasSkeleton && (inst.skelState.clipIndex >= 0);

    // Prepare bone matrices: use instance's bone matrices if a clip is active,
    // otherwise a local identity array so the shader gets valid data.
    static float s_identityBones[PB3D_MAX_BONES * 16] = {};
    static bool  s_identityBonesInit = false;
    if (!s_identityBonesInit) {
        for (int bi = 0; bi < PB3D_MAX_BONES; bi++) {
            float* m = s_identityBones + bi * 16;
            m[0] = m[5] = m[10] = m[15] = 1.0f;
        }
        s_identityBonesInit = true;
    }
    const float* activeBones   = hasActiveClip ? inst.skelState.boneMatrices.data() : s_identityBones;
    int          activeBoneCount = (int)model3d.skeleton.bones.size();
    if (activeBoneCount > PB3D_MAX_BONES) activeBoneCount = PB3D_MAX_BONES;

    bool currentBlend = (inst.alpha < 1.0f);
    ogl3dSetBlend(currentBlend);

    for (auto& mesh : model3d.meshes) {
        // Effective alpha = instance alpha × material base_color_factor alpha.
        // This honours per-material transparency (e.g. glass with materialBaseAlpha < 1)
        // independently of any animation or instance-level alpha fade.
        float effectiveAlpha = inst.alpha * mesh.materialBaseAlpha;
        bool meshBlend = (effectiveAlpha < 1.0f) || mesh.needsBlend;

        // Use skinned shader for skinned VAOs, static shader for non-skinned VAOs.
        // This must match how each VAO's attribute pointers were originally set up.
        if (mesh.isSkinned && !m_skinnedShaderActive) {
            ogl3dActivateSkinnedShader();
            m_skinnedShaderActive = true;
        } else if (!mesh.isSkinned && m_skinnedShaderActive) {
            ogl3dActivateStaticShader();
            m_skinnedShaderActive = false;
        }

        // Switch blend state if it differs from current
        if (meshBlend != currentBlend) {
            ogl3dSetBlend(meshBlend);
            currentBlend = meshBlend;
        }

        // Upload per-mesh uniforms with the effective alpha for this mesh.
        // Always upload so that effectiveAlpha (which can vary per mesh) is current.
        if (m_skinnedShaderActive) {
            ogl3dSetSkinnedInstanceUniforms((const float*)mvp, (const float*)model,
                                            effectiveAlpha, activeBones, activeBoneCount);
        } else {
            ogl3dSetInstanceUniforms((const float*)mvp, (const float*)model, effectiveAlpha);
        }

        ogl3dDrawMeshPrimitive(mesh.vao, mesh.textureId, mesh.indexCount);
    }

    // Ensure blend is disabled after this instance so the next draw call is clean
    if (currentBlend) {
        ogl3dSetBlend(false);
    }
}

void PB3D::pb3dRenderAll() {
    pb3dBegin();
    for (auto& pair : m_3dInstanceList) {
        if (pair.second.visible) {
            pb3dRenderInstance(pair.first);
        }
    }
    pb3dEnd();
}

// ============================================================================
// Animation System
// ============================================================================

float PB3D::pb3dGetRandomFloat(float min, float max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    if (min > max) std::swap(min, max);  // guard against inverted range
    return dist(gen) * (max - min) + min;
}

bool PB3D::pb3dCreateAnimation(st3DAnimateData anim, bool replaceExisting) {
    if (m_3dInstanceList.find(anim.animateInstanceId) == m_3dInstanceList.end()) return false;

    // Convert pixel-space start/end X/Y to world units now, once, before storing
    if (anim.usePxCoords) {
        float wx0, wy0, wx1, wy1;
        // Always convert at Z=0: the pixel anchor system in pb3dRenderInstance
        // handles depth compensation at render time. Converting at posZ here
        // would double-count the depth offset and push objects off-screen.
        pb3dPixelToWorld(anim.startPxX, anim.startPxY, 0.0f, wx0, wy0);
        pb3dPixelToWorld(anim.endPxX,   anim.endPxY,   0.0f, wx1, wy1);
        anim.startPosX = wx0;  anim.startPosY = wy0;
        anim.endPosX   = wx1;  anim.endPosY   = wy1;
        anim.usePxCoords = false;  // now stored as world units
    }

    auto existingIt = m_3dAnimateList.find(anim.animateInstanceId);
    if (existingIt != m_3dAnimateList.end()) {
        if (!replaceExisting) return false;
    }

    m_3dAnimateList[anim.animateInstanceId] = anim;
    return true;
}

bool PB3D::pb3dAnimateInstance(unsigned int instanceId, unsigned int currentTick) {
    if (instanceId == 0) {
        // Update all active transform animations
        bool anyActive = false;
        for (auto& pair : m_3dAnimateList) {
            if (pair.second.isActive) {
                pb3dProcessAnimation(pair.second, currentTick);
                anyActive = true;
            }
        }
        // Update all active skeleton animations
        for (auto& pair : m_3dInstanceList) {
            st3DInstance& inst = pair.second;
            if (inst.skelState.isPlaying) {
                auto modelIt = m_3dModelList.find(inst.modelId);
                if (modelIt != m_3dModelList.end() && modelIt->second.hasSkeleton) {
                    pb3dUpdateSkelState(inst, modelIt->second.skeleton, currentTick);
                    anyActive = true;
                }
            }
        }
        return anyActive;
    }

    bool anyActive = false;

    // Update transform animation if one exists
    auto animIt = m_3dAnimateList.find(instanceId);
    if (animIt != m_3dAnimateList.end() && animIt->second.isActive) {
        pb3dProcessAnimation(animIt->second, currentTick);
        anyActive = true;
    }

    // Update skeleton animation if playing
    auto instIt = m_3dInstanceList.find(instanceId);
    if (instIt != m_3dInstanceList.end() && instIt->second.skelState.isPlaying) {
        auto modelIt = m_3dModelList.find(instIt->second.modelId);
        if (modelIt != m_3dModelList.end() && modelIt->second.hasSkeleton) {
            pb3dUpdateSkelState(instIt->second, modelIt->second.skeleton, currentTick);
            anyActive = true;
        }
    }

    return anyActive;
}

void PB3D::pb3dProcessAnimation(st3DAnimateData& anim, unsigned int currentTick) {
    // If startTick is the sentinel value (UINT_MAX from pb3dAnimateRestart), snap to currentTick
    // so the animation begins from now rather than t=0 which would be many seconds into the future.
    if (anim.startTick == UINT_MAX) {
        anim.startTick = currentTick;
    }
    // Guard against tick underflow (e.g. startTick set in future or wrap-around)
    float timeSinceStart = (currentTick >= anim.startTick)
                           ? (currentTick - anim.startTick) / 1000.0f
                           : 0.0f;
    float percentComplete = (anim.animateTimeSec > 0.0f) ? (timeSinceStart / anim.animateTimeSec) : 1.0f;

    // Check if animation time has elapsed
    if (percentComplete >= 1.0f && anim.animType != GFX_ANIM_ACCL) {
        // Handle loop types
        if (anim.loop == GFX_NOLOOP) {
            pb3dSetFinalAnimationValues(anim);
            anim.isActive = false;
            return;
        } else if (anim.loop == GFX_RESTART) {
            if (anim.animType == GFX_ANIM_JUMP) {
                // Snap to end values, then swap start/end so next cycle jumps back
                pb3dSetFinalAnimationValues(anim);
                std::swap(anim.startPosX,  anim.endPosX);
                std::swap(anim.startPosY,  anim.endPosY);
                std::swap(anim.startPosZ,  anim.endPosZ);
                std::swap(anim.startRotX,  anim.endRotX);
                std::swap(anim.startRotY,  anim.endRotY);
                std::swap(anim.startRotZ,  anim.endRotZ);
                std::swap(anim.startScale, anim.endScale);
                std::swap(anim.startAlpha, anim.endAlpha);
            }
            anim.startTick = currentTick;
            timeSinceStart = 0.0f;
            percentComplete = 0.0f;
        } else if (anim.loop == GFX_REVERSE) {
            // Swap start and end values
            std::swap(anim.startPosX, anim.endPosX); std::swap(anim.startPosY, anim.endPosY); std::swap(anim.startPosZ, anim.endPosZ);
            std::swap(anim.startRotX, anim.endRotX); std::swap(anim.startRotY, anim.endRotY); std::swap(anim.startRotZ, anim.endRotZ);
            std::swap(anim.startScale, anim.endScale);
            std::swap(anim.startAlpha, anim.endAlpha);
            anim.startTick = currentTick;
            timeSinceStart = 0.0f;
            percentComplete = 0.0f;
        }
    }

    // Dispatch to appropriate animation handler
    switch (anim.animType) {
        case GFX_ANIM_NORMAL:
            pb3dAnimateNormal(anim, currentTick, timeSinceStart, percentComplete);
            break;
        case GFX_ANIM_ACCL:
            pb3dAnimateAcceleration(anim, currentTick, timeSinceStart);
            break;
        case GFX_ANIM_JUMP:
            pb3dAnimateJump(anim, currentTick, timeSinceStart);
            break;
        case GFX_ANIM_JUMPRANDOM:
            pb3dAnimateJumpRandom(anim, currentTick, timeSinceStart);
            break;
    }
}

void PB3D::pb3dAnimateNormal(st3DAnimateData& anim, unsigned int currentTick, float timeSinceStart, float percentComplete) {
    auto instIt = m_3dInstanceList.find(anim.animateInstanceId);
    if (instIt == m_3dInstanceList.end()) return;

    st3DInstance& inst = instIt->second;
    float t = percentComplete;

    if (anim.typeMask & ANIM3D_POSX_MASK) inst.posX = anim.startPosX + (anim.endPosX - anim.startPosX) * t;
    if (anim.typeMask & ANIM3D_POSY_MASK) inst.posY = anim.startPosY + (anim.endPosY - anim.startPosY) * t;
    if (anim.typeMask & ANIM3D_POSZ_MASK) inst.posZ = anim.startPosZ + (anim.endPosZ - anim.startPosZ) * t;
    if (anim.typeMask & ANIM3D_ROTX_MASK) inst.rotX = anim.startRotX + (anim.endRotX - anim.startRotX) * t;
    if (anim.typeMask & ANIM3D_ROTY_MASK) inst.rotY = anim.startRotY + (anim.endRotY - anim.startRotY) * t;
    if (anim.typeMask & ANIM3D_ROTZ_MASK) inst.rotZ = anim.startRotZ + (anim.endRotZ - anim.startRotZ) * t;
    if (anim.typeMask & ANIM3D_SCALE_MASK) inst.scale = anim.startScale + (anim.endScale - anim.startScale) * t;
    if (anim.typeMask & ANIM3D_ALPHA_MASK) inst.alpha = anim.startAlpha + (anim.endAlpha - anim.startAlpha) * t;
}

void PB3D::pb3dAnimateAcceleration(st3DAnimateData& anim, unsigned int currentTick, float timeSinceStart) {
    auto instIt = m_3dInstanceList.find(anim.animateInstanceId);
    if (instIt == m_3dInstanceList.end()) return;

    st3DInstance& inst = instIt->second;
    float t = timeSinceStart;

    // position = start + v0*t + 0.5*a*t^2
    // velocity = v0 + a*t
    if (anim.typeMask & ANIM3D_POSX_MASK) {
        inst.posX = anim.startPosX + anim.initialVelX * t + 0.5f * anim.accelX * t * t;
        anim.currentVelX = anim.initialVelX + anim.accelX * t;
    }
    if (anim.typeMask & ANIM3D_POSY_MASK) {
        inst.posY = anim.startPosY + anim.initialVelY * t + 0.5f * anim.accelY * t * t;
        anim.currentVelY = anim.initialVelY + anim.accelY * t;
    }
    if (anim.typeMask & ANIM3D_POSZ_MASK) {
        inst.posZ = anim.startPosZ + anim.initialVelZ * t + 0.5f * anim.accelZ * t * t;
        anim.currentVelZ = anim.initialVelZ + anim.accelZ * t;
    }
    if (anim.typeMask & ANIM3D_ROTX_MASK) {
        inst.rotX = anim.startRotX + anim.initialVelRotX * t + 0.5f * anim.accelRotX * t * t;
        anim.currentVelRotX = anim.initialVelRotX + anim.accelRotX * t;
    }
    if (anim.typeMask & ANIM3D_ROTY_MASK) {
        inst.rotY = anim.startRotY + anim.initialVelRotY * t + 0.5f * anim.accelRotY * t * t;
        anim.currentVelRotY = anim.initialVelRotY + anim.accelRotY * t;
    }
    if (anim.typeMask & ANIM3D_ROTZ_MASK) {
        inst.rotZ = anim.startRotZ + anim.initialVelRotZ * t + 0.5f * anim.accelRotZ * t * t;
        anim.currentVelRotZ = anim.initialVelRotZ + anim.accelRotZ * t;
    }

    // Handle time elapsed — all three loop types for acceleration mode
    if (anim.animateTimeSec > 0.0f && timeSinceStart >= anim.animateTimeSec) {
        if (anim.loop == GFX_RESTART) {
            anim.startTick = currentTick;
            // Continue from current position with same velocity/acceleration
            anim.startPosX = inst.posX; anim.startPosY = inst.posY; anim.startPosZ = inst.posZ;
            anim.startRotX = inst.rotX; anim.startRotY = inst.rotY; anim.startRotZ = inst.rotZ;
        } else if (anim.loop == GFX_REVERSE) {
            // Mirror the arc: negate initial velocity (using end-of-cycle velocity)
            // and negate acceleration so the object travels back along the same path.
            anim.startPosX = inst.posX; anim.startPosY = inst.posY; anim.startPosZ = inst.posZ;
            anim.startRotX = inst.rotX; anim.startRotY = inst.rotY; anim.startRotZ = inst.rotZ;
            anim.initialVelX    = -anim.currentVelX;
            anim.initialVelY    = -anim.currentVelY;
            anim.initialVelZ    = -anim.currentVelZ;
            anim.initialVelRotX = -anim.currentVelRotX;
            anim.initialVelRotY = -anim.currentVelRotY;
            anim.initialVelRotZ = -anim.currentVelRotZ;
            anim.accelX    = -anim.accelX;
            anim.accelY    = -anim.accelY;
            anim.accelZ    = -anim.accelZ;
            anim.accelRotX = -anim.accelRotX;
            anim.accelRotY = -anim.accelRotY;
            anim.accelRotZ = -anim.accelRotZ;
            anim.startTick = currentTick;
        } else {
            // GFX_NOLOOP: freeze at current position
            anim.isActive = false;
        }
    }
}

void PB3D::pb3dAnimateJump(st3DAnimateData& anim, unsigned int /*currentTick*/, float /*timeSinceStart*/) {
    // Hold at start values during the wait interval.
    // The snap to end values on loop completion is handled in pb3dProcessAnimation.
    auto instIt = m_3dInstanceList.find(anim.animateInstanceId);
    if (instIt == m_3dInstanceList.end()) return;

    st3DInstance& inst = instIt->second;
    if (anim.typeMask & ANIM3D_POSX_MASK)  inst.posX  = anim.startPosX;
    if (anim.typeMask & ANIM3D_POSY_MASK)  inst.posY  = anim.startPosY;
    if (anim.typeMask & ANIM3D_POSZ_MASK)  inst.posZ  = anim.startPosZ;
    if (anim.typeMask & ANIM3D_ROTX_MASK)  inst.rotX  = anim.startRotX;
    if (anim.typeMask & ANIM3D_ROTY_MASK)  inst.rotY  = anim.startRotY;
    if (anim.typeMask & ANIM3D_ROTZ_MASK)  inst.rotZ  = anim.startRotZ;
    if (anim.typeMask & ANIM3D_SCALE_MASK) inst.scale = anim.startScale;
    if (anim.typeMask & ANIM3D_ALPHA_MASK) inst.alpha = anim.startAlpha;
}

void PB3D::pb3dAnimateJumpRandom(st3DAnimateData& anim, unsigned int currentTick, float timeSinceStart) {
    auto instIt = m_3dInstanceList.find(anim.animateInstanceId);
    if (instIt == m_3dInstanceList.end()) return;

    st3DInstance& inst = instIt->second;

    // Randomly decide whether to jump based on randomPercent
    float roll = pb3dGetRandomFloat(0.0f, 1.0f);
    if (roll <= anim.randomPercent) {
        if (anim.typeMask & ANIM3D_POSX_MASK) inst.posX = pb3dGetRandomFloat(anim.startPosX, anim.endPosX);
        if (anim.typeMask & ANIM3D_POSY_MASK) inst.posY = pb3dGetRandomFloat(anim.startPosY, anim.endPosY);
        if (anim.typeMask & ANIM3D_POSZ_MASK) inst.posZ = pb3dGetRandomFloat(anim.startPosZ, anim.endPosZ);
        if (anim.typeMask & ANIM3D_ROTX_MASK) inst.rotX = pb3dGetRandomFloat(anim.startRotX, anim.endRotX);
        if (anim.typeMask & ANIM3D_ROTY_MASK) inst.rotY = pb3dGetRandomFloat(anim.startRotY, anim.endRotY);
        if (anim.typeMask & ANIM3D_ROTZ_MASK) inst.rotZ = pb3dGetRandomFloat(anim.startRotZ, anim.endRotZ);
        if (anim.typeMask & ANIM3D_SCALE_MASK) inst.scale = pb3dGetRandomFloat(anim.startScale, anim.endScale);
        if (anim.typeMask & ANIM3D_ALPHA_MASK) inst.alpha = pb3dGetRandomFloat(anim.startAlpha, anim.endAlpha);
    }
}

void PB3D::pb3dSetFinalAnimationValues(const st3DAnimateData& anim) {
    auto instIt = m_3dInstanceList.find(anim.animateInstanceId);
    if (instIt == m_3dInstanceList.end()) return;

    st3DInstance& inst = instIt->second;

    if (anim.typeMask & ANIM3D_POSX_MASK) inst.posX = anim.endPosX;
    if (anim.typeMask & ANIM3D_POSY_MASK) inst.posY = anim.endPosY;
    if (anim.typeMask & ANIM3D_POSZ_MASK) inst.posZ = anim.endPosZ;
    if (anim.typeMask & ANIM3D_ROTX_MASK) inst.rotX = anim.endRotX;
    if (anim.typeMask & ANIM3D_ROTY_MASK) inst.rotY = anim.endRotY;
    if (anim.typeMask & ANIM3D_ROTZ_MASK) inst.rotZ = anim.endRotZ;
    if (anim.typeMask & ANIM3D_SCALE_MASK) inst.scale = anim.endScale;
    if (anim.typeMask & ANIM3D_ALPHA_MASK) inst.alpha = anim.endAlpha;
}

bool PB3D::pb3dAnimateActive(unsigned int instanceId) {
    if (instanceId == 0) {
        for (auto& pair : m_3dAnimateList) {
            if (pair.second.isActive) return true;
        }
        return false;
    }

    auto it = m_3dAnimateList.find(instanceId);
    if (it != m_3dAnimateList.end()) return it->second.isActive;
    return false;
}

void PB3D::pb3dAnimateClear(unsigned int instanceId) {
    if (instanceId == 0) {
        m_3dAnimateList.clear();
        return;
    }
    m_3dAnimateList.erase(instanceId);
}

void PB3D::pb3dAnimateRestart(unsigned int instanceId) {
    if (instanceId == 0) {
        for (auto& pair : m_3dAnimateList) {
            pair.second.isActive  = true;
            pair.second.startTick = UINT_MAX; // Sentinel: will be set to currentTick on first process call
        }
        return;
    }
    auto it = m_3dAnimateList.find(instanceId);
    if (it != m_3dAnimateList.end()) {
        it->second.isActive  = true;
        it->second.startTick = UINT_MAX; // Sentinel: will be set to currentTick on first process call
    }
}

void PB3D::pb3dAnimateRestart(unsigned int instanceId, unsigned long startTick) {
    if (instanceId == 0) {
        for (auto& pair : m_3dAnimateList) {
            pair.second.isActive = true;
            pair.second.startTick = (unsigned int)startTick;
        }
        return;
    }
    auto it = m_3dAnimateList.find(instanceId);
    if (it != m_3dAnimateList.end()) {
        it->second.isActive = true;
        it->second.startTick = (unsigned int)startTick;
    }
}

// Convenience fade helpers — create a one-shot alpha animation (0→1 or 1→0).
void PB3D::pb3dFadeIn(unsigned int instanceId, float timeSec, unsigned long startTick) {
    st3DAnimateData anim = {};
    anim.animateInstanceId = instanceId;
    anim.typeMask          = ANIM3D_ALPHA_MASK;
    anim.animType          = GFX_ANIM_NORMAL;
    anim.loop              = GFX_NOLOOP;
    anim.startAlpha        = 0.0f;
    anim.endAlpha          = 1.0f;
    anim.animateTimeSec    = timeSec;
    anim.startTick         = (unsigned int)startTick;
    anim.isActive          = true;
    pb3dCreateAnimation(anim, true);
}

void PB3D::pb3dFadeOut(unsigned int instanceId, float timeSec, unsigned long startTick) {
    st3DAnimateData anim = {};
    anim.animateInstanceId = instanceId;
    anim.typeMask          = ANIM3D_ALPHA_MASK;
    anim.animType          = GFX_ANIM_NORMAL;
    anim.loop              = GFX_NOLOOP;
    anim.startAlpha        = 1.0f;
    anim.endAlpha          = 0.0f;
    anim.animateTimeSec    = timeSec;
    anim.startTick         = (unsigned int)startTick;
    anim.isActive          = true;
    pb3dCreateAnimation(anim, true);
}

// ============================================================================
// Skeleton Animation API
// ============================================================================

std::vector<std::string> PB3D::pb3dListAnimClips(unsigned int modelId) {
    std::vector<std::string> names;
    auto it = m_3dModelList.find(modelId);
    if (it == m_3dModelList.end() || !it->second.hasSkeleton) return names;
    for (const auto& clip : it->second.skeleton.clips) {
        names.push_back(clip.name);
    }
    return names;
}

int PB3D::pb3dFindAnimClip(unsigned int modelId, const std::string& clipName) {
    auto it = m_3dModelList.find(modelId);
    if (it == m_3dModelList.end() || !it->second.hasSkeleton) return -1;
    const auto& clips = it->second.skeleton.clips;
    for (int i = 0; i < (int)clips.size(); i++) {
        if (clips[i].name == clipName) return i;
    }
    return -1;
}

bool PB3D::pb3dPlayAnimClip(unsigned int instanceId, const std::string& clipName, bool loop) {
    auto instIt = m_3dInstanceList.find(instanceId);
    if (instIt == m_3dInstanceList.end()) return false;
    auto modelIt = m_3dModelList.find(instIt->second.modelId);
    if (modelIt == m_3dModelList.end() || !modelIt->second.hasSkeleton) return false;
    int clipIdx = pb3dFindAnimClip(instIt->second.modelId, clipName);
    if (clipIdx < 0) return false;
    return pb3dPlayAnimClip(instanceId, clipIdx, loop);
}

bool PB3D::pb3dPlayAnimClip(unsigned int instanceId, int clipIndex, bool loop) {
    auto instIt = m_3dInstanceList.find(instanceId);
    if (instIt == m_3dInstanceList.end()) return false;
    auto modelIt = m_3dModelList.find(instIt->second.modelId);
    if (modelIt == m_3dModelList.end() || !modelIt->second.hasSkeleton) return false;
    if (clipIndex < 0 || clipIndex >= (int)modelIt->second.skeleton.clips.size()) return false;

    st3DSkelState& ss = instIt->second.skelState;
    ss.clipIndex      = clipIndex;
    ss.currentTime    = 0.0f;
    ss.looping        = loop;
    ss.isPlaying      = true;
    ss.lastUpdateTick = 0;
    // Compute initial bone matrices for time 0
    pb3dUpdateSkelState(instIt->second, modelIt->second.skeleton, 0);
    return true;
}

bool PB3D::pb3dStopAnimClip(unsigned int instanceId) {
    auto instIt = m_3dInstanceList.find(instanceId);
    if (instIt == m_3dInstanceList.end()) return false;
    st3DSkelState& ss = instIt->second.skelState;
    ss.isPlaying   = false;
    ss.clipIndex   = -1;  // Reset to -1 so hasActiveClip becomes false and identity
    ss.currentTime = 0.0f; // matrices are used, returning the model to its bind pose.
    return true;
}

bool PB3D::pb3dIsAnimClipPlaying(unsigned int instanceId) const {
    auto instIt = m_3dInstanceList.find(instanceId);
    if (instIt == m_3dInstanceList.end()) return false;
    return instIt->second.skelState.isPlaying;
}

float PB3D::pb3dGetAnimClipTime(unsigned int instanceId) const {
    auto instIt = m_3dInstanceList.find(instanceId);
    if (instIt == m_3dInstanceList.end()) return 0.0f;
    return instIt->second.skelState.currentTime;
}

bool PB3D::pb3dSetAnimClipTime(unsigned int instanceId, float timeSec) {
    auto instIt = m_3dInstanceList.find(instanceId);
    if (instIt == m_3dInstanceList.end()) return false;
    auto modelIt = m_3dModelList.find(instIt->second.modelId);
    if (modelIt == m_3dModelList.end() || !modelIt->second.hasSkeleton) return false;
    instIt->second.skelState.currentTime = timeSec;
    pb3dUpdateSkelState(instIt->second, modelIt->second.skeleton, 0);
    return true;
}

// ============================================================================
// Skeleton Animation Internal Helpers
// ============================================================================

// Update bone matrices for an instance based on the current time stored in skelState.
// Call every frame for playing instances via pb3dAnimateInstance.
void PB3D::pb3dUpdateSkelState(st3DInstance& inst, const st3DSkeleton& skel, unsigned int currentTick) {
    st3DSkelState& ss = inst.skelState;
    if (!ss.isPlaying || ss.clipIndex < 0 || ss.clipIndex >= (int)skel.clips.size()) return;

    // Advance time if currentTick is valid
    if (currentTick > 0 && ss.lastUpdateTick > 0 && currentTick > ss.lastUpdateTick) {
        float dt = (currentTick - ss.lastUpdateTick) / 1000.0f;
        ss.currentTime += dt;
        const st3DAnimClip& clip = skel.clips[ss.clipIndex];
        if (clip.duration > 0.0f && ss.currentTime > clip.duration) {
            if (ss.looping) {
                ss.currentTime = fmodf(ss.currentTime, clip.duration);
            } else {
                ss.currentTime = clip.duration;
                ss.isPlaying   = false;
            }
        }
    }
    ss.lastUpdateTick = currentTick;

    pb3dComputeBoneMatrices(skel, skel.clips[ss.clipIndex], ss.currentTime, ss.boneMatrices.data());
}

// Evaluate a single animation channel at the given time; writes 3 or 4 floats to out[].
void PB3D::pb3dSkelEvalChannel(const st3DAnimChannel& ch, float time, float out[4]) {
    if (ch.keyframes.empty()) {
        out[0] = out[1] = out[2] = 0.0f; out[3] = 1.0f;
        return;
    }

    // Clamp to first / last keyframe
    if (time <= ch.keyframes.front().time) {
        memcpy(out, ch.keyframes.front().value, sizeof(float) * 4);
        return;
    }
    if (time >= ch.keyframes.back().time) {
        memcpy(out, ch.keyframes.back().value, sizeof(float) * 4);
        return;
    }

    // Find surrounding keyframes (linear search; typical clips have few keyframes per bone)
    size_t k = 0;
    while (k + 1 < ch.keyframes.size() && ch.keyframes[k + 1].time <= time) k++;

    const st3DAnimKeyframe& kf0 = ch.keyframes[k];
    const st3DAnimKeyframe& kf1 = ch.keyframes[k + 1];
    float span = kf1.time - kf0.time;
    float t    = (span > 1e-8f) ? (time - kf0.time) / span : 0.0f;

    if (ch.interpolation == ANIM_INTERP_STEP) {
        memcpy(out, kf0.value, sizeof(float) * 4);
    } else if (ch.type == ANIM_CHANNEL_ROTATION) {
        pb3dSkelSlerpQuat(kf0.value, kf1.value, t, out);
    } else {
        // LINEAR translation or scale
        out[0] = pb3dSkelInterpolateFloat(kf0.value[0], kf1.value[0], t);
        out[1] = pb3dSkelInterpolateFloat(kf0.value[1], kf1.value[1], t);
        out[2] = pb3dSkelInterpolateFloat(kf0.value[2], kf1.value[2], t);
        out[3] = 0.0f;
    }
}

float PB3D::pb3dSkelInterpolateFloat(float a, float b, float t) {
    return a + (b - a) * t;
}

// Spherical linear interpolation for unit quaternions (xyzw).
void PB3D::pb3dSkelSlerpQuat(const float qa[4], const float qb[4], float t, float out[4]) {
    float dot = qa[0]*qb[0] + qa[1]*qb[1] + qa[2]*qb[2] + qa[3]*qb[3];
    float qb2[4] = {qb[0], qb[1], qb[2], qb[3]};
    if (dot < 0.0f) {
        // Flip qb to take the shorter arc
        dot = -dot;
        qb2[0] = -qb2[0]; qb2[1] = -qb2[1]; qb2[2] = -qb2[2]; qb2[3] = -qb2[3];
    }
    if (dot > 0.9995f) {
        // Quaternions nearly identical — linear lerp to avoid division by zero
        out[0] = qa[0] + t*(qb2[0]-qa[0]);
        out[1] = qa[1] + t*(qb2[1]-qa[1]);
        out[2] = qa[2] + t*(qb2[2]-qa[2]);
        out[3] = qa[3] + t*(qb2[3]-qa[3]);
        float len = sqrtf(out[0]*out[0]+out[1]*out[1]+out[2]*out[2]+out[3]*out[3]);
        if (len > 1e-8f) { out[0]/=len; out[1]/=len; out[2]/=len; out[3]/=len; }
        return;
    }
    float theta0 = acosf(dot);
    float theta  = theta0 * t;
    float sinT0  = sinf(theta0);
    float sinT   = sinf(theta);
    float s0 = cosf(theta) - dot * sinT / sinT0;
    float s1 = sinT / sinT0;
    out[0] = s0*qa[0] + s1*qb2[0];
    out[1] = s0*qa[1] + s1*qb2[1];
    out[2] = s0*qa[2] + s1*qb2[2];
    out[3] = s0*qa[3] + s1*qb2[3];
}

// Build a 4×4 matrix (column-major) from TRS components.
// Quaternion rotation is in xyzw order.
void PB3D::pb3dMat4FromTRS(const float t[3], const float r[4], const float s[3], float out[16]) {
    float qx = r[0], qy = r[1], qz = r[2], qw = r[3];
    float x2 = qx+qx, y2 = qy+qy, z2 = qz+qz;
    float xx = qx*x2, xy = qx*y2, xz = qx*z2;
    float yy = qy*y2, yz = qy*z2, zz = qz*z2;
    float wx = qw*x2, wy = qw*y2, wz = qw*z2;

    // Column 0
    out[0]  = (1.0f - (yy+zz)) * s[0];
    out[1]  = (xy + wz)         * s[0];
    out[2]  = (xz - wy)         * s[0];
    out[3]  = 0.0f;
    // Column 1
    out[4]  = (xy - wz)         * s[1];
    out[5]  = (1.0f - (xx+zz)) * s[1];
    out[6]  = (yz + wx)         * s[1];
    out[7]  = 0.0f;
    // Column 2
    out[8]  = (xz + wy)         * s[2];
    out[9]  = (yz - wx)         * s[2];
    out[10] = (1.0f - (xx+yy)) * s[2];
    out[11] = 0.0f;
    // Column 3
    out[12] = t[0];
    out[13] = t[1];
    out[14] = t[2];
    out[15] = 1.0f;
}

// Multiply two column-major 4×4 matrices: out = a * b
void PB3D::pb3dMat4Mul(const float a[16], const float b[16], float out[16]) {
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            out[col*4+row] = a[0*4+row]*b[col*4+0]
                           + a[1*4+row]*b[col*4+1]
                           + a[2*4+row]*b[col*4+2]
                           + a[3*4+row]*b[col*4+3];
        }
    }
}

// Invert an affine 4×4 matrix (column-major).
// Works for any combination of rotation, scale, and translation (non-singular 3×3 part).
// Returns false if the matrix is singular (det ≈ 0).
bool PB3D::pb3dMat4InvertAffine(const float m[16], float out[16]) {
    // Extract 3×3 upper-left (column-major: col0=[0,1,2], col1=[4,5,6], col2=[8,9,10])
    float a00 = m[0], a01 = m[4], a02 = m[8];
    float a10 = m[1], a11 = m[5], a12 = m[9];
    float a20 = m[2], a21 = m[6], a22 = m[10];
    float tx  = m[12], ty = m[13], tz = m[14];

    // Cofactors for 3×3 inverse
    float c00 = a11*a22 - a12*a21;
    float c01 = a12*a20 - a10*a22;
    float c02 = a10*a21 - a11*a20;
    float c10 = a02*a21 - a01*a22;
    float c11 = a00*a22 - a02*a20;
    float c12 = a01*a20 - a00*a21;
    float c20 = a01*a12 - a02*a11;
    float c21 = a02*a10 - a00*a12;
    float c22 = a00*a11 - a01*a10;

    float det = a00*c00 + a01*c01 + a02*c02;
    if (fabsf(det) < 1e-12f) return false;
    float invDet = 1.0f / det;

    // inv(A) = cofactorMatrix^T / det  (note: cofactors above are already transposed)
    float i00 = c00*invDet, i01 = c10*invDet, i02 = c20*invDet;
    float i10 = c01*invDet, i11 = c11*invDet, i12 = c21*invDet;
    float i20 = c02*invDet, i21 = c12*invDet, i22 = c22*invDet;

    // Translation: -inv(A) * t
    float itx = -(i00*tx + i01*ty + i02*tz);
    float ity = -(i10*tx + i11*ty + i12*tz);
    float itz = -(i20*tx + i21*ty + i22*tz);

    // Column-major output
    out[0]  = i00;  out[1]  = i10;  out[2]  = i20;  out[3]  = 0.0f;
    out[4]  = i01;  out[5]  = i11;  out[6]  = i21;  out[7]  = 0.0f;
    out[8]  = i02;  out[9]  = i12;  out[10] = i22;  out[11] = 0.0f;
    out[12] = itx;  out[13] = ity;  out[14] = itz;  out[15] = 1.0f;
    return true;
}

// Compute final skinning matrices for all bones in a clip at a given time.
// outMatrices is laid out as [bone0_mat16, bone1_mat16, ...] column-major.
void PB3D::pb3dComputeBoneMatrices(const st3DSkeleton& skel, const st3DAnimClip& clip,
                                    float time, float* outMatrices) {
    int numBones = (int)skel.bones.size();
    if (numBones > PB3D_MAX_BONES) numBones = PB3D_MAX_BONES;

    // Stack-allocated scratch buffers (safe for concurrent calls)
    float localMatrices[PB3D_MAX_BONES * 16];
    float worldMatrices[PB3D_MAX_BONES * 16];
    // Zero-init worldMatrices so unvisited slots are identity-neutral for the
    // second pass rather than uninitialized stack garbage.
    memset(worldMatrices, 0, sizeof(worldMatrices));

    // Evaluate each bone's animated TRS (fall back to rest pose if no channel)
    // (For small bone counts a linear search per bone is fast enough)
    for (int bi = 0; bi < numBones; bi++) {
        const st3DBone& bone = skel.bones[bi];
        float T[3], R[4], S[3];

        // Start from rest pose
        memcpy(T, bone.restTranslation, 12);
        memcpy(R, bone.restRotation,    16);
        memcpy(S, bone.restScale,       12);

        // Override with animation channels if present
        for (const auto& ch : clip.channels) {
            if (ch.boneIndex != bi) continue;
            float val[4] = {0,0,0,1};
            pb3dSkelEvalChannel(ch, time, val);
            switch (ch.type) {
                case ANIM_CHANNEL_TRANSLATION:
                    T[0]=val[0]; T[1]=val[1]; T[2]=val[2]; break;
                case ANIM_CHANNEL_ROTATION:
                    R[0]=val[0]; R[1]=val[1]; R[2]=val[2]; R[3]=val[3]; break;
                case ANIM_CHANNEL_SCALE:
                    S[0]=val[0]; S[1]=val[1]; S[2]=val[2]; break;
            }
        }

        pb3dMat4FromTRS(T, R, S, localMatrices + bi * 16);
    }

    // Forward pass: compute global (world) transforms via hierarchy.
    // The unified bone array merges multiple glTF skins, so parent indices can
    // point to bones that appear LATER in the array (violating topological order).
    // A naive single forward pass reads uninitialized parent world matrices for
    // those bones.  Two passes fixes one level of disorder but not chains
    // (e.g. wingtip→wingmid→wingroot all from Skin[0] whose wingroot's parent
    // is in Skin[1]).
    //
    // Solution: build a topological order (parent before child) via iterative DFS,
    // then do a single pass in that order.  No heap allocation — all working
    // memory is stack-allocated.
    int  topoOrder[PB3D_MAX_BONES];
    int  topoCount = 0;
    bool visited[PB3D_MAX_BONES]  = {};
    bool inStack[PB3D_MAX_BONES]  = {};
    int  dfsStack[PB3D_MAX_BONES];
    int  dfsTop = 0;

    for (int root = 0; root < numBones; root++) {
        if (visited[root]) continue;
        dfsStack[dfsTop++] = root;
        inStack[root] = true;
        while (dfsTop > 0) {
            int bi     = dfsStack[dfsTop - 1];
            int parent = skel.bones[bi].parentIndex;
            if (parent >= 0 && parent < numBones && !visited[parent] && !inStack[parent]) {
                // Parent not yet processed — visit parent first
                dfsStack[dfsTop++] = parent;
                inStack[parent] = true;
            } else {
                // All ancestors processed (or no parent): record this bone
                dfsTop--;
                if (!visited[bi]) {
                    visited[bi] = true;
                    topoOrder[topoCount++] = bi;
                }
            }
        }
    }

    // Single pass in topological order — parent world matrix is always ready.
    // Unified formula: worldMatrix = parentWorldMatrix × parentOffsetMatrix × localTRS
    //   - Root bones (parent<0): parentWorldMatrix = identity, so worldMatrix = parentOffsetMatrix × localTRS
    //   - Non-root bones: parentOffsetMatrix accounts for any non-joint nodes between parent and
    //     this bone (identity for normal bones; clavicle-local-TRS for arm roots etc.)
    float tmp16[16];  // scratch for intermediate multiply
    for (int ti = 0; ti < topoCount; ti++) {
        int bi     = topoOrder[ti];
        int parent = skel.bones[bi].parentIndex;
        if (parent < 0 || parent >= numBones) {
            // Root: worldMatrix = parentOffsetMatrix × localTRS
            pb3dMat4Mul(skel.bones[bi].parentOffsetMatrix, localMatrices + bi*16,
                        worldMatrices + bi*16);
        } else {
            // Non-root: worldMatrix = worldMatrix[parent] × parentOffsetMatrix × localTRS
            pb3dMat4Mul(worldMatrices + parent*16, skel.bones[bi].parentOffsetMatrix, tmp16);
            pb3dMat4Mul(tmp16, localMatrices + bi*16, worldMatrices + bi*16);
        }
    }

    // Skinning matrix = meshNodeGlobalInv[skinIndex] × worldMatrix × IBM
    // glTF spec: IBM = inv(jointWorldBind) × meshNodeWorldBind, so
    //   jointWorldBind × IBM = meshNodeWorldBind (a rotation/axis-swap).
    // Multiplying by meshNodeGlobalInv cancels this, yielding identity at bind pose.
    // Virtual bones (skinIndex < 0) use just worldMatrix × IBM (identity IBM).
    for (int bi = 0; bi < numBones; bi++) {
        float wib[16];
        pb3dMat4Mul(worldMatrices + bi*16, skel.bones[bi].inverseBindMatrix, wib);
        int si = skel.bones[bi].skinIndex;
        if (si >= 0 && si < skel.skinCount) {
            pb3dMat4Mul(skel.meshNodeGlobalInv[si], wib, outMatrices + bi*16);
        } else {
            memcpy(outMatrices + bi*16, wib, 64);
        }
    }
}

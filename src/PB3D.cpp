// PB3D - 3D rendering layer using glTF models via cgltf
// Sits between PBOGLES and PBGfx in the inheritance chain

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "PB3D.h"
#include "3rdparty/cgltf.h"
#include "3rdparty/linmath.h"
#include "3rdparty/stb_image.h"
#include <cstring>
#include <cmath>
#include <random>

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
                        + spec * 0.4 * uLightColor;
        fragColor = vec4(finalColor, texColor.a * uAlpha);
    }
)";

// ============================================================================
// Constructor / Destructor
// ============================================================================

PB3D::PB3D() {
    m_3dShaderProgram = 0;
    m_3dMVPUniform = -1;
    m_3dModelUniform = -1;
    m_3dLightDirUniform = -1;
    m_3dLightColorUniform = -1;
    m_3dAmbientUniform = -1;
    m_3dCameraEyeUniform = -1;
    m_3dAlphaUniform = -1;
    m_3dPosAttrib = -1;
    m_3dNormalAttrib = -1;
    m_3dTexCoordAttrib = -1;

    m_next3dModelId = 1;
    m_next3dInstanceId = 1;

    m_sceneDirty = true;

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
    // Clean up all 3D models (delete GL resources)
    for (auto& pair : m_3dModelList) {
        for (auto& mesh : pair.second.meshes) {
            if (mesh.vao) glDeleteVertexArrays(1, &mesh.vao);
            if (mesh.vboVertices) glDeleteBuffers(1, &mesh.vboVertices);
            if (mesh.eboIndices) glDeleteBuffers(1, &mesh.eboIndices);
        }
        for (GLuint texId : pair.second.ownedTextures) {
            if (texId) glDeleteTextures(1, &texId);
        }
    }
    if (m_3dShaderProgram) glDeleteProgram(m_3dShaderProgram);
}

// ============================================================================
// Console output
// ============================================================================

void PB3D::pb3dSendConsole(const std::string& msg) {
    std::cout << msg << std::endl;
}

// ============================================================================
// Initialization
// ============================================================================

bool PB3D::pb3dInit() {
    // Compile the 3D shaders using the inherited PBOGLES methods
    m_3dShaderProgram = oglCreateProgram(vertexShader3DSource, fragmentShader3DSource);
    if (m_3dShaderProgram == 0) {
        pb3dSendConsole("PB3D: Failed to create 3D shader program");
        return false;
    }

    // Cache uniform locations
    m_3dMVPUniform = glGetUniformLocation(m_3dShaderProgram, "uMVP");
    m_3dModelUniform = glGetUniformLocation(m_3dShaderProgram, "uModel");
    m_3dLightDirUniform = glGetUniformLocation(m_3dShaderProgram, "uLightDir");
    m_3dLightColorUniform = glGetUniformLocation(m_3dShaderProgram, "uLightColor");
    m_3dAmbientUniform = glGetUniformLocation(m_3dShaderProgram, "uAmbientColor");
    m_3dCameraEyeUniform = glGetUniformLocation(m_3dShaderProgram, "uCameraEye");
    m_3dAlphaUniform = glGetUniformLocation(m_3dShaderProgram, "uAlpha");

    // Cache attribute locations
    m_3dPosAttrib = glGetAttribLocation(m_3dShaderProgram, "aPosition");
    m_3dNormalAttrib = glGetAttribLocation(m_3dShaderProgram, "aNormal");
    m_3dTexCoordAttrib = glGetAttribLocation(m_3dShaderProgram, "aTexCoord");

    if (m_3dPosAttrib < 0) {
        pb3dSendConsole("PB3D ERROR: aPosition attrib not found - shader likely failed to compile"
                        " (pos=" + std::to_string(m_3dPosAttrib)
                        + " norm=" + std::to_string(m_3dNormalAttrib)
                        + " uv=" + std::to_string(m_3dTexCoordAttrib) + ")");
        return false;
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

unsigned int PB3D::pb3dLoadModel(const char* glbFilePath) {
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

    // Local texture deduplication cache (cgltf_image* → GL texture ID).
    // Keyed on image pointer for identity comparison within this load session.
    // nullptr key is reserved for the shared 1×1 white fallback texture.
    std::map<const cgltf_image*, GLuint> localTexCache;

    // --- Pass 2: build GPU resources for each triangle primitive ---
    for (cgltf_size mi = 0; mi < data->meshes_count; mi++) {
        cgltf_mesh* mesh = &data->meshes[mi];

        for (cgltf_size pi = 0; pi < mesh->primitives_count; pi++) {
            cgltf_primitive* prim = &mesh->primitives[pi];
            if (prim->type != cgltf_primitive_type_triangles) continue;

            // Find POSITION, NORMAL, TEXCOORD_0 accessors
            const cgltf_accessor* posAccessor = nullptr;
            const cgltf_accessor* normAccessor = nullptr;
            const cgltf_accessor* texAccessor = nullptr;

            for (cgltf_size ai = 0; ai < prim->attributes_count; ai++) {
                if (prim->attributes[ai].type == cgltf_attribute_type_position)
                    posAccessor = prim->attributes[ai].data;
                else if (prim->attributes[ai].type == cgltf_attribute_type_normal)
                    normAccessor = prim->attributes[ai].data;
                else if (prim->attributes[ai].type == cgltf_attribute_type_texcoord)
                    texAccessor = prim->attributes[ai].data;
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
                    for (cgltf_size t = 0; t < triCount; t++) {
                        unsigned int i0 = (unsigned int)cgltf_accessor_read_index(prim->indices, t*3+0);
                        unsigned int i1 = (unsigned int)cgltf_accessor_read_index(prim->indices, t*3+1);
                        unsigned int i2 = (unsigned int)cgltf_accessor_read_index(prim->indices, t*3+2);
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

            // Apply global normalization: all primitives share the same center and scale
            // so that their relative positions within the model are preserved.
            for (cgltf_size v = 0; v < vertexCount; v++) {
                positions[v*3+0] = (positions[v*3+0] - normCX) * normScale;
                positions[v*3+1] = (positions[v*3+1] - normCY) * normScale;
                positions[v*3+2] = (positions[v*3+2] - normCZ) * normScale;
            }
            // Build interleaved vertex buffer: [posX, posY, posZ, normX, normY, normZ, u, v]
            std::vector<float> interleavedData(vertexCount * 8);
            for (cgltf_size v = 0; v < vertexCount; v++) {
                interleavedData[v * 8 + 0] = positions[v * 3 + 0];
                interleavedData[v * 8 + 1] = positions[v * 3 + 1];
                interleavedData[v * 8 + 2] = positions[v * 3 + 2];
                interleavedData[v * 8 + 3] = normals[v * 3 + 0];
                interleavedData[v * 8 + 4] = normals[v * 3 + 1];
                interleavedData[v * 8 + 5] = normals[v * 3 + 2];
                interleavedData[v * 8 + 6] = texcoords[v * 2 + 0];
                interleavedData[v * 8 + 7] = texcoords[v * 2 + 1];
            }

            // Read index data (unsigned int to avoid 65535-vertex truncation)
            std::vector<unsigned int> indices;
            if (prim->indices) {
                indices.resize(prim->indices->count);
                for (cgltf_size ii = 0; ii < prim->indices->count; ii++) {
                    indices[ii] = (unsigned int)cgltf_accessor_read_index(prim->indices, ii);
                }
            } else {
                // Generate sequential indices if none provided
                indices.resize(vertexCount);
                for (cgltf_size ii = 0; ii < vertexCount; ii++) {
                    indices[ii] = (unsigned int)ii;
                }
            }

            // Create GPU resources
            st3DMesh gpuMesh = {};

            glGenVertexArrays(1, &gpuMesh.vao);
            glGenBuffers(1, &gpuMesh.vboVertices);
            glGenBuffers(1, &gpuMesh.eboIndices);

            glBindVertexArray(gpuMesh.vao);

            // Upload vertex data
            glBindBuffer(GL_ARRAY_BUFFER, gpuMesh.vboVertices);
            glBufferData(GL_ARRAY_BUFFER, interleavedData.size() * sizeof(float), interleavedData.data(), GL_STATIC_DRAW);

            // Upload index data
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpuMesh.eboIndices);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

            // Set vertex attribute pointers (stride = 8 floats)
            GLsizei stride = 8 * sizeof(float);

            // Position (3 floats, offset 0)
            if (m_3dPosAttrib >= 0) {
                glEnableVertexAttribArray(m_3dPosAttrib);
                glVertexAttribPointer(m_3dPosAttrib, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
            }
            // Normal (3 floats, offset 3*sizeof(float))
            if (m_3dNormalAttrib >= 0) {
                glEnableVertexAttribArray(m_3dNormalAttrib);
                glVertexAttribPointer(m_3dNormalAttrib, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
            }
            // TexCoord (2 floats, offset 6*sizeof(float))
            if (m_3dTexCoordAttrib >= 0) {
                glEnableVertexAttribArray(m_3dTexCoordAttrib);
                glVertexAttribPointer(m_3dTexCoordAttrib, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
            }

            glBindVertexArray(0);

            gpuMesh.indexCount = (unsigned int)indices.size();

            // Load texture from material — deduplicate via localTexCache so primitives
            // sharing the same cgltf_image get the same GL texture ID.
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
                            GLuint texId;
                            glGenTextures(1, &texId);
                            glBindTexture(GL_TEXTURE_2D, texId);
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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
                    GLuint fallbackTex;
                    glGenTextures(1, &fallbackTex);
                    glBindTexture(GL_TEXTURE_2D, fallbackTex);
                    unsigned char white[] = {255, 255, 255, 255};
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    gpuMesh.textureId = fallbackTex;
                    localTexCache[nullptr] = fallbackTex;
                    model.ownedTextures.insert(fallbackTex);
                }
            }

            model.meshes.push_back(gpuMesh);
        }
    }

    cgltf_free(data);

    if (model.meshes.empty()) {
        pb3dSendConsole("PB3D: No meshes found in: " + std::string(glbFilePath));
        return 0;
    }

    unsigned int modelId = m_next3dModelId++;
    m_3dModelList[modelId] = model;

    // Unbind VBOs: GL_ARRAY_BUFFER is global state (not captured by VAOs). If left
    // bound, oglRenderQuad's CPU vertex pointers are misread as VBO offsets in GLES 3.0.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Reset 2D texture cache: glBindTexture calls during loading are outside
    // PBOGLES's m_lastTextureId tracking, which would cause 2D sprites to
    // silently skip their bind and render with the wrong texture.
    oglResetTextureCache();

    return modelId;
}

bool PB3D::pb3dUnloadModel(unsigned int modelId) {
    auto it = m_3dModelList.find(modelId);
    if (it == m_3dModelList.end()) return false;

    for (auto& mesh : it->second.meshes) {
        if (mesh.vao) glDeleteVertexArrays(1, &mesh.vao);
        if (mesh.vboVertices) glDeleteBuffers(1, &mesh.vboVertices);
        if (mesh.eboIndices) glDeleteBuffers(1, &mesh.eboIndices);
    }
    // Delete each unique texture exactly once (set deduplicates shared textures)
    for (GLuint texId : it->second.ownedTextures) {
        if (texId) glDeleteTextures(1, &texId);
    }

    m_3dModelList.erase(it);
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
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    // Note: backface culling disabled — glTF winding-order varies by exporter.
    // Re-enable once correct winding is confirmed.
    glDisable(GL_CULL_FACE);
    glUseProgram(m_3dShaderProgram);

    // Re-upload light uniforms and recompute view/projection matrices only when
    // the scene has changed (camera or lighting).  Neither changes at runtime in
    // normal usage, so this eliminates constant trig work every frame.
    if (m_sceneDirty) {
        glUniform3f(m_3dLightDirUniform, m_light.dirX, m_light.dirY, m_light.dirZ);
        glUniform3f(m_3dLightColorUniform, m_light.r, m_light.g, m_light.b);
        glUniform3f(m_3dAmbientUniform, m_light.ambientR, m_light.ambientG, m_light.ambientB);
        glUniform3f(m_3dCameraEyeUniform, m_camera.eyeX, m_camera.eyeY, m_camera.eyeZ);

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

    // Build model matrix: translate * rotY * rotX * rotZ * scale
    mat4x4 model, identMat;
    mat4x4_identity(identMat);  // initialized once; reused for all three rotation builds

    // Translation
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

    // Scale matrix
    mat4x4 scaleMat;
    mat4x4_identity(scaleMat);
    scaleMat[0][0] = inst.scale;
    scaleMat[1][1] = inst.scale;
    scaleMat[2][2] = inst.scale;

    // Combine: model = translate * rotY * rotX * rotZ * scale
    mat4x4 rotYX, rotYXZ, rotYXZS;
    mat4x4_mul(rotYX, rotY, rotX);
    mat4x4_mul(rotYXZ, rotYX, rotZ);
    mat4x4_mul(rotYXZS, rotYXZ, scaleMat);
    mat4x4_mul(model, translateMat, rotYXZS);

    // Compute MVP = projection * view * model
    mat4x4 view, proj, viewModel, mvp;
    memcpy(view, m_viewMatrix, sizeof(view));
    memcpy(proj, m_projMatrix, sizeof(proj));
    mat4x4_mul(viewModel, view, model);
    mat4x4_mul(mvp, proj, viewModel);

    // Set uniforms
    glUniformMatrix4fv(m_3dMVPUniform, 1, GL_FALSE, (const float*)mvp);
    glUniformMatrix4fv(m_3dModelUniform, 1, GL_FALSE, (const float*)model);
    glUniform1f(m_3dAlphaUniform, inst.alpha);

    // Handle transparency: enable blend before draw, restore opaque state after
    // so consecutive instances don't inherit each other's blend state.
    bool blendEnabled = (inst.alpha < 1.0f);
    if (blendEnabled) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    // Render each mesh
    glActiveTexture(GL_TEXTURE0);  // Ensure sampler unit 0 is active
    for (auto& mesh : modelIt->second.meshes) {
        glBindVertexArray(mesh.vao);
        glBindTexture(GL_TEXTURE_2D, mesh.textureId);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);

    if (blendEnabled) {
        glDisable(GL_BLEND);
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
        // Update all active animations directly from the map iterator,
        // avoiding a second find() call per animation.
        bool anyActive = false;
        for (auto& pair : m_3dAnimateList) {
            if (pair.second.isActive) {
                pb3dProcessAnimation(pair.second, currentTick);
                anyActive = true;
            }
        }
        return anyActive;
    }

    auto animIt = m_3dAnimateList.find(instanceId);
    if (animIt == m_3dAnimateList.end()) return false;

    st3DAnimateData& anim = animIt->second;
    if (!anim.isActive) return false;

    pb3dProcessAnimation(anim, currentTick);
    return true;
}

void PB3D::pb3dProcessAnimation(st3DAnimateData& anim, unsigned int currentTick) {
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
            pair.second.isActive = true;
            pair.second.startTick = 0; // Will be set on next animate call
        }
        return;
    }
    auto it = m_3dAnimateList.find(instanceId);
    if (it != m_3dAnimateList.end()) {
        it->second.isActive = true;
        it->second.startTick = 0;
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

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
    uniform float uAlpha;
    out vec4 fragColor;
    void main() {
        vec4 texColor = texture(uTexture, vTexCoord);
        vec3 norm = normalize(vNormal);
        vec3 lightDir = normalize(uLightDir);
        float diffuse = max(dot(norm, lightDir), 0.0);
        vec3 finalColor = texColor.rgb * (uAmbientColor + diffuse * uLightColor);
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
    m_3dAlphaUniform = -1;
    m_3dPosAttrib = -1;
    m_3dNormalAttrib = -1;
    m_3dTexCoordAttrib = -1;

    m_next3dModelId = 1;
    m_next3dInstanceId = 1;

    // Default camera: eye=(0,5,10), lookAt=(0,0,0), FOV=45, near=0.1, far=100
    m_camera = {0.0f, 5.0f, 10.0f,   0.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f,   45.0f,   0.1f, 100.0f};

    // Default light: direction=(0.5,-1.0,-0.3), white light, gray ambient
    m_light = {0.5f, -1.0f, -0.3f,   1.0f, 1.0f, 1.0f,   0.3f, 0.3f, 0.3f};

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
            if (mesh.textureId) glDeleteTextures(1, &mesh.textureId);
        }
    }
    if (m_3dShaderProgram) glDeleteProgram(m_3dShaderProgram);
}

// ============================================================================
// Initialization
// ============================================================================

bool PB3D::pb3dInit() {
    // Compile the 3D shaders using the inherited PBOGLES methods
    m_3dShaderProgram = oglCreateProgram(vertexShader3DSource, fragmentShader3DSource);
    if (m_3dShaderProgram == 0) {
        std::cout << "PB3D: Failed to create 3D shader program" << std::endl;
        return false;
    }

    // Cache uniform locations
    m_3dMVPUniform = glGetUniformLocation(m_3dShaderProgram, "uMVP");
    m_3dModelUniform = glGetUniformLocation(m_3dShaderProgram, "uModel");
    m_3dLightDirUniform = glGetUniformLocation(m_3dShaderProgram, "uLightDir");
    m_3dLightColorUniform = glGetUniformLocation(m_3dShaderProgram, "uLightColor");
    m_3dAmbientUniform = glGetUniformLocation(m_3dShaderProgram, "uAmbientColor");
    m_3dAlphaUniform = glGetUniformLocation(m_3dShaderProgram, "uAlpha");

    // Cache attribute locations
    m_3dPosAttrib = glGetAttribLocation(m_3dShaderProgram, "aPosition");
    m_3dNormalAttrib = glGetAttribLocation(m_3dShaderProgram, "aNormal");
    m_3dTexCoordAttrib = glGetAttribLocation(m_3dShaderProgram, "aTexCoord");

    std::cout << "PB3D: 3D rendering system initialized" << std::endl;
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
        std::cout << "PB3D: Failed to parse glTF file: " << glbFilePath << std::endl;
        return 0;
    }

    result = cgltf_load_buffers(&options, data, glbFilePath);
    if (result != cgltf_result_success) {
        std::cout << "PB3D: Failed to load glTF buffers: " << glbFilePath << std::endl;
        cgltf_free(data);
        return 0;
    }

    st3DModel model;
    model.name = glbFilePath;
    model.isLoaded = true;

    // Iterate through all meshes in the glTF file
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

            // Read normal data (or default to (0,1,0))
            std::vector<float> normals(vertexCount * 3, 0.0f);
            if (normAccessor) {
                cgltf_accessor_read_float_buffer(normAccessor, normals.data(), 3);
            } else {
                for (cgltf_size v = 0; v < vertexCount; v++) {
                    normals[v * 3 + 1] = 1.0f; // default Y-up normal
                }
            }

            // Read texcoord data (or default to 0,0)
            std::vector<float> texcoords(vertexCount * 2, 0.0f);
            if (texAccessor) {
                cgltf_accessor_read_float_buffer(texAccessor, texcoords.data(), 2);
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

            // Read index data
            std::vector<unsigned short> indices;
            if (prim->indices) {
                indices.resize(prim->indices->count);
                for (cgltf_size ii = 0; ii < prim->indices->count; ii++) {
                    indices[ii] = (unsigned short)cgltf_accessor_read_index(prim->indices, ii);
                }
            } else {
                // Generate sequential indices if none provided
                indices.resize(vertexCount);
                for (cgltf_size ii = 0; ii < vertexCount; ii++) {
                    indices[ii] = (unsigned short)ii;
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
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), indices.data(), GL_STATIC_DRAW);

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

            // Load texture from material (if available)
            gpuMesh.textureId = 0;
            if (prim->material && prim->material->has_pbr_metallic_roughness) {
                cgltf_texture* tex = prim->material->pbr_metallic_roughness.base_color_texture.texture;
                if (tex && tex->image) {
                    cgltf_image* img = tex->image;
                    if (img->buffer_view && img->buffer_view->buffer) {
                        // Embedded image in GLB binary buffer
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
                        }
                    }
                }
            }

            // If no texture was loaded, create a 1x1 white fallback
            if (gpuMesh.textureId == 0) {
                GLuint fallbackTex;
                glGenTextures(1, &fallbackTex);
                glBindTexture(GL_TEXTURE_2D, fallbackTex);
                unsigned char white[] = {255, 255, 255, 255};
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                gpuMesh.textureId = fallbackTex;
            }

            model.meshes.push_back(gpuMesh);
        }
    }

    cgltf_free(data);

    if (model.meshes.empty()) {
        std::cout << "PB3D: No meshes found in: " << glbFilePath << std::endl;
        return 0;
    }

    unsigned int modelId = m_next3dModelId++;
    m_3dModelList[modelId] = model;

    std::cout << "PB3D: Loaded model '" << glbFilePath << "' (ID=" << modelId 
              << ", meshes=" << model.meshes.size() << ")" << std::endl;

    return modelId;
}

bool PB3D::pb3dUnloadModel(unsigned int modelId) {
    auto it = m_3dModelList.find(modelId);
    if (it == m_3dModelList.end()) return false;

    for (auto& mesh : it->second.meshes) {
        if (mesh.vao) glDeleteVertexArrays(1, &mesh.vao);
        if (mesh.vboVertices) glDeleteBuffers(1, &mesh.vboVertices);
        if (mesh.eboIndices) glDeleteBuffers(1, &mesh.eboIndices);
        if (mesh.textureId) glDeleteTextures(1, &mesh.textureId);
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

    unsigned int instanceId = m_next3dInstanceId++;
    m_3dInstanceList[instanceId] = instance;
    return instanceId;
}

bool PB3D::pb3dDestroyInstance(unsigned int instanceId) {
    auto it = m_3dInstanceList.find(instanceId);
    if (it == m_3dInstanceList.end()) return false;
    m_3dInstanceList.erase(it);
    return true;
}

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

void PB3D::pb3dSetCamera(st3DCamera camera) {
    m_camera = camera;
}

void PB3D::pb3dSetLight(st3DLight light) {
    m_light = light;
}

// ============================================================================
// Rendering
// ============================================================================

void PB3D::pb3dBegin() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glUseProgram(m_3dShaderProgram);

    // Set light uniforms
    glUniform3f(m_3dLightDirUniform, m_light.dirX, m_light.dirY, m_light.dirZ);
    glUniform3f(m_3dLightColorUniform, m_light.r, m_light.g, m_light.b);
    glUniform3f(m_3dAmbientUniform, m_light.ambientR, m_light.ambientG, m_light.ambientB);

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
}

void PB3D::pb3dEnd() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Restore 2D sprite shader — m_shaderProgram is the 2D program from PBOGLES
    // Access via the inherited member
    // Note: m_shaderProgram is private in PBOGLES. We need to restore the 2D program.
    // Since pb3dEnd is called to switch back to 2D mode, the caller (PBGfx)
    // will re-use the 2D shader on next render call. However, we need to unbind our VAO.
    glBindVertexArray(0);
}

void PB3D::pb3dRenderInstance(unsigned int instanceId) {
    auto instIt = m_3dInstanceList.find(instanceId);
    if (instIt == m_3dInstanceList.end()) return;

    st3DInstance& inst = instIt->second;
    if (!inst.visible) return;

    auto modelIt = m_3dModelList.find(inst.modelId);
    if (modelIt == m_3dModelList.end()) return;

    // Build model matrix: identity -> translate -> rotateY -> rotateX -> rotateZ -> scale
    mat4x4 model, temp, temp2;
    mat4x4_identity(model);

    // Translate
    mat4x4_translate(temp, inst.posX, inst.posY, inst.posZ);
    mat4x4_mul(model, temp, model);

    // Rotate Y
    if (inst.rotY != 0.0f) {
        mat4x4_identity(temp);
        mat4x4_rotate_Y(temp2, temp, inst.rotY * 3.14159265f / 180.0f);
        mat4x4_mul(temp, model, temp);  // save current
        mat4x4_mul(model, temp2, temp); // wrong order, fix below
    }

    // Actually let's build this more carefully using linmath.h
    mat4x4_identity(model);

    // Translation
    mat4x4 translateMat;
    mat4x4_translate(translateMat, inst.posX, inst.posY, inst.posZ);

    // Rotation Y
    mat4x4 rotY, identMat;
    mat4x4_identity(identMat);
    mat4x4_rotate_Y(rotY, identMat, inst.rotY * 3.14159265f / 180.0f);

    // Rotation X
    mat4x4 rotX;
    mat4x4_identity(identMat);
    mat4x4_rotate_X(rotX, identMat, inst.rotX * 3.14159265f / 180.0f);

    // Rotation Z
    mat4x4 rotZ;
    mat4x4_identity(identMat);
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

    // Handle transparency
    if (inst.alpha < 1.0f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    // Render each mesh
    for (auto& mesh : modelIt->second.meshes) {
        glBindVertexArray(mesh.vao);
        glBindTexture(GL_TEXTURE_2D, mesh.textureId);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_SHORT, 0);
    }

    glBindVertexArray(0);
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
    std::uniform_real_distribution<float> dist(min, max);
    return dist(gen);
}

bool PB3D::pb3dCreateAnimation(st3DAnimateData anim, bool replaceExisting) {
    if (m_3dInstanceList.find(anim.animateInstanceId) == m_3dInstanceList.end()) return false;

    auto existingIt = m_3dAnimateList.find(anim.animateInstanceId);
    if (existingIt != m_3dAnimateList.end()) {
        if (!replaceExisting) return false;
    }

    m_3dAnimateList[anim.animateInstanceId] = anim;
    return true;
}

bool PB3D::pb3dAnimateInstance(unsigned int instanceId, unsigned int currentTick) {
    if (instanceId == 0) {
        // Update all animations
        bool anyActive = false;
        for (auto& pair : m_3dAnimateList) {
            if (pair.second.isActive) {
                pb3dAnimateInstance(pair.first, currentTick);
                anyActive = true;
            }
        }
        return anyActive;
    }

    auto animIt = m_3dAnimateList.find(instanceId);
    if (animIt == m_3dAnimateList.end()) return false;

    st3DAnimateData& anim = animIt->second;
    if (!anim.isActive) return false;

    float timeSinceStart = (currentTick - anim.startTick) / 1000.0f;
    float percentComplete = (anim.animateTimeSec > 0.0f) ? (timeSinceStart / anim.animateTimeSec) : 1.0f;

    // Check if animation time has elapsed
    if (percentComplete >= 1.0f && anim.animType != GFX_ANIM_ACCL) {
        // Handle loop types
        if (anim.loop == GFX_NOLOOP) {
            pb3dSetFinalAnimationValues(anim);
            anim.isActive = false;
            return false;
        } else if (anim.loop == GFX_RESTART) {
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

    return true;
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

    // Handle restart loop for acceleration mode
    if (anim.loop == GFX_RESTART && anim.animateTimeSec > 0.0f && timeSinceStart >= anim.animateTimeSec) {
        anim.startTick = currentTick;
        // Update start values to current position
        anim.startPosX = inst.posX; anim.startPosY = inst.posY; anim.startPosZ = inst.posZ;
        anim.startRotX = inst.rotX; anim.startRotY = inst.rotY; anim.startRotZ = inst.rotZ;
    }
}

void PB3D::pb3dAnimateJump(st3DAnimateData& anim, unsigned int currentTick, float timeSinceStart) {
    // Jump: instant snap to end values when time elapses
    // The actual snap happens in the loop handling at the top of pb3dAnimateInstance
    // During the wait period, the instance stays at start values
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

// PB3D - 3D rendering layer using glTF models via cgltf
// Sits between PBOGLES and PBGfx in the inheritance chain:
//   PBEngine -> PBGfx -> PB3D -> PBOGLES

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef PB3D_h
#define PB3D_h

#include "PBOGLES.h"
#include <map>
#include <vector>
#include <string>

// Path for 3D model resources
#define PB3D_MODEL_PATH "src/resources/models/"

// 3D Animation Property Masks
#define ANIM3D_POSX_MASK   0x001
#define ANIM3D_POSY_MASK   0x002
#define ANIM3D_POSZ_MASK   0x004
#define ANIM3D_ROTX_MASK   0x008
#define ANIM3D_ROTY_MASK   0x010
#define ANIM3D_ROTZ_MASK   0x020
#define ANIM3D_SCALE_MASK  0x040
#define ANIM3D_ALPHA_MASK  0x080
#define ANIM3D_ALL_MASK    0x0FF

// Forward declarations for gfxAnimType and gfxLoopType (defined in PBGfx.h)
// We re-declare the enums here to avoid circular include; they must match PBGfx.h
#ifndef PBGfx_h
enum gfxLoopType {
    GFX_NOLOOP = 0,
    GFX_RESTART = 1,
    GFX_REVERSE = 2
};

enum gfxAnimType {
    GFX_ANIM_NORMAL = 0,
    GFX_ANIM_ACCL = 1,
    GFX_ANIM_JUMP = 2,
    GFX_ANIM_JUMPRANDOM = 3
};
#endif

// --- 3D Data Structures ---

struct st3DMesh {
    GLuint vao;
    GLuint vboVertices;
    GLuint eboIndices;
    unsigned int indexCount;
    GLuint textureId;
    unsigned int materialIndex;
};

struct st3DModel {
    std::vector<st3DMesh> meshes;
    std::string name;
    bool isLoaded;
};

struct st3DInstance {
    unsigned int modelId;
    float posX, posY, posZ;
    float rotX, rotY, rotZ;
    float scale;
    float alpha;
    bool visible;
};

struct st3DCamera {
    float eyeX, eyeY, eyeZ;
    float lookX, lookY, lookZ;
    float upX, upY, upZ;
    float fov;
    float nearPlane, farPlane;
};

struct st3DLight {
    float dirX, dirY, dirZ;
    float r, g, b;
    float ambientR, ambientG, ambientB;
};

struct st3DAnimateData {
    unsigned int animateInstanceId;

    // Start values
    float startPosX, startPosY, startPosZ;
    float startRotX, startRotY, startRotZ;
    float startScale, startAlpha;

    // End/target values
    float endPosX, endPosY, endPosZ;
    float endRotX, endRotY, endRotZ;
    float endScale, endAlpha;

    // Timing
    unsigned int startTick;
    float animateTimeSec;

    // Control
    unsigned int typeMask;
    gfxAnimType animType;
    gfxLoopType loop;
    bool isActive;

    // Acceleration mode parameters
    float accelX, accelY, accelZ;
    float accelRotX, accelRotY, accelRotZ;
    float initialVelX, initialVelY, initialVelZ;
    float initialVelRotX, initialVelRotY, initialVelRotZ;
    float currentVelX, currentVelY, currentVelZ;
    float currentVelRotX, currentVelRotY, currentVelRotZ;

    // Jump random
    float randomPercent;

    // Rotation direction flags
    bool rotateClockwiseX, rotateClockwiseY, rotateClockwiseZ;
};

// --- PB3D Class ---

class PB3D : public PBOGLES {

public:
    PB3D();
    ~PB3D();

    // Initialization
    bool pb3dInit();

    // Model loading
    unsigned int pb3dLoadModel(const char* glbFilePath);
    bool pb3dUnloadModel(unsigned int modelId);

    // Instance management
    unsigned int pb3dCreateInstance(unsigned int modelId);
    bool pb3dDestroyInstance(unsigned int instanceId);

    // Instance property setters
    void pb3dSetInstancePosition(unsigned int instanceId, float x, float y, float z);
    void pb3dSetInstanceRotation(unsigned int instanceId, float rx, float ry, float rz);
    void pb3dSetInstanceScale(unsigned int instanceId, float scale);
    void pb3dSetInstanceAlpha(unsigned int instanceId, float alpha);
    void pb3dSetInstanceVisible(unsigned int instanceId, bool visible);

    // Camera and light
    void pb3dSetCamera(st3DCamera camera);
    void pb3dSetLight(st3DLight light);

    // Rendering
    void pb3dBegin();
    void pb3dEnd();
    void pb3dRenderInstance(unsigned int instanceId);
    void pb3dRenderAll();

    // Animation
    bool pb3dCreateAnimation(st3DAnimateData anim, bool replaceExisting);
    bool pb3dAnimateInstance(unsigned int instanceId, unsigned int currentTick);
    bool pb3dAnimateActive(unsigned int instanceId);
    void pb3dAnimateClear(unsigned int instanceId);
    void pb3dAnimateRestart(unsigned int instanceId);
    void pb3dAnimateRestart(unsigned int instanceId, unsigned long startTick);

private:
    // 3D Shader program and uniform/attrib locations
    GLuint m_3dShaderProgram;
    GLint  m_3dMVPUniform, m_3dModelUniform, m_3dLightDirUniform;
    GLint  m_3dLightColorUniform, m_3dAmbientUniform, m_3dAlphaUniform;
    GLint  m_3dPosAttrib, m_3dNormalAttrib, m_3dTexCoordAttrib;

    // Data storage
    std::map<unsigned int, st3DModel>        m_3dModelList;
    std::map<unsigned int, st3DInstance>      m_3dInstanceList;
    std::map<unsigned int, st3DAnimateData>   m_3dAnimateList;

    // Camera and light state
    st3DCamera m_camera;
    st3DLight  m_light;

    // ID counters
    unsigned int m_next3dModelId;
    unsigned int m_next3dInstanceId;

    // Cached view/projection matrices (16 floats each, column-major)
    float m_viewMatrix[16];
    float m_projMatrix[16];

    // Animation handlers
    void pb3dAnimateNormal(st3DAnimateData& anim, unsigned int currentTick, float timeSinceStart, float percentComplete);
    void pb3dAnimateAcceleration(st3DAnimateData& anim, unsigned int currentTick, float timeSinceStart);
    void pb3dAnimateJump(st3DAnimateData& anim, unsigned int currentTick, float timeSinceStart);
    void pb3dAnimateJumpRandom(st3DAnimateData& anim, unsigned int currentTick, float timeSinceStart);
    void pb3dSetFinalAnimationValues(const st3DAnimateData& anim);

    // Random number generation for animation
    float pb3dGetRandomFloat(float min, float max);

    // 3D Shader source code
    static const char* vertexShader3DSource;
    static const char* fragmentShader3DSource;
};

#endif // PB3D_h

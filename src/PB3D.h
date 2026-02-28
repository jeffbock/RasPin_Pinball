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
#include <set>
#include <vector>
#include <string>

// Path for 3D model resources
#define PB3D_MODEL_PATH "src/resources/3d/"

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

// Animation loop and type enums (shared between 2D and 3D animation systems)
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
    std::set<GLuint> ownedTextures;  // unique GL texture IDs owned by this model (ref-safe cleanup)
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
    // Pixel-space anchor: when set, a Z-depth perspective correction is applied
    // each frame as an additive delta so the object doesn't drift toward center
    // as Z changes. XY animation (e.g. JUMPRANDOM jitter) is preserved.
    bool  hasPixelAnchor;
    float anchorPixelX, anchorPixelY;   // screen pixels — the intended screen position
    float anchorBaseX,  anchorBaseY;    // world X/Y computed at Z=0 (reference frame)
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

    // Start values — world units.  If usePxCoords=true, startPxX/Y / endPxX/Y
    // are used instead for X and Y (converted to world units at pb3dCreateAnimation time).
    // Z, rotation, scale, alpha always use world-unit / degree / 0-1 values.
    bool usePxCoords;           // When true, startPxX/Y and endPxX/Y are in screen pixels
    float startPxX, startPxY;   // Pixel-space start position (X/Y only)
    float endPxX,   endPxY;     // Pixel-space end   position (X/Y only)

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

    // Initialization (called once from gfxInit)
    bool pb3dInit();

    // -----------------------------------------------------------------------
    // Model loading
    // -----------------------------------------------------------------------
    unsigned int pb3dLoadModel(const char* glbFilePath);
    bool         pb3dUnloadModel(unsigned int modelId);

    // -----------------------------------------------------------------------
    // Instance management
    // -----------------------------------------------------------------------
    unsigned int pb3dCreateInstance(unsigned int modelId);
    bool         pb3dDestroyInstance(unsigned int instanceId);

    // -----------------------------------------------------------------------
    // Instance properties  — all positions are in SCREEN PIXELS.
    // depthZ is a relative depth offset from the screen plane:
    //   0.0  = at the screen surface (default)
    //  >0.0  = closer to the viewer (in front)
    //  <0.0  = further away (behind)
    // -----------------------------------------------------------------------
    void pb3dSetInstancePositionPx(unsigned int instanceId, float pixelX, float pixelY);                    // Z = 0
    void pb3dSetInstancePositionPx(unsigned int instanceId, float pixelX, float pixelY, float depthZ);      // Z = offset
    void pb3dSetInstanceRotation(unsigned int instanceId, float rx, float ry, float rz);  // degrees
    void pb3dSetInstanceScale(unsigned int instanceId, float scale);   // 1.0 = native size
    void pb3dSetInstanceAlpha(unsigned int instanceId, float alpha);   // 0.0 – 1.0
    void pb3dSetInstanceVisible(unsigned int instanceId, bool visible);

    // -----------------------------------------------------------------------
    // Lighting  (direction is in normalised world-space, colors are 0.0–1.0)
    // -----------------------------------------------------------------------
    void pb3dSetLightDirection(float x, float y, float z);
    void pb3dSetLightColor(float r, float g, float b);
    void pb3dSetLightAmbient(float r, float g, float b);

    // -----------------------------------------------------------------------
    // Rendering
    // -----------------------------------------------------------------------
    void pb3dBegin();
    void pb3dEnd();
    void pb3dRenderInstance(unsigned int instanceId);
    void pb3dRenderAll();

    // -----------------------------------------------------------------------
    // Animation
    // -----------------------------------------------------------------------
    bool pb3dCreateAnimation(st3DAnimateData anim, bool replaceExisting);
    bool pb3dAnimateInstance(unsigned int instanceId, unsigned int currentTick);
    bool pb3dAnimateActive(unsigned int instanceId);
    void pb3dAnimateClear(unsigned int instanceId);
    void pb3dAnimateRestart(unsigned int instanceId);
    void pb3dAnimateRestart(unsigned int instanceId, unsigned long startTick);

protected:
    // Console output — default uses stdout; overridden in PBEngine to route to the on-screen console
    virtual void pb3dSendConsole(const std::string& msg);

private:
    // 3D Shader program and uniform/attrib locations
    GLuint m_3dShaderProgram;
    GLint  m_3dMVPUniform, m_3dModelUniform, m_3dLightDirUniform;
    GLint  m_3dLightColorUniform, m_3dAmbientUniform, m_3dCameraEyeUniform, m_3dAlphaUniform;
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

    // Dirty flag: set whenever camera or lighting changes; cleared after upload in pb3dBegin()
    bool m_sceneDirty;

    // Animation handlers
    void pb3dProcessAnimation(st3DAnimateData& anim, unsigned int currentTick);
    void pb3dAnimateNormal(st3DAnimateData& anim, unsigned int currentTick, float timeSinceStart, float percentComplete);
    void pb3dAnimateAcceleration(st3DAnimateData& anim, unsigned int currentTick, float timeSinceStart);
    void pb3dAnimateJump(st3DAnimateData& anim, unsigned int currentTick, float timeSinceStart);
    void pb3dAnimateJumpRandom(st3DAnimateData& anim, unsigned int currentTick, float timeSinceStart);
    void pb3dSetFinalAnimationValues(const st3DAnimateData& anim);

    // Internal position setters
    void pb3dSetInstancePosition(unsigned int instanceId, float x, float y, float z);  // world units — internal only
    void pb3dSetInstancePositionPxImpl(unsigned int instanceId, float pixelX, float pixelY, float depthZ);
    void pb3dPixelToWorld(float pixelX, float pixelY, float depthZ, float& outX, float& outY);
    void pb3dSetCamera(st3DCamera camera);  // internal — camera is managed automatically

    // Random number generation for animation
    float pb3dGetRandomFloat(float min, float max);

    // 3D Shader source code
    static const char* vertexShader3DSource;
    static const char* fragmentShader3DSource;
};

#endif // PB3D_h

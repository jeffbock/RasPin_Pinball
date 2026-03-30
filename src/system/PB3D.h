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

// Maximum bones per skinned mesh uploaded to the GPU shader
#define PB3D_MAX_BONES 64

// Path for 3D model resources
#define PB3D_MODEL_PATH "src/user/resources/3d/"

// ============================================================================
// Skeleton Animation Data Structures
// ============================================================================

// Interpolation type for animation keyframes (mirrors glTF interpolation types)
enum e3DInterpolationType {
    ANIM_INTERP_LINEAR      = 0,
    ANIM_INTERP_STEP        = 1,
    ANIM_INTERP_CUBICSPLINE = 2
};

// Target property for an animation channel
enum e3DAnimChannelType {
    ANIM_CHANNEL_TRANSLATION = 0,
    ANIM_CHANNEL_ROTATION    = 1,
    ANIM_CHANNEL_SCALE       = 2
};

// A single keyframe value: xyz0 for translation/scale, xyzw quaternion for rotation
struct st3DAnimKeyframe {
    float time;      // seconds from start of clip
    float value[4];  // translation: xyz0  |  rotation: xyzw quat  |  scale: xyz0
};

// One animation channel drives a single property on a single bone
struct st3DAnimChannel {
    int boneIndex;
    e3DAnimChannelType type;
    e3DInterpolationType interpolation;
    std::vector<st3DAnimKeyframe> keyframes;
};

// One named animation clip (corresponds to one glTF animation)
struct st3DAnimClip {
    std::string name;
    float duration;   // seconds (max input sampler time)
    std::vector<st3DAnimChannel> channels;
};

// One bone in the skeleton hierarchy
struct st3DBone {
    std::string name;
    int parentIndex;             // -1 for root bone(s)
    float inverseBindMatrix[16]; // column-major 4×4 — transforms from model to bone space
    float restTranslation[3];    // rest-pose translation from glTF node
    float restRotation[4];       // rest-pose rotation quaternion xyzw from glTF node
    float restScale[3];          // rest-pose scale from glTF node
};

// Skeleton: bone hierarchy + all animation clips for a model
struct st3DSkeleton {
    std::vector<st3DBone>    bones;
    std::vector<st3DAnimClip> clips;
};

// Per-instance skeleton animation playback state
struct st3DSkelState {
    int   clipIndex;                               // index into model's skeleton.clips, -1 = none
    float currentTime;                             // playback position in seconds
    bool  looping;
    bool  isPlaying;
    unsigned int lastUpdateTick;                   // millisecond tick of last update
    float boneMatrices[PB3D_MAX_BONES * 16];       // final skinning matrices (flattened column-major)
};

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
    unsigned int vao;
    unsigned int vboVertices;
    unsigned int eboIndices;
    unsigned int indexCount;
    unsigned int textureId;
    unsigned int materialIndex;
    bool isSkinned;  // true when JOINTS_0/WEIGHTS_0 were present (uses skinned VAO layout)
};

struct st3DModel {
    std::vector<st3DMesh> meshes;
    std::set<unsigned int> ownedTextures;  // unique GPU texture handles owned by this model (ref-safe cleanup)
    std::string name;
    bool isLoaded;
    bool hasSkeleton;    // true when skin + bone hierarchy was loaded from the glTF
    st3DSkeleton skeleton;
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
    // Skeleton animation state (only meaningful when the model has a skeleton)
    st3DSkelState skelState;
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
    // Skeleton animation API
    // -----------------------------------------------------------------------
    // Query available clips for a loaded model (returns empty vector if none)
    std::vector<std::string> pb3dListAnimClips(unsigned int modelId);
    int  pb3dFindAnimClip(unsigned int modelId, const std::string& clipName);

    // Play a skeleton animation clip on an instance (-1 clipIndex or unknown name stops)
    bool pb3dPlayAnimClip(unsigned int instanceId, const std::string& clipName, bool loop = true);
    bool pb3dPlayAnimClip(unsigned int instanceId, int clipIndex, bool loop = true);
    bool pb3dStopAnimClip(unsigned int instanceId);
    bool pb3dIsAnimClipPlaying(unsigned int instanceId) const;
    float pb3dGetAnimClipTime(unsigned int instanceId) const;  // current playback time in seconds
    bool pb3dSetAnimClipTime(unsigned int instanceId, float timeSec);

    // -----------------------------------------------------------------------
    // Animation (high-level transform animation — position/rotation/scale/alpha)
    // -----------------------------------------------------------------------
    bool pb3dCreateAnimation(st3DAnimateData anim, bool replaceExisting);
    bool pb3dAnimateInstance(unsigned int instanceId, unsigned int currentTick);
    bool pb3dAnimateActive(unsigned int instanceId);
    void pb3dAnimateClear(unsigned int instanceId);
    void pb3dAnimateRestart(unsigned int instanceId);
    void pb3dAnimateRestart(unsigned int instanceId, unsigned long startTick);

    // Convenience fade helpers — start a GFX_ANIM_NORMAL / GFX_NOLOOP animation
    // that interpolates instance alpha 0→1 (fade in) or 1→0 (fade out).
    void pb3dFadeIn (unsigned int instanceId, float timeSec, unsigned long startTick);
    void pb3dFadeOut(unsigned int instanceId, float timeSec, unsigned long startTick);

protected:
    // Console output — default uses stdout; overridden in PBEngine to route to the on-screen console
    virtual void pb3dSendConsole(const std::string& msg);

private:
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

    // Tracks whether the skinned shader is currently active (for mid-frame switching)
    bool m_skinnedShaderActive;

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

    // Skeleton animation helpers
    void  pb3dUpdateSkelState(st3DInstance& inst, const st3DSkeleton& skel, unsigned int currentTick);
    void  pb3dComputeBoneMatrices(const st3DSkeleton& skel, const st3DAnimClip& clip,
                                  float time, float outMatrices[PB3D_MAX_BONES * 16]);
    float pb3dSkelInterpolateFloat(float a, float b, float t);
    void  pb3dSkelSlerpQuat(const float qa[4], const float qb[4], float t, float out[4]);
    void  pb3dSkelEvalChannel(const st3DAnimChannel& ch, float time, float out[4]);
    void  pb3dMat4Mul(const float a[16], const float b[16], float out[16]);
    void  pb3dMat4FromTRS(const float t[3], const float r[4], const float s[3], float out[16]);

    // 3D Shader source code
    static const char* vertexShader3DSource;
    static const char* fragmentShader3DSource;
    static const char* vertexShader3DSkinnedSource;  // skinned-mesh variant (adds bone matrices)
};

#endif // PB3D_h

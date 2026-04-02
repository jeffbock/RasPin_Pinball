# PB3D API Reference — 3D Rendering

## Overview

The `PB3D` class provides hardware-accelerated 3D model rendering built on OpenGL ES.  It sits in the class hierarchy between `PBOGLES` (the raw GL wrapper) and `PBGfx` (the 2D graphics layer), so every `PBEngine` subclass has full access to both 2D and 3D rendering in the same frame:

```
PBOGLES   (raw OpenGL ES)
  └─ PB3D          (3D model rendering — this API)
       └─ PBGfx    (2D sprites, text, animation)
            └─ PBEngine  (game logic, state, timers)
```

**Supported format:** glTF binary (`.glb`) loaded via the embedded **cgltf** library.  No external 3D library dependencies are required.

**Key design goals:**
- Pixel-coordinate positioning — place 3D objects using the same screen-pixel coordinates used for 2D sprites
- Depth-Z offset for layering objects in front of / behind the 2D scene
- Perspective-correct depth animation — objects do not drift toward centre when their Z depth animates
- Full high-level transform animation mirroring the 2D animation API (property masks, loop types, animation types)
- Full skeleton animation for rigged glTF models (clip playback, per-bone keyframe evaluation, GPU skinning)

---

## Key Concepts

| Concept | Description |
|---------|-------------|
| **Model** | Geometry loaded from a `.glb` file.  One model can be shared by many instances. |
| **Instance** | A renderable copy of a model with its own position, rotation, scale, and alpha. |
| **Pixel anchor** | An instance can be anchored to a screen-pixel coordinate.  When Z depth changes, the X/Y world position is automatically corrected so the object stays locked to that pixel. |
| **Transform animation** | Struct-driven interpolation between start and end values for any combination of position/rotation/scale/alpha, using the same loop and type enums as the 2D system. |
| **Skeleton animation** | Bone-driven deformation loaded from the glTF skin and animation objects.  Play named clips directly from the model file without any additional authoring step. |
| **Animation clip** | A named set of keyframe channels embedded in the `.glb` file.  Each clip drives one or more bones over time.  Use `pb3dListAnimClips()` to discover what clips a model contains. |
| **Static model** | A `.glb` that has no skin / joint attributes, or one that is intentionally loaded with `forceStatic = true`.  Uses a simpler 8-float vertex layout and renders faster. |
| **Skinned model** | A `.glb` that contains a skin with at least one joint.  Loaded automatically with the 16-float vertex layout (pos + normal + UV + joint indices + weights) and rendered with the skinned shader. |

---

## Coordinate System

### Pixel coordinates (public API)

All public position-setting functions and animation pixel coordinates use **screen pixels**, matching the 2D sprite system:

- Origin `(0, 0)` is the **upper-left** corner of the screen
- `(PB_SCREENWIDTH, PB_SCREENHEIGHT)` is the lower-right corner
- `depthZ` is a **relative depth offset**:  `0.0` = at the screen plane, `< 0.0` = behind the screen, `> 0.0` = in front

`PB_SCREENWIDTH` and `PB_SCREENHEIGHT` are defined in `src/Pinball.h` and control the target resolution for the whole project.  PB3D reads the actual screen dimensions at runtime via `oglGetScreenWidth()` / `oglGetScreenHeight()` (the same source used by the PBGfx 2D layer), so pixel positions are always correct regardless of the compile-time values.

```cpp
// Place a die at the left-top area of the screen, 1.5 units behind the screen plane
pb3dSetInstancePositionPx(instanceId, 170.0f, 290.0f, -1.5f);

// Centre-screen position using compile-time constants
pb3dSetInstancePositionPx(instanceId, PB_SCREENWIDTH / 2.0f, PB_SCREENHEIGHT / 2.0f, 0.0f);
```

### World coordinates (internal)

PB3D internally converts pixel positions to 3D world coordinates using the camera FOV and aspect ratio.  You never need to deal with world units directly; the pixel-coordinate API handles all conversions.

---

## Initialization

`pb3dInit()` is called once automatically from `gfxInit()`.  It compiles and caches both the static 3D GLSL shader and the skinned-mesh GLSL shader.  You do not need to call it manually.

**Status:** returns `false` (and logs an error to the on-screen console) if the static shader fails to compile.  If the skinned shader fails to compile (non-fatal), a warning is logged and skinned models will fall back to static rendering.

---

## Static vs Animated Models

PB3D supports two rendering paths that are selected **per model** at load time based on the content of the `.glb` file and optional load flags:

### Static models

A model is treated as **static** when:
- The `.glb` contains no skin / `JOINTS_0` / `WEIGHTS_0` vertex attributes, **or**
- `pb3dLoadModel()` is called with `forceStatic = true`

**Vertex layout:** 8 floats per vertex — position (3) + normal (3) + UV (2)

**Rendering:** Uses the standard static shader.  Lighting and texture sampling apply exactly as documented.

**Typical use cases:** scenery props, background objects, decorative elements that do not need bone deformation.

### Skinned (animated) models

A model is treated as **skinned** when the `.glb` contains at least one skin with joint attributes and `forceStatic` is not set.

**Vertex layout:** 16 floats per vertex — position (3) + normal (3) + UV (2) + joint indices (4) + blend weights (4)

**Rendering:** Uses the skinned shader, which applies the `uBones[]` array of bone matrices to deform vertices on the GPU before lighting.

**Typical use cases:** characters, creatures, mechanical parts with articulated joints.

### forceStatic flag

```cpp
// Normal load — auto-detect static vs skinned from .glb content
unsigned int modelId = pb3dLoadModel(PB3D_MODEL_PATH "character.glb");

// Force static even if the .glb has skin data (useful for debugging or decorative instances)
unsigned int modelId = pb3dLoadModel(PB3D_MODEL_PATH "character.glb", true);
```

Use `forceStatic = true` when:
- You want the lighter 8-float vertex path for a rigged asset used only as a static prop
- You need to debug geometry without skinning distortion
- The model's skeleton exceeds `PB3D_MAX_BONES` and you have not yet run `pb3dutil --simplify-bones`

> **Tip:** Use `pb3dutil --info` to inspect a `.glb` before loading it to see whether it has skin data and how many bones it contains.

---

## Configuration Defines

The following defines in `PB3D.h` control the skeleton animation limits:

| Define | Default | Description |
|--------|---------|-------------|
| `PB3D_MAX_BONES` | 1024 | Maximum number of bones uploaded to the GPU per frame.  Must match the `uBones[]` array size in the skinned vertex shader.  Models with more bones must be simplified with `pb3dutil --simplify-bones` or split into multiple models. |
| `PB3D_MAX_SKINS` | 8 | Maximum number of skins tracked per model (the normal case is 1; only multi-skin exports from some DCCs need more). |
| `PB3D_MODEL_PATH` | `"src/user/resources/3d/"` | Default path prefix for model files.  Use with `pb3dLoadModel(PB3D_MODEL_PATH "file.glb")` for consistent cross-platform paths. |

> **Hardware note:** The Raspberry Pi 5 VideoCore VII reports `GL_MAX_VERTEX_UNIFORM_VECTORS = 4096`.  Each `mat4` bone costs 4 vectors, so the Pi 5 supports up to 4096 / 4 = 1024 bones, matching `PB3D_MAX_BONES`.  `pb3dInit()` queries the actual hardware value at startup and logs a warning if `PB3D_MAX_BONES` exceeds what the GPU can support.

---

## Model Loading

### pb3dLoadModel()

Loads a `.glb` file from disk, creates GPU buffers (VAO/VBO/EBO), and returns a model ID.

```cpp
unsigned int pb3dLoadModel(const char* glbFilePath, bool forceStatic = false);
```

**Parameters:**
- `glbFilePath` — Path to a glTF binary file.  Place models in `src/user/resources/3d/` and use the `PB3D_MODEL_PATH` macro
- `forceStatic` — Optional.  When `true`, JOINTS_0/WEIGHTS_0 vertex attributes are ignored and the model is uploaded using the static 8-float vertex layout.  See [Static vs Animated Models](#static-vs-animated-models) for details.

**Returns:** Model ID on success, `0` on failure (error logged to console)

**Notes:**
- Models are automatically centred and normalised to a `[-1, 1]` bounding box, so `scale = 1.0` gives a roughly 2-unit object
- If the `.glb` contains no normals, flat face normals are computed automatically from the geometry so shading still works correctly
- When a `.glb` contains skin/joint data, all bones and animation clips are loaded automatically and stored in the model.  Call `pb3dListAnimClips()` to inspect them.

**Example:**
```cpp
#define MODEL_DICE      PB3D_MODEL_PATH "dice.glb"
#define MODEL_CHARACTER PB3D_MODEL_PATH "character.glb"

// Static model — no skin data in .glb
m_diceModelId = pb3dLoadModel(MODEL_DICE);
if (m_diceModelId == 0) {
    pbeSendConsole("Failed to load dice model");
    return false;
}

// Skinned model — automatically detects JOINTS_0/WEIGHTS_0 and loads bones/clips
m_charModelId = pb3dLoadModel(MODEL_CHARACTER);
if (m_charModelId == 0) {
    pbeSendConsole("Failed to load character model");
    return false;
}

// List available animation clips in the character model
auto clips = pb3dListAnimClips(m_charModelId);
// clips → {"Idle", "Walk", "Attack"}
```

### pb3dUnloadModel()

Frees GPU buffers and removes the model from memory.

```cpp
bool pb3dUnloadModel(unsigned int modelId);
```

---

## Instance Management

A model can have multiple independent instances, each with its own transform.

### pb3dCreateInstance()

Creates a new instance of a previously loaded model.

```cpp
unsigned int pb3dCreateInstance(unsigned int modelId);
```

**Returns:** Instance ID on success, `0` on failure

**Example:**
```cpp
m_diceInstance[0] = pb3dCreateInstance(m_diceModelId);
m_diceInstance[1] = pb3dCreateInstance(m_diceModelId);
```

### pb3dDestroyInstance()

Removes an instance and its animation data.

```cpp
bool pb3dDestroyInstance(unsigned int instanceId);
```

---

## Instance Properties

### pb3dSetInstancePositionPx()

Positions an instance using screen pixels.  Two overloads:

```cpp
// Place at (pixelX, pixelY) exactly on the screen plane (depthZ = 0)
void pb3dSetInstancePositionPx(unsigned int instanceId, float pixelX, float pixelY);

// Place at (pixelX, pixelY) with a Z-depth offset
void pb3dSetInstancePositionPx(unsigned int instanceId, float pixelX, float pixelY, float depthZ);
```

**depthZ values:**
- `0.0` — at the screen surface
- `-1.0` to `-5.0` — behind the screen (good range for background/mid-field objects)
- `+1.0` and above — in front of the screen

**Pixel anchor:** calling this function stores a **pixel anchor** on the instance.  When the instance's Z depth changes (e.g. via animation), the world-space X/Y is automatically corrected each frame so the object stays locked to the same screen-pixel position.

```cpp
// Four dice at screen corners
pb3dSetInstancePositionPx(m_diceInstance[0],  170.0f,  290.0f, 0.0f);
pb3dSetInstancePositionPx(m_diceInstance[1], 1750.0f,  290.0f, 0.0f);
pb3dSetInstancePositionPx(m_diceInstance[2],  170.0f,  760.0f, 0.0f);
pb3dSetInstancePositionPx(m_diceInstance[3], 1750.0f,  760.0f, 0.0f);
```

### pb3dSetInstanceRotation()

Sets rotation in degrees around each world axis.

```cpp
void pb3dSetInstanceRotation(unsigned int instanceId, float rx, float ry, float rz);
```

```cpp
pb3dSetInstanceRotation(instanceId, 0.0f, 45.0f, 0.0f);  // 45° around Y
```

### pb3dSetInstanceScale()

Sets uniform scale factor.  `1.0` = model's natural normalised size.

```cpp
void pb3dSetInstanceScale(unsigned int instanceId, float scale);
```

```cpp
pb3dSetInstanceScale(instanceId, 0.18f);  // Roughly die-sized on a 1080p screen
```

### pb3dSetInstanceAlpha()

Sets transparency.  Range `0.0` (invisible) to `1.0` (opaque).

```cpp
void pb3dSetInstanceAlpha(unsigned int instanceId, float alpha);
```

### pb3dSetInstanceVisible()

Shows or hides an instance without destroying it.

```cpp
void pb3dSetInstanceVisible(unsigned int instanceId, bool visible);
```

---

## Lighting

The 3D shader uses **Blinn-Phong** shading with a single directional light plus ambient fill.

### pb3dSetLightDirection()

Sets the light direction vector (normalised world-space).  The light shines *in the direction specified*, so `(0, 1, 1)` means the light comes from the upper-front area and illuminates the top and front faces of objects.

```cpp
void pb3dSetLightDirection(float x, float y, float z);
```

**Default:** `(0.5, 1.0, 1.0)` — upper-right, slightly in front

```cpp
pb3dSetLightDirection(0.5f, 1.0f, 1.0f);
```

### pb3dSetLightColor()

Sets the colour of the directional light.  Values `0.0–1.0` per channel.

```cpp
void pb3dSetLightColor(float r, float g, float b);
```

**Default:** `(1.0, 0.95, 0.85)` — warm white

```cpp
pb3dSetLightColor(1.0f, 0.95f, 0.85f);
```

### pb3dSetLightAmbient()

Sets the ambient fill colour.  Prevents completely dark faces.

```cpp
void pb3dSetLightAmbient(float r, float g, float b);
```

**Default:** `(0.15, 0.15, 0.2)` — cool dark fill

```cpp
pb3dSetLightAmbient(0.15f, 0.15f, 0.2f);
```

**Lighting setup example:**
```cpp
pb3dSetLightDirection(0.5f, 1.0f, 1.0f);
pb3dSetLightColor(1.0f, 0.95f, 0.85f);
pb3dSetLightAmbient(0.15f, 0.15f, 0.2f);
```

---

## Rendering

3D rendering must be wrapped in a `pb3dBegin()` / `pb3dEnd()` pair.  Call this from your screen render function, typically after drawing background 2D layers and before drawing foreground 2D overlays.

### pb3dBegin()

Activates the 3D shader, enables depth testing, and uploads the current view/projection matrices.

```cpp
void pb3dBegin();
```

### pb3dEnd()

Restores full 2D rendering state after the 3D pass by delegating to `PBOGLES::oglRestore2DState()`.  Specifically: disables depth test and face culling, re-enables alpha blending, unbinds the VAO, rebinds the 2D sprite shader program, unbinds VBOs (so 2D CPU vertex pointers are not misread as VBO offsets), re-enables the 2D vertex attrib arrays, and resets the PBOGLES texture cache.

```cpp
void pb3dEnd();
```

### pb3dRenderInstance()

Renders a single instance.

```cpp
void pb3dRenderInstance(unsigned int instanceId);
```

### pb3dRenderAll()

Renders every visible instance (convenience wrapper).

```cpp
void pb3dRenderAll();
```

**Typical render loop:**
```cpp
bool PBEngine::pbeRenderMyScreen(unsigned long currentTick, unsigned long lastTick) {
    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);

    // 2D background
    gfxRenderSprite(m_backgroundId, 0, 0);

    // 3D layer
    pb3dBegin();
    for (int i = 0; i < 4; i++) {
        pb3dAnimateInstance(m_diceInstance[i], currentTick);
        pb3dRenderInstance(m_diceInstance[i]);
    }
    pb3dEnd();

    // 2D overlay (HUD, text on top of 3D objects)
    gfxRenderString(m_fontId, "ROLL", 960, 50, 2, GFX_TEXTCENTER);

    return true;
}
```

---

## Animation

The 3D animation system mirrors the 2D `gfxLoadAnimateData` / `gfxCreateAnimation` pattern.  It uses the same `gfxLoopType` and `gfxAnimType` enums.

### Animation Property Masks

Bitmask controlling which properties are interpolated:

```cpp
#define ANIM3D_POSX_MASK   0x001   // X position
#define ANIM3D_POSY_MASK   0x002   // Y position
#define ANIM3D_POSZ_MASK   0x004   // Z depth
#define ANIM3D_ROTX_MASK   0x008   // Rotation around X axis
#define ANIM3D_ROTY_MASK   0x010   // Rotation around Y axis
#define ANIM3D_ROTZ_MASK   0x020   // Rotation around Z axis
#define ANIM3D_SCALE_MASK  0x040   // Uniform scale
#define ANIM3D_ALPHA_MASK  0x080   // Alpha transparency
#define ANIM3D_ALL_MASK    0x0FF   // All properties
```

### st3DAnimateData Structure

```cpp
struct st3DAnimateData {
    unsigned int animateInstanceId;   // Instance to animate

    // Pixel-space start/end coordinates (X/Y only, used when usePxCoords=true)
    bool  usePxCoords;
    float startPxX, startPxY;
    float endPxX,   endPxY;

    // Start state — world units for Z, rotation (degrees), scale, alpha
    float startPosX, startPosY, startPosZ;
    float startRotX, startRotY, startRotZ;
    float startScale, startAlpha;

    // End/target state
    float endPosX, endPosY, endPosZ;
    float endRotX, endRotY, endRotZ;
    float endScale, endAlpha;

    // Timing
    unsigned int startTick;      // 0 = start immediately when pb3dAnimateRestart is called
    float animateTimeSec;        // Duration (NORMAL/ACCL) or interval (JUMP/JUMPRANDOM)

    // Control
    unsigned int typeMask;       // ANIM3D_*_MASK bitmask
    gfxAnimType  animType;       // GFX_ANIM_NORMAL, ACCL, JUMP, JUMPRANDOM
    gfxLoopType  loop;           // GFX_NOLOOP, GFX_RESTART, GFX_REVERSE
    bool         isActive;       // Start active immediately

    // Acceleration mode (GFX_ANIM_ACCL)
    float accelX, accelY, accelZ;
    float accelRotX, accelRotY, accelRotZ;
    float initialVelX, initialVelY, initialVelZ;
    float initialVelRotX, initialVelRotY, initialVelRotZ;

    // JUMPRANDOM
    float randomPercent;         // 0.0–1.0 probability of jumping per interval

    // Rotation direction
    bool rotateClockwiseX, rotateClockwiseY, rotateClockwiseZ;
};
```

### pb3dCreateAnimation()

Registers an animation with the system.

```cpp
bool pb3dCreateAnimation(st3DAnimateData anim, bool replaceExisting);
```

**Parameters:**
- `anim` — Populated `st3DAnimateData` struct
- `replaceExisting` — `true` overwrites any existing animation on this instance

**Returns:** `true` on success

### pb3dAnimateInstance()

Advances the animation and updates the instance's properties.  **Call once per frame per animated instance**, before `pb3dRenderInstance()`.

```cpp
bool pb3dAnimateInstance(unsigned int instanceId, unsigned int currentTick);
```

**Returns:** `true` if the animation is still active

### Animation Control Functions

```cpp
// Check if animation is currently running
bool pb3dAnimateActive(unsigned int instanceId);

// Stop and remove the animation
void pb3dAnimateClear(unsigned int instanceId);

// Restart animation from beginning, using stored startTick
void pb3dAnimateRestart(unsigned int instanceId);

// Restart with an explicit start tick (e.g. synchronised with currentTick)
void pb3dAnimateRestart(unsigned int instanceId, unsigned long startTick);
```

---

## Animation Examples

### Continuous Y-axis spin

```cpp
st3DAnimateData anim = {};
anim.animateInstanceId = m_diceInstance[0];
anim.usePxCoords       = false;
anim.typeMask          = ANIM3D_ROTY_MASK;
anim.animType          = GFX_ANIM_NORMAL;
anim.loop              = GFX_RESTART;
anim.isActive          = true;
anim.animateTimeSec    = 2.0f;
anim.startRotY         = 0.0f;
anim.endRotY           = 360.0f;
anim.rotateClockwiseY  = true;
pb3dCreateAnimation(anim, true);
```

### Depth pulse (Z-axis, perspective-correct)

```cpp
st3DAnimateData anim = {};
anim.animateInstanceId = m_diceInstance[0];
anim.usePxCoords       = false;
anim.typeMask          = ANIM3D_POSZ_MASK;
anim.animType          = GFX_ANIM_NORMAL;
anim.loop              = GFX_REVERSE;
anim.isActive          = true;
anim.animateTimeSec    = 1.5f;
anim.startPosZ         = 0.0f;
anim.endPosZ           = -4.5f;
pb3dCreateAnimation(anim, true);
// Note: pixel anchor (set automatically by pb3dSetInstancePositionPx) keeps the
// screen-pixel position locked as Z changes — no perspective drift toward centre.
```

### Random jitter (JUMPRANDOM) with pixel coordinates

```cpp
st3DAnimateData anim = {};
anim.animateInstanceId = m_diceInstance[0];
anim.usePxCoords       = true;          // Start/end in screen pixels
anim.startPxX = anim.endPxX = 170.0f;  // Jitter X around the anchor
anim.startPxY = 270.0f;
anim.endPxY   = 310.0f;
anim.typeMask          = ANIM3D_POSX_MASK | ANIM3D_POSY_MASK;
anim.animType          = GFX_ANIM_JUMPRANDOM;
anim.loop              = GFX_RESTART;
anim.isActive          = true;
anim.animateTimeSec    = 0.08f;         // Check every 80 ms
anim.randomPercent     = 0.7f;          // 70% chance of jumping per interval
pb3dCreateAnimation(anim, true);
```

### Combined rotation + depth + scale (all in one animation)

```cpp
st3DAnimateData anim = {};
anim.animateInstanceId = m_diceInstance[0];
anim.usePxCoords       = false;
anim.typeMask          = ANIM3D_ROTY_MASK | ANIM3D_POSZ_MASK | ANIM3D_SCALE_MASK;
anim.animType          = GFX_ANIM_NORMAL;
anim.loop              = GFX_REVERSE;
anim.isActive          = true;
anim.animateTimeSec    = 2.0f;
anim.startRotY         = 0.0f;    anim.endRotY   = 180.0f;
anim.startPosZ         = 0.0f;    anim.endPosZ   = -3.0f;
anim.startScale        = 0.16f;   anim.endScale  = 0.22f;
pb3dCreateAnimation(anim, true);
```

---

## Complete Setup Example

```cpp
// ---- Load function (called once) ----------------------------------------
bool PBEngine::pbeLoadDiceScreen(bool forceReload) {
    static bool loaded = false;
    if (forceReload) loaded = false;
    if (loaded) return true;

    // Set lighting
    pb3dSetLightDirection(0.5f, 1.0f, 1.0f);
    pb3dSetLightColor(1.0f, 0.95f, 0.85f);
    pb3dSetLightAmbient(0.15f, 0.15f, 0.2f);

    // Load model
    m_diceModelId = pb3dLoadModel(PB3D_MODEL_PATH "dice.glb");
    if (m_diceModelId == 0) return false;

    // Create four instances
    float posX[4] = { 170.0f, 1750.0f,  170.0f, 1750.0f };
    float posY[4] = { 290.0f,  290.0f,  760.0f,  760.0f };

    for (int i = 0; i < 4; i++) {
        m_diceInstance[i] = pb3dCreateInstance(m_diceModelId);
        pb3dSetInstanceScale(m_diceInstance[i], 0.18f);
        pb3dSetInstanceAlpha(m_diceInstance[i], 1.0f);
        pb3dSetInstancePositionPx(m_diceInstance[i], posX[i], posY[i], 0.0f);

        // Spin + depth animation
        st3DAnimateData anim = {};
        anim.animateInstanceId = m_diceInstance[i];
        anim.usePxCoords       = false;
        anim.typeMask          = ANIM3D_ROTY_MASK | ANIM3D_POSZ_MASK;
        anim.animType          = GFX_ANIM_NORMAL;
        anim.loop              = GFX_REVERSE;
        anim.isActive          = true;
        anim.animateTimeSec    = 2.0f + i * 0.3f;   // Slightly different per die
        anim.startRotY = 0.0f;    anim.endRotY   = 360.0f;
        anim.startPosZ = 0.0f;    anim.endPosZ   = -4.5f;
        pb3dCreateAnimation(anim, true);
    }

    loaded = true;
    return true;
}

// ---- Render function (called every frame) --------------------------------
bool PBEngine::pbeRenderDiceScreen(unsigned long currentTick, unsigned long lastTick) {
    if (!pbeLoadDiceScreen(false)) return false;

    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
    gfxRenderSprite(m_backgroundId, 0, 0);

    pb3dBegin();
    for (int i = 0; i < 4; i++) {
        pb3dAnimateInstance(m_diceInstance[i], currentTick);
        pb3dRenderInstance(m_diceInstance[i]);
    }
    pb3dEnd();

    // Overlay labels
    gfxRenderString(m_fontId, "ROLL THE DICE", PB_SCREENWIDTH / 2, 50, 2, GFX_TEXTCENTER);

    return true;
}
```

---

## Skeleton Animation

For models loaded from `.glb` files that contain a skin (bone hierarchy) and one or more animation clips, PB3D supports full GPU-skinned skeletal animation.  The system automatically detects skin data at load time — no special flags are required.

### How it works

1. `pb3dLoadModel()` reads the glTF skin hierarchy, inverse bind matrices, and all animation clip keyframes.
2. `pb3dPlayAnimClip()` activates a named clip on a specific instance.
3. `pb3dAnimateInstance()` — the **same call used for transform animations** — also advances the skeleton playback timer and recomputes the per-bone skinning matrices via forward kinematics.
4. `pb3dRenderInstance()` / `pb3dRenderAll()` detects when a clip is playing and automatically switches to the skinned shader, uploading the bone matrices to the GPU `uBones[]` uniform array.

Both **transform animations** (position / rotation / scale / alpha via `pb3dCreateAnimation`) and **skeleton animations** (bone deformation via `pb3dPlayAnimClip`) can run simultaneously on the same instance.

### Skeleton Animation Data Structures

```cpp
// Keyframe interpolation modes (mirrors glTF)
enum e3DInterpolationType {
    ANIM_INTERP_LINEAR      = 0,  // Linear interpolation between keyframes
    ANIM_INTERP_STEP        = 1,  // Snap (no interpolation)
    ANIM_INTERP_CUBICSPLINE = 2   // Cubic spline (smooth tangents)
};

// The property each animation channel drives on a bone
enum e3DAnimChannelType {
    ANIM_CHANNEL_TRANSLATION = 0,
    ANIM_CHANNEL_ROTATION    = 1,
    ANIM_CHANNEL_SCALE       = 2
};

// A single keyframe (time + value)
struct st3DAnimKeyframe {
    float time;      // seconds from the start of the clip
    float value[4];  // translation: xyz | rotation: xyzw quaternion | scale: xyz
};

// One channel drives a single property on a single bone
struct st3DAnimChannel {
    int                      boneIndex;
    e3DAnimChannelType       type;
    e3DInterpolationType     interpolation;
    std::vector<st3DAnimKeyframe> keyframes;
};

// One named animation clip (one glTF animation object)
struct st3DAnimClip {
    std::string name;
    float       duration;  // seconds
    std::vector<st3DAnimChannel> channels;
};

// Per-instance skeleton playback state (embedded in st3DInstance)
struct st3DSkelState {
    int   clipIndex;       // index into model's skeleton.clips; -1 = no clip playing
    float currentTime;     // playback position in seconds
    bool  looping;
    bool  isPlaying;
    // boneMatrices: final skinning matrices uploaded to the GPU (dynamically sized)
    std::vector<float> boneMatrices;
};
```

### Skeleton Animation API

#### pb3dListAnimClips()

Returns the names of all animation clips embedded in a model.

```cpp
std::vector<std::string> pb3dListAnimClips(unsigned int modelId);
```

```cpp
auto clips = pb3dListAnimClips(m_charModelId);
// e.g.  clips = {"Idle", "Walk", "Run", "Attack"}
```

#### pb3dFindAnimClip()

Finds the index of a named clip; returns `-1` if not found.

```cpp
int pb3dFindAnimClip(unsigned int modelId, const std::string& clipName);
```

#### pb3dPlayAnimClip()

Starts playing an animation clip on an instance.  Two overloads:

```cpp
// Play by name (most convenient)
bool pb3dPlayAnimClip(unsigned int instanceId, const std::string& clipName, bool loop = true);

// Play by index (from pb3dFindAnimClip or pb3dListAnimClips position)
bool pb3dPlayAnimClip(unsigned int instanceId, int clipIndex, bool loop = true);
```

**Parameters:**
- `instanceId` — Instance to animate
- `clipName` / `clipIndex` — Which clip to play
- `loop` — When `true`, the clip loops continuously.  When `false`, it plays once and stops at the last frame.

**Returns:** `true` on success, `false` if the instance or clip was not found, or the model has no skeleton.

**Notes:**
- Calling `pb3dPlayAnimClip()` while a clip is already playing replaces it immediately
- The bone matrices are computed for time 0 during this call so the first rendered frame is correct

#### pb3dStopAnimClip()

Stops skeleton animation on an instance.  The model freezes in the last computed pose.

```cpp
bool pb3dStopAnimClip(unsigned int instanceId);
```

#### pb3dIsAnimClipPlaying()

Returns `true` if a skeleton clip is currently playing on the instance.

```cpp
bool pb3dIsAnimClipPlaying(unsigned int instanceId) const;
```

#### pb3dGetAnimClipTime() / pb3dSetAnimClipTime()

Read or seek the current playback position in seconds.

```cpp
float pb3dGetAnimClipTime(unsigned int instanceId) const;
bool  pb3dSetAnimClipTime(unsigned int instanceId, float timeSec);
```

### Skeleton Animation Example

```cpp
// ---- Load ----------------------------------------------------------------
bool PBEngine::pbeLoadCharacterScreen(bool forceReload) {
    static bool loaded = false;
    if (forceReload) loaded = false;
    if (loaded) return true;

    pb3dSetLightDirection(0.5f, 1.0f, 1.0f);
    pb3dSetLightColor(1.0f, 0.95f, 0.85f);
    pb3dSetLightAmbient(0.15f, 0.15f, 0.2f);

    // Load a skinned model — skeleton and clips are loaded automatically
    m_charModelId = pb3dLoadModel(PB3D_MODEL_PATH "character.glb");
    if (m_charModelId == 0) return false;

    // Log all available animation clips
    auto clips = pb3dListAnimClips(m_charModelId);
    for (auto& name : clips) pbeSendConsole("Clip: " + name);

    // Create instance, position it at centre screen
    m_charInstance = pb3dCreateInstance(m_charModelId);
    pb3dSetInstanceScale(m_charInstance, 0.5f);
    pb3dSetInstancePositionPx(m_charInstance,
        PB_SCREENWIDTH / 2.0f, PB_SCREENHEIGHT / 2.0f, 0.0f);

    // Start the "Idle" skeleton clip, looping
    pb3dPlayAnimClip(m_charInstance, "Idle", true);

    loaded = true;
    return true;
}

// ---- Render (called every frame) -----------------------------------------
bool PBEngine::pbeRenderCharacterScreen(unsigned long currentTick, unsigned long lastTick) {
    if (!pbeLoadCharacterScreen(false)) return false;

    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);

    pb3dBegin();
    // pb3dAnimateInstance advances BOTH the transform animation (if any)
    // AND the skeleton clip timer — one call does both
    pb3dAnimateInstance(m_charInstance, currentTick);
    pb3dRenderInstance(m_charInstance);
    pb3dEnd();

    return true;
}

// ---- Input event — switch clip on button press ---------------------------
void PBEngine::pbeOnButtonAttack() {
    pb3dPlayAnimClip(m_charInstance, "Attack", false);  // play once, no loop
}
```

### Combining Transform and Skeleton Animation

Both animation systems work independently on the same instance.  For example, you can spin or translate a character with `pb3dCreateAnimation()` while the skeleton plays a walk cycle:

```cpp
// Skeleton clip: walk cycle, looping
pb3dPlayAnimClip(m_charInstance, "Walk", true);

// Transform animation: slowly move across screen at the same time
st3DAnimateData anim = {};
anim.animateInstanceId = m_charInstance;
anim.usePxCoords       = true;
anim.startPxX          = 200.0f;   anim.endPxX = 1720.0f;
anim.startPxY          = 540.0f;   anim.endPxY = 540.0f;
anim.typeMask          = ANIM3D_POSX_MASK;
anim.animType          = GFX_ANIM_NORMAL;
anim.loop              = GFX_NOLOOP;
anim.isActive          = true;
anim.animateTimeSec    = 4.0f;
pb3dCreateAnimation(anim, true);
```

---

## Notes and Limitations

- **One light source** — the shader supports a single directional light.  Ambient fills in shadows.
- **Texture support** — the PBR metallic roughness **base color texture** embedded in the `.glb` file is loaded automatically and sampled in the fragment shader alongside the Blinn-Phong lighting.  GLB (binary glTF) packs all textures into the single file, so no separate image files are needed.  If a primitive has no texture, a 1×1 white fallback is used so shading still works.  Other PBR texture maps (normal map, roughness/metallic, emissive) are not currently sampled.
- **No shadows** — shadow mapping is not implemented.  Use ambient + directional tuning to fake depth cues.
- **Single transform animation per instance** — each instance supports one active transform animation at a time.  `replaceExisting = true` in `pb3dCreateAnimation()` replaces a running animation.
- **Single skeleton clip per instance** — calling `pb3dPlayAnimClip()` replaces any currently playing clip.
- **Bone limit** — skinned models are capped at `PB3D_MAX_BONES` (1024) bones per frame.  Models with more bones must be simplified using `pb3dutil --simplify-bones` or split into multiple models.  Use `pb3dutil --info` to check bone counts before loading.
- **Model normalisation** — all models are centred and normalised on load.  The `scale` property (via `pb3dSetInstanceScale`) controls final display size in world units.
- **Model path convention** — use `PB3D_MODEL_PATH` (`"src/user/resources/3d/"`) for all model paths to keep them consistent and cross-platform.
- **Interpolation support** — LINEAR, STEP, and CUBICSPLINE glTF animation keyframe types are all supported.  Cubicspline uses the value knot only (tangents are not used).

---

## Related Documentation

- **[Game Creation API](Game_Creation_API.md)** — 2D sprites, animation, sound, and video
- **[RasPin Overview](RasPin_Overview.md)** — architecture overview and system introduction
- **[Utilities Guide](Utilities_Guide.md)** — pb3dutil model analysis tool (bone counts, clip listing, simplification analysis)

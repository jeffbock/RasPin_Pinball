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
- Full animation system mirroring the 2D animation API (property masks, loop types, animation types)

---

## Key Concepts

| Concept | Description |
|---------|-------------|
| **Model** | Geometry loaded from a `.glb` file.  One model can be shared by many instances. |
| **Instance** | A renderable copy of a model with its own position, rotation, scale, and alpha. |
| **Pixel anchor** | An instance can be anchored to a screen-pixel coordinate.  When Z depth changes, the X/Y world position is automatically corrected so the object stays locked to that pixel. |
| **Animation** | Struct-driven interpolation between start and end values for any combination of position/rotation/scale/alpha, using the same loop and type enums as the 2D system. |

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

`pb3dInit()` is called once automatically from `gfxInit()`.  It compiles and caches the 3D GLSL shader.  You do not need to call it manually.

**Status:** returns `false` (and logs an error to the on-screen console) if the shader fails to compile.

---

## Model Loading

### pb3dLoadModel()

Loads a `.glb` file from disk, creates GPU buffers (VAO/VBO/EBO), and returns a model ID.

```cpp
unsigned int pb3dLoadModel(const char* glbFilePath);
```

**Parameters:**
- `glbFilePath` — Path to a glTF binary file.  Place models in `src/resources/3d/` and use the `PB3D_MODEL_PATH` macro

**Returns:** Model ID on success, `0` on failure (error logged to console)

**Notes:**
- Models are automatically centred and normalised to a `[-1, 1]` bounding box, so `scale = 1.0` gives a roughly 2-unit object
- If the `.glb` contains no normals, flat face normals are computed automatically from the geometry so shading still works correctly

**Example:**
```cpp
#define MODEL_DICE  PB3D_MODEL_PATH "dice.glb"

m_diceModelId = pb3dLoadModel(MODEL_DICE);
if (m_diceModelId == 0) {
    pbeSendConsole("Failed to load dice model");
    return false;
}
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

## Notes and Limitations

- **One light source** — the shader supports a single directional light.  Ambient fills in shadows.
- **Texture support** — the PBR metallic roughness **base color texture** embedded in the `.glb` file is loaded automatically and sampled in the fragment shader alongside the Blinn-Phong lighting.  GLB (binary glTF) packs all textures into the single file, so no separate image files are needed.  If a primitive has no texture, a 1×1 white fallback is used so shading still works.  Other PBR texture maps (normal map, roughness/metallic, emissive) are not currently sampled.
- **No shadows** — shadow mapping is not implemented.  Use ambient + directional tuning to fake depth cues.
- **Single animation per instance** — each instance supports one active animation at a time.  `replaceExisting = true` in `pb3dCreateAnimation()` replaces a running animation.
- **Model normalisation** — all models are centred and normalised on load.  The `scale` property (via `pb3dSetInstanceScale`) controls final display size in world units.
- **Model path convention** — use `PB3D_MODEL_PATH` (`"src/resources/3d/"`) for all model paths to keep them consistent and cross-platform.

---

## Related Documentation

- **[Game Creation API](Game_Creation_API.md)** — 2D sprites, animation, sound, and video
- **[RasPin Overview](RasPin_Overview.md)** — architecture overview and system introduction

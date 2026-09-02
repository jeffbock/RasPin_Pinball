# PBGfx 2D Graphics API Reference

## Overview

`PBGfx` is the primary 2D rendering layer of the RasPin engine.  It provides sprite loading,
instancing, rendering, text output, scissor clipping, and a full property-interpolation
animation system — everything you need to build game screens without writing any OpenGL code.

> **3D is optional.**  `PBGfx` is a fully self-contained 2D system.  If your table never needs
> a 3D model, ignore `PB3D` entirely.  See [PB3D_API.md](PB3D_API.md) only if you want to add
> hardware-accelerated 3D objects on top of your 2D scene.

### Class hierarchy

```
PBOGLES     (raw OpenGL ES wrapper — internal, never call directly)
  └─ PB3D           (optional 3D add-on layer)
       └─ PBGfx     (2D sprites, text, animation — this document)
            └─ PBEngine   (game logic, state, I/O, timers)
```

Because `PBEngine` inherits from `PBGfx`, every table class has direct access to all functions
described in this document with no extra setup.

---

## Table of Contents

1. [Key Concepts](#1-key-concepts)
2. [Coordinate System](#2-coordinate-system)
3. [Enumerations and Constants](#3-enumerations-and-constants)
4. [Initialization](#4-initialization)
5. [Loading Sprites](#5-loading-sprites)
   - 5.1 [gfxLoadSprite()](#51-gfxloadsprite)
   - 5.2 [gfxLoadTileSprite()](#52-gfxloadtilesprite)
   - 5.3 [Texture lifecycle — unload / reload](#53-texture-lifecycle--unload--reload)
6. [Instancing Sprites](#6-instancing-sprites)
   - 6.1 [gfxInstanceSprite()](#61-gfxinstancesprite)
   - 6.2 [Query functions](#62-query-functions)
7. [Setting Instance Properties](#7-setting-instance-properties)
8. [Query Functions](#8-query-functions)
9. [Rendering](#9-rendering)
   - 9.1 [gfxRenderSprite()](#91-gfxrendersprite)
   - 9.2 [gfxClear() and gfxSwap()](#92-gfxclear-and-gfxswap)
   - 9.3 [gfxSetScissor()](#93-gfxsetscissor)
10. [Text Rendering](#10-text-rendering)
    - 10.1 [gfxRenderString()](#101-gfxrenderstring)
    - 10.2 [gfxRenderShadowString()](#102-gfxrendershadowstring)
    - 10.3 [gfxStringWidth()](#103-gfxstringwidth)
11. [Tile-Mapped Sprites](#11-tile-mapped-sprites)
12. [Animation System](#12-animation-system)
    - 12.1 [How animations work](#121-how-animations-work)
    - 12.2 [Animation type masks](#122-animation-type-masks)
    - 12.3 [Animation types (gfxAnimType)](#123-animation-types-gfxanimtype)
    - 12.4 [Loop types (gfxLoopType)](#124-loop-types-gfxlooptype)
    - 12.5 [stAnimateData structure](#125-stanimatedata-structure)
    - 12.6 [gfxLoadAnimateData()](#126-gfxloadanimatedata)
    - 12.7 [gfxLoadAnimateDataShort()](#127-gfxloadanimatedatashort)
    - 12.8 [gfxCreateAnimation()](#128-gfxcreateanimation)
    - 12.9 [gfxAnimateSprite()](#129-gfxanimatesprite)
    - 12.10 [Animation control functions](#1210-animation-control-functions)
13. [Video Texture Sprites](#13-video-texture-sprites)
14. [Complete Example — Animated Torch](#14-complete-example--animated-torch)

---

## 1. Key Concepts

| Term | Description |
|------|-------------|
| **Sprite** | A texture (PNG or BMP) loaded into GPU memory, plus metadata (size, map type, tile info). One sprite definition is shared across all instances. |
| **Instance** | A renderable copy of a sprite with its own position, scale, rotation, color, and alpha. The ID returned by `gfxLoadSprite()` *is* the ID of the first instance; additional instances are created with `gfxInstanceSprite()`. |
| **Base sprite ID** | The ID returned by `gfxLoadSprite()`. Also used directly as an instance ID for the first (default) instance. |
| **Parent sprite ID** | Every instance stores the ID of its parent `stSpriteInfo` entry. For the first instance, parent == own ID. |
| **Font sprite** | A sprite loaded with `GFX_TEXTMAP`. The engine reads a companion `.json` UV map to locate each character glyph in the texture atlas. |
| **Tile sprite** | A sprite loaded with `gfxLoadTileSprite()`. The texture sheet is divided into equal tiles; the active tile is selected at runtime with `gfxSetSelectedTile()`. |
| **`NOSPRITE` (0)** | Sentinel value returned on error. Always check return values against `NOSPRITE` before use. |

---

## 2. Coordinate System

All position parameters are **screen pixels**:

- Origin `(0, 0)` is the **upper-left** corner of the screen.
- `(PB_SCREENWIDTH, PB_SCREENHEIGHT)` is the lower-right corner.
- Both constants are defined in `src/Pinball.h`.

The anchor point of a sprite is controlled by `gfxTexCenter`:

| Enum | Anchor |
|------|--------|
| `GFX_UPPERLEFT` | X, Y is the top-left corner of the sprite |
| `GFX_CENTER` | X, Y is the center of the sprite |

Font sprites (`GFX_TEXTMAP`) and tile sprites (`GFX_SPRITEMAP`) are always anchored
`GFX_UPPERLEFT` regardless of the value passed; the engine overrides this internally.

---

## 3. Enumerations and Constants

### gfxTexType

```cpp
enum gfxTexType {
    GFX_BMP   = 0,   // BMP image (no alpha channel)
    GFX_PNG   = 1,   // PNG image (supports alpha)
    GFX_NONE  = 2,   // No texture (color-only quad)
    GFX_VIDEO = 3    // Video texture updated per-frame by PBVideoPlayer
};
```

### gfxTexCenter

```cpp
enum gfxTexCenter {
    GFX_UPPERLEFT = 0,  // Anchor at top-left
    GFX_CENTER    = 1   // Anchor at center
};
```

### gfxSpriteMap

```cpp
enum gfxSpriteMap {
    GFX_NOMAP    = 0,  // Normal sprite — full texture is displayed
    GFX_TEXTMAP  = 1,  // Font sprite — UV map from companion .json
    GFX_SPRITEMAP = 2  // Tile sprite — loaded with gfxLoadTileSprite()
};
```

### gfxTextJustify

```cpp
enum gfxTextJustify {
    GFX_TEXTLEFT   = 0,  // X is the left edge of the string
    GFX_TEXTCENTER = 1,  // X is the horizontal center of the string
    GFX_TEXTRIGHT  = 2   // X is the right edge of the string
};
```

### Sentinel constant

```cpp
#define NOSPRITE 0   // Returned by all load/instance functions on failure
```

---

## 4. Initialization

`gfxInit()` is called once automatically by the engine at startup.  It loads the built-in
system font (`Ubuntu-Regular_24_256.png`) and initializes the 3D sub-layer.  You do not call
it directly.

The system font sprite ID is available at any time via `gfxGetSystemFontSpriteId()`.

---

## 5. Loading Sprites

Loading a sprite uploads a texture to the GPU and registers it in the engine's sprite table.
If a sprite with the same name has already been loaded, the existing ID is returned immediately
(idempotent — safe to call from a load-guard block every frame).

### 5.1 gfxLoadSprite()

Loads a normal or font sprite.

```cpp
unsigned int gfxLoadSprite(const std::string& spriteName,
                           const std::string& textureFileName,
                           gfxTexType  textureType,
                           gfxSpriteMap mapType,
                           gfxTexCenter textureCenter,
                           bool keepResident,
                           bool useTexture);
```

**Parameters:**

| Parameter | Description |
|-----------|-------------|
| `spriteName` | Unique human-readable name. Used as the idempotency key. |
| `textureFileName` | Path to the image file relative to the project root. |
| `textureType` | `GFX_PNG` or `GFX_BMP` (use `GFX_NONE` for a color-only quad). |
| `mapType` | `GFX_NOMAP` for a normal image; `GFX_TEXTMAP` for a font atlas. |
| `textureCenter` | `GFX_UPPERLEFT` or `GFX_CENTER`. Overridden to `GFX_UPPERLEFT` for font/tile sprites. |
| `keepResident` | `true` keeps the texture in GPU memory across `gfxUnloadAllTextures()` calls. Use `true` for anything loaded in a keep-resident load block. |
| `useTexture` | Almost always `true`. Pass `false` only for an untextured colored quad. |

**Returns:** Sprite instance ID, or `NOSPRITE` on failure.

**Examples:**

```cpp
// Background image — upper-left anchor, always resident
m_bgId = gfxLoadSprite("Background",
                       "src/user/resources/textures/backglass.png",
                       GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);

// Centered flame sprite — released when not needed
m_flameId = gfxLoadSprite("Flame",
                          "src/user/resources/textures/flame.png",
                          GFX_PNG, GFX_NOMAP, GFX_CENTER, false, true);

// Font atlas — companion .json UV map must exist at the same path
m_fontId = gfxLoadSprite("MenuFont",
                         "src/user/resources/fonts/Baldur_96_768.png",
                         GFX_PNG, GFX_TEXTMAP, GFX_UPPERLEFT, true, true);
```

> **Font sprites require a companion JSON file** at the same path with a `.json` extension
> (e.g. `Baldur_96_768.json`).  The JSON maps each ASCII character to its UV rectangle and
> pixel dimensions in the atlas.  The engine loads this automatically.

---

### 5.2 gfxLoadTileSprite()

Loads a sprite sheet divided into equal-sized tiles.  Use this for frame-by-frame character
animation or any image where you want to display one tile at a time.

```cpp
unsigned int gfxLoadTileSprite(const std::string& spriteName,
                               const std::string& textureFileName,
                               gfxTexType  textureType,
                               gfxTexCenter textureCenter,
                               bool keepResident,
                               unsigned int tileWidth,
                               unsigned int tileHeight);
```

The engine computes `tileCount = (sheetWidth / tileWidth) × (sheetHeight / tileHeight)`
automatically when the texture loads.  Tiles are numbered **left-to-right, top-to-bottom**
starting at index 0.  The instance's rendered size is set to `tileWidth × tileHeight`.

**Example:**

```cpp
// 256×256 sheet, 32×32 tiles → 64 total tiles
m_warriorId = gfxLoadTileSprite("Warrior",
                                "src/user/resources/textures/warrior.png",
                                GFX_PNG, GFX_CENTER, true, 32, 32);
if (m_warriorId != NOSPRITE)
    gfxSetSelectedTile(m_warriorId, 0);
```

See [§11 Tile-Mapped Sprites](#11-tile-mapped-sprites) for the tile selection API.

---

### 5.3 Texture lifecycle — unload / reload

| Function | Description |
|----------|-------------|
| `gfxUnloadTexture(spriteId)` | Frees the GPU texture for one sprite (skipped if `keepResident`). |
| `gfxUnloadAllTextures()` | Frees GPU textures for every non-resident sprite. |
| `gfxReloadTexture(spriteId)` | Re-uploads a freed texture. Called automatically by `gfxRenderSprite()` if needed. |
| `gfxTextureLoaded(spriteId)` | Returns `true` if the GPU texture is currently loaded. |
| `gfxIsLoaded(spriteId)` | Alias for `gfxTextureLoaded`. |

---

## 6. Instancing Sprites

A **base sprite** has a single default instance (the ID returned by `gfxLoadSprite()`).
Create additional independent instances to render the same texture at multiple positions,
scales, or colors simultaneously.

All three are separate instances of the same texture — only one texture upload is needed.

### 6.1 gfxInstanceSprite()

Three overloads:

```cpp
// Clone with all properties copied from the base sprite's default instance
unsigned int gfxInstanceSprite(unsigned int parentSpriteId);

// Clone the base, then override properties from a pre-filled stSpriteInstance struct
unsigned int gfxInstanceSprite(unsigned int parentSpriteId, stSpriteInstance instance);

// Clone with explicit numeric properties (0–255 for color/alpha components)
unsigned int gfxInstanceSprite(unsigned int parentSpriteId,
                               int x, int y,
                               unsigned int textureAlpha,
                               unsigned int vertRed, unsigned int vertGreen,
                               unsigned int vertBlue, unsigned int vertAlpha,
                               float scaleFactor, float rotateDegrees);
```

**Returns:** New instance ID, or `NOSPRITE` if `parentSpriteId` was not found.

> Instances are used as animation **start** and **end** sentinel sprites.  The animated
> sprite is a third instance that the engine interpolates between the other two each frame.

**Example — creating start/end/animated triple:**

```cpp
m_coinStart = gfxInstanceSprite(m_coinId);
gfxSetXY(m_coinStart, 100, 300, false);
gfxSetScaleFactor(m_coinStart, 0.5f, false);

m_coinEnd = gfxInstanceSprite(m_coinId);
gfxSetXY(m_coinEnd, 700, 300, false);
gfxSetScaleFactor(m_coinEnd, 1.5f, false);

m_coinAnim = gfxInstanceSprite(m_coinId);
// m_coinAnim will be moved and scaled automatically by gfxAnimateSprite()
```

### 6.2 Query functions

```cpp
bool gfxIsSprite(unsigned int spriteId);      // Does this ID exist as any instance?
bool gfxIsFontSprite(unsigned int spriteId);  // Is this a GFX_TEXTMAP instance?
```

---

## 7. Setting Instance Properties

All setters take a sprite **instance** ID and return the same ID on success or `NOSPRITE` on
failure.  The `add` / `addXY` / `addFactor` / `addDegrees` boolean parameter, when `true`,
*adds* the value to the current state instead of replacing it.

| Function | Description |
|----------|-------------|
| `gfxSetXY(id, x, y, addXY)` | Set or offset the screen position. |
| `gfxSetColor(id, r, g, b, a)` | Vertex color tint and alpha (0–255 each). White `(255,255,255,255)` shows the texture unmodified. |
| `gfxSetScaleFactor(id, scale, addFactor)` | Uniform scale. `1.0` = native size. Minimum `0.1` when adding. |
| `gfxSetRotateDegrees(id, deg, addDegrees)` | Clockwise rotation in degrees. Wraps at 360 when adding. |
| `gfxSetTextureAlpha(id, alpha)` | Separate transparency for BMP and video textures (0.0–1.0). PNG textures use their own alpha channel; this value applies an additional multiplier. |
| `gfxSetUV(id, u1, v1, u2, v2)` | Override the UV rectangle (normalized 0.0–1.0). Rarely needed — use this for manual texture-atlas cropping when not using the tile system. |
| `gfxSetWH(id, w, h)` | Override the rendered pixel dimensions without changing the texture. |
| `gfxSetUpdateBoundingBox(id, bool)` | Enable bounding-box computation during rendering. Required before calling `gfxGetBoundingBox()`. |
| `gfxSetSelectedTile(id, tileIndex)` | Select the active tile on a tile-mapped sprite (see §11). |

---

## 8. Query Functions

All getters take a sprite **instance** ID.

| Function | Returns |
|----------|---------|
| `gfxGetBaseWidth(id)` | Native texture width in pixels. |
| `gfxGetBaseHeight(id)` | Native texture height in pixels. |
| `gfxGetXY(id, *x, *y)` | Current X/Y position (out params). Returns sprite ID or `NOSPRITE`. |
| `gfxGetScaleFactor(id)` | Current scale factor. |
| `gfxGetRotateDegrees(id)` | Current rotation in degrees. |
| `gfxGetTextureAlpha(id)` | Current texture alpha (0–255). |
| `gfxGetColor(id, *r, *g, *b, *a)` | Current vertex color components (out params, 0–255). Returns sprite ID or `NOSPRITE`. |
| `gfxGetBoundingBox(id)` | `stBoundingBox {x1,y1,x2,y2}` in screen pixels. Returns all-zero if bounding box tracking is not enabled. |
| `gfxGetTextHeight(id)` | Character glyph height in pixels for a font sprite. |
| `gfxGetSystemFontSpriteId()` | ID of the built-in system font (useful for on-screen debug text). |
| `gfxGetSelectedTile(id)` | Active tile index for a tile-mapped sprite. |
| `gfxGetTileCount(id)` | Total tile count for a tile-mapped sprite. |
| `gfxTextureLoaded(id)` | `true` if the GPU texture is currently resident. |
| `gfxIsLoaded(id)` | Alias for `gfxTextureLoaded`. |
| `gfxIsSprite(id)` | `true` if this ID is a registered instance. |
| `gfxIsFontSprite(id)` | `true` if this is a `GFX_TEXTMAP` instance. |

---

## 9. Rendering

### 9.1 gfxRenderSprite()

Three overloads — all draw the sprite to the current frame buffer:

```cpp
// Uses the position stored on the instance (set with gfxSetXY or a previous render call)
bool gfxRenderSprite(unsigned int spriteId);

// Convenience: updates x/y on the instance, then renders
bool gfxRenderSprite(unsigned int spriteId, int x, int y);

// Convenience: updates x/y, scale, and rotation on the instance, then renders
bool gfxRenderSprite(unsigned int spriteId, int x, int y,
                     float scaleFactor, float rotateDegrees);
```

> **Note on bitmaps and video:** For `GFX_BMP` and `GFX_VIDEO` sprites, the `textureAlpha`
> value is used as an explicit alpha multiplier at render time.  For `GFX_PNG` sprites, the
> texture's own alpha channel is used and `textureAlpha` provides an additional multiplier.

If the sprite's texture has been unloaded, `gfxRenderSprite()` will attempt to reload it
before drawing.  If the reload fails and the sprite has no texture, the quad is still drawn
in its vertex color.

**Bounding box tracking:**

Enable bounding-box computation by calling `gfxSetUpdateBoundingBox(id, true)` once after
loading.  Every subsequent `gfxRenderSprite()` call will update `stBoundingBox` with the
actual screen-pixel rectangle of the rendered quad (accounting for scale and rotation).
Retrieve it with `gfxGetBoundingBox(id)`.

### 9.2 gfxClear() and gfxSwap()

```cpp
// Clear the frame buffer to a solid color.  doFlip=false keeps the buffer active for more drawing.
void gfxClear(float red, float green, float blue, float alpha, bool doFlip);

// Present the completed frame.  flush=true forces the GPU command queue to drain (rarely needed).
void gfxSwap();
void gfxSwap(bool flush);
```

The engine calls `gfxClear()` and `gfxSwap()` automatically for every game frame; you only
need these directly if you are building a completely custom render loop.

### 9.3 gfxSetScissor()

Clips all subsequent rendering to a pixel rectangle.  Use to constrain drawing to a sub-region
of the screen (for example, a status panel or scrolling area).

```cpp
void gfxSetScissor(bool enable, stBoundingBox rect);
```

```cpp
stBoundingBox panel = {600, 0, 800, 480};
gfxSetScissor(true, panel);
// ... render sprites that should be clipped to the right-side panel ...
gfxSetScissor(false, {0,0,0,0});  // disable scissor
```

`stBoundingBox` uses `{x1, y1, x2, y2}` in screen pixels (upper-left / lower-right corners).

---

## 10. Text Rendering

Text rendering requires a sprite loaded with `GFX_TEXTMAP`.  A companion `.json` file at the
same path provides the UV rectangle and pixel dimensions for every glyph.  Only printable
ASCII (codes 32–126) is supported; any other character is silently skipped.

The current vertex color (`gfxSetColor`) and scale factor (`gfxSetScaleFactor`) of the font
sprite instance apply to all characters in the string.

### 10.1 gfxRenderString()

```cpp
// Uses the position stored on the sprite instance
bool gfxRenderString(unsigned int spriteId,
                     std::string input,
                     unsigned int spacingPixels,
                     gfxTextJustify justify);

// Explicit position (does not permanently store x/y on the instance)
bool gfxRenderString(unsigned int spriteId,
                     std::string input,
                     int x, int y,
                     int spacingPixels,
                     gfxTextJustify justify);
```

**Parameters:**

| Parameter | Description |
|-----------|-------------|
| `spriteId` | Font sprite ID (must be a `GFX_TEXTMAP` sprite). |
| `input` | The string to render. |
| `spacingPixels` | Extra pixels added between each character (0 = natural spacing). |
| `justify` | `GFX_TEXTLEFT`, `GFX_TEXTCENTER`, or `GFX_TEXTRIGHT`. |
| `x`, `y` | Screen-pixel anchor position (meaning depends on `justify`). |

**Example:**

```cpp
gfxSetColor(m_fontId, 255, 215, 0, 255);           // gold
gfxSetScaleFactor(m_fontId, 1.2f, false);
gfxRenderString(m_fontId, "High Score", 400, 50, 2, GFX_TEXTCENTER);
```

### 10.2 gfxRenderShadowString()

Renders the string twice: first in a shadow color offset diagonally, then in the current
vertex color at the original position.  Produces a drop-shadow effect.

```cpp
bool gfxRenderShadowString(unsigned int spriteId,
                           std::string input,
                           int x, int y,
                           unsigned int spacingPixels,
                           gfxTextJustify justify,
                           unsigned int red, unsigned int green,
                           unsigned int blue, unsigned int alpha,
                           unsigned int shadowOffset);
```

The shadow color parameters (`red`, `green`, `blue`, `alpha`) are applied only to the shadow
pass; the main string uses the sprite's current vertex color.  `shadowOffset` shifts the
shadow right and down by this many pixels.

**Example:**

```cpp
// White text with a dark grey shadow, 2-pixel offset
gfxSetColor(m_fontId, 255, 255, 255, 255);
gfxRenderShadowString(m_fontId, "Player 1", 400, 200, 2,
                      GFX_TEXTCENTER, 40, 40, 40, 200, 2);
```

### 10.3 gfxStringWidth()

Returns the total pixel width of a string, including inter-character spacing, at the current
scale factor of the sprite.  Use this to position text manually without relying on built-in
justification.

```cpp
int gfxStringWidth(unsigned int spriteId,
                   std::string input,
                   unsigned int spacingPixels);
```

---

## 11. Tile-Mapped Sprites

A tile-mapped sprite is a texture sheet divided into a grid of equally-sized tiles.  Only one
tile is displayed at a time — select it by index at runtime.  This is the recommended approach
for character walk cycles, icon sets, or any sprite sheet where individual frames are equal size.

**Loading:** Use `gfxLoadTileSprite()` (see §5.2).

**Tile indexing:** Tiles are numbered left-to-right, top-to-bottom starting at 0.  If an
out-of-range index is set, the engine clamps to tile 0.

### gfxSetSelectedTile()

```cpp
unsigned int gfxSetSelectedTile(unsigned int spriteId, unsigned int tileIndex);
```

Sets the active tile for a sprite instance.  Returns the sprite ID on success or `NOSPRITE`.

### gfxGetSelectedTile()

```cpp
unsigned int gfxGetSelectedTile(unsigned int spriteId);
```

Returns the current tile index (0 if the sprite was not found).

### gfxGetTileCount()

```cpp
unsigned int gfxGetTileCount(unsigned int spriteId);
```

Returns the total tile count computed from the sheet dimensions at load time.

### Manual walk-cycle example

```cpp
// Advance one frame every 150 ms
unsigned int frame = (currentTick / 150) % gfxGetTileCount(m_warriorId);
gfxSetSelectedTile(m_warriorId, frame);
gfxRenderSprite(m_warriorId, x, y);
```

### Animated walk-cycle using the animation system

Tile index can also be driven automatically by the animation system using
`ANIMATE_TILE_MASK` (see §12.2).  Set the start instance's tile to the first frame and the
end instance's tile to the last frame, then create a `GFX_RESTART` animation.  The engine
interpolates the tile index each frame.

---

## 12. Animation System

### 12.1 How animations work

The animation system interpolates one sprite instance (the **animate** sprite) between two
others (the **start** and **end** sentinel sprites) over time.  All three instances must be
from the same parent texture.

```
start instance   ──── interpolate over animateTimeSec ────►  end instance
                          │
                    animate instance (rendered)
```

Each frame, call `gfxAnimateSprite(animateSpriteId, currentTick)`.  The engine locates the
registered animation, computes the elapsed fraction, and updates the animate instance's
properties according to the `typeMask`.  Then call `gfxRenderSprite(animateSpriteId)` to draw
the updated instance.

Multiple animations can run simultaneously on different sprite IDs.  Pass `NOSPRITE` (0) as
the ID to `gfxAnimateSprite()` to update **all** registered animations in one call.

---

### 12.2 Animation type masks

Bitmask that controls which properties the animation updates:

```cpp
#define ANIMATE_NOMASK         0x000  // No properties (inactive)
#define ANIMATE_X_MASK         0x001  // X position
#define ANIMATE_Y_MASK         0x002  // Y position
#define ANIMATE_U_MASK         0x004  // U texture coordinate (u1)
#define ANIMATE_V_MASK         0x008  // V texture coordinate (v1)
#define ANIMATE_TEXALPHA_MASK  0x010  // Texture alpha
#define ANIMATE_COLOR_MASK     0x020  // Vertex color (R, G, B, A together)
#define ANIMATE_SCALE_MASK     0x040  // Scale factor
#define ANIMATE_ROTATE_MASK    0x080  // Rotation in degrees
#define ANIMATE_ALL_MASK       0x0FF  // All of the above except tile
#define ANIMATE_TILE_MASK      0x100  // Tile index (tile-mapped sprites only)
```

Combine masks with `|` to animate multiple properties simultaneously.

**`ANIMATE_TILE_MASK`** steps the `selectedTile` of a tile-mapped sprite from the tile index
on the start instance to the tile index on the end instance over the animation duration.  Use
`GFX_RESTART` looping to create a continuous walk cycle.  This mask is only meaningful on
sprites loaded with `gfxLoadTileSprite()`.

---

### 12.3 Animation types (gfxAnimType)

```cpp
enum gfxAnimType {
    GFX_ANIM_NORMAL     = 0,  // Linear interpolation
    GFX_ANIM_ACCL       = 1,  // Physics: position/rotation driven by velocity + acceleration
    GFX_ANIM_JUMP       = 2,  // Discrete: snap to end state when timer expires
    GFX_ANIM_JUMPRANDOM = 3   // Discrete: randomly snap to a value in the start–end range
};
```

| Type | Best used for | Notes |
|------|---------------|-------|
| `GFX_ANIM_NORMAL` | Slide-in/out, fade, color shift, scale pulse | Smooth linear interpolation. `animateTimeSec` controls duration. |
| `GFX_ANIM_ACCL` | Projectiles, falling objects, physics-feel motion | Uses initial velocity + acceleration. Duration field is ignored; animation ends when the target is reached. Free-spin rotation supported (set start == end with a non-zero velocity/accel). `GFX_REVERSE` loop not supported. |
| `GFX_ANIM_JUMP` | Blinking, discrete state flips | Stays at start, snaps to end when `animateTimeSec` elapses. With `GFX_RESTART`, swaps start/end and repeats — blink effect. |
| `GFX_ANIM_JUMPRANDOM` | Flame flicker, shimmer | Randomly decides whether to jump (probability = `randomPercent`). Target is a random value between start and end. |

---

### 12.4 Loop types (gfxLoopType)

```cpp
enum gfxLoopType {
    GFX_NOLOOP  = 0,  // Play once; animate sprite stays at end state when complete
    GFX_RESTART = 1,  // Loop: reset to start and repeat
    GFX_REVERSE = 2   // Ping-pong: swap start/end and repeat in reverse direction
};
```

> **`GFX_ANIM_ACCL` + `GFX_REVERSE`:** Not supported — acceleration animations cannot be
> meaningfully reversed since the final position depends on velocity and trajectory.

---

### 12.5 stAnimateData structure

```cpp
struct stAnimateData {
    unsigned int animateSpriteId;   // The sprite instance to animate each frame
    unsigned int startSpriteId;     // Sentinel: defines starting property values
    unsigned int endSpriteId;       // Sentinel: defines ending property values
    unsigned int startTick;         // Millisecond tick when animation began
    unsigned int typeMask;          // Bitmask of properties to animate
    float animateTimeSec;           // Duration in seconds (interval for JUMP types)
    float accelPixelPerSecX;        // X acceleration in px/s² (ACCL only)
    float accelPixelPerSecY;        // Y acceleration in px/s² (ACCL only)
    float accelDegPerSec;           // Rotation acceleration in deg/s² (ACCL only)
    float randomPercent;            // Jump probability 0.0–1.0 (JUMPRANDOM only)
    bool  isActive;                 // false = animation is paused / complete
    bool  rotateClockwise;          // Rotation direction for NORMAL/ROTATE_MASK
    gfxLoopType loop;               // GFX_NOLOOP, GFX_RESTART, GFX_REVERSE
    gfxAnimType animType;           // GFX_ANIM_NORMAL, ACCL, JUMP, JUMPRANDOM
    float initialVelocityX;         // Starting X velocity in px/s (ACCL only)
    float initialVelocityY;         // Starting Y velocity in px/s (ACCL only)
    float initialVelocityDeg;       // Starting rotation velocity in deg/s (ACCL only)
};
```

Do not write to the internal `currentVelocity*` fields; they are managed by the engine.

---

### 12.6 gfxLoadAnimateData()

Fills an `stAnimateData` struct with all parameters.  Use this when you need acceleration,
random, or velocity control.

```cpp
void gfxLoadAnimateData(stAnimateData* animateData,
                        unsigned int animateSpriteId,
                        unsigned int startSpriteId,
                        unsigned int endSpriteId,
                        unsigned int typeMask,
                        float animateTimeSec,
                        bool isActive,
                        gfxLoopType loop,
                        gfxAnimType animType,
                        unsigned int startTick,         // 0 = use current tick
                        float accelPixelPerSecX,
                        float accelPixelPerSecY,
                        float accelDegPerSec,
                        float randomPercent,
                        bool rotateClockwise,
                        float initialVelocityX,
                        float initialVelocityY,
                        float initialVelocityDeg);
```

Pass `0` for `startTick` to start the animation from the current millisecond tick
(recommended).  Pass an explicit tick value to synchronize multiple animations to the same
start point.

---

### 12.7 gfxLoadAnimateDataShort()

Simplified version for the common case: linear animation with no acceleration, no random
behavior, and no initial velocity.  All optional parameters are set to zero internally.

```cpp
void gfxLoadAnimateDataShort(stAnimateData* animateData,
                             unsigned int animateSpriteId,
                             unsigned int startSpriteId,
                             unsigned int endSpriteId,
                             unsigned int typeMask,
                             float animateTimeSec,
                             bool isActive,
                             gfxLoopType loop,
                             gfxAnimType animType);
```

---

### 12.8 gfxCreateAnimation()

Registers a completed `stAnimateData` struct with the animation engine.

```cpp
bool gfxCreateAnimation(stAnimateData animateData, bool replaceExisting);
```

- `replaceExisting = true` — remove any existing animation for `animateSpriteId` and replace it.
- `replaceExisting = false` — return `false` (no-op) if an animation already exists for that sprite.

**Returns** `false` if any of the three sprite IDs are not found, or if the start and end
sprites do not share the same parent texture.

When created, the engine copies the start sprite's properties into the animate sprite so it
begins at the correct starting state.

---

### 12.9 gfxAnimateSprite()

Updates the animate sprite's properties based on the elapsed time.  Call once per frame before
rendering the animate sprite.

```cpp
bool gfxAnimateSprite(unsigned int animateSpriteId, unsigned int currentTick);
```

Pass `NOSPRITE` (0) as `animateSpriteId` to update **all** registered animations in one call.

---

### 12.10 Animation control functions

| Function | Description |
|----------|-------------|
| `gfxAnimateActive(spriteId)` | `true` if a specific animation is active. Pass `NOSPRITE` to return `true` if *any* animation is active. |
| `gfxAnimateClear(spriteId)` | Remove a specific animation. Pass `NOSPRITE` to clear *all* animations. |
| `gfxAnimateRestart(spriteId)` | Restart a specific animation from the current tick. Resets velocity for `GFX_ANIM_ACCL`. |
| `gfxAnimateRestart(spriteId, startTick)` | Restart with an explicit start tick (for synchronizing multiple animations). |

---

### Animation examples

**Slide text onto the screen (linear, no loop):**

```cpp
// In load:
m_textStart = gfxInstanceSprite(m_fontId);
gfxSetXY(m_textStart, -300, 100, false);   // off-screen left

m_textEnd = gfxInstanceSprite(m_fontId);
gfxSetXY(m_textEnd, 100, 100, false);      // final position

m_textAnim = gfxInstanceSprite(m_fontId);

gfxLoadAnimateDataShort(&m_slideAnim,
                        m_textAnim, m_textStart, m_textEnd,
                        ANIMATE_X_MASK, 0.5f, true,
                        GFX_NOLOOP, GFX_ANIM_NORMAL);
gfxCreateAnimation(m_slideAnim, true);

// In render:
gfxAnimateSprite(m_textAnim, currentTick);
gfxRenderString(m_textAnim, "Ready!", spacingPixels, GFX_TEXTLEFT);
```

**Pulsing glow (scale ping-pong):**

```cpp
m_glowStart = gfxInstanceSprite(m_glowId);
gfxSetScaleFactor(m_glowStart, 0.9f, false);

m_glowEnd = gfxInstanceSprite(m_glowId);
gfxSetScaleFactor(m_glowEnd, 1.1f, false);

m_glowAnim = gfxInstanceSprite(m_glowId);

gfxLoadAnimateDataShort(&m_pulseAnim,
                        m_glowAnim, m_glowStart, m_glowEnd,
                        ANIMATE_SCALE_MASK, 0.8f, true,
                        GFX_REVERSE, GFX_ANIM_NORMAL);
gfxCreateAnimation(m_pulseAnim, true);
```

**Blinking LED (jump, loop):**

```cpp
m_ledOn  = gfxInstanceSprite(m_ledId);
gfxSetColor(m_ledOn,  255, 255,   0, 255);  // yellow

m_ledOff = gfxInstanceSprite(m_ledId);
gfxSetColor(m_ledOff, 40,  40,   0, 255);   // dim

m_ledAnim = gfxInstanceSprite(m_ledId);

gfxLoadAnimateDataShort(&m_blinkAnim,
                        m_ledAnim, m_ledOn, m_ledOff,
                        ANIMATE_COLOR_MASK, 0.4f, true,
                        GFX_RESTART, GFX_ANIM_JUMP);
gfxCreateAnimation(m_blinkAnim, true);
```

**Projectile with gravity (acceleration):**

```cpp
m_ballStart = gfxInstanceSprite(m_ballId);
gfxSetXY(m_ballStart, 400, 50, false);

m_ballEnd = gfxInstanceSprite(m_ballId);
gfxSetXY(m_ballEnd, 400, 900, false);   // well below screen

m_ballAnim = gfxInstanceSprite(m_ballId);

gfxLoadAnimateData(&m_gravAnim,
                   m_ballAnim, m_ballStart, m_ballEnd,
                   ANIMATE_Y_MASK,
                   2.0f,    // animateTimeSec (ignored by ACCL)
                   true,
                   GFX_NOLOOP,
                   GFX_ANIM_ACCL,
                   0,       // startTick = now
                   0.0f,    // accelPixelPerSecX
                   400.0f,  // accelPixelPerSecY (downward gravity)
                   0.0f,    // accelDegPerSec
                   0.0f,    // randomPercent
                   true,    // rotateClockwise (unused here)
                   0.0f,    // initialVelocityX
                   0.0f,    // initialVelocityY
                   0.0f);   // initialVelocityDeg
gfxCreateAnimation(m_gravAnim, true);
```

---

## 13. Video Texture Sprites

`PBVideoPlayer` manages video playback and feeds decoded frames to a sprite via
`gfxUpdateVideoTexture()`.  You do not normally call this directly — use the `PBVideoPlayer`
API instead.  The sprite must have been loaded with `textureType = GFX_VIDEO`.

```cpp
bool gfxUpdateVideoTexture(unsigned int spriteId,
                           const uint8_t* frameData,
                           unsigned int width,
                           unsigned int height);
```

Returns `false` if the sprite is not a video type, is not loaded, or if the supplied
dimensions do not match the texture dimensions.  The `PBVideoPlayer` layer handles all of
this automatically.

For details on loading and playing video, see [PBVideoPlayer API](PBVideo_API.md).

---

## 14. Complete Example — Animated Torch

This example demonstrates the full lifecycle: load, instance, configure, animate, and render.

```cpp
// ── In the load function ──────────────────────────────────────────────

// Load the torch PNG (centered, resident)
m_torchId = gfxLoadSprite("Torch",
                          "src/user/resources/textures/torch.png",
                          GFX_PNG, GFX_NOMAP, GFX_CENTER, true, true);
if (m_torchId == NOSPRITE) return false;
gfxSetColor(m_torchId, 255, 180, 60, 255);   // warm orange tint

// Create start / end / animate instances for scale pulse
m_torchSmall = gfxInstanceSprite(m_torchId);
gfxSetXY(m_torchSmall, 400, 300, false);
gfxSetScaleFactor(m_torchSmall, 0.85f, false);
gfxSetColor(m_torchSmall, 220, 100, 20, 255);

m_torchBig = gfxInstanceSprite(m_torchId);
gfxSetXY(m_torchBig, 400, 300, false);
gfxSetScaleFactor(m_torchBig, 1.15f, false);
gfxSetColor(m_torchBig, 255, 200, 80, 255);

m_torchAnim = gfxInstanceSprite(m_torchId);

// Random-jump shimmer on scale + color over 0.12-second intervals
gfxLoadAnimateData(&m_torchFlicker,
                   m_torchAnim, m_torchSmall, m_torchBig,
                   ANIMATE_SCALE_MASK | ANIMATE_COLOR_MASK,
                   0.12f, true,
                   GFX_RESTART, GFX_ANIM_JUMPRANDOM,
                   0,
                   0.0f, 0.0f, 0.0f,
                   0.65f,   // 65% chance to jump each interval
                   true, 0.0f, 0.0f, 0.0f);
gfxCreateAnimation(m_torchFlicker, true);

// ── In the render function ─────────────────────────────────────────────

gfxAnimateSprite(m_torchAnim, currentTick);
gfxRenderSprite(m_torchAnim);
```

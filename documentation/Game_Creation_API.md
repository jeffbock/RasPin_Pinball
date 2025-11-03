# Game Creation API Reference

## Overview

This guide covers creating and animating game screens using the PBGfx graphics class and PBSound audio engine. These are the core tools for building visual and audio experiences in your pinball table, from attract modes to gameplay screens.

## Key Concepts

- **Sprites**: Images loaded into memory that can be rendered, scaled, rotated, and colored
- **Instances**: Multiple renderable versions of a sprite with different positions/properties
- **Animations**: Automatic interpolation between sprite states over time
- **Screen Management**: Load and render functions that control what's displayed
- **Sound Effects**: Multiple simultaneous audio channels for music and effects

---

## Screen Creation Pattern

All game screens follow a consistent pattern with load and render functions.

### Load Function Pattern

Load functions are called once to load resources (sprites, sounds) into memory.

**Pattern:**
```cpp
bool PBEngine::pbeLoadYourScreen(bool forceReload) {
    static bool screenLoaded = false;
    if (forceReload) screenLoaded = false;
    if (screenLoaded) return true;
    
    // Load sprites, fonts, sounds
    m_yourSpriteId = gfxLoadSprite("SpriteName", "path/to/texture.png", 
                                    GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, 
                                    true, true);
    
    // Configure sprite properties
    gfxSetColor(m_yourSpriteId, 255, 255, 255, 255);
    gfxSetScaleFactor(m_yourSpriteId, 1.0, false);
    
    // Create animations if needed
    // (see Animation section)
    
    screenLoaded = true;
    return screenLoaded;
}
```

**Example from Sandbox Screen:**
```cpp
bool PBEngine::pbeLoadTestSandbox(bool forceReload) {
    // Reuse start menu assets
    if (!pbeLoadStartMenu(false)) return false; 
    return true;
}
```

### Render Function Pattern

Render functions are called every frame to draw the screen.

**Pattern:**
```cpp
bool PBEngine::pbeRenderYourScreen(unsigned long currentTick, unsigned long lastTick) {
    // Load resources if not already loaded
    if (!pbeLoadYourScreen(false)) return false;
    
    // Initialize or reset state variables
    if (m_RestartYourScreen) {
        m_RestartYourScreen = false;
        // Reset timers, counters, etc.
    }
    
    // Clear screen
    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
    
    // Render background
    gfxRenderSprite(m_backgroundId, 0, 0);
    
    // Render animated elements
    gfxAnimateSprite(m_animatedSpriteId, currentTick);
    gfxRenderSprite(m_animatedSpriteId);
    
    // Render text
    gfxSetColor(m_fontId, 255, 255, 255, 255);
    gfxRenderString(m_fontId, "Your Text", x, y, spacing, GFX_TEXTCENTER);
    
    return true;
}
```

**Example from Sandbox Screen:**
```cpp
bool PBEngine::pbeRenderTestSandbox(unsigned long currentTick, unsigned long lastTick) {
    if (!pbeLoadTestSandbox(false)) return false;
    
    if (m_RestartTestSandbox) {
        m_RestartTestSandbox = false;
    }
    
    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
    pbeRenderDefaultBackground(currentTick, lastTick);
    
    // Title
    gfxSetColor(m_StartMenuFontId, 255, 165, 0, 255);
    gfxSetScaleFactor(m_StartMenuFontId, 2.0, false);
    gfxRenderShadowString(m_StartMenuFontId, "Test Sandbox", 
                          PB_SCREENWIDTH/2, 15, 2, GFX_TEXTCENTER, 
                          0, 0, 0, 255, 6);
    
    return true;
}
```

---

## PBGfx Class - Graphics System

The `PBGfx` class handles all sprite loading, rendering, animation, and text display.

### Loading Sprites

#### gfxLoadSprite()

Loads an image file into memory as a sprite.

**Signature:**
```cpp
unsigned int gfxLoadSprite(const std::string& spriteName, 
                           const std::string& textureFileName, 
                           gfxTexType textureType,
                           gfxSpriteMap mapType, 
                           gfxTexCenter textureCenter, 
                           bool keepResident, 
                           bool useTexture);
```

**Parameters:**
- `spriteName` - Unique identifier for the sprite
- `textureFileName` - Path to image file
- `textureType` - `GFX_PNG` or `GFX_BMP`
- `mapType` - `GFX_NOMAP` (normal), `GFX_TEXTMAP` (font), `GFX_SPRITEMAP` (sprite sheet)
- `textureCenter` - `GFX_UPPERLEFT` or `GFX_CENTER`
- `keepResident` - Keep texture in memory (`true` recommended)
- `useTexture` - Enable texture rendering (`true` for images)

**Returns:** Sprite ID (use to reference this sprite later)

**Example - Load Background:**
```cpp
m_backgroundId = gfxLoadSprite("Background", 
                               "src/resources/textures/Backglass.png", 
                               GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, 
                               true, true);
```

**Example - Load Centered Sprite:**
```cpp
m_flameId = gfxLoadSprite("Flame1", 
                          "src/resources/textures/flame1.png", 
                          GFX_PNG, GFX_NOMAP, GFX_CENTER, 
                          false, true);
```

**Example - Load Font:**
```cpp
m_fontId = gfxLoadSprite("MenuFont", 
                         "src/resources/fonts/Baldur_96_768.png", 
                         GFX_PNG, GFX_TEXTMAP, GFX_UPPERLEFT, 
                         true, true);
```

### Configuring Sprites

#### gfxSetColor()

Sets the color tint and alpha transparency for a sprite.

**Signature:**
```cpp
unsigned int gfxSetColor(unsigned int spriteId, 
                        unsigned int red, unsigned int green, 
                        unsigned int blue, unsigned int alpha);
```

**Parameters:**
- `spriteId` - Sprite to modify
- `red`, `green`, `blue` - Color values (0-255)
- `alpha` - Transparency (0=invisible, 255=opaque)

**Example:**
```cpp
// Full color, opaque
gfxSetColor(m_spriteId, 255, 255, 255, 255);

// Red tint
gfxSetColor(m_spriteId, 255, 0, 0, 255);

// Semi-transparent
gfxSetColor(m_spriteId, 255, 255, 255, 128);

// Orange color
gfxSetColor(m_fontId, 255, 165, 0, 255);
```

#### gfxSetScaleFactor()

Scales a sprite larger or smaller.

**Signature:**
```cpp
unsigned int gfxSetScaleFactor(unsigned int spriteId, 
                               float scaleFactor, 
                               bool addFactor);
```

**Parameters:**
- `spriteId` - Sprite to scale
- `scaleFactor` - Scale multiplier (1.0 = original size, 2.0 = double, 0.5 = half)
- `addFactor` - Add to current scale (`true`) or replace (`false`)

**Example:**
```cpp
// Double size
gfxSetScaleFactor(m_titleId, 2.0, false);

// 80% of original
gfxSetScaleFactor(m_dragonId, 0.8, false);

// Grow by 10%
gfxSetScaleFactor(m_flameId, 0.1, true);
```

#### gfxSetXY()

Sets the position of a sprite.

**Signature:**
```cpp
unsigned int gfxSetXY(unsigned int spriteId, int X, int Y, bool addXY);
```

**Parameters:**
- `spriteId` - Sprite to position
- `X`, `Y` - Screen coordinates
- `addXY` - Add to current position (`true`) or replace (`false`)

**Example:**
```cpp
// Absolute position
gfxSetXY(m_spriteId, 100, 200, false);

// Move 10 pixels right
gfxSetXY(m_spriteId, 10, 0, true);
```

#### gfxSetRotateDegrees()

Rotates a sprite around its center point.

**Signature:**
```cpp
unsigned int gfxSetRotateDegrees(unsigned int spriteId, 
                                float rotateDegrees, 
                                bool addDegrees);
```

**Example:**
```cpp
// Rotate 45 degrees
gfxSetRotateDegrees(m_swordId, 45.0, false);

// Rotate another 5 degrees
gfxSetRotateDegrees(m_swordId, 5.0, true);
```

### Rendering Sprites

#### gfxRenderSprite()

Draws a sprite to the screen using current properties.

**Signatures:**
```cpp
bool gfxRenderSprite(unsigned int spriteId);
bool gfxRenderSprite(unsigned int spriteId, int x, int y);
bool gfxRenderSprite(unsigned int spriteId, int x, int y, 
                    float scaleFactor, float rotateDegrees);
```

**Example - Using Current Properties:**
```cpp
// Render with previously set position/scale/color
gfxRenderSprite(m_backgroundId);
```

**Example - Specify Position:**
```cpp
// Render at specific coordinates
gfxRenderSprite(m_doorId, 315, 112);
```

**Example - Full Control:**
```cpp
// Render with inline position, scale, rotation
gfxRenderSprite(m_swordId, 500, 300, 1.5, 45.0);
```

### Text Rendering

#### gfxRenderString()

Renders text using a font sprite.

**Signature:**
```cpp
bool gfxRenderString(unsigned int spriteId, std::string input, 
                    int x, int y, unsigned int spacingPixels, 
                    gfxTextJustify justify);
```

**Parameters:**
- `spriteId` - Font sprite ID
- `input` - Text to display
- `x`, `y` - Position
- `spacingPixels` - Space between characters
- `justify` - `GFX_TEXTLEFT`, `GFX_TEXTCENTER`, `GFX_TEXTRIGHT`

**Example:**
```cpp
gfxSetColor(m_fontId, 255, 255, 255, 255);
gfxRenderString(m_fontId, "PRESS START", 
               PB_SCREENWIDTH/2, 500, 2, GFX_TEXTCENTER);
```

#### gfxRenderShadowString()

Renders text with a drop shadow for better visibility.

**Signature:**
```cpp
bool gfxRenderShadowString(unsigned int spriteId, std::string input, 
                          int x, int y, unsigned int spacingPixels, 
                          gfxTextJustify justify,
                          unsigned int red, unsigned int green, 
                          unsigned int blue, unsigned int alpha, 
                          unsigned int shadowOffset);
```

**Example:**
```cpp
gfxSetColor(m_fontId, 255, 165, 0, 255);  // Orange text
gfxSetScaleFactor(m_fontId, 2.0, false);  // Double size
gfxRenderShadowString(m_fontId, "Test Sandbox", 
                     PB_SCREENWIDTH/2, 15, 2, GFX_TEXTCENTER, 
                     0, 0, 0, 255, 6);  // Black shadow, 6px offset
```

#### gfxStringWidth()

Calculates the pixel width of a text string (useful for centering).

**Signature:**
```cpp
int gfxStringWidth(unsigned int spriteId, std::string input, 
                  unsigned int spacingPixels);
```

**Example:**
```cpp
std::string text = "High Score: 1000000";
int width = gfxStringWidth(m_fontId, text, 1);
int centerX = (PB_SCREENWIDTH - width) / 2;
gfxRenderString(m_fontId, text, centerX, 100, 1, GFX_TEXTLEFT);
```

---

## Sprite Instances

Instances allow multiple renderable versions of the same sprite with different properties.

### gfxInstanceSprite()

Creates an instance of a sprite with its own properties.

**Signature:**
```cpp
unsigned int gfxInstanceSprite(unsigned int parentSpriteId);
unsigned int gfxInstanceSprite(unsigned int parentSpriteId, 
                              int x, int y, 
                              unsigned int textureAlpha,
                              unsigned int vertRed, unsigned int vertGreen, 
                              unsigned int vertBlue, unsigned int vertAlpha, 
                              float scaleFactor, float rotateDegrees);
```

**Returns:** Instance ID (use like a sprite ID)

**Example - Basic Instance:**
```cpp
// Load the base sprite
m_doorId = gfxLoadSprite("LeftDoor", "textures/DoorLeft.png", ...);

// Create instances at different positions
m_doorStartId = gfxInstanceSprite(m_doorId);
gfxSetXY(m_doorStartId, 315, 112, false);

m_doorEndId = gfxInstanceSprite(m_doorId);
gfxSetXY(m_doorEndId, 90, 112, false);
```

**Example - Instance with Properties:**
```cpp
m_flameInstance = gfxInstanceSprite(m_flameId, 
                                   400, 300,     // x, y position
                                   255,          // alpha
                                   255, 255, 255, 255,  // color
                                   1.2,          // scale
                                   0.0);         // rotation
```

**Use Case:**
Instances are essential for animations where you need to interpolate between different states of the same image.

---

## Animations

Animations automatically interpolate sprite properties over time without manual frame-by-frame control.

### Animation Structure

```cpp
struct stAnimateData {
    unsigned int animateSpriteId;   // Sprite to animate
    unsigned int startSpriteId;     // Starting state instance
    unsigned int endSpriteId;       // Ending state instance
    unsigned int startTick;         // When to start (0 = immediate)
    unsigned int typeMask;          // What properties to animate
    float animateTimeSec;           // Duration in seconds
    float accelPixPerSec;           // Acceleration (usually 0.0)
    bool isActive;                  // Start active
    bool rotateClockwise;           // Rotation direction
    gfxLoopType loop;              // Loop behavior
};
```

### Animation Type Masks

Control which properties are animated:

```cpp
#define ANIMATE_X_MASK           0x1   // Animate X position
#define ANIMATE_Y_MASK           0x2   // Animate Y position
#define ANIMATE_U_MASK           0x4   // Animate U texture coord
#define ANIMATE_V_MASK           0x8   // Animate V texture coord
#define ANIMATE_TEXALPHA_MASK    0x10  // Animate texture alpha
#define ANIMATE_COLOR_MASK       0x20  // Animate color
#define ANIMATE_SCALE_MASK       0x40  // Animate scale factor
#define ANIMATE_ROTATE_MASK      0x80  // Animate rotation
#define ANIMATE_ALL_MASK         0xFF  // Animate everything
```

### Loop Types

```cpp
enum gfxLoopType {
    GFX_NOLOOP = 0,    // Play once and stop
    GFX_RESTART = 1,   // Loop continuously from start
    GFX_REVERSE = 2    // Ping-pong back and forth
};
```

### Creating Animations

#### gfxLoadAnimateData()

Helper function to populate animation structure.

**Signature:**
```cpp
void gfxLoadAnimateData(stAnimateData *animateData, 
                       unsigned int animateSpriteId, 
                       unsigned int startSpriteId, 
                       unsigned int endSpriteId, 
                       unsigned int startTick,
                       unsigned int typeMask, 
                       float animateTimeSec, 
                       float accelPixPerSec, 
                       bool isActive, 
                       bool rotateClockwise, 
                       gfxLoopType loop);
```

#### gfxCreateAnimation()

Registers an animation with the system.

**Signature:**
```cpp
bool gfxCreateAnimation(stAnimateData animateData, bool replaceExisting);
```

**Complete Example - Sliding Door:**
```cpp
// Load sprite
m_leftDoorId = gfxLoadSprite("LeftDoor", "textures/DoorLeft.png", 
                             GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, 
                             true, true);

// Create start position instance
m_doorStartId = gfxInstanceSprite(m_leftDoorId);
gfxSetXY(m_doorStartId, 315, 112, false);

// Create end position instance
m_doorEndId = gfxInstanceSprite(m_leftDoorId);
gfxSetXY(m_doorEndId, 90, 112, false);

// Set up animation data
stAnimateData animateData;
gfxLoadAnimateData(&animateData, 
                   m_leftDoorId,        // Sprite to animate
                   m_doorStartId,       // Start instance
                   m_doorEndId,         // End instance
                   0,                   // Start immediately
                   ANIMATE_X_MASK,      // Animate X position only
                   1.25f,               // 1.25 seconds duration
                   0.0f,                // No acceleration
                   false,               // Not active yet
                   true,                // N/A for position
                   GFX_NOLOOP);        // Play once

// Register the animation
gfxCreateAnimation(animateData, true);
```

**Example - Pulsing Flame:**
```cpp
m_flameId = gfxLoadSprite("Flame", "textures/flame.png", 
                         GFX_PNG, GFX_NOMAP, GFX_CENTER, 
                         false, true);

// Small instance
m_flameStartId = gfxInstanceSprite(m_flameId);
gfxSetScaleFactor(m_flameStartId, 1.1, false);

// Large instance
m_flameEndId = gfxInstanceSprite(m_flameId);
gfxSetScaleFactor(m_flameEndId, 1.2, false);

stAnimateData animateData;
gfxLoadAnimateData(&animateData, 
                   m_flameId, 
                   m_flameStartId, 
                   m_flameEndId, 
                   0, 
                   ANIMATE_SCALE_MASK,  // Animate scale
                   1.0f,                // 1 second
                   0.0f, 
                   true,                // Start active
                   true, 
                   GFX_REVERSE);       // Ping-pong

gfxCreateAnimation(animateData, true);
```

**Example - Text Fade In:**
```cpp
// Create transparent instance
m_textStartId = gfxInstanceSprite(m_fontId);
gfxSetColor(m_textStartId, 0, 0, 0, 0);  // Invisible

// Create opaque instance
m_textEndId = gfxInstanceSprite(m_fontId);
gfxSetColor(m_textEndId, 255, 255, 255, 255);  // Fully visible

stAnimateData animateData;
gfxLoadAnimateData(&animateData, 
                   m_fontId, 
                   m_textStartId, 
                   m_textEndId, 
                   0, 
                   ANIMATE_COLOR_MASK,  // Fade color/alpha
                   2.0f,                // 2 seconds
                   0.0f, 
                   true, 
                   true, 
                   GFX_NOLOOP);

gfxCreateAnimation(animateData, true);
```

### Running Animations

#### gfxAnimateSprite()

Updates animation state based on elapsed time.

**Signature:**
```cpp
bool gfxAnimateSprite(unsigned int animateSpriteId, 
                     unsigned int currentTick);
```

**Parameters:**
- `animateSpriteId` - Sprite with animation
- `currentTick` - Current frame timestamp (from `GetTickCountGfx()`)

**Usage Pattern:**
```cpp
// In render function
unsigned long currentTick = GetTickCountGfx();

// Update animation
gfxAnimateSprite(m_doorId, currentTick);

// Render sprite with updated properties
gfxRenderSprite(m_doorId);
```

#### Animation Control Functions

```cpp
// Check if animation is active
bool gfxAnimateActive(unsigned int animateSpriteId);

// Restart animation from beginning
bool gfxAnimateRestart(unsigned int animateSpriteId);
bool gfxAnimateRestart(unsigned int animateSpriteId, unsigned long startTick);

// Stop animation
bool gfxAnimateClear(unsigned int animateSpriteId);
```

**Example - Conditional Animation:**
```cpp
if (m_openDoors) {
    if (!gfxAnimateActive(m_leftDoorId)) {
        // Start door animation
        gfxAnimateRestart(m_leftDoorId, currentTick);
    }
    gfxAnimateSprite(m_leftDoorId, currentTick);
    gfxRenderSprite(m_leftDoorId);
} else {
    // Render door in closed position
    gfxRenderSprite(m_leftDoorId, 315, 112);
}
```

---

## Complete Screen Example

**Game Start Screen from Pinball_Table.cpp:**

```cpp
// Load function - called once
bool PBEngine::pbeLoadGameStart(bool forceReload) {
    static bool gameStartLoaded = false;
    if (forceReload) gameStartLoaded = false;
    if (gameStartLoaded) return true;
    
    stAnimateData animateData;
    
    // Load background
    m_PBTBLBackglassId = gfxLoadSprite("Backglass", 
                                       "src/resources/textures/Backglass.png", 
                                       GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, 
                                       true, true);
    gfxSetColor(m_PBTBLBackglassId, 255, 255, 255, 255);
    
    // Load door sprites
    m_PBTBLLeftDoorId = gfxLoadSprite("LeftDoor", 
                                      "src/resources/textures/DoorLeft2.png", 
                                      GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, 
                                      true, true);
    
    // Create door animation - closed to open
    m_PBTBLeftDoorStartId = gfxInstanceSprite(m_PBTBLLeftDoorId);
    gfxSetXY(m_PBTBLeftDoorStartId, 315, 112, false);
    
    m_PBTBLeftDoorEndId = gfxInstanceSprite(m_PBTBLLeftDoorId);
    gfxSetXY(m_PBTBLeftDoorEndId, 90, 112, false);
    
    gfxLoadAnimateData(&animateData, 
                       m_PBTBLLeftDoorId, 
                       m_PBTBLeftDoorStartId, 
                       m_PBTBLeftDoorEndId, 
                       0, ANIMATE_X_MASK, 1.25f, 0.0f, 
                       false, true, GFX_NOLOOP);
    gfxCreateAnimation(animateData, true);
    
    // Load flame sprite (centered for pulsing)
    m_PBTBLFlame1Id = gfxLoadSprite("Flame1", 
                                    "src/resources/textures/flame1.png", 
                                    GFX_PNG, GFX_NOMAP, GFX_CENTER, 
                                    false, true);
    gfxSetColor(m_PBTBLFlame1Id, 255, 255, 255, 92);  // Semi-transparent
    
    // Create pulsing animation
    m_PBTBLFlame1StartId = gfxInstanceSprite(m_PBTBLFlame1Id);
    gfxSetScaleFactor(m_PBTBLFlame1StartId, 1.1, false);
    
    m_PBTBLFlame1EndId = gfxInstanceSprite(m_PBTBLFlame1Id);
    gfxSetScaleFactor(m_PBTBLFlame1EndId, 1.2, false);
    
    gfxLoadAnimateData(&animateData, 
                       m_PBTBLFlame1Id, 
                       m_PBTBLFlame1StartId, 
                       m_PBTBLFlame1EndId, 
                       0, ANIMATE_SCALE_MASK, 1.0f, 0.0f, 
                       true, true, GFX_REVERSE);
    gfxCreateAnimation(animateData, true);
    
    gameStartLoaded = true;
    return gameStartLoaded;
}

// Render function - called every frame
bool PBEngine::pbeRenderGameStart(unsigned long currentTick, unsigned long lastTick) {
    // Initialize state on first run
    static bool doorsOpened = false;
    
    if (m_RestartTable) {
        m_RestartTable = false;
        doorsOpened = false;
    }
    
    // Ensure resources loaded
    if (!pbeLoadGameStart(false)) return false;
    
    // Clear screen to black
    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
    
    // Render static background
    gfxRenderSprite(m_PBTBLBackglassId, 0, 0);
    
    // Render doors - animate if opening
    if (!doorsOpened) {
        gfxRenderSprite(m_PBTBLLeftDoorId, 315, 112);  // Closed position
    } else {
        gfxAnimateSprite(m_PBTBLLeftDoorId, currentTick);
        gfxRenderSprite(m_PBTBLLeftDoorId);  // Animated position
    }
    
    // Render animated flames (always pulsing)
    gfxAnimateSprite(m_PBTBLFlame1Id, currentTick);
    gfxRenderSprite(m_PBTBLFlame1Id, 400, 300);
    
    // Render text
    gfxSetColor(m_StartMenuFontId, 255, 165, 0, 255);
    gfxSetScaleFactor(m_StartMenuFontId, 1.25, false);
    gfxRenderShadowString(m_StartMenuFontId, "ENTER THE DUNGEON", 
                         PB_SCREENWIDTH/2, 500, 2, GFX_TEXTCENTER, 
                         0, 0, 0, 255, 4);
    
    return true;
}
```

---

## PBSound Class - Audio System

The `PBSound` class provides background music and sound effect playback with volume control.

### Initialization

**Signature:**
```cpp
bool pbsInitialize();
```

**Called automatically during startup:**
```cpp
// In PBInitRender()
g_PBEngine.m_soundSystem.pbsInitialize();
```

### Background Music

#### pbsPlayMusic()

Plays looping background music.

**Signature:**
```cpp
bool pbsPlayMusic(const std::string& mp3FilePath);
```

**Example:**
```cpp
#define MUSICFANTASY "src/resources/sound/fantasymusic.mp3"

// Start background music
g_PBEngine.m_soundSystem.pbsPlayMusic(MUSICFANTASY);
```

#### pbsStopMusic()

Stops the currently playing music.

**Signature:**
```cpp
void pbsStopMusic();
```

**Example:**
```cpp
// Stop music when game ends
g_PBEngine.m_soundSystem.pbsStopMusic();
```

### Sound Effects

The system supports up to 4 simultaneous sound effects.

#### pbsPlayEffect()

Plays a sound effect on an available channel.

**Signature:**
```cpp
int pbsPlayEffect(const std::string& mp3FilePath);
```

**Returns:** Effect ID (1-4) for tracking, or 0 on failure

**Example:**
```cpp
#define EFFECTSWORDHIT "src/resources/sound/swordcut.mp3"
#define EFFECTCLICK "src/resources/sound/click.mp3"

// Play effect when target hit
int effectId = g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTSWORDHIT);

// Play menu click
g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTCLICK);
```

#### pbsIsEffectPlaying()

Check if an effect is still playing.

**Signature:**
```cpp
bool pbsIsEffectPlaying(int effectId);
```

**Example:**
```cpp
int effectId = g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTSWORDHIT);

// Wait for effect to finish before playing next
if (!g_PBEngine.m_soundSystem.pbsIsEffectPlaying(effectId)) {
    // Effect finished, play next sound
}
```

#### pbsStopEffect()

Stop a specific effect.

**Signature:**
```cpp
void pbsStopEffect(int effectId);
```

#### pbsStopAllEffects()

Stop all currently playing effects.

**Signature:**
```cpp
void pbsStopAllEffects();
```

### Volume Control

#### pbsSetMasterVolume()

Sets overall system volume (affects music and effects).

**Signature:**
```cpp
void pbsSetMasterVolume(int volume);
```

**Parameters:**
- `volume` - Volume level (0-100%)

**Example:**
```cpp
g_PBEngine.m_soundSystem.pbsSetMasterVolume(100);  // Full volume
```

#### pbsSetMusicVolume()

Sets music volume independently.

**Signature:**
```cpp
void pbsSetMusicVolume(int volume);
```

**Example:**
```cpp
// Load volume from saved settings
int musicVol = g_PBEngine.m_saveFileData.musicVolume * 10;
g_PBEngine.m_soundSystem.pbsSetMusicVolume(musicVol);
```

**Example - Complete Audio Setup:**
```cpp
// Initialize sound system (done automatically)
g_PBEngine.m_soundSystem.pbsInitialize();

// Set volumes
g_PBEngine.m_soundSystem.pbsSetMasterVolume(100);
g_PBEngine.m_soundSystem.pbsSetMusicVolume(80);

// Start background music
g_PBEngine.m_soundSystem.pbsPlayMusic(MUSICFANTASY);
```

---

## Screen State Management

Use state flags to control screen behavior and trigger events.

**Example Pattern:**
```cpp
// In class header
bool m_RestartYourScreen;
unsigned long m_animationStartTick;
bool m_animationComplete;

// In render function
if (m_RestartYourScreen) {
    m_RestartYourScreen = false;
    m_animationStartTick = currentTick;
    m_animationComplete = false;
    
    // Reset animations
    gfxAnimateRestart(m_doorId, currentTick);
}

// Check animation completion
if (!m_animationComplete && gfxAnimateActive(m_doorId)) {
    unsigned long elapsed = currentTick - m_animationStartTick;
    if (elapsed > 2000) {  // 2 seconds
        m_animationComplete = true;
        // Trigger next event
    }
}
```

---

## Best Practices

### Performance

- **Keep Resident:** Set `keepResident = true` for frequently used sprites
- **Batch Operations:** Set sprite properties before calling render
- **Reuse Instances:** Create instances once in load function
- **Unload Unused:** Call `gfxUnloadTexture()` for one-time screens

### Organization

- **Consistent Naming:** Use descriptive sprite IDs (m_backgroundId, m_titleFontId)
- **Group Related:** Load related sprites together
- **Static Flags:** Use static booleans in load functions to prevent reloading
- **Member Variables:** Store sprite IDs as class members for access across functions

### Animation

- **Instance First:** Always create instances before animating
- **Clear Timing:** Use realistic durations (1-2 seconds typical)
- **Loop Wisely:** Use GFX_REVERSE for pulsing, GFX_RESTART for continuous motion
- **Layering:** Animate properties separately (position, scale, color) for complex effects

### Audio

- **Define Paths:** Use #define for audio file paths
- **Volume Balance:** Keep music lower than effects (e.g., 80% vs 100%)
- **Limit Simultaneous:** Maximum 4 effects at once
- **Effect Management:** Track effect IDs if you need to stop specific sounds

---

## Complete Game Screen Template

```cpp
// In your .h file
class PBEngine : public PBGfx {
public:
    bool pbeLoadMyScreen(bool forceReload);
    bool pbeRenderMyScreen(unsigned long currentTick, unsigned long lastTick);
    
    // Screen sprite IDs
    unsigned int m_myBackgroundId;
    unsigned int m_myAnimatedId, m_myAnimStartId, m_myAnimEndId;
    unsigned int m_myFontId;
    
    // Screen state
    bool m_RestartMyScreen;
};

// In your .cpp file
bool PBEngine::pbeLoadMyScreen(bool forceReload) {
    static bool loaded = false;
    if (forceReload) loaded = false;
    if (loaded) return true;
    
    // Load background
    m_myBackgroundId = gfxLoadSprite("MyBG", "textures/bg.png", 
                                     GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, 
                                     true, true);
    gfxSetColor(m_myBackgroundId, 255, 255, 255, 255);
    
    // Load animated sprite
    m_myAnimatedId = gfxLoadSprite("MySprite", "textures/sprite.png", 
                                   GFX_PNG, GFX_NOMAP, GFX_CENTER, 
                                   true, true);
    
    // Create animation instances
    m_myAnimStartId = gfxInstanceSprite(m_myAnimatedId);
    gfxSetXY(m_myAnimStartId, 100, 200, false);
    gfxSetScaleFactor(m_myAnimStartId, 1.0, false);
    
    m_myAnimEndId = gfxInstanceSprite(m_myAnimatedId);
    gfxSetXY(m_myAnimEndId, 500, 200, false);
    gfxSetScaleFactor(m_myAnimEndId, 1.5, false);
    
    // Create animation
    stAnimateData animData;
    gfxLoadAnimateData(&animData, m_myAnimatedId, 
                       m_myAnimStartId, m_myAnimEndId, 
                       0, ANIMATE_X_MASK | ANIMATE_SCALE_MASK, 
                       2.0f, 0.0f, true, true, GFX_RESTART);
    gfxCreateAnimation(animData, true);
    
    // Load font
    m_myFontId = gfxLoadSprite("MyFont", "fonts/font.png", 
                               GFX_PNG, GFX_TEXTMAP, GFX_UPPERLEFT, 
                               true, true);
    
    loaded = true;
    return loaded;
}

bool PBEngine::pbeRenderMyScreen(unsigned long currentTick, unsigned long lastTick) {
    // Load resources
    if (!pbeLoadMyScreen(false)) return false;
    
    // Initialize state
    if (m_RestartMyScreen) {
        m_RestartMyScreen = false;
        gfxAnimateRestart(m_myAnimatedId, currentTick);
    }
    
    // Clear screen
    gfxClear(0.0f, 0.0f, 0.2f, 1.0f, false);  // Dark blue
    
    // Render background
    gfxRenderSprite(m_myBackgroundId, 0, 0);
    
    // Render animated sprite
    gfxAnimateSprite(m_myAnimatedId, currentTick);
    gfxRenderSprite(m_myAnimatedId);
    
    // Render text
    gfxSetColor(m_myFontId, 255, 255, 0, 255);  // Yellow
    gfxSetScaleFactor(m_myFontId, 1.5, false);
    gfxRenderShadowString(m_myFontId, "MY SCREEN TITLE", 
                         PB_SCREENWIDTH/2, 50, 2, GFX_TEXTCENTER, 
                         0, 0, 0, 255, 3);
    
    return true;
}
```

---

## See Also

- **PBEngine_API.md** - Core engine class and state management
- **Platform_Init_API.md** - Main loop and initialization
- **UsersGuide.md** - Complete framework documentation

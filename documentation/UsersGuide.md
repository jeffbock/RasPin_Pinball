# PInball User's Guide for Pinball Programmers
## Note:  This was AI generated on 10/8/2025 -  it reflects a certain snapshot of the code and can be incorrect / incomplete but it's a decent start to reflect the operation of the code to date.

## Table of Contents
1. [Introduction](#introduction)
2. [System Architecture Overview](#system-architecture-overview)
3. [Hardware Interface Overview](#hardware-interface-overview)
4. [Software Architecture](#software-architecture)
5. [Creating a New Pinball Machine](#creating-a-new-pinball-machine)
6. [Graphics and Screen Management](#graphics-and-screen-management)
7. [LED Control and Sequences](#led-control-and-sequences)
8. [Function Reference](#function-reference)

---

## Introduction

Welcome to the PInball framework! This guide is designed for programmers who want to create their own pinball machine software using the PInball framework. You don't need to understand the low-level hardware implementation details - this guide focuses on the high-level concepts and APIs you'll need to create engaging pinball experiences.

**What is the RasPin PInball Framework?**
- A cross-platform pinball framework for ~1/2 scale physical pinball machines
- Intended to be a hobby level, low cost system for creating full feature pinball machines with Raspberry Pi hardware
- Cross platform VS Code environment for Raspberry Pi (full HW) and Windows (simulation / fast development)
- Supports a primary Pinball HDMI screen, plus 2nd monitor support for ease of debug / development
- Built on OpenGL ES for graphics rendering, with sprite, animation and text rendering support
- Simplified HW architecture for easy debug, understanding and implemention
- Message based input and output processing utilizing Raspberry Pi, and TI I2C IO and LED expanders optimized to decrease latency and minimize HW traffic
- Automatic LED control and sequence animation for dynamic lighting effects
- Easy to use music and sound effect system with multiple channels

---

## System Architecture Overview

### High-Level System Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                        Pinball.cpp                          │
│                    (Main Entry Point)                       │
│  • Platform-specific initialization                         │
│  • Main game loop                                           │
│  • I/O message processing                                   │
└──────────────────┬──────────────────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────────────────┐
│                      PBEngine Class                         │
│                  (Game State Manager)                       │
│  • State machine (Boot, Menu, Play, Test, etc.)            │
│  • Screen rendering coordination                            │
│  • Input/Output message queuing                             │
│  • Save/Load game data                                      │
└──────┬────────────────┬─────────────────┬──────────────────┘
       │                │                 │
       ▼                ▼                 ▼
┌─────────────┐  ┌─────────────┐  ┌──────────────┐
│  PBGfx      │  │ Pinball_IO  │  │ PBSequences  │
│ (Graphics)  │  │ (Hardware)  │  │ (LED Anims)  │
│             │  │             │  │              │
│ • Sprites   │  │ • Messages  │  │ • Sequences  │
│ • Text      │  │ • IO Chips  │  │ • Timing     │
│ • Animation │  │ • LED Chips │  │ • Patterns   │
└─────────────┘  └─────────────┘  └──────────────┘
```

### Main Components

1. **Pinball.cpp**: Entry point and main game loop
2. **PBEngine**: Core game engine managing all states and rendering
3. **Pinball_Table**: Your custom table implementation
4. **PBGfx**: Graphics rendering system
5. **Pinball_IO**: Hardware I/O definitions and drivers
6. **PBSequences**: Pre-defined LED animation sequences

---

## Hardware Interface Overview

### Hardware Communication Model

The PInball framework uses a **message-based I/O system** to decouple inputs from outputs. This means:

- **Inputs** are detected and placed into a message queue
- **Your code** processes input messages and decides what to do
- **Output messages** are sent to control LEDs, solenoids, etc.

This design gives you complete flexibility in how your pinball machine responds to player actions.

### Message Flow Diagram

```
Input Hardware          Message Queue           Your Code           Output Hardware
─────────────          ─────────────          ──────────          ───────────────

┌──────────┐                                                     ┌──────────┐
│ Flippers │──┐                                              ┌──▶│   LEDs   │
└──────────┘  │                                              │   └──────────┘
              │         ┌──────────────┐    ┌──────────┐    │
┌──────────┐  │         │ Input Queue  │    │ Process  │    │   ┌──────────┐
│ Buttons  │──┼────────▶│ (stInput     │───▶│ Messages │────┼──▶│Solenoids │
└──────────┘  │         │  Message)    │    │          │    │   └──────────┘
              │         └──────────────┘    └──────────┘    │
┌──────────┐  │                                              │   ┌──────────┐
│ Sensors  │──┘         ┌──────────────┐                    └──▶│ Outputs  │
└──────────┘            │ Output Queue │                        └──────────┘
                        │ (stOutput    │
                        │  Message)    │
                        └──────────────┘
```

### Hardware Chips

The framework supports three types of hardware expansion:

#### 1. **IO Chips (TCA9555)** - Input/Output Expander
- **Purpose**: Expand GPIO pins for buttons, sensors, and solenoids
- **Configuration**: Each chip provides 16 pins (configurable as input or output)
- **Default Setup**: 3 chips (IO0, IO1, IO2) at I2C addresses 0x20, 0x21, 0x22
- **Usage**: Define in `Pinball_IO.cpp` with board type `PB_IO`

#### 2. **LED Chips (TLC59116)** - LED Driver
- **Purpose**: Control individual LEDs with brightness and blinking
- **Configuration**: Each chip controls 16 LEDs with PWM brightness control
- **Default Setup**: 3 chips (LED0, LED1, LED2) at I2C addresses 0x60, 0x61, 0x62
- **Features**: Individual brightness, group dimming, group blinking, LED sequences
- **Usage**: Define in `Pinball_IO.cpp` with board type `PB_LED`

#### 3. **Raspberry Pi GPIO**
- **Purpose**: Direct GPIO pins on the Raspberry Pi
- **Configuration**: Uses standard WiringPi pin numbering
- **Usage**: Define in `Pinball_IO.cpp` with board type `PB_RASPI`

### Input/Output Messages

#### Input Message Types (`PBInputMsg`)
- `PB_IMSG_BUTTON`: Player buttons (flippers, start, etc.)
- `PB_IMSG_SENSOR`: Ball detection sensors
- `PB_IMSG_TARGET`: Target switches
- `PB_IMSG_JETBUMPER`: Jet bumper triggers
- `PB_IMSG_POPBUMPER`: Pop bumper triggers

#### Output Message Types (`PBOutputMsg`)
- `PB_OMSG_LED`: Simple LED on/off/blink
- `PB_OMSG_LEDCFG_GROUPDIM`: Configure group dimming
- `PB_OMSG_LEDCFG_GROUPBLINK`: Configure group blinking
- `PB_OMSG_LEDSET_BRIGHTNESS`: Set LED brightness
- `PB_OMSG_LED_SEQUENCE`: Play LED animation sequence
- `PB_OMSG_GENERIC_IO`: Generic I/O output (solenoids, etc.)

---

## Software Architecture

### File Organization

```
src/
├── Pinball.cpp              # Main entry point and game loop
├── Pinball.h                # Main header with platform selection
├── Pinball_Engine.cpp       # Core game engine implementation
├── Pinball_Engine.h         # Engine class definition
├── Pinball_Table.cpp        # YOUR CUSTOM TABLE CODE
├── Pinball_Table.h          # YOUR CUSTOM TABLE DEFINITIONS
├── Pinball_IO.cpp           # I/O definitions and hardware drivers
├── Pinball_IO.h             # I/O structures and chip classes
├── PBGfx.cpp/h              # Graphics rendering system
├── PBSequences.cpp/h        # LED sequence definitions
├── PBSound.cpp/h            # Sound system
└── PinballMenus.h           # Menu text definitions
```

### Pinball.cpp - Main Game Loop

The main game loop in `Pinball.cpp` runs continuously:

```cpp
// Simplified main loop structure
while (running) {
    // 1. Process hardware inputs → input queue
    PBProcessInput();
    
    // 2. Process output queue → hardware
    PBProcessOutput();
    
    // 3. Handle input messages from queue
    if (!g_PBEngine.m_inputQueue.empty()) {
        stInputMessage msg = g_PBEngine.m_inputQueue.front();
        g_PBEngine.m_inputQueue.pop();
        g_PBEngine.pbeUpdateState(msg);
    }
    
    // 4. Render current screen
    g_PBEngine.pbeRenderScreen(currentTick, lastTick);
    
    // 5. Swap buffers and display
    g_PBEngine.gfxSwap();
}
```

### Pinball_Engine - Core Engine

The `PBEngine` class manages the entire game state machine:

#### Main State Machine (`PBMainState`)
- `PB_BOOTUP`: System boot and self-test
- `PB_STARTMENU`: Main menu
- `PB_PLAYGAME`: **Active pinball gameplay** ← Your code runs here!
- `PB_TESTMODE`: Hardware test mode
- `PB_BENCHMARK`: Performance benchmark
- `PB_CREDITS`: Credits screen
- `PB_SETTINGS`: Settings menu
- `PB_DIAGNOSTICS`: Diagnostic tools

When `PB_PLAYGAME` is active, the engine delegates to your table-specific code.

### Pinball_Table - Your Custom Code

This is where you implement your pinball machine!

#### Key Components You Define

1. **Table States** (`PBTableState` enum)
   - Define the phases of your game (start screen, multiball, bonus, etc.)

2. **Screen States** (`PBTBLScreenState` enum)
   - Define sub-states within table states (instructions, scores, attract mode)

3. **Render Functions**
   - `pbeRenderGameScreen()`: Main render coordinator
   - Custom render functions for each table state

4. **Update Functions**
   - `pbeUpdateGameState()`: Process input messages during gameplay
   - State transition logic

**Example Structure:**
```cpp
enum class PBTableState {
    PBTBL_START,        // Attract mode
    PBTBL_STDPLAY,      // Normal gameplay
    PBTBL_MULTIBALL,    // Multiball mode
    PBTBL_BONUS,        // Bonus calculation
    PBTBL_END
};

enum class PBTBLScreenState {
    START_START,        // "Press Start"
    START_INST,         // Instructions
    START_SCORES,       // High scores
    START_OPENDOOR,     // Door opening animation
    START_END
};
```

---

## Creating a New Pinball Machine

This section guides you through setting up a new pinball machine from scratch.

### Step 1: Define Your I/O Pins

Edit `Pinball_IO.h` and `Pinball_IO.cpp` to define your inputs and outputs.

#### Input Definition Structure

```cpp
struct stInputDef {
    std::string inputName;      // Descriptive name
    std::string simMapKey;      // Keyboard key for Windows simulation
    PBInputMsg inputMsg;        // Message type
    unsigned int id;            // Unique ID
    unsigned int pin;           // Pin number on chip or GPIO
    PBBoardType boardType;      // PB_RASPI, PB_IO, PB_LED
    unsigned int boardIndex;    // Which chip (0, 1, 2, etc.)
    PBPinState lastState;       // Internal state (set to PB_OFF)
    unsigned long lastStateTick;// Internal timing (set to 0)
    unsigned long debounceTimeMS; // Debounce time in milliseconds
    bool autoOutput;            // Auto-fire output on trigger?
    unsigned int autoOutputId;  // Output ID to auto-fire
    PBPinState autoPinState;    // State to set output
};
```

#### Example Input Definitions

```cpp
// Define your input IDs
#define IDI_LEFTFLIPPER     0
#define IDI_RIGHTFLIPPER    1
#define IDI_START           2
#define IDI_BALL_SENSOR     3
#define NUM_INPUTS          4

// Define the input array
stInputDef g_inputDef[] = {
    // Left Flipper: GPIO 27, simulated with 'A' key
    {"Left Flipper", "A", PB_IMSG_BUTTON, IDI_LEFTFLIPPER, 
     27, PB_RASPI, 0, PB_OFF, 0, 5, 
     true, IDO_LEFT_FLIPPER_LED, PB_ON},
    
    // Right Flipper: GPIO 17, simulated with 'D' key
    {"Right Flipper", "D", PB_IMSG_BUTTON, IDI_RIGHTFLIPPER, 
     17, PB_RASPI, 0, PB_OFF, 0, 5, 
     true, IDO_RIGHT_FLIPPER_LED, PB_ON},
    
    // Start Button: IO Chip 0, Pin 0, simulated with 'Z' key
    {"Start", "Z", PB_IMSG_BUTTON, IDI_START, 
     0, PB_IO, 0, PB_OFF, 0, 5, 
     false, 0, PB_OFF},
    
    // Ball Sensor: IO Chip 0, Pin 5, simulated with '1' key
    {"Ball Sensor", "1", PB_IMSG_SENSOR, IDI_BALL_SENSOR, 
     5, PB_IO, 0, PB_OFF, 0, 10, 
     false, 0, PB_OFF}
};
```

#### Output Definition Structure

```cpp
struct stOutputDef {
    std::string outputName;     // Descriptive name
    PBOutputMsg outputMsg;      // Message type
    unsigned int id;            // Unique ID
    unsigned int pin;           // Pin number on chip or GPIO
    PBBoardType boardType;      // PB_RASPI, PB_IO, PB_LED
    unsigned int boardIndex;    // Which chip (0, 1, 2, etc.)
    PBPinState lastState;       // Internal state (set to PB_OFF)
    unsigned int onTimeMS;      // Pulse on time (0 = no pulse)
    unsigned int offTimeMS;     // Pulse off time (0 = no pulse)
};
```

#### Example Output Definitions

```cpp
// Define your output IDs
#define IDO_LEFT_FLIPPER_LED    0
#define IDO_RIGHT_FLIPPER_LED   1
#define IDO_SLINGSHOT           2
#define IDO_POPBUMPER           3
#define IDO_BALL_EJECT          4
#define IDO_SCORE_LED           5
#define NUM_OUTPUTS             6

// Define the output array
stOutputDef g_outputDef[] = {
    // Left Flipper LED: LED Chip 0, Pin 0
    {"Left Flipper LED", PB_OMSG_LED, IDO_LEFT_FLIPPER_LED,
     0, PB_LED, 0, PB_OFF, 0, 0},
    
    // Right Flipper LED: LED Chip 0, Pin 1
    {"Right Flipper LED", PB_OMSG_LED, IDO_RIGHT_FLIPPER_LED,
     1, PB_LED, 0, PB_OFF, 0, 0},
    
    // Slingshot Solenoid: IO Chip 0, Pin 8, 500ms pulse
    {"Slingshot", PB_OMSG_GENERIC_IO, IDO_SLINGSHOT,
     8, PB_IO, 0, PB_OFF, 500, 0},
    
    // Pop Bumper Solenoid: IO Chip 1, Pin 8, 1000ms pulse
    {"Pop Bumper", PB_OMSG_GENERIC_IO, IDO_POPBUMPER,
     8, PB_IO, 1, PB_OFF, 1000, 0},
    
    // Ball Eject: IO Chip 2, Pin 8, 2000ms pulse
    {"Ball Eject", PB_OMSG_GENERIC_IO, IDO_BALL_EJECT,
     8, PB_IO, 2, PB_OFF, 2000, 0},
    
    // Score LED: LED Chip 1, Pin 10
    {"Score LED", PB_OMSG_LED, IDO_SCORE_LED,
     10, PB_LED, 1, PB_OFF, 0, 0}
};
```

#### Pin Configuration Notes

**boardType Options:**
- `PB_RASPI`: Direct Raspberry Pi GPIO (use WiringPi pin numbers)
- `PB_IO`: TCA9555 I/O expander chip
- `PB_LED`: TLC59116 LED driver chip

**boardIndex:**
- For `PB_IO`: 0 = address 0x20, 1 = address 0x21, 2 = address 0x22
- For `PB_LED`: 0 = address 0x60, 1 = address 0x61, 2 = address 0x62
- For `PB_RASPI`: Always 0 (ignored)

**Auto-Output Feature:**
When `autoOutput` is `true`, the framework automatically sends an output message when this input is triggered. This is useful for simple feedback like lighting an LED when a button is pressed.

### Step 2: Define Table States and Screens

Edit `Pinball_Table.h` to define your game states.

```cpp
// Main table states
enum class PBTableState {
    PBTBL_ATTRACT,      // Attract mode / waiting for start
    PBTBL_GAMEPLAY,     // Active gameplay
    PBTBL_MULTIBALL,    // Multiball mode
    PBTBL_BONUS,        // End-of-ball bonus
    PBTBL_GAMEOVER      // Game over screen
};

// Screen sub-states (used within attract mode, for example)
enum class PBTBLScreenState {
    ATTRACT_TITLE,      // Show title
    ATTRACT_INST,       // Show instructions
    ATTRACT_SCORES      // Show high scores
};
```

### Step 3: Implement State Update Logic

In `Pinball_Table.cpp`, implement `pbeUpdateGameState()` to handle input messages.

```cpp
void PBEngine::pbeUpdateGameState(stInputMessage inputMessage) {
    
    // Enable auto-outputs during gameplay
    SetAutoOutputEnable(true);
    
    switch (m_tableState) {
        case PBTableState::PBTBL_ATTRACT:
            // Handle attract mode inputs
            if (inputMessage.inputMsg == PB_IMSG_BUTTON && 
                inputMessage.inputId == IDI_START && 
                inputMessage.inputState == PB_ON) {
                // Start button pressed - start game!
                m_tableState = PBTableState::PBTBL_GAMEPLAY;
                m_RestartTable = true;
                
                // Play start sound
                m_soundSystem.PlayEffect(EFFECTCLICK);
                
                // Turn on start LED
                SendOutputMsg(PB_OMSG_LED, IDO_START_LED, PB_ON, false);
            }
            break;
            
        case PBTableState::PBTBL_GAMEPLAY:
            // Handle gameplay inputs
            if (inputMessage.inputMsg == PB_IMSG_SENSOR && 
                inputMessage.inputId == IDI_BALL_SENSOR && 
                inputMessage.inputState == PB_ON) {
                // Ball detected - add score and flash LED
                m_currentScore += 100;
                
                // Flash the score LED
                stOutputOptions options;
                options.onBlinkMS = 100;
                options.offBlinkMS = 100;
                SendOutputMsg(PB_OMSG_LED, IDO_SCORE_LED, PB_BLINK, 
                              true, &options);
            }
            
            // Check for multiball trigger
            if (inputMessage.inputMsg == PB_IMSG_TARGET && 
                inputMessage.inputId == IDI_MULTIBALL_TARGET && 
                inputMessage.inputState == PB_ON) {
                m_tableState = PBTableState::PBTBL_MULTIBALL;
                m_RestartTable = true;
            }
            break;
            
        case PBTableState::PBTBL_MULTIBALL:
            // Multiball logic...
            break;
    }
}
```

### Step 4: Implement Render Functions

Implement rendering for each of your table states.

```cpp
bool PBEngine::pbeRenderGameScreen(unsigned long currentTick, unsigned long lastTick) {
    
    switch (m_tableState) {
        case PBTableState::PBTBL_ATTRACT:
            return pbeRenderAttractMode(currentTick, lastTick);
            
        case PBTableState::PBTBL_GAMEPLAY:
            return pbeRenderGameplay(currentTick, lastTick);
            
        case PBTableState::PBTBL_MULTIBALL:
            return pbeRenderMultiball(currentTick, lastTick);
            
        default:
            return false;
    }
}

bool PBEngine::pbeRenderAttractMode(unsigned long currentTick, unsigned long lastTick) {
    // Load resources if needed
    if (!pbeLoadAttractMode(false)) return false;
    
    // Clear screen
    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
    
    // Render backglass
    gfxRenderSprite(m_BackglassId, 0, 0);
    
    // Render "Press Start" text with blinking
    if (m_blinkState) {
        gfxRenderShadowString(m_FontId, "Press Start", 
                              PB_SCREENWIDTH/2, 500, 1, 
                              GFX_TEXTCENTER, 
                              0, 0, 0, 255, 3);
    }
    
    return true;
}
```

### Step 5: Create Loading Functions

Create functions to load sprites and resources for each state.

```cpp
bool PBEngine::pbeLoadAttractMode(bool forceReload) {
    static bool loaded = false;
    if (forceReload) loaded = false;
    if (loaded) return true;
    
    // Load backglass texture
    m_BackglassId = gfxLoadSprite("Backglass", 
                                  "src/resources/textures/MyBackglass.png",
                                  GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, 
                                  true, true);
    
    // Load font
    m_FontId = gfxLoadSprite("Font", 
                             "src/resources/fonts/MyFont_60_512.png",
                             GFX_PNG, GFX_TEXTMAP, GFX_UPPERLEFT, 
                             true, true);
    
    if (m_BackglassId == NOSPRITE || m_FontId == NOSPRITE) {
        return false;
    }
    
    loaded = true;
    return true;
}
```

---

## Graphics and Screen Management

### Sprite System Overview

The PInball framework uses a sprite-based rendering system built on OpenGL ES.

#### Sprite Loading

```cpp
unsigned int spriteId = gfxLoadSprite(
    const std::string& spriteName,      // Name for reference
    const std::string& textureFileName, // Path to image file
    gfxTexType textureType,             // GFX_PNG, GFX_BMP, GFX_NONE
    gfxSpriteMap mapType,               // GFX_NOMAP, GFX_TEXTMAP, GFX_SPRITEMAP
    gfxTexCenter textureCenter,         // GFX_UPPERLEFT, GFX_CENTER
    bool keepResident,                  // Keep in memory?
    bool useTexture                     // Use texture or solid color?
);
```

**Texture Types:**
- `GFX_PNG`: PNG image file
- `GFX_BMP`: BMP image file
- `GFX_NONE`: No texture (solid color quad)

**Map Types:**
- `GFX_NOMAP`: Standard sprite
- `GFX_TEXTMAP`: Font sprite with character mapping
- `GFX_SPRITEMAP`: Sprite sheet with multiple sub-sprites

**Center Types:**
- `GFX_UPPERLEFT`: X,Y coordinates are upper-left corner
- `GFX_CENTER`: X,Y coordinates are sprite center

#### Sprite Rendering

```cpp
// Simple render at current position
gfxRenderSprite(spriteId);

// Render at specific position
gfxRenderSprite(spriteId, x, y);

// Render with scale and rotation
gfxRenderSprite(spriteId, x, y, scaleFactor, rotateDegrees);
```

#### Sprite Manipulation

```cpp
// Set position
gfxSetXY(spriteId, x, y, addToExisting);

// Set color (RGBA, 0-255)
gfxSetColor(spriteId, red, green, blue, alpha);

// Set texture alpha (0.0 - 1.0)
gfxSetTextureAlpha(spriteId, alpha);

// Set scale factor (1.0 = 100%)
gfxSetScaleFactor(spriteId, scaleFactor, addToExisting);

// Set rotation (degrees)
gfxSetRotateDegrees(spriteId, degrees, addToExisting);

// Set UV coordinates for texture mapping
gfxSetUV(spriteId, u1, v1, u2, v2);

// Set width and height
gfxSetWH(spriteId, width, height);
```

### Text Rendering

Fonts are sprites with a JSON mapping file that defines character positions.

```cpp
// Render text string
gfxRenderString(fontSpriteId, "Hello World", x, y, 
                spacingPixels, GFX_TEXTLEFT);

// Render text with shadow
gfxRenderShadowString(fontSpriteId, "Score: 1000", x, y,
                      spacingPixels, GFX_TEXTCENTER,
                      shadowRed, shadowGreen, shadowBlue, 
                      shadowAlpha, shadowOffset);
```

**Text Justification:**
- `GFX_TEXTLEFT`: Left-aligned
- `GFX_TEXTCENTER`: Center-aligned
- `GFX_TEXTRIGHT`: Right-aligned

**Get text dimensions:**
```cpp
int width = gfxStringWidth(fontSpriteId, "Hello", spacingPixels);
unsigned int height = gfxGetTextHeight(fontSpriteId);
```

### Sprite Animation

The framework includes an automatic animation system.

```cpp
// Create animation data structure
stAnimateData animData;
gfxLoadAnimateData(&animData,
    animateSpriteId,     // Sprite to animate
    startSpriteId,       // Starting state
    endSpriteId,         // Ending state
    startTick,           // Start time
    ANIMATE_X_MASK | ANIMATE_Y_MASK,  // What to animate
    2.0f,                // Duration in seconds
    0.0f,                // Acceleration
    true,                // Active immediately?
    true,                // Rotate clockwise?
    GFX_NOLOOP           // Loop mode
);

// Create the animation
gfxCreateAnimation(animData, replaceExisting);

// Animate the sprite (call each frame)
gfxAnimateSprite(spriteId, currentTick);

// Check if animation is active
if (gfxAnimateActive(spriteId)) {
    // Animation still running...
}

// Restart animation
gfxAnimateRestart(spriteId, startTick);
```

**Animation Masks:**
- `ANIMATE_X_MASK`: Animate X position
- `ANIMATE_Y_MASK`: Animate Y position
- `ANIMATE_U_MASK`: Animate U texture coordinate
- `ANIMATE_V_MASK`: Animate V texture coordinate
- `ANIMATE_TEXALPHA_MASK`: Animate texture alpha
- `ANIMATE_COLOR_MASK`: Animate color (RGBA)
- `ANIMATE_SCALE_MASK`: Animate scale
- `ANIMATE_ROTATE_MASK`: Animate rotation
- `ANIMATE_ALL_MASK`: Animate everything

**Loop Modes:**
- `GFX_NOLOOP`: Play once and stop
- `GFX_RESTART`: Loop by restarting
- `GFX_REVERSE`: Play forward then backward once

### Instance Sprites

Create multiple instances of the same sprite with different properties.

```cpp
// Create instance
unsigned int instanceId = gfxInstanceSprite(parentSpriteId);

// Instances share texture but have independent properties
gfxSetXY(instanceId, 100, 200, false);
gfxSetColor(instanceId, 255, 0, 0, 255);  // Red tint
gfxSetScaleFactor(instanceId, 2.0, false);
```

This is useful for:
- Multiple enemies with the same sprite
- Particle effects
- Repeated decorative elements

---

## LED Control and Sequences

### Basic LED Control

#### Turn LED On/Off

```cpp
// Turn LED on
SendOutputMsg(PB_OMSG_LED, IDO_MY_LED, PB_ON, false);

// Turn LED off
SendOutputMsg(PB_OMSG_LED, IDO_MY_LED, PB_OFF, false);
```

#### LED Blinking

```cpp
// Create options for blinking
stOutputOptions options;
options.onBlinkMS = 200;   // On for 200ms
options.offBlinkMS = 200;  // Off for 200ms

// Send blink command
SendOutputMsg(PB_OMSG_LED, IDO_MY_LED, PB_BLINK, false, &options);
```

#### LED Brightness

```cpp
// Set brightness (0-255)
stOutputOptions options;
options.brightness = 128;  // 50% brightness

SendOutputMsg(PB_OMSG_LEDSET_BRIGHTNESS, IDO_MY_LED, 
              PB_BRIGHTNESS, false, &options);
```

### RGB LED Control

RGB LEDs use three output pins (red, green, blue).

```cpp
// Turn LED red
SendRGBMsg(IDO_LED_RED, IDO_LED_GREEN, IDO_LED_BLUE,
           PB_LEDRED, PB_ON, false);

// Turn LED purple (red + blue)
SendRGBMsg(IDO_LED_RED, IDO_LED_GREEN, IDO_LED_BLUE,
           PB_LEDPURPLE, PB_ON, false);

// Turn LED white
SendRGBMsg(IDO_LED_RED, IDO_LED_GREEN, IDO_LED_BLUE,
           PB_LEDWHITE, PB_ON, false);
```

**RGB Color Constants:**
- `PB_LEDRED`: Red only
- `PB_LEDGREEN`: Green only
- `PB_LEDBLUE`: Blue only
- `PB_LEDWHITE`: Red + Green + Blue
- `PB_LEDPURPLE`: Red + Blue (Magenta)
- `PB_LEDYELLOW`: Red + Green
- `PB_LEDCYAN`: Green + Blue

### LED Sequences

LED sequences create complex animations across multiple LEDs.

#### Defining a Sequence

In `PBSequences.cpp`:

```cpp
// Define sequence steps
static const stLEDSequence MyChase_Steps[] = {
    {{0x0001, 0x0000, 0x0000}, 100, 0},  // Chip 0, LED 0 on for 100ms
    {{0x0002, 0x0000, 0x0000}, 100, 0},  // Chip 0, LED 1 on for 100ms
    {{0x0004, 0x0000, 0x0000}, 100, 0},  // Chip 0, LED 2 on for 100ms
    {{0x0008, 0x0000, 0x0000}, 100, 0},  // Chip 0, LED 3 on for 100ms
};

// Create sequence data structure
const LEDSequence MyChase = {
    MyChase_Steps,
    sizeof(MyChase_Steps) / sizeof(MyChase_Steps[0])
};

// Define which LEDs the sequence can control
const uint16_t MyChase_Mask[NUM_LED_CHIPS] = {
    0xFFFF,  // Chip 0: all LEDs
    0x0000,  // Chip 1: no LEDs
    0x0000   // Chip 2: no LEDs
};
```

In `PBSequences.h`:

```cpp
extern const LEDSequence MyChase;
extern const uint16_t MyChase_Mask[NUM_LED_CHIPS];
```

#### Understanding Sequence Data

```cpp
struct stLEDSequence {
    uint16_t LEDOnBits[NUM_LED_CHIPS];  // Which LEDs are on (bit mask)
    unsigned int onDurationMS;           // How long this step lasts
    unsigned int offDurationMS;          // Gap before next step
};
```

**LED Bit Masks:**
- Each LED chip has 16 LEDs (bits 0-15)
- Bit 0 = LED 0, Bit 1 = LED 1, etc.
- Use hex notation: `0x0001` = LED 0, `0x00FF` = LEDs 0-7, `0xFFFF` = all 16

**Examples:**
```cpp
0x0001  // Only LED 0
0x000F  // LEDs 0-3
0x00FF  // LEDs 0-7
0xFF00  // LEDs 8-15
0xFFFF  // All 16 LEDs
0x5555  // Every other LED (0, 2, 4, 6, 8, 10, 12, 14)
```

#### Playing a Sequence

```cpp
// Play sequence with mask and loop mode
SendSeqMsg(&MyChase,           // Sequence to play
           MyChase_Mask,        // LED mask
           PB_LOOP,             // Loop mode
           PB_ON);              // Start playing
```

**Loop Modes:**
- `PB_NOLOOP`: Play once and stop
- `PB_LOOP`: Loop continuously
- `PB_PINGPONG`: Play forward, then backward, then stop
- `PB_PINGPONGLOOP`: Ping-pong continuously

#### Stopping a Sequence

```cpp
// Stop the active sequence
SendSeqMsg(nullptr, nullptr, PB_NOLOOP, PB_OFF);
```

### Group Dimming and Blinking

Control multiple LEDs together as a group.

```cpp
// Group dimming (all LEDs at same brightness)
stOutputOptions options;
options.brightness = 64;  // 25% brightness

SendOutputMsg(PB_OMSG_LEDCFG_GROUPDIM, outputId, 
              PB_ON, false, &options);

// Group blinking (all LEDs blink together)
options.onBlinkMS = 500;
options.offBlinkMS = 500;

SendOutputMsg(PB_OMSG_LEDCFG_GROUPBLINK, outputId, 
              PB_ON, false, &options);
```

---

## Function Reference

### Output Message Functions

#### SendOutputMsg
```cpp
void SendOutputMsg(PBOutputMsg outputMsg, 
                   unsigned int outputId, 
                   PBPinState outputState, 
                   bool usePulse, 
                   stOutputOptions* options = nullptr);
```

**Purpose:** Send an output message to control LEDs, solenoids, or other outputs.

**Parameters:**
- `outputMsg`: Type of output message (`PB_OMSG_LED`, `PB_OMSG_GENERIC_IO`, etc.)
- `outputId`: ID of the output (defined in `Pinball_IO.h`)
- `outputState`: Desired state (`PB_ON`, `PB_OFF`, `PB_BLINK`, `PB_BRIGHTNESS`)
- `usePulse`: If `true`, use pulse timing from output definition
- `options`: Optional parameters (brightness, blink timing, etc.)

**Example:**
```cpp
// Flash slingshot solenoid
SendOutputMsg(PB_OMSG_GENERIC_IO, IDO_SLINGSHOT, PB_ON, true);

// Turn on LED
SendOutputMsg(PB_OMSG_LED, IDO_SCORE_LED, PB_ON, false);
```

#### SendRGBMsg
```cpp
void SendRGBMsg(unsigned int redId, 
                unsigned int greenId, 
                unsigned int blueId, 
                PBLEDColor color, 
                PBPinState outputState, 
                bool usePulse, 
                stOutputOptions* options = nullptr);
```

**Purpose:** Control an RGB LED with a single color command.

**Parameters:**
- `redId`, `greenId`, `blueId`: Output IDs for R, G, B channels
- `color`: Desired color (`PB_LEDRED`, `PB_LEDGREEN`, `PB_LEDWHITE`, etc.)
- `outputState`: State to set
- `usePulse`: Use pulse timing
- `options`: Optional parameters

**Example:**
```cpp
// Turn RGB LED yellow
SendRGBMsg(IDO_RGB1_RED, IDO_RGB1_GREEN, IDO_RGB1_BLUE,
           PB_LEDYELLOW, PB_ON, false);
```

#### SendSeqMsg
```cpp
void SendSeqMsg(const LEDSequence* sequence, 
                const uint16_t* mask, 
                PBSequenceLoopMode loopMode, 
                PBPinState outputState);
```

**Purpose:** Start or stop an LED sequence animation.

**Parameters:**
- `sequence`: Pointer to sequence data (or `nullptr` to stop)
- `mask`: Array of masks for each LED chip (or `nullptr` to stop)
- `loopMode`: How to loop (`PB_NOLOOP`, `PB_LOOP`, `PB_PINGPONG`, etc.)
- `outputState`: `PB_ON` to start, `PB_OFF` to stop

**Example:**
```cpp
// Start chase sequence with looping
SendSeqMsg(&PBSeq_ChasePattern, PBSeq_ChasePattern_Mask, 
           PB_LOOP, PB_ON);

// Stop sequence
SendSeqMsg(nullptr, nullptr, PB_NOLOOP, PB_OFF);
```

### Graphics Functions

#### gfxLoadSprite
```cpp
unsigned int gfxLoadSprite(const std::string& spriteName,
                           const std::string& textureFileName,
                           gfxTexType textureType,
                           gfxSpriteMap mapType,
                           gfxTexCenter textureCenter,
                           bool keepResident,
                           bool useTexture);
```

**Purpose:** Load a sprite from a texture file.

**Returns:** Sprite ID (or `NOSPRITE` if failed)

**Example:**
```cpp
unsigned int bgId = gfxLoadSprite("Background",
                                  "src/resources/textures/bg.png",
                                  GFX_PNG, GFX_NOMAP, 
                                  GFX_UPPERLEFT, true, true);
```

#### gfxRenderSprite
```cpp
bool gfxRenderSprite(unsigned int spriteId);
bool gfxRenderSprite(unsigned int spriteId, int x, int y);
bool gfxRenderSprite(unsigned int spriteId, int x, int y, 
                     float scaleFactor, float rotateDegrees);
```

**Purpose:** Render a sprite to the screen.

**Returns:** `true` if successful

**Example:**
```cpp
// Render at stored position
gfxRenderSprite(spriteId);

// Render at specific position
gfxRenderSprite(spriteId, 100, 200);

// Render with transformations
gfxRenderSprite(spriteId, 100, 200, 2.0f, 45.0f);
```

#### gfxRenderString
```cpp
bool gfxRenderString(unsigned int fontSpriteId, 
                     std::string text, 
                     int x, int y,
                     unsigned int spacingPixels, 
                     gfxTextJustify justify);
```

**Purpose:** Render text using a font sprite.

**Parameters:**
- `fontSpriteId`: ID of font sprite (must be loaded with `GFX_TEXTMAP`)
- `text`: String to render
- `x`, `y`: Position
- `spacingPixels`: Extra spacing between characters
- `justify`: Text alignment (`GFX_TEXTLEFT`, `GFX_TEXTCENTER`, `GFX_TEXTRIGHT`)

**Example:**
```cpp
gfxRenderString(fontId, "Score: 1000", 100, 50, 
                2, GFX_TEXTLEFT);
```

#### gfxRenderShadowString
```cpp
bool gfxRenderShadowString(unsigned int fontSpriteId,
                          std::string text,
                          int x, int y,
                          unsigned int spacingPixels,
                          gfxTextJustify justify,
                          unsigned int shadowRed,
                          unsigned int shadowGreen,
                          unsigned int shadowBlue,
                          unsigned int shadowAlpha,
                          unsigned int shadowOffset);
```

**Purpose:** Render text with a drop shadow effect.

**Example:**
```cpp
// White text with black shadow
gfxSetColor(fontId, 255, 255, 255, 255);
gfxRenderShadowString(fontId, "Game Over", 
                      PB_SCREENWIDTH/2, 500, 1,
                      GFX_TEXTCENTER,
                      0, 0, 0, 255, 4);
```

#### gfxSetXY, gfxSetColor, gfxSetScaleFactor, gfxSetRotateDegrees
```cpp
unsigned int gfxSetXY(unsigned int spriteId, int x, int y, 
                      bool addToExisting);
unsigned int gfxSetColor(unsigned int spriteId, 
                         unsigned int red, unsigned int green,
                         unsigned int blue, unsigned int alpha);
unsigned int gfxSetScaleFactor(unsigned int spriteId, 
                               float scaleFactor, bool addToExisting);
unsigned int gfxSetRotateDegrees(unsigned int spriteId, 
                                 float degrees, bool addToExisting);
```

**Purpose:** Modify sprite properties.

**Parameters:**
- `addToExisting`: If `true`, add to current value; if `false`, replace

**Example:**
```cpp
// Set absolute position
gfxSetXY(spriteId, 100, 200, false);

// Move relative
gfxSetXY(spriteId, 10, 0, true);  // Move right 10 pixels

// Set color to semi-transparent red
gfxSetColor(spriteId, 255, 0, 0, 128);

// Double the size
gfxSetScaleFactor(spriteId, 2.0f, false);

// Rotate 45 degrees
gfxSetRotateDegrees(spriteId, 45.0f, false);
```

#### gfxCreateAnimation, gfxAnimateSprite
```cpp
bool gfxCreateAnimation(stAnimateData animateData, 
                       bool replaceExisting);
bool gfxAnimateSprite(unsigned int spriteId, 
                     unsigned long currentTick);
bool gfxAnimateActive(unsigned int spriteId);
```

**Purpose:** Create and run sprite animations.

**Example:**
```cpp
// Setup animation data
stAnimateData anim;
gfxLoadAnimateData(&anim, spriteId, startId, endId,
                   GetTickCountGfx(),
                   ANIMATE_X_MASK | ANIMATE_Y_MASK,
                   2.0f, 0.0f, true, true, GFX_NOLOOP);

// Create animation
gfxCreateAnimation(anim, true);

// In render loop:
gfxAnimateSprite(spriteId, currentTick);
gfxRenderSprite(spriteId);

// Check if done
if (!gfxAnimateActive(spriteId)) {
    // Animation finished
}
```

#### gfxClear, gfxSwap
```cpp
void gfxClear(float red, float green, float blue, float alpha, 
              bool doFlip);
void gfxSwap();
```

**Purpose:** Clear screen and swap buffers.

**Example:**
```cpp
// Clear to black
gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);

// Render everything...

// Display to screen
gfxSwap();
```

### State Management Functions

#### pbeUpdateState, pbeUpdateGameState
```cpp
void pbeUpdateState(stInputMessage inputMessage);
void pbeUpdateGameState(stInputMessage inputMessage);
```

**Purpose:** Process input messages and update game state.

**Note:** `pbeUpdateState` handles menu navigation. You implement `pbeUpdateGameState` for your table.

**Example:**
```cpp
void PBEngine::pbeUpdateGameState(stInputMessage inputMessage) {
    if (inputMessage.inputMsg == PB_IMSG_BUTTON &&
        inputMessage.inputState == PB_ON) {
        
        if (inputMessage.inputId == IDI_START) {
            // Handle start button
            m_tableState = PBTableState::PBTBL_GAMEPLAY;
        }
    }
}
```

#### pbeLoadGameScreen, pbeRenderGameScreen
```cpp
bool pbeLoadGameScreen(PBMainState state);
bool pbeRenderGameScreen(unsigned long currentTick, 
                        unsigned long lastTick);
```

**Purpose:** Load and render your table screens.

**Note:** You implement these for your specific table needs.

### Sound Functions

#### PlayMusic, PlayEffect
```cpp
bool PlayMusic(const std::string& filename, int loops = -1);
bool PlayEffect(const std::string& filename, int loops = 0);
void StopMusic();
void StopAllEffects();
```

**Purpose:** Play background music and sound effects.

**Parameters:**
- `filename`: Path to audio file (MP3, WAV, OGG supported)
- `loops`: Number of times to loop (-1 = infinite, 0 = play once)

**Example:**
```cpp
// Start background music (loop forever)
m_soundSystem.PlayMusic("src/resources/sound/bgmusic.mp3", -1);

// Play bumper hit sound once
m_soundSystem.PlayEffect("src/resources/sound/bumper.mp3", 0);

// Stop music
m_soundSystem.StopMusic();
```

#### SetVolume
```cpp
void SetMusicVolume(unsigned int volumePercent);
void SetEffectsVolume(unsigned int volumePercent);
void SetMasterVolume(unsigned int volumePercent);
```

**Purpose:** Control volume levels (0-100%).

**Example:**
```cpp
m_soundSystem.SetMusicVolume(m_saveFileData.musicVolume);
m_soundSystem.SetEffectsVolume(50);
m_soundSystem.SetMasterVolume(m_saveFileData.mainVolume);
```

### Utility Functions

#### GetTickCountGfx
```cpp
unsigned long GetTickCountGfx();
```

**Purpose:** Get current system time in milliseconds.

**Returns:** Milliseconds since program start

**Example:**
```cpp
unsigned long currentTime = GetTickCountGfx();
unsigned long elapsed = currentTime - m_startTime;
```

#### SetAutoOutputEnable
```cpp
void SetAutoOutputEnable(bool enable);
bool GetAutoOutputEnable() const;
```

**Purpose:** Enable or disable automatic outputs (defined in input configuration).

**Example:**
```cpp
// Enable auto-outputs during gameplay
SetAutoOutputEnable(true);

// Disable during menus
SetAutoOutputEnable(false);
```

#### pbeSendConsole, pbeClearConsole
```cpp
void pbeSendConsole(std::string output);
void pbeClearConsole();
void pbeRenderConsole(unsigned int startingX, unsigned int startingY);
```

**Purpose:** Debug console for displaying text messages.

**Example:**
```cpp
pbeSendConsole("Loading resources...");
pbeSendConsole("Game started!");

// Render console during boot screen
pbeRenderConsole(10, 50);
```

---

## Best Practices and Tips

### Performance Tips

1. **Load Once, Render Many**
   - Load sprites during initialization, not every frame
   - Use the `keepResident` flag for frequently used textures

2. **Use Instance Sprites**
   - Multiple instances share one texture = less memory
   - Perfect for repeated elements (stars, particles, etc.)

3. **Unload Unused Textures**
   - Call `gfxUnloadTexture()` when switching game modes
   - Free up GPU memory for new assets

4. **Limit Animation Complexity**
   - Don't animate everything at once
   - Use LED sequences instead of individual LED commands

### Debugging Tips

1. **Use the Console**
   ```cpp
   pbeSendConsole("Debug: Score = " + std::to_string(score));
   ```

2. **Test Mode**
   - Access from main menu to test inputs/outputs individually
   - Verify all hardware is working before debugging software

3. **Windows Simulation**
   - Develop and test on Windows first (faster iteration)
   - Use keyboard keys defined in `simMapKey` field

4. **Diagnostics Menu**
   - Enable FPS display to monitor performance
   - Use overlay mode to see debug information

### Code Organization Tips

1. **Separate States Clearly**
   - One render function per table state
   - One load function per table state
   - Use `m_Restart` flags to reset state variables

2. **Use Constants**
   ```cpp
   #define ACTIVEDISPX 448
   #define ACTIVEDISPY 268
   #define MULTIBALL_SCORE_BONUS 5000
   ```

3. **Comment Your Sequences**
   ```cpp
   // Rainbow chase: cycles through all colors
   static const stLEDSequence Rainbow_Steps[] = {
       {{0x0001, 0x0000, 0x0000}, 200, 0},  // Red
       {{0x0002, 0x0000, 0x0000}, 200, 0},  // Green
       // ...
   };
   ```

### Common Patterns

#### State Restart Pattern
```cpp
bool PBEngine::pbeRenderMyState(unsigned long currentTick, 
                                unsigned long lastTick) {
    if (m_RestartMyState) {
        // Reset all variables
        m_counter = 0;
        m_timer = 5000;
        m_RestartMyState = false;
        
        // Restart animations
        gfxAnimateRestart(m_animId, currentTick);
    }
    
    // Render logic...
}
```

#### Timeout Pattern
```cpp
// In restart block:
m_timeoutMS = 10000;  // 10 second timeout

// In render loop:
if (m_timeoutMS > 0) {
    m_timeoutMS -= (currentTick - lastTick);
    if (m_timeoutMS <= 0) {
        // Timeout occurred!
        m_tableScreenState = PBTBL_ATTRACT;
    }
}
```

#### Blink Pattern
```cpp
// In restart block:
m_blinkInterval = 500;  // Blink every 500ms
m_blinkOn = true;

// In render loop:
m_blinkInterval -= (currentTick - lastTick);
if (m_blinkInterval <= 0) {
    m_blinkOn = !m_blinkOn;
    m_blinkInterval = 500;
}

if (m_blinkOn) {
    gfxRenderString(fontId, "Press Start", x, y, 1, GFX_TEXTCENTER);
}
```

---

## Appendix: Quick Reference

### Message Types Quick Reference

| Input Message | Description |
|---------------|-------------|
| `PB_IMSG_BUTTON` | Player buttons |
| `PB_IMSG_SENSOR` | Ball sensors |
| `PB_IMSG_TARGET` | Target switches |
| `PB_IMSG_JETBUMPER` | Jet bumpers |
| `PB_IMSG_POPBUMPER` | Pop bumpers |

| Output Message | Description |
|----------------|-------------|
| `PB_OMSG_LED` | LED control |
| `PB_OMSG_LEDSET_BRIGHTNESS` | Set LED brightness |
| `PB_OMSG_LEDCFG_GROUPDIM` | Group dimming |
| `PB_OMSG_LEDCFG_GROUPBLINK` | Group blinking |
| `PB_OMSG_LED_SEQUENCE` | LED sequence |
| `PB_OMSG_GENERIC_IO` | Generic output |

### Pin States

| State | Description |
|-------|-------------|
| `PB_ON` | Output on / Input pressed |
| `PB_OFF` | Output off / Input released |
| `PB_BLINK` | LED blinking mode |
| `PB_BRIGHTNESS` | LED brightness mode |

### Board Types

| Type | Description |
|------|-------------|
| `PB_RASPI` | Raspberry Pi GPIO |
| `PB_IO` | TCA9555 I/O expander |
| `PB_LED` | TLC59116 LED driver |

### File Checklist for New Machine

- [ ] `Pinball_IO.h`: Define input/output IDs
- [ ] `Pinball_IO.cpp`: Define `g_inputDef[]` and `g_outputDef[]` arrays
- [ ] `Pinball_Table.h`: Define `PBTableState` and screen state enums
- [ ] `Pinball_Table.cpp`: Implement `pbeUpdateGameState()` and render functions
- [ ] `PBSequences.h/.cpp`: Define custom LED sequences (optional)
- [ ] `Pinball_TableStr.h`: Define text strings (optional)
- [ ] `PinballMenus.h`: Update menu text (optional)
- [ ] `resources/textures/`: Add your texture files
- [ ] `resources/fonts/`: Add your font files
- [ ] `resources/sound/`: Add your sound files

---

## Conclusion

You now have a comprehensive understanding of the PInball framework! Remember:

1. **Start simple** - Get basic I/O working first
2. **Use the simulator** - Test on Windows before deploying to Pi
3. **Leverage animations** - The sprite animation system is powerful
4. **Think in states** - Clear state machines make debugging easier
5. **Experiment with LEDs** - Sequences create impressive effects

Happy pinball programming! 🎮

For more information, see:
- `ARCHITECTURE.md` - Technical architecture details
- `HowToBuild.md` - Build instructions
- `README.md` - Project overview
- `PBSound_README.md` - Sound system details

---

*This guide was created for the PInball framework v0.5*
*Copyright (c) 2025 Jeffrey D. Bock*

# RasPin Pinball Framework - Architecture Overview

## Introduction

The RasPin Pinball Framework is a complete software and hardware solution for building half-scale physical pinball machines using Raspberry Pi. This document provides a high-level overview of the framework's architecture, how components interact, and guides you to detailed documentation for each subsystem.

### Framework Goals

- **Hobby-Friendly**: Low cost, easy to source components, designed for intermediate-level programmers
- **Cross-Platform**: Develop on Windows, deploy to Raspberry Pi hardware
- **Flexible**: Message-based architecture allows custom game logic without modifying core engine
- **Complete**: Graphics, sound, I/O, LED control, and game state management in one framework
- **Efficient**: Optimized for Raspberry Pi performance with smart hardware management

---

### AI Usage Notes

Development of this framework and game would have take signficantly possible longer without the use of Microsoft Copliot AI tools (Claude Sonnet 4.5 in VSCode was particularly useful).  It was also incredibly useful enhancing art and creating the various sprites used thorughout the game since the author has limited art skills to begin with...

Agentic Code AI is highly recommended as tool to utilize the RaspPin framework.  While you *could* write all the code manually, it excels at setting up rendering screens, following instructions to get base screen rendering, sprites, text and animation functions all set up - so you don't have to worry about the specifics of using the framework immediately.  After initial setup, the code can be tweaked to get exactly what you want.

## System Architecture

### High-Level Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        User's Game Code                         │
│              (Pinball_Table.cpp - Your Custom Table)            │
│                                                                  │
│  • Game logic and rules                                         │
│  • Screen rendering                                             │
│  • Input handling                                               │
│  • Score tracking                                               │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                         PBEngine Class                          │
│                    (Core Game Engine)                           │
│                                                                  │
│  • State machine (Menu, Play, Test, etc.)                       │
│  • Message queue management                                     │
│  • Output control (SendOutputMsg, SendSeqMsg)                   │
│  • Save/Load system                                             │
└──┬──────────────┬─────────────────┬────────────────┬───────────┘
   │              │                 │                │
   ▼              ▼                 ▼                ▼
┌────────┐  ┌──────────┐  ┌──────────┐  ┌─────────────┐  ┌─────────────┐
│ PBGfx  │  │ PBSound  │  │PBVideo   │  │ I/O System  │  │ LED System  │
│        │  │          │  │Player    │  │             │  │             │
│Sprites │  │ Music    │  │          │  │ Input Msg   │  │ Sequences   │
│Anims   │  │ Effects  │  │FFmpeg    │  │ Output Msg  │  │ Patterns    │
│Text    │  │ Volume   │  │Sync      │  │ Debounce    │  │ Brightness  │
└────────┘  └──────────┘  └──────────┘  └─────────────┘  └─────────────┘
```

### Data Flow Architecture

```
Hardware Inputs → Debounce → Input Messages → Game Logic → Output Messages → Hardware
     │                            ▲                 │              │
     │                            │                 │              │
     └────────────────────────────┘                 └──────────────┘
           (Optional Auto-Output)                   (LED Sequences)
```

---

## Core Components

### 1. PBEngine - The Central Controller

**What It Does:**
- Manages overall game state (menu, gameplay, test modes)
- Coordinates between graphics, sound, and I/O subsystems
- Provides message queue system for inputs and outputs
- Handles save files and high scores

**Key Capabilities:**
```
Main States:
    Menu System → Test Mode → Gameplay → Settings
         ↓            ↓          ↓          ↓
    Navigation    Hardware    Custom     Config
                  Testing     Logic      Storage
```

**Documentation:** [PBEngine_API.md](PBEngine_API.md)

---

### 2. Graphics System (PBGfx)

**What It Does:**
- Sprite loading and rendering
- Text rendering with custom fonts
- Automatic sprite animations (position, scale, rotation, color)
- OpenGL ES 3.1 backend for hardware acceleration

**Rendering Pipeline:**
```
Load Sprite → Create Instances → Configure Properties → Animate → Render
     │              │                   │                │         │
   .png          Multiple            Color            Position   Display
   file          versions            Scale            Smooth     on screen
                                     Rotation         Interp.
```

**Key Features:**
- Texture atlases for efficient memory use
- Sprite instances allow multiple renderable versions
- Animation system interpolates between states automatically
- Font rendering from generated texture atlases

**Documentation:** [Game_Creation_API.md](Game_Creation_API.md)

---

### 3. Sound System (PBSound)

**What It Does:**
- Background music playback (looping)
- Sound effects (up to 4 simultaneous)
- Volume control (master and music independent)
- MP3/WAV file support

**Audio Architecture:**
```
┌─────────────┐
│ Music Loop  │ ───────────┐
└─────────────┘            │
                           ▼
┌─────────────┐      ┌──────────┐      ┌──────────┐
│ Effect 1-4  │ ───→ │  Mixer   │ ───→ │ Hardware │
└─────────────┘      └──────────┘      └──────────┘
                           ▲
┌─────────────┐            │
│ Volume Ctrl │ ───────────┘
└─────────────┘
```

**Documentation:** [Game_Creation_API.md](Game_Creation_API.md) (PBSound section)

---

### 4. Video Playback System (PBVideoPlayer)

**What It Does:**
- Full-motion video playback with synchronized audio
- FFmpeg-based decoding for broad format support
- Integration with PBGfx for texture rendering
- Integration with PBSound for audio playback
- Sprite-like transformations (scale, rotate, position, alpha)

**Video Playback Pipeline:**
```
┌─────────────┐
│ Video File  │ (MP4, AVI, WebM, etc.)
└──────┬──────┘
       │ FFmpeg Decode
       ▼
┌──────────────────────┐
│  Video Frames (RGBA) │───────────┐
└──────────────────────┘           │
                                   ▼
┌──────────────────────┐     ┌──────────┐
│  Audio Samples       │────→│  PBGfx   │
└──────────────────────┘     │ Texture  │
       │                     │ Update   │
       ▼                     └─────┬────┘
┌──────────────────────┐           │
│     PBSound          │           │ Render as Sprite
│  (Raspberry Pi)      │           ▼
└──────────────────────┘    ┌──────────┐
                            │  Screen  │
                            └──────────┘
```

**Key Features:**
- Plays videos like animated sprites
- Supports seeking, looping, speed control
- Automatic frame synchronization
- Standard sprite transformations work on video
- Audio playback (Raspberry Pi only)

**Use Cases:**
- Attract mode loops
- Tutorial videos
- Cutscenes and story elements
- Animated backgrounds
- Dynamic content displays

**Basic Usage:**
```cpp
// Initialize video player
PBVideoPlayer videoPlayer(&gfx, &sound);

// Load and play video
unsigned int sprite = videoPlayer.pbvpLoadVideo(
    "src/resources/videos/intro.mp4", 
    100, 100,  // x, y position
    false      // keepResident
);

videoPlayer.pbvpSetLooping(true);
videoPlayer.pbvpPlay();

// In render loop
videoPlayer.pbvpUpdate(currentTick);  // Decode & sync
videoPlayer.pbvpRender();             // Draw to screen
```

**Advanced Features:**
```cpp
// Transform video like any sprite
videoPlayer.pbvpSetScaleFactor(0.5f);   // Scale to 50%
videoPlayer.pbvpSetAlpha(0.8f);         // 80% opacity
videoPlayer.pbvpSetRotation(45.0f);     // Rotate 45°

// Playback control
videoPlayer.pbvpSetPlaybackSpeed(2.0f); // 2x speed
videoPlayer.pbvpSeekTo(30.0f);          // Jump to 30s

// Query video info
stVideoInfo info = videoPlayer.pbvpGetVideoInfo();
// width, height, fps, duration, hasAudio, hasVideo
```

**Supported Formats:**
- **Containers:** MP4, AVI, MOV, WebM, MKV
- **Video Codecs:** H.264, H.265, VP8, VP9, MPEG-4
- **Audio Codecs:** AAC, MP3, Vorbis, PCM
- **Recommended:** MP4 + H.264 + AAC for best compatibility

**Performance Considerations:**
- Raspberry Pi: Keep videos ≤720p for smooth playback
- CPU-intensive decoding (no HW acceleration yet)
- Each frame uses width × height × 4 bytes of memory
- Limit simultaneous videos

**Platform Differences:**
| Feature | Windows | Raspberry Pi |
|---------|---------|--------------|
| Video playback | ✅ Supported | ✅ Supported |
| Audio playback | ❌ Stubbed | ✅ Via SDL2 |
| Hardware decode | ❌ Not yet | ❌ Not yet |

**Documentation:** [Game_Creation_API.md](Game_Creation_API.md) (PBVideoPlayer section)

---

### 5. Input/Output System

**What It Does:**
- Message-based I/O decouples hardware from game logic
- Debounced inputs prevent spurious signals
- Pulse outputs for timed solenoid control
- Auto-output for instant feedback (flippers)

**I/O Message Flow:**
```
┌─────────────────┐
│ Physical Inputs │
│ (Switches, etc) │
└────────┬────────┘
         │ Read & Debounce
         ▼
┌─────────────────┐
│  Input Queue    │
└────────┬────────┘
         │ Process
         ▼
┌─────────────────┐     Auto-Output?     ┌─────────────────┐
│   Game Logic    │ ───────Yes──────────→│  Output Queue   │
└─────────────────┘                      └────────┬────────┘
         │                                        │
         └─────────Manual Output─────────────────┘
                                                  │
                                                  ▼
                                         ┌─────────────────┐
                                         │ Physical Outputs│
                                         │(Solenoids, etc) │
                                         └─────────────────┘
```

**Platform Differences:**
- **Windows:** Keyboard simulation, no physical outputs
- **Raspberry Pi:** GPIO + I2C I/O expanders, real hardware control

**Documentation:** [IO_Processing_API.md](IO_Processing_API.md)

---

### 6. LED Control System

**What It Does:**
- Individual LED on/off/brightness control
- Group operations (chip-wide dimming/blinking)
- Animated sequences with multiple loop modes
- Deferred message queue during sequences

**LED Sequence Flow:**
```
Define Sequence Steps ──→ Start Sequence ──→ Auto-Play Animation ──→ End
        │                      │                    │                  │
   LED patterns          Set loop mode        Update LEDs         Restore
   + timings             Select LEDs          each frame          state
```

**Loop Modes:**
- **No-Loop:** Play once and stop
- **Loop:** Continuous repeat from start
- **Ping-Pong:** Forward then backward, stop
- **Ping-Pong Loop:** Bounce back and forth continuously

**Documentation:** [LED_Control_API.md](LED_Control_API.md)

---

### 7. Device Management System (PBDevice)

**What It Does:**
- Base class for managing complex pinball device states and flows
- Structured framework for multi-step device operations
- Built-in state tracking, timing control, and error handling
- Integration with PBEngine for timing and output control

**Device Architecture:**
```
┌─────────────────────────────────────────────────┐
│              PBDevice Base Class                │
│                                                 │
│  • State management (m_state)                  │
│  • Timing control (m_startTimeMS)              │
│  • Enable/disable control (m_enabled)          │
│  • Run status tracking (m_running)             │
│  • Error handling (m_error)                    │
│  • Engine access (m_pEngine)                   │
└───────────────┬─────────────────────────────────┘
                │
                │ Derive Custom Devices
                │
    ┌───────────┴──────────┬──────────────┐
    ▼                      ▼              ▼
┌─────────┐          ┌──────────┐    ┌──────────┐
│Ejector  │          │  Scoop   │    │ Diverter │
│         │          │          │    │          │
│Ball     │          │Ball      │    │Ball      │
│Detection│          │Capture   │    │Routing   │
│LED Ctrl │          │Hold      │    │Direction │
│Solenoid │          │Release   │    │Control   │
└─────────┘          └──────────┘    └──────────┘
```

**When to Use PBDevice:**

Use for devices requiring:
- Multi-step operations (e.g., ball ejectors, scoops, diverters)
- Precise timing between states
- Multiple coordinated outputs (LEDs, solenoids, motors)
- State persistence across game ticks
- Error tracking and recovery

Simple devices (basic slingshots, bumpers) can use direct output messages.

**Base Class Functions:**
```cpp
// Virtual functions (override in derived classes)
virtual void pbdInit();           // Initialize/reset device
virtual void pbdEnable(bool);     // Enable/disable device
virtual void pdbStartRun();       // Start device operation
virtual void pbdExecute() = 0;    // Update device (pure virtual)

// Utility functions
bool pdbIsRunning() const;        // Check if operation in progress
bool pbdIsError() const;          // Check for error state
int pbdResetError();              // Get and clear error
void pbdSetState(unsigned int);   // Set current state
unsigned int pbdGetState() const; // Get current state
```

**Example: pbdEjector Class**

The framework includes a complete implementation of a ball ejector:

```cpp
class pbdEjector : public PBDevice {
    // Manages ball ejection with LED and solenoid control
    
    States:
        IDLE → BALL_DETECTED → SOLENOID_ON → SOLENOID_OFF
                                    ↓              ↓
                                    └──(retry)─────┘
                                    ↓
                                COMPLETE
};

// Usage in game code
pbdEjector* m_scoop = new pbdEjector(&g_PBEngine, 
                                    INPUT_SCOOP,
                                    LED_SCOOP,
                                    OUTPUT_SCOOP_EJECT);
m_scoop->pbdEnable(true);

// In game loop
m_scoop->pbdExecute();  // Call every frame

// Check completion
if (!m_scoop->pdbIsRunning()) {
    // Ejection complete, award points
}
```

**Device Flow Pattern:**
```
Initialize Device ──→ Enable ──→ Start Run ──→ Execute Loop
       │                             │              │
       └───────────────────────────┘              │
              (Reset/Cleanup)                      │
                                                   ▼
                                          ┌──────────────┐
                                          │ State Machine│
                                          │              │
                                          │ IDLE         │
                                          │   ↓          │
                                          │ ACTIVE       │
                                          │   ↓          │
                                          │ COMPLETE     │
                                          └──────────────┘
```

**Key Features:**
- **Timing Access:** Get current time via `m_pEngine->GetTickCountGfx()`
- **Output Control:** Send outputs via `m_pEngine->SendOutputMsg()`
- **State Persistence:** State maintained across frames automatically
- **Error Handling:** Built-in error flag and reporting
- **Extensibility:** Easy to create new device types

**Common Patterns:**
- **Retry Pattern:** Repeat operation if conditions not met
- **Timeout Pattern:** Error out after maximum time
- **Multi-Phase:** Sequential operations with timing between phases
- **Feedback Loop:** Check sensor state between actions

**Documentation:** [PBDevice_API.md](PBDevice_API.md)

---

## Platform Architecture

### Cross-Platform Design

```
┌──────────────────────────────────────────────────────────┐
│                    Common Code Layer                     │
│  (PBEngine, PBGfx, Game Logic - 95% of codebase)        │
└───────────────────┬──────────────────────────────────────┘
                    │
        ┌───────────┴──────────┐
        │                      │
        ▼                      ▼
┌──────────────┐      ┌──────────────┐
│   Windows    │      │ Raspberry Pi │
│              │      │              │
│ • Keyboard   │      │ • GPIO       │
│ • Simulation │      │ • I2C        │
│ • Fast Dev   │      │ • Real HW    │
└──────────────┘      └──────────────┘
```

### Build Switching

Platform selection via `PBBuildSwitch.h`:
```
Windows Build:
    #define EXE_MODE_WINDOWS
    
Raspberry Pi Build:
    #define EXE_MODE_RASPI
```

**Documentation:** [Platform_Init_API.md](Platform_Init_API.md)

---

## Hardware Architecture

### Physical Components

```
┌────────────────────────────────────────────────────────────┐
│                    Raspberry Pi 5                          │
│                  (Core Controller)                         │
└──┬──────────┬──────────┬──────────┬──────────┬───────────┘
   │          │          │          │          │
   ▼          ▼          ▼          ▼          ▼
┌──────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌──────┐
│ HDMI │ │ Audio  │ │  GPIO  │ │  I2C   │ │ USB  │
│Screen│ │  Out   │ │  Pins  │ │  Bus   │ │      │
└──────┘ └────────┘ └───┬────┘ └───┬────┘ └──────┘
                        │          │
                        ▼          ▼
                  ┌─────────┐ ┌──────────────────┐
                  │ Direct  │ │  I2C Expanders   │
                  │ Inputs  │ │                  │
                  └─────────┘ │ • IO (TCA9555)   │
                              │ • LED (TLC59116) │
                              └────────┬─────────┘
                                       │
                ┌──────────────────────┴────────────────────┐
                ▼                                           ▼
        ┌───────────────┐                          ┌───────────────┐
        │ Input Devices │                          │Output Devices │
        │               │                          │               │
        │ • Switches    │                          │ • Solenoids   │
        │ • Buttons     │                          │ • Motors      │
        │ • Sensors     │                          │ • LEDs        │
        └───────────────┘                          └───────────────┘
```

### I/O Chip Configuration

**IO Expanders (TCA9555):**
- 16 GPIO pins per chip (configurable input/output)
- I2C communication reduces Pi GPIO usage
- Debouncing handled in software
- Staged writes minimize bus traffic

**LED Drivers (TLC59116):**
- 16 LED outputs per chip with PWM
- Individual brightness control (256 levels)
- Group dimming and blinking modes
- Hardware handles LED timing

---

## Message Queue System

### Why Message Queues?

Traditional pinball programming tightly couples inputs to outputs:
```
If (flipper_button) then fire_flipper_solenoid
```

RasPin uses message queues for flexibility:
```
Input Detection → Input Message → Game Logic → Output Message → Hardware
```

**Benefits:**
- **Flexibility:** Change behavior without rewiring
- **Logging:** Easy to record and replay games
- **Auto-Output:** Optional direct routing for instant response
- **Testing:** Inject messages without hardware

### Message Flow Example

```
1. Player presses left flipper button
   ↓
2. Input system creates message:
   {inputMsg: LEFT_FLIPPER, state: ON, tick: 1234}
   ↓
3. Message placed in input queue
   ↓
4. Game code processes message OR auto-output triggers
   ↓
5. Output message created:
   {outputMsg: GENERIC_IO, id: FLIPPER_SOLENOID, state: ON}
   ↓
6. Message placed in output queue
   ↓
7. I/O system sends signal to hardware
```

---

## Animation System

### Sprite Animation Architecture

Instead of manual frame-by-frame updates, RasPin uses automatic interpolation:

```
Define States:
    Start State: Position X=100, Scale=1.0
    End State:   Position X=500, Scale=1.5
    
Configure Animation:
    Duration: 2 seconds
    Loop: Ping-Pong
    Properties: Position + Scale
    
Engine Handles:
    • Calculate intermediate values
    • Update each frame
    • Handle timing
    • Manage loops
```

**Animation Types:**
- Position (X, Y)
- Scale (size)
- Rotation (degrees)
- Color (RGBA)
- Texture coordinates (UV)

**Documentation:** [Game_Creation_API.md](Game_Creation_API.md) (Animation section)

---

## Font System

### Font Generation Pipeline

```
TrueType Font (.ttf)
        │
        ▼
    FontGen Utility
        │
        ├──→ Texture Atlas (.png)
        │    [All characters in one image]
        │
        └──→ UV Map (.json)
             [Character positions/sizes]
        
Load in Engine:
    gfxLoadSprite(..., GFX_TEXTMAP, ...)
        │
        ▼
    Engine automatically loads both files
        │
        ▼
    Ready for text rendering
```

**FontGen Workflow:**
```bash
# Generate 60-pixel font with 512x512 texture
FontGen MyFont.ttf 60 512

# Creates:
#   MyFont_60_512.png  (texture atlas)
#   MyFont_60_512.json (UV coordinates)

# Use in code:
m_fontId = gfxLoadSprite("Font", "MyFont_60_512.png", 
                         GFX_PNG, GFX_TEXTMAP, ...);
gfxRenderString(m_fontId, "HELLO", x, y, ...);
```

**Documentation:** [FontGen_Guide.md](FontGen_Guide.md)

---

## Game State Machine

### Main State Flow

```
┌──────────┐
│  BOOTUP  │ (Initialize systems)
└────┬─────┘
     │
     ▼
┌──────────┐
│START MENU│◄──────────────┐
└────┬─────┘               │
     │                     │
     ├──→ Test Mode ───────┤
     │                     │
     ├──→ Settings ────────┤
     │                     │
     ├──→ Diagnostics ─────┤
     │                     │
     └──→ PLAY GAME        │
          └───────────────┘
```

### Game Loop Structure

```
Initialize:
    • Load resources
    • Setup I/O
    • Load settings
    
Main Loop (60 FPS):
    1. Process I/O
       └─→ Read inputs
       └─→ Send outputs
    
    2. Process Input Messages
       └─→ Update game state
       └─→ Generate output messages
    
    3. Render Frame
       └─→ Clear screen
       └─→ Draw sprites
       └─→ Render text
       └─→ Swap buffers
    
    4. Frame Limiting
       └─→ Wait if needed
    
    Repeat ↑
```

---

## Screen Management Pattern

All game screens follow a consistent pattern:

### Load Function (Called Once)

```
Pseudo-code:

function LoadScreen(forceReload):
    if already_loaded and not forceReload:
        return success
    
    // Load sprites
    sprite_id = LoadSprite(texture_file)
    
    // Configure properties
    SetColor(sprite_id, color)
    SetScale(sprite_id, scale)
    
    // Create animations
    start_instance = CreateInstance(sprite_id)
    end_instance = CreateInstance(sprite_id)
    CreateAnimation(sprite_id, start, end, duration, loop)
    
    // Load sounds
    PlayBackgroundMusic(music_file)
    
    mark_as_loaded
    return success
```

### Render Function (Called Every Frame)

```
Pseudo-code:

function RenderScreen(currentTick, lastTick):
    // Ensure resources loaded
    if not LoadScreen():
        return error
    
    // Reset state if needed
    if restart_flag:
        reset_counters()
        restart_animations()
        restart_flag = false
    
    // Clear screen
    ClearScreen(background_color)
    
    // Render static elements
    RenderSprite(background_sprite)
    
    // Update and render animations
    AnimateSprite(animated_sprite, currentTick)
    RenderSprite(animated_sprite)
    
    // Render text
    SetColor(font, color)
    RenderString(font, "Text", position, justify)
    
    return success
```

---

## Development Workflow

### 1. Design Your Table

```
Plan gameplay:
    • Input mapping (buttons, switches)
    • Output mapping (solenoids, LEDs)
    • Game rules
    • Scoring system
```

### 2. Setup I/O Definitions

```
Edit Pinball_IO.cpp:

    Define inputs:
        {id, message_type, hardware_type, chip, pin}
    
    Define outputs:
        {id, message_type, hardware_type, chip, pin, pulse_times}
```

### 3. Create Game Screens

```
Add to Pinball_Table.cpp:

    Load function:
        • Load sprites and fonts
        • Create animations
        • Setup sound
    
    Render function:
        • Clear screen
        • Render sprites
        • Display text
```

### 4. Implement Game Logic

```
Update game state function:

    Process input messages:
        • Check game rules
        • Update score
        • Send output messages
        • Trigger LED sequences
```

### 5. Test and Iterate

```
Windows development:
    • Fast compilation
    • Keyboard simulation
    • Quick testing
    
Deploy to Raspberry Pi:
    • Test with real hardware
    • Tune pulse timings
    • Adjust LED brightness
```

---

## Documentation Guide

This PR includes comprehensive documentation for each subsystem:

### Getting Started
1. **[Platform_Init_API.md](Platform_Init_API.md)** - Start here to understand initialization and the main loop
2. **[PBEngine_API.md](PBEngine_API.md)** - Core engine functions and state management

### Graphics, Audio, and Video
3. **[Game_Creation_API.md](Game_Creation_API.md)** - Complete guide to sprites, animations, sound, and video playback
4. **[FontGen_Guide.md](FontGen_Guide.md)** - Creating custom fonts for text rendering

### Hardware Control
5. **[IO_Processing_API.md](IO_Processing_API.md)** - Input/output message system and hardware communication
6. **[LED_Control_API.md](LED_Control_API.md)** - LED control and animation sequences
7. **[PBDevice_API.md](PBDevice_API.md)** - Device management for complex pinball mechanisms

### Additional Resources
8. **[HowToBuild.md](HowToBuild.md)** - Build instructions for Windows and Raspberry Pi
9. **[UsersGuide.md](UsersGuide.md)** - Comprehensive framework guide (pre-existing)

---

## Architecture Principles

### Separation of Concerns

```
Game Logic ─┬─ Never directly manipulates hardware
            └─ Only sends/receives messages

I/O System ─┬─ Never knows about game rules
            └─ Only processes messages

Graphics   ─┬─ Never knows about I/O
            └─ Only renders what it's told
```

### Message-Based Communication

- **Decoupling:** Components don't directly call each other
- **Flexibility:** Easy to change behavior without rewiring
- **Testability:** Can inject messages for testing

### Hardware Abstraction

- **Platform Independence:** Same code runs on Windows and Pi
- **Staged Operations:** Changes batched to minimize I2C traffic
- **Efficient Timing:** Hardware handles pulse and PWM timing

### Resource Management

- **Keep Resident:** Frequently used sprites stay in memory
- **Lazy Loading:** Resources loaded only when needed
- **Unload on Demand:** Free memory for one-time screens

---

## Performance Considerations

### Graphics Optimization

```
Best Practices:
    • Batch sprite property changes before rendering
    • Use instances instead of duplicate sprites
    • Keep frequently used textures resident
    • Limit active animations
```

### I/O Optimization

```
Staged Writes:
    • Multiple output changes accumulated
    • Single I2C transaction sends all updates
    • Reduces bus traffic significantly
    
Debouncing:
    • Prevents spurious input messages
    • Configurable time window per input
    • Minimal CPU overhead
```

### LED Optimization

```
Sequence System:
    • Hardware handles PWM timing
    • Deferred messages during sequences
    • Group operations affect entire chip
```

---

## Summary

The RasPin Pinball Framework provides a complete, flexible foundation for building custom pinball machines. Its message-based architecture separates game logic from hardware control, making it easy to develop on Windows and deploy to Raspberry Pi hardware.

### Key Strengths

✓ **Easy to Learn:** Consistent patterns, clear documentation, practical examples  
✓ **Flexible:** Message queues decouple inputs from outputs  
✓ **Efficient:** Optimized for Raspberry Pi performance  
✓ **Complete:** Graphics, sound, I/O, LED control all integrated  
✓ **Cross-Platform:** Develop on Windows, deploy to Pi  

### Next Steps

1. Read **[Platform_Init_API.md](Platform_Init_API.md)** to understand the main loop
2. Review **[Game_Creation_API.md](Game_Creation_API.md)** for graphics and sound
3. Study **[IO_Processing_API.md](IO_Processing_API.md)** for hardware control
4. Explore example code in `Pinball_Table.cpp` and sandbox screen
5. Build your custom pinball table!

---

**Documentation in Framework:**
- PBEngine_API.md - Core engine class
- IO_Processing_API.md - Message-based I/O system  
- LED_Control_API.md - LED control and sequences
- PBDevice_API.md - Device management framework for complex mechanisms
- Platform_Init_API.md - Initialization and main loop
- Game_Creation_API.md - Graphics, sound, video playback, and screen management
- FontGen_Guide.md - Font generation utility

Each document contains detailed API references, code examples, and best practices based on actual framework usage.

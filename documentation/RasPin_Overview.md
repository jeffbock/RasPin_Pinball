# RasPin Pinball Framework - Architecture Overview

## Introduction

The RasPin Pinball Framework is a complete software and hardware solution for building half-scale physical pinball machines using Raspberry Pi. This document provides a high-level overview of the framework's architecture, how components interact, and guides you to detailed documentation for each subsystem.

### Framework Goals

- **Hobby-Friendly**: Low cost, easy to source components, designed for intermediate-level programmers
- **Cross-Platform**: Develop on Windows or Debian/Linux, deploy to Raspberry Pi hardware
- **Flexible**: Message-based architecture allows custom game logic without modifying core engine
- **Complete**: Graphics, sound, I/O, LED control, and game state management in one framework
- **Efficient**: Optimized for Raspberry Pi performance with smart hardware management

---

### AI Usage Notes

Development of this framework and game would have take signficantly possible longer without the use of Microsoft Copliot AI tools (Claude Sonnet 4.5 in VSCode was particularly useful).  It was also incredibly useful enhancing art and creating the various sprites used thorughout the game since the author has limited art skills to begin with...

Agentic Code AI is highly recommended as tool to utilize the RaspPin framework.  While you *could* write all the code manually, it excels at setting up rendering screens, following instructions to get base screen rendering, sprites, text and animation functions all set up - so you don't have to worry about the specifics of using the framework immediately.  After initial setup, the code can be tweaked to get exactly what you want.

---

## Project Directory Structure

```
PInball/
├── build/                     # All compiled outputs (git-ignored)
│   ├── windows/
│   │   ├── debug/             # Windows debug exe + required DLLs
│   │   └── release/           # Windows release exe + DLLs
│   ├── raspi/
│   │   ├── debug/             # Raspberry Pi debug binary + utilities
│   │   └── release/           # Raspberry Pi optimized release binary
│   └── debian/
│       ├── debug/             # Debian/Linux debug binary
│       └── release/           # Debian/Linux release binary
├── CMake/                     # CMakeLists.txt for Raspberry Pi/Debian builds
├── documentation/             # All framework documentation
├── scripts/                   # Build helper scripts
│   ├── copy_win_dlls.ps1      # Copies ANGLE + FFmpeg DLLs to Windows build dirs
│   └── generate_io_header.py  # Auto-generates io_defs_generated.h from io_definitions.json
├── src/
│   ├── system/                # Framework core — do NOT modify these files
│   │   ├── PBEngine.*         # Central game engine
│   │   ├── PBGfx.*            # Graphics / OpenGL ES 3.1
│   │   ├── PBSound.*          # Audio (SDL2_mixer)
│   │   ├── PBVideo.*          # Video playback (FFmpeg)
│   │   ├── Pinball_IO.*       # I/O hardware abstraction
│   │   ├── Pinball_Engine.*   # Engine entry points
│   │   └── ...
│   ├── user/                  # Your game-specific code — edit these files
│   │   ├── Pinball_Table.*    # Main table logic (Init/Start/Main/Reset/GameEnd/PlayerEnd)
│   │   ├── tablemodes/        # Per-mode game logic split into individual files
│   │   ├── PBDevice.*         # Custom device state machines
│   │   ├── PBSequences.*      # LED animation sequences
│   │   ├── PinballMenus.*     # Menu screens
│   │   ├── PBSequences.*      # NeoPixel / LED animation sequences
│   │   └── resources/         # Sprites, fonts, sounds, videos for your game
│   ├── 3rdparty/              # Third-party libraries (stb, cgltf, etc.)
│   ├── include_ogl_win/       # ANGLE OpenGL ES headers (Windows)
│   └── lib_ogl_win/           # ANGLE import libraries (Windows)
└── io_definitions.json        # I/O pin mapping — edit to match your hardware
```

---

## Files You Should Modify

The framework separates engine code from game code. Everything under `src/user/` is yours to customize; everything under `src/system/` is the framework engine.

### Primary Game Files

| File | Purpose |
|------|---------|
| `src/user/Pinball_Table.cpp/.h` | Your pinball table — game rules, scoring, input handling |
| `src/user/tablemodes/Pinball_Table_Mode*.cpp` | Per-mode logic: Init, Start, Main, Reset, GameEnd, PlayerEnd |
| `src/user/PinballMenus.cpp/.h` | Menu screen layouts and navigation logic |
| `src/user/PBSequences.cpp/.h` | LED sequence and NeoPixel animation definitions |
| `src/user/PBDevice.cpp/.h` | Custom physical device state machines (ejectors, hoppers, etc.) |

### Configuration

| File | Purpose |
|------|---------|
| `io_definitions.json` | Maps your physical I/O: input pins, outputs, LEDs, NeoPixel strips |
| `src/user/PBBuildSwitch.h` | Build-time flags: platform, hardware enable, window size |

### Resources

All game assets live under `src/user/resources/`:
- `fonts/` — TrueType fonts (run FontGen to generate the texture atlas/UV files)
- `images/` — Sprite sheets and background images (`.png`)
- `sounds/` — Music (`music/`) and sound effects (`sfx/`) (`.mp3`/`.wav`)
- `videos/` — Video clips for attract mode, cutscenes, etc.

### What to Leave Alone

Files under `src/system/`, `src/3rdparty/`, `src/include_ogl_win/`, and `src/lib_ogl_win/` are framework internals. Modify them only if you are extending the engine itself.

---

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
│  • Timer system (watchdog + user timers)                        │
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
│        │  │          │  │          │  │             │  │ NeoPixels   │
└────────┘  └──────────┘  └──────────┘  └─────────────┘  └─────────────┘
```

### Data Flow Architecture

```
Hardware Inputs → Debounce → Input Messages → Game Logic → Output Messages → Hardware
      │                            ▲                 │              │
      │                            │                 │              │
      └────────────────────────────┘                 └──────────────┘
            (Optional Auto-Output)                   (LED Sequences/NeoPixels)
                                   ▲                           
                                   │                           
                         ┌─────────┴──────────┐               
                         │                    │               
                  Timer Expirations    Device Processing      
                  (Delayed Events)     (State Machines)       
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
- Standard sprite transformations (scale, rotation, position, alpha)
- Audio playback (Raspberry Pi and Debian simulator)

**Use Cases:**
- Attract mode loops
- Tutorial videos
- Cutscenes and story elements
- Animated backgrounds
- Dynamic content displays

**Supported Formats:**
- **Containers:** MP4, AVI, MOV, WebM, MKV
- **Video Codecs:** H.264, H.265, VP8, VP9, MPEG-4
- **Audio Codecs:** AAC, MP3, Vorbis, PCM
- **Recommended:** MP4 + H.264 + AAC for best compatibility

**Performance Considerations:**
- Raspberry Pi: Keep videos ≤720p for smooth playback
- Software decoding only by default; hardware decode path (`h264_v4l2m2m`) exists but is disabled (`ENABLE_HW_VIDEO_DECODE=0` in `PBBuildSwitch.h`)
- Each frame uses width × height × 4 bytes of memory
- Limit simultaneous videos

**Platform Differences:**
| Feature | Windows | Raspberry Pi / Debian |
|---------|---------|----------------------|
| Video playback | ✅ Supported | ✅ Supported |
| Audio playback | ❌ Stubbed | ✅ Via SDL2 |
| Hardware decode | ❌ Not applicable | ❌ Disabled by default (`ENABLE_HW_VIDEO_DECODE=0`) |

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
- RGB addressable LED strip control (NeoPixels/WS2812B)

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

**NeoPixel RGB LEDs:**
- Full 24-bit RGB color control per LED
- WS2812B/SK6812 addressable LED strips
- Multiple timing methods (clock_gettime, NOP, SPI, PWM)
- Hardware-based SPI timing (recommended for production)
- Animation sequences with color patterns
- Brightness control and color presets

**Documentation:** 
- [LED_Control_API.md](LED_Control_API.md) - LED drivers and sequences
- [NeoPixel_Timing_Methods.md](NeoPixel_Timing_Methods.md) - Timing methods for reliable NeoPixel control
- [NeoPixel_Instrumentation.md](NeoPixel_Instrumentation.md) - Diagnostic tools for timing verification

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
- `pbdInit()` — Initialize/reset device state
- `pbdEnable(bool)` — Enable or disable the device
- `pdbStartRun()` — Start a device operation cycle
- `pbdExecute()` — Update device state machine (call every frame, pure virtual)
- `pdbIsRunning()` — Check if an operation is in progress
- `pbdIsError()` / `pbdResetError()` — Error state management
- `pbdSetState()` / `pbdGetState()` — Direct state control

**Example: pbdEjector Class**

The framework includes a complete ball ejector implementation that manages LED feedback, solenoid firing, and retry logic through a state machine: `IDLE → BALL_DETECTED → SOLENOID_ON → SOLENOID_OFF → COMPLETE`.

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

### 8. Timer System

**What It Does:**
- Delayed event generation for timed game mechanics
- Dedicated watchdog timer for critical timing
- Multiple concurrent user timers (up to 10)
- Automatic input message generation on expiration
- Integration with game state management

**Timer Architecture:**
```
┌─────────────────────────────────────────────────┐
│           Timer System Components               │
│                                                 │
│  ┌───────────────────────────────────┐         │
│  │    Watchdog Timer (ID=0)         │         │
│  │  • Dedicated storage             │         │
│  │  • Critical timing events        │         │
│  │  • Doesn't count against limit   │         │
│  └───────────────────────────────────┘         │
│                                                 │
│  ┌───────────────────────────────────┐         │
│  │    User Timer Queue (1-10 timers)│         │
│  │  • Custom timer IDs              │         │
│  │  • Game-specific events          │         │
│  │  • Optional repeat mode          │         │
│  └───────────────────────────────────┘         │
└─────────────────────────────────────────────────┘
                     ▼
         ┌─────────────────────┐
         │ Timer Expiration    │
         │ Generates Input Msg │
         │ PB_IMSG_TIMER       │
         └─────────────────────┘
                     ▼
         ┌─────────────────────┐
         │   Game Logic        │
         │   Processes Event   │
         └─────────────────────┘
```

**Timer Types:**

**Watchdog Timer (ID=0):**
- Single dedicated timer with its own storage
- Doesn't consume a slot in the 10-timer queue
- Ideal for critical game events (ball save, skill shot windows)
- Checked first in all timer operations for efficiency

**User Timers (IDs 1+):**
- Up to 10 concurrent timers in queue
- Custom IDs for different game events
- Optional repeat mode for periodic events
- Processed after watchdog timer

**Timer Message Flow:**
```
Set Timer ──→ Timer Running ──→ Timer Expires ──→ Input Message Generated
    │              │                  │                    │
    └──(start)     └──(countdown)     └──(expire)         └──(PB_IMSG_TIMER)
                                                                │
                                                                ▼
                                                         Game Logic Handles
```

**Common Use Cases:**
- **Ball Save Timer**: Grace period after ball launch
- **Skill Shot Window**: Time limit for skill shot scoring
- **Multiball Delay**: Delay between ejecting multiple balls
- **Mode Timeout**: Time-limited game modes
- **Bonus Countdown**: Animated bonus display timing
- **Periodic Events**: Repeating timers for animations or effects

**Key Features:**
- **Automatic Management**: Timers are processed automatically in main loop
- **Flexible IDs**: Use meaningful names via #define constants
- **Non-blocking**: Timers run in background without halting game
- **Message-based**: Timer events delivered through input message queue
- **Status Checking**: Query if timer is still active
- **Early Cancellation**: Stop timers before expiration if needed

**Documentation:** [PBEngine_API.md](PBEngine_API.md) (Timer System section)

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
Windows Build (simulator only):
    #define EXE_MODE_WINDOWS

Raspberry Pi Hardware Build (physical pinball hardware):
    #define EXE_MODE_RASPI
    #define ENABLE_PINBALL_HARDWARE

Raspberry Pi Simulator Build (no hardware required, uses X11 window + keyboard):
    #define EXE_MODE_RASPI

Debian/Linux Build (simulator only):
    #define EXE_MODE_DEBIAN
```

`ENABLE_PINBALL_HARDWARE` is only valid with `EXE_MODE_RASPI`. When omitted, any platform
runs in simulator mode — presenting a windowed display and accepting keyboard input.
This lets you develop and test on a Raspberry Pi without any physical hardware attached.

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
                  │ GPIO    │ │ • IO (TCA9555)   │
                  │         │ │ • LED (TLC59116) │
                  │NeoPixel │ │                  │
                  │ Strips  │ └────────┬─────────┘
                  └─────────┘          │
                      │                │
        ┌─────────────┴────────────────┴────────────────────┐
        ▼                                                    ▼
┌───────────────┐                                   ┌───────────────┐
│ Input Devices │                                   │Output Devices │
│               │                                   │               │
│ • Switches    │                                   │ • Solenoids   │
│ • Buttons     │                                   │ • Motors      │
│ • Sensors     │                                   │ • LEDs        │
│               │                                   │ • RGB Strips  │
└───────────────┘                                   └───────────────┘
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

**NeoPixel RGB LED Strips (WS2812B/SK6812):**
- Direct GPIO control for addressable RGB LED strips
- Full 24-bit color control per LED
- Multiple timing methods (clock_gettime, NOP, SPI, PWM)
- **SPI GPIO pins (GPIO 10 or 20) are strongly preferred** for reliable operation
- SPI method recommended for production (hardware-based, most reliable)
- SPI must be enabled via Raspberry Pi Configuration tool (Preferences → Interfaces → SPI)
- Independent brightness control and color presets
- Animation sequences with autonomous playback
- Recommended maximum: 60 LEDs per driver
- Can use multiple drivers for larger installations

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
- **Timers:** Time-delayed events generate messages automatically

### Message Flow Example

**Hardware Input Example:**
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

**Timer-Generated Input Example:**
```
1. Game sets timer: pbeSetTimer(TIMER_BALL_SAVE, 5000)
   ↓
2. Timer runs in background for 5 seconds
   ↓
3. Timer expires - input message created:
   {inputMsg: PB_IMSG_TIMER, inputId: TIMER_BALL_SAVE, tick: 6234}
   ↓
4. Message placed in input queue
   ↓
5. Game code processes timer event
   ↓
6. Game takes appropriate action (end ball save, etc.)
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
    1. Execute Devices
       └─→ Process device state machines
       └─→ Update device states
    
    2. Process Timers
       └─→ Check for expired timers
       └─→ Generate timer input messages
    
    3. Process I/O
       └─→ Read inputs
       └─→ Send outputs
    
    4. Process Input Messages
       └─→ Update game state
       └─→ Generate output messages
    
    5. Render Frame
       └─→ Clear screen
       └─→ Draw sprites
       └─→ Render text
       └─→ Swap buffers
    
    6. Frame Limiting
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
Edit io_definitions.json:

    Define inputs (in "inputs" array):
        {id, idx, name, key, msg, pin, board, boardIdx, debMs, auto, autoOut, ...}
    
    Define outputs (in "outputs" array):
        {id, idx, name, msg, pin, board, boardIdx, onMs, offMs}

Run: python scripts/generate_io_header.py
    → regenerates src/io_defs_generated.h with IDI_*/IDO_* #defines
    → rebuild the project to pick up changes
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
6. **[LED_Control_API.md](LED_Control_API.md)** - LED control, NeoPixel RGB strips, and animation sequences
7. **[NeoPixel_Timing_Methods.md](NeoPixel_Timing_Methods.md)** - Timing methods for reliable NeoPixel control (clock_gettime, NOP, SPI, PWM)
8. **[NeoPixel_Instrumentation.md](NeoPixel_Instrumentation.md)** - Diagnostic tools for NeoPixel timing verification
9. **[PBDevice_API.md](PBDevice_API.md)** - Device management for complex pinball mechanisms

### Additional Resources
10. **[HowToBuild.md](HowToBuild.md)** - Build instructions for Windows and Raspberry Pi
11. **[Utilities_Guide.md](Utilities_Guide.md)** - Utility tools and helpers
12. **[UsersGuide.md](UsersGuide.md)** - Comprehensive framework guide (pre-existing)

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

## Advanced Topics

### Creating Custom Devices

A *device* is a state machine that manages a physical mechanism (ejector, hopper, gate, motor, etc.) that requires timed, multi-step control. The `PBDevice` base class in `src/user/PBDevice.h` provides the lifecycle scaffolding; you supply the state machine by overriding `pbdExecute()`.

Derive your class from `PBDevice`, override the virtual functions (`pbdInit`, `pbdEnable`, `pdbStartRun`, and the required `pbdExecute`), and add whatever state variables your mechanism needs. Inside `pbdExecute()` — which is called every frame — use `m_state` to track progress through the sequence, `m_pEngine->GetTickCountGfx()` for elapsed-time checks, and `m_pEngine->SendOutputMsg()` to fire solenoids, LEDs, and motors. Create an instance in `Pinball_Table.cpp`, call `pbdInit()` during table initialization, and call `pbdExecute()` each frame from your update function.

The included `pbdEjector` and `pbdHopperEjector` classes in `PBDevice.h` are fully-working reference implementations that cover the most common patterns.

**Reference:** [PBDevice_API.md](PBDevice_API.md)

---

### Creating Custom LED Sequences

LED sequences animate the chip-based LED drivers (TLC59116). A sequence is a fixed list of steps defined at compile time in `src/user/PBSequences.h/.cpp`. Each step specifies which LEDs on each chip are active (as a 16-bit bitmask per chip), how long the pattern stays on, and an optional off-gap before the next step.

To add a new sequence, declare the `LEDSequence` and its chip mask in `PBSequences.h`, define the step array and data structures in `PBSequences.cpp`, then trigger it at runtime with `SendSeqMsg()`. The engine plays the sequence autonomously — no per-frame management needed. The existing `PBSeq_AllChipsTest`, `PBSeq_LastThreeTest`, and `PBSeq_RGBColorCycle` sequences are fully-working examples.

**References:** [LED_Control_API.md](LED_Control_API.md)

---

### Creating Custom NeoPixel Animation Functions

NeoPixel animation functions produce real-time, dynamic effects on WS2812B/SK6812 RGB LED strips. Unlike LED sequences (a static step list for chip-based LEDs), these are `PBEngine` methods called every frame that compute pixel colors on the fly — enabling smooth fades, moving snakes, strobes, and anything else you can calculate.

The built-in animations in `Pinball_Engine.cpp` are the reference implementations:

| Function | Effect |
|----------|--------|
| `neoPixelGradualFade` | Smooth cross-fade from one color to another across all pixels |
| `neoPixelSweepFromEnds` | Color change sweeps from outer ends toward center (or reverse) |
| `neoPixelToggle` | Up to 4 colors rotate/shift along the strip every step |
| `neoPixelSplitToggle` | First and second halves swap between two colors |
| `neoPixelStrobe` | Flashes between black and a color; off-time can shrink over a duration |
| `neoPixelSnake` | A moving "snake" of pixels runs along the strip over a base color |

Every animation follows the same pattern: it's called every frame, throttles itself internally using `stepTimeMS`, and keeps per-strip state in a static map keyed by `neoPixelId` so multiple strips run independently. Pixel changes are staged with `SendNeoPixelAllMsg()` or `SendNeoPixelSingleMsg()`, and the engine flushes them to hardware each frame via `SendAllStagedNeoPixels()`.

To add a new animation, declare it in `Pinball_Engine.h` alongside the existing functions, implement it in `Pinball_Engine.cpp` following the state-map pattern of the existing functions, then call it each frame from your table update code. Use `GetTickCountGfx()` for timing and `m_NeoPixelDriverMap` to query LED count for per-pixel effects.

**References:** [LED_Control_API.md](LED_Control_API.md) · [NeoPixel_Timing_Methods.md](NeoPixel_Timing_Methods.md) · [NeoPixel_Instrumentation.md](NeoPixel_Instrumentation.md)

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
- **PBEngine_API.md** - Core engine class with timer system
- **IO_Processing_API.md** - Message-based I/O system  
- **LED_Control_API.md** - LED control, NeoPixel RGB strips, and sequences
- **NeoPixel_Timing_Methods.md** - Timing methods for reliable NeoPixel control
- **NeoPixel_Instrumentation.md** - Diagnostic tools for NeoPixel timing
- **PBDevice_API.md** - Device management framework for complex mechanisms
- **Platform_Init_API.md** - Initialization and main loop
- **Game_Creation_API.md** - Graphics, sound, video playback, and screen management
- **Utilities_Guide.md** - Utility tools and helpers

Each document contains detailed API references, code examples, and best practices based on actual framework usage.

# Platform Initialization API Reference

## Overview

The Platform Initialization system handles platform-specific startup, rendering initialization, and main loop setup for both Windows (development/simulation) and Raspberry Pi (production hardware). The same API works across platforms with different underlying implementations.

## Key Concepts

- **Cross-Platform Design**: Same function signatures for Windows and Raspberry Pi
- **Build Switching**: Use `PBBuildSwitch.h` to select target platform
- **Render Initialization**: OpenGL ES setup for both platforms
- **Main Loop**: Consistent game loop structure

---

## Build Configuration

### PBBuildSwitch.h

Select the target platform by defining the appropriate mode.

**Windows Build:**
```cpp
#define EXE_MODE_WINDOWS
// #define EXE_MODE_RASPI
```

**Raspberry Pi Build:**
```cpp
// #define EXE_MODE_WINDOWS
#define EXE_MODE_RASPI
```

**Note:** Only one mode should be defined at a time.

---

## Initialization Functions

### PBInitRender()

Initializes the rendering system for the selected platform.

**Signature:**
```cpp
bool PBInitRender(long width, long height);
```

**Parameters:**
- `width` - Screen width in pixels
- `height` - Screen height in pixels

**Returns:** `true` if successful, `false` on error

**Standard Usage:**
```cpp
if (!PBInitRender(PB_SCREENWIDTH, PB_SCREENHEIGHT)) {
    // Initialization failed
    return false;
}
```

**What It Does:**

**Windows:**
- Creates a native Windows window
- Initializes OpenGL ES via ANGLE
- Sets up the graphics context
- Initializes sound system

**Raspberry Pi:**
- Initializes native EGL surface
- Sets up OpenGL ES 3.1 context
- Configures full-screen rendering
- Initializes sound system

**Internal Flow (Both Platforms):**
```cpp
bool PBInitRender(long width, long height) {
    // 1. Create platform-specific window/surface
    // 2. Initialize OpenGL ES context
    if (!g_PBEngine.oglInit(width, height, nativeWindow)) return false;
    
    // 3. Initialize graphics subsystem
    if (!g_PBEngine.gfxInit()) return false;
    
    // 4. Initialize sound system
    g_PBEngine.m_soundSystem.pbsInitialize();
    
    return true;
}
```

---

## Version Information

### ShowVersion()

Displays version information to the console.

**Signature:**
```cpp
void ShowVersion();
```

**Example:**
```cpp
ShowVersion();
// Output: "RasPin Pinball Engine v0.5.102"
```

**Version Constants:**
```cpp
#define PB_VERSION_MAJOR 0
#define PB_VERSION_MINOR 5  
#define PB_VERSION_BUILD 102
```

---

## Main Program Entry

### main()

The main entry point for the application.

**Signature:**
```cpp
int main(int argc, char const *argv[]);
```

**Standard Implementation:**
```cpp
int main(int argc, char const *argv[]) {
    // 1. Show version
    ShowVersion();
    
    // 2. Initialize rendering
    g_PBEngine.pbeSendConsole("OpenGL ES: Initialize");
    if (!PBInitRender(PB_SCREENWIDTH, PB_SCREENHEIGHT)) {
        return false;
    }
    g_PBEngine.pbeSendConsole("OpenGL ES: Successful");
    
    // 3. Load system font
    g_PBEngine.m_defaultFontSpriteId = g_PBEngine.gfxGetSystemFontSpriteId();
    g_PBEngine.m_consoleTextHeight = g_PBEngine.gfxGetTextHeight(
        g_PBEngine.m_defaultFontSpriteId);
    
    // 4. Setup I/O hardware
    g_PBEngine.pbeSetupIO();
    
    // 5. Load saved settings and high scores
    if (!g_PBEngine.pbeLoadSaveFile(false, false)) {
        g_PBEngine.pbeSaveFile();  // Create default save file
    }
    
    // 6. Configure audio
    g_PBEngine.m_ampDriver.SetVolume(
        g_PBEngine.m_saveFileData.mainVolume * 10);
    g_PBEngine.m_soundSystem.pbsSetMasterVolume(100);
    g_PBEngine.m_soundSystem.pbsSetMusicVolume(
        g_PBEngine.m_saveFileData.musicVolume * 10);
    g_PBEngine.m_soundSystem.pbsPlayMusic(MUSICFANTASY);
    
    // 7. Main game loop
    unsigned long currentTick = g_PBEngine.GetTickCountGfx();
    unsigned long lastTick = currentTick;
    unsigned int startFrameTime = currentTick;
    bool didLimitRender = false;
    bool firstLoop = true;
    
    while (true) {
        currentTick = g_PBEngine.GetTickCountGfx();
        
        // Process I/O (skip on first loop)
        if (!firstLoop) {
            PBProcessIO();
            
            // Process input messages
            if (!g_PBEngine.m_inputQueue.empty()) {
                stInputMessage inputMessage = g_PBEngine.m_inputQueue.front();
                g_PBEngine.m_inputQueue.pop();
                
                if (!g_PBEngine.m_GameStarted) {
                    g_PBEngine.pbeUpdateState(inputMessage);
                } else {
                    g_PBEngine.pbeUpdateGameState(inputMessage);
                }
            }
        } else {
            firstLoop = false;
        }
        
        // Frame rate limiting
        if (((currentTick - startFrameTime) >= PB_MS_PER_FRAME) || 
            (PB_MS_PER_FRAME == 0)) {
            didLimitRender = false;
            startFrameTime = currentTick;
        }
        
        // Render when not rate-limited
        if (!didLimitRender) {
            // Render current screen
            if (!g_PBEngine.m_GameStarted) {
                g_PBEngine.pbeRenderScreen(currentTick, lastTick);
            } else {
                g_PBEngine.pbeRenderGameScreen(currentTick, lastTick);
            }
            
            // Render overlay if enabled
            if (g_PBEngine.m_EnableOverlay) {
                g_PBEngine.pbeRenderOverlay(currentTick, lastTick);
            }
            
            // Render FPS counter if enabled
            if (g_PBEngine.m_ShowFPS) {
                std::string temp = "FPS: " + std::to_string(g_PBEngine.m_RenderFPS);
                g_PBEngine.gfxSetColor(g_PBEngine.m_defaultFontSpriteId, 
                                       255, 255, 255, 255);
                g_PBEngine.gfxRenderShadowString(
                    g_PBEngine.m_defaultFontSpriteId, temp, 
                    10, PB_SCREENHEIGHT - 30, 1, 
                    GFX_TEXTLEFT, 0, 0, 0, 255, 1);
            }
            
            // Swap buffers
            g_PBEngine.gfxSwap(false);
            
            lastTick = currentTick;
            didLimitRender = true;
        }
    }
    
    return 0;
}
```

---

## Configuration Constants

### Screen Resolution

```cpp
#define PB_SCREENWIDTH 1920
#define PB_SCREENHEIGHT 1080
```

Set these to match your display resolution. Common values:
- **1920x1080** - Full HD (standard for pinball displays)
- **1280x720** - HD (testing/development)
- **3840x2160** - 4K (high-end displays)

### Frame Rate Limiting

```cpp
#define PB_FPSLIMIT 30
#define PB_MS_PER_FRAME (PB_FPSLIMIT == 0 ? 0 : (1000 / PB_FPSLIMIT))
```

**Common Settings:**
- `30` - 30 FPS (good balance for pinball)
- `60` - 60 FPS (smooth, higher CPU usage)
- `0` - Unlimited (benchmark mode)

**Example:**
```cpp
// Limit to 60 FPS
#define PB_FPSLIMIT 60

// Unlimited FPS
#define PB_FPSLIMIT 0
```

---

## Platform-Specific Details

### Windows Platform

#### Native Window Handle

```cpp
HWND g_WHND = NULL;  // Windows window handle
```

#### PBWinSimInput()

Translates keyboard input to pinball input messages for simulation.

**Signature:**
```cpp
void PBWinSimInput(std::string character, PBPinState inputState, 
                   stInputMessage* inputMessage);
```

**Keyboard Mapping:**
Defined in `g_inputDef[].simMapKey`:
```cpp
// Example mappings
"z" → Left flipper
"/" → Right flipper
"1" → Start button
"a" → Left slingshot
```

**Usage (called internally):**
```cpp
MSG msg;
while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_KEYDOWN) {
        char character = MapVirtualKey(msg.wParam, MAPVK_VK_TO_CHAR);
        std::string key(1, character);
        
        stInputMessage inputMessage;
        PBWinSimInput(key, PB_ON, &inputMessage);
        g_PBEngine.m_inputQueue.push(inputMessage);
    }
}
```

#### Windows I/O Processing

```cpp
bool PBProcessInput() {
    // Processes Windows message queue
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return false;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        // Handle keyboard input
    }
    return true;
}

bool PBProcessOutput() {
    // No-op for Windows (no hardware to control)
    return true;
}

bool PBProcessIO() {
    PBProcessInput();
    PBProcessOutput();
    return true;
}
```

### Raspberry Pi Platform

#### Native Window Type

```cpp
EGLNativeWindowType g_PiWindow;  // Raspberry Pi window surface
```

#### Raspberry Pi I/O Processing

```cpp
bool PBProcessInput() {
    // Read GPIO pins
    // Read I2C I/O expanders
    // Debounce inputs
    // Generate input messages
    // Handle auto-outputs
    return true;
}

bool PBProcessOutput() {
    // Process output message queue
    // Control I/O expander chips
    // Control LED driver chips
    // Manage pulse outputs
    // Handle LED sequences
    return true;
}

bool PBProcessIO() {
    PBProcessInput();
    PBProcessOutput();
    return true;
}
```

---

## Startup Sequence

### Complete Initialization Flow

```
1. main() entry
   │
   ├─→ ShowVersion()
   │   └─→ Display version to console
   │
   ├─→ PBInitRender()
   │   ├─→ Create native window/surface
   │   ├─→ g_PBEngine.oglInit()
   │   ├─→ g_PBEngine.gfxInit()
   │   └─→ g_PBEngine.m_soundSystem.pbsInitialize()
   │
   ├─→ Load system font
   │   ├─→ gfxGetSystemFontSpriteId()
   │   └─→ gfxGetTextHeight()
   │
   ├─→ g_PBEngine.pbeSetupIO()
   │   ├─→ Initialize I/O expanders (Pi only)
   │   ├─→ Initialize LED drivers (Pi only)
   │   ├─→ Setup GPIO pins (Pi only)
   │   └─→ Configure debouncing
   │
   ├─→ g_PBEngine.pbeLoadSaveFile()
   │   ├─→ Load settings
   │   └─→ Load high scores
   │
   ├─→ Audio configuration
   │   ├─→ m_ampDriver.SetVolume()
   │   ├─→ pbsSetMasterVolume()
   │   ├─→ pbsSetMusicVolume()
   │   └─→ pbsPlayMusic()
   │
   └─→ Enter main game loop
       └─→ Process I/O, input, render (repeat)
```

---

## Minimal Main Program

For a simple application, here's the minimal main():

```cpp
int main(int argc, char const *argv[]) {
    // Initialize
    if (!PBInitRender(PB_SCREENWIDTH, PB_SCREENHEIGHT)) {
        return -1;
    }
    
    g_PBEngine.pbeSetupIO();
    g_PBEngine.SetAutoOutputEnable(true);
    
    // Main loop
    unsigned long lastTick = g_PBEngine.GetTickCountGfx();
    
    while (true) {
        unsigned long currentTick = g_PBEngine.GetTickCountGfx();
        
        // Process hardware I/O
        PBProcessIO();
        
        // Process input messages
        while (!g_PBEngine.m_inputQueue.empty()) {
            stInputMessage msg = g_PBEngine.m_inputQueue.front();
            g_PBEngine.m_inputQueue.pop();
            // Handle input
        }
        
        // Render
        g_PBEngine.pbeRenderGameScreen(currentTick, lastTick);
        g_PBEngine.gfxSwap(false);
        
        lastTick = currentTick;
    }
    
    return 0;
}
```

---

## Global Engine Instance

### g_PBEngine

The global engine object is declared in Pinball.cpp:

```cpp
PBEngine g_PBEngine;
```

Accessible from anywhere via:
```cpp
extern PBEngine g_PBEngine;
```

**Example:**
```cpp
// In your game code
g_PBEngine.pbeSendConsole("Game started!");
g_PBEngine.SendOutputMsg(PB_OMSG_LED, LED_START, PB_ON, false);
```

---

## Resource File Paths

### Default Resource Locations

```cpp
#define MENUFONT "src/resources/fonts/Baldur_96_768.png"
#define MENUSWORD "src/resources/textures/MenuSword.png"
#define SAVEFILENAME "src/resources/savefile.bin"

// Sound files
#define MUSICFANTASY "src/resources/sound/fantasymusic.mp3"
#define EFFECTSWORDHIT "src/resources/sound/swordcut.mp3"
#define EFFECTCLICK "src/resources/sound/click.mp3"
```

**Customize for your table:**
```cpp
#define TABLE_BACKGROUND "src/resources/textures/my_table_bg.png"
#define TABLE_MUSIC "src/resources/sound/my_table_music.mp3"
```

---

## Debugging and Diagnostics

### Console Output

Send debug messages during initialization:

```cpp
g_PBEngine.pbeSendConsole("Initializing custom hardware...");
// Your initialization code
g_PBEngine.pbeSendConsole("Hardware ready!");
```

### Enable Debug Overlay

```cpp
g_PBEngine.m_EnableOverlay = true;  // Show I/O state overlay
g_PBEngine.m_ShowFPS = true;        // Show FPS counter
```

### Check Initialization Status

```cpp
if (!PBInitRender(PB_SCREENWIDTH, PB_SCREENHEIGHT)) {
    g_PBEngine.pbeSendConsole("ERROR: Render initialization failed");
    return -1;
}

std::string screenInfo = "Screen: " + 
    std::to_string(g_PBEngine.oglGetScreenWidth()) + "x" + 
    std::to_string(g_PBEngine.oglGetScreenHeight());
g_PBEngine.pbeSendConsole(screenInfo);
```

---

## Platform-Specific Includes

### Windows

```cpp
#ifdef EXE_MODE_WINDOWS
#include <windows.h>
#include "PBWinRender.h"
#endif
```

### Raspberry Pi

```cpp
#ifdef EXE_MODE_RASPI
#include "wiringPi.h"
#include "wiringPiI2C.h"
#include "PBDebounce.h"
#include "PBRasPiRender.h"
#endif
```

### Common (All Platforms)

```cpp
#include <egl.h>
#include <gl31.h>
#include "PBOGLES.h"
#include "PBGfx.h"
#include "Pinball_IO.h"
#include "Pinball_Table.h"
#include "PBSound.h"
```

---

## Best Practices

### Initialization Order

1. **Version display** - First console output
2. **Render system** - Required for all graphics
3. **Fonts** - Needed for console and text rendering
4. **I/O setup** - Initialize hardware interfaces
5. **Load settings** - Restore user preferences
6. **Audio setup** - Configure sound and music
7. **Main loop** - Start game processing

### Error Handling

```cpp
if (!PBInitRender(PB_SCREENWIDTH, PB_SCREENHEIGHT)) {
    std::cerr << "Failed to initialize render system" << std::endl;
    return -1;
}

if (!g_PBEngine.pbeSetupIO()) {
    g_PBEngine.pbeSendConsole("WARNING: I/O setup incomplete");
    // Continue with limited functionality
}
```

### Platform Testing

```cpp
#ifdef EXE_MODE_WINDOWS
    g_PBEngine.pbeSendConsole("Running in Windows simulation mode");
#endif

#ifdef EXE_MODE_RASPI
    g_PBEngine.pbeSendConsole("Running on Raspberry Pi hardware");
#endif
```

---

## See Also

- **PBEngine_API.md** - Core engine class reference
- **IO_Processing_API.md** - I/O processing details
- **LED_Control_API.md** - LED and sequence control
- **HowToBuild.md** - Build instructions and configuration
- **UsersGuide.md** - Complete framework documentation

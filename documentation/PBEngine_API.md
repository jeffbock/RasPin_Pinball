# PBEngine Class API Reference

## Overview

The `PBEngine` class is the core game engine for the RasPin Pinball framework. It manages game states, rendering, I/O message queues, and provides the main interface for controlling your pinball machine.

## Key Responsibilities

- **Game State Management**: Controls main menu, gameplay, test mode, settings, etc.
- **Message Queue Management**: Handles input and output message queues
- **Rendering Coordination**: Manages all screen rendering operations
- **Hardware Control**: Interfaces with I/O and LED chips
- **Save/Load Management**: Handles high scores and settings persistence

## Global Instance

```cpp
extern PBEngine g_PBEngine;
```

The engine is accessed through the global `g_PBEngine` object throughout the application.

---

## Core State Management

### pbeGetMainState()

Returns the current main state of the engine.

**Signature:**
```cpp
PBMainState pbeGetMainState();
```

**Returns:** Current `PBMainState` enum value

**Available States:**
- `PB_BOOTUP` - System initialization
- `PB_STARTMENU` - Main menu
- `PB_PLAYGAME` - Active gameplay
- `PB_TESTMODE` - Hardware testing
- `PB_SETTINGS` - Settings menu
- `PB_DIAGNOSTICS` - System diagnostics

**Example:**
```cpp
if (g_PBEngine.pbeGetMainState() == PB_PLAYGAME) {
    // Handle gameplay-specific logic
}
```

### pbeUpdateState()

Processes input messages when not in active gameplay to update menu states.

**Signature:**
```cpp
void pbeUpdateState(stInputMessage inputMessage);
```

**Parameters:**
- `inputMessage` - Input message structure containing button/switch state

**Example:**
```cpp
stInputMessage inputMessage;
if (!g_PBEngine.m_inputQueue.empty()) {
    inputMessage = g_PBEngine.m_inputQueue.front();
    g_PBEngine.m_inputQueue.pop();
    
    if (!g_PBEngine.m_GameStarted) {
        g_PBEngine.pbeUpdateState(inputMessage);
    }
}
```

### pbeUpdateGameState()

Processes input messages during active gameplay.

**Signature:**
```cpp
void pbeUpdateGameState(stInputMessage inputMessage);
```

**Parameters:**
- `inputMessage` - Input message from switches, buttons, or sensors

**Example:**
```cpp
if (g_PBEngine.m_GameStarted) {
    g_PBEngine.pbeUpdateGameState(inputMessage);
}
```

---

## Output Control

### SendOutputMsg()

Sends a message to control I/O or LED outputs.

**Signature:**
```cpp
void SendOutputMsg(PBOutputMsg outputMsg, unsigned int outputId, 
                   PBPinState outputState, bool usePulse, 
                   stOutputOptions* options = nullptr);
```

**Parameters:**
- `outputMsg` - Type of output message (LED, IO, etc.)
- `outputId` - Output identifier from your I/O definitions
- `outputState` - `PB_ON` or `PB_OFF`
- `usePulse` - Enable pulse mode for timed outputs
- `options` - Optional settings for brightness, blink, sequences

**Output Message Types:**
- `PB_OMSG_GENERIC_IO` - Generic I/O output (solenoids, motors)
- `PB_OMSG_LED` - LED on/off control
- `PB_OMSG_LEDSET_BRIGHTNESS` - Set LED brightness
- `PB_OMSG_LEDCFG_GROUPDIM` - Configure group dimming
- `PB_OMSG_LEDCFG_GROUPBLINK` - Configure group blinking
- `PB_OMSG_LED_SEQUENCE` - Start/stop LED sequence

**Example - Fire a Solenoid:**
```cpp
// Fire left slingshot with pulse
g_PBEngine.SendOutputMsg(PB_OMSG_GENERIC_IO, OUTPUT_LEFT_SLINGSHOT, 
                         PB_ON, true);
```

**Example - Turn on an LED:**
```cpp
// Turn on rollover lane LED
g_PBEngine.SendOutputMsg(PB_OMSG_LED, LED_ROLLOVER_1, 
                         PB_ON, false);
```

**Example - Set LED Brightness:**
```cpp
stOutputOptions options;
options.brightness = 128; // 50% brightness (0-255)
g_PBEngine.SendOutputMsg(PB_OMSG_LEDSET_BRIGHTNESS, LED_BACKGLASS, 
                         PB_ON, false, &options);
```

### SendRGBMsg()

Sends coordinated messages to control an RGB LED.

**Signature:**
```cpp
void SendRGBMsg(unsigned int redId, unsigned int greenId, 
                unsigned int blueId, PBLEDColor color, 
                PBPinState outputState, bool usePulse, 
                stOutputOptions* options = nullptr);
```

**Parameters:**
- `redId`, `greenId`, `blueId` - Output IDs for RGB components
- `color` - Color enumeration value
- `outputState` - `PB_ON` or `PB_OFF`
- `usePulse` - Enable pulse mode
- `options` - Optional settings

**Example:**
```cpp
// Set RGB LED to purple
g_PBEngine.SendRGBMsg(LED_RGB_RED, LED_RGB_GREEN, LED_RGB_BLUE, 
                      PB_LED_PURPLE, PB_ON, false);
```

### SendSeqMsg()

Starts or stops an LED sequence animation.

**Signature:**
```cpp
void SendSeqMsg(const LEDSequence* sequence, const uint16_t* mask, 
                PBSequenceLoopMode loopMode, PBPinState outputState);
```

**Parameters:**
- `sequence` - Pointer to LED sequence data structure
- `mask` - Bitmask array indicating which LEDs participate
- `loopMode` - `PB_NOLOOP`, `PB_LOOP`, `PB_PINGPONG`, `PB_PINGPONGLOOP`
- `outputState` - `PB_ON` to start, `PB_OFF` to stop

**Example:**
```cpp
// Start a chase sequence on specific LEDs
uint16_t ledMask[NUM_LED_CHIPS] = {0xFFFF, 0x0000, 0x0000}; // All LEDs on chip 0
g_PBEngine.SendSeqMsg(&g_ChaseSequence, ledMask, PB_LOOP, PB_ON);

// Stop the sequence later
g_PBEngine.SendSeqMsg(nullptr, nullptr, PB_NOLOOP, PB_OFF);
```

---

## Auto Output Control

Auto output allows inputs to automatically trigger outputs without manual processing in your game code. This is useful for flippers and instant feedback mechanisms.

### SetAutoOutputEnable()

Globally enables or disables auto output functionality.

**Signature:**
```cpp
void SetAutoOutputEnable(bool enable);
```

**Example:**
```cpp
// Enable auto outputs during gameplay
g_PBEngine.SetAutoOutputEnable(true);

// Disable during test mode
g_PBEngine.SetAutoOutputEnable(false);
```

### GetAutoOutputEnable()

Returns current auto output enable state.

**Signature:**
```cpp
bool GetAutoOutputEnable() const;
```

### SetAutoOutput()

Configures auto output for a specific input.

**Signature:**
```cpp
bool SetAutoOutput(unsigned int index, bool autoOutputEnabled);
```

**Parameters:**
- `index` - Input definition array index
- `autoOutputEnabled` - Enable/disable auto output for this input

**Example:**
```cpp
// Configure in your I/O setup
g_inputDef[INPUT_LEFT_FLIPPER].autoOutput = true;
g_inputDef[INPUT_LEFT_FLIPPER].autoOutputId = OUTPUT_LEFT_FLIPPER;
g_inputDef[INPUT_LEFT_FLIPPER].autoPinState = PB_ON;

// Enable globally
g_PBEngine.SetAutoOutputEnable(true);
```

---

## Rendering Functions

### pbeRenderScreen()

Renders the current non-game screen (menus, settings, etc.).

**Signature:**
```cpp
bool pbeRenderScreen(unsigned long currentTick, unsigned long lastTick);
```

**Parameters:**
- `currentTick` - Current frame timestamp in milliseconds
- `lastTick` - Previous frame timestamp

**Example:**
```cpp
unsigned long currentTick = g_PBEngine.GetTickCountGfx();
if (!g_PBEngine.m_GameStarted) {
    g_PBEngine.pbeRenderScreen(currentTick, lastTick);
}
```

### pbeRenderGameScreen()

Renders the active pinball gameplay screen.

**Signature:**
```cpp
bool pbeRenderGameScreen(unsigned long currentTick, unsigned long lastTick);
```

**Example:**
```cpp
if (g_PBEngine.m_GameStarted) {
    g_PBEngine.pbeRenderGameScreen(currentTick, lastTick);
}
```

---

## Console Output

### pbeSendConsole()

Sends a message to the debug console queue for display.

**Signature:**
```cpp
void pbeSendConsole(std::string output);
```

**Example:**
```cpp
g_PBEngine.pbeSendConsole("Ball locked in multiball");
g_PBEngine.pbeSendConsole("Score: " + std::to_string(currentScore));
```

### pbeClearConsole()

Clears all messages from the console queue.

**Signature:**
```cpp
void pbeClearConsole();
```

---

## Save File Management

### pbeLoadSaveFile()

Loads settings and high scores from disk.

**Signature:**
```cpp
bool pbeLoadSaveFile(bool loadDefaults, bool resetScores);
```

**Parameters:**
- `loadDefaults` - Load default values instead of from file
- `resetScores` - Clear high score table

**Returns:** `true` if successful

**Example:**
```cpp
if (!g_PBEngine.pbeLoadSaveFile(false, false)) {
    g_PBEngine.pbeSendConsole("Failed to load save file, using defaults");
    g_PBEngine.pbeSaveFile();
}
```

### pbeSaveFile()

Saves current settings and high scores to disk.

**Signature:**
```cpp
bool pbeSaveFile();
```

**Example:**
```cpp
// After updating high score
g_PBEngine.m_saveFileData.highScores[0].highScore = newScore;
g_PBEngine.m_saveFileData.highScores[0].playerInitials = "ABC";
g_PBEngine.pbeSaveFile();
```

---

## Hardware Interface

### pbeSetupIO()

Initializes all I/O and LED hardware based on your definitions.

**Signature:**
```cpp
bool pbeSetupIO();
```

**Called automatically during startup:**
```cpp
int main(int argc, char const *argv[]) {
    // ... initialization ...
    g_PBEngine.pbeSetupIO();
    // ... main loop ...
}
```

---

## Member Variables (Key Public Access)

### Message Queues

```cpp
std::queue<stInputMessage> m_inputQueue;    // Input messages to process
std::queue<stOutputMessage> m_outputQueue;  // Output messages to send
```

**Example:**
```cpp
// Check for pending input
if (!g_PBEngine.m_inputQueue.empty()) {
    stInputMessage msg = g_PBEngine.m_inputQueue.front();
    g_PBEngine.m_inputQueue.pop();
    // Process message
}
```

### Hardware Chip Arrays

```cpp
IODriverDebounce m_IOChip[NUM_IO_CHIPS];   // I/O expander chips
LEDDriver m_LEDChip[NUM_LED_CHIPS];        // LED driver chips
AmpDriver m_ampDriver;                      // Audio amplifier
```

### Sound System

```cpp
PBSound m_soundSystem;  // Background music and sound effects
```

**Example:**
```cpp
g_PBEngine.m_soundSystem.pbsPlayMusic(MUSICFANTASY);
g_PBEngine.m_soundSystem.pbsSetMusicVolume(80);
```

### Game State Flags

```cpp
bool m_GameStarted;       // True when in active gameplay
bool m_EnableOverlay;     // Show I/O debug overlay
bool m_ShowFPS;           // Display FPS counter
int m_RenderFPS;          // Current frames per second
```

### Save File Data

```cpp
stSaveFileData m_saveFileData;  // Settings and high scores
```

**Example:**
```cpp
// Access current settings
int volume = g_PBEngine.m_saveFileData.mainVolume;
int ballsPerGame = g_PBEngine.m_saveFileData.ballsPerGame;

// Access high scores
for (int i = 0; i < NUM_HIGHSCORES; i++) {
    unsigned long score = g_PBEngine.m_saveFileData.highScores[i].highScore;
    std::string initials = g_PBEngine.m_saveFileData.highScores[i].playerInitials;
}
```

---

## Typical Usage Pattern

```cpp
// In main loop
int main(int argc, char const *argv[]) {
    // 1. Initialize
    PBInitRender(PB_SCREENWIDTH, PB_SCREENHEIGHT);
    g_PBEngine.pbeSetupIO();
    g_PBEngine.pbeLoadSaveFile(false, false);
    
    // 2. Main game loop
    while (true) {
        unsigned long currentTick = g_PBEngine.GetTickCountGfx();
        
        // 3. Process I/O
        PBProcessIO();
        
        // 4. Handle input messages
        if (!g_PBEngine.m_inputQueue.empty()) {
            stInputMessage msg = g_PBEngine.m_inputQueue.front();
            g_PBEngine.m_inputQueue.pop();
            
            if (!g_PBEngine.m_GameStarted) {
                g_PBEngine.pbeUpdateState(msg);
            } else {
                g_PBEngine.pbeUpdateGameState(msg);
            }
        }
        
        // 5. Render
        if (!g_PBEngine.m_GameStarted) {
            g_PBEngine.pbeRenderScreen(currentTick, lastTick);
        } else {
            g_PBEngine.pbeRenderGameScreen(currentTick, lastTick);
        }
        
        g_PBEngine.gfxSwap(false);
        lastTick = currentTick;
    }
}
```

---

## See Also

- **IO_Processing_API.md** - Details on I/O message processing
- **LED_Control_API.md** - LED sequence and animation details
- **Platform_Init_API.md** - Platform-specific initialization
- **UsersGuide.md** - Complete framework documentation

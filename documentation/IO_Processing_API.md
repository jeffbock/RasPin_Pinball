# I/O Processing API Reference

## Overview

The I/O Processing system handles all input and output communication between the game engine and physical hardware. It uses a message-based architecture to decouple game logic from hardware control, making the code more maintainable and testable.

## Key Concepts

- **Message Queues**: Inputs and outputs use separate queues for asynchronous processing
- **Debouncing**: All inputs are debounced to prevent spurious signals
- **Pulse Outputs**: Timed outputs for solenoids (fire and hold patterns)
- **Auto Output**: Direct input-to-output routing for instant response (flippers)
- **Platform Independence**: Same API works on Windows (simulation) and Raspberry Pi

---

## Platform Functions

These functions have different implementations for Windows and Raspberry Pi, but provide the same interface.

### PBProcessIO()

Main I/O processing loop - reads inputs and sends outputs.

**Signature:**
```cpp
bool PBProcessIO();
```

**Returns:** `true` if successful

**Call Frequency:** Should be called as often as possible in your main loop

**Example:**
```cpp
while (true) {
    // Process I/O every loop iteration
    PBProcessIO();
    
    // Process input messages
    if (!g_PBEngine.m_inputQueue.empty()) {
        stInputMessage msg = g_PBEngine.m_inputQueue.front();
        g_PBEngine.m_inputQueue.pop();
        // Handle input
    }
    
    // Render and other game logic
}
```

### PBProcessInput()

Reads all input hardware and generates input messages. Called internally by `PBProcessIO()`.

**Signature:**
```cpp
bool PBProcessInput();
```

**Platform-Specific Behavior:**

**Windows:**
- Reads keyboard input for simulation
- Maps keys to pinball inputs (defined in `g_inputDef[].simMapKey`)
- Generates input messages for key press/release

**Raspberry Pi:**
- Reads Raspberry Pi GPIO pins
- Reads I2C I/O expander chips
- Debounces all inputs
- Generates messages only on state changes
- Triggers auto-outputs if enabled

**Example Input Definitions:**
```json
// In io_definitions.json → "inputs" array
{"id":"IDI_LFLIP",  "idx":0, "name":"LFlipper", "key":"A", "msg":"BUTTON", "pin":27, "board":"RASPI", "boardIdx":0, "state":"OFF", "tick":0, "debMs":5, "auto":false, "autoOut":"", "autoState":"OFF", "autoPulse":false},
{"id":"IDI_RFLIP",  "idx":1, "name":"RFlipper", "key":"D", "msg":"BUTTON", "pin":17, "board":"RASPI", "boardIdx":0, "state":"OFF", "tick":0, "debMs":5, "auto":false, "autoOut":"", "autoState":"OFF", "autoPulse":false},
{"id":"IDI_START",  "idx":4, "name":"Start",    "key":"Z", "msg":"BUTTON", "pin":6,  "board":"RASPI", "boardIdx":0, "state":"OFF", "tick":0, "debMs":5, "auto":false, "autoOut":"", "autoState":"OFF", "autoPulse":false}
```

After editing `io_definitions.json`, run:
```
python scripts/generate_io_header.py
```
This regenerates `src/io_defs_generated.h` with updated `IDI_*` / `IDO_*` `#define` constants and array size macros.

### PBProcessOutput()

Processes all output messages from the queue and controls hardware. Called internally by `PBProcessIO()`.

**Signature:**
```cpp
bool PBProcessOutput();
```

**Responsibilities:**
- Dequeues output messages
- Routes to appropriate hardware (I/O chips, LED chips, GPIO)
- Manages pulse outputs (timed solenoid firing)
- Handles LED sequences
- Manages deferred LED messages during sequences
- Updates hardware efficiently (staged writes)

---

## Raspberry Pi I/O Functions

These functions are only compiled when building for Raspberry Pi (`EXE_MODE_RASPI`).

### FindOutputDefIndex()

Finds the array index for an output definition by its ID.

**Signature:**
```cpp
int FindOutputDefIndex(unsigned int outputId);
```

**Parameters:**
- `outputId` - Output identifier to search for

**Returns:** Array index in `g_outputDef[]`, or `-1` if not found

**Example:**
```cpp
int index = FindOutputDefIndex(OUTPUT_LEFT_SLINGSHOT);
if (index != -1) {
    stOutputDef& outputDef = g_outputDef[index];
    // Use output definition
}
```

### ProcessIOOutputMessage()

Processes generic I/O output messages (solenoids, motors, etc.).

**Signature:**
```cpp
void ProcessIOOutputMessage(const stOutputMessage& message, 
                           stOutputDef& outputDef);
```

**Parameters:**
- `message` - Output message to process
- `outputDef` - Output definition structure

**Behavior:**
- Checks if output is currently in pulse mode (ignores new messages if so)
- Sets up pulse timing if `usePulse` is enabled
- Stages output to appropriate hardware (GPIO or I/O chip)
- Updates output state tracking

**Important Note:**
- **Message Dropping:** Any I/O message sent to an output that is currently running a pulse will be dropped/ignored. This protects the pulse timing and prevents interrupting timed operations like solenoid firing. Wait for the pulse to complete before sending new messages to that output.

**Example Usage (called internally):**
```cpp
// User code sends message
g_PBEngine.SendOutputMsg(PB_OMSG_GENERIC_IO, OUTPUT_BUMPER_1, 
                         PB_ON, true);

// System internally calls ProcessIOOutputMessage
// which sets up a timed pulse and controls the solenoid
```

### ProcessActivePulseOutputs()

Manages timing and state of all active pulse outputs.

**Signature:**
```cpp
void ProcessActivePulseOutputs();
```

**Called internally by:** `PBProcessOutput()`

**Behavior:**
- Tracks elapsed time for each pulse
- Controls ON phase (solenoid energized)
- Controls OFF phase (solenoid de-energized)
- Removes completed pulses from tracking map
- Works with I/O, GPIO, and LED outputs

**Pulse Definition Example:**
```json
// In io_definitions.json → "outputs" array
// Configure onMs / offMs for pulse timing
{"id":"IDO_RSLING", "idx":1, "name":"RSling", "msg":"GENERIC_IO", "pin":0, "board":"IO", "boardIdx":0, "state":"OFF", "onMs":50, "offMs":100, "neo":0}
//                                                                                                                "onMs":^^ "offMs":^^^
//                                                                                                                  ON duration  OFF duration
```

**Timing Diagram:**
```
Time:      0ms                    50ms                    150ms
           │                       │                       │
Message ───┘                       │                       │
           │                       │                       │
Output: ───┐                       ┌───────────────────────┘
   ON      │███████████████████████│  (onTimeMS = 50ms)
           └───────────────────────┐
   OFF                             │███████████████████████  (offTimeMS = 100ms)
                                   └───────────────────────
                                                           │
                                            Pulse Complete ┘
```

---

## Input Message Structure

### stInputMessage

Contains information about an input event.

```cpp
struct stInputMessage {
    PBInputMsg inputMsg;        // Message type identifier
    unsigned int inputId;       // Input ID from definitions
    PBPinState inputState;      // PB_ON or PB_OFF
    unsigned long sentTick;     // Timestamp in milliseconds
};
```

### Input Message Types

The `PBInputMsg` enum defines the types of input messages:

```cpp
enum PBInputMsg {
    PB_IMSG_EMPTY = 0,      // Empty/no message
    PB_IMSG_SENSOR = 1,     // Generic sensor
    PB_IMSG_TARGET = 2,     // Target hit
    PB_IMSG_JETBUMPER = 3,  // Jet bumper hit
    PB_IMSG_POPBUMPER = 4,  // Pop bumper hit
    PB_IMSG_BUTTON = 5,     // Button press/release
    PB_IMSG_TIMER = 6,      // Timer expiration (see Timer System in PBEngine_API.md)
};
```

**Note:** `PB_IMSG_TIMER` messages are generated by the timer system when a timer set via `pbeSetTimer()` expires. The `inputId` will contain the user-supplied timer ID.

**Example Processing:**
```cpp
stInputMessage msg = g_PBEngine.m_inputQueue.front();
g_PBEngine.m_inputQueue.pop();

switch (msg.inputMsg) {
    case PB_IMSG_BUTTON:
        if (msg.inputId == IDI_LEFTFLIPPER && msg.inputState == PB_ON) {
            // Flipper button pressed
        }
        break;
        
    case PB_IMSG_SENSOR:
        if (msg.inputState == PB_ON) {
            // Ball detected in lane
            AddScore(1000);
            g_PBEngine.SendOutputMsg(PB_OMSG_LED, LED_LANE_1, PB_ON, false);
        }
        break;
        
    case PB_IMSG_TIMER:
        // Timer expired - inputId contains the timer ID
        if (msg.inputId == TIMER_BALL_SAVE) {
            // Ball save timer expired
            m_ballSaveActive = false;
        }
        break;
}
```

---

## Output Message Structure

### stOutputMessage

Contains information for controlling an output.

```cpp
struct stOutputMessage {
    PBOutputMsg outputMsg;         // Message type
    unsigned int outputId;         // Output ID
    PBPinState outputState;        // PB_ON or PB_OFF
    bool usePulse;                 // Enable pulse mode
    unsigned long sentTick;        // Timestamp
    stOutputOptions *options;      // Optional parameters
};
```

### stOutputOptions

Optional parameters for output messages.

```cpp
struct stOutputOptions {
    unsigned int onBlinkMS;                    // LED blink on time
    unsigned int offBlinkMS;                   // LED blink off time
    unsigned int brightness;                   // LED brightness (0-255)
    PBSequenceLoopMode loopMode;              // Sequence loop mode
    uint16_t activeLEDMask[NUM_LED_CHIPS];    // Sequence LED mask
    const LEDSequence *setLEDSequence;        // Sequence data
};
```

**Note:** The `options` parameter can be `nullptr` if not required. Only pass an `stOutputOptions` structure when you need to specify brightness, blink rates, or sequence parameters.

**Example - Simple LED (no options needed):**
```cpp
// options parameter is nullptr (default)
g_PBEngine.SendOutputMsg(PB_OMSG_LED, LED_ROLLOVER_1, PB_ON, false);
```

**Example - LED with Brightness:**
```cpp
stOutputOptions options;
options.brightness = 200;

g_PBEngine.SendOutputMsg(PB_OMSG_LEDSET_BRIGHTNESS, LED_GI_STRIP, 
                         PB_ON, false, &options);
```

---

## Input/Output Definitions

All I/O definitions are stored in `io_definitions.json` at the project root. This is the **single source of truth** for every input and output. The workflow is:

1. Edit `io_definitions.json` to add, remove, or modify inputs/outputs.
2. Run `python scripts/generate_io_header.py` to regenerate `src/io_defs_generated.h`.
3. Rebuild the project. The generated header provides the `IDI_*` / `IDO_*` `#define` constants and `NUM_INPUTS` / `NUM_OUTPUTS` macros used throughout the codebase.
4. At runtime, `InitializeInputDefs()` and `InitializeOutputDefs()` load `io_definitions.json` and populate the `g_inputDef[]` and `g_outputDef[]` arrays automatically.

**Do not** manually edit `Pinball_IO.cpp` to define I/O entries or `src/io_defs_generated.h` — these are managed by the JSON file and the generator script.

### stInputDef

Defines an input's hardware connection and behavior. Populated at runtime from `io_definitions.json` — do not edit directly.

```cpp
struct stInputDef {
    std::string inputName;          // Human-readable name
    std::string simMapKey;          // Keyboard key for Windows simulation
    PBInputMsg inputMsg;            // Message type to generate
    unsigned int pin;               // Hardware pin number
    PBBoardType boardType;          // PB_RASPI, PB_IO, etc.
    unsigned int boardIndex;        // Which chip (for I/O expanders)
    PBPinState lastState;           // Last known state
    unsigned long lastStateTick;    // Timestamp of last state change
    unsigned long debounceTimeMS;   // Debounce window in milliseconds
    bool autoOutput;                // Enable auto output
    unsigned int autoOutputId;      // Output index to trigger
    PBPinState autoPinState;        // State to send to output
    bool autoOutputUsePulse;        // If true, auto-output uses pulse mode
};
```

**Note:** The array index of `g_inputDef[]` is the input ID and matches the corresponding `IDI_*` `#define` from `io_defs_generated.h`.

### stOutputDef

Defines an output's hardware connection and pulse timing. Populated at runtime from `io_definitions.json` — do not edit directly.

```cpp
struct stOutputDef {
    std::string outputName;         // Human-readable name
    PBOutputMsg outputMsg;          // Message type
    unsigned int pin;               // Hardware pin number
    PBBoardType boardType;          // PB_RASPI, PB_IO, PB_LED
    unsigned int boardIndex;        // Which chip
    PBPinState lastState;           // Last known state
    unsigned int onTimeMS;          // Pulse ON duration
    unsigned int offTimeMS;         // Pulse OFF duration
    unsigned int neoPixelIndex;     // NeoPixel chain index (0 if not applicable)
};
```

**Note:** The array index of `g_outputDef[]` is the output ID and matches the corresponding `IDO_*` `#define` from `io_defs_generated.h`.

**Example Definitions (in `io_definitions.json`):**
```json
"outputs": [
  {"id":"IDO_RSLING",   "idx":1, "name":"RSling",   "msg":"GENERIC_IO", "pin":0, "board":"IO",  "boardIdx":0, "state":"OFF", "onMs":50,  "offMs":100, "neo":0},
  {"id":"IDO_SAVELED",  "idx":11,"name":"Save LED", "msg":"LED",        "pin":4, "board":"LED", "boardIdx":0, "state":"OFF", "onMs":0,   "offMs":0,   "neo":0},
  {"id":"IDO_LFLIP",   "idx":3, "name":"LFlipper", "msg":"GENERIC_IO", "pin":2, "board":"IO",  "boardIdx":0, "state":"OFF", "onMs":100, "offMs":100, "neo":0}
]
```

---

## Auto Output System

Auto output allows an input to directly trigger an output without game code processing. This is essential for responsive feedback like flippers.

### Configuration

1. **Define Auto Output in `io_definitions.json`:**
```json
// In io_definitions.json → "inputs" array
// Set "auto":true, "autoOut":"<IDO_*>", "autoState":"ON|OFF", "autoPulse":true|false
{"id":"IDI_LFLIP", "idx":0, "name":"LFlipper", "key":"A", "msg":"BUTTON",
 "pin":27, "board":"RASPI", "boardIdx":0, "state":"OFF", "tick":0, "debMs":5,
 "auto":true, "autoOut":"IDO_LFLIP", "autoState":"ON", "autoPulse":false}
//              ^^^^^^^^^^^  ^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^
// Enable auto  Output to trigger            State to send   Pulse or track input
```

After editing, run `python scripts/generate_io_header.py` to regenerate `src/io_defs_generated.h`.

2. **Enable Auto Output Globally:**
```cpp
g_PBEngine.SetAutoOutputEnable(true);
```

### Behavior

When an input with auto output enabled changes state:
1. Normal input message is generated and queued
2. **Immediately** (before game code processes), output message is sent
3. Output uses the `autoState` from the JSON definition (typically `ON` for direct pass-through)
4. If `autoPulse` is `true`, the output fires a timed pulse using its `onMs`/`offMs` values

**Example - Flipper Setup (`io_definitions.json`):**
```json
// Input: left flipper with auto-output to solenoid (tracks input, no pulse)
{"id":"IDI_LFLIP", "idx":0, "name":"LFlipper", "key":"A", "msg":"BUTTON",
 "pin":27, "board":"RASPI", "boardIdx":0, "state":"OFF", "tick":0, "debMs":5,
 "auto":true, "autoOut":"IDO_LFLIP", "autoState":"ON", "autoPulse":false},

// Output: flipper solenoid
{"id":"IDO_LFLIP", "idx":3, "name":"LFlipper", "msg":"GENERIC_IO",
 "pin":2, "board":"IO", "boardIdx":0, "state":"OFF", "onMs":100, "offMs":100, "neo":0}
```

```cpp
// Enable in your initialization
g_PBEngine.SetAutoOutputEnable(true);

// Result: Flipper button press instantly triggers flipper solenoid
// with minimal latency, no game code processing required
```

---

## Windows Simulation Functions

### PBWinSimInput()

Translates Windows keyboard input to pinball input messages (Windows only).

**Signature:**
```cpp
void PBWinSimInput(std::string character, PBPinState inputState, 
                   stInputMessage* inputMessage);
```

**Parameters:**
- `character` - Keyboard character pressed
- `inputState` - `PB_ON` (key down) or `PB_OFF` (key up)
- `inputMessage` - Output parameter for generated message

**Example:**
```cpp
// Called internally when 'z' key is pressed
stInputMessage msg;
PBWinSimInput("z", PB_ON, &msg);
g_PBEngine.m_inputQueue.push(msg);

// Results in left flipper input message based on simMapKey in definitions
```

---

## Typical I/O Processing Flow

### Input Flow
```
Hardware Input → Debounce → State Change? → Generate Input Message
                                 │
                                 └──→ Auto Output? → Generate Output Message
                                 │
                                 └──→ Push to Input Queue
```

### Output Flow
```
Game Code → SendOutputMsg() → Push to Output Queue
                                     │
                                     ▼
                                 Dequeue Message
                                     │
                                     ├──→ Pulse Mode? → Track in Pulse Map
                                     │
                                     ├──→ LED Sequence Active? → Defer Message
                                     │
                                     └──→ Stage/Send to Hardware
```

### Complete Loop Example
```cpp
int main(int argc, char const *argv[]) {
    // Initialize
    g_PBEngine.pbeSetupIO();
    g_PBEngine.SetAutoOutputEnable(true);
    
    while (true) {
        // 1. Process all I/O (inputs and outputs)
        PBProcessIO();
        
        // 2. Handle input messages
        while (!g_PBEngine.m_inputQueue.empty()) {
            stInputMessage msg = g_PBEngine.m_inputQueue.front();
            g_PBEngine.m_inputQueue.pop();
            
            // Process input based on game state
            switch (msg.inputMsg) {
                case PB_IMSG_ROLLOVER_LANE_1:
                    if (msg.inputState == PB_ON) {
                        AddScore(1000);
                        // Turn on lane LED
                        g_PBEngine.SendOutputMsg(PB_OMSG_LED, 
                                                LED_LANE_1, 
                                                PB_ON, false);
                    }
                    break;
                    
                case PB_IMSG_SLINGSHOT_LEFT:
                    if (msg.inputState == PB_ON) {
                        AddScore(100);
                        // Fire slingshot (pulse mode)
                        g_PBEngine.SendOutputMsg(PB_OMSG_GENERIC_IO, 
                                                OUTPUT_SLINGSHOT_LEFT, 
                                                PB_ON, true);
                    }
                    break;
            }
        }
        
        // 3. Render
        // 4. Other game logic
    }
}
```

---

## Hardware Efficiency Notes

### Staged Writes

The I/O system uses "staged writes" for efficiency:
- Changes are accumulated in memory
- Hardware is updated only when values actually change
- Reduces I2C bus traffic significantly

```cpp
// Multiple staging calls
g_PBEngine.m_LEDChip[0].StageLEDControl(false, 0, LEDOn);
g_PBEngine.m_LEDChip[0].StageLEDControl(false, 1, LEDOn);
g_PBEngine.m_LEDChip[0].StageLEDControl(false, 2, LEDOff);

// Single hardware write with all changes
g_PBEngine.m_LEDChip[0].SendStagedLED();  // Called by PBProcessOutput()
```

### Debouncing

All inputs are automatically debounced to prevent switch bounce:
- Uses debounce time window (typically 10-50ms)
- Only generates messages on stable state changes
- Prevents multiple messages from single physical event

---

## See Also

- **[PBEngine_API.md](PBEngine_API.md)** - Core engine class, timer system, and message queue management
- **[LED_Control_API.md](LED_Control_API.md)** - LED sequence, NeoPixel control, and animation
- **[Platform_Init_API.md](Platform_Init_API.md)** - Hardware initialization
- **[RasPin_Overview.md](RasPin_Overview.md)** - Overall framework architecture

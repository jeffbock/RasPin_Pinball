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
```cpp
// In Pinball_IO.cpp
stInputDef g_inputDef[NUM_INPUTS] = {
    // id, inputMsg, boardType, boardIndex, pin, simMapKey
    {INPUT_LEFT_FLIPPER, PB_IMSG_LEFT_FLIPPER, PB_IO, 0, 0, "z"},
    {INPUT_RIGHT_FLIPPER, PB_IMSG_RIGHT_FLIPPER, PB_IO, 0, 1, "/"},
    {INPUT_START_BUTTON, PB_IMSG_START_BUTTON, PB_RASPI, 0, 17, "1"},
};
```

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
```cpp
// In Pinball_IO.cpp
stOutputDef g_outputDef[NUM_OUTPUTS] = {
    // id, outputMsg, boardType, boardIndex, pin, onTimeMS, offTimeMS
    {OUTPUT_LEFT_SLINGSHOT, PB_OMSG_GENERIC_IO, PB_IO, 0, 8, 50, 100},
    //                                                      ^^  ^^^
    //                                                      ON  OFF duration
};
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

### stInputDef

Defines an input's hardware connection and behavior.

```cpp
struct stInputDef {
    unsigned int id;                // Unique input ID
    PBInputMsg inputMsg;            // Message type to generate
    PBBoardType boardType;          // PB_RASPI, PB_IO, etc.
    unsigned int boardIndex;        // Which chip (for I/O expanders)
    unsigned int pin;               // Hardware pin number
    std::string simMapKey;          // Keyboard key for Windows simulation
    bool autoOutput;                // Enable auto output
    unsigned int autoOutputId;      // Output to trigger
    PBPinState autoPinState;        // State to send to output
    PBPinState lastState;           // Last known state
    unsigned long lastStateTick;    // Timestamp of last state change
};
```

### stOutputDef

Defines an output's hardware connection and pulse timing.

```cpp
struct stOutputDef {
    unsigned int id;                // Unique output ID
    PBOutputMsg outputMsg;          // Message type
    PBBoardType boardType;          // PB_RASPI, PB_IO, PB_LED
    unsigned int boardIndex;        // Which chip
    unsigned int pin;               // Hardware pin number
    unsigned int onTimeMS;          // Pulse ON duration
    unsigned int offTimeMS;         // Pulse OFF duration
    PBPinState lastState;           // Last known state
};
```

**Example Definitions:**
```cpp
// Define outputs in Pinball_IO.cpp
stOutputDef g_outputDef[NUM_OUTPUTS] = {
    // Solenoid with 50ms pulse
    {OUTPUT_LEFT_SLINGSHOT, PB_OMSG_GENERIC_IO, PB_IO, 0, 8, 50, 100},
    
    // LED output (no pulse timing needed)
    {LED_ROLLOVER_1, PB_OMSG_LED, PB_LED, 0, 3, 0, 0},
    
    // Flipper (no pulse - controlled by input)
    {OUTPUT_LEFT_FLIPPER, PB_OMSG_GENERIC_IO, PB_IO, 0, 10, 0, 0},
};
```

---

## Auto Output System

Auto output allows an input to directly trigger an output without game code processing. This is essential for responsive feedback like flippers.

### Configuration

1. **Define Auto Output in Input Definition:**
```cpp
stInputDef g_inputDef[NUM_INPUTS] = {
    // id, inputMsg, boardType, boardIndex, pin, simMapKey, 
    // autoOutput, autoOutputId, autoPinState
    {INPUT_LEFT_FLIPPER, PB_IMSG_LEFT_FLIPPER, PB_IO, 0, 0, "z", 
     true, OUTPUT_LEFT_FLIPPER, PB_ON},
     // ^^^^  ^^^^^^^^^^^^^^^^^^^  ^^^^^
     // Enable, Output to trigger, State to send
};
```

2. **Enable Auto Output Globally:**
```cpp
g_PBEngine.SetAutoOutputEnable(true);
```

### Behavior

When an input with auto output enabled changes state:
1. Normal input message is generated and queued
2. **Immediately** (before game code processes), output message is sent
3. Output uses the `autoPinState` (typically `PB_ON` for direct pass-through)

**Example - Flipper Setup:**
```cpp
// Input definition
{INPUT_LEFT_FLIPPER, PB_IMSG_LEFT_FLIPPER, PB_IO, 0, 0, "z", 
 true, OUTPUT_LEFT_FLIPPER, PB_ON},

// Output definition  
{OUTPUT_LEFT_FLIPPER, PB_OMSG_GENERIC_IO, PB_IO, 1, 5, 0, 0},

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

- **PBEngine_API.md** - Core engine class and message queue management
- **LED_Control_API.md** - LED sequence and animation control
- **Platform_Init_API.md** - Hardware initialization
- **UsersGuide.md** - Complete framework guide

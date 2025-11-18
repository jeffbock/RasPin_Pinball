# LED Control and Sequences API Reference

## Overview

The LED Control system provides advanced LED management including on/off control, brightness, group dimming/blinking, and pre-programmed animation sequences. The sequence system allows complex LED patterns to run autonomously without game code intervention.

## Key Features

- **Individual LED Control**: Turn LEDs on/off with brightness control
- **Group Operations**: Dimming and blinking across entire LED chips
- **Sequences**: Pre-programmed LED animation patterns
- **Loop Modes**: No-loop, continuous loop, ping-pong patterns
- **Deferred Messaging**: Queues LED changes during sequences
- **Automatic State Restore**: Returns LEDs to previous state after sequences

---

## Basic LED Control

### Turn LED On/Off

**Example:**
```cpp
// Turn on an LED
g_PBEngine.SendOutputMsg(PB_OMSG_LED, LED_ROLLOVER_1, PB_ON, false);

// Turn off an LED
g_PBEngine.SendOutputMsg(PB_OMSG_LED, LED_ROLLOVER_1, PB_OFF, false);
```

### Set LED Brightness

Control individual LED brightness from 0 (off) to 255 (full brightness).

**Example:**
```cpp
stOutputOptions options;
options.brightness = 128;  // 50% brightness

g_PBEngine.SendOutputMsg(PB_OMSG_LEDSET_BRIGHTNESS, LED_BACKGLASS, 
                         PB_ON, false, &options);
```

**Brightness Levels:**
- `0` - Off
- `64` - 25% brightness
- `128` - 50% brightness
- `192` - 75% brightness
- `255` - 100% brightness (full on)

---

## Group Control

Group control affects all LEDs on a single LED driver chip simultaneously.

### Group Dimming

Sets the brightness level for all LEDs on a chip.

**Example:**
```cpp
stOutputOptions options;
options.brightness = 100;  // ~40% brightness for entire chip

g_PBEngine.SendOutputMsg(PB_OMSG_LEDCFG_GROUPDIM, LED_ANY_ON_CHIP_0, 
                         PB_ON, false, &options);
```

### Group Blinking

Makes all LEDs on a chip blink at a specified rate.

**Example:**
```cpp
stOutputOptions options;
options.onBlinkMS = 500;   // On for 500ms
options.offBlinkMS = 500;  // Off for 500ms

g_PBEngine.SendOutputMsg(PB_OMSG_LEDCFG_GROUPBLINK, LED_ANY_ON_CHIP_0, 
                         PB_ON, false, &options);
```

**Note:** Group blink affects the entire chip. Individual LED controls still work but will follow the blink pattern.

---

## LED Sequences

Sequences are pre-programmed LED animation patterns that run autonomously. They're perfect for attract modes, jackpot celebrations, or any repeating pattern.

### Sequence Structure

#### LEDSequence

The main sequence container, defined as a compile-time constant.

```cpp
typedef const stLEDSequenceData LEDSequence;

struct stLEDSequenceData {
    const stLEDSequence* steps;   // Array of sequence steps
    int stepCount;                // Number of steps
};
```

#### stLEDSequence

Individual step in a sequence.

```cpp
struct stLEDSequence {
    uint16_t LEDOnBits[NUM_LED_CHIPS];  // LED state bitmask per chip
    unsigned int onDurationMS;          // How long to show this pattern
    unsigned int offDurationMS;         // Optional off time after pattern
};
```

### Creating a Sequence

**Example - Chase Pattern:**
```cpp
// Define the steps (in Pinball_IO.cpp or similar)
const stLEDSequence g_ChaseSteps[] = {
    // LEDOnBits (chip0, chip1, chip2), onDuration, offDuration
    {{0x0001, 0x0000, 0x0000}, 100, 0},  // LED 0 on
    {{0x0002, 0x0000, 0x0000}, 100, 0},  // LED 1 on
    {{0x0004, 0x0000, 0x0000}, 100, 0},  // LED 2 on
    {{0x0008, 0x0000, 0x0000}, 100, 0},  // LED 3 on
    {{0x0010, 0x0000, 0x0000}, 100, 0},  // LED 4 on
};

// Create the sequence
const stLEDSequenceData g_ChaseSeqData = {
    g_ChaseSteps,
    5  // Number of steps
};

const LEDSequence* g_ChaseSequence = &g_ChaseSeqData;
```

**Example - Multiple LEDs:**
```cpp
const stLEDSequence g_FlashSteps[] = {
    // All on
    {{0xFFFF, 0xFFFF, 0x0000}, 200, 0},
    // All off
    {{0x0000, 0x0000, 0x0000}, 200, 0},
};

const stLEDSequenceData g_FlashSeqData = {g_FlashSteps, 2};
const LEDSequence* g_FlashSequence = &g_FlashSeqData;
```

### Starting a Sequence

Use `SendSeqMsg()` to start a sequence with specified LEDs.

**Signature:**
```cpp
void SendSeqMsg(const LEDSequence* sequence, const uint16_t* mask, 
                PBSequenceLoopMode loopMode, PBPinState outputState);
```

**Parameters:**
- `sequence` - Pointer to sequence data
- `mask` - Array of bitmasks specifying which LEDs participate
- `loopMode` - How the sequence repeats
- `outputState` - `PB_ON` to start, `PB_OFF` to stop

**Example:**
```cpp
// Define which LEDs participate (all LEDs on chip 0)
uint16_t ledMask[NUM_LED_CHIPS] = {
    0xFFFF,  // All 16 LEDs on chip 0
    0x0000,  // No LEDs on chip 1
    0x0000   // No LEDs on chip 2
};

// Start the sequence with continuous loop
g_PBEngine.SendSeqMsg(g_ChaseSequence, ledMask, PB_LOOP, PB_ON);
```

**Selective LED Participation:**
```cpp
// Only LEDs 0-7 on chip 0
uint16_t selectiveMask[NUM_LED_CHIPS] = {
    0x00FF,  // LEDs 0-7 on chip 0
    0x0000,
    0x0000
};

g_PBEngine.SendSeqMsg(g_ChaseSequence, selectiveMask, PB_LOOP, PB_ON);
```

### Stopping a Sequence

```cpp
// Stop the current sequence
g_PBEngine.SendSeqMsg(nullptr, nullptr, PB_NOLOOP, PB_OFF);
```

When stopped, LEDs automatically return to their state before the sequence started.

### Loop Modes

**`PB_NOLOOP`**
- Plays sequence once and stops
- LEDs restore to previous state

```cpp
g_PBEngine.SendSeqMsg(g_JackpotSequence, mask, PB_NOLOOP, PB_ON);
// Plays once, then restores LED states
```

**`PB_LOOP`**
- Continuously repeats from start
- Plays: Step 0 → 1 → 2 → 3 → 0 → 1 → ...

```cpp
g_PBEngine.SendSeqMsg(g_AttractSequence, mask, PB_LOOP, PB_ON);
// Loops forever until stopped
```

**`PB_PINGPONG`**
- Plays forward then backward, then stops
- Plays: Step 0 → 1 → 2 → 3 → 2 → 1 → 0 [stops]

```cpp
g_PBEngine.SendSeqMsg(g_StartupSequence, mask, PB_PINGPONG, PB_ON);
// Plays forward and back once, then stops
```

**`PB_PINGPONGLOOP`**
- Continuously plays forward and backward
- Plays: 0 → 1 → 2 → 3 → 2 → 1 → 0 → 1 → 2 → ...

```cpp
g_PBEngine.SendSeqMsg(g_IdleSequence, mask, PB_PINGPONGLOOP, PB_ON);
// Bounces back and forth continuously
```

---

## Sequence Internals

### ProcessLEDSequenceMessage()

Handles starting and stopping sequences (called internally).

**Signature:**
```cpp
void ProcessLEDSequenceMessage(const stOutputMessage& message);
```

**Start Behavior:**
- Saves current LED states for all active LEDs
- Clears any active pulses for sequence LEDs
- Turns off all participating LEDs
- Initializes timing and index tracking
- Enables sequence mode

**Stop Behavior:**
- Disables sequence mode
- Restores saved LED states
- Processes deferred LED messages

### ProcessActiveLEDSequence()

Advances the sequence through its steps (called every frame).

**Signature:**
```cpp
void ProcessActiveLEDSequence();
```

**Behavior:**
- Tracks elapsed time for current step
- Applies LED states when step changes
- Advances to next step based on timing
- Handles loop boundaries
- Only updates hardware when state changes (efficient)

### HandleLEDSequenceBoundaries()

Manages sequence boundaries and loop transitions.

**Signature:**
```cpp
void HandleLEDSequenceBoundaries();
```

**Handles:**
- End of sequence (index >= stepCount)
- Beginning of sequence (index < 0)
- Loop mode transitions
- Direction changes for ping-pong modes

### EndLEDSequence()

Stops sequence and restores LED states.

**Signature:**
```cpp
void EndLEDSequence();
```

**Actions:**
- Disables sequence mode
- Restores saved LED control values for active LEDs only
- Updates output definitions with restored states

---

## Deferred LED Messages

During an active sequence, LED messages for participating LEDs are automatically deferred to avoid conflicts.

### Deferred Message Queue

Messages sent to LEDs participating in a sequence are queued instead of processed immediately.

```cpp
std::queue<stOutputMessage> m_deferredQueue;
```

**Maximum Queue Size:** 100 messages (`MAX_DEFERRED_LED_QUEUE`)

### Behavior

**While Sequence Active:**
```cpp
// This LED is in active sequence
g_PBEngine.SendOutputMsg(PB_OMSG_LED, LED_ROLLOVER_1, PB_ON, false);
// → Message is queued, not processed
```

**After Sequence Stops:**
```cpp
g_PBEngine.SendSeqMsg(nullptr, nullptr, PB_NOLOOP, PB_OFF);
// → Deferred messages are processed in order
// → Your LED_ROLLOVER_1 ON message now takes effect
```

### ProcessDeferredLEDQueue()

Processes deferred messages when sequence ends.

**Signature:**
```cpp
void ProcessDeferredLEDQueue();
```

**Called automatically by:** `PBProcessOutput()` when no sequence is active

---

## Pulse Mode with LEDs

LEDs can also use pulse mode for timed effects.

**Example:**
```cpp
// Define LED output with pulse timing
stOutputDef g_outputDef[] = {
    // id, outputMsg, boardType, boardIndex, pin, onTimeMS, offTimeMS
    {LED_EXTRA_BALL, PB_OMSG_LED, PB_LED, 0, 10, 500, 100},
    //                                           ^^^  ^^^
    //                                           On for 500ms, off for 100ms
};

// Send with pulse enabled
g_PBEngine.SendOutputMsg(PB_OMSG_LED, LED_EXTRA_BALL, PB_ON, true);
// → LED will turn on for 500ms, then off for 100ms, then stop
```

**Important Notes:**
- **Pulse Protection:** When an LED is in pulse mode, subsequent messages to that LED are ignored until the pulse completes. This prevents interrupting the timed effect.
- **Sequence Cancellation:** Starting an LED sequence will cancel any active pulse on participating LEDs. The sequence takes priority over pulse mode.

**Use Cases:**
- Brief flash on target hit
- Ball lock indicator
- Bonus multiplier flash

---

## Complete Sequence Example

**Scenario:** Create a rainbow chase effect for backglass LEDs during attract mode.

**Step 1: Define Sequence Steps**
```cpp
// In Pinball_IO.cpp or your table file
const stLEDSequence g_RainbowChaseSteps[] = {
    // Step 0: LEDs 0-2 on
    {{0x0007, 0x0000, 0x0000}, 150, 0},
    
    // Step 1: LEDs 3-5 on
    {{0x0038, 0x0000, 0x0000}, 150, 0},
    
    // Step 2: LEDs 6-8 on
    {{0x01C0, 0x0000, 0x0000}, 150, 0},
    
    // Step 3: LEDs 9-11 on
    {{0x0E00, 0x0000, 0x0000}, 150, 0},
    
    // Step 4: All flash
    {{0x0FFF, 0x0000, 0x0000}, 200, 100},
};

const stLEDSequenceData g_RainbowChaseData = {
    g_RainbowChaseSteps,
    5
};

const LEDSequence* g_RainbowChaseSequence = &g_RainbowChaseData;
```

**Step 2: Start Sequence in Attract Mode**
```cpp
void StartAttractMode() {
    // Define participating LEDs (LEDs 0-11 on chip 0)
    uint16_t attractMask[NUM_LED_CHIPS] = {
        0x0FFF,  // LEDs 0-11
        0x0000,
        0x0000
    };
    
    // Start looping sequence
    g_PBEngine.SendSeqMsg(g_RainbowChaseSequence, attractMask, 
                          PB_LOOP, PB_ON);
}
```

**Step 3: Stop When Game Starts**
```cpp
void StartGame() {
    // Stop attract sequence
    g_PBEngine.SendSeqMsg(nullptr, nullptr, PB_NOLOOP, PB_OFF);
    // LEDs automatically restore to state before sequence
    
    // Now safe to control LEDs individually for gameplay
}
```

---

## Advanced Patterns

### Cascading Effect

```cpp
const stLEDSequence g_CascadeSteps[] = {
    {{0x8000, 0x0000, 0x0000}, 50, 0},  // LED 15
    {{0xC000, 0x0000, 0x0000}, 50, 0},  // LEDs 14-15
    {{0xE000, 0x0000, 0x0000}, 50, 0},  // LEDs 13-15
    {{0xF000, 0x0000, 0x0000}, 50, 0},  // LEDs 12-15
    {{0xF800, 0x0000, 0x0000}, 50, 0},  // LEDs 11-15
    // ... continue pattern
};
```

### Alternating Pattern

```cpp
const stLEDSequence g_AlternateSteps[] = {
    {{0x5555, 0x0000, 0x0000}, 200, 0},  // Odd LEDs (binary: 0101010101010101)
    {{0xAAAA, 0x0000, 0x0000}, 200, 0},  // Even LEDs (binary: 1010101010101010)
};
```

### Multi-Chip Coordination

```cpp
const stLEDSequence g_MultiChipSteps[] = {
    // All LEDs on chip 0
    {{0xFFFF, 0x0000, 0x0000}, 200, 0},
    // All LEDs on chip 1
    {{0x0000, 0xFFFF, 0x0000}, 200, 0},
    // All LEDs on chip 2
    {{0x0000, 0x0000, 0xFFFF}, 200, 0},
    // All LEDs on all chips
    {{0xFFFF, 0xFFFF, 0xFFFF}, 200, 0},
};
```

---

## Best Practices

### Performance

- **Minimize Step Count**: Keep sequences concise for memory efficiency
- **Appropriate Timing**: Use durations >= 50ms for visible patterns
- **Selective Masks**: Only include necessary LEDs in sequence mask

### Design

- **Plan State Restore**: Consider what state LEDs should return to
- **Avoid Conflicts**: Don't manually control LEDs in active sequence
- **Test Loop Modes**: Verify ping-pong and loop behavior match intent

### Debugging

- **Check Masks**: Verify bitmasks match your LED layout
- **Monitor Queue**: Watch deferred queue size during sequences
- **Enable Overlay**: Use `g_PBEngine.m_EnableOverlay = true` to see I/O state

---

## LED Bitmask Reference

Bitmask values for common LED patterns:

```cpp
// Single LEDs
0x0001  // LED 0
0x0002  // LED 1
0x0004  // LED 2
0x0008  // LED 3
// ...
0x8000  // LED 15

// Groups
0x000F  // LEDs 0-3
0x00FF  // LEDs 0-7
0x0FFF  // LEDs 0-11
0xFFFF  // All 16 LEDs

// Patterns
0x5555  // Odd LEDs (binary: 0101010101010101)
0xAAAA  // Even LEDs (binary: 1010101010101010)
0xF0F0  // Alternating groups of 4
```

### Bitmask Calculation Helper

```cpp
// Turn on LED at specific position
uint16_t ledMask = (1 << ledNumber);

// Turn on range of LEDs (inclusive)
uint16_t rangeMask = 0;
for (int i = startLED; i <= endLED; i++) {
    rangeMask |= (1 << i);
}
```

---

## NeoPixel RGB LED Control

The NeoPixel system provides control for WS2812B-style addressable RGB LED strips using direct Raspberry Pi GPIO pins.

### Key Features

- **Direct GPIO Control**: Uses bit-banging for precise timing
- **RGB Color Control**: Full 24-bit color per LED (8 bits per channel)
- **Single LED or Array Updates**: Update one LED or entire strip at once
- **Sequence Support**: Pre-programmed color animation patterns
- **Automatic Staging**: Only sends changes to minimize data transmission

### Basic NeoPixel Control

#### Set Single LED Color

**Example:**
```cpp
// Set LED 0 to red (RGB: 255, 0, 0)
stOutputOptions options;
options.neoPixelRed = 255;
options.neoPixelGreen = 0;
options.neoPixelBlue = 0;

g_PBEngine.SendOutputMsg(PB_OMSG_NEOPIXEL, IDO_NEOPIXEL0, PB_ON, false, &options);
```

#### Set Multiple LEDs (Array Update)

**Example:**
```cpp
// Create array of LED colors
stNeoPixelNode ledArray[10];
for (int i = 0; i < 10; i++) {
    ledArray[i].red = 255;
    ledArray[i].green = i * 25;  // Gradient effect
    ledArray[i].blue = 0;
}

// Send array to driver
stOutputOptions options;
options.neoPixelArray = ledArray;
options.neoPixelArrayCount = 10;

g_PBEngine.SendOutputMsg(PB_OMSG_NEOPIXEL, IDO_NEOPIXEL0, PB_ON, false, &options);
```

### NeoPixel Sequences

Create animated patterns that run autonomously.

#### Define a Sequence

**Example:**
```cpp
// Define color patterns
stNeoPixelNode redPattern[10];
stNeoPixelNode bluePattern[10];
stNeoPixelNode offPattern[10];

// Initialize patterns
for (int i = 0; i < 10; i++) {
    redPattern[i] = {255, 0, 0};    // Red
    bluePattern[i] = {0, 0, 255};   // Blue
    offPattern[i] = {0, 0, 0};      // Off
}

// Define sequence steps
stNeoPixelSequence colorSequence[] = {
    {redPattern, 500},    // Red for 500ms
    {bluePattern, 500},   // Blue for 500ms
    {offPattern, 200}     // Off for 200ms
};

// Create sequence data structure
stNeoPixelSequenceData mySequence = {
    colorSequence,
    3  // Number of steps
};
```

#### Start a Sequence

**Example:**
```cpp
stOutputOptions options;
options.setNeoPixelSequence = &mySequence;
options.loopMode = PB_LOOP;  // Continuous loop

g_PBEngine.SendOutputMsg(PB_OMSG_NEOPIXEL_SEQUENCE, IDO_NEOPIXEL0, 
                         PB_ON, false, &options);
```

#### Stop a Sequence

**Example:**
```cpp
g_PBEngine.SendOutputMsg(PB_OMSG_NEOPIXEL_SEQUENCE, IDO_NEOPIXEL0, 
                         PB_OFF, false);
```

### Configuration

#### Hardware Setup

Edit `PBEngine` constructor in `Pinball_Engine.cpp`:

```cpp
// Configure NeoPixel driver
NeoPixelDriver m_NeoPixelDriver[NUM_NEOPIXEL_DRIVERS] = {
    NeoPixelDriver(18, 30)  // GPIO 18, 30 LEDs
};
```

#### Output Definition

Add to `g_outputDef[]` in `Pinball_IO.cpp`:

```cpp
{"NeoPixel LED0", PB_OMSG_NEOPIXEL, IDO_NEOPIXEL0, 0, PB_NEOPIXEL, 0, PB_OFF, 0, 0}
```

Where:
- `IDO_NEOPIXEL0` - Unique output ID
- `0` - LED index in the strip (pin field)
- `PB_NEOPIXEL` - Board type
- `0` - Driver index (first driver)

### Important Notes

1. **Timing Critical**: NeoPixel communication requires precise timing and disables interrupts briefly
2. **GRB Order**: WS2812B uses GRB color order internally (handled automatically)
3. **No Direct Control in Test Mode**: NeoPixel outputs show "N/A" in diagnostics/test modes
4. **Sequence Exclusive**: During sequences, direct LED messages are ignored
5. **GPIO Only**: NeoPixels use direct GPIO pins, not I2C like LED drivers
6. **CPU Frequency Independent**: Uses `clock_gettime()` with nanosecond precision instead of CPU-dependent instruction counting
7. **LED Count Limits**: 
   - **Recommended maximum: 60 LEDs per driver** (1.8ms interrupt disable)
   - **Absolute maximum: 100 LEDs per driver** (3ms interrupt disable)
   - Exceeding limits may cause USB packet drops, audio glitches, or network issues
   - Use multiple drivers for larger installations

### Timing Parameters

WS2812B timing (automatically handled using `clock_gettime()` with `CLOCK_MONOTONIC`):
- Bit 1: 0.8µs high, 0.45µs low
- Bit 0: 0.4µs high, 0.85µs low  
- Reset: 60µs low signal
- Timing is CPU frequency independent and robust to system load changes

### Interrupt Disable Duration

The implementation disables interrupts during LED data transmission for timing accuracy:

| LED Count | Interrupt Disable Time | Impact Level |
|-----------|------------------------|--------------|
| 10 LEDs   | 0.3ms | Minimal - safe for all use cases |
| 30 LEDs   | 0.9ms | Low - safe for most use cases |
| 60 LEDs   | 1.8ms | **Recommended maximum** - generally safe |
| 100 LEDs  | 3.0ms | **Absolute maximum** - may cause occasional issues |
| 300 LEDs  | 9.0ms | Not recommended - will cause system problems |

**Why interrupt disable is necessary**: The WS2812B protocol requires sub-microsecond timing precision. Context switching during transmission would corrupt the data stream. Disabling interrupts is the simplest and most reliable approach for userspace GPIO control.

**Alternative approaches considered**:
- RT_PREEMPT kernel: Overly complex for this use case
- DMA/PWM hardware: Hardware-specific, less flexible
- Higher-priority scheduling: Insufficient timing guarantee

---

## See Also

- **PBEngine_API.md** - Core engine and SendOutputMsg() details
- **IO_Processing_API.md** - Output message processing
- **UsersGuide.md** - Complete framework guide with hardware details

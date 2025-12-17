// Pinball_IO.h:  Header file to define all the input sensors and buttons in the pinball machine
// These need to be defined by the user for the specific table

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#ifndef Pinball_IO_h
#define Pinball_IO_h

#include <string>
#include <cstdint>

#define PB_I2C_AMPLIFIER 0x4B

// Geneic IO definitions
// PB_Blink and PB_Brightness added for LED driver support - they are not valid for inputs or other outputs
enum PBPinState {
    PB_ON = 0,
    PB_OFF = 1,
    PB_BLINK = 2,
    PB_BRIGHTNESS = 3
};

// NeoPixel index constant for all pixels
#define ALLNEOPIXELS 9999

// RGB LED Color definitions
enum PBLEDColor {
    PB_LEDBLACK = 0,      // Red only
    PB_LEDRED = 1,      // Red only
    PB_LEDGREEN = 2,    // Green only  
    PB_LEDBLUE = 3,     // Blue only
    PB_LEDWHITE = 4,    // Red + Green + Blue
    PB_LEDPURPLE = 5,   // Red + Blue (Magenta)
    PB_LEDYELLOW = 6,   // Red + Green
    PB_LEDCYAN = 7      // Green + Blue
};

enum PBBoardType {
    PB_RASPI = 0,
    PB_IO = 1,
    PB_LED = 2,
    PB_NEOPIXEL = 3,
    PB_NOBOARD
};

// Input message structs and types
enum PBInputMsg {
    PB_IMSG_EMPTY = 0,
    PB_IMSG_SENSOR = 1,
    PB_IMSG_TARGET = 2,
    PB_IMSG_JETBUMPER = 3,
    PB_IMSG_POPBUMPER = 4,
    PB_IMSG_BUTTON = 5,
    PB_IMSG_TIMER = 6,
};

struct stInputDef{
    std::string inputName; 
    std::string simMapKey;
    PBInputMsg inputMsg; 
    unsigned int id;
    unsigned int pin;  // GPIO pin number, or the pin index for IODriver Chips
    PBBoardType boardType;
    unsigned int boardIndex;
    PBPinState lastState;
    unsigned long lastStateTick;
    unsigned long debounceTimeMS;
    bool autoOutput;
    unsigned int autoOutputId;
    PBPinState autoPinState;
};

// Output defintions
// Input message structs and types
enum PBOutputMsg {
    PB_OMSG_LED = 1,
    PB_OMSG_LEDCFG_GROUPDIM = 2,
    PB_OMSG_LEDCFG_GROUPBLINK = 3,
    PB_OMSG_LEDSET_BRIGHTNESS = 4,
    PB_OMSG_LED_SEQUENCE = 5,
    PB_OMSG_GENERIC_IO = 6,
    PB_OMSG_NEOPIXEL = 7,
    PB_OMSG_NEOPIXEL_SEQUENCE = 8
};

// Placeholder - there are probably items to add here
struct stOutputDef{
    std::string outputName; 
    PBOutputMsg outputMsg; 
    unsigned int id;
    unsigned int pin; // GPIO pin number, or the pin index for IODriver and LED Chips
    PBBoardType boardType;
    unsigned int boardIndex;
    PBPinState lastState;
    unsigned int onTimeMS;
    unsigned int offTimeMS;
    unsigned int neoPixelIndex;  // Index of specific NeoPixel LED in chain (for single pixel operations)
};

// The actual definition of the input items - the user of the library will need to define these for the specific table
// The IDs need to be unique, and will be checked during initilization - if there are duplicates, the engine will not start

// The actual definition of the output items - the user of the library will need to define these for the specific table

#define IDO_SLINGSHOT 0
#define IDO_POPBUMPER 1
#define IDO_LED1 2
#define IDO_BALLEJECT 3
#define IDO_LED2 4
#define IDO_LED3 5
#define IDO_LED4 6
#define IDO_LED5 7
#define IDO_LED6 8
#define IDO_LED7 9
#define IDO_LED8 10
#define IDO_LED9 11
#define IDO_LED10 12
#define IDO_BALLEJECT2 13
#define IDO_NEOPIXEL0 14
#define IDO_NEOPIXEL1 15
#define NUM_OUTPUTS 15

#define IDI_LEFTFLIPPER 0
#define IDI_RIGHTFLIPPER 1
#define IDI_LEFTACTIVATE 2
#define IDI_RIGHTACTIVATE 3
#define IDI_START 4
#define IDI_RESET 5
#define IDI_SENSOR1 6
#define IDI_SENSOR2 7
#define IDI_SENSOR3 8
#define NUM_INPUTS 9

// Declare the shared variables for input / output structures.
extern stInputDef g_inputDef[];
extern stOutputDef g_outputDef[];

// TLC59116 Register Definitions
#define TLC59116_MODE1      0x00
#define TLC59116_MODE2      0x01
#define TLC59116_PWM0       0x02
#define TLC59116_PWM15      0x11
#define TLC59116_GRPPWM     0x12
#define TLC59116_GRPFREQ    0x13
#define TLC59116_LEDOUT0    0x14
#define TLC59116_LEDOUT1    0x15
#define TLC59116_LEDOUT2    0x16
#define TLC59116_LEDOUT3    0x17

// TLC59116 Constants
#define TLC59116_AUTO_INCREMENT 0x80
#define TLC59116_RESET_VAL      0x00
#define TLC59116_MODE1_NORMAL   0x00
#define TLC59116_MODE2_DMBLNK   0x20

// TLC5916 Addresses
#define PB_ADD_LED0 0x60
#define PB_ADD_LED1 0x61
#define PB_ADD_LED2 0x62

// Simple LED Driver class for controlling a single tlc59116 chip
enum LEDState {
    LEDOn,
    LEDOff,
    LEDDimming,
    LEDGroup
};

enum LEDGroupMode {
    GroupModeDimming,   // Group mode set to dimming only
    GroupModeBlinking   // Group mode set to blinking
};

enum LEDHardwareState {
    StagedHW,          // Read from staged values (m_ledControl)
    CurrentHW          // Read from current hardware state (m_currentControl)
};

// Max Volume for MAX9744 IC
// Max is 0x3F but that is so sensitive, even small changes can be very loud.
#define MAX9744_VOLUME_MAX 0x26
#define MAX9744_VOLUME_MIN 0x0A

// LED Driver class for controlling a single tlc59116 chip
class LEDDriver {
public:
    LEDDriver(uint8_t address);
    ~LEDDriver();

    void SetGroupMode(LEDGroupMode groupMode, unsigned int brightness, unsigned int msTimeOn, unsigned int msTimeOff);
    void StageLEDControl(bool setAll, unsigned int LEDIndex, LEDState state);
    void StageLEDControl(unsigned int registerIndex, uint8_t value);
    void SyncStagedWithHardware(unsigned int registerIndex);  // Sync m_ledControl with m_currentControl
    void StageLEDBrightness(bool setAll, unsigned int LEDIndex, uint8_t brightness);
    void SendStagedLED();
    LEDGroupMode GetGroupMode() const;
    bool HasStagedChanges() const;
    uint8_t GetAddress() const;                               // Get I2C address

    // Register reading functions
    uint8_t ReadModeRegister(uint8_t modeRegister) const;     // Read MODE1 or MODE2
    uint8_t ReadPWMRegister(uint8_t pwmIndex) const;          // Read PWM0-PWM15 (pwmIndex 0-15)
    uint8_t ReadLEDOutRegister(uint8_t ledOutIndex) const;    // Read LEDOUT0-LEDOUT3 (ledOutIndex 0-3)
    uint8_t ReadGroupPWM() const;                             // Read GRPPWM register
    uint8_t ReadGroupFreq() const;                            // Read GRPFREQ register
    uint8_t ReadLEDControl(LEDHardwareState hwState, uint8_t registerIndex) const;  // Read staged or current control register
    uint8_t ReadLEDBrightness(LEDHardwareState hwState, uint8_t ledIndex) const;   // Read staged or current brightness register
    
    // Helper function to convert 2-bit register value to LEDState for a specific pin
    LEDState GetLEDStateFromVal(uint8_t regValue, unsigned int pin) const;

private:
    uint8_t m_address;
    uint8_t m_ledBrightness[16];
    uint8_t m_ledControl[4];
    int m_i2cFd;
    LEDGroupMode m_groupMode;  // Track current group mode state
    bool m_pwmStaged[16];      // Track which PWM registers (PWM0-PWM15) have changes
    bool m_ledOutStaged[4];    // Track which LEDOUT registers (LEDOUT0-LEDOUT3) have changes
    
    // Current state tracking for comparison (what was last sent to hardware)
    uint8_t m_currentBrightness[16];  // Last brightness values sent to PWM registers
    uint8_t m_currentControl[4];      // Last control values sent to LEDOUT registers
    
    // Helper function to convert LEDState to control value
    uint8_t GetControlValue(LEDState state) const;
};

// TCA9555 Register Definitions
#define TCA9555_INPUT_PORT0     0x00
#define TCA9555_INPUT_PORT1     0x01
#define TCA9555_OUTPUT_PORT0    0x02
#define TCA9555_OUTPUT_PORT1    0x03
#define TCA9555_POLARITY_PORT0  0x04
#define TCA9555_POLARITY_PORT1  0x05
#define TCA9555_CONFIG_PORT0    0x06
#define TCA9555_CONFIG_PORT1    0x07

// TCA9555 Addresses
#define PB_ADD_IO0              0x20
#define PB_ADD_IO1              0x21
#define PB_ADD_IO2              0x22

// Pin direction enum for IODriver
enum PBPinDirection {
    PB_OUTPUT = 0,  // Pin configured as output
    PB_INPUT = 1    // Pin configured as input
};

// Simple IO Driver class for controlling a single TCA9555 chip
class IODriver {
public:
    IODriver(uint8_t address, uint16_t inputMask);
    ~IODriver();

    void StageOutput(uint16_t value);  // 16-bit value for both ports
    void StageOutputPin(uint8_t pinIndex, PBPinState value);  // Set individual pin (0-15)
    void SendStagedOutput();
    uint16_t ReadInputs();
    bool HasStagedChanges() const;
    void ConfigurePin(uint8_t pinIndex, PBPinDirection direction);  
    uint8_t GetAddress() const;                               // Get I2C address

    // Register reading functions
    uint8_t ReadOutputPort(uint8_t portIndex) const;     // Read OUTPUT_PORT0 or OUTPUT_PORT1 (portIndex 0-1)
    uint8_t ReadPolarityPort(uint8_t portIndex) const;   // Read POLARITY_PORT0 or PORT1 (portIndex 0-1)
    uint8_t ReadConfigPort(uint8_t portIndex) const;     // Read CONFIG_PORT0 or PORT1 (portIndex 0-1)
    uint8_t ReadOutputValues(LEDHardwareState hwState, uint8_t portIndex) const;  // Read staged or current output values

private:
    uint8_t m_address;
    uint8_t m_outputValues[2];    // Output values for port 0 and port 1
    bool m_outputStaged[2];       // Track which output ports have staged changes
    int m_i2cFd;
    uint16_t m_inputMask;         // Bit mask indicating which pins are inputs (1=input, 0=output)
    
    // Current state tracking for comparison (what was last sent to hardware)
    uint8_t m_currentOutputValues[2];  // Last output values sent to OUTPUT_PORT registers
};

// Simple Amplifier Driver class for controlling a single MAX9744 chip
class AmpDriver {
public:
    AmpDriver(uint8_t address);
    ~AmpDriver();

    void SetVolume(uint8_t volumePercent);  // Set volume 0-100% (0 = mute)
    uint8_t GetAddress() const;             // Get I2C address
    bool IsConnected() const;               // Check if amplifier is responding

private:
    uint8_t m_address;
    int m_i2cFd;
    uint8_t m_currentVolume;               // Track current volume setting
    
    // Helper function to convert percentage to MAX9744 register value
    uint8_t PercentToRegisterValue(uint8_t percent) const;
};

// Debug flag to enable real-time scheduling priority for NeoPixel transmission
// Requires running with elevated privileges (sudo)
#define NEOPIXEL_USE_RT_PRIORITY 1  // Set to 1 to enable SCHED_FIFO priority

// NeoPixel timing method selection
enum NeoPixelTimingMethod {
    NEOPIXEL_TIMING_CLOCKGETTIME = 0,  // Use clock_gettime() for timing (current default)
    NEOPIXEL_TIMING_NOP = 1,           // Use assembly NOP instructions (more deterministic)
    NEOPIXEL_TIMING_SPI = 2,           // Use SPI hardware (most reliable, requires SPI channel)
    NEOPIXEL_TIMING_SPI_BURST = 3,     // Use SPI hardware with full string burst transmission
    NEOPIXEL_TIMING_DISABLED = 4       // Hardware initialization failed - NeoPixels disabled
};

// NeoPixel LED node structure - defines RGB color for a single LED
// This are used by the NeoPixelDriver and are pre-allocated during boot time
struct stNeoPixelNode {
    uint8_t currentRed;
    uint8_t currentGreen;
    uint8_t currentBlue;
    uint8_t currentBrightness;

    uint8_t stagedRed;
    uint8_t stagedGreen;
    uint8_t stagedBlue;
    uint8_t stagedBrightness;
};

// NeoPixel timing and safety constants
// Maximum recommended LEDs per driver to keep interrupt disable time < 2ms
// Each LED takes ~30µs to transmit (24 bits × 1.25µs)
// 60 LEDs = 1.8ms interrupt disable (safe for most systems)
#define NEOPIXEL_MAX_LEDS_RECOMMENDED 60
#define NEOPIXEL_MAX_LEDS_ABSOLUTE 100  // Absolute limit (3ms) - may cause issues

// NeoPixel timing instrumentation constants
#define NEOPIXEL_INSTRUMENTATION_BITS 32  // Track timing for 32 bits (4 bytes / 1 complete 32-bit value)
#define NEOPIXEL_MAX_CAPTURED_BYTES 4     // Maximum number of bytes captured (32 bits / 8)

// NeoPixel timing specification tolerances (in nanoseconds)
// SK6812 timing requirements with ±150ns tolerance:
// Bit 1: 600ns high (±150ns), 600ns low (±150ns)
// Bit 0: 300ns high (±150ns), 900ns low (±150ns)
// Reset: >80μs (more strict than WS2812B's >50μs)
#define NEOPIXEL_BIT1_HIGH_MIN_NS 450    // 600ns - 150ns tolerance
#define NEOPIXEL_BIT1_HIGH_MAX_NS 750    // 600ns + 150ns tolerance
#define NEOPIXEL_BIT1_LOW_MIN_NS 450     // 600ns - 150ns tolerance
#define NEOPIXEL_BIT1_LOW_MAX_NS 750     // 600ns + 150ns tolerance

#define NEOPIXEL_BIT0_HIGH_MIN_NS 150    // 300ns - 150ns tolerance
#define NEOPIXEL_BIT0_HIGH_MAX_NS 450    // 300ns + 150ns tolerance
#define NEOPIXEL_BIT0_LOW_MIN_NS 750     // 900ns - 150ns tolerance
#define NEOPIXEL_BIT0_LOW_MAX_NS 1050    // 900ns + 150ns tolerance

// Structure to track timing data for a single bit transmission
struct stNeoPixelBitTiming {
    uint32_t highTimeNs;      // Time pin was high in nanoseconds
    uint32_t lowTimeNs;       // Time pin was low in nanoseconds
    bool meetsSpec;           // True if timing meets SK6812 specification
    bool bitValue;            // The actual bit value sent (0 or 1)
};

// Structure to track timing instrumentation for SendByte operations
// Tracks up to 32 bits (4 bytes) of timing data for diagnostics
struct stNeoPixelInstrumentation {
    stNeoPixelBitTiming bitTimings[NEOPIXEL_INSTRUMENTATION_BITS];  // Timing data for each bit
    unsigned int numBitsCaptured;     // Number of bits captured (0-32)
    uint32_t capturedBytes;           // The actual byte values captured (for verification)
    bool instrumentationEnabled;      // Flag to enable/disable instrumentation
    unsigned int byteSequenceNumber;  // Sequential byte counter for tracking order
    uint64_t totalTransmissionTimeNs; // Total time for all captured bits
};

// NeoPixel Driver class for controlling SK6812-style RGB LED strips
// Uses Raspberry Pi GPIO pins directly for bit-banged communication
// IMPORTANT: Disables interrupts during transmission - limit LED count to avoid system issues
class NeoPixelDriver {
public:
    NeoPixelDriver(unsigned int driverIndex, unsigned char* spiBuffer);
    ~NeoPixelDriver();

    // Initialize GPIO pins (must be called after wiringPiSetup())
    void InitializeGPIO();

    // Stage a single LED color value
    void StageNeoPixel(unsigned int ledIndex, uint8_t red, uint8_t green, uint8_t blue);
    void StageNeoPixel(unsigned int ledIndex, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness);
    void StageNeoPixel(unsigned int ledIndex, const stNeoPixelNode& node);
    void StageNeoPixel(unsigned int ledIndex, const stNeoPixelNode& node, uint8_t brightness);
    void StageNeoPixel(unsigned int ledIndex, PBLEDColor color);
    void StageNeoPixel(unsigned int ledIndex, PBLEDColor color, uint8_t brightness);
    
    // Stage all LEDs in the chain to the same color
    void StageNeoPixelAll(uint8_t red, uint8_t green, uint8_t blue);
    void StageNeoPixelAll(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness);
    void StageNeoPixelAll(const stNeoPixelNode& node);
    void StageNeoPixelAll(const stNeoPixelNode& node, uint8_t brightness);
    void StageNeoPixelAll(PBLEDColor color);
    void StageNeoPixelAll(PBLEDColor color, uint8_t brightness);
    
    // Stage an array of LED values
    void StageNeoPixelArray(const stNeoPixelNode* nodeArray, unsigned int count);
    
    // Send staged values to the LED string (only if different from current hardware)
    void SendStagedNeoPixels();
    
    // Check if there are staged changes
    bool HasStagedChanges() const;
    
    // Set timing method (must be set before calling SendStagedNeoPixels)
    // Note: Once set to DISABLED, cannot be changed
    void SetTimingMethod(NeoPixelTimingMethod method);
    NeoPixelTimingMethod GetTimingMethod() const { return m_timingMethod; }
    
    // Get pin and LED count
    unsigned int GetOutputPin() const { return m_outputPin; }
    unsigned int GetNumLEDs() const { return m_numLEDs; }
    
    // Instrumentation control and access methods
    void EnableInstrumentation(bool enable);
    bool IsInstrumentationEnabled() const;
    void ResetInstrumentation();
    const stNeoPixelInstrumentation& GetInstrumentationData() const;
    
    // Brightness control
    void SetMaxBrightness(uint8_t maxBrightness);
    uint8_t GetMaxBrightness() const { return m_maxBrightness; }

private:
    unsigned int m_driverIndex;    // Driver index (corresponds to boardIndex)
    unsigned int m_outputPin;      // GPIO pin number
    unsigned int m_numLEDs;        // Number of LEDs in the string
    stNeoPixelNode* m_nodes;       // Pointer to node array (from g_NeoPixelNodeArray)
    bool m_hasChanges;             // Flag indicating staged changes exist
    bool m_gpioInitialized;        // Flag indicating GPIO has been initialized
    NeoPixelTimingMethod m_timingMethod;  // Timing method to use
    stNeoPixelInstrumentation m_instrumentation;  // Timing instrumentation data
    
    // SPI-specific members
    int m_spiChannel;              // SPI channel (0 or 1)
    int m_spiFd;                   // SPI file descriptor (-1 if not initialized)
    unsigned char* m_spiBuffer;    // Pre-allocated SPI buffer (passed from initialization)
    
    // Brightness control
    uint8_t m_maxBrightness;       // Maximum allowed brightness (0-255)
    
    // Helper function to convert PBLEDColor enum to RGB values
    void ColorToRGB(PBLEDColor color, uint8_t& red, uint8_t& green, uint8_t& blue);
    
    // Helper function to cap brightness at maximum
    inline uint8_t CapBrightness(uint8_t brightness) const {
        return (brightness > m_maxBrightness) ? m_maxBrightness : brightness;
    }
    
    // Helper function to apply brightness scaling to a color component
    // Optimized using bit shifting for better performance
    inline uint8_t ApplyBrightness(uint8_t color, uint8_t brightness) const {
        // Fast approximation: (color * brightness + 127) >> 8
        // Provides similar accuracy to division by 255 with faster execution
        return (uint8_t)((((uint16_t)color * brightness) + 127) >> 8);
    }
    
    // Helper function to send RGB data for a single LED using SK6812 timing
    void SendByte(uint8_t byte, NeoPixelTimingMethod method);
    
    // Helper functions for different timing methods
    void SendByteClockGetTime(uint8_t byte);
    void SendByteNOP(uint8_t byte);
    
    // Helper functions for SPI mode
    void InitializeSPI();
    void SendByteSPI(uint8_t byte);
    void SendAllPixelsSPI();  // Send entire pixel string in one burst
    void CloseSPI();
    
    // Helper function to check if bit timing meets SK6812 specification
    bool CheckBitTimingSpec(uint32_t highTimeNs, uint32_t lowTimeNs, bool bitValue) const;
    
    // Helper function to initialize instrumentation data structure
    void InitializeInstrumentationData();
    
    // Helper function to send reset/latch signal
    void SendReset();
};


#endif // Pinball_IO_h
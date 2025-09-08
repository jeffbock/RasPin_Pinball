// PInball_IO.h:  Header file to define all the input sensors and buttons in the pinball machine
// These need to be defined by the user for the specific table

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#ifndef PInball_IO_h
#define PInball_IO_h

#include <string>

#define PB_I2C_AMPLIFIER 0x4B

// Geneic IO definitions
// PB_Blink and PB_Brightness added for LED driver support - they are not valid for inputs or other outputs
enum PBPinState {
    PB_ON = 0,
    PB_OFF = 1,
    PB_BLINK = 2,
    PB_BRIGHTNESS = 3
};

enum PBBoardType {
    PB_RASPI = 0,
    PB_IO = 1,
    PB_LED = 2,
    PB_NOBOARD
};

// Input message structs and types
enum PBInputMsg {
    PB_IMSG_BUTTON = 0,
    PB_IMSG_SENSOR = 1,
    PB_IMSG_TARGET = 2,
    PB_IMSG_JETBUMPER = 3,
    PB_IMSG_POPBUMPER = 4,
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
    PB_OMSG_GENERIC_IO = 6
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
#define NUM_OUTPUTS 7

#define IDI_LEFTFLIPPER 0
#define IDI_RIGHTFLIPPER 1
#define IDI_LEFTACTIVATE 2
#define IDI_RIGHTACTIVATE 3
#define IDI_START 4
#define IDI_SENSOR1 5
#define IDI_SENSOR2 6
#define IDI_SENSOR3 7
#define NUM_INPUTS 8

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
#define MAX9744_VOLUME_MIN 0x14

// LED Driver class for controlling a single tlc59116 chip
class LEDDriver {
public:
    LEDDriver(uint8_t address);
    ~LEDDriver();

    void SetGroupMode(LEDGroupMode groupMode, unsigned int brightness, unsigned int msTimeOn, unsigned int msTimeOff);
    void StageLEDControl(bool setAll, unsigned int LEDIndex, LEDState state);
    void StageLEDControl(unsigned int registerIndex, uint8_t value);
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


#endif
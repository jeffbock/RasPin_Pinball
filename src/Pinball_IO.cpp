// Pinball_IO.cpp:  Shared input / output structures used the pinball engine
// These need to be defined by the user for the specific table

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

// NOTE:  This IO file is to be used with the basic Pinball breakout box setup, (see HW schematics /hw/SimpleBreakoutSchematic.png)
// All active connections use the GPIO pins on the Raspberry Pi directily, rather than I2C and expansion ICs

#include "Pinball_IO.h"
#include "Pinball_Engine.h"
#include "PBBuildSwitch.h"
#include <cstring>  // For memset in SPI buffer operations

#ifdef EXE_MODE_RASPI
#include "wiringPi.h"
#include "wiringPiI2C.h"
#include "wiringPiSPI.h"
#include <time.h>  // For clock_gettime with nanosecond precision
#endif

// NeoPixel SPI pin constants (Raspberry Pi GPIO numbers)
constexpr int SPI0_MOSI_PIN = 10;  // SPI0 MOSI (Physical Pin 19)
constexpr int SPI1_MOSI_PIN = 20;  // SPI1 MOSI (Physical Pin 38)

// Declare static arrays - initialized by functions below
static stOutputDef g_outputDef[NUM_OUTPUTS];
static stInputDef g_inputDef[NUM_INPUTS];

// Forward declare PBEngine for console output during initialization
class PBEngine;
extern PBEngine g_PBEngine;

// Initialize output definitions array
// This function must be called before using g_outputDef
void InitializeOutputDefs() {
    // Track which indices have been initialized to detect duplicates
    bool initialized[NUM_OUTPUTS] = {false};
    bool hasErrors = false;
    
    // Helper lambda to check and set an index
    auto setOutput = [&](int index, const char* name, PBOutputMsg msg, unsigned int pin, 
                         PBBoardType boardType, unsigned int boardIndex, PBPinState lastState,
                         unsigned int onTimeMS, unsigned int offTimeMS, unsigned int neoPixelIndex) {
        // Bounds check
        if (index < 0 || index >= NUM_OUTPUTS) {
            g_PBEngine.pbeSendConsole("RasPin: ERROR: Output index " + std::to_string(index) + 
                                     " out of bounds (max " + std::to_string(NUM_OUTPUTS-1) + ")");
            hasErrors = true;
            return;
        }
        
        // Duplicate check
        if (initialized[index]) {
            g_PBEngine.pbeSendConsole("RasPin: ERROR: Duplicate output index " + std::to_string(index));
            hasErrors = true;
            return;
        }
        
        // Initialize the entry
        g_outputDef[index].outputName = name;
        g_outputDef[index].outputMsg = msg;
        g_outputDef[index].pin = pin;
        g_outputDef[index].boardType = boardType;
        g_outputDef[index].boardIndex = boardIndex;
        g_outputDef[index].lastState = lastState;
        g_outputDef[index].onTimeMS = onTimeMS;
        g_outputDef[index].offTimeMS = offTimeMS;
        g_outputDef[index].neoPixelIndex = neoPixelIndex;
        
        initialized[index] = true;
    };
    
    // Initialize each output using its #define index
    setOutput(IDO_LEFTSLING, "IO0P08 LeftSling", PB_OMSG_GENERIC_IO, 8, PB_IO, 0, PB_OFF, 250, 250, 0);
    setOutput(IDO_POPBUMPER, "IO1P08 Pop Bumper", PB_OMSG_GENERIC_IO, 8, PB_IO, 1, PB_OFF, 1000, 1000, 0);
    setOutput(IDO_LED1, "Start LED", PB_OMSG_GENERIC_IO, 23, PB_RASPI, 0, PB_ON, 0, 0, 0);
    setOutput(IDO_BALLEJECT, "IO2P08 Ball Eject", PB_OMSG_GENERIC_IO, 8, PB_IO, 2, PB_OFF, 2000, 2000, 0);
    setOutput(IDO_LED2, "LED0P08 LED", PB_OMSG_LED, 8, PB_LED, 0, PB_OFF, 100, 100, 0);
    setOutput(IDO_LED3, "LED0P09 LED", PB_OMSG_LED, 9, PB_LED, 0, PB_OFF, 150, 50, 0);
    setOutput(IDO_LED4, "LED0P10 LED", PB_OMSG_LED, 10, PB_LED, 0, PB_OFF, 200, 0, 0);
    setOutput(IDO_LED5, "LED1P08 LED", PB_OMSG_LED, 8, PB_LED, 1, PB_OFF, 50, 0, 0);
    setOutput(IDO_LED6, "LED1P09 LED", PB_OMSG_LED, 9, PB_LED, 1, PB_OFF, 50, 0, 0);
    setOutput(IDO_LED7, "LED1P10 LED", PB_OMSG_LED, 10, PB_LED, 1, PB_OFF, 50, 0, 0);
    setOutput(IDO_LED8, "LED2P08 LED", PB_OMSG_LED, 8, PB_LED, 2, PB_OFF, 500, 0, 0);
    setOutput(IDO_LED9, "LED2P09 LED", PB_OMSG_LED, 9, PB_LED, 2, PB_OFF, 300, 0, 0);
    setOutput(IDO_LED10, "LED2P10 LED", PB_OMSG_LED, 10, PB_LED, 2, PB_OFF, 100, 0, 0);
    setOutput(IDO_BALLEJECT2, "IO0P15 Ball Eject", PB_OMSG_GENERIC_IO, 15, PB_IO, 0, PB_OFF, 500, 500, 0);
    setOutput(IDO_NEOPIXEL0, "NeoPixel0", PB_OMSG_NEOPIXEL, 10, PB_NEOPIXEL, 0, PB_OFF, 0, 0, 0);
    setOutput(IDO_NEOPIXEL1, "NeoPixel1", PB_OMSG_NEOPIXEL, 12, PB_NEOPIXEL, 1, PB_OFF, 0, 0, 0);
    setOutput(IDO_RIGHTSLING, "IO0P09 RightSling", PB_OMSG_GENERIC_IO, 9, PB_IO, 0, PB_OFF, 250, 250, 0);
    setOutput(IDO_LEFTFLIP, "IO0P10 LeftFlipper", PB_OMSG_GENERIC_IO, 10, PB_IO, 0, PB_OFF, 100, 100, 0);
    setOutput(IDO_RIGHTFLIP, "IO0P11 RightFlipper", PB_OMSG_GENERIC_IO, 11, PB_IO, 0, PB_OFF, 100, 100, 0);
    
    if (hasErrors) {
        g_PBEngine.pbeSendConsole("RasPin: ERROR: Output initialization failed!");
    } else {
        g_PBEngine.pbeSendConsole("RasPin: Output definitions initialized successfully");
    }
}

// Initialize input definitions array
// This function must be called before using g_inputDef
void InitializeInputDefs() {
    // Track which indices have been initialized to detect duplicates
    bool initialized[NUM_INPUTS] = {false};
    bool hasErrors = false;
    
    // Helper lambda to check and set an index
    auto setInput = [&](int index, const char* name, const char* simKey, PBInputMsg msg,
                        unsigned int pin, PBBoardType boardType, unsigned int boardIndex,
                        PBPinState lastState, unsigned long lastStateTick, unsigned long debounceTimeMS,
                        bool autoOutput, unsigned int autoOutputId, PBPinState autoPinState, 
                        bool autoOutputUsePulse) {
        // Bounds check
        if (index < 0 || index >= NUM_INPUTS) {
            g_PBEngine.pbeSendConsole("RasPin: ERROR: Input index " + std::to_string(index) + 
                                     " out of bounds (max " + std::to_string(NUM_INPUTS-1) + ")");
            hasErrors = true;
            return;
        }
        
        // Duplicate check
        if (initialized[index]) {
            g_PBEngine.pbeSendConsole("RasPin: ERROR: Duplicate input index " + std::to_string(index));
            hasErrors = true;
            return;
        }
        
        // Initialize the entry
        g_inputDef[index].inputName = name;
        g_inputDef[index].simMapKey = simKey;
        g_inputDef[index].inputMsg = msg;
        g_inputDef[index].pin = pin;
        g_inputDef[index].boardType = boardType;
        g_inputDef[index].boardIndex = boardIndex;
        g_inputDef[index].lastState = lastState;
        g_inputDef[index].lastStateTick = lastStateTick;
        g_inputDef[index].debounceTimeMS = debounceTimeMS;
        g_inputDef[index].autoOutput = autoOutput;
        g_inputDef[index].autoOutputId = autoOutputId;
        g_inputDef[index].autoPinState = autoPinState;
        g_inputDef[index].autoOutputUsePulse = autoOutputUsePulse;
        
        initialized[index] = true;
    };
    
    // Initialize each input using its #define index
    setInput(IDI_LEFTFLIPPER, "Left Flipper", "A", PB_IMSG_BUTTON, 27, PB_RASPI, 0, PB_OFF, 0, 5, true, IDO_LEFTFLIP, PB_ON, false);
    setInput(IDI_RIGHTFLIPPER, "Right Flipper", "D", PB_IMSG_BUTTON, 17, PB_RASPI, 0, PB_OFF, 0, 5, true, IDO_RIGHTFLIP, PB_ON, false);
    setInput(IDI_LEFTACTIVATE, "Left Activate", "Q", PB_IMSG_BUTTON, 5, PB_RASPI, 0, PB_OFF, 0, 5, false, 0, PB_OFF, false);
    setInput(IDI_RIGHTACTIVATE, "Right Activate", "E", PB_IMSG_BUTTON, 22, PB_RASPI, 0, PB_OFF, 0, 5, false, 0, PB_OFF, false);
    setInput(IDI_START, "Start", "Z", PB_IMSG_BUTTON, 6, PB_RASPI, 0, PB_OFF, 0, 5, false, 0, PB_OFF, false);
    setInput(IDI_RESET, "Reset", "C", PB_IMSG_BUTTON, 24, PB_RASPI, 0, PB_OFF, 0, 5, false, 0, PB_OFF, false);
    setInput(IDI_SENSOR1, "IO0P07 Eject SW2", "1", PB_IMSG_SENSOR, 7, PB_IO, 0, PB_OFF, 0, 5, false, 0, PB_OFF, false);
    setInput(IDI_SENSOR2, "IO1P07", "2", PB_IMSG_SENSOR, 7, PB_IO, 1, PB_OFF, 0, 5, false, 0, PB_OFF, false);
    setInput(IDI_SENSOR3, "IO2P07", "3", PB_IMSG_SENSOR, 7, PB_IO, 2, PB_OFF, 0, 5, false, 0, PB_OFF, false);
    setInput(IDI_LEFTSLING, "IO2P06 LSLING", "4", PB_IMSG_JETBUMPER, 6, PB_IO, 2, PB_OFF, 0, 5, true, IDO_LEFTSLING, PB_ON, true);
    setInput(IDI_RIGHTSLING, "IO2P05 RSLING", "5", PB_IMSG_JETBUMPER, 5, PB_IO, 2, PB_OFF, 0, 5, true, IDO_RIGHTSLING, PB_ON, true);
    
    if (hasErrors) {
        g_PBEngine.pbeSendConsole("RasPin: ERROR: Input initialization failed!");
    } else {
        g_PBEngine.pbeSendConsole("RasPin: Input definitions initialized successfully");
    }
}


// LEDDriver Class Implementation for TLC59116 LED Driver Chip

LEDDriver::LEDDriver(uint8_t address) : m_address(address), m_i2cFd(-1), m_groupMode(GroupModeDimming) {
    // Initialize brightness and control arrays
    for (int i = 0; i < 16; i++) {
        m_ledBrightness[i] = 0xFF;  // Start with all LEDs with maximum brightness
        m_pwmStaged[i] = false;     // No PWM changes staged initially
        m_currentBrightness[i] = 0xFF;  // Initialize current state tracking (hardware starts at 0)
    }
    for (int i = 0; i < 4; i++) {
        m_ledControl[i] = 0x00;     // Start with all LEDs in OFF state
        m_ledOutStaged[i] = false;  // No LEDOUT changes staged initially
        m_currentControl[i] = 0x00;  // Initialize current state tracking (hardware starts at 0)
    }

#ifdef EXE_MODE_RASPI
    // Initialize the TLC59116 chip
    m_i2cFd = wiringPiI2CSetup(m_address);
    if (m_i2cFd >= 0) {
        // Reset and configure MODE1 register (normal operation)
        wiringPiI2CWriteReg8(m_i2cFd, TLC59116_MODE1, TLC59116_MODE1_NORMAL);
        
        // Configure MODE2 register (enable group control dimming/blinking)
        wiringPiI2CWriteReg8(m_i2cFd, TLC59116_MODE2, TLC59116_MODE2_DMBLNK);
   
        // Initialize all LEDOUT registers to 0 (all LEDs off)
        for (int i = 0; i < 4; i++) {
            wiringPiI2CWriteReg8(m_i2cFd, TLC59116_LEDOUT0 + i, 0x00);
        }
        
        // Initialize all PWM registers to 0xFF (LEDs Max Brightness)
        for (int i = 0; i < 16; i++) {
            wiringPiI2CWriteReg8(m_i2cFd, TLC59116_PWM0 + i, 0xFF);
        }   
    }
#endif
}

LEDDriver::~LEDDriver() {
#ifdef EXE_MODE_RASPI
    // Clean up I2C connection
    if (m_i2cFd >= 0) {
        // Turn off all LEDs before closing
        for (int i = 0; i < 4; i++) {
            wiringPiI2CWriteReg8(m_i2cFd, TLC59116_LEDOUT0 + i, 0x00);
        }
        // Note: wiringPi doesn't provide an explicit close function for I2C
        // The file descriptor will be cleaned up when the process ends
        m_i2cFd = -1;
    }
#endif
    // Reset group mode state
    m_groupMode = GroupModeDimming;
}

void LEDDriver::SetGroupMode(LEDGroupMode groupMode, unsigned int brightness, unsigned int msTimeOn, unsigned int msTimeOff) {
    // Set the group mode based on the parameter
    m_groupMode = groupMode;

#ifdef EXE_MODE_RASPI
    if (m_i2cFd >= 0) {
        
        uint8_t groupBrightness = (brightness > 255) ? 255 : (uint8_t)brightness;
        uint8_t grpFreq = 0;
        // Handle different group modes
        switch (groupMode) {
            case GroupModeDimming:
                // Set group brightness (0-255
                wiringPiI2CWriteReg8(m_i2cFd, TLC59116_GRPPWM, groupBrightness);
                // Disable group mode - set frequency to 0 (no blinking)
                wiringPiI2CWriteReg8(m_i2cFd, TLC59116_GRPFREQ, 0x00);
            break;
    
            case GroupModeBlinking:
                // Enable blinking - calculate and set frequency
                unsigned int totalTimeMs = msTimeOn + msTimeOff;
                
                // Program GRPPWM with duty cycle percent (ON/OFF Ratio)
                uint8_t groupDutyCycle = (msTimeOn * 255 / totalTimeMs);
                wiringPiI2CWriteReg8(m_i2cFd, TLC59116_GRPPWM, groupDutyCycle);

                if (totalTimeMs > 0) {
                    // TLC59116 blink period = (GRPFREQ + 1) / 24 seconds
                    // We want: period = (msTimeOn + msTimeOff) / 1000 seconds
                    // So: GRPFREQ = ((msTimeOn + msTimeOff) * 24 / 1000) - 1
                    int freqCalc = (totalTimeMs * 24 / 1000) - 1;
                    // Ensure grpFreq is not negative
                    if (freqCalc < 0) {
                        grpFreq = 0;
                    } else if (freqCalc > 255) {
                        grpFreq = 255;
                    } else {
                        grpFreq = (uint8_t)freqCalc;
                    }
                } else {
                    // Default to reasonable blink rate if no timing specified
                    grpFreq = 23;  // Approximately 1 second period (24/24 seconds)
                }
                
                wiringPiI2CWriteReg8(m_i2cFd, TLC59116_GRPFREQ, grpFreq);
            break;
        }
    }
#endif
}

uint8_t LEDDriver::GetControlValue(LEDState state) const {
    switch (state) {
        case LEDOff:     return 0x00;  // 00 = LED off
        case LEDOn:      return 0x01;  // 01 = LED on
        case LEDDimming: return 0x02;  // 10 = LED individual brightness
        case LEDGroup:   return 0x03;  // 11 = LED group control
        default:         return 0x00;  // Default to off for safety
    }
}

LEDState LEDDriver::GetLEDStateFromVal(uint8_t regValue, unsigned int pin) const {
    // Calculate bit position within the register (pin % 4 gives position 0-3, then * 2 for 2-bit values)
    int pinInReg = pin % 4;  // Position within the register (0-3)
    int bitPos = pinInReg * 2;  // Each pin uses 2 bits
    
    // Extract the 2-bit LED state from the register value
    uint8_t pinState = (regValue >> bitPos) & 0x03;  // Extract 2 bits
    
    // Convert 2-bit value to LEDState
    switch (pinState) {
        case 0x00: return LEDOff;      // 00 = LED off
        case 0x01: return LEDOn;       // 01 = LED on  
        case 0x02: return LEDDimming;  // 10 = PWM (individual brightness)
        case 0x03: return LEDGroup;    // 11 = PWM+Group (group control)
        default:   return LEDOff;      // Default to off for safety
    }
}

void LEDDriver::StageLEDControl(bool setAll, unsigned int LEDIndex, LEDState state) {
    
    uint8_t controlValue = GetControlValue(state);

    if (setAll) {
        // Set all LEDs to the same state    // Each register controls 4 LEDs (2 bits per LED)
        uint8_t regValue = (controlValue << 6) | (controlValue << 4) | (controlValue << 2) | controlValue;
        for (int i = 0; i < 4; i++) {
            // Only stage if the value differs from what was last sent to hardware
            if (m_currentControl[i] != regValue) {
                m_ledControl[i] = regValue;
                m_ledOutStaged[i] = true;  // Mark this LEDOUT register as staged
            }
        }
    } else {
        // Set specific LED
        if (LEDIndex < 16) {
            uint8_t regIndex = LEDIndex / 4;      // Which LEDOUT register (0-3)
            uint8_t bitPos = (LEDIndex % 4) * 2;  // Bit position within register (0,2,4,6)
            
            // Calculate new register value starting from the last staged state
            uint8_t newRegValue = m_ledControl[regIndex];
            newRegValue &= ~(0x03 << bitPos);     // Clear the 2 bits for this LED
            newRegValue |= (controlValue << bitPos);  // Set new value
            
            // Always update m_ledControl to keep it in sync with intended state
            m_ledControl[regIndex] = newRegValue;
            
            // Only set the staged flag if the value differs from what was last sent to hardware
            if (m_currentControl[regIndex] != newRegValue) {
                m_ledOutStaged[regIndex] = true;  // Mark this LEDOUT register as staged
            }
        }
    }
}

void LEDDriver::StageLEDControl(unsigned int registerIndex, uint8_t value) {
    if (registerIndex < 4) {
        // Only stage if the value differs from what was last sent to hardware
        if (m_currentControl[registerIndex] != value) {
            m_ledControl[registerIndex] = value;
            m_ledOutStaged[registerIndex] = true;
        }
    }
}

void LEDDriver::SyncStagedWithHardware(unsigned int registerIndex) {
    // Sync the staged register value with current hardware state
    // This ensures subsequent staging operations build upon the correct baseline
    if (registerIndex < 4) {
        m_ledControl[registerIndex] = m_currentControl[registerIndex];
    }
}

void LEDDriver::StageLEDBrightness(bool setAll, unsigned int LEDIndex, uint8_t brightness) {
    if (setAll) {
        // Set all LEDs to the same brightness
        for (int i = 0; i < 16; i++) {
            // Only stage if the brightness value differs from what was last sent to hardware
            if (m_currentBrightness[i] != brightness) {
                m_ledBrightness[i] = brightness;
                m_pwmStaged[i] = true;  // Mark this PWM register as staged
            }
        }
    } else {
        // Set specific LED brightness
        if (LEDIndex < 16) {
            // Only stage if the brightness value differs from what was last sent to hardware
            if (m_currentBrightness[LEDIndex] != brightness) {
                m_ledBrightness[LEDIndex] = brightness;
                m_pwmStaged[LEDIndex] = true;  // Mark this PWM register as staged
            }
        }
    }
}

void LEDDriver::SendStagedLED() {
#ifdef EXE_MODE_RASPI
    if (m_i2cFd >= 0) {
        // Send only staged PWM brightness values
        for (int i = 0; i < 16; i++) {
            if (m_pwmStaged[i]) {
               int result = wiringPiI2CWriteReg8(m_i2cFd, TLC59116_PWM0 + i, m_ledBrightness[i]);
                if (result >= 0) {
                    // Success - update current state tracking and clear staged flag
                    m_currentBrightness[i] = m_ledBrightness[i];
                    m_pwmStaged[i] = false;
                } 
                // On failure, keep staged flag set so value will be retried on next call
            }
        }
        
        // Send only staged LEDOUT control values
        for (int i = 0; i < 4; i++) {
            if (m_ledOutStaged[i]) {
                int result = wiringPiI2CWriteReg8(m_i2cFd, TLC59116_LEDOUT0 + i, m_ledControl[i]);
                if (result >= 0) {
                    // Success - update current state tracking and clear staged flag
                    m_currentControl[i] = m_ledControl[i];
                    m_ledOutStaged[i] = false;
                }
                // On failure, keep staged flag set so value will be retried on next call
            }
        }
    }
#endif
#ifdef EXE_MODE_WINDOWS
    // Windows mode simulation - update tracking and clear flags without hardware writes
        for (int i = 0; i < 16; i++) {
            if (m_pwmStaged[i]) {
                m_currentBrightness[i] = m_ledBrightness[i];
                m_pwmStaged[i] = false;
            }
        }

        for (int i = 0; i < 4; i++) {
            if (m_ledOutStaged[i]) {
                m_currentControl[i] = m_ledControl[i];
                m_ledOutStaged[i] = false;
            }
        }
#endif
}

LEDGroupMode LEDDriver::GetGroupMode() const {
    return m_groupMode;
}

uint8_t LEDDriver::GetAddress() const {
    return m_address;
}

bool LEDDriver::HasStagedChanges() const {
    // Check if any PWM registers have staged changes
    for (int i = 0; i < 16; i++) {
        if (m_pwmStaged[i]) {
            return true;
        }
    }
    
    // Check if any LEDOUT registers have staged changes
    for (int i = 0; i < 4; i++) {
        if (m_ledOutStaged[i]) {
            return true;
        }
    }
    
    return false;  // No changes staged
}

uint8_t LEDDriver::ReadModeRegister(uint8_t modeRegister) const {
    uint8_t value = 0;
#ifdef EXE_MODE_RASPI
    if (m_i2cFd >= 0) {
        if (modeRegister == 1 || modeRegister == 2) {
            value = wiringPiI2CReadReg8(m_i2cFd, TLC59116_MODE1 + (modeRegister - 1));
        }
    }
#endif
    return value;
}

uint8_t LEDDriver::ReadPWMRegister(uint8_t pwmIndex) const {
    uint8_t value = 0;
#ifdef EXE_MODE_RASPI
    if (m_i2cFd >= 0 && pwmIndex < 16) {
        value = wiringPiI2CReadReg8(m_i2cFd, TLC59116_PWM0 + pwmIndex);
    }
#endif
    return value;
}

uint8_t LEDDriver::ReadLEDOutRegister(uint8_t ledOutIndex) const {
    uint8_t value = 0;
#ifdef EXE_MODE_RASPI
    if (m_i2cFd >= 0 && ledOutIndex < 4) {
        value = wiringPiI2CReadReg8(m_i2cFd, TLC59116_LEDOUT0 + ledOutIndex);
    }
#endif
    return value;
}

uint8_t LEDDriver::ReadGroupPWM() const {
    uint8_t value = 0;
#ifdef EXE_MODE_RASPI
    if (m_i2cFd >= 0) {
        value = wiringPiI2CReadReg8(m_i2cFd, TLC59116_GRPPWM);
    }
#endif
    return value;
}

uint8_t LEDDriver::ReadGroupFreq() const {
    uint8_t value = 0;
#ifdef EXE_MODE_RASPI
    if (m_i2cFd >= 0) {
        value = wiringPiI2CReadReg8(m_i2cFd, TLC59116_GRPFREQ);
    }
#endif
    return value;
}

uint8_t LEDDriver::ReadLEDControl(LEDHardwareState hwState, uint8_t registerIndex) const {
    // Read from staged or current LED control register (LEDOUT0-LEDOUT3)
    if (registerIndex < 4) {
        switch (hwState) {
            case StagedHW:
                return m_ledControl[registerIndex];    // Return staged value
            case CurrentHW:
                return m_currentControl[registerIndex]; // Return current hardware state
            default:
                return 0x00;  // Default to 0 for invalid hwState
        }
    }
    return 0x00;  // Return 0 for invalid registerIndex
}

uint8_t LEDDriver::ReadLEDBrightness(LEDHardwareState hwState, uint8_t ledIndex) const {
    // Read from staged or current LED brightness register (PWM0-PWM15)
    if (ledIndex < 16) {
        switch (hwState) {
            case StagedHW:
                return m_ledBrightness[ledIndex];    // Return staged value
            case CurrentHW:
                return m_currentBrightness[ledIndex]; // Return current hardware state
            default:
                return 0x00;  // Default to 0 for invalid hwState
        }
    }
    return 0x00;  // Return 0 for invalid ledIndex
}

//==============================================================================
// IODriver Class Implementation for TCA9555 I/O Expander Chip
//==============================================================================

IODriver::IODriver(uint8_t address, uint16_t inputMask) : m_address(address), m_i2cFd(-1), m_inputMask(inputMask) {
    // Initialize output values and staging flags
    for (int i = 0; i < 2; i++) {
        m_outputValues[i] = 0x00;      // Start with all outputs low
        m_outputStaged[i] = false;     // No changes staged initially
        m_currentOutputValues[i] = 0x00;  // Initialize current state tracking (hardware starts at 0)
    }

#ifdef EXE_MODE_RASPI
    // Initialize the TCA9555 chip
    m_i2cFd = wiringPiI2CSetup(m_address);
    if (m_i2cFd >= 0) {
        // Configure port directions based on input mask
        // TCA9555: 1 = input, 0 = output in configuration registers
        uint8_t configPort0 = (uint8_t)(m_inputMask & 0xFF);         // Lower 8 bits
        uint8_t configPort1 = (uint8_t)((m_inputMask >> 8) & 0xFF);  // Upper 8 bits
        
        // Set pins based on input mask
        wiringPiI2CWriteReg8(m_i2cFd, TCA9555_CONFIG_PORT0, configPort0);
        wiringPiI2CWriteReg8(m_i2cFd, TCA9555_CONFIG_PORT1, configPort1);
        
        // Set polarity registers to normal (0 = normal polarity)
        wiringPiI2CWriteReg8(m_i2cFd, TCA9555_POLARITY_PORT0, 0x00);
        wiringPiI2CWriteReg8(m_i2cFd, TCA9555_POLARITY_PORT1, 0x00);
        
        // Initialize output registers to 0
        wiringPiI2CWriteReg8(m_i2cFd, TCA9555_OUTPUT_PORT0, 0x00);
        wiringPiI2CWriteReg8(m_i2cFd, TCA9555_OUTPUT_PORT1, 0x00);
    }
#endif
}

IODriver::~IODriver() {
#ifdef EXE_MODE_RASPI
    // Clean up I2C connection
    if (m_i2cFd >= 0) {
        // Set all outputs to low before closing
        wiringPiI2CWriteReg8(m_i2cFd, TCA9555_OUTPUT_PORT0, 0x00);
        wiringPiI2CWriteReg8(m_i2cFd, TCA9555_OUTPUT_PORT1, 0x00);
        // Note: wiringPi doesn't provide an explicit close function for I2C
        // The file descriptor will be cleaned up when the process ends
        m_i2cFd = -1;
    }
#endif
}

void IODriver::StageOutput(uint16_t value) {
    // Split 16-bit value into two 8-bit ports
    uint8_t newPort0 = (uint8_t)(value & 0xFF);         // Lower 8 bits for port 0
    uint8_t newPort1 = (uint8_t)((value >> 8) & 0xFF);  // Upper 8 bits for port 1
    
    // Only stage ports that differ from what was last sent to hardware
    if (m_currentOutputValues[0] != newPort0) {
        m_outputValues[0] = newPort0;
        m_outputStaged[0] = true;
    }
    if (m_currentOutputValues[1] != newPort1) {
        m_outputValues[1] = newPort1;
        m_outputStaged[1] = true;
    }
}

void IODriver::StageOutputPin(uint8_t pinIndex, PBPinState value) {
    if (pinIndex < 16) {
        uint8_t port = pinIndex / 8;      // Which port (0 or 1)
        uint8_t bitPos = pinIndex % 8;    // Bit position within port (0-7)
        
        uint8_t newPortValue =  m_outputValues[port];  // Start with current staged state
        // Set the bits - This system uses active low outputs
        if (value == PB_OFF) {
            // Set the bit
            newPortValue |= (1 << bitPos);
        } else {
            // Clear the bit
            newPortValue &= ~(1 << bitPos);
        }
        
        // Only stage if the port value differs from what was last sent to hardware
        if (m_currentOutputValues[port] != newPortValue) {
            m_outputValues[port] = newPortValue;
            m_outputStaged[port] = true;
        }
    }
}

void IODriver::SendStagedOutput() {
#ifdef EXE_MODE_RASPI
    if (m_i2cFd >= 0) {
        // Send only staged output values
        for (int i = 0; i < 2; i++) {
            if (m_outputStaged[i]) {
                wiringPiI2CWriteReg8(m_i2cFd, TCA9555_OUTPUT_PORT0 + i, m_outputValues[i]);
                m_currentOutputValues[i] = m_outputValues[i];  // Update current state tracking
                m_outputStaged[i] = false;  // Clear the staged flag after sending
            }
        }
    }
#endif
}

uint16_t IODriver::ReadInputs() {
    uint16_t inputValue = 0;
    
#ifdef EXE_MODE_RASPI
    if (m_i2cFd >= 0) {
        // Read both input ports
        uint8_t port0 = wiringPiI2CReadReg8(m_i2cFd, TCA9555_INPUT_PORT0);
        uint8_t port1 = wiringPiI2CReadReg8(m_i2cFd, TCA9555_INPUT_PORT1);
        
        // Combine into 16-bit value (port1 in upper 8 bits, port0 in lower 8 bits)
        inputValue = ((uint16_t)port1 << 8) | port0;
    }
#endif
    
    return inputValue;
}

bool IODriver::HasStagedChanges() const {
    // Check if any output ports have staged changes
    for (int i = 0; i < 2; i++) {
        if (m_outputStaged[i]) {
            return true;
        }
    }
    
    return false;  // No changes staged
}

uint8_t IODriver::GetAddress() const {
    return m_address;
}

void IODriver::ConfigurePin(uint8_t pinIndex, PBPinDirection direction) {
    if (pinIndex >= 16) {
        return;  // Invalid pin index
    }

#ifdef EXE_MODE_RASPI
    if (m_i2cFd >= 0) {
        uint8_t port = pinIndex / 8;      // Which port (0 or 1)
        uint8_t bitPos = pinIndex % 8;    // Bit position within port (0-7)
        
        // Read current configuration register
        uint8_t configReg = TCA9555_CONFIG_PORT0 + port;
        uint8_t currentConfig = wiringPiI2CReadReg8(m_i2cFd, configReg);
        
        // Update the specific bit for this pin
        // TCA9555: 1 = input, 0 = output in configuration registers
        if (direction == PB_INPUT) {
            // Set bit to 1 for input
            currentConfig |= (1 << bitPos);
            // Update input mask to reflect this pin is now an input
            m_inputMask |= (1 << pinIndex);
        } else {
            // Clear bit to 0 for output
            currentConfig &= ~(1 << bitPos);
            // Update input mask to reflect this pin is now an output
            m_inputMask &= ~(1 << pinIndex);
        }
        
        // Write the updated configuration back to the chip
        wiringPiI2CWriteReg8(m_i2cFd, configReg, currentConfig);
    }
#endif
}

uint8_t IODriver::ReadOutputPort(uint8_t portIndex) const {
    uint8_t value = 0;
#ifdef EXE_MODE_RASPI
    if (m_i2cFd >= 0 && portIndex < 2) {
        value = wiringPiI2CReadReg8(m_i2cFd, TCA9555_OUTPUT_PORT0 + portIndex);
    }
#endif
    return value;
}

uint8_t IODriver::ReadPolarityPort(uint8_t portIndex) const {
    uint8_t value = 0;
#ifdef EXE_MODE_RASPI
    if (m_i2cFd >= 0 && portIndex < 2) {
        value = wiringPiI2CReadReg8(m_i2cFd, TCA9555_POLARITY_PORT0 + portIndex);
    }
#endif
    return value;
}

uint8_t IODriver::ReadConfigPort(uint8_t portIndex) const {
    uint8_t value = 0;
#ifdef EXE_MODE_RASPI
    if (m_i2cFd >= 0 && portIndex < 2) {
        value = wiringPiI2CReadReg8(m_i2cFd, TCA9555_CONFIG_PORT0 + portIndex);
    }
#endif
    return value;
}

uint8_t IODriver::ReadOutputValues(LEDHardwareState hwState, uint8_t portIndex) const {
    // Read from staged or current output values (OUTPUT_PORT0-OUTPUT_PORT1)
    if (portIndex < 2) {
        switch (hwState) {
            case StagedHW:
                return m_outputValues[portIndex];    // Return staged value
            case CurrentHW:
                return m_currentOutputValues[portIndex]; // Return current hardware state
            default:
                return 0x00;  // Default to 0 for invalid hwState
        }
    }
    return 0x00;  // Return 0 for invalid portIndex
}

//==============================================================================
// AmpDriver Class Implementation for MAX9744 Amplifier Chip
//==============================================================================

AmpDriver::AmpDriver(uint8_t address) : m_address(address), m_i2cFd(-1), m_currentVolume(0) {
#ifdef EXE_MODE_RASPI
    // Initialize the MAX9744 amplifier chip
    m_i2cFd = wiringPiI2CSetup(m_address);
    if (m_i2cFd >= 0) {
        // Set initial volume to 0 (mute)
        SetVolume(0);
    }
#endif
}

AmpDriver::~AmpDriver() {
#ifdef EXE_MODE_RASPI
    // Mute the amplifier before cleanup
    if (m_i2cFd >= 0) {
        SetVolume(0);
        // Note: wiringPi doesn't provide an explicit close function for I2C
        // The file descriptor will be cleaned up when the process ends
        m_i2cFd = -1;
    }
#endif
}

void AmpDriver::SetVolume(uint8_t volumePercent) {
    // Clamp volume to 0-100%
    if (volumePercent > 100) {
        volumePercent = 100;
    }
    
    m_currentVolume = volumePercent;
    
#ifdef EXE_MODE_RASPI
    if (m_i2cFd >= 0) {
        uint8_t registerValue = PercentToRegisterValue(volumePercent);
        wiringPiI2CRawWrite(m_i2cFd, &registerValue, 1);
    }
#endif
}

uint8_t AmpDriver::GetAddress() const {
    return m_address;
}

bool AmpDriver::IsConnected() const {
#ifdef EXE_MODE_RASPI
    if (m_i2cFd < 0) {
        return false;  // I2C setup failed
    }
    
    // Try to read from the device to verify it's responding
    uint8_t readValue = 0;
    int result = wiringPiI2CRawRead(m_i2cFd, &readValue, 1);
    
    // Device is connected if read succeeds and doesn't return 0xFF (typical I2C error value)
    return (result >= 0 && readValue != 0xFF);
#else
    return true;  // Always return true for Windows builds
#endif
}

uint8_t AmpDriver::PercentToRegisterValue(uint8_t percent) const {
    if (percent == 0) {
        return 0x00;  // Mute
    } 

    uint8_t range = MAX9744_VOLUME_MAX - MAX9744_VOLUME_MIN;

    // MAX9744 has 64 volume levels (0x00 to 0x3F)
    // Map 1-100% to MAX9744_VOLUME_MIN-MAX9744_VOLUME_MAX
    
    uint8_t value = ((percent * range) / 100) + MAX9744_VOLUME_MIN;

    // Ensure we don't exceed MAX9744_VOLUME_MAX
    if (value > MAX9744_VOLUME_MAX) {
        value = MAX9744_VOLUME_MAX;
    }

    if (value < MAX9744_VOLUME_MIN) {
        value = MAX9744_VOLUME_MIN;
    }
    
    return value;
}

//==============================================================================
// NeoPixelDriver Class Implementation for SK6812 RGB LED Strips
//==============================================================================

NeoPixelDriver::NeoPixelDriver(unsigned int driverIndex, unsigned char* spiBuffer) 
    : m_driverIndex(driverIndex), m_outputPin(0), m_numLEDs(0), m_nodes(nullptr), m_hasChanges(false), m_gpioInitialized(false), m_spiBuffer(spiBuffer) {
    
    // Get the number of LEDs from g_NeoPixelSize array
    if (driverIndex < sizeof(g_NeoPixelSize) / sizeof(g_NeoPixelSize[0])) {
        m_numLEDs = g_NeoPixelSize[driverIndex];
    }
    
    // Safety check: Validate LED count
    if (m_numLEDs == 0 || m_numLEDs > NEOPIXEL_MAX_LEDS_ABSOLUTE) {
        if (m_numLEDs > NEOPIXEL_MAX_LEDS_ABSOLUTE) {
            m_numLEDs = NEOPIXEL_MAX_LEDS_ABSOLUTE;
        }
        
    }
    
    // Find the GPIO pin from g_outputDef for this driver index
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        if (g_outputDef[i].boardType == PB_NEOPIXEL && 
            g_outputDef[i].boardIndex == static_cast<int>(driverIndex)) {
            m_outputPin = g_outputDef[i].pin;
            break;
        }
    }
    
    // Use pre-allocated array from g_NeoPixelNodeArray
    m_nodes = g_NeoPixelNodeArray[driverIndex];
    
    // Initialize SPI-specific members
    // Determine SPI channel based on output pin (if SPI-capable)
    if (m_outputPin == SPI0_MOSI_PIN) {
        m_spiChannel = 0;  // SPI0
    } else if (m_outputPin == SPI1_MOSI_PIN) {
        m_spiChannel = 1;  // SPI1
    } else {
        m_spiChannel = -1;  // Not an SPI pin
    }
    m_spiFd = -1;      // Not initialized
    
    // Initialize brightness control
    m_maxBrightness = 255;  // Default to maximum brightness
    
    // Select default timing method based on pin capabilities
    // Priority: SPI_BURST (fastest) > SPI > clock_gettime (fallback)
    if (m_spiChannel >= 0) {
        m_timingMethod = NEOPIXEL_TIMING_SPI_BURST;  // Use burst mode by default for SPI
    } else {
        m_timingMethod = NEOPIXEL_TIMING_CLOCKGETTIME;
    }
    
    // Initialize all LEDs to off (black) with full brightness in both staged and current fields
    for (unsigned int i = 0; i < m_numLEDs; i++) {
        m_nodes[i].stagedRed = 0;
        m_nodes[i].stagedGreen = 0;
        m_nodes[i].stagedBlue = 0;
        m_nodes[i].stagedBrightness = 255;
        m_nodes[i].currentRed = 0;
        m_nodes[i].currentGreen = 0;
        m_nodes[i].currentBlue = 0;
        m_nodes[i].currentBrightness = 255;
    }

    // Note: GPIO initialization is deferred to InitializeGPIO() method
    // This allows the constructor to be called during static initialization
    // before wiringPiSetup() has been called
    
    // Initialize instrumentation
    InitializeInstrumentationData();
}

#ifdef EXE_MODE_RASPI
// Initialize GPIO pins - must be called after wiringPiSetup()
void NeoPixelDriver::InitializeGPIO() {
    // Initialize based on the timing method
    switch (m_timingMethod) {
        case NEOPIXEL_TIMING_SPI:
        case NEOPIXEL_TIMING_SPI_BURST:
            // Initialize SPI hardware - this will set up the pin for SPI mode
            InitializeSPI();
            // Send initial reset to put NeoPixels in known state after power-up
            if (m_timingMethod != NEOPIXEL_TIMING_DISABLED) {
                SendReset();
            }
            break;
            
        case NEOPIXEL_TIMING_CLOCKGETTIME:
        case NEOPIXEL_TIMING_NOP:
            // For bit-banging methods, configure as regular GPIO output
            pinMode(m_outputPin, OUTPUT);
            digitalWrite(m_outputPin, LOW);
            
            // Send initial reset to ensure LEDs are in known state
            SendReset();
            break;
            
        case NEOPIXEL_TIMING_DISABLED:
        default:
            // NeoPixels are disabled - do nothing
            break;
    }
    
    // Mark GPIO as initialized
    m_gpioInitialized = true;
}
#else
// Windows/non-RasPi version - no GPIO initialization needed
void NeoPixelDriver::InitializeGPIO() {
    // No-op on non-Raspberry Pi platforms
    m_gpioInitialized = true;
}
#endif

NeoPixelDriver::~NeoPixelDriver() {
#ifdef EXE_MODE_RASPI
    // Turn off all LEDs before cleanup
    for (unsigned int i = 0; i < m_numLEDs; i++) {
        m_nodes[i].stagedRed = 0;
        m_nodes[i].stagedGreen = 0;
        m_nodes[i].stagedBlue = 0;
    }
    m_hasChanges = true;
    SendStagedNeoPixels();
    
    // Clean up SPI if initialized
    if (m_spiFd >= 0) {
        CloseSPI();
    }
#endif
}

void NeoPixelDriver::StageNeoPixel(unsigned int ledIndex, uint8_t red, uint8_t green, uint8_t blue) {
    // Call with default brightness of 255
    StageNeoPixel(ledIndex, red, green, blue, 255);
}

// Helper function to convert PBLEDColor enum to RGB values
void NeoPixelDriver::ColorToRGB(PBLEDColor color, uint8_t& red, uint8_t& green, uint8_t& blue) {
    // Initialize all channels to 0
    red = green = blue = 0;
    
    // Set channels to 255 based on color
    switch (color) {
        case PB_LEDRED:
            red = 255;
            break;
        case PB_LEDGREEN:
            green = 255;
            break;
        case PB_LEDBLUE:
            blue = 255;
            break;
        case PB_LEDWHITE:
            red = green = blue = 255;
            break;
        case PB_LEDPURPLE:  // Magenta
            red = blue = 255;
            break;
        case PB_LEDYELLOW:
            red = green = 255;
            break;
        case PB_LEDCYAN:
            green = blue = 255;
            break;
    }
}

void NeoPixelDriver::StageNeoPixel(unsigned int ledIndex, const stNeoPixelNode& node) {
    StageNeoPixel(ledIndex, node.stagedRed, node.stagedGreen, node.stagedBlue, node.stagedBrightness);
}

void NeoPixelDriver::StageNeoPixel(unsigned int ledIndex, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness) {
    if (ledIndex < m_numLEDs) {
        // Cap brightness at maximum
        brightness = CapBrightness(brightness);
        
        // Store the RGB values and brightness
        // Only mark as changed if values actually differ from current hardware
        if (m_nodes[ledIndex].currentRed != red ||
            m_nodes[ledIndex].currentGreen != green ||
            m_nodes[ledIndex].currentBlue != blue ||
            m_nodes[ledIndex].currentBrightness != brightness) {
            m_nodes[ledIndex].stagedRed = red;
            m_nodes[ledIndex].stagedGreen = green;
            m_nodes[ledIndex].stagedBlue = blue;
            m_nodes[ledIndex].stagedBrightness = brightness;
            m_hasChanges = true;
        }
    }
}

void NeoPixelDriver::StageNeoPixel(unsigned int ledIndex, const stNeoPixelNode& node, uint8_t brightness) {
    StageNeoPixel(ledIndex, node.stagedRed, node.stagedGreen, node.stagedBlue, brightness);
}

void NeoPixelDriver::StageNeoPixel(unsigned int ledIndex, PBLEDColor color) {
    // Convert PBLEDColor to RGB values
    uint8_t red, green, blue;
    ColorToRGB(color, red, green, blue);
    
    StageNeoPixel(ledIndex, red, green, blue);
}

void NeoPixelDriver::StageNeoPixel(unsigned int ledIndex, PBLEDColor color, uint8_t brightness) {
    // Convert PBLEDColor to RGB values, then apply brightness
    uint8_t red, green, blue;
    ColorToRGB(color, red, green, blue);
    
    StageNeoPixel(ledIndex, red, green, blue, brightness);
}

// Stage all LEDs in the chain - efficiently updates entire array
void NeoPixelDriver::StageNeoPixelAll(uint8_t red, uint8_t green, uint8_t blue) {
    // Use default brightness of 255
    StageNeoPixelAll(red, green, blue, 255);
}

void NeoPixelDriver::StageNeoPixelAll(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness) {
    // Cap brightness at maximum
    brightness = CapBrightness(brightness);
    
    // Directly update the staged array for efficiency
    for (unsigned int i = 0; i < m_numLEDs; i++) {
        // Check if value differs from current hardware
        if (m_nodes[i].currentRed != red || 
            m_nodes[i].currentGreen != green || 
            m_nodes[i].currentBlue != blue ||
            m_nodes[i].currentBrightness != brightness) {
            m_nodes[i].stagedRed = red;
            m_nodes[i].stagedGreen = green;
            m_nodes[i].stagedBlue = blue;
            m_nodes[i].stagedBrightness = brightness;
            m_hasChanges = true;
        }
    }
}

void NeoPixelDriver::StageNeoPixelAll(const stNeoPixelNode& node) {
    StageNeoPixelAll(node.stagedRed, node.stagedGreen, node.stagedBlue, node.stagedBrightness);
}

void NeoPixelDriver::StageNeoPixelAll(const stNeoPixelNode& node, uint8_t brightness) {
    StageNeoPixelAll(node.stagedRed, node.stagedGreen, node.stagedBlue, brightness);
}

void NeoPixelDriver::StageNeoPixelAll(PBLEDColor color) {
    // Convert PBLEDColor to RGB values
    uint8_t red, green, blue;
    ColorToRGB(color, red, green, blue);
    
    StageNeoPixelAll(red, green, blue);
}

void NeoPixelDriver::StageNeoPixelAll(PBLEDColor color, uint8_t brightness) {
    // Convert PBLEDColor to RGB values, then apply brightness
    uint8_t red, green, blue;
    ColorToRGB(color, red, green, blue);
    
    StageNeoPixelAll(red, green, blue, brightness);
}

void NeoPixelDriver::StageNeoPixelArray(const stNeoPixelNode* nodeArray, unsigned int count) {
    unsigned int numToStage = (count < m_numLEDs) ? count : m_numLEDs;
    for (unsigned int i = 0; i < numToStage; i++) {
        StageNeoPixel(i, nodeArray[i]);
    }
}

// Brightness control
void NeoPixelDriver::SetMaxBrightness(uint8_t maxBrightness) {
    m_maxBrightness = maxBrightness;
}

void NeoPixelDriver::SendStagedNeoPixels() {
    // Only send if there are changes
    if (!m_hasChanges) {
        return;
    }

    // Check to see if the GPIO has been initialized
    if (!m_gpioInitialized) {
        return;
    }

    
    #ifdef EXE_MODE_RASPI
    #if NEOPIXEL_USE_RT_PRIORITY
    // Temporarily elevate to real-time priority for deterministic timing
    // Requires sudo privileges: sudo ./Pinball
    struct sched_param sp;
    sp.sched_priority = 99;  // Highest priority
    int oldPolicy = SCHED_OTHER;
    struct sched_param oldParam;
    pthread_getschedparam(pthread_self(), &oldPolicy, &oldParam);
    
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp) != 0) {
        // Failed to set RT priority - continue with normal priority
        // This is expected if not running with sudo
    }
    #endif

    // Send data for each LED in the string
    // SK6812 expects GRB format (not RGB)
    // Apply brightness scaling during transmission
    
    if (m_timingMethod == NEOPIXEL_TIMING_SPI_BURST) {
        // Use optimized SPI burst mode - sends entire string at once
        SendAllPixelsSPI();
    } else {
        // Use byte-at-a-time mode for other timing methods
        for (unsigned int i = 0; i < m_numLEDs; i++) {
            uint8_t brightness = m_nodes[i].stagedBrightness;
            uint8_t green = ApplyBrightness(m_nodes[i].stagedGreen, brightness);
            uint8_t red = ApplyBrightness(m_nodes[i].stagedRed, brightness);
            uint8_t blue = ApplyBrightness(m_nodes[i].stagedBlue, brightness);
            
            SendByte(green, m_timingMethod);  // Green first
            SendByte(red, m_timingMethod);    // Red second
            SendByte(blue, m_timingMethod);   // Blue third
        }
    }
    
    // Send reset/latch signal (interrupts enabled for this)
    SendReset();

    #if NEOPIXEL_USE_RT_PRIORITY
    // Restore original scheduling policy
    pthread_setschedparam(pthread_self(), oldPolicy, &oldParam);
    #endif
    #endif // EXE_MODE_RASPI

    // Update current values to match staged values
    for (unsigned int i = 0; i < m_numLEDs; i++) {
        m_nodes[i].currentRed = m_nodes[i].stagedRed;
        m_nodes[i].currentGreen = m_nodes[i].stagedGreen;
        m_nodes[i].currentBlue = m_nodes[i].stagedBlue;
        m_nodes[i].currentBrightness = m_nodes[i].stagedBrightness;
    }
    
    // Clear the changes flag
    m_hasChanges = false;

#ifdef EXE_MODE_WINDOWS
    // Windows mode simulation - just update tracking
    // Changes flag already cleared above
#endif
}

bool NeoPixelDriver::HasStagedChanges() const {
    return m_hasChanges;
}

void NeoPixelDriver::SetTimingMethod(NeoPixelTimingMethod method) {
    // Once disabled, cannot change timing method
    if (m_timingMethod == NEOPIXEL_TIMING_DISABLED) {
        return;
    }
    m_timingMethod = method;
}

void NeoPixelDriver::SendByte(uint8_t byte, NeoPixelTimingMethod method) {
#ifdef EXE_MODE_RASPI
    // If disabled, do nothing
    if (method == NEOPIXEL_TIMING_DISABLED) {
        return;
    }
    
    // SK6812 timing requirements (at ~833kHz):
    // Bit 1: 0.6us high, 0.6us low (1.2us total)
    // Bit 0: 0.3us high, 0.9us low (1.2us total)
    
    switch (method) {
        case NEOPIXEL_TIMING_CLOCKGETTIME:
            SendByteClockGetTime(byte);
            break;
        case NEOPIXEL_TIMING_NOP:
            SendByteNOP(byte);
            break;
        case NEOPIXEL_TIMING_SPI:
            SendByteSPI(byte);
            break;
        case NEOPIXEL_TIMING_DISABLED:
        default:
            // Do nothing - NeoPixels disabled
            break;
    }
#endif
}

void NeoPixelDriver::SendByteClockGetTime(uint8_t byte) {
#ifdef EXE_MODE_RASPI
        // Option 1: Use clock_gettime() for timing (original implementation)
        // Pros: Portable, no CPU frequency dependency
        // Cons: Syscall overhead, can be affected by scheduling
        struct timespec ts_start, ts_now, ts_high_start, ts_high_end, ts_low_start, ts_low_end, ts_byte_start;
        
        // Capture start time for total transmission if instrumentation enabled
        if (m_instrumentation.instrumentationEnabled) {
            clock_gettime(CLOCK_MONOTONIC, &ts_byte_start);
        }
        
        for (int bit = 7; bit >= 0; bit--) {
            bool bitValue = (byte & (1 << bit)) != 0;
            uint32_t highTimeNs = 0;
            uint32_t lowTimeNs = 0;
            
            if (bitValue) {
                // Send a '1' bit: 0.6us high, 0.6us low
                digitalWrite(m_outputPin, HIGH);
                if (m_instrumentation.instrumentationEnabled) {
                    clock_gettime(CLOCK_MONOTONIC, &ts_high_start);
                }
                clock_gettime(CLOCK_MONOTONIC, &ts_start);
                // Wait for 0.6us (600ns)
                do {
                    clock_gettime(CLOCK_MONOTONIC, &ts_now);
                } while ((ts_now.tv_sec - ts_start.tv_sec) * 1000000000L + 
                         (ts_now.tv_nsec - ts_start.tv_nsec) < 600);
                
                if (m_instrumentation.instrumentationEnabled) {
                    clock_gettime(CLOCK_MONOTONIC, &ts_high_end);
                    highTimeNs = (ts_high_end.tv_sec - ts_high_start.tv_sec) * 1000000000L + 
                                 (ts_high_end.tv_nsec - ts_high_start.tv_nsec);
                }
                
                digitalWrite(m_outputPin, LOW);
                if (m_instrumentation.instrumentationEnabled) {
                    clock_gettime(CLOCK_MONOTONIC, &ts_low_start);
                }
                clock_gettime(CLOCK_MONOTONIC, &ts_start);
                // Wait for 0.6us (600ns)
                do {
                    clock_gettime(CLOCK_MONOTONIC, &ts_now);
                } while ((ts_now.tv_sec - ts_start.tv_sec) * 1000000000L + 
                         (ts_now.tv_nsec - ts_start.tv_nsec) < 600);
                
                if (m_instrumentation.instrumentationEnabled) {
                    clock_gettime(CLOCK_MONOTONIC, &ts_low_end);
                    lowTimeNs = (ts_low_end.tv_sec - ts_low_start.tv_sec) * 1000000000L + 
                                (ts_low_end.tv_nsec - ts_low_start.tv_nsec);
                }
            } else {
                // Send a '0' bit: 0.3us high, 0.9us low
                digitalWrite(m_outputPin, HIGH);
                if (m_instrumentation.instrumentationEnabled) {
                    clock_gettime(CLOCK_MONOTONIC, &ts_high_start);
                }
                clock_gettime(CLOCK_MONOTONIC, &ts_start);
                // Wait for 0.3us (300ns)
                do {
                    clock_gettime(CLOCK_MONOTONIC, &ts_now);
                } while ((ts_now.tv_sec - ts_start.tv_sec) * 1000000000L + 
                         (ts_now.tv_nsec - ts_start.tv_nsec) < 300);
                
                if (m_instrumentation.instrumentationEnabled) {
                    clock_gettime(CLOCK_MONOTONIC, &ts_high_end);
                    highTimeNs = (ts_high_end.tv_sec - ts_high_start.tv_sec) * 1000000000L + 
                                 (ts_high_end.tv_nsec - ts_high_start.tv_nsec);
                }
                
                digitalWrite(m_outputPin, LOW);
                if (m_instrumentation.instrumentationEnabled) {
                    clock_gettime(CLOCK_MONOTONIC, &ts_low_start);
                }
                clock_gettime(CLOCK_MONOTONIC, &ts_start);
                // Wait for 0.9us (900ns)
                do {
                    clock_gettime(CLOCK_MONOTONIC, &ts_now);
                } while ((ts_now.tv_sec - ts_start.tv_sec) * 1000000000L + 
                         (ts_now.tv_nsec - ts_start.tv_nsec) < 900);
                
                if (m_instrumentation.instrumentationEnabled) {
                    clock_gettime(CLOCK_MONOTONIC, &ts_low_end);
                    lowTimeNs = (ts_low_end.tv_sec - ts_low_start.tv_sec) * 1000000000L + 
                                (ts_low_end.tv_nsec - ts_low_start.tv_nsec);
                }
            }
            
            // Store instrumentation data if enabled and space available
            if (m_instrumentation.instrumentationEnabled && 
                m_instrumentation.numBitsCaptured < NEOPIXEL_INSTRUMENTATION_BITS) {
                unsigned int idx = m_instrumentation.numBitsCaptured;
                m_instrumentation.bitTimings[idx].highTimeNs = highTimeNs;
                m_instrumentation.bitTimings[idx].lowTimeNs = lowTimeNs;
                m_instrumentation.bitTimings[idx].bitValue = bitValue;
                m_instrumentation.bitTimings[idx].meetsSpec = 
                    CheckBitTimingSpec(highTimeNs, lowTimeNs, bitValue);
                m_instrumentation.numBitsCaptured++;
            }
        }
        
        // Update instrumentation metadata
        if (m_instrumentation.instrumentationEnabled) {
            // Store the byte value for verification (only first NEOPIXEL_MAX_CAPTURED_BYTES)
            if (m_instrumentation.byteSequenceNumber < NEOPIXEL_MAX_CAPTURED_BYTES) {
                m_instrumentation.capturedBytes |= 
                    (static_cast<uint32_t>(byte) << (m_instrumentation.byteSequenceNumber * 8));
                m_instrumentation.byteSequenceNumber++;
                
                // Calculate total transmission time for this byte
                // Note: This is cumulative across all bytes sent since instrumentation was enabled
                // Only calculate when we're still capturing bytes to minimize overhead
                clock_gettime(CLOCK_MONOTONIC, &ts_now);
                uint64_t byteTransmissionTimeNs = 
                    (ts_now.tv_sec - ts_byte_start.tv_sec) * 1000000000L + 
                    (ts_now.tv_nsec - ts_byte_start.tv_nsec);
                m_instrumentation.totalTransmissionTimeNs += byteTransmissionTimeNs;
            }
        }
#endif
}

void NeoPixelDriver::SendByteNOP(uint8_t byte) {
#ifdef EXE_MODE_RASPI
    // Option 2: Use assembly NOP instructions (deterministic timing)
    // Pros: More deterministic, no syscall overhead
    // Cons: CPU frequency dependent (calibration required)
    //
    // CALIBRATION FOR RASPBERRY PI 5 with SK6812:
    // - CPU: ARM Cortex-A76 @ 2.4GHz (4 cores)
    // - Clock cycle: ~0.42ns per cycle
    // - NOP takes ~1 cycle on ARM Cortex-A76
    // 
    // SK6812 timing calculations (includes digitalWrite overhead of ~50-100ns):
    // For 600ns: ~1300 NOPs (600ns / 0.42ns - overhead margin)
    // For 300ns: ~550 NOPs (300ns / 0.42ns - overhead margin)  
    // For 900ns: ~2000 NOPs (900ns / 0.42ns - overhead margin)
    //
    // Note: These counts compensate for digitalWrite() function call overhead
    // and are optimized for Pi 5. May need fine-tuning via oscilloscope.
    
    for (int bit = 7; bit >= 0; bit--) {
        if (byte & (1 << bit)) {
            // Send a '1' bit: 0.6us high, 0.6us low
            digitalWrite(m_outputPin, HIGH);
            // Wait ~600ns (Pi 5 @ 2.4GHz)
            for (volatile int i = 0; i < 1300; i++) {
                __asm__ __volatile__("nop");
            }
            
            digitalWrite(m_outputPin, LOW);
            // Wait ~600ns
            for (volatile int i = 0; i < 1300; i++) {
                __asm__ __volatile__("nop");
            }
        } else {
            // Send a '0' bit: 0.3us high, 0.9us low
            digitalWrite(m_outputPin, HIGH);
            // Wait ~300ns
            for (volatile int i = 0; i < 550; i++) {
                __asm__ __volatile__("nop");
            }
            
            digitalWrite(m_outputPin, LOW);
            // Wait ~900ns
            for (volatile int i = 0; i < 2000; i++) {
                __asm__ __volatile__("nop");
            }
        }
    }
#endif
}

// Instrumentation control methods
void NeoPixelDriver::EnableInstrumentation(bool enable) {
    m_instrumentation.instrumentationEnabled = enable;
    if (enable) {
        // Reset instrumentation data when enabling
        ResetInstrumentation();
    }
}

bool NeoPixelDriver::IsInstrumentationEnabled() const {
    return m_instrumentation.instrumentationEnabled;
}

void NeoPixelDriver::ResetInstrumentation() {
    // Preserve the enabled state
    bool wasEnabled = m_instrumentation.instrumentationEnabled;
    InitializeInstrumentationData();
    m_instrumentation.instrumentationEnabled = wasEnabled;
}

const stNeoPixelInstrumentation& NeoPixelDriver::GetInstrumentationData() const {
    return m_instrumentation;
}

// Helper function to initialize instrumentation data structure
void NeoPixelDriver::InitializeInstrumentationData() {
    m_instrumentation.instrumentationEnabled = false;
    m_instrumentation.numBitsCaptured = 0;
    m_instrumentation.capturedBytes = 0;
    m_instrumentation.byteSequenceNumber = 0;
    m_instrumentation.totalTransmissionTimeNs = 0;
    for (unsigned int i = 0; i < NEOPIXEL_INSTRUMENTATION_BITS; i++) {
        m_instrumentation.bitTimings[i].highTimeNs = 0;
        m_instrumentation.bitTimings[i].lowTimeNs = 0;
        m_instrumentation.bitTimings[i].meetsSpec = false;
        m_instrumentation.bitTimings[i].bitValue = false;
    }
}

// Helper function to check if bit timing meets SK6812 specification
bool NeoPixelDriver::CheckBitTimingSpec(uint32_t highTimeNs, uint32_t lowTimeNs, bool bitValue) const {
    if (bitValue) {
        // Bit 1: 800ns high (650-950ns), 450ns low (300-600ns)
        return (highTimeNs >= NEOPIXEL_BIT1_HIGH_MIN_NS && highTimeNs <= NEOPIXEL_BIT1_HIGH_MAX_NS &&
                lowTimeNs >= NEOPIXEL_BIT1_LOW_MIN_NS && lowTimeNs <= NEOPIXEL_BIT1_LOW_MAX_NS);
    } else {
        // Bit 0: 400ns high (250-550ns), 850ns low (700-1000ns)
        return (highTimeNs >= NEOPIXEL_BIT0_HIGH_MIN_NS && highTimeNs <= NEOPIXEL_BIT0_HIGH_MAX_NS &&
                lowTimeNs >= NEOPIXEL_BIT0_LOW_MIN_NS && lowTimeNs <= NEOPIXEL_BIT0_LOW_MAX_NS);
    }
}

//==============================================================================
// SPI-based NeoPixel transmission
//==============================================================================

void NeoPixelDriver::InitializeSPI() {
#ifdef EXE_MODE_RASPI
    // Validate that the output pin is SPI-capable
    if (m_spiChannel < 0) {
        // Pin is not SPI-capable - disable NeoPixels
        char msg[256];
        snprintf(msg, sizeof(msg), "NeoPixel driver %u: Pin %u is not SPI-capable. NeoPixels disabled.", 
                 m_driverIndex, m_outputPin);
        g_PBEngine.pbeSendConsole(msg);
        m_timingMethod = NEOPIXEL_TIMING_DISABLED;
        return;
    }
    
    // Initialize SPI with appropriate speed for SK6812
    // SK6812 timing: Each bit is 1.2μs (~833kHz)
    //   Bit 1: 600ns HIGH, 600ns LOW
    //   Bit 0: 300ns HIGH, 900ns LOW
    // 
    // Using 4 SPI bits per SK6812 bit:
    //   SPI clock: 3.333MHz (300ns per SPI bit)
    //   Bit 1: "1110" = 900ns HIGH, 300ns LOW (close to 600ns/600ns spec)
    //   Bit 0: "1000" = 300ns HIGH, 900ns LOW (perfect match for spec)
    const int SPI_SPEED = 3333333;  // 3.333 MHz
    
    m_spiFd = wiringPiSPISetup(m_spiChannel, SPI_SPEED);
    if (m_spiFd < 0) {
        // Failed to initialize SPI - disable NeoPixels
        char msg[256];
        snprintf(msg, sizeof(msg), "NeoPixel driver %u: Failed to initialize SPI channel %d. NeoPixels disabled.", 
                 m_driverIndex, m_spiChannel);
        g_PBEngine.pbeSendConsole(msg);
        m_timingMethod = NEOPIXEL_TIMING_DISABLED;
    }
#endif
}

void NeoPixelDriver::SendByteSPI(uint8_t byte) {
#ifdef EXE_MODE_RASPI
    // Ensure SPI is initialized
    if (m_spiFd < 0) {
        InitializeSPI();
        if (m_spiFd < 0) {
            return;  // SPI initialization failed
        }
    }
    
    // Convert each SK6812 bit to 4 SPI bits
    // Bit 1: 1110 pattern (900ns high, 300ns low at 3.333MHz)
    // Bit 0: 1000 pattern (300ns high, 900ns low at 3.333MHz)
    unsigned char spiData[4] = {0, 0, 0, 0};  // 8 SK6812 bits = 32 SPI bits = 4 bytes
    
    for (int i = 0; i < 8; i++) {
        bool bitValue = (byte & (0x80 >> i)) != 0;
        int spiBitPos = i * 4;  // Starting bit position (0, 4, 8, 12, 16, 20, 24, 28)
        int spiByteIdx = spiBitPos / 8;  // Which SPI byte (0-3)
        int bitOffset = spiBitPos % 8;   // Bit offset within that byte (0 or 4)
        
        if (bitValue) {
            // Bit 1: 1110 pattern
            spiData[spiByteIdx] |= (0b1110 << (4 - bitOffset));
        } else {
            // Bit 0: 1000 pattern
            spiData[spiByteIdx] |= (0b1000 << (4 - bitOffset));
        }
    }
    
    // Send the data via SPI
    wiringPiSPIDataRW(m_spiChannel, spiData, 4);
#endif
}

void NeoPixelDriver::CloseSPI() {
#ifdef EXE_MODE_RASPI
    if (m_spiFd >= 0) {
        // wiringPi doesn't provide explicit SPI close, just mark as closed
        m_spiFd = -1;
    }
#endif
}

void NeoPixelDriver::SendAllPixelsSPI() {
#ifdef EXE_MODE_RASPI
    // Ensure SPI is initialized
    if (m_spiFd < 0) {
        InitializeSPI();
        if (m_spiFd < 0) {
            return;  // SPI initialization failed
        }
    }
    
    // Calculate buffer size: each LED is 3 bytes (GRB), each byte needs 4 SPI bytes
    // Total: m_numLEDs * 3 * 4 bytes
    unsigned int bufferSize = m_numLEDs * 3 * 4;
    
    // Use pre-allocated SPI buffer (passed during construction)
    // Clear the buffer
    if (m_spiBuffer == nullptr) {
        return;  // No buffer available
    }
    memset(m_spiBuffer, 0, bufferSize);
    
    // Build the entire SPI data buffer
    unsigned int bufferIndex = 0;
    for (unsigned int i = 0; i < m_numLEDs; i++) {
        // Apply brightness scaling
        uint8_t brightness = m_nodes[i].stagedBrightness;
        uint8_t green = ApplyBrightness(m_nodes[i].stagedGreen, brightness);
        uint8_t red = ApplyBrightness(m_nodes[i].stagedRed, brightness);
        uint8_t blue = ApplyBrightness(m_nodes[i].stagedBlue, brightness);
        
        // Convert GRB to SPI format
        uint8_t colors[3] = {green, red, blue};
        for (int colorIdx = 0; colorIdx < 3; colorIdx++) {
            uint8_t byte = colors[colorIdx];
            
            // Convert each SK6812 bit to 4 SPI bits
            // Each bit becomes: 1=1110, 0=1000 (4 SPI bits each)
            // bitOffset is always 0 or 4 since we pack 2 SK6812 bits per SPI byte
            for (int bit = 0; bit < 8; bit++) {
                bool bitValue = (byte & (0x80 >> bit)) != 0;
                int spiBitPos = bit * 4;
                int spiByteIdx = spiBitPos / 8;
                int bitOffset = spiBitPos % 8;  // Will be 0 or 4
                
                if (bitValue) {
                    // Bit 1: 1110 pattern
                    m_spiBuffer[bufferIndex + spiByteIdx] |= (0b1110 << (4 - bitOffset));
                } else {
                    // Bit 0: 1000 pattern
                    m_spiBuffer[bufferIndex + spiByteIdx] |= (0b1000 << (4 - bitOffset));
                }
            }
            
            bufferIndex += 4;  // Move to next 4-byte block
        }
    }
    
    // Send the entire buffer in one SPI transaction
    wiringPiSPIDataRW(m_spiChannel, m_spiBuffer, bufferSize);
#endif
}

//==============================================================================
// Reset/Latch signal
//==============================================================================

void NeoPixelDriver::SendReset() {
#ifdef EXE_MODE_RASPI
    // SK6812 requires >80us low signal to latch the data
    
    switch (m_timingMethod) {
        case NEOPIXEL_TIMING_SPI:
            // For SPI mode, send enough zero bytes to create >80us low period
            // At 3.333MHz SPI, each byte takes ~2.4us, so send 34 bytes = ~81.6us
            if (m_spiFd >= 0) {
                unsigned char resetData[34] = {0};  // 34 bytes of zeros
                write(m_spiFd, resetData, sizeof(resetData));
            }
            break;
            
        case NEOPIXEL_TIMING_CLOCKGETTIME:
        case NEOPIXEL_TIMING_NOP:
            // For bit-banging modes, direct GPIO control works fine
            digitalWrite(m_outputPin, LOW);
            delayMicroseconds(80);  // 80us reset time
            break;
            
        case NEOPIXEL_TIMING_DISABLED:
        default:
            // Do nothing if disabled
            break;
    }
#endif
}
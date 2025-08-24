// PInball_IO.cpp:  Shared input / output structures used the pinball engine
// These need to be defined by the user for the specific table

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

// NOTE:  This IO file is to be used with the basic PInball breakout box setup, (see HW schematics /hw/SimpleBreakoutSchematic.png)
// All active connections use the GPIO pins on the Raspberry Pi directily, rather than I2C and expansion ICs

#include "PInball_IO.h"
#include "PBBuildSwitch.h"

#ifdef EXE_MODE_RASPI
#include "wiringPi.h"
#include "wiringPiI2C.h"
#endif

// Output definitions
stOutputDef g_outputDef[] = {
    {"IO0P8 Sling Shot", PB_OUTPUT_SLINGSHOT, IDO_SLINGSHOT, 8, PB_IO, 0, PB_OFF},
    {"IO1P8 Pop Bumper", PB_OUTPUT_POPBUMPER, IDO_POPBUMPER, 8, PB_IO, 1, PB_OFF},
    {"IO2P8 Ball Eject", PB_OUTPUT_BALLEJECT, IDO_BALLEJECT, 8, PB_IO, 2, PB_OFF},
    {"Start LED", PB_OUTPUT_LED, IDO_LED1, 23, PB_RASPI, 0, PB_OFF},
    {"LED0P8 LED", PB_OUTPUT_LED, IDO_LED2, 8, PB_LED, 0, PB_ON},
    {"LED1P8 LED", PB_OUTPUT_LED, IDO_LED3, 8, PB_LED, 1, PB_ON},
    {"LED2P8 LED", PB_OUTPUT_LED, IDO_LED4, 8, PB_LED, 2, PB_ON}
};

// Input definitions
stInputDef g_inputDef[] = {
    {"Left Flipper", "A", PB_INPUT_BUTTON, IDI_LEFTFLIPPER, 27, PB_RASPI, 0, PB_OFF, 0, 0, 4},
    {"Right Flipper", "D", PB_INPUT_BUTTON, IDI_RIGHTFLIPPER, 17, PB_RASPI, 0, PB_OFF, 0, 0, 4},
    {"Left Activate", "Q", PB_INPUT_BUTTON, IDI_LEFTACTIVATE, 5, PB_RASPI, 0, PB_OFF, 0, 0, 4},
    {"Right Activate", "E", PB_INPUT_BUTTON, IDI_RIGHTACTIVATE,22, PB_RASPI, 0, PB_OFF, 0, 0, 4},
    {"Start", "Z", PB_INPUT_BUTTON, IDI_START, 6, PB_RASPI, 0, PB_OFF, 0, 0, 4},
    {"IO0P7", "1", PB_INPUT_BUTTON, IDI_SENSOR1, 7, PB_IO, 0, PB_OFF, 0, 0, 4},
    {"IO1P7", "2", PB_INPUT_BUTTON, IDI_SENSOR2, 7, PB_IO, 1, PB_OFF, 0, 0, 4},
    {"IO2P7", "3", PB_INPUT_BUTTON, IDI_SENSOR3, 7, PB_IO, 2, PB_OFF, 0, 0, 4},
};

// LEDDriver Class Implementation for TLC59116 LED Driver Chip

LEDDriver::LEDDriver(uint8_t address) : m_address(address), m_i2cFd(-1), m_groupMode(GroupModeDimming) {
    // Initialize brightness and control arrays
    for (int i = 0; i < 16; i++) {
        m_ledBrightness[i] = 0xFF;  // Start with all LEDs with maximum brightness
        m_pwmStaged[i] = false;     // No PWM changes staged initially
    }
    for (int i = 0; i < 4; i++) {
        m_ledControl[i] = 0x00;     // Start with all LEDs in OFF state
        m_ledOutStaged[i] = false;  // No LEDOUT changes staged initially
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

void LEDDriver::StageLEDControl(bool setAll, unsigned int LEDIndex, LEDState state) {
    
    uint8_t controlValue = GetControlValue(state);

    if (setAll) {
        // Set all LEDs to the same state    // Each register controls 4 LEDs (2 bits per LED)
        uint8_t regValue = (controlValue << 6) | (controlValue << 4) | (controlValue << 2) | controlValue;
        for (int i = 0; i < 4; i++) {
            m_ledControl[i] = regValue;
            m_ledOutStaged[i] = true;  // Mark this LEDOUT register as staged
        }
    } else {
        // Set specific LED
        if (LEDIndex < 16) {
            uint8_t regIndex = LEDIndex / 4;      // Which LEDOUT register (0-3)
            uint8_t bitPos = (LEDIndex % 4) * 2;  // Bit position within register (0,2,4,6)
            
            // Clear the 2 bits for this LED and set new value
            m_ledControl[regIndex] &= ~(0x03 << bitPos);
            m_ledControl[regIndex] |= (controlValue << bitPos);
            m_ledOutStaged[regIndex] = true;  // Mark this LEDOUT register as staged
        }
    }
}

void LEDDriver::StageLEDBrightness(bool setAll, unsigned int LEDIndex, uint8_t brightness) {
    if (setAll) {
        // Set all LEDs to the same brightness
        for (int i = 0; i < 16; i++) {
            m_ledBrightness[i] = brightness;
            m_pwmStaged[i] = true;  // Mark this PWM register as staged
        }
    } else {
        // Set specific LED brightness
        if (LEDIndex < 16) {
            m_ledBrightness[LEDIndex] = brightness;
            m_pwmStaged[LEDIndex] = true;  // Mark this PWM register as staged
        }
    }
}

void LEDDriver::SendStagedLED() {
#ifdef EXE_MODE_RASPI
    if (m_i2cFd >= 0) {
        // Send only staged PWM brightness values
        for (int i = 0; i < 16; i++) {
            if (m_pwmStaged[i]) {
                wiringPiI2CWriteReg8(m_i2cFd, TLC59116_PWM0 + i, m_ledBrightness[i]);
                m_pwmStaged[i] = false;  // Clear the staged flag after sending
            }
        }
        
        // Send only staged LEDOUT control values
        for (int i = 0; i < 4; i++) {
            if (m_ledOutStaged[i]) {
                wiringPiI2CWriteReg8(m_i2cFd, TLC59116_LEDOUT0 + i, m_ledControl[i]);
                m_ledOutStaged[i] = false;  // Clear the staged flag after sending
            }
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

//==============================================================================
// IODriver Class Implementation for TCA9555 I/O Expander Chip
//==============================================================================

IODriver::IODriver(uint8_t address, uint16_t inputMask) : m_address(address), m_i2cFd(-1), m_inputMask(inputMask) {
    // Initialize output values and staging flags
    for (int i = 0; i < 2; i++) {
        m_outputValues[i] = 0x00;      // Start with all outputs low
        m_outputStaged[i] = false;     // No changes staged initially
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
    m_outputValues[0] = (uint8_t)(value & 0xFF);         // Lower 8 bits for port 0
    m_outputValues[1] = (uint8_t)((value >> 8) & 0xFF);  // Upper 8 bits for port 1
    
    // Mark both ports as having staged changes
    m_outputStaged[0] = true;
    m_outputStaged[1] = true;
}

void IODriver::StageOutputPin(uint8_t pinIndex, PBPinState value) {
    if (pinIndex < 16) {
        uint8_t port = pinIndex / 8;      // Which port (0 or 1)
        uint8_t bitPos = pinIndex % 8;    // Bit position within port (0-7)
        
        if (value == PB_ON) {
            // Set the bit
            m_outputValues[port] |= (1 << bitPos);
        } else {
            // Clear the bit
            m_outputValues[port] &= ~(1 << bitPos);
        }
        
        // Mark this port as having staged changes
        m_outputStaged[port] = true;
    }
}

void IODriver::SendStagedOutput() {
#ifdef EXE_MODE_RASPI
    if (m_i2cFd >= 0) {
        // Send only staged output values
        for (int i = 0; i < 2; i++) {
            if (m_outputStaged[i]) {
                wiringPiI2CWriteReg8(m_i2cFd, TCA9555_OUTPUT_PORT0 + i, m_outputValues[i]);
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
    
    // MAX9744 has 64 volume levels (0x00 to 0x3F)
    // Map 1-100% to 0x01-0x3F
    // Formula: ((percent * 63) / 100) + 1
    uint8_t value = ((percent * 63) / 100) + 1;
    
    // Ensure we don't exceed 0x3F (63 decimal)
    if (value > 0x3F) {
        value = 0x3F;
    }
    
    return value;
}
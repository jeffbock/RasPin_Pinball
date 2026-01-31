
// Pinball:  A complete pinball framework for building 1/2 scale phyical pinball machines with Raspberry Pi

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

// Pinball.cpp is the main entry and control flow for the RasPi Pinball Engine.
// It includes the primary global PBEngine Class, and I/O functions for Windows and Raspberry Pi, all all intial setup and initialization flows.
// Selection of the platform is done through the EXE_MODE_RASPI or EXE_MODE_WINDOWS defines in PBuildSwitch.h

// All I/O is done thorugh input and output message queues.  The pinball engine consumes input messages and produces output messages.
// The I/O process is kept separate from the engine to allow for different I/O methods on different platforms, and potentially multi-threading in the future.
// The Raspberry Pi IO and LED chip managemeent is designed to minimize I2C traffic, as well as independently handle messages from the engine. 
// As much as possible, the RasPi engine is "fire and forget" to the I/O chips, and complexities are managed in the this main I/O file.
// All IO and LED Pin and behavior definitions can be found in Pinball_IO.cpp and Pinball_IO.h and should be defined as needed for the specific table.

// Key IO Points
// - All IO chips are handled through the IODriverDebounce and LEDDriver classes, which manage the low level I2C and state tracking
// - The IO loop is designed to be called as often as possible, and will read inputs and process outputs as quickly as possible
//   - Includes capability for automatic input to output message generation (eg: flipper button results in immediate output to flipper)
// - Input pins are debounced in the IODriverDebounce class, which is used for each IO chip, as well as for the Raspberry Pi GPIO pins
// - Input messages are generated when a pin state changes, and placed on the input message queue.
// - Output messages are placed on the output message queue by the engine, and processed in the IO loop
//    - IO and LED Messages support pulsing, which means a defined on/off time when until completed, other output requests are ignored (good for solenoids)
//    - LED Message support On, Off, Automatic Blink and Automatic Brightness
//    - LED system supports "sequences" which are pre-defined patterns of LED states and brightness that can be started and stopped by the engine
//      - When sequences are active, LED messages are put in a defferred queue and processed when the sequence is stopped

#include "Pinball.h"
#include <string.h>
#include <limits.h>
#include <errno.h>

#ifdef EXE_MODE_WINDOWS
#include <direct.h>
#include <stdlib.h>
#define getcwd _getcwd
#define chdir _chdir
// Define PATH_MAX using Windows MAX_PATH value
#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#endif

#ifdef EXE_MODE_RASPI
#include <unistd.h>
// PATH_MAX should be defined in limits.h on POSIX systems, but provide fallback
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#endif

 // Global pinball engine object
PBEngine g_PBEngine;

// Check and adjust working directory if needed
// Returns true on success, false on failure
bool AdjustWorkingDirectory(const char* exePath) {
    // Check current working directory and change if necessary
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        std::cerr << "ERROR: Failed to get current working directory: " << strerror(errno) << std::endl;
        return false;
    }
    
    // Extract the directory path from the executable path
    std::string exePathStr(exePath);
    size_t lastSlash = exePathStr.find_last_of("/\\");
    std::string exeDir;
    
    if (lastSlash != std::string::npos) {
        exeDir = exePathStr.substr(0, lastSlash);
    } else {
        exeDir = ".";
    }
    
    // Get absolute path of exe directory
    char absExeDir[PATH_MAX];
    #ifdef EXE_MODE_WINDOWS
    if (_fullpath(absExeDir, exeDir.c_str(), PATH_MAX) == NULL) {
    #endif
    #ifdef EXE_MODE_RASPI
    if (realpath(exeDir.c_str(), absExeDir) == NULL) {
    #endif
        std::cerr << "ERROR: Failed to resolve executable directory path: " << strerror(errno) << std::endl;
        return false;
    }
    
    // Compare current working directory with executable directory
    std::string cwdStr(cwd);
    std::string absExeDirStr(absExeDir);
    
    if (cwdStr == absExeDirStr) {
        // Current directory is the same as executable directory, change to parent
        
        // Get parent directory
        size_t lastSlashInExeDir = absExeDirStr.find_last_of("/\\");
        std::string parentDir;
        
        if (lastSlashInExeDir != std::string::npos) {
            parentDir = absExeDirStr.substr(0, lastSlashInExeDir);
        } else {
            parentDir = "..";
        }
        
        // Change to parent directory
        if (chdir(parentDir.c_str()) != 0) {
            std::cerr << "ERROR: Failed to change working directory to parent: " << strerror(errno) << std::endl;
            return false;
        }
    }

    // Get the new working directory after change and output it
    char newCwd[PATH_MAX];
    if (getcwd(newCwd, sizeof(newCwd)) != NULL) {
      g_PBEngine.pbeSendConsole("RasPin: Confirmed correct working directory: " + std::string(newCwd));
    }

    return true;
}

// Version display function
void ShowVersion() {
    std::string versionStr = "RasPin Pinball Engine v" + 
                           std::to_string(PB_VERSION_MAJOR) + "." + 
                           std::to_string(PB_VERSION_MINOR) + "." + 
                           std::to_string(PB_VERSION_BUILD);
    g_PBEngine.pbeSendConsole(versionStr);
}

// Windows startup and render code
#ifdef EXE_MODE_WINDOWS
#include "PBWinRender.h"

HWND g_WHND = NULL;

// Init the render system for Windows
bool PBInitRender (long width, long height) {

g_WHND = PBInitWinRender (width, height);
if (g_WHND == NULL) return (false);

// For windows, OGLNativeWindows type is HWND
if (!g_PBEngine.oglInit (width, height, g_WHND)) return (false);

if (!g_PBEngine.gfxInit()) return (false);

// Initialize sound system
g_PBEngine.m_soundSystem.pbsInitialize();

return (true);
}

// Tranlates windows keys into PBEngine input messages
void PBWinSimInput(std::string character, PBPinState inputState, stInputMessage* inputMessage){

    // Find the character in the input definition global
    for (int i = 0; i < NUM_INPUTS; i++) {
        if (g_inputDef[i].simMapKey == character) {
            inputMessage->inputMsg = g_inputDef[i].inputMsg;
            inputMessage->inputId = i;  // Use array index as ID
            inputMessage->inputState = inputState;
            inputMessage->sentTick = g_PBEngine.GetTickCountGfx();

            // Update the various state items for the input, could be used by the progam later
            g_inputDef[i].lastState = inputState;
            g_inputDef[i].lastStateTick = inputMessage->sentTick;

            return;
        }
    }
}

// Process input for Windows
// Returns true as long as the application should continue running
// This will be need to be expanding to take input for the windows simluator
bool PBProcessInput() {

    // Process Windows Messages
    MSG msg;
    
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return (false);

        TranslateMessage(&msg);
        DispatchMessage(&msg);

        // Handle WM_KEYDOWN message
        if ((msg.message == WM_KEYDOWN) || (msg.message == WM_KEYUP)) {
            // Get the virtual key code
            WPARAM key = msg.wParam;

            // Convert the virtual key code to a string
            char character = MapVirtualKey(key, MAPVK_VK_TO_CHAR);
            std::string temp = "";
            temp += character;
            
            // Check if the key press is an auto-repeat
            bool isAutoRepeat = (msg.lParam & (1 << 30)) != 0;

            // Simulate receiving a message
            if ((!isAutoRepeat) || (msg.message == WM_KEYUP)) {
                stInputMessage inputMessage;
                // Translate input keys to PBMessages and place on message queue
                if (msg.message == WM_KEYDOWN) PBWinSimInput(temp, PB_ON, &inputMessage);
                if (msg.message == WM_KEYUP) PBWinSimInput(temp, PB_OFF, &inputMessage);

                g_PBEngine.m_inputQueue.push(inputMessage);
            }
        }
    }
    return (true);
}

bool PBProcessOutput() {
    return (true);
}

bool PBProcessIO() {

    PBProcessInput();
    PBProcessOutput();
    return (true);
}

#endif

// Raspeberry Pi startup and render code
#ifdef EXE_MODE_RASPI
#include "PBRasPiRender.h"

EGLNativeWindowType g_PiWindow;

bool PBInitRender (long width, long height) {

g_PiWindow = PBInitPiRender (width, height);
if (g_PiWindow == 0) return (false);

// For Rasberry Pi, OGLNativeWindows type is TBD
if (!g_PBEngine.oglInit (width, height, g_PiWindow)) return (false);

if (!g_PBEngine.gfxInit()) return (false);

// Initialize sound system
g_PBEngine.m_soundSystem.pbsInitialize();

return true;

}

// Reads all the inputs as defined by the input map from Raspberry Pi and any I/O chips and returns the values.

bool  PBProcessInput() {

    static stInputMessage inputMessage;
    static int IOReadValue[NUM_IO_CHIPS] = {0};

    // Loop through all the inputs in m_inputPiMap and, read them, check for state change, and send a message if changed

    // Read the Raspberry Pi inputs first
    for (auto& inputPair : g_PBEngine.m_inputPiMap) {
        int inputId = inputPair.first;
        cDebounceInput& input = inputPair.second;

        // Read the current state of the input
        int currentState = input.readPin();
        inputMessage.sentTick = g_PBEngine.GetTickCountGfx();

        // inputId is the array index (pre-validated, no bounds check needed)
        int inputDefIndex = inputId;

        // Check if the state has changed
        if (currentState != g_inputDef[inputDefIndex].lastState) {
            // Create an input message
            inputMessage.inputMsg = g_inputDef[inputDefIndex].inputMsg;
            inputMessage.inputId = inputDefIndex;  // Use array index as ID
            if (currentState == 0) {
                inputMessage.inputState = PB_ON;
                g_inputDef[inputDefIndex].lastState = PB_ON;
            }
            else{
                inputMessage.inputState = PB_OFF;
                g_inputDef[inputDefIndex].lastState = PB_OFF;
            }
            
            g_PBEngine.m_inputQueue.push(inputMessage);
            
            // Check if autoOutput is enabled globally and for this input
            if (g_PBEngine.GetAutoOutputEnable() && g_inputDef[inputDefIndex].autoOutput) {
                // Get output type for autoOutputId (array index - no bounds check needed)
                unsigned int autoOutputId = g_inputDef[inputDefIndex].autoOutputId;
                PBOutputMsg outputType = g_outputDef[autoOutputId].outputMsg;
                
                // Send output message with current input state (autoPinState can be used to invert if needed)
                PBPinState outputState = (g_inputDef[inputDefIndex].autoPinState == PB_ON) ? inputMessage.inputState : 
                                         (inputMessage.inputState == PB_ON ? PB_OFF : PB_ON);
                // Use pulse mode based on autoOutputUsePulse field
                g_PBEngine.SendOutputMsg(outputType, g_inputDef[inputDefIndex].autoOutputId, outputState, g_inputDef[inputDefIndex].autoOutputUsePulse);
            }
        }
    }
    
    // Read each IODriver and place it in the array value
    for (int i = 0; i < NUM_IO_CHIPS; i++) {
        IOReadValue[i] = g_PBEngine.m_IOChip[i].ReadInputsDB();
    }

    inputMessage.sentTick = g_PBEngine.GetTickCountGfx();

    // Loop through the g_inputDef values, and for each PB_IO input, check the corresponding read value and pin
    for (int i = 0; i < NUM_INPUTS; i++) {
        if (g_inputDef[i].boardType == PB_IO) {
            int chip = g_inputDef[i].boardIndex;
            int pin = g_inputDef[i].pin;
            int mask = 1 << pin;
            PBPinState pinState = (IOReadValue[chip] & mask) ? PB_OFF : PB_ON;

            // Check if the state has changed
            if (pinState != g_inputDef[i].lastState) {
                // Create an input message
                inputMessage.inputMsg = g_inputDef[i].inputMsg;
                inputMessage.inputId = i;  // Use array index as ID
                inputMessage.inputState = pinState;
                
                // Update the last state
                g_inputDef[i].lastState = pinState;

                // Push the message to the queue
                g_PBEngine.m_inputQueue.push(inputMessage);
                
                // Check if autoOutput is enabled globally and for this input
                if (g_PBEngine.GetAutoOutputEnable() && g_inputDef[i].autoOutput) {
                    // Get output type for autoOutputId (which is now also an array index)
                    unsigned int autoOutputId = g_inputDef[i].autoOutputId;
                    PBOutputMsg outputType = g_outputDef[autoOutputId].outputMsg;
                    
                    // Send output message with current input state (autoPinState can be used to invert if needed)
                    PBPinState outputState = (g_inputDef[i].autoPinState == PB_ON) ? inputMessage.inputState : 
                                             (inputMessage.inputState == PB_ON ? PB_OFF : PB_ON);
                    // Use pulse mode based on autoOutputUsePulse field
                    g_PBEngine.SendOutputMsg(outputType, g_inputDef[i].autoOutputId, outputState, g_inputDef[i].autoOutputUsePulse);
                }
            }
        }
    }
            
    return (true);
}

// Process output messages from the output queue, sending them to the appropriate hardware
// Key Points - there are two deferred modes for output messages - Pulse outputs and LED sequences
//   Pulse Outputs:  Once fired, the pulse must be completed (fixed on/off times).  Any message for that outputId is ignored until the pulse is complete
//                   These are intended to used for things like slingshots, which may need to ignore messages, inputs while the pulse is active
//
//   LED Sequence:  Can be set per LED control chip.  Once started, all LED output messages for that chip put into a deferred queue until the sequence is stopped
//                  The deferred queue is processed in the same order as the messages were received once the sequence is stopped
//
// Note: LED and IO chips are most efficient at managing I2C as HW transactions will only happen when there are changes to the staged values
//       Raspberry PI GPIOs and LED Config changes are generally sent immediately as there is no staging capability - so use these messages sparingly

bool PBProcessOutput() {

    // Process all messages from the output queue
    while (!g_PBEngine.m_outputQueue.empty()) {    
        stOutputMessage tempMessage = g_PBEngine.m_outputQueue.front();
        g_PBEngine.m_outputQueue.pop();
        
        // Fix the options pointer to point to our copied data
        if (tempMessage.hasOptions) {
            tempMessage.options = &tempMessage.optionsCopy;
        }

        // Find the output definition that matches this outputId
        int outputDefIndex = FindOutputDefIndex(tempMessage.outputId);

        // If we found a matching output definition, process it
        if (outputDefIndex != -1) {
            stOutputDef& outputDef = g_outputDef[outputDefIndex];
            bool skipProcessing = false;
            
            // Handle different message types
            switch (tempMessage.outputMsg) {
                case PB_OMSG_GENERIC_IO:
                    if (outputDef.boardType == PB_IO || outputDef.boardType == PB_RASPI) ProcessIOOutputMessage(tempMessage, outputDef);
                    break;
                case PB_OMSG_LED:
                case PB_OMSG_LEDSET_BRIGHTNESS:
                    if (outputDef.boardType == PB_LED) ProcessLEDOutputMessage(tempMessage, outputDef);
                    break;
                case PB_OMSG_LEDCFG_GROUPDIM:
                case PB_OMSG_LEDCFG_GROUPBLINK:
                    if (outputDef.boardType == PB_LED) ProcessLEDConfigMessage(tempMessage, outputDef);
                    break;
                case PB_OMSG_LED_SEQUENCE:
                    ProcessLEDSequenceMessage(tempMessage);
                    skipProcessing = true;
                    break;
                case PB_OMSG_NEOPIXEL:
                    if (outputDef.boardType == PB_NEOPIXEL) ProcessNeoPixelOutputMessage(tempMessage, outputDef);
                    break;
                case PB_OMSG_NEOPIXEL_SEQUENCE:
                    ProcessNeoPixelSequenceMessage(tempMessage);
                    skipProcessing = true;
                    break;
                default:
                    break;
            }
            
            if (skipProcessing) continue;  // Jump to the next message
        }
    }
    
    // Process pulse outputs from the pulse map
    ProcessActivePulseOutputs();
    
    // Send all staged outputs to IODriver chips
    SendAllStagedIO();
    
    // Handle LED sequence processing or deferred queue
    if (g_PBEngine.m_LEDSequenceInfo.sequenceEnabled) {
        ProcessActiveLEDSequence();
    } else {
        ProcessDeferredLEDQueue();
    }
    
    // Send all staged outputs to LED chips
    SendAllStagedLED();
    
    // Handle NeoPixel sequence processing for each driver
    for (auto& pair : g_PBEngine.m_NeoPixelSequenceMap) {
        if (pair.second.sequenceEnabled) {
            ProcessActiveNeoPixelSequence(pair.first);
        }
    }
    
    // Send all staged outputs to NeoPixel drivers
    SendAllStagedNeoPixels();
    
    return true;
}

// Helper function to find output definition index by outputId
// Since outputId is the array index (pre-validated), this is trivial
int FindOutputDefIndex(unsigned int outputId) {
    return outputId;
}

// Helper function to send all staged IO outputs to hardware
void SendAllStagedIO() {
    for (int i = 0; i < NUM_IO_CHIPS; i++) {
        g_PBEngine.m_IOChip[i].SendStagedOutput();
    }
}

// Helper function to send all staged LED outputs to hardware
void SendAllStagedLED() {
    for (int i = 0; i < NUM_LED_CHIPS; i++) {
        g_PBEngine.m_LEDChip[i].SendStagedLED();
    }
}

// Process LED sequence start/stop messages
void ProcessLEDSequenceMessage(const stOutputMessage& message) {
    if (message.outputState == PB_ON && message.options != nullptr) {
        // Start LED sequence mode
        unsigned long currentTick = g_PBEngine.GetTickCountGfx();
        bool sequenceAlreadyActive = g_PBEngine.m_LEDSequenceInfo.sequenceEnabled;
        
        g_PBEngine.m_LEDSequenceInfo.sequenceEnabled = true;
        g_PBEngine.m_LEDSequenceInfo.firstTime = true;
        g_PBEngine.m_LEDSequenceInfo.sequenceStartTick = currentTick;
        g_PBEngine.m_LEDSequenceInfo.stepStartTick = currentTick;
        g_PBEngine.m_LEDSequenceInfo.currentSeqIndex = 0;
        g_PBEngine.m_LEDSequenceInfo.previousSeqIndex = -1; // Initialize to indicate never set
        g_PBEngine.m_LEDSequenceInfo.indexStep = 1;
        
        g_PBEngine.m_LEDSequenceInfo.loopMode = message.options->loopMode;
        g_PBEngine.m_LEDSequenceInfo.pLEDSequence = const_cast<LEDSequence*>(message.options->setLEDSequence);
            
        // Copy activeLEDMask from output options to sequence info
        for (int chipIndex = 0; chipIndex < NUM_LED_CHIPS; chipIndex++) {
            g_PBEngine.m_LEDSequenceInfo.activeLEDMask[chipIndex] = message.options->activeLEDMask[chipIndex];
        }
        
        // Only save hardware state if no sequence was already active
        // We want to preserve the original pre-sequence state, not intermediate sequence states
        if (!sequenceAlreadyActive) {
            // Send all staged LED values to hardware BEFORE saving state
            // This ensures we capture the actual current hardware state, not pending staged changes
            SendAllStagedLED();
            
            // Now save the current hardware state for later restoration
            for (int chipIndex = 0; chipIndex < NUM_LED_CHIPS; chipIndex++) {
                // Save all 4 LED control registers for later restore 
                for (int regIndex = 0; regIndex < 4; regIndex++) {
                    g_PBEngine.m_LEDSequenceInfo.previousLEDValues[chipIndex][regIndex] = g_PBEngine.m_LEDChip[chipIndex].ReadLEDControl(CurrentHW, regIndex);
                }
            }
        }
        
        // Clear pulse map and stage LEDs to OFF for all active pins
        for (int chipIndex = 0; chipIndex < NUM_LED_CHIPS; chipIndex++) {
            // For each active pin, clear it from pulse map and stage to OFF
            uint16_t activeMask = g_PBEngine.m_LEDSequenceInfo.activeLEDMask[chipIndex];
            for (int pin = 0; pin < 16; pin++) {
                if (activeMask & (1 << pin)) {
                    // Find output ID for this chip/pin combination
                    for (size_t i = 0; i < NUM_OUTPUTS; i++) {
                        if (g_outputDef[i].boardType == PB_LED && 
                            g_outputDef[i].boardIndex == chipIndex && 
                            g_outputDef[i].pin == pin) {
                            
                            // Remove from pulse map if present (using array index as ID)
                            auto pulseIt = g_PBEngine.m_outputPulseMap.find(i);
                            if (pulseIt != g_PBEngine.m_outputPulseMap.end()) {
                                g_PBEngine.m_outputPulseMap.erase(pulseIt);
                            }
                            
                            // Stage LED to OFF
                            g_PBEngine.m_LEDChip[chipIndex].StageLEDControl(false, pin, LEDOff);
                            g_outputDef[i].lastState = PB_OFF;
                            break;
                        }
                    }
                }
            }
        }
} else {
    // Stop LED sequence mode
    EndLEDSequence();
}
}

// Process IO and RASPI output messages
void ProcessIOOutputMessage(const stOutputMessage& message, stOutputDef& outputDef) {
    // Check if it's currently in the pulse output map - if so, ignore this message
    if (g_PBEngine.m_outputPulseMap.find(message.outputId) != g_PBEngine.m_outputPulseMap.end()) {
        return;
    }
    
    // Check if it's a pulse output
    bool isPulseOutput = message.usePulse && (outputDef.onTimeMS > 0 || outputDef.offTimeMS > 0);
    
    if (isPulseOutput) {
        // Put it in the pulse output map
        stOutputPulse pulse;
        pulse.outputId = message.outputId;
        pulse.onTimeMS = outputDef.onTimeMS;
        pulse.offTimeMS = outputDef.offTimeMS;
        pulse.startTickMS = message.sentTick;
        g_PBEngine.m_outputPulseMap[message.outputId] = pulse;
    } else {
        // Not a pulse output - stage or send immediately
        // All logic is active low (so ON = LOW, OFF = HIGH)
        int outputValue = (message.outputState == PB_OFF) ? HIGH : LOW;
        
        if (outputDef.boardType == PB_RASPI) {
            // Send immediately to GPIO pin
            digitalWrite(outputDef.pin, outputValue);
        } else if (outputDef.boardType == PB_IO) {
            // Stage the output value to the appropriate IODriver chip
            if (outputDef.boardIndex < NUM_IO_CHIPS) {
                g_PBEngine.m_IOChip[outputDef.boardIndex].StageOutputPin(outputDef.pin, message.outputState);
            }
        }
        
        // Update the lastState in the output definition
        outputDef.lastState = message.outputState;
    }
}

// Process LED output messages
void ProcessLEDOutputMessage(const stOutputMessage& message, stOutputDef& outputDef, bool skipSequenceCheck) {
    // Check if LED sequence is active for this specific chip/pin (unless skip is requested)
    if (!skipSequenceCheck) {
        bool sequenceActiveForPin = false;
        if (g_PBEngine.m_LEDSequenceInfo.sequenceEnabled && 
            outputDef.boardIndex < NUM_LED_CHIPS) {
            // Check if this specific pin is in the active LED mask
            sequenceActiveForPin = (g_PBEngine.m_LEDSequenceInfo.activeLEDMask[outputDef.boardIndex] & (1 << outputDef.pin)) != 0;
        }
        
        if (sequenceActiveForPin) {
            // Push message to deferred queue, but check size limit
            if (g_PBEngine.m_deferredQueue.size() < MAX_DEFERRED_LED_QUEUE) {
                g_PBEngine.m_deferredQueue.push(message);
            }
            // Drop message if queue is full
            return;
        }
    }

     // Check if it's currently in the pulse output map - if so, ignore this message
    if (g_PBEngine.m_outputPulseMap.find(message.outputId) != g_PBEngine.m_outputPulseMap.end()) {
        return;
    }
    
    // Check if it's a pulse output
    bool isPulseOutput = message.usePulse && (outputDef.onTimeMS > 0 || outputDef.offTimeMS > 0);
    
    if (isPulseOutput) {
        // Put it in the pulse output map
        stOutputPulse pulse;
        pulse.outputId = message.outputId;
        pulse.onTimeMS = outputDef.onTimeMS;
        pulse.offTimeMS = outputDef.offTimeMS;
        pulse.startTickMS = message.sentTick;
        g_PBEngine.m_outputPulseMap[message.outputId] = pulse;
    } else {
        // Handle regular LED pin control
        if (message.outputMsg == PB_OMSG_LED) {
            // Stage the LED control to the appropriate LED chip
            if (outputDef.boardIndex < NUM_LED_CHIPS) {
                LEDState ledState = (message.outputState == PB_ON) ? LEDOn : LEDOff;
                g_PBEngine.m_LEDChip[outputDef.boardIndex].StageLEDControl(false, outputDef.pin, ledState);
            }
            
            // Update the lastState in the output definition
            outputDef.lastState = message.outputState;
        }
        else if (message.outputMsg == PB_OMSG_LEDSET_BRIGHTNESS) {
            // Stage the LED brightness to the appropriate LED chip
            if (outputDef.boardIndex < NUM_LED_CHIPS) {
                uint8_t brightness = message.options ? message.options->brightness : 255;
                g_PBEngine.m_LEDChip[outputDef.boardIndex].StageLEDBrightness(false, outputDef.pin, brightness);
            }
        }
    }
}

// Process LED configuration messages that write immediately
void ProcessLEDConfigMessage(const stOutputMessage& message, stOutputDef& outputDef) {
    // LED config messages always write immediately, no staging
    if (outputDef.boardIndex < NUM_LED_CHIPS) {
        if (message.outputMsg == PB_OMSG_LEDCFG_GROUPDIM) {
            unsigned int brightness = message.options ? message.options->brightness : 255;
            g_PBEngine.m_LEDChip[outputDef.boardIndex].SetGroupMode(GroupModeDimming, brightness, 0, 0);
        } else if (message.outputMsg == PB_OMSG_LEDCFG_GROUPBLINK) {
            unsigned int onTime = message.options ? message.options->onBlinkMS : 500;
            unsigned int offTime = message.options ? message.options->offBlinkMS : 500;
            g_PBEngine.m_LEDChip[outputDef.boardIndex].SetGroupMode(GroupModeBlinking, 0, onTime, offTime);
        }
    }
}

// Process active pulse outputs and manage their timing
void ProcessActivePulseOutputs() {
    auto pulseIt = g_PBEngine.m_outputPulseMap.begin();
    while (pulseIt != g_PBEngine.m_outputPulseMap.end()) {
        stOutputPulse& pulse = pulseIt->second;
        unsigned long currentTime = g_PBEngine.GetTickCountGfx();
        unsigned long elapsedTime = currentTime - pulse.startTickMS;
        
        // Find the output definition for this pulse
        int outputDefIndex = FindOutputDefIndex(pulse.outputId);
        
        if (outputDefIndex != -1) {
            stOutputDef& outputDef = g_outputDef[outputDefIndex];
            bool pulseComplete = false;
            
            if (elapsedTime < pulse.onTimeMS) {
                // ON phase
                if (outputDef.boardType == PB_RASPI) {
                    int outputValue = LOW;
                    digitalWrite(outputDef.pin, outputValue);
                } else if (outputDef.boardType == PB_IO && outputDef.boardIndex < NUM_IO_CHIPS) {
                    g_PBEngine.m_IOChip[outputDef.boardIndex].StageOutputPin(outputDef.pin, PB_ON);
                } else if (outputDef.boardType == PB_LED && outputDef.boardIndex < NUM_LED_CHIPS) {
                    // Stage the LED ON to the appropriate LED chip
                    g_PBEngine.m_LEDChip[outputDef.boardIndex].StageLEDControl(false, outputDef.pin, LEDOn);
                }
                outputDef.lastState = PB_ON;
            } else if (elapsedTime >= pulse.onTimeMS ) {
                // OFF phase
                if (outputDef.boardType == PB_RASPI) {
                    int outputValue = HIGH;
                    digitalWrite(outputDef.pin, outputValue);
                } else if (outputDef.boardType == PB_IO && outputDef.boardIndex < NUM_IO_CHIPS) {
                    g_PBEngine.m_IOChip[outputDef.boardIndex].StageOutputPin(outputDef.pin, PB_OFF);
                } else if (outputDef.boardType == PB_LED && outputDef.boardIndex < NUM_LED_CHIPS) {
                    // Stage the LED OFF to the appropriate LED chip
                    g_PBEngine.m_LEDChip[outputDef.boardIndex].StageLEDControl(false, outputDef.pin, LEDOff);
                }
                outputDef.lastState = PB_OFF;
                if (elapsedTime >= (pulse.onTimeMS + pulse.offTimeMS)) {
                    // Pulse complete
                    pulseComplete = true;
                }
            }
            
            if (pulseComplete) {
                pulseIt = g_PBEngine.m_outputPulseMap.erase(pulseIt);
            } else {
                ++pulseIt;
            }
        } else {
            // Output definition not found, remove from map
            pulseIt = g_PBEngine.m_outputPulseMap.erase(pulseIt);
        }
    }
}

// Process active LED sequence progression and loop handling
void ProcessActiveLEDSequence() {
    // Check if sequence time has completed (implement sequence timing logic)
    
    if (g_PBEngine.m_LEDSequenceInfo.pLEDSequence != nullptr && 
        g_PBEngine.m_LEDSequenceInfo.pLEDSequence->stepCount > 0) {
        
        unsigned long currentTick = g_PBEngine.GetTickCountGfx();
        int nextSeqIndex = g_PBEngine.m_LEDSequenceInfo.currentSeqIndex;
        bool shouldAdvanceSequence = false;
        bool isFirstTime = g_PBEngine.m_LEDSequenceInfo.firstTime;
        
        // On first time through, reset the step start tick to current time
        // This ensures we don't skip the first step due to time elapsed during initialization
        if (isFirstTime) {
            g_PBEngine.m_LEDSequenceInfo.stepStartTick = currentTick;
            g_PBEngine.m_LEDSequenceInfo.firstTime = false;
        }
        
        // Initialize step timing when index changes (but not on firstTime, as it's already set above)
        if (g_PBEngine.m_LEDSequenceInfo.previousSeqIndex != g_PBEngine.m_LEDSequenceInfo.currentSeqIndex &&
            !isFirstTime) {
            g_PBEngine.m_LEDSequenceInfo.stepStartTick = currentTick;
        }
        
        // Calculate timing for current step
        if (g_PBEngine.m_LEDSequenceInfo.currentSeqIndex >= 0 && 
            g_PBEngine.m_LEDSequenceInfo.currentSeqIndex < g_PBEngine.m_LEDSequenceInfo.pLEDSequence->stepCount) {

            const auto& currentStep = g_PBEngine.m_LEDSequenceInfo.pLEDSequence->steps[g_PBEngine.m_LEDSequenceInfo.currentSeqIndex];
            unsigned long elapsedStepTime = currentTick - g_PBEngine.m_LEDSequenceInfo.stepStartTick;
            unsigned long totalStepDuration = currentStep.onDurationMS + currentStep.offDurationMS;
            
            // Check if it's time to advance to the next step
            if (elapsedStepTime >= totalStepDuration) {
                shouldAdvanceSequence = true;
                nextSeqIndex = g_PBEngine.m_LEDSequenceInfo.currentSeqIndex + g_PBEngine.m_LEDSequenceInfo.indexStep;
            }
        }
        
        // Stage LED values if: the sequence index has changed, OR this is the first time, OR previousSeqIndex == -1
        if (g_PBEngine.m_LEDSequenceInfo.previousSeqIndex != g_PBEngine.m_LEDSequenceInfo.currentSeqIndex ||
            g_PBEngine.m_LEDSequenceInfo.previousSeqIndex == -1 ||
            isFirstTime) {
            
            // Stage current sequence values to LED chips
            if (g_PBEngine.m_LEDSequenceInfo.currentSeqIndex >= 0 && 
                g_PBEngine.m_LEDSequenceInfo.currentSeqIndex < g_PBEngine.m_LEDSequenceInfo.pLEDSequence->stepCount) {
                const auto& currentStep = g_PBEngine.m_LEDSequenceInfo.pLEDSequence->steps[g_PBEngine.m_LEDSequenceInfo.currentSeqIndex];
                
                for (int chipIndex = 0; chipIndex < NUM_LED_CHIPS; chipIndex++) {
                    // Get the active LED mask for this chip
                    uint16_t activeMask = g_PBEngine.m_LEDSequenceInfo.activeLEDMask[chipIndex];
                    
                    // Get the LED bits from the sequence step for this chip
                    uint16_t ledBits = currentStep.LEDOnBits[chipIndex];
                        
                    // Stage individual LEDs based on the sequence bits, but only for active pins
                    for (int ledPin = 0; ledPin < 16; ledPin++) {
                        if (activeMask & (1 << ledPin)) {
                            LEDState state = (ledBits & (1 << ledPin)) ? LEDOn : LEDOff;
                            g_PBEngine.m_LEDChip[chipIndex].StageLEDControl(false, ledPin, state);
                            
                            // Update lastState for this LED in the output definition
                            for (size_t i = 0; i < NUM_OUTPUTS; i++) {
                                if (g_outputDef[i].boardType == PB_LED && 
                                    g_outputDef[i].boardIndex == chipIndex && 
                                    g_outputDef[i].pin == ledPin) {
                                    g_outputDef[i].lastState = (state == LEDOn) ? PB_ON : PB_OFF;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            
            // Update previous sequence index to avoid redundant programming
            g_PBEngine.m_LEDSequenceInfo.previousSeqIndex = g_PBEngine.m_LEDSequenceInfo.currentSeqIndex;
        }
        
        // Advance sequence index if timing indicates we should
        if (shouldAdvanceSequence) {
            g_PBEngine.m_LEDSequenceInfo.currentSeqIndex = nextSeqIndex;
            g_PBEngine.m_LEDSequenceInfo.stepStartTick = currentTick; // Reset step timer
            
            // Handle sequence boundaries and loop modes
            HandleLEDSequenceBoundaries();
        }
    }
}

// Handle LED sequence boundary conditions and loop modes
void HandleLEDSequenceBoundaries() {
    unsigned long currentTick = g_PBEngine.GetTickCountGfx();
    
    if (g_PBEngine.m_LEDSequenceInfo.currentSeqIndex >= g_PBEngine.m_LEDSequenceInfo.pLEDSequence->stepCount) {
        switch (g_PBEngine.m_LEDSequenceInfo.loopMode) {
            case PB_NOLOOP:
                // End sequence
                EndLEDSequence();
                break;
            case PB_LOOP:
                g_PBEngine.m_LEDSequenceInfo.currentSeqIndex = 0;
                g_PBEngine.m_LEDSequenceInfo.sequenceStartTick = currentTick;
                g_PBEngine.m_LEDSequenceInfo.stepStartTick = currentTick; // Reset step timer
                break;
            case PB_PINGPONG:
            case PB_PINGPONGLOOP:
                g_PBEngine.m_LEDSequenceInfo.indexStep = -1;
                g_PBEngine.m_LEDSequenceInfo.currentSeqIndex = g_PBEngine.m_LEDSequenceInfo.pLEDSequence->stepCount - 2;
                g_PBEngine.m_LEDSequenceInfo.stepStartTick = currentTick; // Reset step timer
                break;
        }
    } else if (g_PBEngine.m_LEDSequenceInfo.currentSeqIndex < 0) {
        switch (g_PBEngine.m_LEDSequenceInfo.loopMode) {
            case PB_NOLOOP:
            case PB_PINGPONG:
                // End sequence
                EndLEDSequence();
                break;
            case PB_LOOP:
                g_PBEngine.m_LEDSequenceInfo.currentSeqIndex = 0;
                g_PBEngine.m_LEDSequenceInfo.indexStep = 1;
                g_PBEngine.m_LEDSequenceInfo.sequenceStartTick = currentTick;
                g_PBEngine.m_LEDSequenceInfo.stepStartTick = currentTick; // Reset step timer
                break;
            case PB_PINGPONGLOOP:
                g_PBEngine.m_LEDSequenceInfo.currentSeqIndex = 1;
                g_PBEngine.m_LEDSequenceInfo.indexStep = 1;
                g_PBEngine.m_LEDSequenceInfo.sequenceStartTick = currentTick;
                g_PBEngine.m_LEDSequenceInfo.stepStartTick = currentTick; // Reset step timer
                break;
        }
    }
}

// End LED sequence and restore saved values
void EndLEDSequence() {
    g_PBEngine.m_LEDSequenceInfo.sequenceEnabled = false;
    
    // Don't send staged values here - we want to restore first, then let deferred 
    // messages overwrite, then send everything together
    // SendAllStagedLED();  // REMOVED - was causing issues with deferred message processing
    
    // Before restoration, sync m_ledControl with current hardware state to ensure we have
    // a clean baseline for restoration and deferred message processing
    for (int chipIndex = 0; chipIndex < NUM_LED_CHIPS; chipIndex++) {
        for (int regIndex = 0; regIndex < 4; regIndex++) {
            g_PBEngine.m_LEDChip[chipIndex].SyncStagedWithHardware(regIndex);
        }
    }
    
    // Restore saved LED values only for pins that were in the active mask
    for (int chipIndex = 0; chipIndex < NUM_LED_CHIPS; chipIndex++) {
        uint16_t activeMask = g_PBEngine.m_LEDSequenceInfo.activeLEDMask[chipIndex];
        for (int ledPin = 0; ledPin < 16; ledPin++) {
            if (activeMask & (1 << ledPin)) {
                // Calculate which register this pin corresponds to
                int regIndex = ledPin / 4;  // Each register controls 4 pins (2 bits per pin)
                
                // Get the saved register value and convert to LEDState using helper function
                uint8_t savedRegValue = g_PBEngine.m_LEDSequenceInfo.previousLEDValues[chipIndex][regIndex];
                LEDState state = g_PBEngine.m_LEDChip[chipIndex].GetLEDStateFromVal(savedRegValue, ledPin);
                g_PBEngine.m_LEDChip[chipIndex].StageLEDControl(false, ledPin, state);
                
                // Update lastState for this LED in the output definition
                for (size_t i = 0; i < NUM_OUTPUTS; i++) {
                    if (g_outputDef[i].boardType == PB_LED && 
                        g_outputDef[i].boardIndex == chipIndex && 
                        g_outputDef[i].pin == ledPin) {
                        g_outputDef[i].lastState = (state == LEDOn) ? PB_ON : PB_OFF;
                        break;
                    }
                }
            }
        }
    }
    
    // Send restored values to hardware so that m_currentControl matches m_ledControl
    // This ensures deferred messages will properly detect changes and set staged flags
    SendAllStagedLED();

    // Process deferred LED queue immediately after restoring state
    // This ensures deferred messages overwrite restored values before sending to hardware
    ProcessDeferredLEDQueue();
}

// Process deferred LED messages when sequence is not active
void ProcessDeferredLEDQueue() {
    while (!g_PBEngine.m_deferredQueue.empty()) {
        stOutputMessage deferredMessage = g_PBEngine.m_deferredQueue.front();
        g_PBEngine.m_deferredQueue.pop();
        
        // Fix the options pointer to point to our copied data
        if (deferredMessage.hasOptions) {
            deferredMessage.options = &deferredMessage.optionsCopy;
        }
        
        // Find the output definition
        int outputDefIndex = FindOutputDefIndex(deferredMessage.outputId);
        
        if (outputDefIndex != -1) {
            stOutputDef& outputDef = g_outputDef[outputDefIndex];
            
            if (outputDef.boardType == PB_LED) {
                // Use existing processing functions to avoid code duplication
                if (deferredMessage.outputMsg == PB_OMSG_LEDCFG_GROUPDIM || 
                    deferredMessage.outputMsg == PB_OMSG_LEDCFG_GROUPBLINK) {
                    // Use ProcessLEDConfigMessage for config messages
                    ProcessLEDConfigMessage(deferredMessage, outputDef);
                }
                else if (deferredMessage.outputMsg == PB_OMSG_LED || 
                         deferredMessage.outputMsg == PB_OMSG_LEDSET_BRIGHTNESS) {
                    // Use ProcessLEDOutputMessage with skipSequenceCheck=true for deferred messages
                    ProcessLEDOutputMessage(deferredMessage, outputDef, true);
                }
            }
        }
    }
}

//==============================================================================
// NeoPixel Output Processing Functions
//==============================================================================

// Helper function to send all staged NeoPixel outputs to hardware
void SendAllStagedNeoPixels() {
    for (auto& pair : g_PBEngine.m_NeoPixelDriverMap) {
        pair.second.SendStagedNeoPixels();
    }
}

// Process NeoPixel output messages
void ProcessNeoPixelOutputMessage(const stOutputMessage& message, stOutputDef& outputDef) {
    int boardIndex = outputDef.boardIndex;
    
    // Check if driver exists
    if (g_PBEngine.m_NeoPixelDriverMap.find(boardIndex) == g_PBEngine.m_NeoPixelDriverMap.end()) {
        return;  // Driver not initialized
    }
    
    // Check if NeoPixel sequence is active for this driver
    if (g_PBEngine.m_NeoPixelSequenceMap.find(boardIndex) != g_PBEngine.m_NeoPixelSequenceMap.end()) {
        if (g_PBEngine.m_NeoPixelSequenceMap[boardIndex].sequenceEnabled) {
            // Sequence is active - ignore direct NeoPixel messages during sequence
            return;
        }
    }
    
    // Get RGB values from message options
    if (message.hasOptions) {
        uint8_t red = message.optionsCopy.neoPixelRed;
        uint8_t green = message.optionsCopy.neoPixelGreen;
        uint8_t blue = message.optionsCopy.neoPixelBlue;
        uint8_t brightness = message.optionsCopy.brightness;
        unsigned int neoPixelIndex = message.optionsCopy.neoPixelIndex;  // Get index from options
        
        // Check if this is a single pixel operation (neoPixelIndex != ALLNEOPIXELS)
        // or all pixels operation (neoPixelIndex == ALLNEOPIXELS)
        if (neoPixelIndex != ALLNEOPIXELS) {
            // Single pixel operation, index is already 0-based, use directly
            g_PBEngine.m_NeoPixelDriverMap.at(boardIndex).StageNeoPixel(neoPixelIndex, red, green, blue, brightness);
        } else {
            // All pixels operation - stage all LEDs
            // Note: brightness is stored in the node and applied during transmission
            g_PBEngine.m_NeoPixelDriverMap.at(boardIndex).StageNeoPixelAll(red, green, blue, brightness);
        }
    }
    
    // Update last state
    outputDef.lastState = message.outputState;
}

// Process NeoPixel sequence start/stop messages
void ProcessNeoPixelSequenceMessage(const stOutputMessage& message) {
    // Find the driver index from the outputId
    // For NeoPixel sequences, get the boardIndex from the output def using outputId as index (no bounds check needed)
    int driverIndex = 0;  // Default to first driver
    
    if (g_outputDef[message.outputId].boardType == PB_NEOPIXEL) {
        driverIndex = g_outputDef[message.outputId].boardIndex;
    }
    
    // Check if driver exists
    if (g_PBEngine.m_NeoPixelDriverMap.find(driverIndex) == g_PBEngine.m_NeoPixelDriverMap.end()) {
        return;  // Driver not initialized
    }
    
    if (message.outputState == PB_ON && message.options != nullptr && 
        message.options->setNeoPixelSequence != nullptr) {
        // Start NeoPixel sequence mode
        unsigned long currentTick = g_PBEngine.GetTickCountGfx();
        
        // Initialize sequence info if not exists
        if (g_PBEngine.m_NeoPixelSequenceMap.find(driverIndex) == g_PBEngine.m_NeoPixelSequenceMap.end()) {
            g_PBEngine.m_NeoPixelSequenceMap[driverIndex] = stNeoPixelSequenceInfo();
        }
        
        g_PBEngine.m_NeoPixelSequenceMap[driverIndex].sequenceEnabled = true;
        g_PBEngine.m_NeoPixelSequenceMap[driverIndex].firstTime = true;
        g_PBEngine.m_NeoPixelSequenceMap[driverIndex].sequenceStartTick = currentTick;
        g_PBEngine.m_NeoPixelSequenceMap[driverIndex].stepStartTick = currentTick;
        g_PBEngine.m_NeoPixelSequenceMap[driverIndex].currentSeqIndex = 0;
        g_PBEngine.m_NeoPixelSequenceMap[driverIndex].previousSeqIndex = -1;
        g_PBEngine.m_NeoPixelSequenceMap[driverIndex].indexStep = 1;
        g_PBEngine.m_NeoPixelSequenceMap[driverIndex].loopMode = message.options->loopMode;
        g_PBEngine.m_NeoPixelSequenceMap[driverIndex].pNeoPixelSequence = 
            const_cast<NeoPixelSequence*>(message.options->setNeoPixelSequence);
        g_PBEngine.m_NeoPixelSequenceMap[driverIndex].driverIndex = driverIndex;
    } else {
        // Stop NeoPixel sequence mode
        EndNeoPixelSequence(driverIndex);
    }
}

// Process active NeoPixel sequence progression and loop handling
void ProcessActiveNeoPixelSequence(int driverIndex) {
    // Check if driver and sequence exist
    if (g_PBEngine.m_NeoPixelDriverMap.find(driverIndex) == g_PBEngine.m_NeoPixelDriverMap.end() ||
        g_PBEngine.m_NeoPixelSequenceMap.find(driverIndex) == g_PBEngine.m_NeoPixelSequenceMap.end()) {
        return;
    }
    
    stNeoPixelSequenceInfo& seqInfo = g_PBEngine.m_NeoPixelSequenceMap[driverIndex];
    
    if (!seqInfo.sequenceEnabled || seqInfo.pNeoPixelSequence == nullptr || 
        seqInfo.pNeoPixelSequence->stepCount <= 0) {
        return;
    }
    
    unsigned long currentTick = g_PBEngine.GetTickCountGfx();
    int nextSeqIndex = seqInfo.currentSeqIndex;
    bool shouldAdvanceSequence = false;
    bool isFirstTime = seqInfo.firstTime;
    
    // On first time through, reset the step start tick
    if (isFirstTime) {
        seqInfo.stepStartTick = currentTick;
        seqInfo.firstTime = false;
    }
    
    // Initialize step timing when index changes (but not on firstTime)
    if (seqInfo.previousSeqIndex != seqInfo.currentSeqIndex && !isFirstTime) {
        seqInfo.stepStartTick = currentTick;
    }
    
    // Calculate timing for current step
    if (seqInfo.currentSeqIndex >= 0 && seqInfo.currentSeqIndex < seqInfo.pNeoPixelSequence->stepCount) {
        const auto& currentStep = seqInfo.pNeoPixelSequence->steps[seqInfo.currentSeqIndex];
        unsigned long elapsedStepTime = currentTick - seqInfo.stepStartTick;
        
        // Check if it's time to advance to the next step
        if (elapsedStepTime >= currentStep.onDurationMS) {
            shouldAdvanceSequence = true;
            nextSeqIndex = seqInfo.currentSeqIndex + seqInfo.indexStep;
        }
    }
    
    // Stage NeoPixel values if the sequence index has changed or this is the first time
    if (seqInfo.previousSeqIndex != seqInfo.currentSeqIndex || 
        seqInfo.previousSeqIndex == -1 || isFirstTime) {
        
        if (seqInfo.currentSeqIndex >= 0 && seqInfo.currentSeqIndex < seqInfo.pNeoPixelSequence->stepCount) {
            const auto& currentStep = seqInfo.pNeoPixelSequence->steps[seqInfo.currentSeqIndex];
            
            // Stage the entire array for this step
            if (currentStep.nodeArray != nullptr) {
                // If step has a brightness value (not 255), apply it to override node brightness
                // Otherwise use the brightness stored in each node
                if (currentStep.brightness < 255) {
                    // Apply step brightness as a scaling factor to all LEDs
                    unsigned int numLEDs = g_PBEngine.m_NeoPixelDriverMap.at(driverIndex).GetNumLEDs();
                    for (unsigned int i = 0; i < numLEDs; i++) {
                        uint8_t scaledBrightness = (currentStep.nodeArray[i].stagedBrightness * currentStep.brightness) / 255;
                        g_PBEngine.m_NeoPixelDriverMap.at(driverIndex).StageNeoPixel(
                            i,
                            currentStep.nodeArray[i].stagedRed,
                            currentStep.nodeArray[i].stagedGreen,
                            currentStep.nodeArray[i].stagedBlue,
                            scaledBrightness
                        );
                    }
                } else {
                    // Use node brightness as-is
                    g_PBEngine.m_NeoPixelDriverMap.at(driverIndex).StageNeoPixelArray(
                        currentStep.nodeArray,
                        g_PBEngine.m_NeoPixelDriverMap.at(driverIndex).GetNumLEDs()
                    );
                }
            }
        }
        
        seqInfo.previousSeqIndex = seqInfo.currentSeqIndex;
    }
    
    // Advance sequence index if timing indicates we should
    if (shouldAdvanceSequence) {
        seqInfo.currentSeqIndex = nextSeqIndex;
        seqInfo.stepStartTick = currentTick;
        
        // Handle sequence boundaries and loop modes
        HandleNeoPixelSequenceBoundaries(driverIndex);
    }
}

// Handle NeoPixel sequence boundary conditions and loop modes
void HandleNeoPixelSequenceBoundaries(int driverIndex) {
    if (g_PBEngine.m_NeoPixelSequenceMap.find(driverIndex) == g_PBEngine.m_NeoPixelSequenceMap.end()) {
        return;
    }
    
    stNeoPixelSequenceInfo& seqInfo = g_PBEngine.m_NeoPixelSequenceMap[driverIndex];
    unsigned long currentTick = g_PBEngine.GetTickCountGfx();
    
    if (seqInfo.currentSeqIndex >= seqInfo.pNeoPixelSequence->stepCount) {
        switch (seqInfo.loopMode) {
            case PB_NOLOOP:
                // End sequence
                EndNeoPixelSequence(driverIndex);
                break;
            case PB_LOOP:
                seqInfo.currentSeqIndex = 0;
                seqInfo.sequenceStartTick = currentTick;
                seqInfo.stepStartTick = currentTick;
                break;
            case PB_PINGPONG:
            case PB_PINGPONGLOOP:
                seqInfo.indexStep = -1;
                seqInfo.currentSeqIndex = seqInfo.pNeoPixelSequence->stepCount - 2;
                if (seqInfo.currentSeqIndex < 0) seqInfo.currentSeqIndex = 0;
                seqInfo.stepStartTick = currentTick;
                break;
        }
    } else if (seqInfo.currentSeqIndex < 0) {
        switch (seqInfo.loopMode) {
            case PB_PINGPONG:
                // End sequence
                EndNeoPixelSequence(driverIndex);
                break;
            case PB_PINGPONGLOOP:
                seqInfo.indexStep = 1;
                seqInfo.currentSeqIndex = 1;
                seqInfo.sequenceStartTick = currentTick;
                seqInfo.stepStartTick = currentTick;
                break;
            default:
                // Should not reach here
                EndNeoPixelSequence(driverIndex);
                break;
        }
    }
}

// End NeoPixel sequence for a specific driver
void EndNeoPixelSequence(int driverIndex) {
    if (g_PBEngine.m_NeoPixelSequenceMap.find(driverIndex) != g_PBEngine.m_NeoPixelSequenceMap.end()) {
        g_PBEngine.m_NeoPixelSequenceMap[driverIndex].sequenceEnabled = false;
    }
}

// Overall IO processing - putting this in one function allows for easier timing control and to process all at once
bool PBProcessIO() {

    PBProcessInput();
    PBProcessOutput();
    return (true);
}

#endif // RapberryPi Specific code

// End the platform specific code and functions

// Main program start!!   
int main(int argc, char const *argv[])
{
    // Show version information first
    ShowVersion();

    // Check and adjust working directory if needed
    if (!AdjustWorkingDirectory(argv[0])) {
        return 1;
    }
    
    std::string temp;
    static unsigned int startFrameTime = 0;
    static bool didLimitRender = false;
    
    g_PBEngine.pbeSendConsole("OpenGL ES: Initialize");
    if (!PBInitRender (PB_SCREENWIDTH, PB_SCREENHEIGHT)) return (false);

    g_PBEngine.pbeSendConsole("OpenGL ES: Successful");

    temp = "Screen Width: " + std::to_string(g_PBEngine.oglGetScreenWidth()) + " Screen Height: " + std::to_string(g_PBEngine.oglGetScreenHeight());
    g_PBEngine.pbeSendConsole(temp);

    g_PBEngine.pbeSendConsole("RasPin: Loading system font");

    // Get the system font sprite Id and default height for console
    g_PBEngine.m_defaultFontSpriteId = g_PBEngine.gfxGetSystemFontSpriteId();
    g_PBEngine.m_consoleTextHeight = g_PBEngine.gfxGetTextHeight(g_PBEngine.m_defaultFontSpriteId);

    g_PBEngine.pbeSendConsole("RasPin: System font ready");

    // Setup the inputs and outputs
    g_PBEngine.pbeSendConsole("RasPin: Setting up I/O");
    
    // Initialize input and output definition arrays
    InitializeInputDefs();
    InitializeOutputDefs();
    
    // Initialize NeoPixel array pointers
    initNeoPixelArrays();
    g_PBEngine.pbeSetupIO();
    
    // Load the saved values for settings and high scores
    if (!g_PBEngine.pbeLoadSaveFile(false, false)) {
        std::string temp2 = SAVEFILENAME;
        std::string temp = "RasPin: ERROR Using settings defaults, failed: " + temp2;
        g_PBEngine.pbeSendConsole(temp);
        g_PBEngine.pbeSaveFile ();
    }
    else g_PBEngine.pbeSendConsole("RasPin: Loaded settings and score file"); 

    // Set amplifier volume from saved settings (convert 0-10 range to 0-100%)
    g_PBEngine.m_ampDriver.SetVolume(g_PBEngine.m_saveFileData.mainVolume * 10);
    g_PBEngine.pbeSendConsole("RasPin: Set amplifier volume to " + std::to_string(g_PBEngine.m_saveFileData.mainVolume * 10) + "%");

    // Set the mixer volumes and start the background music
    g_PBEngine.pbeSendConsole("RasPin: Starting main menu music");    
    g_PBEngine.m_soundSystem.pbsSetMasterVolume(100);
    g_PBEngine.m_soundSystem.pbsSetMusicVolume(g_PBEngine.m_saveFileData.musicVolume * 10);

    g_PBEngine.pbeSendConsole("RasPin: Starting main processing loop");    
   
    // Main loop for the pinball game                                
    unsigned long currentTick = g_PBEngine.GetTickCountGfx();
    unsigned long lastTick = currentTick;

    // Start the input thread
    // std::thread inputThread(&PBProcessInput);

    // The main game engine loop
    startFrameTime = g_PBEngine.GetTickCountGfx();

    while (true) {

        currentTick = g_PBEngine.GetTickCountGfx();
        stInputMessage inputMessage;
        static bool firstLoop = true;
        
        // PB Process Input and Output will be a thread in the PiOS side since it won't have windows messages
        // Will need to fix this later...
        // Don't want to do it on the first render loop since all the state may not be set up yet
        if (!firstLoop){

            // Execute all registered devices
            g_PBEngine.pbeExecuteDevices();

            // Process timers and generate timer expiration input messages
            g_PBEngine.pbeProcessTimers();

            PBProcessIO();
            // Process all the input message queue and update the game state
            if (!g_PBEngine.m_inputQueue.empty()){
                inputMessage = g_PBEngine.m_inputQueue.front();
                g_PBEngine.m_inputQueue.pop();

                // Update the game state based on the input message
                if (!g_PBEngine.m_GameStarted) g_PBEngine.pbeUpdateState (inputMessage); 
                else g_PBEngine.pbeUpdateGameState (inputMessage);
            }
        }
        else firstLoop = false;

        // Limit the render speed to per the #defines ( PB_MS_PER_FRAME)
        if (((currentTick - startFrameTime) >= PB_MS_PER_FRAME) || (PB_MS_PER_FRAME == 0)) {
            didLimitRender = false;
            startFrameTime = currentTick;
        }

        if (!didLimitRender){
            
            // FPS tracking variables - only count when actually rendering
            static unsigned long frameCount = 0;
            static unsigned long fpsLastTime = currentTick;
            static unsigned long fpsUpdateInterval = 1000; // Update FPS every 1 second
            
            // Calculate FPS based on actual rendered frames
            frameCount++;
            if (currentTick - fpsLastTime >= fpsUpdateInterval) {
                g_PBEngine.m_RenderFPS = (int)((float)frameCount / ((float)(currentTick - fpsLastTime) / 1000.0f));
                frameCount = 0;
                fpsLastTime = currentTick;
            }

            if (!g_PBEngine.m_GameStarted)g_PBEngine.pbeRenderScreen(currentTick, lastTick);
            else g_PBEngine.pbeRenderGameScreen(currentTick, lastTick);

            // Show the IO overlay if enabled
            if (g_PBEngine.m_EnableOverlay) g_PBEngine.pbeRenderOverlay(currentTick, lastTick);

            // Render the current FPS if enabled, at the bottom left corner
            if (g_PBEngine.m_ShowFPS) {
                std::string temp = "FPS: " + std::to_string(g_PBEngine.m_RenderFPS);
                g_PBEngine.gfxSetColor(g_PBEngine.m_defaultFontSpriteId, 255, 255, 255, 255);
                g_PBEngine.gfxRenderShadowString(g_PBEngine.m_defaultFontSpriteId, temp, 10, PB_SCREENHEIGHT - 30, 1, GFX_TEXTLEFT, 0, 0, 0, 255, 1);
            }

            // Flush the swap when running the benchmark
            if (g_PBEngine.pbeGetMainState() == PB_BENCHMARK) g_PBEngine.gfxSwap(true);
            else g_PBEngine.gfxSwap(false);

            lastTick = currentTick;
            didLimitRender = true;
        }
    }

   // Join the input thread before exiting
   // inputThread.join();

   return 0;
}


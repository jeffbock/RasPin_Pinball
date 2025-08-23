// PBDebounce.cpp includes functions that are used to debounce inputs on the Raspberry Pi.
// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "PBDebounce.h"
#include "PBBuildSwitch.h"

cDebounceInput::cDebounceInput(int pin, int debounceTimeMS, bool usePullUpDown, bool pullUpOn){
      
#ifdef EXE_MODE_RASPI
  pinMode(pin, INPUT);
#endif
  m_lastPinState = pinHigh;
  m_lastValidPinState = pinHigh;
  m_pin = pin;
  m_debounceTimeMS = debounceTimeMS;
  m_timeInStateMS = 0;
  m_firstRead = true;

#ifdef EXE_MODE_RASPI
  if (usePullUpDown)
  {
    if (pullUpOn) {
      pullUpDnControl(pin, PUD_UP);
    }
    else {
      pullUpDnControl(pin, PUD_DOWN);
      m_lastPinState = pinLow;
    }
  }
  else pullUpDnControl(pin, PUD_OFF);
#endif
}

int cDebounceInput::readPin (){
  
  // Get the current time
  std::chrono::steady_clock::time_point currentClock = std::chrono::steady_clock::now();

  int tempPinState;
  pinState currentPinState;
  bool elapsedIsZero=false;

  if (m_firstRead){
    m_lastClock = currentClock;
    m_firstRead = false;
  }

#ifdef EXE_MODE_RASPI
  tempPinState = digitalRead(m_pin);
#else
  tempPinState = 1;  // Default to high for Windows builds
#endif

  if (tempPinState == 0) currentPinState = pinLow;
  else currentPinState = pinHigh;

  if (currentPinState == m_lastPinState){
    auto elapsedMS = std::chrono::duration_cast<std::chrono::milliseconds>(currentClock - m_lastClock);
    if (elapsedMS.count() == 0) elapsedIsZero = true;
    else m_timeInStateMS += elapsedMS.count();

    if (m_timeInStateMS > m_debounceTimeMS) {
      m_lastValidPinState = currentPinState;
    }
  }
  else {
    m_timeInStateMS = 0;
  }

  m_lastPinState = currentPinState;
  if (!elapsedIsZero) m_lastClock = currentClock;
  return ((int)m_lastValidPinState);
}

//==============================================================================
// IODriverDebounce Class Implementation
//==============================================================================

IODriverDebounce::IODriverDebounce(uint8_t address, uint16_t inputMask, int defaultDebounceTimeMS) 
    : IODriver(address, inputMask), m_debouncedValues(0) {
    
    // Initialize debounce data for all 16 pins
    for (int i = 0; i < 16; i++) {
        m_pinData[i].debounceTimeMS = defaultDebounceTimeMS;
        m_pinData[i].timeInStateMS = 0;
        m_pinData[i].lastPinState = pinHigh;
        m_pinData[i].lastValidPinState = pinHigh;
        m_pinData[i].firstRead = true;
    }
}

IODriverDebounce::~IODriverDebounce() {
    // Base class destructor will handle cleanup
}

void IODriverDebounce::SetPinDebounceTime(uint8_t pinIndex, int debounceTimeMS) {
    if (pinIndex < 16) {
        m_pinData[pinIndex].debounceTimeMS = debounceTimeMS;
    }
}

uint16_t IODriverDebounce::ReadInputsDB() {
    // Read raw inputs from the IODriver base class
    uint16_t rawInputs = ReadInputs();
    
    // Get current time
    std::chrono::steady_clock::time_point currentClock = std::chrono::steady_clock::now();
    
    // Process each pin through debounce logic
    for (int i = 0; i < 16; i++) {
        // Extract current pin state from raw inputs
        bool rawPinHigh = (rawInputs & (1 << i)) != 0;
        pinState currentPinState = rawPinHigh ? pinHigh : pinLow;
        
        PinDebounceData& pinData = m_pinData[i];
        bool elapsedIsZero = false;
        
        if (pinData.firstRead) {
            pinData.lastClock = currentClock;
            pinData.firstRead = false;
        }
        
        if (currentPinState == pinData.lastPinState) {
            // Pin state is stable, accumulate time
            auto elapsedMS = std::chrono::duration_cast<std::chrono::milliseconds>(currentClock - pinData.lastClock);
            if (elapsedMS.count() == 0) {
                elapsedIsZero = true;
            } else {
                pinData.timeInStateMS += elapsedMS.count();
            }
            
            // Check if debounce time has been met
            if (pinData.timeInStateMS > pinData.debounceTimeMS) {
                pinData.lastValidPinState = currentPinState;
                
                // Update the debounced values
                if (currentPinState == pinHigh) {
                    m_debouncedValues |= (1 << i);   // Set bit
                } else {
                    m_debouncedValues &= ~(1 << i);  // Clear bit
                }
            }
        } else {
            // Pin state changed, reset time accumulator
            pinData.timeInStateMS = 0;
        }
        
        pinData.lastPinState = currentPinState;
        if (!elapsedIsZero) {
            pinData.lastClock = currentClock;
        }
    }
    
    return m_debouncedValues;
}

int IODriverDebounce::ReadPinDB(uint8_t pinIndex) {
    if (pinIndex >= 16) {
        return 0;  // Invalid pin index
    }
    
    // Update all debounced values
    ReadInputsDB();
    
    // Return 1 if pin is high, 0 if pin is low
    return (m_debouncedValues & (1 << pinIndex)) != 0 ? 1 : 0;
}
  
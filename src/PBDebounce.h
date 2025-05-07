// Input debounce class, used by the Pinball Engine when running on the Raspberry Pi
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where otherwise noted

#ifndef PBDebounce_h
#define PBDebounce_h

#include "wiringPi.h"
#include "wiringPiI2C.h"
#include <chrono>

class cDebounceInput {

    public:
  
    enum pinState {
      pinLow = 0,
      pinHigh = 1
    };
  
    cDebounceInput(int pin, int debounceTimeMS, bool usePullUpDown, bool pullUpOn);
    int readPin ();
    
    private:
  
    int m_pin, m_debounceTimeMS;
    unsigned long m_timeInStateMS;
    std::chrono::steady_clock::time_point m_lastClock;
    pinState m_lastPinState, m_lastValidPinState; 
    bool m_firstRead;
  };

#endif
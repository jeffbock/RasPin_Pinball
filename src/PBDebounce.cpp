// PBDebounce.cpp includes functions that are used to debounce inputs on the Raspberry Pi.
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#include "PBDebounce.h"

class cDebounceInput {

    public:
  
    enum pinState {
      pinLow = 0,
      pinHigh = 1
    };
  
    cDebounceInput(int pin, int debounceTimeMS, bool usePullUpDown, bool pullUpOn){
      
      pinMode(pin, INPUT);
      m_lastPinState = pinHigh;
      m_lastValidPinState = pinHigh;
      m_pin = pin;
      m_debounceTimeMS = debounceTimeMS;
      m_timeInStateMS = 0;
      m_firstRead = true;
  
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
    }
  
    int readPin ()
    {
      // Get the current time
      std::chrono::steady_clock::time_point currentClock = std::chrono::steady_clock::now();
  
      int tempPinState;
      pinState currentPinState;
      bool elapsedIsZero=false;
  
      if (m_firstRead){
        m_lastClock = currentClock;
        m_firstRead = false;
      }
  
      tempPinState = digitalRead(m_pin);
  
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
  
    private:
  
    int m_pin, m_debounceTimeMS;
    unsigned long m_timeInStateMS;
    std::chrono::steady_clock::time_point m_lastClock;
    pinState m_lastPinState, m_lastValidPinState; 
    bool m_firstRead;
  };
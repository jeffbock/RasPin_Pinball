// PBDevice.cpp includes base class implementation and sample derived class (pbdEjector)
// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "PBDevice.h"

//==============================================================================
// PBDevice Base Class Implementation
//==============================================================================

PBDevice::PBDevice() {
    m_startTimeMS = 0;
    m_enabled = false;
    m_state = 0;
    m_running = false;
    m_error = 0;
}

PBDevice::~PBDevice() {
    // Cleanup if needed
}

// Reset the start time to current time, disable the device
void PBDevice::pbdInit() {
    m_startTimeMS = getCurrentTimeMS();
    m_enabled = false;
    m_running = false;
    m_error = 0;
    m_state = 0;
}

// Enable or disable the device
void PBDevice::pbdEnable(bool enable) {
    m_enabled = enable;
}

// Enable the device and set the m_running boolean
void PBDevice::pdbStartRun() {
    m_enabled = true;
    m_running = true;
    m_startTimeMS = getCurrentTimeMS();
}

// Return the m_running boolean
bool PBDevice::pdbIsRunning() const {
    return m_running;
}

// Check if current error value is non-zero
bool PBDevice::pbdIsError() const {
    return (m_error != 0);
}

// Return the error value and clear it to zero
int PBDevice::pbdResetError() {
    int tempError = m_error;
    m_error = 0;
    return tempError;
}

// Set the state value
void PBDevice::pbdSetState(unsigned int state) {
    m_state = state;
}

// Return the value of m_state
unsigned int PBDevice::pbdGetState() const {
    return m_state;
}

// Helper function to get current time in milliseconds
unsigned long PBDevice::getCurrentTimeMS() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return static_cast<unsigned long>(duration.count());
}

//==============================================================================
// pbdEjector Derived Class - Sample Implementation
//==============================================================================

class pbdEjector : public PBDevice {
public:
    pbdEjector(unsigned int inputPin, unsigned int ledOutputId, unsigned int solenoidOutputId);
    ~pbdEjector();

    // Override base class virtual functions
    void pbdInit() override;
    void pbdEnable(bool enable) override;
    void pdbStartRun() override;
    void pbdExecute() override;

private:
    // Derived class specific member variables
    unsigned int m_inputPin;          // Input pin to check (ball in ejector)
    unsigned int m_ledOutputId;       // LED output ID
    unsigned int m_solenoidOutputId;  // Solenoid output ID
    unsigned long m_solenoidStartMS;  // Time when solenoid was turned on
    unsigned long m_solenoidOffMS;    // Time when solenoid was turned off
    bool m_solenoidActive;            // Solenoid currently active
    bool m_ledActive;                 // LED currently active

    // State definitions for the ejector
    enum EjectorState {
        STATE_IDLE = 0,
        STATE_BALL_DETECTED = 1,
        STATE_SOLENOID_ON = 2,
        STATE_SOLENOID_OFF = 3,
        STATE_COMPLETE = 4
    };
};

pbdEjector::pbdEjector(unsigned int inputPin, unsigned int ledOutputId, unsigned int solenoidOutputId) {
    m_inputPin = inputPin;
    m_ledOutputId = ledOutputId;
    m_solenoidOutputId = solenoidOutputId;
    m_solenoidStartMS = 0;
    m_solenoidOffMS = 0;
    m_solenoidActive = false;
    m_ledActive = false;
    pbdInit();
}

pbdEjector::~pbdEjector() {
    // Cleanup if needed
}

// Override pbdInit - reset derived class variables, then call base class
void pbdEjector::pbdInit() {
    m_solenoidStartMS = 0;
    m_solenoidOffMS = 0;
    m_solenoidActive = false;
    m_ledActive = false;
    
    // Call base class implementation at the end
    PBDevice::pbdInit();
}

// Override pbdEnable - perform any pre-enable work, then call base class
void pbdEjector::pbdEnable(bool enable) {
    // Derived class pre-work before enabling
    if (!enable) {
        // If disabling, ensure outputs are turned off
        if (m_ledActive) {
            // TODO: Replace with actual output message
            // Example: sendOutputMessage(m_ledOutputId, PB_OFF);
            m_ledActive = false;
        }
        if (m_solenoidActive) {
            // TODO: Replace with actual output message
            // Example: sendOutputMessage(m_solenoidOutputId, PB_OFF);
            m_solenoidActive = false;
        }
    }
    
    // Call base class implementation at the end
    PBDevice::pbdEnable(enable);
}

// Override pdbStartRun - perform any pre-start work, then call base class
void pbdEjector::pdbStartRun() {
    // Reset derived class state for new run
    m_solenoidStartMS = 0;
    m_solenoidOffMS = 0;
    m_solenoidActive = false;
    m_ledActive = false;
    
    // Call base class implementation at the end
    PBDevice::pdbStartRun();
}

void pbdEjector::pbdExecute() {
    // Only execute if device is enabled
    if (!m_enabled) {
        return;
    }

    unsigned long currentTimeMS = getCurrentTimeMS();
    
    switch (m_state) {
        case STATE_IDLE:
            // Check if ball is in ejector (input pin is set)
            // TODO: Replace with actual input pin read
            // Example: if (readInputPin(m_inputPin)) {
            {
                // For demonstration purposes, we'll assume ball is detected
                bool ballDetected = false;  // Placeholder - would read actual pin
                
                if (ballDetected) {
                    // Ball detected, transition to next state
                    m_state = STATE_BALL_DETECTED;
                    m_solenoidStartMS = currentTimeMS;
                    
                    // Turn on LED
                    // TODO: Replace with actual output message
                    // Example: sendOutputMessage(m_ledOutputId, PB_ON);
                    m_ledActive = true;
                    
                    // Turn on solenoid
                    // TODO: Replace with actual output message
                    // Example: sendOutputMessage(m_solenoidOutputId, PB_ON);
                    m_solenoidActive = true;
                }
            }
            break;

        case STATE_BALL_DETECTED:
        case STATE_SOLENOID_ON:
            // Check if 250ms has expired since starting
            if ((currentTimeMS - m_solenoidStartMS) >= 250) {
                // Turn off solenoid
                // TODO: Replace with actual output message
                // Example: sendOutputMessage(m_solenoidOutputId, PB_OFF);
                m_solenoidActive = false;
                m_solenoidOffMS = currentTimeMS;
                m_state = STATE_SOLENOID_OFF;
            }
            break;

        case STATE_SOLENOID_OFF:
            // Check if another 250ms has expired since turning off solenoid
            if ((currentTimeMS - m_solenoidOffMS) >= 250) {
                // Check if ball is still in ejector
                // TODO: Replace with actual input pin read
                // Example: if (readInputPin(m_inputPin)) {
                {
                    bool ballStillDetected = false;  // Placeholder - would read actual pin
                    
                    if (ballStillDetected) {
                        // Ball still there, repeat the cycle
                        m_solenoidStartMS = currentTimeMS;
                        
                        // Turn on solenoid again
                        // TODO: Replace with actual output message
                        // Example: sendOutputMessage(m_solenoidOutputId, PB_ON);
                        m_solenoidActive = true;
                        m_state = STATE_SOLENOID_ON;
                    } else {
                        // Ball ejected successfully
                        m_state = STATE_COMPLETE;
                        
                        // Turn off LED
                        // TODO: Replace with actual output message
                        // Example: sendOutputMessage(m_ledOutputId, PB_OFF);
                        m_ledActive = false;
                        
                        // Mark run as complete
                        m_running = false;
                    }
                }
            }
            break;

        case STATE_COMPLETE:
            // Run is complete, reset to idle
            m_state = STATE_IDLE;
            m_running = false;
            break;

        default:
            // Unknown state, set error and reset
            m_error = 1;
            m_state = STATE_IDLE;
            m_running = false;
            break;
    }
}

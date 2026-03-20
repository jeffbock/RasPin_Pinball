// PBDevice.cpp includes base class implementation and sample derived class (pbdEjector)
// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "PBDevice.h"

//==============================================================================
// PBDevice Base Class Implementation
//==============================================================================

PBDevice::PBDevice(PBEngine* pEngine) {
    m_pEngine = pEngine;
    m_startTimeMS = 0;
    m_enabled = false;
    m_state = 0;
    m_running = false;
    m_error = 0;
    m_runMode = RUN_ONCE;
}

PBDevice::~PBDevice() {
    // Cleanup if needed
}

// Reset the start time to current time, disable the device
void PBDevice::pbdInit() {
    m_startTimeMS = m_pEngine->GetTickCountGfx();
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
void PBDevice::pdbStartRun(PBDeviceRunMode runMode) {
    m_runMode = runMode;
    m_enabled = true;
    m_running = true;
    m_startTimeMS = m_pEngine->GetTickCountGfx();
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

//==============================================================================
// pbdEjector Derived Class - Sample Implementation
//==============================================================================

pbdEjector::pbdEjector(PBEngine* pEngine, unsigned int inputId, unsigned int ledOutputId, unsigned int solenoidOutputId) 
    : PBDevice(pEngine) {
    m_inputId = inputId;
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
            m_pEngine->SendOutputMsg(PB_OMSG_LED, m_ledOutputId, PB_OFF, false);
            m_ledActive = false;
        }
        if (m_solenoidActive) {
            m_pEngine->SendOutputMsg(PB_OMSG_GENERIC_IO, m_solenoidOutputId, PB_OFF, false);
            m_solenoidActive = false;
        }
    }
    
    // Call base class implementation at the end
    PBDevice::pbdEnable(enable);
}

// Override pdbStartRun - perform any pre-start work, then call base class
void pbdEjector::pdbStartRun(PBDeviceRunMode runMode) {
    // Reset derived class state for new run
    m_solenoidStartMS = 0;
    m_solenoidOffMS = 0;
    m_solenoidActive = false;
    m_ledActive = false;
    
    // Call base class implementation at the end
    PBDevice::pdbStartRun(runMode);
}

void pbdEjector::pbdExecute() {
    if (!m_enabled) return;

    unsigned long currentTimeMS = m_pEngine->GetTickCountGfx();
    
    // m_inputId is the array index (pre-validated, no bounds check needed)
    int inputDefIndex = m_inputId;
    
    switch (m_state) {
        case STATE_IDLE:
            // Check if ball is in ejector using input state
            if (g_inputDef[inputDefIndex].lastState == PB_ON) {
                m_state = STATE_BALL_DETECTED;
                m_solenoidStartMS = currentTimeMS;
                 m_pEngine->SendOutputMsg(PB_OMSG_GENERIC_IO, m_ledOutputId, PB_OFF, true);
                m_ledActive = true;
                m_pEngine->SendOutputMsg(PB_OMSG_GENERIC_IO, m_solenoidOutputId, PB_ON, false);
                m_solenoidActive = true;
            }
            break;

        case STATE_BALL_DETECTED:
        case STATE_SOLENOID_ON:
            if ((currentTimeMS - m_solenoidStartMS) >= EJECTOR_ON_MS) {
                m_pEngine->SendOutputMsg(PB_OMSG_GENERIC_IO, m_solenoidOutputId, PB_OFF, false);
                m_pEngine->SendOutputMsg(PB_OMSG_GENERIC_IO, m_ledOutputId, PB_OFF, false);
                m_solenoidActive = false;
                m_solenoidOffMS = currentTimeMS;
                m_state = STATE_SOLENOID_OFF;
            }
            break;

        case STATE_SOLENOID_OFF:
            if ((currentTimeMS - m_solenoidOffMS) >= EJECTOR_OFF_MS) {
                if (g_inputDef[inputDefIndex].lastState == PB_ON) {
                    // Ball still there, repeat the cycle
                    m_solenoidStartMS = currentTimeMS;
                    m_pEngine->SendOutputMsg(PB_OMSG_GENERIC_IO, m_solenoidOutputId, PB_ON, false);
                    m_pEngine->SendOutputMsg(PB_OMSG_GENERIC_IO, m_ledOutputId, PB_ON, true);
                    m_solenoidActive = true;
                    m_state = STATE_SOLENOID_ON;
                } else {
                    // Ball ejected successfully
                    m_state = STATE_COMPLETE;
                    m_ledActive = false;
                    m_running = false;
                }
            }
            break;

        case STATE_COMPLETE:
            m_state = STATE_IDLE;
            m_running = false;
            break;

        default:
            m_error = 1;
            m_state = STATE_IDLE;
            m_running = false;
            break;
    }
}

//==============================================================================
// pbdHopperEjector Derived Class Implementation
//==============================================================================

pbdHopperEjector::pbdHopperEjector(PBEngine* pEngine, unsigned int ballReadyInputId,
                                    unsigned int solenoidOutputId, unsigned int ballDeliveredInputId)
    : PBDevice(pEngine) {
    m_ballReadyInputId     = ballReadyInputId;
    m_solenoidOutputId     = solenoidOutputId;
    m_ballDeliveredInputId = ballDeliveredInputId;
    m_solenoidStartMS      = 0;
    m_stateStartMS         = 0;
    m_solenoidActive       = false;
    pbdInit();
}

pbdHopperEjector::~pbdHopperEjector() {}

void pbdHopperEjector::pbdInit() {
    m_solenoidStartMS = 0;
    m_stateStartMS    = 0;
    m_solenoidActive  = false;
    PBDevice::pbdInit();
}

void pbdHopperEjector::pbdEnable(bool enable) {
    if (!enable && m_solenoidActive) {
        m_pEngine->SendOutputMsg(PB_OMSG_GENERIC_IO, m_solenoidOutputId, PB_OFF, false);
        m_solenoidActive = false;
    }
    PBDevice::pbdEnable(enable);
}

void pbdHopperEjector::pdbStartRun(PBDeviceRunMode runMode) {
    m_solenoidStartMS = 0;
    m_solenoidActive  = false;
    m_stateStartMS    = m_pEngine->GetTickCountGfx();
    m_state           = STATE_CHECK_BALL_READY;
    PBDevice::pdbStartRun(runMode);
}

void pbdHopperEjector::pbdExecute() {
    if (!m_enabled) return;

    unsigned long currentTimeMS = m_pEngine->GetTickCountGfx();

    // Overall timeout check (applies in any active state)
    if (m_state != STATE_IDLE && m_state != STATE_COMPLETE) {
        if ((currentTimeMS - m_startTimeMS) >= HOPPER_OVERALL_TIMEOUT_MS) {
            if (m_solenoidActive) {
                m_pEngine->SendOutputMsg(PB_OMSG_GENERIC_IO, m_solenoidOutputId, PB_OFF, false);
                m_solenoidActive = false;
            }
            // Reset and loop back to check for the next ball
            m_startTimeMS  = currentTimeMS;
            m_stateStartMS = currentTimeMS;
            if (m_runMode == RUN_CONTINUOUS) {
                m_state = STATE_CHECK_BALL_READY;
            } else {
                m_running = false;
                m_state   = STATE_IDLE;
            }
            return;
        }
    }

    switch (m_state) {
        case STATE_IDLE:
            // Waiting to be triggered externally via pdbStartRun()
            break;

        case STATE_CHECK_BALL_READY:
            // Step 1: wait for ball-ready sensor to go ON
            if (g_inputDef[m_ballReadyInputId].lastState == PB_ON) {
                // Ball present - fire solenoid
                m_pEngine->SendOutputMsg(PB_OMSG_GENERIC_IO, m_solenoidOutputId, PB_ON, false);
                m_solenoidActive  = true;
                m_solenoidStartMS = currentTimeMS;
                m_state           = STATE_EJECTING;
            }
            break;

        case STATE_EJECTING:
            // Step 2: keep solenoid on for kick duration
            if ((currentTimeMS - m_solenoidStartMS) >= HOPPER_SOLENOID_ON_MS) {
                m_pEngine->SendOutputMsg(PB_OMSG_GENERIC_IO, m_solenoidOutputId, PB_OFF, false);
                m_solenoidActive = false;
                m_stateStartMS   = currentTimeMS;
                m_state          = STATE_WAIT_DELIVERY;
            }
            break;

        case STATE_WAIT_DELIVERY:
            // Step 3: wait for ball-delivered sensor
            if (g_inputDef[m_ballDeliveredInputId].lastState == PB_ON) {
                // Ball delivered
                m_startTimeMS  = currentTimeMS;
                m_stateStartMS = currentTimeMS;
                if (m_runMode == RUN_CONTINUOUS) {
                    // Loop: immediately watch for the next ball
                    m_state = STATE_CHECK_BALL_READY;
                } else {
                    // Single cycle complete - go idle
                    m_running = false;
                    m_state   = STATE_IDLE;
                }
            } else if ((currentTimeMS - m_stateStartMS) >= HOPPER_DELIVERY_TIMEOUT_MS) {
                // Ball-delivered not seen in 1s - go back and wait for ball-ready again
                m_stateStartMS = currentTimeMS;
                m_state        = STATE_CHECK_BALL_READY;
            }
            break;

        case STATE_COMPLETE:
            m_startTimeMS  = m_pEngine->GetTickCountGfx();
            m_stateStartMS = m_startTimeMS;
            if (m_runMode == RUN_CONTINUOUS) {
                m_state = STATE_CHECK_BALL_READY;
            } else {
                m_running = false;
                m_state   = STATE_IDLE;
            }
            break;

        default:
            m_error   = 1;
            m_running = false;
            m_state   = STATE_IDLE;
            break;
    }
}

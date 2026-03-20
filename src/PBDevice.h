// PBDevice base class for managing complex pinball device states and flows
// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef PBDevice_h
#define PBDevice_h

#include "PBGfx.h"
#include "Pinball_IO.h"
#include "Pinball_Engine.h"

// Run mode for device operation
enum PBDeviceRunMode {
    RUN_ONCE       = 0,  // Run a single cycle then go idle
    RUN_CONTINUOUS = 1   // Loop continuously until stopped
};

// Base class for pinball device management
class PBDevice {
public:
    PBDevice(PBEngine* pEngine);
    virtual ~PBDevice();

    // Base class functions that can be overridden by derived classes
    // Derived classes should call base class implementation at the end
    virtual void pbdInit();
    virtual void pbdEnable(bool enable);
    virtual void pdbStartRun(PBDeviceRunMode runMode = RUN_ONCE);
    
    // Base class functions (non-virtual)
    bool pdbIsRunning() const;
    bool pbdIsError() const;
    int pbdResetError();
    void pbdSetState(unsigned int state);
    unsigned int pbdGetState() const;

    // Pure virtual function to be implemented by derived classes
    virtual void pbdExecute() = 0;

protected:
    // Member variables
    unsigned long m_startTimeMS;  // Time the flow was started in MS
    bool m_enabled;               // Device enabled or not
    unsigned int m_state;         // Key state of the device
    bool m_running;               // Current run is done or not
    int m_error;                  // Error state (0 = no error)
    PBEngine* m_pEngine;          // Pointer to PBEngine (provides GetTickCountGfx and SendOutputMsg)
    PBDeviceRunMode m_runMode;    // RUN_ONCE or RUN_CONTINUOUS
};

//==============================================================================
// pbdEjector Derived Class - Ball Ejector Device
//==============================================================================

#define EJECTOR_ON_MS 1500  // Time to keep solenoid on
#define EJECTOR_OFF_MS 1000  // Time to wait after solenoid off before completing

class pbdEjector : public PBDevice {
public:
    pbdEjector(PBEngine* pEngine, unsigned int inputId, unsigned int ledOutputId, unsigned int solenoidOutputId);
    ~pbdEjector();

    // Override base class virtual functions
    void pbdInit() override;
    void pbdEnable(bool enable) override;
    void pdbStartRun(PBDeviceRunMode runMode = RUN_ONCE) override;
    void pbdExecute() override;

private:
    // Derived class specific member variables
    unsigned int m_inputId;           // Input ID (not pin) to check (ball in ejector)
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

//==============================================================================
// pbdHopperEjector Derived Class - Ball Hopper Ejector Device
// Triggered externally via pdbStartRun(). Checks ball-ready sensor, fires
// the solenoid, then waits for the ball-delivered sensor before completing.
//==============================================================================

#define HOPPER_SOLENOID_ON_MS        50    // Sharp solenoid kick (ms)
#define HOPPER_DELIVERY_TIMEOUT_MS   1000  // Wait for ball-delivered before retrying (ms)
#define HOPPER_OVERALL_TIMEOUT_MS    3000  // Overall operation timeout (ms)

class pbdHopperEjector : public PBDevice {
public:
    pbdHopperEjector(PBEngine* pEngine, unsigned int ballReadyInputId,
                     unsigned int solenoidOutputId, unsigned int ballDeliveredInputId);
    ~pbdHopperEjector();

    void pbdInit() override;
    void pbdEnable(bool enable) override;
    void pdbStartRun(PBDeviceRunMode runMode = RUN_ONCE) override;
    void pbdExecute() override;

private:
    unsigned int m_ballReadyInputId;      // Input ID for ball-ready sensor
    unsigned int m_solenoidOutputId;      // Solenoid output ID
    unsigned int m_ballDeliveredInputId;  // Input ID for ball-delivered sensor
    unsigned long m_solenoidStartMS;      // When solenoid was fired
    unsigned long m_stateStartMS;         // When current state started (for timeouts)
    bool m_solenoidActive;

    enum HopperState {
        STATE_IDLE = 0,
        STATE_CHECK_BALL_READY = 1,
        STATE_EJECTING = 2,
        STATE_WAIT_DELIVERY = 3,
        STATE_COMPLETE = 4
    };
};

#endif // PBDevice_h

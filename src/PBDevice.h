// PBDevice base class for managing complex pinball device states and flows
// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef PBDevice_h
#define PBDevice_h

#include "PBGfx.h"
#include "Pinball_IO.h"
#include "Pinball_Engine.h"

// Base class for pinball device management
class PBDevice {
public:
    PBDevice(PBEngine* pEngine);
    virtual ~PBDevice();

    // Base class functions that can be overridden by derived classes
    // Derived classes should call base class implementation at the end
    virtual void pbdInit();
    virtual void pbdEnable(bool enable);
    virtual void pdbStartRun();
    
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
    void pdbStartRun() override;
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

#endif // PBDevice_h

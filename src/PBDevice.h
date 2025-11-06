// PBDevice base class for managing complex pinball device states and flows
// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef PBDevice_h
#define PBDevice_h

#include <chrono>

// Base class for pinball device management
class PBDevice {
public:
    PBDevice();
    virtual ~PBDevice();

    // Base class functions
    void pbdInit();
    void pbdEnable(bool enable);
    void pdbStartRun();
    bool pdbIsRunning() const;
    bool pbdIsError() const;
    int pbdResetError();
    void pbdSetState(unsigned int state);
    unsigned int pbdGetState() const;

    // Virtual function to be overridden by derived classes
    virtual void pbdExecute() = 0;

protected:
    // Member variables
    unsigned long m_startTimeMS;  // Time the flow was started in MS
    bool m_enabled;               // Device enabled or not
    unsigned int m_state;         // Key state of the device
    bool m_running;               // Current run is done or not
    int m_error;                  // Error state (0 = no error)

    // Helper function to get current time in milliseconds
    unsigned long getCurrentTimeMS() const;
};

#endif // PBDevice_h

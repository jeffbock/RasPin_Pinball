# PBDevice Class API Reference

## Overview

The `PBDevice` class is a base class for managing complex pinball device states and flows. It provides a structured framework for controlling devices that require multi-step operations, state management, and timing control. The class works seamlessly with the PBEngine to access timing functions and output control.

## Key Features

- **State Management**: Built-in state tracking for complex device flows
- **Timing Control**: Access to engine timing for precise temporal control
- **Error Handling**: Integrated error state management
- **Run Control**: Enable/disable and start/stop functionality
- **Extensible Design**: Easy to derive custom device classes

## When to Use PBDevice

Use PBDevice for pinball devices that:
- Require multi-step operations (e.g., ball ejectors, scoops, diverters)
- Need precise timing between states
- Combine multiple outputs (LEDs, solenoids, motors)
- Require state persistence across game ticks
- Benefit from error tracking and recovery

**Simple devices** (like basic slingshots or bumpers) can use direct output messages without PBDevice overhead.

---

## Base Class Architecture

### Class Declaration

```cpp
class PBDevice {
public:
    PBDevice(PBEngine* pEngine);
    virtual ~PBDevice();

    // Virtual functions for derived classes
    virtual void pbdInit();
    virtual void pbdEnable(bool enable);
    virtual void pdbStartRun();
    virtual void pbdExecute() = 0;  // Pure virtual - must implement
    
    // Non-virtual utility functions
    bool pdbIsRunning() const;
    bool pbdIsError() const;
    int pbdResetError();
    void pbdSetState(unsigned int state);
    unsigned int pbdGetState() const;

protected:
    unsigned long m_startTimeMS;  // Flow start time
    bool m_enabled;               // Device enabled/disabled
    unsigned int m_state;         // Current state
    bool m_running;               // Flow in progress
    int m_error;                  // Error code (0 = no error)
    PBEngine* m_pEngine;          // Engine reference
};
```

---

## Constructor and Destructor

### PBDevice()

Initializes the base class with a reference to the PBEngine.

**Signature:**
```cpp
PBDevice(PBEngine* pEngine);
```

**Parameters:**
- `pEngine` - Pointer to PBEngine instance for timing and output control

**Example:**
```cpp
class MyCustomDevice : public PBDevice {
public:
    MyCustomDevice(PBEngine* pEngine, unsigned int outputId) 
        : PBDevice(pEngine) {
        m_outputId = outputId;
        pbdInit();
    }
};
```

### ~PBDevice()

Virtual destructor for proper cleanup in derived classes.

**Signature:**
```cpp
virtual ~PBDevice();
```

---

## Virtual Functions (Override in Derived Classes)

### pbdInit()

Initializes or resets the device to default state. Derived classes should call the base implementation.

**Signature:**
```cpp
virtual void pbdInit();
```

**Base Implementation:**
- Sets `m_startTimeMS` to current time
- Sets `m_enabled` to false
- Sets `m_running` to false
- Resets `m_error` to 0
- Resets `m_state` to 0

**Example Override:**
```cpp
void MyDevice::pbdInit() {
    // Reset derived class variables
    m_outputActive = false;
    m_timeoutMS = 0;
    
    // Call base class implementation at the end
    PBDevice::pbdInit();
}
```

### pbdEnable()

Enables or disables the device. Use for global device control.

**Signature:**
```cpp
virtual void pbdEnable(bool enable);
```

**Parameters:**
- `enable` - true to enable, false to disable

**Base Implementation:**
- Sets `m_enabled` flag

**Example Override:**
```cpp
void MyDevice::pbdEnable(bool enable) {
    // Perform cleanup when disabling
    if (!enable && m_outputActive) {
        m_pEngine->SendOutputMsg(PB_OMSG_GENERIC_IO, m_outputId, 
                                PB_OFF, false);
        m_outputActive = false;
    }
    
    // Call base class implementation
    PBDevice::pbdEnable(enable);
}
```

### pdbStartRun()

Starts a device operation run. Sets enabled and running flags, records start time.

**Signature:**
```cpp
virtual void pdbStartRun();
```

**Base Implementation:**
- Sets `m_enabled` to true
- Sets `m_running` to true
- Updates `m_startTimeMS` to current time

**Example Override:**
```cpp
void MyDevice::pdbStartRun() {
    // Reset derived class run state
    m_cycleCount = 0;
    m_currentPhase = PHASE_START;
    
    // Call base class implementation
    PBDevice::pdbStartRun();
}
```

### pbdExecute()

**Pure virtual function** - must be implemented by derived classes. Called each frame to update device state.

**Signature:**
```cpp
virtual void pbdExecute() = 0;
```

**Implementation Guidelines:**
1. Check `m_enabled` flag early, return if disabled
2. Get current time from `m_pEngine->GetTickCountGfx()`
3. Use switch statement on `m_state` for state machine logic
4. Update outputs using `m_pEngine->SendOutputMsg()`
5. Set `m_running` to false when operation completes
6. Set `m_error` if error conditions occur

**Example:**
```cpp
void MyDevice::pbdExecute() {
    if (!m_enabled) return;
    
    unsigned long currentTimeMS = m_pEngine->GetTickCountGfx();
    
    switch (m_state) {
        case STATE_IDLE:
            // Wait for trigger condition
            break;
            
        case STATE_ACTIVE:
            // Perform timed operation
            if ((currentTimeMS - m_startTimeMS) >= 1000) {
                m_state = STATE_COMPLETE;
            }
            break;
            
        case STATE_COMPLETE:
            m_running = false;
            m_state = STATE_IDLE;
            break;
    }
}
```

---

## Non-Virtual Utility Functions

### pdbIsRunning()

Returns whether a device run is currently in progress.

**Signature:**
```cpp
bool pdbIsRunning() const;
```

**Returns:** true if device is running, false otherwise

**Example:**
```cpp
if (!myDevice.pdbIsRunning()) {
    // Start a new run
    myDevice.pdbStartRun();
}
```

### pbdIsError()

Checks if device is in an error state.

**Signature:**
```cpp
bool pbdIsError() const;
```

**Returns:** true if `m_error` is non-zero

**Example:**
```cpp
if (myDevice.pbdIsError()) {
    int errorCode = myDevice.pbdResetError();
    g_PBEngine.pbeSendConsole("Device error: " + std::to_string(errorCode));
}
```

### pbdResetError()

Returns the current error code and resets it to zero.

**Signature:**
```cpp
int pbdResetError();
```

**Returns:** Previous error code value

**Example:**
```cpp
// Check and clear error
int error = myDevice.pbdResetError();
if (error != 0) {
    // Handle error based on code
    switch (error) {
        case 1: // Timeout
            break;
        case 2: // Hardware failure
            break;
    }
}
```

### pbdSetState()

Sets the device state value.

**Signature:**
```cpp
void pbdSetState(unsigned int state);
```

**Parameters:**
- `state` - New state value

**Example:**
```cpp
myDevice.pbdSetState(STATE_READY);
```

### pbdGetState()

Returns the current device state.

**Signature:**
```cpp
unsigned int pbdGetState() const;
```

**Returns:** Current state value

**Example:**
```cpp
if (myDevice.pbdGetState() == STATE_READY) {
    // Device is ready for operation
}
```

---

## Protected Member Variables

Direct access for derived classes:

```cpp
unsigned long m_startTimeMS;  // Time when run started (milliseconds)
bool m_enabled;               // Device enabled or not
unsigned int m_state;         // Current state of device
bool m_running;               // Run in progress
int m_error;                  // Error code (0 = no error)
PBEngine* m_pEngine;          // Pointer to engine for timing/output
```

---

## Example: pbdEjector Class

The framework includes a complete example implementation: `pbdEjector`, a ball ejector device.

### Features

- Detects ball presence via input sensor
- Activates LED when ball detected
- Fires solenoid to eject ball
- Repeats ejection if ball remains
- Cleans up outputs on completion

### Class Declaration

```cpp
class pbdEjector : public PBDevice {
public:
    pbdEjector(PBEngine* pEngine, unsigned int inputId, 
              unsigned int ledOutputId, unsigned int solenoidOutputId);
    ~pbdEjector();

    void pbdInit() override;
    void pbdEnable(bool enable) override;
    void pdbStartRun() override;
    void pbdExecute() override;

private:
    unsigned int m_inputId;
    unsigned int m_ledOutputId;
    unsigned int m_solenoidOutputId;
    unsigned long m_solenoidStartMS;
    unsigned long m_solenoidOffMS;
    bool m_solenoidActive;
    bool m_ledActive;

    enum EjectorState {
        STATE_IDLE = 0,
        STATE_BALL_DETECTED = 1,
        STATE_SOLENOID_ON = 2,
        STATE_SOLENOID_OFF = 3,
        STATE_COMPLETE = 4
    };
};
```

### Usage Example

```cpp
// In your game class header
pbdEjector* m_leftScoop;

// In initialization
m_leftScoop = new pbdEjector(&g_PBEngine, 
                            INPUT_LEFT_SCOOP,      // Input sensor
                            LED_SCOOP_LEFT,        // LED output
                            OUTPUT_SCOOP_EJECT);   // Solenoid output

// Enable the device
m_leftScoop->pbdEnable(true);

// In main game loop - call every frame
m_leftScoop->pbdExecute();

// Check if ejection is complete
if (!m_leftScoop->pdbIsRunning() && 
    m_leftScoop->pbdGetState() == STATE_COMPLETE) {
    // Ball was ejected, award points
    AddScore(5000);
}
```

### State Machine Flow

```
STATE_IDLE
    ↓ (ball detected)
STATE_BALL_DETECTED
    ↓ (turn on LED and solenoid)
STATE_SOLENOID_ON
    ↓ (250ms elapsed)
STATE_SOLENOID_OFF (turn off solenoid)
    ↓ (250ms elapsed)
    ├─ Ball still present → STATE_SOLENOID_ON (retry)
    └─ Ball ejected → STATE_COMPLETE
```

---

## Creating Custom Devices

### Step-by-Step Guide

**1. Define Your Device Class**

```cpp
class MyCustomDevice : public PBDevice {
public:
    MyCustomDevice(PBEngine* pEngine, /* your params */);
    ~MyCustomDevice();
    
    void pbdInit() override;
    void pbdEnable(bool enable) override;
    void pdbStartRun() override;
    void pbdExecute() override;

private:
    // Your device-specific variables
    unsigned int m_myOutputId;
    unsigned long m_myTimer;
    
    // Define your states
    enum MyStates {
        STATE_IDLE = 0,
        STATE_PHASE1 = 1,
        STATE_PHASE2 = 2,
        STATE_COMPLETE = 3
    };
};
```

**2. Implement Constructor**

```cpp
MyCustomDevice::MyCustomDevice(PBEngine* pEngine, unsigned int outputId)
    : PBDevice(pEngine) {
    m_myOutputId = outputId;
    m_myTimer = 0;
    pbdInit();
}
```

**3. Override pbdInit()**

```cpp
void MyCustomDevice::pbdInit() {
    // Reset your variables
    m_myTimer = 0;
    
    // Call base class
    PBDevice::pbdInit();
}
```

**4. Override pbdEnable()**

```cpp
void MyCustomDevice::pbdEnable(bool enable) {
    if (!enable) {
        // Turn off outputs when disabling
        m_pEngine->SendOutputMsg(PB_OMSG_GENERIC_IO, m_myOutputId, 
                                PB_OFF, false);
    }
    
    PBDevice::pbdEnable(enable);
}
```

**5. Override pdbStartRun()**

```cpp
void MyCustomDevice::pdbStartRun() {
    // Reset run-specific state
    m_myTimer = 0;
    
    PBDevice::pdbStartRun();
}
```

**6. Implement pbdExecute()**

```cpp
void MyCustomDevice::pbdExecute() {
    if (!m_enabled) return;
    
    unsigned long currentTimeMS = m_pEngine->GetTickCountGfx();
    
    switch (m_state) {
        case STATE_IDLE:
            // Waiting for trigger
            break;
            
        case STATE_PHASE1:
            // First phase of operation
            m_pEngine->SendOutputMsg(PB_OMSG_GENERIC_IO, m_myOutputId, 
                                    PB_ON, false);
            m_myTimer = currentTimeMS;
            m_state = STATE_PHASE2;
            break;
            
        case STATE_PHASE2:
            // Wait for duration
            if ((currentTimeMS - m_myTimer) >= 500) {
                m_pEngine->SendOutputMsg(PB_OMSG_GENERIC_IO, m_myOutputId, 
                                        PB_OFF, false);
                m_state = STATE_COMPLETE;
            }
            break;
            
        case STATE_COMPLETE:
            m_running = false;
            m_state = STATE_IDLE;
            break;
            
        default:
            m_error = 1;  // Unknown state error
            m_state = STATE_IDLE;
            m_running = false;
            break;
    }
}
```

---

## Integration with Game Code

### Typical Usage Pattern

```cpp
// In your game class header (Pinball_Table.h)
class PBTable : public PBEngine {
private:
    pbdEjector* m_leftScoop;
    pbdEjector* m_rightScoop;
};

// In your initialization function
void PBTable::pbtInitTable() {
    // Create devices
    m_leftScoop = new pbdEjector(&g_PBEngine, 
                                INPUT_LEFT_SCOOP, 
                                LED_SCOOP_LEFT, 
                                OUTPUT_SCOOP_LEFT);
    
    m_rightScoop = new pbdEjector(&g_PBEngine,
                                 INPUT_RIGHT_SCOOP,
                                 LED_SCOOP_RIGHT,
                                 OUTPUT_SCOOP_RIGHT);
    
    // Enable devices
    m_leftScoop->pbdEnable(true);
    m_rightScoop->pbdEnable(true);
}

// In your main game loop
void PBTable::pbtUpdateGameLogic() {
    // Execute all devices every frame
    m_leftScoop->pbdExecute();
    m_rightScoop->pbdExecute();
    
    // Check for completion and award points
    if (!m_leftScoop->pdbIsRunning() && 
        m_leftScoop->pbdGetState() == 4) {  // STATE_COMPLETE
        AddScore(5000);
        pbeSendConsole("Left scoop ejected ball");
    }
    
    // Check for errors
    if (m_leftScoop->pbdIsError()) {
        int error = m_leftScoop->pbdResetError();
        pbeSendConsole("Scoop error: " + std::to_string(error));
    }
}

// Cleanup
void PBTable::pbtCleanup() {
    delete m_leftScoop;
    delete m_rightScoop;
}
```

---

## Best Practices

### State Management

- **Use enums for states**: Makes code readable and prevents magic numbers
- **Document state transitions**: Comment your state machine flow
- **Handle invalid states**: Always include default case with error handling
- **Keep states simple**: Each state should do one clear thing

### Timing

- **Use relative timing**: Calculate elapsed time from `m_startTimeMS`
- **Don't block**: Never use delays, always check elapsed time
- **Be consistent**: Use milliseconds throughout for timing

### Error Handling

- **Define error codes**: Create meaningful error codes for your device
- **Log errors**: Use `pbeSendConsole()` to report errors
- **Recover gracefully**: Reset to idle state on errors
- **Clear errors**: Check and clear errors regularly

### Output Control

- **Clean up outputs**: Turn off all outputs in `pbdEnable(false)`
- **Track output state**: Keep boolean flags for active outputs
- **Use pulse mode wisely**: Use `usePulse=true` for timed solenoids

### Memory Management

- **Allocate dynamically**: Create devices with `new` in initialization
- **Delete in destructor**: Clean up in your game class destructor
- **Avoid leaks**: Ensure every `new` has a matching `delete`

---

## Common Patterns

### Retry Pattern

For operations that may need multiple attempts:

```cpp
void MyDevice::pbdExecute() {
    if (!m_enabled) return;
    
    unsigned long currentTimeMS = m_pEngine->GetTickCountGfx();
    
    switch (m_state) {
        case STATE_ATTEMPT:
            // Try operation
            if (OperationSuccessful()) {
                m_state = STATE_COMPLETE;
            } else if (m_retryCount < MAX_RETRIES) {
                m_retryCount++;
                m_waitTimer = currentTimeMS;
                m_state = STATE_WAITING;
            } else {
                m_error = ERROR_MAX_RETRIES;
                m_state = STATE_IDLE;
            }
            break;
            
        case STATE_WAITING:
            if ((currentTimeMS - m_waitTimer) >= RETRY_DELAY_MS) {
                m_state = STATE_ATTEMPT;
            }
            break;
    }
}
```

### Timeout Pattern

For operations that should not run indefinitely:

```cpp
void MyDevice::pbdExecute() {
    if (!m_enabled) return;
    
    unsigned long currentTimeMS = m_pEngine->GetTickCountGfx();
    unsigned long elapsed = currentTimeMS - m_startTimeMS;
    
    // Check for timeout
    if (elapsed > TIMEOUT_MS) {
        m_error = ERROR_TIMEOUT;
        m_running = false;
        m_state = STATE_IDLE;
        return;
    }
    
    // Normal state machine
    switch (m_state) {
        // ...
    }
}
```

### Multi-Phase Pattern

For complex operations with multiple phases:

```cpp
enum Phases {
    PHASE_PREPARE,
    PHASE_EXECUTE,
    PHASE_COOLDOWN,
    PHASE_COMPLETE
};

void MyDevice::pbdExecute() {
    if (!m_enabled) return;
    
    unsigned long currentTimeMS = m_pEngine->GetTickCountGfx();
    
    switch (m_state) {
        case PHASE_PREPARE:
            // Setup for execution
            m_phaseTimer = currentTimeMS;
            m_state = PHASE_EXECUTE;
            break;
            
        case PHASE_EXECUTE:
            if ((currentTimeMS - m_phaseTimer) >= EXECUTE_TIME_MS) {
                m_phaseTimer = currentTimeMS;
                m_state = PHASE_COOLDOWN;
            }
            break;
            
        case PHASE_COOLDOWN:
            if ((currentTimeMS - m_phaseTimer) >= COOLDOWN_TIME_MS) {
                m_state = PHASE_COMPLETE;
            }
            break;
            
        case PHASE_COMPLETE:
            m_running = false;
            m_state = PHASE_PREPARE;  // Ready for next run
            break;
    }
}
```

---

## Troubleshooting

### Device Not Responding

**Symptom:** `pbdExecute()` called but nothing happens

**Solutions:**
- Verify device is enabled: `pbdEnable(true)`
- Check `m_enabled` flag in `pbdExecute()`
- Confirm `pdbStartRun()` was called to set `m_running`
- Add debug output to verify `pbdExecute()` is being called

### Outputs Not Activating

**Symptom:** State machine runs but outputs don't respond

**Solutions:**
- Verify output IDs are correct
- Check output message type matches hardware
- Confirm `m_pEngine` pointer is valid
- Test outputs directly with `SendOutputMsg()` first

### State Machine Stuck

**Symptom:** Device stays in one state indefinitely

**Solutions:**
- Add timeout checking to all waiting states
- Verify timing calculations are correct
- Check for missing state transitions
- Add error state for unhandled conditions

### Memory Leaks

**Symptom:** Memory usage grows over time

**Solutions:**
- Ensure every `new` has matching `delete`
- Use destructor to clean up derived class resources
- Avoid creating devices in loops
- Check for leaked output messages

---

## See Also

- **[PBEngine_API.md](PBEngine_API.md)** - Engine timing, timer system, and output control
- **[IO_Processing_API.md](IO_Processing_API.md)** - Input/output message details
- **[Game_Creation_API.md](Game_Creation_API.md)** - Game integration patterns
- **[Platform_Init_API.md](Platform_Init_API.md)** - Main loop and initialization
- **[RasPin_Overview.md](RasPin_Overview.md)** - Overall framework architecture

# Pinball Mode System Guide

## Overview

The RasPin Pinball Mode System provides a framework for managing multiple game modes within a pinball game. Each mode represents a distinct play state with its own rules, scoring, and screen displays. Modes can transition between each other based on game conditions, creating dynamic and engaging gameplay.

## Architecture

### Components

The mode system consists of three main components:

1. **Mode States** - Enum-based mode types and sub-states within each mode
2. **Mode Manager** - Functions that handle mode transitions and state updates
3. **Screen Manager** - Centralized system for managing screen display requests with priorities

### Mode Flow Hierarchy

```
PBTableState (Top Level)
    │
    ├─ PBTBL_INIT
    ├─ PBTBL_START
    ├─ PBTBL_MAINSCREEN ──────┐
    ├─ PBTBL_STDPLAY          │ Mode System Active
    ├─ PBTBL_RESET            │
    └─ PBTBL_END              ┘
                              │
                    ┌─────────┴─────────┐
                    │                   │
            MODE_NORMAL_PLAY     MODE_MULTIBALL
                    │                   │
            ┌───────┼───────┐   ┌──────┼──────┐
            │       │       │   │      │      │
         IDLE   ACTIVE  DRAIN START ACTIVE ENDING
```

## Core Data Structures

### PBTableMode Enum

Defines the different game modes available:

```cpp
enum class PBTableMode {
    MODE_NORMAL_PLAY = 0,     // Normal play mode - main gameplay
    MODE_MULTIBALL = 1,       // Multiball mode - triggered by specific conditions
    MODE_END
};
```

### Mode Sub-States

Each mode has its own set of sub-states:

**Normal Play Mode:**
```cpp
enum class PBNormalPlayState {
    NORMAL_IDLE = 0,          // Waiting for ball to be active
    NORMAL_ACTIVE = 1,        // Ball in play
    NORMAL_DRAIN = 2,         // Ball drained, handling end of ball
    NORMAL_END
};
```

**Multiball Mode:**
```cpp
enum class PBMultiballState {
    MULTIBALL_START = 0,      // Starting multiball sequence
    MULTIBALL_ACTIVE = 1,     // Multiball gameplay active
    MULTIBALL_ENDING = 2,     // Transitioning back to normal play
    MULTIBALL_END
};
```

### ModeState Structure

Tracks the current mode and all mode-specific state data:

```cpp
struct ModeState {
    PBTableMode currentMode;              // Current active mode
    PBTableMode previousMode;             // Previous mode (for returning)
    
    // Normal play mode state
    PBNormalPlayState normalPlayState;
    unsigned long normalPlayStateStartTick;
    
    // Multiball mode state
    PBMultiballState multiballState;
    unsigned long multiballStateStartTick;
    int multiballCount;                   // Number of balls in multiball
    bool multiballQualified;              // Is multiball qualified to start?
    
    // Mode transition tracking
    bool modeTransitionActive;
    unsigned long modeTransitionStartTick;
};
```

## Screen Management System

### Screen Priority System

The screen manager uses a priority-based queue system to determine which screen to display:

```cpp
enum class ScreenPriority {
    PRIORITY_LOW = 0,         // Normal gameplay screens
    PRIORITY_MEDIUM = 1,      // Mode transitions, bonus screens
    PRIORITY_HIGH = 2,        // Important events, jackpots
    PRIORITY_CRITICAL = 3     // Cannot be preempted (e.g., game over)
};
```

### Screen Request Structure

```cpp
struct ScreenRequest {
    int screenId;                    // ID of screen to display
    ScreenPriority priority;         // Priority level
    unsigned long durationMs;        // How long to display (0 = until cleared)
    unsigned long requestTick;       // When request was made
    bool canBePreempted;             // Can this screen be preempted by any higher priority request?
};
```

### Screen Manager Functions

- **pbeRequestScreen()** - Request a screen to be displayed
- **pbeUpdateScreenManager()** - Update screen queue and determine current screen
- **pbeClearScreenRequests()** - Clear all pending screen requests
- **pbeGetCurrentScreen()** - Get the currently displayed screen ID

## Mode System Flow Charts

### Overall Mode System Flow

```
┌──────────────────────────────────────────────────┐
│           Game Update Loop (Each Frame)          │
└──────────────────────────┬───────────────────────┘
                           │
                           ▼
              ┌────────────────────────┐
              │  pbeUpdateGameState()  │
              └────────────┬───────────┘
                           │
                           ▼
              ┌────────────────────────┐
              │  pbeUpdateModeState()  │
              │  - Check transitions   │
              │  - Update current mode │
              └────────────┬───────────┘
                           │
                           ▼
              ┌────────────────────────────┐
              │ pbeUpdateScreenManager()   │
              │ - Process screen queue     │
              │ - Handle priorities        │
              └────────────────────────────┘
```

### Mode Transition Flow

```
┌─────────────────────────────────────────────────────┐
│               Check Mode Transition                 │
│          pbeCheckModeTransition()                   │
└──────────────────┬──────────────────────────────────┘
                   │
                   ├───► Qualification Met? ───No───► Continue Current Mode
                   │                                          │
                   Yes                                        │
                   │                                          │
                   ▼                                          │
         ┌─────────────────┐                                │
         │  Exit Old Mode  │                                │
         │ pbeExitMode()   │                                │
         │ - Cleanup       │                                │
         │ - Save state    │                                │
         └────────┬────────┘                                │
                  │                                          │
                  ▼                                          │
         ┌─────────────────┐                                │
         │  Enter New Mode │                                │
         │ pbeEnterMode()  │                                │
         │ - Initialize    │                                │
         │ - Set state     │                                │
         │ - Request screen│                                │
         └────────┬────────┘                                │
                  │                                          │
                  └──────────────────────────────────────────┘
                                   │
                                   ▼
                        ┌──────────────────┐
                        │ Return to Update │
                        └──────────────────┘
```

### Normal Play Mode State Flow

```
              ┌──────────────┐
       ┌─────►│  NORMAL_IDLE │◄──────┐
       │      └──────┬───────┘       │
       │             │                │
       │     Ball Active Event        │
       │             │                │
       │             ▼                │
       │      ┌─────────────┐        │
       │      │NORMAL_ACTIVE│        │
       │      └──────┬──────┘        │
       │             │                │
       │  Check Multiball Qualified  │
       │             │                │
       │      Drain Event             │
       │             │                │
       │             ▼                │
       │      ┌─────────────┐        │
       │      │NORMAL_DRAIN │        │
       │      └──────┬──────┘        │
       │             │                │
       │       After Delay            │
       │             │                │
       └─────────────┘                │
                                      │
            Multiball Qualified       │
            & Triggered               │
                     │                │
                     ▼                │
            ┌────────────────┐       │
            │MODE_MULTIBALL  │───────┘
            │  (Transition)  │  Return
            └────────────────┘
```

### Multiball Mode State Flow

```
     Enter Multiball Mode
             │
             ▼
    ┌────────────────┐
    │MULTIBALL_START │
    │ - Show intro   │
    │ - Prepare game │
    └────────┬───────┘
             │
      3 second delay
             │
             ▼
    ┌────────────────┐
    │MULTIBALL_ACTIVE│
    │ - Track balls  │
    │ - Higher score │
    │ - Jackpots     │
    └────────┬───────┘
             │
    Only 1 ball remains
    or timeout (20s)
             │
             ▼
    ┌────────────────┐
    │MULTIBALL_ENDING│
    │ - Show total   │
    │ - Cleanup      │
    └────────┬───────┘
             │
      2 second delay
             │
             ▼
    ┌────────────────┐
    │ Exit Multiball │
    │ Return to      │
    │ Normal Play    │
    └────────────────┘
```

### Screen Manager Flow

```
┌────────────────────────────────────────────────┐
│          Screen Request Made                   │
│     pbeRequestScreen(id, priority, ...)        │
└─────────────────┬──────────────────────────────┘
                  │
                  ▼
         ┌────────────────┐
         │ Add to Queue   │
         └────────┬───────┘
                  │
                  ▼
    ┌──────────────────────────────┐
    │  pbeUpdateScreenManager()    │
    └──────────────┬───────────────┘
                   │
                   ▼
         ┌──────────────────┐
         │  Sort by Priority│
         │  (Highest First) │
         └──────────┬───────┘
                    │
                    ▼
         ┌──────────────────┐
         │ Get Top Request  │
         └──────────┬───────┘
                    │
        ┌───────────┴────────────┐
        │                        │
        ▼                        ▼
  Different from          Same as current
  current screen?              screen
        │                        │
        ▼                        │
  Can preempt? ──No──►      Continue ◄──┘
        │                        │
       Yes                       │
        │                        │
        ▼                        │
  ┌────────────┐                │
  │Change Screen│               │
  └──────┬─────┘                │
         │                      │
         └──────────┬───────────┘
                    │
                    ▼
         ┌──────────────────┐
         │ Remove Expired   │
         │ Requests         │
         └──────────────────┘
```

## Creating a New Mode

To add a new mode to the system, follow these steps:

### 1. Define Mode Enum

Add your new mode to `PBTableMode` in `Pinball_Table.h`:

```cpp
enum class PBTableMode {
    MODE_NORMAL_PLAY = 0,
    MODE_MULTIBALL = 1,
    MODE_YOUR_NEW_MODE = 2,  // Add here
    MODE_END
};
```

### 2. Define Mode Sub-States

Create an enum for your mode's sub-states:

```cpp
enum class PBYourModeState {
    YOURMODE_INIT = 0,
    YOURMODE_ACTIVE = 1,
    YOURMODE_COMPLETE = 2,
    YOURMODE_END
};
```

### 3. Add State Variables to ModeState

Add your mode's state tracking to the `ModeState` structure:

```cpp
struct ModeState {
    // ... existing members ...
    
    // Your new mode state
    PBYourModeState yourModeState;
    unsigned long yourModeStateStartTick;
    int yourModeSpecificData;
    
    // Update reset() method
    void reset() {
        // ... existing resets ...
        yourModeState = PBYourModeState::YOURMODE_INIT;
        yourModeStateStartTick = 0;
        yourModeSpecificData = 0;
    }
};
```

### 4. Create Mode Update Function

Implement a function to handle your mode's logic in `Pinball_Table.cpp`:

```cpp
void PBEngine::pbeUpdateYourMode(stInputMessage inputMessage, unsigned long currentTick) {
    ModeState& modeState = m_playerStates[m_currentPlayer].modeState;
    
    // Handle your mode's state machine
    switch (modeState.yourModeState) {
        case PBYourModeState::YOURMODE_INIT:
            // Initialization logic
            modeState.yourModeState = PBYourModeState::YOURMODE_ACTIVE;
            modeState.yourModeStateStartTick = currentTick;
            break;
            
        case PBYourModeState::YOURMODE_ACTIVE:
            // Active gameplay logic
            // Check for completion conditions
            break;
            
        case PBYourModeState::YOURMODE_COMPLETE:
            // Completion logic
            // Exit mode when done
            break;
    }
}
```

### 5. Add Mode Entry/Exit Logic

Update `pbeEnterMode()` and `pbeExitMode()` in `Pinball_Table.cpp`:

```cpp
void PBEngine::pbeEnterMode(PBTableMode newMode, unsigned long currentTick) {
    // ... existing code ...
    
    switch (newMode) {
        // ... existing modes ...
        
        case PBTableMode::MODE_YOUR_NEW_MODE:
            modeState.yourModeState = PBYourModeState::YOURMODE_INIT;
            modeState.yourModeStateStartTick = currentTick;
            
            // Request your mode's screen
            pbeRequestScreen(300, ScreenPriority::PRIORITY_MEDIUM, 0, true);
            break;
    }
}

void PBEngine::pbeExitMode(PBTableMode exitingMode, unsigned long currentTick) {
    // ... existing code ...
    
    switch (exitingMode) {
        // ... existing modes ...
        
        case PBTableMode::MODE_YOUR_NEW_MODE:
            // Cleanup your mode's state
            break;
    }
}
```

### 6. Add Qualification Check

Create a function to check if your mode should activate:

```cpp
bool PBEngine::pbeCheckYourModeQualified() {
    // Return true when your mode should activate
    // Example: return specific targets were hit
    return false;
}
```

### 7. Update Mode Transition Logic

Add your mode transition check to `pbeCheckModeTransition()`:

```cpp
bool PBEngine::pbeCheckModeTransition(unsigned long currentTick) {
    ModeState& modeState = m_playerStates[m_currentPlayer].modeState;
    
    switch (modeState.currentMode) {
        case PBTableMode::MODE_NORMAL_PLAY: {
            // ... existing checks ...
            
            // Check for your mode
            if (pbeCheckYourModeQualified()) {
                pbeExitMode(PBTableMode::MODE_NORMAL_PLAY, currentTick);
                pbeEnterMode(PBTableMode::MODE_YOUR_NEW_MODE, currentTick);
                return true;
            }
            break;
        }
    }
    return false;
}
```

### 8. Integrate into Update Loop

Add your mode to the switch statement in `pbeUpdateModeState()` and `pbeUpdateGameState()`:

```cpp
switch (modeState.currentMode) {
    // ... existing modes ...
    
    case PBTableMode::MODE_YOUR_NEW_MODE:
        pbeUpdateYourMode(inputMessage, currentTick);
        break;
}
```

## Screen Management Best Practices

### 1. Use Appropriate Priorities

- **PRIORITY_LOW** - Normal scoring displays, ongoing gameplay
- **PRIORITY_MEDIUM** - Mode starts, lane completion, multiplier changes
- **PRIORITY_HIGH** - Jackpots, mode completions, extra balls
- **PRIORITY_CRITICAL** - Ball save warnings, game over screens

### 2. Set Duration Appropriately

- **0 ms** - Stays until cleared (for ongoing displays)
- **2000-3000 ms** - Quick information displays
- **5000+ ms** - Important information that needs time to read

### 3. Use Preemption Wisely

- Set `canBePreempted = false` for critical information
- Set `canBePreempted = true` for normal displays that can be interrupted

### Example Screen Requests:

```cpp
// Normal scoring display - can be interrupted
pbeRequestScreen(SCREEN_SCORE, ScreenPriority::PRIORITY_LOW, 0, true);

// Mode start - stays for 3 seconds, can be preempted by higher priority
pbeRequestScreen(SCREEN_MODE_START, ScreenPriority::PRIORITY_MEDIUM, 3000, true);

// Jackpot - stays for 5 seconds, cannot be preempted
pbeRequestScreen(SCREEN_JACKPOT, ScreenPriority::PRIORITY_HIGH, 5000, false);

// Game over - stays until cleared, cannot be preempted
pbeRequestScreen(SCREEN_GAMEOVER, ScreenPriority::PRIORITY_CRITICAL, 0, false);
```

## Example: Implementing a Wizard Mode

Here's a complete example of implementing a wizard mode:

### 1. Add to enum:
```cpp
enum class PBTableMode {
    MODE_NORMAL_PLAY = 0,
    MODE_MULTIBALL = 1,
    MODE_WIZARD = 2,
    MODE_END
};
```

### 2. Create sub-states:
```cpp
enum class PBWizardState {
    WIZARD_INTRO = 0,
    WIZARD_COMBAT = 1,
    WIZARD_VICTORY = 2,
    WIZARD_END
};
```

### 3. Add to ModeState:
```cpp
PBWizardState wizardState;
unsigned long wizardStateStartTick;
int wizardHitsRemaining;
bool wizardQualified;
```

### 4. Implement qualification:
```cpp
bool PBEngine::pbeCheckWizardQualified() {
    // Qualified when all three characters are level 5+
    return (m_playerStates[m_currentPlayer].knightLevel >= 5 &&
            m_playerStates[m_currentPlayer].priestLevel >= 5 &&
            m_playerStates[m_currentPlayer].rangerLevel >= 5);
}
```

### 5. Create update function with full state flow and appropriate screens at each stage.

## Testing Your Mode

1. **Test Mode Entry** - Verify qualification conditions work correctly
2. **Test State Flow** - Ensure sub-states progress properly
3. **Test Mode Exit** - Confirm cleanup happens correctly
4. **Test Screen Display** - Verify correct screens show at right times
5. **Test Transitions** - Check transitions to/from other modes work

## Debugging Tips

1. **Use Console Output** - Add `pbeSendConsole()` calls to track state changes
2. **Check Mode State** - Print current mode and sub-state each frame
3. **Monitor Screen Queue** - Log screen requests and changes
4. **Verify Timing** - Ensure tick-based timing is working correctly
5. **Test Edge Cases** - Try rapid inputs, mode switches, and timeouts

## Integration with Existing Code

The mode system integrates with the existing table state system:

- **PBTBL_MAINSCREEN** and **PBTBL_STDPLAY** both use the mode system
- Mode state is per-player (stored in `pbGameState`)
- Screen manager is global (shared across all players)
- Mode transitions can happen at any time based on game conditions

## Summary

The mode system provides:
- ✅ Multiple independent game modes
- ✅ Each mode with its own state machine
- ✅ Centralized screen management with priorities
- ✅ Clean transition system between modes
- ✅ Per-player mode state tracking
- ✅ Extensible architecture for adding new modes

This architecture allows for complex, engaging pinball gameplay with multiple layers of rules and objectives, all managed in a clean and maintainable way.

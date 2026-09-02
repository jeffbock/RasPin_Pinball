# Pinball Mode System Guide

## Overview

The RasPin Pinball Mode System provides a framework for managing multiple game states within a pinball game. Each mode (table state) represents a distinct play state with its own rules, rendering, and screen display. The screen manager decouples *what state the game is in* from *what is currently shown on screen*, enabling timed overlays and priority-based preemption without changing game logic. See [Screen Manager API](Screen_Manager_API.md) for the request queue and public API.

## Architecture

### Components

The mode system consists of three main components:

1. **Table States** — `PBTableState` enum values (e.g. `PBTBL_START`, `PBTBL_MAIN`), each with their own sub-state enum defined in the mode's header file
2. **The Three-Function Pattern** — Every mode is implemented as exactly three functions: Load, Render, and UpdateState
3. **Screen Manager** — Centralized system managing screen display requests with priority levels and optional timed expiry

### Table State Hierarchy

```
PBTableState (Top Level)
    │
    ├─ PBTBL_INIT        — One-time hardware/engine setup; no sub-states
    ├─ PBTBL_START       — Attract screen: sub-states START_START / INST / SCORES / OPENDOOR
    ├─ PBTBL_MAIN        — Gameplay: sub-states MAIN_NORMAL / EXTRABALL / BALLSAVED / INN_OPEN / KEY_OBTAINED
    ├─ PBTBL_RESET       — Reset confirmation; no sub-states
    ├─ PBTBL_PLAYEREND   — Between-turn transition: sub-states PLAYEREND_DISPLAY / EJECTING
    ├─ PBTBL_GAMEEND     — High-score entry: sub-states GAMEEND_ENTERINITIALS / COMPLETE
    ├─ PBTBL_INTOWER     — Tower lock mode: single screen state; internal climb/fight/challenge transitions stay self-contained
    └─ PBTBL_DRAGONMULTIBALL — Dragon battle placeholder: left/right activation selects failure/victory
```

### InTower Internal Flow

`PBTBL_INTOWER` intentionally keeps its detailed transitions inside the mode rather than registering each one with the screen manager. The mode owns `TowerInit`, `TowerClimb`, `RoomFight`, floor-challenge video/roll/success, exit, and dragon-video flow, while the screen manager continues to display the outer InTower screen. This keeps animation timing, D20 input routing, and persisted tower progress in one place.

Tower progress belongs to the active player's `pbGameState`. A new player starts with `towerNeedsReset`; `TowerInit` generates the selected level once and clears that flag. Re-entry resets tower HP to 20 while retaining doors, remaining enemies, selected staircase challenges, floor, and resume door. A DragonMultiball victory advances through Level 3, resets to floor 1, and sets `towerNeedsReset` for the next entry.

### The Three-Function Pattern

Every table mode is implemented in its own `.cpp` file as exactly three functions:

```
pbeLoadX()           — Run once on first use (guard-flagged). Load textures, create
                       animations, configure hardware. Returns false on failure.

pbeRenderX(tick, lastTick, subScreenState)
                     — Called every frame. Calls Load first. Dispatches on the
                       subScreenState value delivered by the screen manager.

pbeUpdateStateX(inputMessage)
                     — Called once per input message. Drives sub-state transitions:
                       writes m_tableSubScreenState AND calls pbeRequestScreen() in
                       parallel so the screen manager stays synchronized.
```

The key insight is that `pbeRenderX` receives its `subScreenState` argument from `pbeGetCurrentSubScreenState()` (via the render dispatch in `pbeRenderGameScreen`), not by reading `m_tableSubScreenState` directly. This means the screen manager fully controls what is displayed, including priority-based overlays that temporarily supersede the persistent background state.

### How Sub-States Flow Through the System

```
pbeUpdateStateX():                      pbeRenderGameScreen():
  Transition detected                     (called every frame)
  │                                       │
  ├─ m_tableSubScreenState = NEW          ├─ pbeRequestScreen(state, m_tableSubScreenState,
  └─ pbeRequestScreen(state, NEW,         │    PRIORITY_LOW, 0, true)  ← background request
       PRIORITY_LOW, 0, true)             │
                                          ├─ pbeUpdateScreenManager(currentTick)
  High-priority overlays:                 │    ← resolves priorities, expires timed requests
  └─ pbeRequestScreen(state, OVERLAY,     │
       PRIORITY_HIGH, durationMs, true)   ├─ currentSubScreenState = pbeGetCurrentSubScreenState()
       ← expires automatically            │
                                          └─ pbeRenderX(tick, lastTick, currentSubScreenState)
                                               ← render uses whatever the screen manager chose
```

This means the screen manager can display a timed `MAIN_EXTRABALL` overlay from `PRIORITY_HIGH` while the persistent `MAIN_NORMAL` background sits at `PRIORITY_LOW`. When the overlay expires, the background takes over automatically — the game logic never changes.

## Sub-State Enum Pattern

Each mode defines its sub-states in `tablemodes/Pinball_Table_ModeX.h`:

```cpp
// Example: Pinball_Table_ModeYourMode.h
enum class PBTBLYourModeScreenState {
    YOURMODE_SCREEN_A = 0,   // First display state
    YOURMODE_SCREEN_B = 1,   // Second display state
    YOURMODE_SCREEN_END
};
```

The render function accepts this enum directly:

```cpp
bool PBEngine::pbeRenderYourMode(unsigned long currentTick, unsigned long lastTick,
                                  PBTBLYourModeScreenState subScreenState) {
    if (!pbeLoadYourMode()) return false;

    switch (subScreenState) {
        case PBTBLYourModeScreenState::YOURMODE_SCREEN_A:
            // draw state A
            break;
        case PBTBLYourModeScreenState::YOURMODE_SCREEN_B:
            // draw state B
            break;
    }
    return true;
}
```

The update function writes `m_tableSubScreenState` and calls `pbeRequestScreen` together on every transition:

```cpp
void PBEngine::pbeUpdateStateYourMode(stInputMessage inputMessage) {
    PBTBLYourModeScreenState currentState =
        static_cast<PBTBLYourModeScreenState>(m_tableSubScreenState);

    switch (currentState) {
        case PBTBLYourModeScreenState::YOURMODE_SCREEN_A:
            if (/* transition condition */) {
                m_tableSubScreenState = static_cast<int>(PBTBLYourModeScreenState::YOURMODE_SCREEN_B);
                pbeRequestScreen(PBTableState::PBTBL_YOURMODE,
                                 static_cast<int>(PBTBLYourModeScreenState::YOURMODE_SCREEN_B),
                                 ScreenPriority::PRIORITY_LOW, 0, true);
            }
            break;
        // ...
    }
}
```

## Screen Management System

The queue API, request semantics, expiry behavior, and render-dispatch contract
are defined in [Screen Manager API](Screen_Manager_API.md). This guide explains
how table modes use that API alongside their own state and input handling.

### Screen Priority System

The screen manager uses a priority-based queue system to determine which screen to display:

```cpp
enum class ScreenPriority {
    PRIORITY_LOW = 0,         // Persistent background (current game state)
    PRIORITY_MEDIUM = 1,      // Mode transitions, bonus screens
    PRIORITY_HIGH = 2,        // Important events (extra ball, ball saved, inn open)
    PRIORITY_CRITICAL = 3     // Cannot be preempted (e.g., game over)
};
```

### Screen Request Structure

```cpp
struct ScreenRequest {
    PBTableState tableState;         // Main table state to display
    int subScreenState;              // Sub-state enum value cast to int
    ScreenPriority priority;         // Priority level
    unsigned long durationMs;        // How long to display (0 = until cleared)
    unsigned long requestTick;       // When request was made
    bool canBePreempted;             // Can this screen be preempted by a higher priority request?
};
```

### Screen Manager Functions

| Function | Purpose |
|----------|---------|
| `pbeRequestScreen(state, sub, priority, durationMs, canBePreempted)` | Queue a screen display request |
| `pbeUpdateScreenManager(currentTick)` | Process queue, expire timed requests, select current display |
| `pbeClearScreenRequests()` | Clear all pending requests (use at state transitions) |
| `pbeClearPriority0Screen()` | Clear only the persistent background, leaving overlays queued |
| `pbeGetCurrentScreenState()` | Return the currently displayed `PBTableState` |
| `pbeGetCurrentSubScreenState()` | Return the currently displayed sub-state (int) |

### Screen Manager Flow

```
┌────────────────────────────────────────────────┐
│          Screen Request Made                   │
│  pbeRequestScreen(state, sub, priority, ...)   │
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
         │ Remove Expired   │
         │ Requests         │
         └──────────┬───────┘
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
        │
       Yes
        │
        ▼
  ┌────────────┐
  │Change Screen│
  └──────┬─────┘
         │
         ▼
  m_currentDisplayedTableState / m_currentDisplayedSubScreenState
  updated ← render functions read from here via pbeGetCurrentSubScreenState()
```

## Creating a New Mode

Follow these steps in order when creating a new table mode. The "DoD checklist" in `DoDTable.md` mirrors this but is tailored to that specific table.

### 1. Add a Value to `PBTableState`

In `src/system/Pinball_Table.h`:

```cpp
enum class PBTableState {
    PBTBL_INIT = 0,
    PBTBL_START,
    PBTBL_MAIN,
    PBTBL_RESET,
    PBTBL_PLAYEREND,
    PBTBL_INTOWER,
    PBTBL_GAMEEND,
    PBTBL_YOURMODE,   // ← Add here
    PBTBL_END
};
```

### 2. Create the Mode Header

Create `src/user/tablemodes/Pinball_Table_ModeYourMode.h`:

```cpp
#ifndef Pinball_Table_ModeYourMode_h
#define Pinball_Table_ModeYourMode_h

// Screen sub-states for PBTBL_YOURMODE
enum class PBTBLYourModeScreenState {
    YOURMODE_SCREEN_A = 0,   // First display variant
    YOURMODE_SCREEN_B = 1,   // Second display variant
    YOURMODE_SCREEN_END
};

#endif // Pinball_Table_ModeYourMode_h
```

### 3. Include the Header in `Pinball_Table.h`

```cpp
#include "tablemodes/Pinball_Table_ModeYourMode.h"
```

### 4. Add Function Declarations to `Pinball_Engine.h`

```cpp
bool pbeLoadYourMode();
bool pbeRenderYourMode(unsigned long currentTick, unsigned long lastTick,
                       PBTBLYourModeScreenState subScreenState);
void pbeUpdateStateYourMode(stInputMessage inputMessage);

bool m_yourModeLoaded = false;   // load guard
```

### 5. Create the Mode Implementation

Create `src/user/tablemodes/Pinball_Table_ModeYourMode.cpp`:

```cpp
#include "Pinball_Engine.h"
#include "Pinball_Table.h"

// ── Load (guard-flagged) ────────────────────────────────────────────────
bool PBEngine::pbeLoadYourMode() {
    if (m_yourModeLoaded) return true;
    // Load textures, set up animations, etc.
    m_yourModeLoaded = true;
    return true;
}

// ── Render ──────────────────────────────────────────────────────────────
// subScreenState comes from pbeGetCurrentSubScreenState() — do NOT read
// m_tableSubScreenState directly here.
bool PBEngine::pbeRenderYourMode(unsigned long currentTick, unsigned long lastTick,
                                  PBTBLYourModeScreenState subScreenState) {
    if (!pbeLoadYourMode()) return false;

    switch (subScreenState) {
        case PBTBLYourModeScreenState::YOURMODE_SCREEN_A:
            // draw state A
            break;
        case PBTBLYourModeScreenState::YOURMODE_SCREEN_B:
            // draw state B
            break;
        default:
            break;
    }
    return true;
}

// ── Update ──────────────────────────────────────────────────────────────
// On every sub-state transition: write m_tableSubScreenState AND call
// pbeRequestScreen() so the screen manager stays synchronized.
void PBEngine::pbeUpdateStateYourMode(stInputMessage inputMessage) {
    PBTBLYourModeScreenState currentState =
        static_cast<PBTBLYourModeScreenState>(m_tableSubScreenState);

    switch (currentState) {
        case PBTBLYourModeScreenState::YOURMODE_SCREEN_A:
            if (/* transition condition */) {
                m_tableSubScreenState =
                    static_cast<int>(PBTBLYourModeScreenState::YOURMODE_SCREEN_B);
                pbeRequestScreen(PBTableState::PBTBL_YOURMODE,
                    static_cast<int>(PBTBLYourModeScreenState::YOURMODE_SCREEN_B),
                    ScreenPriority::PRIORITY_LOW, 0, true);
            }
            break;
        case PBTBLYourModeScreenState::YOURMODE_SCREEN_B:
            if (/* exit condition */) {
                // Transition to another top-level state
                pbeClearScreenRequests();
                m_tableState = PBTableState::PBTBL_MAIN;
                m_tableSubScreenState =
                    static_cast<int>(PBTBLMainScreenState::MAIN_NORMAL);
                pbeRequestScreen(PBTableState::PBTBL_MAIN,
                    static_cast<int>(PBTBLMainScreenState::MAIN_NORMAL),
                    ScreenPriority::PRIORITY_LOW, 0, true);
            }
            break;
        default:
            break;
    }
}
```

### 6. Add Dispatch Cases in `Pinball_Table.cpp`

In `pbeRenderGameScreen()`:

```cpp
// Background request (keeps screen manager in sync with m_tableState)
case PBTableState::PBTBL_YOURMODE:
    pbeRequestScreen(PBTableState::PBTBL_YOURMODE, m_tableSubScreenState,
                     ScreenPriority::PRIORITY_LOW, 0, true);
    break;

// Render dispatch
case PBTableState::PBTBL_YOURMODE: {
    PBTBLYourModeScreenState yourModeState =
        static_cast<PBTBLYourModeScreenState>(currentSubScreenState);
    success = pbeRenderYourMode(currentTick, lastTick, yourModeState);
    break;
}
```

In `pbeUpdateGameState()`:

```cpp
case PBTableState::PBTBL_YOURMODE:
    pbeUpdateStateYourMode(inputMessage);
    break;
```

### 7. Reset the Load Guard in `pbeTableReload()`

```cpp
m_yourModeLoaded = false;
```

### 8. Add to `CMakeLists.txt`

Add `src/user/tablemodes/Pinball_Table_ModeYourMode.cpp` to the source file list.

### 9. Trigger Entry from Another Mode

From the mode that should transition into yours:

```cpp
pbeClearScreenRequests();
m_tableState = PBTableState::PBTBL_YOURMODE;
m_tableSubScreenState = static_cast<int>(PBTBLYourModeScreenState::YOURMODE_SCREEN_A);
pbeRequestScreen(PBTableState::PBTBL_YOURMODE,
    static_cast<int>(PBTBLYourModeScreenState::YOURMODE_SCREEN_A),
    ScreenPriority::PRIORITY_LOW, 0, true);
```

## Screen Management Best Practices

### 1. Use Appropriate Priorities

| Priority | Enum | Use for |
|----------|------|---------|
| 0 | `PRIORITY_LOW` | Persistent background state (the current `m_tableState`/sub-state) |
| 1 | `PRIORITY_MEDIUM` | Mode transitions, bonus start screens |
| 2 | `PRIORITY_HIGH` | Timed event overlays: extra ball, ball saved, inn open, key obtained |
| 3 | `PRIORITY_CRITICAL` | Screens that must not be preempted (reserved for critical use) |

### 2. Set Duration Appropriately

- **0 ms** — Stays until cleared (`pbeClearScreenRequests()`). Always use for the persistent `PRIORITY_LOW` background request.
- **2000–3000 ms** — Short event displays (ball saved, inn open, key obtained).
- **4000–5000 ms** — Longer animated sequences (extra ball video).

### 3. Use Preemption Wisely

- Set `canBePreempted = true` for all `PRIORITY_LOW` backgrounds and most overlays.
- Set `canBePreempted = false` only when an overlay absolutely must not be cut short.

### 4. Always Pair `m_tableSubScreenState` Writes with `pbeRequestScreen()`

Every time you assign `m_tableSubScreenState`, call `pbeRequestScreen()` with the same values in the same code path. This is what keeps the screen manager synchronized with game state.

```cpp
// ✅ Correct: write both at the same time
m_tableSubScreenState = static_cast<int>(PBTBLYourModeScreenState::YOURMODE_SCREEN_B);
pbeRequestScreen(PBTableState::PBTBL_YOURMODE,
    static_cast<int>(PBTBLYourModeScreenState::YOURMODE_SCREEN_B),
    ScreenPriority::PRIORITY_LOW, 0, true);

// ❌ Wrong: only writing m_tableSubScreenState (screen manager won't know)
m_tableSubScreenState = static_cast<int>(PBTBLYourModeScreenState::YOURMODE_SCREEN_B);
```

### 5. Example Screen Requests

```cpp
// Persistent background for PBTBL_MAIN / MAIN_NORMAL
pbeRequestScreen(PBTableState::PBTBL_MAIN,
    static_cast<int>(PBTBLMainScreenState::MAIN_NORMAL),
    ScreenPriority::PRIORITY_LOW, 0, true);

// Timed "Extra Ball!" overlay — automatically expires after 3.9 s
pbeRequestScreen(PBTableState::PBTBL_MAIN,
    static_cast<int>(PBTBLMainScreenState::MAIN_EXTRABALL),
    ScreenPriority::PRIORITY_HIGH, 3900, true);

// Timed "Ball Saved" overlay — expires after 2 s
pbeRequestScreen(PBTableState::PBTBL_MAIN,
    static_cast<int>(PBTBLMainScreenState::MAIN_BALLSAVED),
    ScreenPriority::PRIORITY_HIGH, 2000, true);
```

## Testing Your Mode

1. **Test Mode Entry** — Verify the trigger sets `m_tableState`, `m_tableSubScreenState`, and calls `pbeRequestScreen` correctly.
2. **Test Sub-State Transitions** — Confirm sub-states advance on the right inputs/timers, and that `pbeRequestScreen` is called on each transition.
3. **Test Priority Overlays** — If your mode uses timed overlays, verify they expire and fall back to the background automatically.
4. **Test Mode Exit** — Confirm cleanup (reset load guard, clear screen requests, set new `m_tableState`) happens correctly.
5. **Test Render Dispatch** — Ensure `pbeRenderX` correctly dispatches on each sub-state value.

## Debugging Tips

1. **Console Output** — Add `pbeSendConsole()` calls at every sub-state transition to trace the flow.
2. **Screen Manager State** — Log `pbeGetCurrentScreenState()` and `pbeGetCurrentSubScreenState()` each frame to confirm the screen manager is tracking what you expect.
3. **Load Guard** — If rendering looks wrong on re-entry, check whether the load guard (`m_xLoaded`) was reset in `pbeTableReload()`.
4. **Tick Timing** — Always compare against `currentTick - startTick` rather than incrementing a counter.

## Summary

The mode system provides:
- ✅ All top-level states and their sub-states managed through the screen manager
- ✅ Priority-based preemption for timed overlays without changing game logic
- ✅ Consistent three-function pattern (Load / Render / UpdateState) per mode
- ✅ Render functions always receive sub-state from `pbeGetCurrentSubScreenState()` — never read `m_tableSubScreenState` directly in a render function
- ✅ Update functions always keep `m_tableSubScreenState` and `pbeRequestScreen()` in sync
- ✅ Extensible: adding a new mode requires only a header, a `.cpp`, and dispatch cases

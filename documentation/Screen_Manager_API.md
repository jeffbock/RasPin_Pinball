# Screen Manager API Reference

## Overview

The screen manager separates the current table state from the screen currently displayed.
It keeps one priority-0 background request and can temporarily show a higher-priority request.

Most modes have a persistent main display, but gameplay also needs short bursts of
presentation: an extra-ball animation, a status callout, a cutscene, or a video.
Without a screen manager, every mode would need to suspend its own display, track
the temporary screen's timer, restore its previous screen, and handle competing
requests. That support logic quickly becomes duplicated and fragile.

The screen manager puts those requests into one priority-based queue. Mode code
continues to own its rules and persistent background, while the manager selects
and expires temporary displays before returning to that background automatically.
This keeps overlays, videos, and short animations consistent across table modes.

Each game frame, `pbeRenderGameScreen()` requests the table state's background,
calls `pbeUpdateScreenManager(currentTick)`, and renders the selected state and sub-state.

## Priority Levels

```cpp
enum class ScreenPriority {
    PRIORITY_LOW = 0,
    PRIORITY_MEDIUM = 1,
    PRIORITY_HIGH = 2,
    PRIORITY_CRITICAL = 3
};
```

| Priority | Intended use |
|---|---|
| `PRIORITY_LOW` | Persistent current-mode background |
| `PRIORITY_MEDIUM` | Mode transitions and bonus screens |
| `PRIORITY_HIGH` | Timed gameplay overlays |
| `PRIORITY_CRITICAL` | Important display that supersedes other requests |

Only one `PRIORITY_LOW` request is retained. The highest queued non-zero priority
is displayed. When it expires, the manager returns to the priority-0 background.

## Request Data

```cpp
struct ScreenRequest {
    PBTableState tableState;
    int subScreenState;
    ScreenPriority priority;
    unsigned long durationMs;
    unsigned long requestTick;
    bool canBePreempted;
};
```

`durationMs = 0` keeps a request queued until it is cleared; use that for the background.
`canBePreempted` is currently stored but not evaluated: every request above priority 0
can supersede the background.

## Public API

```cpp
void pbeRequestScreen(PBTableState tableState, int subScreenState,
                      ScreenPriority priority, unsigned long durationMs,
                      bool canBePreempted);
void pbeUpdateScreenManager(unsigned long currentTick);
void pbeClearScreenRequests();
void pbeClearPriority0Screen();
PBTableState pbeGetCurrentScreenState();
int pbeGetCurrentSubScreenState();
```

`pbeRequestScreen()` ignores a request for an already queued `(tableState, subScreenState)`
pair, even when its priority or duration differs. A new priority-0 request replaces the old
priority-0 background.

```cpp
// Persistent background for the current mode.
pbeRequestScreen(PBTableState::PBTBL_MAIN,
                 static_cast<int>(PBTBLMainScreenState::MAIN_NORMAL),
                 ScreenPriority::PRIORITY_LOW, 0, true);

// Timed overlay for 3.9 seconds.
pbeRequestScreen(PBTableState::PBTBL_MAIN,
                 static_cast<int>(PBTBLMainScreenState::MAIN_EXTRABALL),
                 ScreenPriority::PRIORITY_HIGH, 3900, true);
```

Call `pbeUpdateScreenManager(currentTick)` once per frame before querying its selection.
It expires timed non-zero requests and selects the highest remaining non-zero request or
the persistent background. `pbeClearScreenRequests()` clears every request;
`pbeClearPriority0Screen()` leaves queued overlays intact.

## Mode Integration

On every sub-state transition, assign `m_tableSubScreenState` and request the same value.

```cpp
m_tableSubScreenState = static_cast<int>(PBTBLYourModeScreenState::SCREEN_B);
pbeRequestScreen(PBTableState::PBTBL_YOUR_MODE, m_tableSubScreenState,
                 ScreenPriority::PRIORITY_LOW, 0, true);
```

Render functions must use their manager-provided `subScreenState` argument, not
`m_tableSubScreenState` directly. Complex modes can retain one screen-manager state and
keep their internal animation and gameplay phases in private members; InTower does this.

## See Also

- [Mode System Guide](Mode_System_Guide.md) - Table modes and sub-state patterns
- [Game Screen Creation Guide](Game_Creation_API.md) - Screen lifecycle and resources
- [PBEngine API](PBEngine_API.md) - Core engine state and timers
- [Dragons of Destiny Table Spec](DoDTable.md) - Production mode examples
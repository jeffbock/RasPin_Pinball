# Game Screen Creation Guide

## Overview

This guide describes how to organize a table screen. API details for rendering,
audio, video, and devices live in their dedicated references:

- [PBGfx 2D Graphics API](PBGfx_2D_API.md)
- [PBSound API](PBSound_API.md)
- [PBVideoPlayer API](PBVideo_API.md)
- [PBDevice API](PBDevice_API.md)
- [PB3D API](PB3D_API.md)
- [Screen Manager API](Screen_Manager_API.md)

## Screen Lifecycle

A screen normally has three responsibilities:

1. **Load** resources once and retain their IDs in `PBEngine` members.
2. **Reset** transient state when the screen is entered or restarted.
3. **Render and update** the screen each frame using `currentTick`.

Keep resource allocation out of the per-frame path. A load function should be
idempotent so the render function can safely call it as a guard.

```cpp
bool PBEngine::pbeLoadYourScreen() {
    if (m_yourScreenLoaded) return true;

    // Load this screen's resources and store IDs in PBEngine members.

    m_yourScreenLoaded = true;
    return true;
}

bool PBEngine::pbeRenderYourScreen(unsigned long currentTick,
                                   unsigned long lastTick) {
    if (!pbeLoadYourScreen()) return false;

    if (m_restartYourScreen) {
        m_restartYourScreen = false;
        // Reset timers, selections, and one-shot presentation state here.
    }

    // Update time-based game state, then render this screen's layers.
    return true;
}
```

## State And Input

Use a screen sub-state enum when a mode has distinct screens that the screen
manager must prioritize or replace. For a complex single screen, keep its
animation and interaction phases in private member variables instead of adding
screen-manager states for every visual transition.

See [Screen Manager API](Screen_Manager_API.md) for requests, priorities,
timed overlays, and render-dispatch rules.

Process input only for the active state and keep each action small: update the
mode state, start an effect, or request the next screen. Time-based transitions
should compare `currentTick` with a stored start tick; do not block the game loop.

## Resource Ownership

- Keep sprite, model, and video IDs in `PBEngine` members when they are used by
  more than one frame.
- Create sprite instances and animation definitions during loading, then reuse
  them during rendering.
- Stop and unload a `PBVideoPlayer` when leaving a screen or replacing its video.
- Keep frequently reused resources resident; release one-time resources when the
  next screen no longer needs them.
- Handle a failed load immediately and avoid rendering an invalid ID.

## Table Screen Checklist

- Add the screen's state and render dispatch using the patterns in existing
  table modes.
- Add a guarded load function and a per-frame render function.
- Reset screen-specific state on entry.
- Route only relevant inputs to the active screen.
- Use elapsed ticks for animations and timed transitions.
- Stop transient audio and video resources on exit.
- Keep table rules separate from presentation code where practical.

## See Also

- [PBEngine API](PBEngine_API.md) - Engine state, timers, and screen coordination
- [PBGfx 2D Graphics API](PBGfx_2D_API.md) - Sprites, text, and animation
- [PBSound API](PBSound_API.md) - Music and sound effects
- [PBVideoPlayer API](PBVideo_API.md) - Video playback
- [PBDevice API](PBDevice_API.md) - Multi-step pinball devices
- [Screen Manager API](Screen_Manager_API.md) - Screen requests and overlays
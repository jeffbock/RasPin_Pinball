# RasPin Architecture and API Guide

## Overview
PInball is a cross-platform pinball simulation and hardware control engine designed for both Windows and Raspberry Pi (PiOS). It provides a framework for building 1/2 scale physical pinball machines, supporting both simulation (on Windows) and real hardware interaction (on PiOS). The engine is written in C++ and uses OpenGL ES for graphics rendering.

---

## Architecture

### 1. Platform Abstraction
- **Windows**: Uses ANGLE for OpenGL ES emulation and Windows-specific input/output handling.
- **Raspberry Pi (PiOS)**: Uses native OpenGL ES 3.1, X11, and direct GPIO/I2C for hardware control.
- Platform-specific code is separated using `#ifdef EXE_MODE_WINDOWS` and `#ifdef EXE_MODE_RASPI`.

### 2. Main Components
- **PBEngine**: The core class managing game state, rendering, input/output, and screen transitions.
- **Rendering**: Abstracted through `PBGfx` and `PBOGLES` for sprite, text, and background rendering.
- **Input/Output**: Handled via message queues (`m_inputQueue`, `m_outputQueue`) and platform-specific input polling.
- **Menus and Screens**: Each screen (boot, menu, settings, test mode, credits, benchmark, play) has dedicated load and render functions.
- **Resource Management**: Sprites, fonts, and textures are loaded and managed per screen.
- **Save/Load**: Game settings and high scores are persisted in a binary file.

### 3. Program Structure
- **Startup**: Platform-specific render initialization, font/sprite loading, and I/O setup.
- **Main Loop**:
  1. Poll input and output (platform-specific).
  2. Process input messages and update game state.
  3. Render the current screen or game state.
  4. Swap graphics buffers.
- **State Machine**: The engine uses a state machine (`m_mainState`) to manage transitions between screens (boot, menu, play, etc.).

---

## File Structure (Key Files)
- `PInball.cpp` / `PInball.h`: Main engine logic, state machine, and entry point.
- `PBGfx.cpp` / `PBGfx.h`: Graphics abstraction and sprite/font management.
- `PBOGLES.cpp` / `PBOGLES.h`: OpenGL ES wrapper functions.
- `PBRaspPiRender.cpp` / `PBWinRender.cpp`: Platform-specific window and rendering setup.
- `PInball_IO.cpp` / `PInball_IO.h`: Input/output definitions and handling.
- `PBDebounce.cpp` / `PBDebounce.h`: Input debouncing for hardware buttons.
- `PInballMenus.h`: Menu definitions and text.
- `resources/`: Textures, fonts, and save files.

---

## API Usage Summary

### 1. Engine Initialization
- **Windows**: `PBInitWinRender(width, height)`
- **PiOS**: `PBInitPiRender(width, height)`
- **Common**: `g_PBEngine.oglInit(...)`, `g_PBEngine.gfxInit()`

### 2. Main Loop
```cpp
while (true) {
    PBProcessInput();
    PBProcessOutput();
    // Process input queue
    if (!g_PBEngine.m_inputQueue.empty()) {
        stInputMessage inputMessage = g_PBEngine.m_inputQueue.front();
        g_PBEngine.m_inputQueue.pop();
        g_PBEngine.pbeUpdateState(inputMessage);
    }
    g_PBEngine.pbeRenderScreen(currentTick, lastTick);
    g_PBEngine.gfxSwap();
    lastTick = currentTick;
}
```

### 3. State Management
- **Change State**: Set `m_mainState` to one of the enum values (e.g., `PB_STARTMENU`, `PB_PLAYGAME`).
- **Screen Loading/Rendering**: Use `pbeLoadScreen(state)` and `pbeRenderScreen(currentTick, lastTick)`.

### 4. Input/Output
- **Input**: Inputs are debounced and pushed to `m_inputQueue` as `stInputMessage`.
- **Output**: Output messages are pushed to `m_outputQueue` as `stOutputMessage`.
- **Processing**: Use `PBProcessInput()` and `PBProcessOutput()` for polling and handling.

### 5. Graphics and Sprites
- **Load Sprite**: `gfxLoadSprite(...)`
- **Render Sprite**: `gfxRenderSprite(spriteId, x, y)`
- **Render Text**: `gfxRenderString(fontSpriteId, text, x, y, ...)`
- **Render Shadowed Text**: `gfxRenderShadowString(...)`
- **Set Color/Scale/Rotation**: `gfxSetColor(...)`, `gfxSetScaleFactor(...)`, `gfxSetRotateDegrees(...)`

### 6. Menus and Screens
- **Generic Menu Rendering**: `pbeRenderGenericMenu(...)` renders a menu with cursor and shadow effects.
- **Screen Functions**: Each screen has `pbeLoad...()` and `pbeRender...()` functions.

### 7. Save/Load
- **Save Settings/High Scores**: `pbeSaveFile()`
- **Load Settings/High Scores**: `pbeLoadSaveFile(loadDefaults, resetScores)`
- **Reset High Scores**: `resetHighScores()`

---

## Extending the Engine
- **Add New Screens**: Implement new `pbeLoad...()` and `pbeRender...()` functions and update the state machine.
- **Add Inputs/Outputs**: Update input/output definitions and handling in `PInball_IO.cpp` and `PBDebounce.cpp`.
- **Add Sprites/Fonts**: Place new assets in `resources/` and load them in the appropriate screen setup functions.

---

## Summary
PInball is a modular, cross-platform pinball engine with a clear separation between platform-specific and common code. It uses a state machine for screen management, message queues for input/output, and OpenGL ES for graphics. The API is designed to be straightforward for extending gameplay, adding new screens, and integrating with real pinball hardware on Raspberry Pi.

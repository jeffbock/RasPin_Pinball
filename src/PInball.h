// PInball:  A complete pinball framework for building 1/2 scale phyical pinball machines with Raspberry Pi
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#ifndef PInball_h
#define PInball_h

// Choose the EXE mode for the code - Windows is used for developent / debug, Rasberry Pi is used for the final build
// Comment out this line if building for Rasberry Pi
#define EXE_MODE_WINDOWS

#ifdef EXE_MODE_WINDOWS
// Windows specific includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "PBWinRender.h"

#else
#define EXE_MODE_RASPI
// Rasberry PI specific includes
// Comment out define below if you want to build / use a PiOS as the simulator instead of windows
// #define RASPI_SIMULATOR
#endif  // Platform include selection

// Includes for all platforms
#include "include_ogl/egl.h"
#include "include_ogl/gl31.h"
#include "PBOGLES.h"
#include "PBGfx.h"
#include <iostream>
#include <stdio.h>
#include <vector>
#include <queue>

// This must be set to whatever actual screen size is being use for Rasbeery Pi
#define PB_SCREENWIDTH 800
#define PB_SCREENHEIGHT 480

bool PBInitRender (long width, long height);

// The main modes / screens for the pinball game.  
enum PBMainState {
    PB_BOOTUP = 0,
    PB_STARTMENU = 1,
    PB_PLAYGAME = 2,
    PB_TESTMODE = 3,
    PB_BENCHMARK = 4,
    PB_CREDITS = 5
};

// Input message structs and types
enum PBInputType {
    PB_INPUT_BUTTON = 0,
    PB_INPUT_BALLSENSOR = 1,
    PB_INPUT_TARGET = 2,
    PB_INPUT_JETBUMPER = 3,
    PB_INPUT_POPBUMPER = 4,
};

enum PBInputState {
    PB_ON = 0,
    PB_OFF = 1,
};

struct stInputMessage {
    PBInputType inputType;
    unsigned int inputId;
    PBInputState inputState;
    unsigned long instanceTick; 
};

// Make new class named PBGame that inheritis from PBOGLES with just essential functions
class PBEngine : public PBGfx {
public:
    PBEngine();
    ~PBEngine();

    // Public funcation for dealing w/ rendering and loading screens by screen state
    bool pbeRenderScreen(unsigned long currentTick, unsigned long lastTick);
    bool pbeLoadScreen (PBMainState state);

    // Console functions
    void pbeSendConsole(std::string output);
    void pbeClearConsole();
    void pbeRenderConsole(unsigned int startingX, unsigned int startingY);

    // Member variables for the sprites used in the screens
    // We might switch this to a query by name mechansim, but that would be slower...
    unsigned int m_defaultFontSpriteId;
    unsigned int m_consoleTextHeight;

    unsigned int m_BootUpConsoleId, m_BootUpStarsId, m_BootUpStarsId2, m_BootUpStarsId3, m_BootUpStarsId4, m_BootUpTitleBarId;
    
private:

    PBMainState m_mainState;
  
    // Load trackers for main screens
    bool m_PBDefaultBackgroundLoaded;
    bool m_PBBootupLoaded, m_PBStartMenuLoaded, m_PBPlayGameLoaded, m_PBTestModeLoaded;
    bool m_PBBenchmarkLoaded, m_PBCreditsLoaded;

    std::vector<std::string> m_consoleQueue;
    unsigned int m_maxConsoleLines;

    std::queue<stInputMessage> m_inputQueue;

    // Private functions for rendering main state screens
    bool pbeRenderDefaultBackground (unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderBootScreen(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderStartMenu(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderPlayGame(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderTestMode(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderBenchmark(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderCredits(unsigned long currentTick, unsigned long lastTick);

    // Private functions for loading main state screens
    bool pbeLoadDefaultBackground();
    bool pbeLoadBootUp();
    bool pbeLoadStartMenu();
    bool pbeLoadPlayGame();
    bool pbeLoadTestMode();
    bool pbeLoadBenchmark();
    bool pbeLoadCredits();

    // Message functions
    void pbeInputLoop();
};

#endif // PInball_h
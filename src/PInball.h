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
#include "PInball_IO.h"
#include <iostream>
#include <stdio.h>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

// This must be set to whatever actual screen size is being use for Rasbeery Pi
#define PB_SCREENWIDTH 800
#define PB_SCREENHEIGHT 480

#define MENUFONT "src/resources/fonts/Baldur_60_512.png"
#define MENUSWORD "src/resources/textures/MenuSword.png"

bool PBInitRender (long width, long height);

// The main modes / screens for the pinball game.  
enum PBMainState {
    PB_BOOTUP = 0,
    PB_STARTMENU = 1,
    PB_PLAYGAME = 2,
    PB_TESTMODE = 3,
    PB_BENCHMARK = 4,
    PB_CREDITS = 5,
    PB_SETTINGS = 6
};

enum PBTestModeState{ 
    PB_TESTINPUT = 0,
    PB_TESTOUTPUT = 1
};

enum PBDifficultyMode{ 
    PB_EASY = 0,
    PB_NORMAL = 1,
    PB_HARD = 2,
    PB_EPIC = 3,
    PB_DMEND
};

#define PB_EASY_TEXT "Easy"
#define PB_NORMAL_TEXT "Normal"
#define PB_HARD_TEXT "Hard"
#define PB_EPIC_TEXT "Epic"

#define MenuTitle "Dungeon  Adventure"
#define Menu1 "Play Pinball"
#define Menu1Fail "Play Pinball (Disabled)"
#define Menu2 "Settings"
#define Menu3 "Test Mode"
#define Menu4 "Benchmark"
#define Menu5 "Console"
#define Menu6 "Credits"

#define MenuSettingsTitle "Settings"
#define MenuSettings1 "Main Volume: "
#define MenuSettings2 "Music Volume: "
#define MenuSettings3 "Balls Per Game: "
#define MenuSettings4 "Difficulty: "
#define MenuSettings5 "Reset High Scores"

#define NUM_SETTINGS 5

struct stInputMessage {
    PBInputType inputType;
    unsigned int inputId;
    PBPinState inputState;
    unsigned long instanceTick; 
};

struct stOutputMessage {
    PBOutputType outputType;
    unsigned int outputId;
    PBPinState outputState;
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

    // Functions to manage the game state
    void pbeUpdateState(stInputMessage inputMessage);

    // Member variables for the sprites used in the screens
    // We might switch this to a query by name mechansim, but that would be slower...
    unsigned int m_defaultFontSpriteId;
    unsigned int m_consoleTextHeight;

    // Boot Screen variables
    unsigned int m_BootUpConsoleId, m_BootUpStarsId, m_BootUpStarsId2, m_BootUpStarsId3, m_BootUpStarsId4, m_BootUpTitleBarId;
    bool m_PassSelfTest, m_RestartBootUp;

    // Start Menu Screen variables
    unsigned int m_StartMenuFontId, m_StartMenuSwordId, m_CurrentMenuItem;
    bool m_RestartMenu;

    // Test Mode Screen variables
    PBTestModeState m_TestMode;
    bool m_LFON, m_RFON, m_LAON, m_RAON,  m_RestartTestMode;
    unsigned int m_CurrentOutputItem;

    // Settings screen / game variables
    unsigned int m_CurrentSettingsItem, m_MainVolume, m_MusicVolume, m_BallsPerGame;
    PBDifficultyMode m_Difficulty;
    bool m_RestartSettings; 

    // Credits screen
    unsigned int m_CreditsScrollY, m_TicksPerPixel, m_StartTick;
    bool m_RestartCredits;

    // Benchmark screen
    unsigned int m_TicksPerScene, m_BenchmarkStartTick, m_CountDownTicks, m_aniId;
    bool m_BenchmarkDone, m_RestartBenchmark;

    // Message queue variables
    std::queue<stInputMessage> m_inputQueue;
    std::mutex m_inputQMutex;
    std::queue<stOutputMessage> m_outputQueue;
    std::mutex m_outputQMutex;
    
private:

    PBMainState m_mainState;
  
    // Load trackers for main screens
    bool m_PBDefaultBackgroundLoaded;
    bool m_PBBootupLoaded, m_PBStartMenuLoaded, m_PBPlayGameLoaded, m_PBTestModeLoaded;
    bool m_PBBenchmarkLoaded, m_PBCreditsLoaded, m_PBSettingsLoaded;

    std::vector<std::string> m_consoleQueue;
    unsigned int m_maxConsoleLines;

    // Private functions for rendering main state screens
    bool pbeRenderDefaultBackground (unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderBootScreen(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderStartMenu(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderPlayGame(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderTestMode(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderBenchmark(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderCredits(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderSettings(unsigned long currentTick, unsigned long lastTick);

    // Private functions for loading main state screens
    bool pbeLoadDefaultBackground();
    bool pbeLoadBootUp();
    bool pbeLoadStartMenu();
    bool pbeLoadPlayGame();
    bool pbeLoadTestMode();
    bool pbeLoadBenchmark();
    bool pbeLoadCredits();
    bool pbeLoadSettings();
};

#endif // PInball_h
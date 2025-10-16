// Pinball_Engine.h:  Header file for the PBEngine class - the main pinball engine

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef Pinball_Engine_h
#define Pinball_Engine_h

// Platform and core includes
#include "PBBuildSwitch.h"
#include "PBOGLES.h"
#include "PBGfx.h"
#include "Pinball_IO.h"
#include "Pinball_Table.h"
#include "PBSound.h"
#include "PBDebounce.h"
#include "PBVideoPlayer.h"

// Standard library includes
#include <iostream>
#include <stdio.h>
#include <vector>
#include <queue>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <string>
#include <array>

// Hardware configuration defines
#define NUM_IO_CHIPS    3
#define NUM_LED_CHIPS   3

// Sequence timing defines - this will need to be updated if more LED chips are added
#define LED0_SEQ_MASK 0x1
#define LED1_SEQ_MASK 0x2
#define LED2_SEQ_MASK 0x4
#define LEDALL_SEQ_MASK 0x7

// Forward declarations for enums and structs
enum PBMainState {
    PB_BOOTUP = 0,
    PB_STARTMENU = 1,
    PB_PLAYGAME = 2,
    PB_TESTMODE = 3,
    PB_BENCHMARK = 4,
    PB_CREDITS = 5,
    PB_SETTINGS = 6,
    PB_DIAGNOSTICS = 7,
    PB_TESTSANDBOX = 8
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

enum PBSequenceLoopMode { 
    PB_NOLOOP = 0,
    PB_LOOP = 1,
    PB_PINGPONG = 2,
    PB_PINGPONGLOOP = 3
};

// Forward declarations for table enums
enum class PBTableState;
enum class PBTBLScreenState;

// Structure forward declarations and type definitions
struct stLEDSequence;

// LED Sequence data structure - compile-time friendly
struct stLEDSequenceData {
    const stLEDSequence* steps;  // Pointer to static array of steps
    int stepCount;               // Number of steps in the array (changed to int for consistency)
};

typedef const stLEDSequenceData LEDSequence;

// Input message structures
struct stInputMessage {
    PBInputMsg inputMsg;
    unsigned int inputId;
    PBPinState inputState;
    unsigned long sentTick; 
};

// Output message structures
struct stOutputOptions {
    unsigned int onBlinkMS;
    unsigned int offBlinkMS;
    unsigned int brightness;
    PBSequenceLoopMode loopMode;
    uint16_t activeLEDMask[NUM_LED_CHIPS];
    const LEDSequence *setLEDSequence;
};

struct stOutputMessage {
    PBOutputMsg outputMsg;
    unsigned int outputId;
    PBPinState outputState;
    bool usePulse;
    unsigned long sentTick;
    stOutputOptions *options;
};

struct stOutputPulse {
    unsigned int outputId;
    unsigned int onTimeMS;
    unsigned int offTimeMS;
    unsigned long startTickMS;
};

struct stLEDSequenceInfo {
    bool sequenceEnabled;
    bool firstTime;
    PBSequenceLoopMode loopMode;              
    unsigned long sequenceStartTick;
    unsigned long stepStartTick;              // Track when current step started
    int currentSeqIndex;                      // Changed to int to handle negative values
    int previousSeqIndex;                     // Changed to int to handle negative values
    int indexStep;
    uint8_t previousLEDValues[NUM_LED_CHIPS][4];
    uint16_t activeLEDMask[NUM_LED_CHIPS];
    LEDSequence *pLEDSequence;
};

struct stLEDSequence {
    uint16_t LEDOnBits [NUM_LED_CHIPS];
    unsigned int onDurationMS;
    unsigned int offDurationMS;
};

// Save file structures
struct stHighScoreData {
    unsigned long highScore;
    std::string playerInitials;
};

#define NUM_HIGHSCORES 5
#define MAINVOLUME_DEFAULT 10
#define MUSICVOLUME_DEFAULT 10
#define BALLSPERGAME_DEFAULT 3
#define DIFFICULTY_DEFAULT PB_NORMAL

struct stSaveFileData {
    unsigned int mainVolume;
    unsigned int musicVolume;
    unsigned int ballsPerGame;
    PBDifficultyMode difficulty;
    std::array<stHighScoreData, NUM_HIGHSCORES> highScores; 
};

// Make new class named PBGame that inheritis from PBOGLES with just essential functions
class PBEngine : public PBGfx {
public:
    PBEngine();
    ~PBEngine();

    // Public funcation for dealing w/ rendering and loading screens by screen state
    bool pbeRenderScreen(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderGameScreen(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderOverlay(unsigned long currentTick, unsigned long lastTick);
    bool pbeLoadGameScreen (PBMainState state);

    // Console functions
    void pbeSendConsole(std::string output);
    void pbeClearConsole();
    void pbeRenderConsole(unsigned int startingX, unsigned int startingY);

    // Functions to manage the game states (main and pinball game)
    void pbeUpdateState(stInputMessage inputMessage);
    void pbeUpdateGameState(stInputMessage inputMessage);
    PBMainState pbeGetMainState() { return m_mainState; }

    // Save File Functions
    bool pbeLoadSaveFile(bool loadDefaults, bool resetScores);
    bool pbeSaveFile();
    void resetHighScores();

    // Setup input / outputs
    bool pbeSetupIO();
    
    // Output message functions
    void SendOutputMsg(PBOutputMsg outputMsg, unsigned int outputId, PBPinState outputState, bool usePulse, stOutputOptions* options = nullptr);
    void SendRGBMsg(unsigned int redId, unsigned int greenId, unsigned int blueId, PBLEDColor color, PBPinState outputState, bool usePulse, stOutputOptions* options = nullptr);
    void SendSeqMsg(const LEDSequence* sequence, const uint16_t* mask, PBSequenceLoopMode loopMode, PBPinState outputState);
    
    // Input configuration functions
    bool SetAutoOutput(unsigned int index, bool autoOutputEnabled);
    
    // Auto output enable/disable functions
    void SetAutoOutputEnable(bool enable) { m_autoOutputEnable = enable; }
    bool GetAutoOutputEnable() const { return m_autoOutputEnable; }
    
    #ifdef EXE_MODE_RASPI
        // This map is used for whatever arbitrary Raspberry Pi inputs are used (from the main board)
        // Note: IO expansion chips are not included in the structure
        std::map<int, cDebounceInput> m_inputPiMap;
    #endif

    // Member variables for LED and IO chips - in Windows they are just faked
    // This would need to changed if there are more or less IO chips in the system
    
    IODriverDebounce m_IOChip[NUM_IO_CHIPS] = {
        IODriverDebounce(PB_ADD_IO0, 0xFF, 1),
        IODriverDebounce(PB_ADD_IO1, 0xFF, 1),
        IODriverDebounce(PB_ADD_IO2, 0xFF, 1)
    };

    LEDDriver m_LEDChip[NUM_LED_CHIPS] = {
        LEDDriver(PB_ADD_LED0),
        LEDDriver(PB_ADD_LED1),
        LEDDriver(PB_ADD_LED2)
    };

    AmpDriver m_ampDriver = AmpDriver(PB_I2C_AMPLIFIER);

    // Sound system for background music and effects
    PBSound m_soundSystem;

    // Member variables for the sprites used in the screens
    // We might switch this to a query by name mechansim, but that would be slower...
    unsigned int m_defaultFontSpriteId;
    unsigned int m_consoleTextHeight;

    // Boot Screen variables
    unsigned int m_BootUpConsoleId, m_BootUpStarsId, m_BootUpStarsId2, m_BootUpStarsId3, m_BootUpStarsId4, m_BootUpTitleBarId;
    bool m_PassSelfTest, m_RestartBootUp;

    // Start Menu Screen variables
    unsigned int m_StartMenuFontId, m_StartMenuSwordId, m_CurrentMenuItem;
    bool m_RestartMenu,  m_GameStarted;

    // Test Mode Screen variables
    PBTestModeState m_TestMode;
    bool m_LFON, m_RFON, m_LAON, m_RAON,  m_RestartTestMode;
    unsigned int m_CurrentOutputItem;

    // Settings screen / game variables
    unsigned int m_CurrentSettingsItem;
    stSaveFileData m_saveFileData;
    bool m_RestartSettings; 

    // Diagnostics Menu
    unsigned int m_CurrentDiagnosticsItem;
    bool m_EnableOverlay;
    bool m_RestartDiagnostics;
    bool m_ShowFPS;
    int m_RenderFPS;

    // Credits screen
    unsigned int m_CreditsScrollY, m_TicksPerPixel, m_StartTick;
    bool m_RestartCredits;

    // Benchmark screen
    unsigned int m_TicksPerScene, m_BenchmarkStartTick, m_CountDownTicks, m_aniId;
    bool m_BenchmarkDone, m_RestartBenchmark;

    // Test Sandbox screen variables
    bool m_RestartTestSandbox;
    PBVideoPlayer* m_sandboxVideoPlayer;
    unsigned int m_sandboxVideoSpriteId;
    bool m_sandboxVideoLoaded;
    unsigned long m_videoFadeStartTick;
    bool m_videoFadingIn;
    bool m_videoFadingOut;
    float m_videoFadeDurationSec;

    // Message queue variables
    std::queue<stInputMessage> m_inputQueue;
    std::mutex m_inputQMutex;
    std::queue<stOutputMessage> m_outputQueue;
    std::mutex m_outputQMutex;
    std::map<unsigned int, stOutputPulse> m_outputPulseMap;
    stLEDSequenceInfo m_LEDSequenceInfo;
    std::queue<stOutputMessage> m_deferredQueue;
    std::mutex m_deferredQMutex;

    // Main Backglass Variables
    unsigned int m_PBTBLBackglassId;

    // Start state
    unsigned int m_PBTBLStartDoorId, m_PBTBLLeftDoorId, m_PBTBLRightDoorId, m_PBTBLFlame1Id, m_PBTBLFlame2Id, m_PBTBLFlame3Id; 
    unsigned int m_PBTBLDoorDragonId, m_PBTBLDragonEyesId, m_PBTBLDoorDungeonId;
    unsigned int m_PBTBLFlame1StartId, m_PBTBLFlame2StartId, m_PBTBLFlame3StartId, m_PBTBLeftDoorStartId, m_PBTBRightDoorStartId;
    unsigned int m_PBTBLFlame1EndId, m_PBTBLFlame2EndId, m_PBTBLFlame3EndId, m_PBTBLeftDoorEndId, m_PBTBRightDoorEndId;
    unsigned int m_PBTBLTextStartId, m_PBTBLTextEndId;
    bool m_PBTBLStartDoorsDone, m_PBTBLOpenDoors;

    bool m_RestartTable;
private:

    PBMainState m_mainState;

    // Main Table Variables, etc..
    PBTableState m_tableState; 
    PBTBLScreenState m_tableScreenState;

    // Console variables
    std::vector<std::string> m_consoleQueue;
    unsigned int m_maxConsoleLines;

    // Main table Variables, etc..
    bool m_PBTBLStartLoaded; 
    
    // Auto output control
    bool m_autoOutputEnable; 

    // Private functions for rendering main state screens
    bool pbeRenderDefaultBackground (unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderBootScreen(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderStartMenu(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderTestMode(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderBenchmark(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderCredits(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderSettings(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderDiagnostics(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderTestSandbox(unsigned long currentTick, unsigned long lastTick);

    // Generic menu rendering function
    bool pbeRenderGenericMenu(unsigned int cursorSprite, unsigned int fontSprite, unsigned int selectedItem, 
                              int x, int y, int lineSpacing, std::map<unsigned int, std::string>* menuItems,
                              bool useShadow, bool useCursor, unsigned int redShadow, 
                              unsigned int greenShadow, unsigned int blueShadow, unsigned int alphaShadow, unsigned int shadowOffset);

    // Private functions for loading main state screens
    bool pbeLoadDefaultBackground(bool forceReload);
    bool pbeLoadBootUp(bool forceReload);
    bool pbeLoadStartMenu(bool forceReload);
    bool pbeLoadTestMode(bool forceReload);
    bool pbeLoadBenchmark(bool forceReload);
    bool pbeLoadCredits(bool forceReload);
    bool pbeLoadSettings(bool forceReload);
    bool pbeLoadDiagnostics(bool forceReload);
    bool pbeLoadTestSandbox(bool forceReload);

    ///////////////////////////////
    // Specfic Game Table Functions
    ///////////////////////////////
    
    // Render functions for the pinball game table
    bool pbeRenderGameStart(unsigned long currentTick, unsigned long lastTick);

    // Load functions for the pinball game table
    bool pbeLoadGameStart(bool forceReload); // Load the start screen for the pinball game

    // Texture release functions
    void pbeReleaseMenuTextures();

    // Main Table functions - these will need to be modified per table
};

// Global variable declaration
extern PBEngine g_PBEngine;

#endif // Pinball_Engine_h

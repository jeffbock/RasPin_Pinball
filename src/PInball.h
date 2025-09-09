// PInball:  A complete pinball framework for building 1/2 scale phyical pinball machines with Raspberry Pi

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef PInball_h
#define PInball_h

// Choose the EXE mode for the code - Windows is used for developent / debug, Rasberry Pi is used for the final build
// Change the PBBuildSwitch.h to change the build mode
#include "PBBuildSwitch.h"

#ifdef EXE_MODE_WINDOWS
// Windows specific includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "PBWinRender.h"
#endif

#ifdef EXE_MODE_RASPI
// Rasberry PI specific includes
#include "wiringPi.h"
#include "wiringPiI2C.h"
#include "PBDebounce.h"

#endif  // Platform include selection

// Includes for all platforms
#include <egl.h>
#include <gl31.h>
//#include "include_ogl/egl.h"
//#include "include_ogl/gl31.h"
#include "PBOGLES.h"
#include "PBGfx.h"
#include "PInball_IO.h"
#include "PInball_Table.h"
#include "PBSound.h"
#include <iostream>
#include <stdio.h>
#include <vector>
#include <queue>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "PBDebounce.h"

// Hardware configuration defines - moved here to be available for structure definitions
#define NUM_IO_CHIPS    3
#define NUM_LED_CHIPS   3

// For the Deferred LED message queue
#define MAX_DEFERRED_LED_QUEUE 100

// Forward declarations and early type definitions
struct stLEDSequence; // Forward declaration
typedef std::vector<std::vector<stLEDSequence>> LEDSequence;

enum PBSequenceLoopMode { 
    PB_NOLOOP = 0,
    PB_LOOP = 1,
    PB_PINGPONG = 2,
    PB_PINGPONGLOOP = 3
};

// Forward declarations for the enums used in the PBEngine class
enum class PBTableState;
enum class PBTBLScreenState;

// This must be set to whatever actual screen size is being use for Rasbeery Pi
#define PB_SCREENWIDTH 1920
#define PB_SCREENHEIGHT 1080

#define MENUFONT "src/resources/fonts/Baldur_96_768.png"
#define MENUSWORD "src/resources/textures/MenuSword.png"
#define SAVEFILENAME "src/resources/savefile.bin"

// Sound files
#define MUSICFANTASY "src/resources/sound/fantasymusic.mp3"
#define EFFECTSWORDHIT "src/resources/sound/swordcut.mp3"
#define EFFECTCLICK "src/resources/sound/click.mp3"

bool PBInitRender (long width, long height);

// The main modes / screens for the pinball menus.  
enum PBMainState {
    PB_BOOTUP = 0,
    PB_STARTMENU = 1,
    PB_PLAYGAME = 2,
    PB_TESTMODE = 3,
    PB_BENCHMARK = 4,
    PB_CREDITS = 5,
    PB_SETTINGS = 6,
    PB_DIAGNOSTICS = 7
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

// Input message structures

struct stInputMessage {
    PBInputMsg inputMsg;
    unsigned int inputId;
    PBPinState inputState;
    unsigned long sentTick; 
};

// Output message structures

struct stOutputOptions {
    unsigned int onTimeMS;
    unsigned int offTimeMS;
    unsigned int brightness;
    unsigned int sequenceMask; // Bit mask of which LED chips are in the sequence
    PBSequenceLoopMode loopMode;
    const LEDSequence *setLEDSequence;
};

struct stOutputMessage {
    PBOutputMsg outputMsg;
    unsigned int outputId;
    PBPinState outputState;
    bool usePulse;
    unsigned long sentTick;
    stOutputOptions *options; // Pointer to options structure if needed
};

// Structures used during the processing of output messages

struct stOutputPulse {
    unsigned int outputId;
    unsigned int onTimeMS;
    unsigned int offTimeMS;
    unsigned long startTickMS;
};

struct stLEDSequenceInfo {
    bool sequenceEnabled;
    bool firstTime; // True if first time through the sequence
    PBSequenceLoopMode loopMode;              
    unsigned int sequenceChipMask; // Bit mask of which LED chips are in the sequence
    unsigned long sequenceStartTick;
    unsigned int currentSeqIndex;    // Current index in the sequence
    int indexStep;        // +1 or -1 depending on direction
    uint8_t savedLEDValues[NUM_LED_CHIPS]; // Saved LED values for each chip in the sequence
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

// Sequence timing defines - this will need to be updated if more LED chips are added
#define LED0_SEQ_MASK 0x1
#define LED1_SEQ_MASK 0x2
#define LED2_SEQ_MASK 0x4
#define LEDALL_SEQ_MASK 0x7

// Make new class named PBGame that inheritis from PBOGLES with just essential functions
class PBEngine : public PBGfx {
public:
    PBEngine();
    ~PBEngine();

    // Public funcation for dealing w/ rendering and loading screens by screen state
    bool pbeRenderScreen(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderGameScreen(unsigned long currentTick, unsigned long lastTick);
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

// Output processing function declarations
int FindOutputDefIndex(unsigned int outputId);
void ProcessLEDSequenceMessage(const stOutputMessage& message);
void ProcessIOOutputMessage(const stOutputMessage& message, stOutputDef& outputDef);
void ProcessLEDOutputMessage(const stOutputMessage& message, stOutputDef& outputDef, bool skipSequenceCheck = false);
void ProcessLEDConfigMessage(const stOutputMessage& message, stOutputDef& outputDef);
void ProcessActivePulseOutputs();
void ProcessActiveLEDSequence();
void HandleLEDSequenceBoundaries();
void EndLEDSequence();
void ProcessDeferredLEDQueue();

#endif // PInball_h
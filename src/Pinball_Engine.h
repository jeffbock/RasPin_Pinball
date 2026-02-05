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

// Forward declarations
class PBDevice;
class pbdEjector;

// Standard library includes
#include <iostream>
#include <stdio.h>
#include <vector>
#include <queue>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <string>
#include <array>

// Hardware configuration defines
#define NUM_IO_CHIPS    3
#define NUM_LED_CHIPS   3

// NeoPixel configuration - LED count for each driver index
// Index corresponds to boardIndex in g_outputDef. Set to 0 for unused indices.
constexpr unsigned int g_NeoPixelSize[] = {35,1};  // Driver 0: 35 LEDs, Driver 1: 1 LED

// Calculate SPI buffer sizes: each LED is 3 bytes (GRB), each byte needs 4 SPI bytes
// Total: numLEDs * 3 * 4 bytes
constexpr unsigned int g_NeoPixelSPIBufferSize[] = {
    g_NeoPixelSize[0] * 3 * 4,  // Driver 0 SPI buffer size
    g_NeoPixelSize[1] * 3 * 4   // Driver 1 SPI buffer size
};

// Create a an array of NeoPixelNode arrays for each NeoPixel driver
// These all are pre-allocated during boot time to avoid dynamic memory allocation during runtime
// If you add more drivers, you will need to update this section - the BoardID of the NeoPixel in g_outputDef defines the index
extern stNeoPixelNode* g_NeoPixelNodeArray[2];
extern stNeoPixelNode g_NeoPixelNodes0[g_NeoPixelSize[0]];  // Driver 0 nodes
extern stNeoPixelNode g_NeoPixelNodes1[g_NeoPixelSize[1]];  // Driver 1 nodes

// Pre-allocated SPI buffers for each NeoPixel driver (can be null if not using SPI modes)
extern unsigned char* g_NeoPixelSPIBufferArray[2];
extern unsigned char g_NeoPixelSPIBuffer0[g_NeoPixelSPIBufferSize[0]];  // Driver 0 SPI buffer
extern unsigned char g_NeoPixelSPIBuffer1[g_NeoPixelSPIBufferSize[1]];  // Driver 1 SPI buffer

// Initialize NeoPixel array pointers - call during early initialization
inline void initNeoPixelArrays() {
    g_NeoPixelNodeArray[0] = g_NeoPixelNodes0;
    g_NeoPixelNodeArray[1] = g_NeoPixelNodes1;
    g_NeoPixelSPIBufferArray[0] = g_NeoPixelSPIBuffer0;
    g_NeoPixelSPIBufferArray[1] = g_NeoPixelSPIBuffer1;
}

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
    PB_TESTSANDBOX = 8,
    PB_SIMPLEFLIPMODE = 9
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
struct stNeoPixelSequence;

// LED Sequence data structure - compile-time friendly
struct stLEDSequenceData {
    const stLEDSequence* steps;  // Pointer to static array of steps
    int stepCount;               // Number of steps in the array (changed to int for consistency)
};

typedef const stLEDSequenceData LEDSequence;

// NeoPixel Sequence data structure - compile-time friendly
struct stNeoPixelSequenceData {
    const stNeoPixelSequence* steps;  // Pointer to static array of steps
    int stepCount;                     // Number of steps in the array
};

typedef const stNeoPixelSequenceData NeoPixelSequence;

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
    const NeoPixelSequence *setNeoPixelSequence;  // For NeoPixel sequences
    uint8_t neoPixelRed;                          // Red channel for single NeoPixel LED (0-255)
    uint8_t neoPixelGreen;                        // Green channel for single NeoPixel LED (0-255)
    uint8_t neoPixelBlue;                         // Blue channel for single NeoPixel LED (0-255)
    unsigned int neoPixelIndex;                   // Index of specific pixel in chain (0=all, 1+=specific pixel)
};

struct stOutputMessage {
    PBOutputMsg outputMsg;
    unsigned int outputId;
    PBPinState outputState;
    bool usePulse;
    unsigned long sentTick;
    stOutputOptions *options;  // Pointer for backward compatibility, but will be copied
    stOutputOptions optionsCopy;  // Actual storage for options data
    bool hasOptions;  // Flag to indicate if options are valid
};

struct stOutputPulse {
    unsigned int outputId;
    unsigned int onTimeMS;
    unsigned int offTimeMS;
    unsigned long startTickMS;
};

// Timer structure for pbeSetTimer functionality
struct stTimerEntry {
    unsigned int timerId;          // User-supplied timer ID
    unsigned int durationMS;       // Timer duration in milliseconds
    unsigned long startTickMS;     // Time when timer was started
    unsigned long expireTickMS;    // Time when timer should expire
    bool repeat;                   // If true, timer automatically restarts after expiring
};

// Reserved timer ID for the watchdog timer
#define WATCHDOGTIMER_ID 0

// Timer ID for sandbox NeoPixel animation
#define SANDBOX_NEOPIXEL_TIMER_ID 100
#define SANDBOX_NEOPIXEL_TIMER_INTERVAL_MS 100

// Maximum number of active timers allowed
#define MAX_TIMERS 10

// Console rendering start Y position (below title bar)
#define CONSOLE_START_Y 42

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

// NeoPixel sequence step structure
struct stNeoPixelSequence {
    const stNeoPixelNode* nodeArray;  // Pointer to array of RGBB values for this step
    unsigned int onDurationMS;         // Duration to display this step
    uint8_t brightness;                // Brightness for this step (0-255, default 255)
};

// NeoPixel sequence info structure
struct stNeoPixelSequenceInfo {
    bool sequenceEnabled;
    bool firstTime;
    PBSequenceLoopMode loopMode;
    unsigned long sequenceStartTick;
    unsigned long stepStartTick;
    int currentSeqIndex;
    int previousSeqIndex;
    int indexStep;
    NeoPixelSequence *pNeoPixelSequence;
    unsigned int driverIndex;  // Which NeoPixel driver this sequence is for
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
    bool isWindows;  // Track which OS created this file
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
    void pbeRenderConsole(unsigned int startingX, unsigned int startingY, unsigned int startLine);

    // Functions to manage the game states (main and pinball game)
    void pbeUpdateState(stInputMessage inputMessage);
    void pbeUpdateGameState(stInputMessage inputMessage);
    void pbeForceUpdateState();
    PBMainState pbeGetMainState() { return m_mainState; }

    // ========================================================================
    // MODE SYSTEM FUNCTIONS
    // ========================================================================
    
    // Mode management functions
    void pbeUpdateModeState(unsigned long currentTick);
    bool pbeCheckModeTransition(unsigned long currentTick);
    void pbeEnterMode(PBTableMode newMode, unsigned long currentTick);
    void pbeExitMode(PBTableMode exitingMode, unsigned long currentTick);
    
    // Helper function to update mode system (reduces code duplication)
    void pbeUpdateModeSystem(stInputMessage inputMessage, unsigned long currentTick);
    
    // Mode-specific state update functions
    void pbeUpdateNormalPlayMode(stInputMessage inputMessage, unsigned long currentTick);
    void pbeUpdateMultiballMode(stInputMessage inputMessage, unsigned long currentTick);
    
    // Mode qualification check functions
    bool pbeCheckMultiballQualified();
    
    // Screen manager functions
    void pbeRequestScreen(int screenId, ScreenPriority priority, unsigned long durationMs, bool canBePreempted);
    void pbeUpdateScreenManager(unsigned long currentTick);
    void pbeClearScreenRequests();
    int pbeGetCurrentScreen();

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
    void SendNeoPixelAllMsg(unsigned int neoPixelId, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness = 255);
    void SendNeoPixelAllMsg(unsigned int neoPixelId, PBLEDColor color, uint8_t brightness = 255);
    void SendNeoPixelSingleMsg(unsigned int neoPixelId, unsigned int pixelIndex, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness = 255);
    void SendNeoPixelSingleMsg(unsigned int neoPixelId, unsigned int pixelIndex, PBLEDColor color, uint8_t brightness = 255);
    
    // Input configuration functions
    bool SetAutoOutput(unsigned int index, bool autoOutputEnabled);
    
    // Auto output enable/disable functions
    void SetAutoOutputEnable(bool enable) { m_autoOutputEnable = enable; }
    bool GetAutoOutputEnable() const { return m_autoOutputEnable; }
    
    // Device management functions
    void pbeAddDevice(PBDevice* device);
    void pbeClearDevices();
    void pbeExecuteDevices();
    
    // Timer functions
    bool pbeSetTimer(unsigned int timerId, unsigned int timerValueMS, bool repeat = false);
    bool pbeSetWatchdogTimer(unsigned int timerValueMS);
    void pbeProcessTimers();
    bool pbeTimerActive(unsigned int timerId);
    void pbeTimerStop(unsigned int timerId);
    void pbeTimerStopAll(bool stopWatchdog = false);
    
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

    // NeoPixel driver map - drivers are created dynamically during initialization
    // Key: boardIndex from g_outputDef
    std::map<int, NeoPixelDriver> m_NeoPixelDriverMap;

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
    pbdEjector* m_sandboxEjector;
    
    // NeoPixel animation variables for sandbox test
    bool m_sandboxNeoPixelAnimActive;    // Animation is running
    bool m_sandboxNeoPixelStepMode;      // True = step mode (manual), False = timer mode (automatic)
    int m_sandboxNeoPixelPosition;       // Current position of the 3-pixel group (1-based)
    bool m_sandboxNeoPixelMovingUp;      // True = moving up, False = moving down
    int m_sandboxNeoPixelMaxPosition;    // Maximum position based on LED count
    int m_sandboxNeoPixelAnimMode;       // 0=original step, 1=gradual fade, 2=sweep from ends

    // Simple Flip Mode screen variables
    bool m_RestartSimpleFlipMode;
    bool m_OverlayWasOnBeforeSimpleFlip;  // Track if overlay was already on when entering

    // Message queue variables
    std::queue<stInputMessage> m_inputQueue;
    std::mutex m_inputQMutex;
    std::queue<stOutputMessage> m_outputQueue;
    std::mutex m_outputQMutex;
    std::map<unsigned int, stOutputPulse> m_outputPulseMap;
    stLEDSequenceInfo m_LEDSequenceInfo;
    std::map<int, stNeoPixelSequenceInfo> m_NeoPixelSequenceMap;  // Key: boardIndex
    std::queue<stOutputMessage> m_deferredQueue;
    std::mutex m_deferredQMutex;
    std::queue<stTimerEntry> m_timerQueue;
    std::mutex m_timerQMutex;
    stTimerEntry m_watchdogTimer;  // Dedicated watchdog timer (timerId = 0)

    // Main Backglass Variables
    unsigned int m_PBTBLBackglassId;
    
    // Main screen variables
    unsigned int m_PBTBLMainScreenBGId;
    unsigned int m_PBTBLCharacterCircle256Id, m_PBTBLDungeon256Id, m_PBTBLShield256Id, m_PBTBLSword256Id, m_PBTBLTreasure256Id;
    unsigned int m_PBTBLArcherHeadshot256Id, m_PBTBLKnightHeadshot256Id, m_PBTBLWolfHeadshot256Id;

    // Start state
    unsigned int m_PBTBLStartDoorId, m_PBTBLLeftDoorId, m_PBTBLRightDoorId, m_PBTBLFlame1Id, m_PBTBLFlame2Id, m_PBTBLFlame3Id; 
    unsigned int m_PBTBLDoorDragonId, m_PBTBLDragonEyesId, m_PBTBLDoorDungeonId;
    unsigned int m_PBTBLFlame1StartId, m_PBTBLFlame2StartId, m_PBTBLFlame3StartId, m_PBTBLeftDoorStartId, m_PBTBRightDoorStartId;
    unsigned int m_PBTBLFlame1EndId, m_PBTBLFlame2EndId, m_PBTBLFlame3EndId, m_PBTBLeftDoorEndId, m_PBTBRightDoorEndId;
    unsigned int m_PBTBLTextStartId, m_PBTBLTextEndId;
    bool m_PBTBLStartDoorsDone, m_PBTBLOpenDoors;

    bool m_RestartTable;
    
    // Reset state tracking
    bool m_ResetButtonPressed;         // Track if reset was pressed once
    PBTableState m_StateBeforeReset;   // State to return to if reset is cancelled
    unsigned int m_PBTBLResetSpriteId; // Sprite ID for reset screen background

    // Multi-player game state
    pbGameState m_playerStates[4];    // Array of 4 player states
    unsigned int m_currentPlayer;     // Current active player (0-3)
    
    // Main score animation tracking
    unsigned long m_mainScoreAnimStartTick;
    bool m_mainScoreAnimActive;
    
    // Secondary score slot animations (up to 3 slots for non-current players)
    SecondaryScoreAnimState m_secondaryScoreAnims[3];
    
    // Status text fade animation tracking
    bool m_statustextFadeIn;
    unsigned long m_statusTextFadeStart;
    unsigned long m_statusTextDisplayStart;
    std::string m_statusText[2];
    int m_currentActiveText;
    int m_previousActiveText;
private:

    PBMainState m_mainState;

    // Main Table Variables, etc..
    PBTableState m_tableState; 
    PBTBLScreenState m_tableScreenState;

    // Console variables
    std::vector<std::string> m_consoleQueue;
    unsigned int m_maxConsoleLines;
    unsigned int m_consoleStartLine;  // Current start line for console rendering in BOOTUP state
    
    // Helper function to calculate console rendering parameters
    unsigned int pbeGetMaxConsoleLines(unsigned int startingY);

    // Helper function to convert PBLEDColor enum to RGB values
    void ConvertColorToRGB(PBLEDColor color, uint8_t& red, uint8_t& green, uint8_t& blue);

    // Main table Variables, etc..
    bool m_PBTBLStartLoaded; 
    
    // Auto output control
    bool m_autoOutputEnable;
    
    // Load state tracking for Engine screens
    bool m_defaultBackgroundLoaded;
    bool m_bootUpLoaded;
    bool m_startMenuLoaded;
    
    // Load state tracking for Table screens
    bool m_initScreenLoaded;
    bool m_gameStartLoaded;
    bool m_mainScreenLoaded;
    bool m_resetLoaded; 

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
    bool pbeRenderSimpleFlipMode(unsigned long currentTick, unsigned long lastTick);

    // Generic menu rendering function
    bool pbeRenderGenericMenu(unsigned int cursorSprite, unsigned int fontSprite, unsigned int selectedItem, 
                              int x, int y, int lineSpacing, std::map<unsigned int, std::string>* menuItems,
                              bool useShadow, bool useCursor, unsigned int redShadow, 
                              unsigned int greenShadow, unsigned int blueShadow, unsigned int alphaShadow, unsigned int shadowOffset,
                              unsigned int disabledItemsMask = 0x0);

    // Private functions for loading main state screens
    bool pbeLoadDefaultBackground();
    bool pbeLoadBootUp();
    bool pbeLoadStartMenu();
    bool pbeLoadTestMode();
    bool pbeLoadBenchmark();
    bool pbeLoadCredits();
    bool pbeLoadSettings();
    bool pbeLoadDiagnostics();
    bool pbeLoadTestSandbox();
    bool pbeLoadSimpleFlipMode();
    
    // Reload functions to reset load state
    void pbeEngineReload();  // Reset all engine screen load states
    void pbeTableReload();   // Reset all table screen load states

    ///////////////////////////////
    // Specfic Game Table Functions
    ///////////////////////////////
    
    // Render functions for the pinball game table
    bool pbeRenderInitScreen(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderGameStart(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderMainScreen(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderStatus(unsigned long currentTick, unsigned long lastTick);
    bool pbeRenderReset(unsigned long currentTick, unsigned long lastTick);

    // Load functions for the pinball game table
    bool pbeLoadInitScreen(); // Load the init screen for the pinball game
    bool pbeLoadGameStart(); // Load the start screen for the pinball game
    bool pbeLoadMainScreen(); // Load the main screen for the pinball game
    bool pbeLoadReset(); // Load the reset screen
    
    // Table initialization
    bool pbeTableInit(); // Initialize table devices and state

    // Player management functions
    bool pbeTryAddPlayer(); // Try to add a new player, returns true if successful
    unsigned long getCurrentPlayerScore(); // Get current player's score
    bool isCurrentPlayerEnabled(); // Get current player's enabled state
    PBTableState& getPlayerGameState(); // Get current player's game state
    PBTBLMainScreenState& getPlayerScreenState(); // Get current player's screen state
    void addPlayerScore(unsigned long points); // Add score to current player
    
    // Helper functions
    std::string formatScoreWithCommas(unsigned long score); // Format score with thousand separators
    void pbeRenderPlayerScores(unsigned long currentTick, unsigned long lastTick); // Render all player scores
    void pbeRenderStatusText(unsigned long currentTick, unsigned long lastTick); // Render status text with fade effects
    void pbeSetStatusText(int index, const std::string& text); // Set status text by index (0 or 1)
    
    // Sandbox NeoPixel helper functions
    void sandboxNeoPixelStep(); // Advance NeoPixel animation by one step
    
    // NeoPixel step functions for color transitions
    void neoPixelGradualFade(uint8_t startR, uint8_t startG, uint8_t startB,
                             uint8_t endR, uint8_t endG, uint8_t endB,
                             unsigned int neoPixelId, unsigned int stepTimeMS,
                             unsigned int totalDurationMS, bool restart, bool loop = false);
    
    void neoPixelSweepFromEnds(uint8_t startR, uint8_t startG, uint8_t startB,
                               uint8_t endR, uint8_t endG, uint8_t endB,
                               unsigned int neoPixelId, unsigned int stepTimeMS,
                               unsigned int totalDurationMS, bool restart, bool loop = false);
    
    void neoPixelToggle(uint8_t color1R, uint8_t color1G, uint8_t color1B,
                        uint8_t color2R, uint8_t color2G, uint8_t color2B,
                        uint8_t color3R, uint8_t color3G, uint8_t color3B,
                        uint8_t color4R, uint8_t color4G, uint8_t color4B,
                        uint8_t colorMask,
                        unsigned int neoPixelId, unsigned int stepTimeMS,
                        unsigned int totalDurationMS, bool loop = false);
    
    void neoPixelSplitToggle(uint8_t startR, uint8_t startG, uint8_t startB,
                             uint8_t endR, uint8_t endG, uint8_t endB,
                             unsigned int neoPixelId, unsigned int stepTimeMS,
                             unsigned int totalDurationMS, bool loop = false);
    
    void neoPixelStrobe(uint8_t colorR, uint8_t colorG, uint8_t colorB,
                        unsigned int neoPixelId, unsigned int timeOffMS,
                        unsigned int timeOverallMS = 0, bool loop = true,
                        bool resetState = false);
    
    void neoPixelSnake(uint8_t baseR, uint8_t baseG, uint8_t baseB,
                       uint8_t snakeR, uint8_t snakeG, uint8_t snakeB,
                       unsigned int snakeLength, bool wrapMode,
                       unsigned int neoPixelId, unsigned int stepTimeMS);

    // Texture release functions
    void pbeReleaseMenuTextures();

    // Main Table functions - these will need to be modified per table
    
    // Device management - vector of all registered devices
    std::vector<PBDevice*> m_devices;
    
    // ========================================================================
    // MODE SYSTEM PRIVATE MEMBERS
    // ========================================================================
    
    // Centralized screen manager
    std::vector<ScreenRequest> m_screenQueue;
    int m_currentDisplayedScreen;
    unsigned long m_currentScreenStartTick;
};

// Global variable declaration
extern PBEngine g_PBEngine;

#endif // Pinball_Engine_h

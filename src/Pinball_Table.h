// Pinball_Table.h:  Header file for the specific functions needed for the pinball table.  
//                   This includes further PBEngine Class functions and any support functions unique to the table
//                   These need to be defined by the user for the specific table

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef Pinball_Table_h
#define Pinball_Table_h

#include "Pinball.h"

enum class PBTableState {
    PBTBL_INIT = 0,
    PBTBL_START = 1,
    PBTBL_MAINSCREEN = 2,
    PBTBL_STDPLAY = 3,
    PBTBL_RESET = 4,
    PBTBL_END
};

enum class PBTBLScreenState {
    START_START = 0,
    START_INST = 1,
    START_SCORES = 2,
    START_OPENDOOR = 3,
    START_END = 4,
    MAIN_NORMAL = 5,        // Normal score and message display (formerly MAIN_SHOWSCORE)
    MAIN_EXTRABALL = 6,     // Extra ball award screen with video
    MAIN_END = 7
};

// ========================================================================
// MODE SYSTEM - Multiple pinball game modes with independent state flows
// ========================================================================

// Mode types - represents different game modes that can be active
enum class PBTableMode {
    MODE_NORMAL_PLAY = 0,     // Normal play mode - main gameplay
    MODE_MULTIBALL = 1,       // Multiball mode - triggered by specific conditions
    MODE_END
};

// Normal play mode sub-states
enum class PBNormalPlayState {
    NORMAL_IDLE = 0,          // Waiting for ball to be active
    NORMAL_ACTIVE = 1,        // Ball in play
    NORMAL_DRAIN = 2,         // Ball drained, handling end of ball
    NORMAL_END
};

// Multiball mode sub-states
enum class PBMultiballState {
    MULTIBALL_START = 0,      // Starting multiball sequence
    MULTIBALL_ACTIVE = 1,     // Multiball gameplay active
    MULTIBALL_ENDING = 2,     // Transitioning back to normal play
    MULTIBALL_END
};

// Screen request priority levels
enum class ScreenPriority {
    PRIORITY_LOW = 0,         // Normal gameplay screens
    PRIORITY_MEDIUM = 1,      // Mode transitions, bonus screens
    PRIORITY_HIGH = 2,        // Important events, jackpots
    PRIORITY_CRITICAL = 3     // Cannot be preempted (e.g., game over)
};

// Screen request structure for centralized screen management
struct ScreenRequest {
    int screenId;                    // ID of screen to display
    ScreenPriority priority;         // Priority level
    unsigned long durationMs;        // How long to display (0 = until cleared)
    unsigned long requestTick;       // When request was made
    bool canBePreempted;             // Can higher priority preempt this?
    
    ScreenRequest() {
        screenId = -1;
        priority = ScreenPriority::PRIORITY_LOW;
        durationMs = 0;
        requestTick = 0;
        canBePreempted = true;
    }
};

// Mode state structure - tracks state for each mode
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
    
    ModeState() {
        currentMode = PBTableMode::MODE_NORMAL_PLAY;
        previousMode = PBTableMode::MODE_NORMAL_PLAY;
        normalPlayState = PBNormalPlayState::NORMAL_IDLE;
        normalPlayStateStartTick = 0;
        multiballState = PBMultiballState::MULTIBALL_START;
        multiballStateStartTick = 0;
        multiballCount = 1;
        multiballQualified = false;
        modeTransitionActive = false;
        modeTransitionStartTick = 0;
    }
    
    void reset() {
        currentMode = PBTableMode::MODE_NORMAL_PLAY;
        previousMode = PBTableMode::MODE_NORMAL_PLAY;
        normalPlayState = PBNormalPlayState::NORMAL_IDLE;
        normalPlayStateStartTick = 0;
        multiballState = PBMultiballState::MULTIBALL_START;
        multiballStateStartTick = 0;
        multiballCount = 1;
        multiballQualified = false;
        modeTransitionActive = false;
        modeTransitionStartTick = 0;
    }
};

// Secondary score slot animation state
struct SecondaryScoreAnimState {
    unsigned long animStartTick;      // Tick when animation started
    float animDurationSec;            // Duration of animation in seconds
    int currentYOffset;               // Current Y offset for scroll animation
    bool animationActive;             // Whether animation is currently active
    int playerIndex;                  // Which player this slot is animating (-1 if none)
    
    SecondaryScoreAnimState() {
        animStartTick = 0;
        animDurationSec = 1.0f;
        currentYOffset = 0;
        animationActive = false;
        playerIndex = -1;
    }
    
    void reset() {
        animStartTick = 0;
        currentYOffset = 0;
        animationActive = false;
        playerIndex = -1;
    }
};

// Per-player game state class
class pbGameState {
public:
    PBTableState mainGameState;       // Main game state for this player
    PBTBLScreenState screenState;     // Screen state for this player (merged main screen states)
    unsigned long score;              // Actual Current score
    unsigned long inProgressScore;    // Score accumulated during current ball
    unsigned long previousScore;      // Previous score to detect changes during animation
    unsigned long scoreUpdateStartTick; // Tick when score update animation started
    bool enabled;                     // Is this player slot enabled/active
    unsigned int currentBall;         // Current ball number (1-based)
    bool ballSaveEnabled;             // Ball save active flag
    bool extraBallEnabled;            // Extra ball earned flag
    
    // Mode system state - tracks current game mode and its sub-states
    ModeState modeState;
    
    // Character states
    bool knightJoined;                // Knight character joined
    bool priestJoined;                // Priest character joined
    bool rangerJoined;                // Ranger character joined
    
    // Character and resource values
    int knightLevel;                  // Knight character level
    int priestLevel;                  // Priest character level
    int rangerLevel;                  // Ranger character level
    int goldValue;                    // Gold/treasure amount
    int attackValue;                  // Attack/sword value
    int defenseValue;                 // Defense/shield value
    int dungeonFloor;                 // Current dungeon floor
    int dungeonLevel;                 // Dungeon difficulty level

    // Constructor
    pbGameState() {
        reset(3); // Default to 3 balls
    }

    // Reset to initial state based on game settings
    void reset(unsigned int ballsPerGame) {
        mainGameState = PBTableState::PBTBL_MAINSCREEN;
        screenState = PBTBLScreenState::MAIN_NORMAL;
        score = 0;
        inProgressScore = 0;
        previousScore = 0;
        scoreUpdateStartTick = 0;
        // Note: enabled flag is NOT reset here - must be set explicitly
        currentBall = 1;
        ballSaveEnabled = false;
        extraBallEnabled = false;
        
        // Reset mode state
        modeState.reset();
        
        // Reset character states
        knightJoined = false;
        priestJoined = false;
        rangerJoined = false;
        
        // Reset character and resource values
        knightLevel = 1;
        priestLevel = 1;
        rangerLevel = 1;
        goldValue = 0;
        attackValue = 0;
        defenseValue = 0;
        dungeonFloor = 1;
        dungeonLevel = 1;
    }
};

#define ACTIVEDISPX 448
#define ACTIVEDISPY 268
#define UPDATESCOREMS 1000

#endif // Pinball_Table_h
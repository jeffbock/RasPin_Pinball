// Pinball_Table.h:  Header file for the specific functions needed for the pinball table.  
//                   This includes further PBEngine Class functions and any support functions unique to the table
//                   These need to be defined by the user for the specific table
//
// Table Mode Architecture:
//   Each PBTableState has its own implementation in src/tablemodes/Pinball_Table_ModeX.h/.cpp
//   Each mode provides three functions: Load, Render, and UpdateState
//   To add a new mode:
//     1. Add a state to PBTableState enum below
//     2. Create Pinball_Table_ModeX.h with any mode-specific enums
//     3. Create Pinball_Table_ModeX.cpp with load, render, and update functions
//     4. Include the new header below
//     5. Add declarations for the three functions in Pinball_Engine.h
//     6. Add dispatch cases in pbeRenderGameScreen() and pbeUpdateGameState()
//     7. Add the new .cpp file to CMakeLists.txt

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef Pinball_Table_h
#define Pinball_Table_h

#include "Pinball.h"

// This enum represents the main states of the pinball table - these are used to control the overall flow of the game and 
// the key rendering function to call when rendering the screen
enum class PBTableState {
    PBTBL_INIT = 0,
    PBTBL_START = 1,
    PBTBL_MAIN = 2,
    PBTBL_RESET = 3,
    PBTBL_GAMEEND = 4,
    PBTBL_END
};

// ========================================================================
// MODE HEADERS - Each mode defines its own sub-states and enums
// ========================================================================
#include "tablemodes/Pinball_Table_ModeInit.h"
#include "tablemodes/Pinball_Table_ModeStart.h"
#include "tablemodes/Pinball_Table_ModeMain.h"
#include "tablemodes/Pinball_Table_ModeReset.h"
#include "tablemodes/Pinball_Table_ModeGameEnd.h"

// Screen request priority levels
enum class ScreenPriority {
    PRIORITY_LOW = 0,         // Normal gameplay screens
    PRIORITY_MEDIUM = 1,      // Mode transitions, bonus screens
    PRIORITY_HIGH = 2,        // Important events, jackpots
    PRIORITY_CRITICAL = 3     // Cannot be preempted (e.g., game over)
};

// Screen request structure for centralized screen management
struct ScreenRequest {
    PBTableState tableState;         // Main table state to display
    int subScreenState;              // Subscreen enum value for the table state
    ScreenPriority priority;         // Priority level
    unsigned long durationMs;        // How long to display (0 = until cleared)
    unsigned long requestTick;       // When request was made
    bool canBePreempted;             // Can higher priority preempt this?
    
    ScreenRequest() {
        tableState = PBTableState::PBTBL_END;
        subScreenState = -1;
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

// Per-player physical/visual state that maps to hardware outputs driven by the table.
// Kept separate so it can be saved, restored, or extended independently of scoring state.
struct stPlayerTableState {
    bool inlaneLEDLeft;       // Left inlane LED is lit
    bool inlaneLEDRight;      // Right inlane LED is lit
    // Additional per-player table visual state can be added here.

    stPlayerTableState() { reset(); }

    void reset() {
        inlaneLEDLeft  = false;
        inlaneLEDRight = false;
    }
};

// Per-player game state class
class pbGameState {
public:
    PBTableState mainGameState;       // Main game state for this player
    PBTBLMainScreenState mainScreenState; // Main screen substate for this player
    unsigned long score;              // Actual Current score
    unsigned long inProgressScore;    // Score accumulated during current ball
    unsigned long previousScore;      // Previous score to detect changes during animation
    unsigned long scoreUpdateStartTick; // Tick when score update animation started
    bool enabled;                     // Is this player slot enabled/active
    unsigned int currentBall;         // Current ball number (1-based)
    bool ballSaveEnabled;             // Ball save active flag
    bool extraBallEnabled;            // Extra ball earned flag
    unsigned long lastExtraBallThreshold; // Last score milestone that triggered extra ball

    stPlayerTableState tableState;    // Per-player physical/visual table output state
    
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
        mainGameState = PBTableState::PBTBL_MAIN;
        mainScreenState = PBTBLMainScreenState::MAIN_NORMAL;
        score = 0;
        inProgressScore = 0;
        previousScore = 0;
        scoreUpdateStartTick = 0;
        // Note: enabled flag is NOT reset here - must be set explicitly
        currentBall = 1;
        ballSaveEnabled = false;
        extraBallEnabled = false;
        lastExtraBallThreshold = 0;

        // Reset per-player table visual state
        tableState.reset();

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
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
    PBTBL_START = 0,
    PBTBL_MAINSCREEN = 1,
    PBTBL_STDPLAY = 2,
    PBTBL_END
};

enum class PBTBLScreenState {
    START_START = 0,
    START_INST = 1,
    START_SCORES = 2,
    START_OPENDOOR = 3,
    START_END
};

enum class PBTBLMainScreenState {
    MAIN_SHOWSCORE = 0,
    MAIN_END
};

// Per-player game state class
class pbGameState {
public:
    PBTableState mainGameState;       // Main game state for this player
    PBTBLMainScreenState screenState; // Screen state for this player
    unsigned long score;              // Current score
    bool enabled;                     // Is this player slot enabled/active
    unsigned int currentBall;         // Current ball number (1-based)
    bool ballSaveEnabled;             // Ball save active flag
    bool extraBallEnabled;            // Extra ball earned flag

    // Constructor
    pbGameState() {
        reset(3); // Default to 3 balls
    }

    // Reset to initial state based on game settings
    void reset(unsigned int ballsPerGame) {
        mainGameState = PBTableState::PBTBL_MAINSCREEN;
        screenState = PBTBLMainScreenState::MAIN_SHOWSCORE;
        score = 0;
        // Note: enabled flag is NOT reset here - must be set explicitly
        currentBall = 1;
        ballSaveEnabled = false;
        extraBallEnabled = false;
    }
};

#define ACTIVEDISPX 448
#define ACTIVEDISPY 268

#endif // Pinball_Table_h
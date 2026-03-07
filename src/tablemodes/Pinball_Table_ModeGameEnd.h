// Pinball_Table_ModeGameEnd.h:  Header for PBTBL_GAMEEND mode
//   The GameEnd mode handles end-of-game high score entry. It renders the
//   main screen layout (scores, status, borders) statically, then checks
//   each active player's score against the high score board. Players who
//   qualify enter their initials using flipper and action buttons.
//
//   Functions:
//     pbeLoadGameEnd()         - Load game end screen resources
//     pbeRenderGameEnd()       - Render game end screen with initials entry
//     pbeUpdateStateGameEnd()  - Handle input during game end state

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef Pinball_Table_ModeGameEnd_h
#define Pinball_Table_ModeGameEnd_h

// Sub-states for the GameEnd screen
enum class PBTBLGameEndState {
    GAMEEND_INIT = 0,           // Initialize - determine which players made high scores
    GAMEEND_ENTERINITIALS = 1,  // Player is entering initials
    GAMEEND_COMPLETE = 2,       // All done, transition back to start
    GAMEEND_END
};

// Tracks a player that qualified for the high score board
struct GameEndQualifier {
    int playerIndex;           // Player index (0-3)
    unsigned long score;       // Player's final score
    char initials[3];          // Initials entered by player ('A'-'Z')
    
    GameEndQualifier() : playerIndex(-1), score(0) {
        initials[0] = 'A';
        initials[1] = 'A';
        initials[2] = 'A';
    }
};

#endif // Pinball_Table_ModeGameEnd_h

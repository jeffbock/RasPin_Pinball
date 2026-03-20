// Pinball_Table_ModePlayerEnd.h:  Header for PBTBL_PLAYEREND mode
//   The PlayerEnd mode is entered when a player's ball drains (no ball save).
//   It displays "Player X" for a short countdown, then fires the hopper
//   ejector to deliver the next ball, and finally returns to PBTBL_MAIN.
//
//   Functions:
//     pbeLoadPlayerEnd()         - Load player-end screen resources
//     pbeRenderPlayerEnd()       - Render the player-end screen
//     pbeUpdateStatePlayerEnd()  - Handle input/timer events during player-end

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef Pinball_Table_ModePlayerEnd_h
#define Pinball_Table_ModePlayerEnd_h

// Sub-states for the PlayerEnd screen
enum class PBTBLPlayerEndState {
    PLAYEREND_DISPLAY  = 0,  // Showing "Player X" countdown (4 seconds)
    PLAYEREND_EJECTING = 1,  // Hopper ejecting ball, waiting for delivery
    PLAYEREND_END
};

#endif // Pinball_Table_ModePlayerEnd_h

// Pinball_Table_ModeInTower.h:  Header for PBTBL_INTOWER mode
//   The InTower mode is entered when a ball is locked in the tower area
//   (IDI_TOWER sensor triggered). The mode handles tower-lock gameplay
//   until a future exit condition is defined.
//
//   Functions:
//     pbeLoadInTower()             - Load InTower screen resources
//     pbeRenderInTower()           - Render InTower screen
//     pbeUpdateStateInTower()      - Handle input during InTower state

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef Pinball_Table_ModeInTower_h
#define Pinball_Table_ModeInTower_h

// Sub-states for the InTower game screen
enum class PBTBLInTowerScreenState {
    INTOWER_SCREEN_ACTIVE = 0,   // Ball is locked in tower, mode is running
    INTOWER_SCREEN_END = 1
};

// InTower mode sub-states (used within the mode system)
enum class PBInTowerState {
    INTOWER_IDLE = 0,     // Waiting to be entered
    INTOWER_RUNNING = 1,  // Tower lock mode active
    INTOWER_COMPLETE = 2  // Mode complete, returning to normal play
};

#endif // Pinball_Table_ModeInTower_h

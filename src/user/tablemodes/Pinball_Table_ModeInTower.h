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

// ========================================================================
// DUNGEON GRID DATA STRUCTURES
// ========================================================================

// State of a single door cell in the dungeon grid
enum class DoorState {
    DOOR_NONE    = 0,  // Location not part of this dungeon layout
    DOOR_OPEN    = 1,  // Door has been opened by the player
    DOOR_CLOSED  = 2   // Door exists but has not been opened
};

// A single door cell in the dungeon grid
// [row][col], row 0 = bottom floor (ground), row 4 = top floor
struct DoorCell {
    DoorState state;    // Current door state
    bool hasLadder;     // Ladder leading up to the row above from this cell
    bool isDragonLair;  // This is the final dragon lair door (top floor)
    bool hasTorch;      // Wall to the right uses doorwall1 (torch); false = doorwall2

    // --- Future room metadata (add fields here as needed) ---
    // int monsterCount;
    // int roomType;
    // int treasureValue;

    DoorCell() : state(DoorState::DOOR_NONE), hasLadder(false), isDragonLair(false), hasTorch(false) {}
};

// The full 5-row × 3-column dungeon grid for one player
struct TowerDungeonGrid {
    DoorCell cells[5][3]; // [row][col]
    TowerDungeonGrid() {} // DoorCell default-constructor zeros all entries
};

#endif // Pinball_Table_ModeInTower_h

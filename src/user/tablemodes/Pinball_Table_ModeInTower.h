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

// ------------------------------------------------------------------------
// D20 DICE CALIBRATION TOOL (developer-only)
//   Uncomment to enable the InTower D20 dice calibration mode.  This is a
//   fully self-contained tool used to generate the per-value face-orientation
//   table (kD20Orient[]) in Pinball_Table_ModeInTower.cpp for a D20 model.
//   When defined, press IDI_START while in the InTower screen to toggle it;
//   the confirmed (value -> rx,ry,rz) tuples are logged to the pinball console
//   (and console.txt) for pasting into the table.  Leave commented out for
//   production builds.  Safe to remove this define and the calibration block.
#define D20_CALIBRATION

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
    int  monsterCount;  // Number of enemies in this room (set per level in pbeInitDungeonGrid)

    // --- Future room metadata (add fields here as needed) ---
    // int roomType;
    // int treasureValue;

    DoorCell() : state(DoorState::DOOR_NONE), hasLadder(false), isDragonLair(false), hasTorch(false), monsterCount(0) {}
};

// The full 5-row × 3-column dungeon grid for one player
struct TowerDungeonGrid {
    DoorCell cells[5][3]; // [row][col]

    // Per-TC open/closed state for the side mini-tower (5 slots covers all dungeon levels).
    // Index i corresponds to the TC sprite between floor i and floor i+1.
    // towerSectionOpen[0] starts true (bottom entrance is always accessible);
    // all others start false (closed) and are opened when the stair door on floor i is opened.
    bool towerSectionOpen[5];

    TowerDungeonGrid() {
        // DoorCell default-constructor zeros all entries.
        // All TC sections start closed; only the TO base sprite is always rendered open.
        for (int i = 0; i < 5; i++) towerSectionOpen[i] = false;
    }
};

#endif // Pinball_Table_ModeInTower_h

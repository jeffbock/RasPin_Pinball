// Pinball_Table_ModeStart.h:  Header for PBTBL_START mode
//   The Start mode displays the attract screen with doors, instructions,
//   high scores, and handles the door-opening transition to gameplay.
//
//   Functions:
//     pbeLoadGameStart()       - Load start screen sprites and animations
//     pbeRenderGameStart()     - Render start screen with door animations
//     pbeUpdateStateStart()    - Handle input during start state

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef Pinball_Table_ModeStart_h
#define Pinball_Table_ModeStart_h

// Sub-states for the Start screen
enum class PBTBLStartScreenState {
    START_START = 0,
    START_INST = 1,
    START_SCORES = 2,
    START_OPENDOOR = 3,
    START_END = 4
};

#endif // Pinball_Table_ModeStart_h

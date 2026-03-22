// Pinball_Table_ModeMain.h:  Header for PBTBL_MAIN mode
//   The Main mode is the primary gameplay screen showing scores, character
//   status, resource icons, and handling the mode system (normal play,
//   multiball, etc.).
//
//   Functions:
//     pbeLoadMainScreen()          - Load main screen resources
//     pbeRenderMainScreen()        - Render main screen (dispatches to sub-screens)
//     pbeUpdateStateMain()         - Handle input during main gameplay
//
//   Supporting render functions:
//     pbeRenderMainScreenBase()    - Render background, scores, status
//     pbeRenderMainScreenNormal()  - Render normal gameplay overlay
//     pbeRenderMainScreenExtraBall() - Render extra ball video
//     pbeRenderPlayerScores()      - Render all player scores with animation
//     pbeRenderStatusText()        - Render status text with fade effects
//     pbeRenderStatus()            - Render character and resource status
//
//   Mode system functions:
//     pbeUpdateModeSystem()        - Update mode system each frame
//     pbeUpdateModeState()         - Update current mode sub-states
//     pbeCheckModeTransition()     - Check for mode transitions
//     pbeEnterMode()               - Enter a new mode
//     pbeExitMode()                - Exit current mode
//     pbeUpdateNormalPlayMode()    - Normal play input handling
//     pbeUpdateMultiballMode()     - Multiball input handling
//     pbeCheckMultiballQualified() - Check multiball qualification

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef Pinball_Table_ModeMain_h
#define Pinball_Table_ModeMain_h

// Sub-states for the Main game screen
enum class PBTBLMainScreenState {
    MAIN_NORMAL = 0,        
    MAIN_EXTRABALL = 1,     
    MAIN_BALLSAVED = 2,     
    MAIN_END = 3
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

#endif // Pinball_Table_ModeMain_h

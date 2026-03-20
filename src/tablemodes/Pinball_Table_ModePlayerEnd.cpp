// Pinball_Table_ModePlayerEnd.cpp:  PBTBL_PLAYEREND mode implementation
//   Handles the between-turn transition when a player's ball drains.
//   Displays "Player X" on screen for 4 seconds, then enables the hopper
//   ejector to deliver the next ball. Transitions back to PBTBL_MAIN once
//   the ball is delivered.

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "Pinball_Engine.h"
#include "Pinball_Table.h"
#include "PBDevice.h"

// ========================================================================
// PBTBL_PLAYEREND: Load Function
// ========================================================================

bool PBEngine::pbeLoadPlayerEnd(){

    if (m_playerEndLoaded) return (true);

    // PlayerEnd reuses resources from the main screen (fonts, BG sprite, etc.)
    if (!pbeLoadMainScreen()) {
        pbeSendConsole("ERROR: Failed to load main screen resources for player end");
        return (false);
    }

    m_playerEndLoaded = true;
    return (true);
}

// ========================================================================
// PBTBL_PLAYEREND: Render Function
// ========================================================================

bool PBEngine::pbeRenderPlayerEnd(unsigned long currentTick, unsigned long lastTick){

    if (!pbeLoadPlayerEnd()) {
        pbeSendConsole("ERROR: Failed to load player end screen resources");
        return (false);
    }

    PBTBLPlayerEndState currentState = static_cast<PBTBLPlayerEndState>(m_tableSubScreenState);

    // Clear to black and render side-panel scores and status icons
    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
    pbeRenderPlayerScores(currentTick, lastTick, false);
    pbeRenderStatus(currentTick, lastTick);

    // Center text: "Player X" in large letters
    int centerX = ACTIVEDISPX + (1024 / 3);

    std::string playerLabel = "Player " + std::to_string(m_playerEndNextPlayer + 1);
    gfxSetColor(m_StartMenuFontId, 255, 215, 0, 255);  // Gold
    gfxSetScaleFactor(m_StartMenuFontId, 1.2, false);
    gfxRenderShadowString(m_StartMenuFontId, playerLabel,
        centerX, ACTIVEDISPY + 305, 5, GFX_TEXTCENTER,
        0, 0, 0, 255, 3);

    // Sub-text line
    gfxSetColor(m_StartMenuFontId, 200, 200, 200, 255);
    gfxSetScaleFactor(m_StartMenuFontId, 0.5, false);
    if (currentState == PBTBLPlayerEndState::PLAYEREND_EJECTING) {
        gfxRenderShadowString(m_StartMenuFontId, "Get Ready!",
            centerX, ACTIVEDISPY + 430, 5, GFX_TEXTCENTER,
            0, 0, 0, 255, 2);
    } else {
        gfxRenderShadowString(m_StartMenuFontId, "Your Turn",
            centerX, ACTIVEDISPY + 430, 5, GFX_TEXTCENTER,
            0, 0, 0, 255, 2);
    }

    // Overlay the main screen divider bars on top
    gfxRenderSprite(m_PBTBLMainScreenBGId, ACTIVEDISPX, ACTIVEDISPY);

    return (true);
}

// ========================================================================
// PBTBL_PLAYEREND: State Update Function
// ========================================================================

void PBEngine::pbeUpdateStatePlayerEnd(stInputMessage inputMessage){

    // --- One-time initialization when entering this mode ---
    if (!m_playerEndInitialized) {
        m_tableSubScreenState     = static_cast<int>(PBTBLPlayerEndState::PLAYEREND_DISPLAY);
        m_playerEndInitialized    = true;
        pbeSetTimer(PLAYEREND_DISPLAY_TIMER_ID, 2000);
        return;
    }

    PBTBLPlayerEndState currentState = static_cast<PBTBLPlayerEndState>(m_tableSubScreenState);

    switch (currentState) {

        case PBTBLPlayerEndState::PLAYEREND_DISPLAY:
            // Wait for the 4-second display timer to fire
            if (inputMessage.inputMsg == PB_IMSG_TIMER &&
                inputMessage.inputId == PLAYEREND_DISPLAY_TIMER_ID) {

                // Switch over to the next player and activate hardware state
                m_currentPlayer = static_cast<unsigned int>(m_playerEndNextPlayer);
                pbeActivatePlayer(m_currentPlayer);

                // Enable and start the hopper ejector
                if (m_hopperDevice) {
                    m_hopperDevice->pbdEnable(true);
                    m_hopperDevice->pdbStartRun();
                }

                m_tableSubScreenState = static_cast<int>(PBTBLPlayerEndState::PLAYEREND_EJECTING);
            }
            break;

        case PBTBLPlayerEndState::PLAYEREND_EJECTING:
            // Wait for ball-delivered sensor to confirm the ball is in play
            if (inputMessage.inputMsg == PB_IMSG_SENSOR &&
                inputMessage.inputState == PB_ON &&
                inputMessage.inputId == IDI_BALLDELIVERED) {

                // Disable hopper – ball is now in play
                if (m_hopperDevice) {
                    m_hopperDevice->pbdEnable(false);
                }

                // Clean up PlayerEnd state and return to main gameplay
                m_playerEndInitialized = false;
                m_tableState           = PBTableState::PBTBL_MAIN;
                m_tableSubScreenState  = static_cast<int>(PBTBLMainScreenState::MAIN_NORMAL);
                pbeClearScreenRequests();
                pbeRequestScreen(PBTableState::PBTBL_MAIN,
                                 static_cast<int>(PBTBLMainScreenState::MAIN_NORMAL),
                                 ScreenPriority::PRIORITY_LOW, 0, true);
            }
            break;

        default:
            break;
    }
}

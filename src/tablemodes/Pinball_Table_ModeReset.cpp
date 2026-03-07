// Pinball_Table_ModeReset.cpp:  PBTBL_RESET mode implementation
//   Handles the reset confirmation screen. Pressing reset again returns
//   to the main menu; any other button cancels and restores the previous state.

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "Pinball_Engine.h"
#include "Pinball_Table.h"

// ========================================================================
// PBTBL_RESET: Load Function
// ========================================================================

bool PBEngine::pbeLoadReset(){
    
    if (m_resetLoaded) return (true);
    
    // Reset screen uses the same font as the start menu
    // The font should already be loaded from pbeLoadGameStart
    
    // Create a solid color sprite (like the title bar in console state)
    m_PBTBLResetSpriteId = gfxLoadSprite("ResetBG", "", GFX_NONE, GFX_NOMAP, GFX_UPPERLEFT, false, false);
    
    if (m_PBTBLResetSpriteId == NOSPRITE) return (false);
    
    gfxSetColor(m_PBTBLResetSpriteId, 0, 0, 0, 255); // Solid black
    gfxSetWH(m_PBTBLResetSpriteId, 700, 200); // Set width and height
    
    m_resetLoaded = true;
    return (true);
}

// ========================================================================
// PBTBL_RESET: Render Function
// ========================================================================

bool PBEngine::pbeRenderReset(unsigned long currentTick, unsigned long lastTick){
    
    if (!pbeLoadReset()) {
        pbeSendConsole("ERROR: Failed to load reset screen resources");
        return (false);
    }
    
    // Clear to "blue screen of death" blue color for consistent, recognizable background
    // RGB(0, 0, 170) is classic BSOD blue (0x0000AA)
    gfxClear(0.0f, 0.667f, 0.0f, 1.0f, false);  // Blue (170/255 = 0.667) - params are R,B,G,A
    
    // Center position in active area
    // fix cernter x to use full screen width
    int centerX = (PB_SCREENWIDTH / 2);
    int centerY = ACTIVEDISPY + 384; // Center of active area (768 / 2)
    
    // Render the black background sprite centered (overlay)
    gfxRenderSprite(m_PBTBLResetSpriteId, centerX - 350, centerY - 80); // 770x200 sprite, so offset by half width/height
    
    // Render white text over the black sprite
    gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
    gfxSetScaleFactor(m_StartMenuFontId, 1.0, false);
    
    // Main text: "Press reset for menu"
    gfxRenderString(m_StartMenuFontId, "Press reset for menu", centerX, centerY - 65, 5, GFX_TEXTCENTER);
    
    // Smaller secondary text: "Any button to cancel"
    gfxSetScaleFactor(m_StartMenuFontId, 0.7, false);
    gfxRenderString(m_StartMenuFontId, "Any button to cancel", centerX, centerY + 30, 3, GFX_TEXTCENTER);
    
    return (true);
}

// ========================================================================
// PBTBL_RESET: State Update Function
// ========================================================================

void PBEngine::pbeUpdateStateReset(stInputMessage inputMessage){
    // Handle reset state inputs
    if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
        if (inputMessage.inputId == IDI_RPIOP24_RESET) {
            // Reset pressed again - clean up and return to main menu
            m_ResetButtonPressed = false;
            m_GameStarted = false; // Reset game started flag
            
            // Reload all resources by resetting load states
            pbeEngineReload();
            pbeTableReload();
            
            m_mainState = PB_STARTMENU; // Return to main menu in Pinball_Engine
            m_tableState = PBTableState::PBTBL_START; // Reset table state
            m_tableSubScreenState = static_cast<int>(PBTBLStartScreenState::START_START);
            
            // Clear screen queue (will be repopulated when start menu state processes)
            pbeClearScreenRequests();

            // Stop any playing music/effects
            m_soundSystem.pbsStopAllEffects();
            m_soundSystem.pbsStopMusic();
        }
        else if (inputMessage.inputId == IDI_RPIOP06_START || 
                 inputMessage.inputId == IDI_RPIOP05_LACTIVATE || 
                 inputMessage.inputId == IDI_RPIOP22_RACTIVATE ||
                 inputMessage.inputId == IDI_RPIOP27_LFLIP ||
                 inputMessage.inputId == IDI_RPIOP17_RFLIP) {
            // Any other button pressed - cancel reset and return to previous state
            m_ResetButtonPressed = false;
            m_tableState = m_StateBeforeReset;
            
            // Clear screen queue and restore the saved screen as priority 0
            pbeClearScreenRequests();
            if (m_hasScreenBeforeReset) {
                PBTableState tableStateToRestore = m_ScreenBeforeResetState;
                int subStateToRestore = m_ScreenBeforeResetSubState;

                if (tableStateToRestore == PBTableState::PBTBL_MAIN) {
                    subStateToRestore = static_cast<int>(PBTBLMainScreenState::MAIN_NORMAL);
                }

                m_tableSubScreenState = subStateToRestore;

                pbeRequestScreen(tableStateToRestore, subStateToRestore, ScreenPriority::PRIORITY_LOW, 0, true);
            }
            // If no saved screen, pbeRenderGameScreen will request appropriate screen based on m_tableState
        }
    }
}

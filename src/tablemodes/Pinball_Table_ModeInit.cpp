// Pinball_Table_ModeInit.cpp:  PBTBL_INIT mode implementation
//   Handles table initialization (devices, NeoPixels) and transitions to start screen.

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "Pinball_Engine.h"
#include "Pinball_Table.h"
#include "PBDevice.h"

// ========================================================================
// PBTBL_INIT: Load Function
// ========================================================================

bool PBEngine::pbeLoadInitScreen(){
    
    if (m_initScreenLoaded) return (true);
    
    // Initialize table devices and state
    pbeTableInit();
    
    m_initScreenLoaded = true;
    return true;
}

// ========================================================================
// PBTBL_INIT: Render Function
// ========================================================================

bool PBEngine::pbeRenderInitScreen(unsigned long currentTick, unsigned long lastTick){
    
    if (!pbeLoadInitScreen()) {
        pbeSendConsole("ERROR: Failed to load init screen resources");
        return (false);
    }
    
    // Transition to start screen
    m_RestartTable = true;  // Initialize start screen on next render
    m_tableState = PBTableState::PBTBL_START;
    return true;
}

// ========================================================================
// PBTBL_INIT: State Update Function
// ========================================================================

void PBEngine::pbeUpdateStateInit(stInputMessage inputMessage){
    // Nothing right now, it's all taken care in the init render function
}

// ========================================================================
// PBTBL_INIT: Helper Functions
// ========================================================================

bool PBEngine::pbeTableInit(){
    
    // Initialize prototype game state variables
    m_hopperDevice       = nullptr;
    m_leftInlaneLEDOn    = false;
    m_rightInlaneLEDOn   = false;
    m_mainNeoPixelMode   = 0;
    m_ballEjectPending   = false;

    // Create and register the ball hopper ejector device
    m_hopperDevice = new pbdHopperEjector(this, IDI_BALLREADY, IDO_EJECT, IDI_BALLDELIVERED);
    pbeAddDevice(m_hopperDevice);
    
    // Initialize all NeoPixels to black/off when starting the pinball game
    // Iterate through all NeoPixel drivers in the map
    for (auto& driver : m_NeoPixelDriverMap) {
        int boardIndex = driver.first;
        // Find the corresponding output ID from g_outputDef
        for (int i = 0; i < NUM_OUTPUTS; i++) {
            if (g_outputDef[i].boardIndex == boardIndex && g_outputDef[i].boardType == PB_NEOPIXEL) {
                // Set all pixels to black/off (0, 0, 0)
                SendNeoPixelAllMsg(i, 0, 0, 0, 255);
                break;
            }
        }
    }
    
    return (true);
}

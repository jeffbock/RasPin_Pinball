// Pinball_Table_ModeInTower.cpp:  PBTBL_INTOWER mode implementation
//   Handles the tower lock screen. Entered when IDI_TOWER sensor is triggered.
//   Reuses the main screen base rendering (background, scores, status) and
//   renders tower-specific content only within the main status/scoring area.
//   No exit condition yet; will be added in a future task.

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "Pinball_Engine.h"
#include "Pinball_Table.h"
#include "PBDevice.h"

// ========================================================================
// PBTBL_INTOWER: Load Function
// ========================================================================

bool PBEngine::pbeLoadInTower() {

    if (m_inTowerLoaded) return (true);

    // InTower reuses the main screen background resources; ensure they are loaded
    if (!pbeLoadMainScreen()) return (false);

    m_TowerClimbId = gfxLoadSprite("TowerClimb", "src/user/resources/textures/towerclimb.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, true, true);
    gfxSetColor(m_TowerClimbId, 80, 80, 160, 255);

    m_inTowerLoaded = true;
    return (true);
}

// ========================================================================
// PBTBL_INTOWER: Render Function
// ========================================================================

bool PBEngine::pbeRenderInTower(unsigned long currentTick, unsigned long lastTick) {

    if (!pbeLoadInTower()) {
        pbeSendConsole("ERROR: Failed to load InTower screen resources");
        return (false);
    }

    // Render the standard main screen base: black background, player scores,
    // status text and icons, NeoPixel animation
    if (!pbeRenderMainScreenBase(currentTick, lastTick)) {
        return (false);
    }

    // Render towerclimb image centered in the main score area
    gfxRenderSprite(m_TowerClimbId, ACTIVEDISPX + (1024 / 3), ACTIVEDISPY + 350);

    // Render divider bars on top, same as pbeRenderMainScreen
    gfxRenderSprite(m_PBTBLMainScreenBGId, ACTIVEDISPX, ACTIVEDISPY);

    // Re-render ball text and status text above the tower image and overlay
    pbeRenderStatusText(currentTick, lastTick);

    return (true);
}

// ========================================================================
// PBTBL_INTOWER: State Update Function
// ========================================================================

void PBEngine::pbeUpdateStateInTower(stInputMessage inputMessage) {
    // No exit condition yet; all input handling will be added in a future task.
    // The mode system update still runs to handle screen manager bookkeeping.
    pbeUpdateModeSystem(inputMessage, GetTickCountGfx());
}

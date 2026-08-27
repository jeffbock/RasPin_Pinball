// Pinball_Table_ModeDragonMultiball.cpp: PBTBL_DRAGONMULTIBALL mode implementation
//
// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found in the project root.

#include "Pinball_Engine.h"
#include "Pinball_Table.h"
#include "PBDevice.h"

bool PBEngine::pbeLoadDragonMultiball() {
    if (m_dragonMultiballLoaded) return true;
    if (!pbeLoadMainScreen()) return false;

    m_dragonMultiballLoaded = true;
    return true;
}

bool PBEngine::pbeRenderDragonMultiball(unsigned long currentTick, unsigned long lastTick,
                                        PBTBLDragonMultiballScreenState subScreenState) {
    (void)subScreenState;
    if (!pbeLoadDragonMultiball() || !pbeRenderMainScreenBase(currentTick, lastTick)) return false;

    const int centerX = ACTIVEDISPX + (1024 / 3);
    const int centerY = ACTIVEDISPY + 350;
    pbeSetStatusText(0, "Dragon Multiball");
    pbeSetStatusText(1, m_dragonMultiballResult == 0 ? "Left Activate: Fail  Right Activate: Defeat" : "");

    gfxSetColor(m_StartMenuFontId, 255, 215, 0, 255);
    gfxRenderString(m_StartMenuFontId, "Dragon Multiball", centerX, centerY - 20, 6, GFX_TEXTCENTER);
    if (m_dragonMultiballResult != 0) {
        gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
        gfxRenderString(m_StartMenuFontId,
                        m_dragonMultiballResult == 1 ? "Failed!" : "Dragon is defeated!",
                        centerX, centerY + 55, 4, GFX_TEXTCENTER);
        if (currentTick - m_dragonMultiballResultTick >= 1500UL) {
            pbeExitMode(PBTableMode::MODE_INTOWER, currentTick);
            pbeEnterMode(PBTableMode::MODE_NORMAL_PLAY, currentTick);
            m_tableState = PBTableState::PBTBL_MAIN;
        }
    }
    return true;
}

void PBEngine::pbeUpdateStateDragonMultiball(stInputMessage inputMessage) {
    if (inputMessage.inputMsg != PB_IMSG_BUTTON || inputMessage.inputState != PB_ON || m_dragonMultiballResult != 0) return;

    pbGameState& player = m_playerStates[m_currentPlayer];
    if (inputMessage.inputId == IDI_LACTIVATE) {
        if (m_inTowerOpenedRow >= 0 && m_inTowerOpenedCol >= 0) {
            player.dungeonGrid.cells[m_inTowerOpenedRow][m_inTowerOpenedCol].state = DoorState::DOOR_CLOSED;
            player.towerResumeFloor = player.dungeonFloor;
            player.towerResumeDoor = m_inTowerOpenedCol;
        }
        m_dragonMultiballResult = 1;
        m_dragonMultiballResultTick = GetTickCountGfx();
    } else if (inputMessage.inputId == IDI_RACTIVATE) {
        if (player.dungeonLevel < 3) player.dungeonLevel++;
        player.dungeonFloor = 1;
        player.towerResumeFloor = 1;
        player.towerResumeDoor = -1;
        player.towerNeedsReset = true;
        m_dragonMultiballResult = 2;
        m_dragonMultiballResultTick = GetTickCountGfx();
    }
}

// Pinball_Table.cpp:  Shared infrastructure for the pinball table
//   This file contains the main state dispatch, screen manager, player management,
//   and helper functions used across all table modes.
//   Mode-specific load/render/update functions are in src/tablemodes/Pinball_Table_ModeX.cpp

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "Pinball_Engine.h"
#include "Pinball_Table.h"
#include <algorithm>  // For std::sort in screen manager

// ========================================================================
// MAIN RENDER DISPATCH
// ========================================================================

// Main render selection function for the pinball table
// All states now use screen manager with priority 0 for state-based screens
bool PBEngine::pbeRenderGameScreen(unsigned long currentTick, unsigned long lastTick){

    bool success = false;

    // Request appropriate priority 0 screen based on current table state
    // The optimization in pbeRequestScreen prevents duplicate requests
    switch (m_tableState) {
        case PBTableState::PBTBL_INIT:
            pbeRequestScreen(PBTableState::PBTBL_INIT, -1, ScreenPriority::PRIORITY_LOW, 0, true);
            break;
        case PBTableState::PBTBL_START:
            pbeRequestScreen(PBTableState::PBTBL_START, m_tableSubScreenState, ScreenPriority::PRIORITY_LOW, 0, true);
            break;
        case PBTableState::PBTBL_MAIN:
            // Maintain the base main screen as the priority-0 background
            pbeRequestScreen(PBTableState::PBTBL_MAIN, m_tableSubScreenState, ScreenPriority::PRIORITY_LOW, 0, true);
            break;
        case PBTableState::PBTBL_RESET:
            pbeRequestScreen(PBTableState::PBTBL_RESET, -1, ScreenPriority::PRIORITY_LOW, 0, true);
            break;
        case PBTableState::PBTBL_GAMEEND:
            pbeRequestScreen(PBTableState::PBTBL_GAMEEND, m_tableSubScreenState, ScreenPriority::PRIORITY_LOW, 0, true);
            break;
        default:
            break;
    }

    // Update screen manager before rendering (processes queue, handles expiration)
    pbeUpdateScreenManager(currentTick);

    // Render based on current screen from screen manager
    PBTableState currentScreenState = pbeGetCurrentScreenState();
    int currentSubScreenState = pbeGetCurrentSubScreenState();

    switch (currentScreenState) {
        case PBTableState::PBTBL_INIT:
            success = pbeRenderInitScreen(currentTick, lastTick);
            break;

        case PBTableState::PBTBL_START:
            success = pbeRenderGameStart(currentTick, lastTick);
            break;

        case PBTableState::PBTBL_MAIN: {
            PBTBLMainScreenState mainState = static_cast<PBTBLMainScreenState>(currentSubScreenState);
            switch (mainState) {
                case PBTBLMainScreenState::MAIN_NORMAL:
                case PBTBLMainScreenState::MAIN_EXTRABALL:
                    success = pbeRenderMainScreen(currentTick, lastTick, mainState);
                    break;
                default:
                    success = false;
                    break;
            }
            break;
        }

        case PBTableState::PBTBL_RESET:
            success = pbeRenderReset(currentTick, lastTick);
            break;

        case PBTableState::PBTBL_GAMEEND:
            success = pbeRenderGameEnd(currentTick, lastTick);
            break;
            
        default:
            // No valid screen, render nothing
            success = false;
            break;
    }

    // Render the backglass all the time (this will cover any other sprites and hide anything that's off the mini-screen)
    // It's got a transparent mini screen which will show the mini render.
    gfxRenderSprite(m_PBTBLBackglassId, 0, 0);

    return (success);
}

// ========================================================================
// MAIN STATE UPDATE DISPATCH
// ========================================================================

//
// Main State Loop for Pinball Table
//
void PBEngine::pbeUpdateGameState(stInputMessage inputMessage){
    
    // Check for reset button press first, regardless of current state (unless already in reset)
    if (m_tableState != PBTableState::PBTBL_RESET) {
        if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON && inputMessage.inputId == IDI_RPIOP24_RESET) {
            if (!m_ResetButtonPressed) {
                // First time reset is pressed - save current state and screen
                m_StateBeforeReset = m_tableState;
                m_ScreenBeforeResetState = m_currentDisplayedTableState;
                m_ScreenBeforeResetSubState = m_currentDisplayedSubScreen;
                m_hasScreenBeforeReset = (m_currentDisplayedTableState != PBTableState::PBTBL_END);
                m_ResetButtonPressed = true;
                m_tableState = PBTableState::PBTBL_RESET;
                
                // Clear all pending screen requests and queue reset screen
                pbeClearScreenRequests();
                pbeRequestScreen(PBTableState::PBTBL_RESET, -1, ScreenPriority::PRIORITY_LOW, 0, true);
                
                return; // Exit early, we're now in reset state
            }
        }
    }
    
    // Dispatch to mode-specific state update functions
    switch (m_tableState) {
        case PBTableState::PBTBL_INIT:
            pbeUpdateStateInit(inputMessage);
            break;
        case PBTableState::PBTBL_START:
            pbeUpdateStateStart(inputMessage);
            break;
        case PBTableState::PBTBL_MAIN:
            pbeUpdateStateMain(inputMessage);
            break;
        case PBTableState::PBTBL_RESET:
            pbeUpdateStateReset(inputMessage);
            break;
        case PBTableState::PBTBL_GAMEEND:
            pbeUpdateStateGameEnd(inputMessage);
            break;
        default:
            break;
    }
}

// ========================================================================
// TABLE RELOAD AND HELPER FUNCTIONS
// ========================================================================

// Reload function to reset all table screen load states
void PBEngine::pbeTableReload() {
    m_initScreenLoaded = false;
    m_gameStartLoaded = false;
    m_mainScreenLoaded = false;
    m_resetLoaded = false;
    m_gameEndLoaded = false;
    m_RestartTable = true;
    
    // Clean up extra ball video player
    if (m_extraBallVideoPlayer) {
        m_extraBallVideoPlayer->pbvpStop();
        m_extraBallVideoPlayer->pbvpUnloadVideo();
        delete m_extraBallVideoPlayer;
        m_extraBallVideoPlayer = nullptr;
        m_extraBallVideoSpriteId = NOSPRITE;
        m_extraBallVideoLoaded = false;
    }
}

// Various Helper and initialization functions used throughout the game

bool PBEngine::pbeTryAddPlayer(){
    // Find the first disabled player slot
    int nextPlayerIdx = -1;
    for (int i = 0; i < 4; i++) {
        if (!m_playerStates[i].enabled) {
            nextPlayerIdx = i;
            break;
        }
    }
    
    // If no disabled player found, cannot add
    if (nextPlayerIdx == -1) {
        return false;
    }
    
    // Check if at least one enabled player is still on their first ball
    bool canAddPlayer = false;
    for (int i = 0; i < 4; i++) {
        if (m_playerStates[i].enabled && m_playerStates[i].currentBall == 1) {
            canAddPlayer = true;
            break;
        }
    }
    
    if (!canAddPlayer) {
        return false;
    }
    
    // Enable and initialize the new player
    m_playerStates[nextPlayerIdx].reset(m_saveFileData.ballsPerGame);
    m_playerStates[nextPlayerIdx].enabled = true;
    
    // Count how many secondary players are now active (excluding current player)
    int secondaryCount = 0;
    for (int i = 0; i < 4; i++) {
        if (i != m_currentPlayer && m_playerStates[i].enabled) {
            secondaryCount++;
        }
    }
    
    // Initialize scroll-up animation for the new secondary score slot
    // The slot index is (secondaryCount - 1) since we just added this player
    if (secondaryCount > 0 && secondaryCount <= 3) {
        int slotIndex = secondaryCount - 1;
        m_secondaryScoreAnims[slotIndex].animStartTick = GetTickCountGfx();
        m_secondaryScoreAnims[slotIndex].animDurationSec = 1.0f;
        m_secondaryScoreAnims[slotIndex].currentYOffset = 50; // Start 200 pixels below (off-screen)
        m_secondaryScoreAnims[slotIndex].animationActive = true;
        m_secondaryScoreAnims[slotIndex].playerIndex = nextPlayerIdx;
    }
    
    // Play feedback sound
    m_soundSystem.pbsPlayEffect(SOUNDCLICK);
    
    return true;
}

unsigned long PBEngine::getCurrentPlayerScore(){
    return m_playerStates[m_currentPlayer].score;
}

bool PBEngine::isCurrentPlayerEnabled(){
    return m_playerStates[m_currentPlayer].enabled;
}

PBTableState& PBEngine::getPlayerGameState(){
    return m_playerStates[m_currentPlayer].mainGameState;
}

PBTBLMainScreenState& PBEngine::getPlayerScreenState(){
    return m_playerStates[m_currentPlayer].mainScreenState;
}

void PBEngine::addPlayerScore(unsigned long points){
    m_playerStates[m_currentPlayer].score += points;
}

std::string PBEngine::formatScoreWithCommas(unsigned long score){
    std::string scoreStr = std::to_string(score);
    int insertPosition = scoreStr.length() - 3;
    
    while (insertPosition > 0) {
        scoreStr.insert(insertPosition, ",");
        insertPosition -= 3;
    }
    
    return scoreStr;
}

// ========================================================================
// SCREEN MANAGER IMPLEMENTATION
// ========================================================================

// Request a screen to be displayed
void PBEngine::pbeRequestScreen(PBTableState tableState, int subScreenState, ScreenPriority priority, 
                                unsigned long durationMs, bool canBePreempted) {
    // Check if this screen is already in the queue (any priority)
    for (const auto& req : m_screenQueue) {
        if (req.tableState == tableState && req.subScreenState == subScreenState) {
            // Screen already queued, skip adding it again
            return;
        }
    }
    
    // Priority 0 screens are persistent background screens - only one should exist
    // Remove any existing priority 0 screen when requesting a new one
    if (static_cast<int>(priority) == 0) {
        auto it = m_screenQueue.begin();
        while (it != m_screenQueue.end()) {
            if (static_cast<int>(it->priority) == 0) {
                it = m_screenQueue.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    ScreenRequest request;
    request.tableState = tableState;
    request.subScreenState = subScreenState;
    request.priority = priority;
    request.durationMs = durationMs;
    request.requestTick = GetTickCountGfx();
    request.canBePreempted = canBePreempted;
    
    // Add to queue (we'll sort by priority in update function)
    m_screenQueue.push_back(request);
}

// Update screen manager - determines which screen should be displayed
void PBEngine::pbeUpdateScreenManager(unsigned long currentTick) {
    // First, remove expired screens from queue (priority >0 with elapsed time)
    auto it = m_screenQueue.begin();
    while (it != m_screenQueue.end()) {
        bool expired = false;
        
        // Priority >0 screens MUST have duration and expire when time is up
        if (static_cast<int>(it->priority) > 0 && it->durationMs > 0) {
            if (currentTick - it->requestTick > it->durationMs) {
                expired = true;
            }
        }
        
        if (expired) {
            if (it->tableState == m_currentDisplayedTableState && it->subScreenState == m_currentDisplayedSubScreen) {
                m_currentDisplayedTableState = PBTableState::PBTBL_END;
                m_currentDisplayedSubScreen = -1; // Clear current screen
            }
            it = m_screenQueue.erase(it);
        } else {
            ++it;
        }
    }
    
    if (m_screenQueue.empty()) {
        m_currentDisplayedTableState = PBTableState::PBTBL_END;
        m_currentDisplayedSubScreen = -1;
        return; // No screen requests
    }
    
    // Sort queue by priority (highest first)
    std::sort(m_screenQueue.begin(), m_screenQueue.end(), 
        [](const ScreenRequest& a, const ScreenRequest& b) {
            return static_cast<int>(a.priority) > static_cast<int>(b.priority);
        });
    
    // Determine which screen should be displayed
    PBTableState tableStateToDisplay = PBTableState::PBTBL_END;
    int subScreenToDisplay = -1;
    
    // Priority 0 is special - it's the "background" screen
    // Priority >0 screens display over the priority 0 screen
    
    // Find the highest priority >0 screen (if any)
    ScreenRequest* highestNonZero = nullptr;
    for (auto& req : m_screenQueue) {
        if (static_cast<int>(req.priority) > 0) {
            highestNonZero = &req;
            break; // Already sorted, so first one is highest
        }
    }
    
    if (highestNonZero != nullptr) {
        // Display the highest priority >0 screen
        tableStateToDisplay = highestNonZero->tableState;
        subScreenToDisplay = highestNonZero->subScreenState;
    } else {
        // No priority >0 screens, use priority 0 screen (if present)
        for (auto& req : m_screenQueue) {
            if (static_cast<int>(req.priority) == 0) {
                tableStateToDisplay = req.tableState;
                subScreenToDisplay = req.subScreenState;
                break;
            }
        }
    }
    
    // Update displayed screen if changed
    if ((m_currentDisplayedTableState != tableStateToDisplay || m_currentDisplayedSubScreen != subScreenToDisplay) &&
        tableStateToDisplay != PBTableState::PBTBL_END) {
        m_currentDisplayedTableState = tableStateToDisplay;
        m_currentDisplayedSubScreen = subScreenToDisplay;
        m_currentScreenStartTick = currentTick;
    }
}

// Clear all screen requests
void PBEngine::pbeClearScreenRequests() {
    m_screenQueue.clear();
    m_currentDisplayedTableState = PBTableState::PBTBL_END;
    m_currentDisplayedSubScreen = -1;
    m_currentScreenStartTick = 0;
}

// Clear only priority 0 screen (background screen)
// This allows numbered priority queue to be followed without a background
void PBEngine::pbeClearPriority0Screen() {
    auto it = m_screenQueue.begin();
    while (it != m_screenQueue.end()) {
        if (static_cast<int>(it->priority) == 0) {
            if (it->tableState == m_currentDisplayedTableState && it->subScreenState == m_currentDisplayedSubScreen) {
                m_currentDisplayedTableState = PBTableState::PBTBL_END;
                m_currentDisplayedSubScreen = -1; // Clear if this was displaying
            }
            it = m_screenQueue.erase(it);
        } else {
            ++it;
        }
    }
}

// Get the current screen that should be displayed
PBTableState PBEngine::pbeGetCurrentScreenState() {
    return m_currentDisplayedTableState;
}

int PBEngine::pbeGetCurrentSubScreenState() {
    return m_currentDisplayedSubScreen;
}


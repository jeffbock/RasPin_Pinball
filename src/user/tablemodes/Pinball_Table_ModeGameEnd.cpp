// Pinball_Table_ModeGameEnd.cpp:  PBTBL_GAMEEND mode implementation
//   Handles end-of-game high score checking and initials entry.
//   Renders the main screen layout statically (scores, status, borders),
//   then allows qualifying players to enter their initials for the high
//   score board.

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "Pinball_Engine.h"
#include "Pinball_Table.h"
#include "PBDevice.h"
#include <algorithm>

// ========================================================================
// PBTBL_GAMEEND: Load Function
// ========================================================================

bool PBEngine::pbeLoadGameEnd(){
    
    if (m_gameEndLoaded) return (true);
    
    // Game end mode reuses resources from the main screen and start menu
    // Ensure main screen resources are loaded (sprites, fonts, etc.)
    if (!pbeLoadMainScreen()) {
        pbeSendConsole("ERROR: Failed to load main screen resources for game end");
        return (false);
    }
    
    // Disable auto-outputs and turn off all gameplay LEDs
    SetAutoOutputEnable(false);
    SendOutputMsg(PB_OMSG_LED, IDO_SAVELED,    PB_OFF, false);
    SendOutputMsg(PB_OMSG_LED, IDO_LINLANELED, PB_OFF, false);
    SendOutputMsg(PB_OMSG_LED, IDO_RINLANELED, PB_OFF, false);
    m_leftInlaneLEDOn  = false;
    m_rightInlaneLEDOn = false;
    m_mainNeoPixelMode = 3;  // Prevent render base from restarting animations
    SendNeoPixelAllMsg(IDO_NEOPIXEL0, 0, 0, 0, 255);

    m_gameEndLoaded = true;
    return (true);
}

// ========================================================================
// PBTBL_GAMEEND: High Score Qualification Logic
// ========================================================================

// Determines which active players qualify for the high score board.
// Merges all active player scores with current high scores, sorts them,
// and identifies which active players remain in the top NUM_HIGHSCORES.
// This ensures a player won't enter initials only to be displaced by
// another active player with a higher score.
static void determineHighScoreQualifiers(
    pbGameState playerStates[4],
    const std::array<stHighScoreData, NUM_HIGHSCORES>& currentHighScores,
    std::vector<GameEndQualifier>& outQualifiers)
{
    outQualifiers.clear();
    
    // Build a combined list of all scores: existing high scores + active player scores
    // Each entry tracks: score value, source index, and whether it's from an active player
    struct ScoreEntry {
        unsigned long score;
        int index;          // Player index (0-3) for active players, high score index for existing
        bool isActivePlayer;
    };
    
    std::vector<ScoreEntry> allScores;
    
    // Add all active player scores
    // Note: enabled is already false for all players when game ends (cleared on ball drain),
    // so we check score > 0 only - unused slots always have score = 0.
    for (int i = 0; i < 4; i++) {
        if (playerStates[i].score > 0) {
            ScoreEntry entry;
            entry.score = playerStates[i].score;
            entry.index = i;
            entry.isActivePlayer = true;
            allScores.push_back(entry);
        }
    }
    
    // Add existing high scores
    for (int i = 0; i < NUM_HIGHSCORES; i++) {
        ScoreEntry entry;
        entry.score = currentHighScores[i].highScore;
        entry.index = i;
        entry.isActivePlayer = false;
        allScores.push_back(entry);
    }
    
    // Sort by score descending
    std::sort(allScores.begin(), allScores.end(), 
        [](const ScoreEntry& a, const ScoreEntry& b) {
            return a.score > b.score;
        });
    
    // Take the top NUM_HIGHSCORES entries and find which are active players
    int count = 0;
    for (size_t i = 0; i < allScores.size() && count < NUM_HIGHSCORES; i++) {
        if (allScores[i].isActivePlayer) {
            GameEndQualifier qualifier;
            qualifier.playerIndex = allScores[i].index;
            qualifier.score = allScores[i].score;
            qualifier.initials[0] = 'A';
            qualifier.initials[1] = 'A';
            qualifier.initials[2] = 'A';
            outQualifiers.push_back(qualifier);
        }
        count++;
    }
    
    // Sort qualifiers by player index so we ask P1 through P4 in order
    std::sort(outQualifiers.begin(), outQualifiers.end(),
        [](const GameEndQualifier& a, const GameEndQualifier& b) {
            return a.playerIndex < b.playerIndex;
        });
}

// ========================================================================
// PBTBL_GAMEEND: Render Function
// ========================================================================

bool PBEngine::pbeRenderGameEnd(unsigned long currentTick, unsigned long lastTick){
    
    if (!pbeLoadGameEnd()) {
        pbeSendConsole("ERROR: Failed to load game end screen resources");
        return (false);
    }
    
    PBTBLGameEndState currentState = static_cast<PBTBLGameEndState>(m_tableSubScreenState);
    
    // Handle initialization on first render
    if (!m_gameEndInitialized) {
        // Settle all score animations so display is static
        for (int i = 0; i < 4; i++) {
            m_playerStates[i].inProgressScore = m_playerStates[i].score;
            m_playerStates[i].previousScore = m_playerStates[i].score;
            m_playerStates[i].scoreUpdateStartTick = 0;
        }
        m_mainScoreAnimActive = false;
        for (int i = 0; i < 3; i++) {
            m_secondaryScoreAnims[i].animationActive = false;
            m_secondaryScoreAnims[i].currentYOffset = 0;
        }
        
        // Determine which players qualify for the high score board
        determineHighScoreQualifiers(m_playerStates, m_saveFileData.highScores, m_gameEndQualifiers);
        
        m_gameEndCurrentQualifierIdx = 0;
        m_gameEndActiveLetterPos = 0;
        m_gameEndInitialized = true;
        
        // Transition to appropriate sub-state
        if (m_gameEndQualifiers.empty()) {
            // No players qualified - show "Game Over" for 10 seconds then return to start
            m_tableSubScreenState = static_cast<int>(PBTBLGameEndState::GAMEEND_COMPLETE);
            currentState = PBTBLGameEndState::GAMEEND_COMPLETE;
            pbeSetTimer(GAMEEND_COMPLETE_TIMER_ID, 10000);
        } else {
            // Set current player to first qualifier for proper score display
            m_currentPlayer = m_gameEndQualifiers[0].playerIndex;
            m_tableSubScreenState = static_cast<int>(PBTBLGameEndState::GAMEEND_ENTERINITIALS);
            currentState = PBTBLGameEndState::GAMEEND_ENTERINITIALS;
        }
    }
    
    // Clear to black and render right-panel status icons and other player scores at the bottom.
    // Ball indicator and status message text are suppressed since the game is over.
    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
    pbeRenderPlayerScores(currentTick, lastTick, false);
    pbeRenderStatus(currentTick, lastTick);
    
    // Render mode-specific content in the center area
    switch (currentState) {
        case PBTBLGameEndState::GAMEEND_ENTERINITIALS: {
            if (m_gameEndCurrentQualifierIdx < (int)m_gameEndQualifiers.size()) {
                GameEndQualifier& qualifier = m_gameEndQualifiers[m_gameEndCurrentQualifierIdx];
                
                // Display "Player X - New High Score!" label, pushed up from normal position
                gfxSetColor(m_StartMenuFontId, 255, 215, 0, 255); // Gold
                gfxSetScaleFactor(m_StartMenuFontId, 0.5, false);
                std::string playerLabel = "Player " + std::to_string(qualifier.playerIndex + 1) + " - New High Score!";
                gfxRenderShadowString(m_StartMenuFontId, playerLabel, 
                    (ACTIVEDISPX + (1024/3)), ACTIVEDISPY + 250, 5, GFX_TEXTCENTER,
                    0, 0, 0, 255, 2);
                
                // Display the score in large font, pushed up from normal center position
                gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
                gfxSetScaleFactor(m_StartMenuFontId, 1.2, false);
                std::string scoreText = formatScoreWithCommas(qualifier.score);
                gfxRenderShadowString(m_StartMenuFontId, scoreText, 
                    (ACTIVEDISPX + (1024/3)), ACTIVEDISPY + 320, 5, GFX_TEXTCENTER,
                    0, 0, 0, 255, 2);
                
                // Display "Enter Initials" prompt
                gfxSetColor(m_StartMenuFontId, 200, 200, 200, 255); // Light grey
                gfxSetScaleFactor(m_StartMenuFontId, 0.4, false);
                gfxRenderString(m_StartMenuFontId, "Enter Initials", 
                    (ACTIVEDISPX + (1024/3)), ACTIVEDISPY + 420, 3, GFX_TEXTCENTER);
                
                // Render three large initials with spacing
                // Active letter blinks red (1s on, 0.5s off); inactive letters are gold
                int initialsBaseX = ACTIVEDISPX + (1024/3);
                int initialsY = ACTIVEDISPY + 490;
                int letterSpacing = 100; // Pixels between letter centers
                
                // Blink cycle: 1500ms total (1000ms on, 500ms off)
                bool activeLetterVisible = (currentTick % 1500) < 1000;
                
                gfxSetScaleFactor(m_StartMenuFontId, 1.5, false);
                
                for (int i = 0; i < 3; i++) {
                    int letterX = initialsBaseX + ((i - 1) * letterSpacing);
                    
                    if (i == m_gameEndActiveLetterPos) {
                        // Active letter - red, blinking
                        if (!activeLetterVisible) continue;
                        gfxSetColor(m_StartMenuFontId, 255, 40, 40, 255);
                    } else {
                        // Inactive letter - gold, always visible
                        gfxSetColor(m_StartMenuFontId, 235, 176, 20, 255);
                    }
                    
                    // Display space as '_' so the user can see it
                    char displayChar = (qualifier.initials[i] == ' ') ? '_' : qualifier.initials[i];
                    std::string letter(1, displayChar);
                    gfxRenderShadowString(m_StartMenuFontId, letter, letterX, initialsY, 5, GFX_TEXTCENTER,
                        0, 0, 0, 255, 2);
                }
            }
            break;
        }
        
        case PBTBLGameEndState::GAMEEND_COMPLETE: {
            // Render "Game Over" title
            gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
            gfxSetScaleFactor(m_StartMenuFontId, 1.0, false);
            gfxRenderShadowString(m_StartMenuFontId, "Game Over",
                (ACTIVEDISPX + (1024/3)), ACTIVEDISPY + 200, 5, GFX_TEXTCENTER,
                0, 0, 0, 255, 3);

            // Shimmer offset cycles 1→2→3→4→5→4→3→2 (repeating every 1600ms)
            int shimStep = static_cast<int>((currentTick / 200) % 8);
            int shimOffset = (shimStep <= 4) ? (shimStep + 1) : (9 - shimStep);

            // Build a set of qualifying player indices for quick lookup
            bool isQualifier[4] = { false, false, false, false };
            for (size_t i = 0; i < m_gameEndQualifiers.size(); i++) {
                int idx = m_gameEndQualifiers[i].playerIndex;
                if (idx >= 0 && idx < 4) isQualifier[idx] = true;
            }

            // Display all player scores in medium white text, format "PX: score"
            gfxSetScaleFactor(m_StartMenuFontId, 0.7, false);
            int scoreY = ACTIVEDISPY + 320;
            int scoreLineSpacing = 70;
            for (int i = 0; i < 4; i++) {
                if (m_playerStates[i].score > 0 || m_playerStates[i].inGame) {
                    std::string scoreStr = "P" + std::to_string(i + 1) + ": " + formatScoreWithCommas(m_playerStates[i].score);
                    if (isQualifier[i]) {
                        // High score qualifier: bright red shadow with shimmer effect
                        gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
                        gfxRenderShadowString(m_StartMenuFontId, scoreStr,
                            (ACTIVEDISPX + (1024/3)), scoreY, 5, GFX_TEXTCENTER,
                            220, 0, 0, 255, shimOffset);
                    } else {
                        // Normal player: plain white text
                        gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
                        gfxRenderShadowString(m_StartMenuFontId, scoreStr,
                            (ACTIVEDISPX + (1024/3)), scoreY, 5, GFX_TEXTCENTER,
                            0, 0, 0, 255, 2);
                    }
                    scoreY += scoreLineSpacing;
                }
            }
            break;
        }
        
        default:
            break;
    }
    
    // Render the main screen divider bars on top (same as main mode)
    gfxRenderSprite(m_PBTBLMainScreenBGId, ACTIVEDISPX, ACTIVEDISPY);
    
    return (true);
}

// ========================================================================
// PBTBL_GAMEEND: State Update Function
// ========================================================================

void PBEngine::pbeUpdateStateGameEnd(stInputMessage inputMessage){
    PBTBLGameEndState currentState = static_cast<PBTBLGameEndState>(m_tableSubScreenState);
    
    switch (currentState) {
        case PBTBLGameEndState::GAMEEND_ENTERINITIALS: {
            if (m_gameEndCurrentQualifierIdx >= (int)m_gameEndQualifiers.size()) {
                // All qualifiers done, should not happen but handle gracefully
                m_tableSubScreenState = static_cast<int>(PBTBLGameEndState::GAMEEND_COMPLETE);
                break;
            }
            
            GameEndQualifier& qualifier = m_gameEndQualifiers[m_gameEndCurrentQualifierIdx];
            
            // Valid characters for initials: space, A-Z, 0-9
            static const char INITIALS_CHARS[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
            static const int INITIALS_CHARS_COUNT = sizeof(INITIALS_CHARS) - 1;
            
            if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
                // Left/right flipper: cycle the active letter through the character set
                if (inputMessage.inputId == IDI_LFLIP || inputMessage.inputId == IDI_RFLIP) {
                    char& letter = qualifier.initials[m_gameEndActiveLetterPos];
                    int idx = 1; // default to 'A' if current char not found
                    for (int c = 0; c < INITIALS_CHARS_COUNT; c++) {
                        if (INITIALS_CHARS[c] == letter) { idx = c; break; }
                    }
                    // Left flipper: cycle DOWN (space wraps to 9); right flipper: cycle UP (9 wraps to space)
                    if (inputMessage.inputId == IDI_LFLIP) {
                        letter = INITIALS_CHARS[(idx - 1 + INITIALS_CHARS_COUNT) % INITIALS_CHARS_COUNT];
                    } else {
                        letter = INITIALS_CHARS[(idx + 1) % INITIALS_CHARS_COUNT];
                    }
                    m_soundSystem.pbsPlayEffect(SOUNDCLICK);
                }
                // Left activate: move cursor left (stay at leftmost if already there)
                else if (inputMessage.inputId == IDI_LACTIVATE) {
                    if (m_gameEndActiveLetterPos > 0) {
                        m_gameEndActiveLetterPos--;
                        m_soundSystem.pbsPlayEffect(SOUNDCLICK);
                    }
                }
                // Right activate: move cursor right, or complete entry if at rightmost
                else if (inputMessage.inputId == IDI_RACTIVATE) {
                    if (m_gameEndActiveLetterPos < 2) {
                        // Move to next letter position
                        m_gameEndActiveLetterPos++;
                        m_soundSystem.pbsPlayEffect(SOUNDCLICK);
                    } else {
                        // At rightmost position - entry is complete
                        m_soundSystem.pbsPlayEffect(SOUNDSWORDCUT);
                        
                        // Move to next qualifying player
                        m_gameEndCurrentQualifierIdx++;
                        m_gameEndActiveLetterPos = 0;
                        
                        if (m_gameEndCurrentQualifierIdx < (int)m_gameEndQualifiers.size()) {
                            // Set current player to next qualifier for proper score display
                            m_currentPlayer = m_gameEndQualifiers[m_gameEndCurrentQualifierIdx].playerIndex;
                        } else {
                            // All players have entered initials - update high score board
                            // Build combined list of existing high scores + new entries
                            std::vector<stHighScoreData> newScores;
                            
                            // Add existing high scores
                            for (int i = 0; i < NUM_HIGHSCORES; i++) {
                                newScores.push_back(m_saveFileData.highScores[i]);
                            }
                            
                            // Add qualifying player entries
                            for (size_t i = 0; i < m_gameEndQualifiers.size(); i++) {
                                stHighScoreData entry;
                                entry.highScore = m_gameEndQualifiers[i].score;
                                strncpy(entry.playerInitials, m_gameEndQualifiers[i].initials, 3);
                                entry.playerInitials[3] = '\0';
                                newScores.push_back(entry);
                            }
                            
                            // Sort by score descending
                            std::sort(newScores.begin(), newScores.end(),
                                [](const stHighScoreData& a, const stHighScoreData& b) {
                                    return a.highScore > b.highScore;
                                });
                            
                            // Take top NUM_HIGHSCORES and update save data
                            for (int i = 0; i < NUM_HIGHSCORES && i < (int)newScores.size(); i++) {
                                m_saveFileData.highScores[i] = newScores[i];
                            }
                            
                            // Save to file
                            pbeSaveFile();
                            
                            // All initials entered - show game over screen for 10 seconds
                            // Keep m_gameEndQualifiers so the render function can apply shimmer
                            m_tableSubScreenState = static_cast<int>(PBTBLGameEndState::GAMEEND_COMPLETE);
                            pbeSetTimer(GAMEEND_COMPLETE_TIMER_ID, 10000);
                        }
                    }
                }
            }
            break;
        }
        
        case PBTBLGameEndState::GAMEEND_COMPLETE: {
            // 10-second timer or any button press returns to start screen
            bool timerFired = (inputMessage.inputMsg == PB_IMSG_TIMER &&
                               inputMessage.inputId == GAMEEND_COMPLETE_TIMER_ID);
            bool buttonPressed = (inputMessage.inputMsg == PB_IMSG_BUTTON &&
                                  inputMessage.inputState == PB_ON);
            if (timerFired || buttonPressed) {
                // Clean up and return to start screen
                m_gameEndInitialized = false;
                m_gameEndQualifiers.clear();
                for (int i = 0; i < 4; i++) {
                    m_playerStates[i].reset(m_saveFileData.ballsPerGame);
                    m_playerStates[i].enabled = false;
                }
                m_tableState = PBTableState::PBTBL_START;
                m_tableSubScreenState = static_cast<int>(PBTBLStartScreenState::START_START);
                m_RestartTable = true;
                pbeClearScreenRequests();
                m_soundSystem.pbsStopAllEffects();
                m_soundSystem.pbsStopMusic();
            }
            break;
        }
        
        default:
            break;
    }
}

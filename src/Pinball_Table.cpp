// Pinball_Table.cpp:  Class functions and code for the specific pinball table

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "Pinball_Engine.h"
#include "Pinball_Table.h"
#include "Pinball_TableStr.h"
#include "PBSequences.h"
#include "PBDevice.h"

// PBEgine Class Fucntions for the main pinball game

bool PBEngine::pbeLoadGameStart(){
    
    if (m_gameStartLoaded) return (true);

    stAnimateData animateData;

    m_PBTBLBackglassId = gfxLoadSprite("Backglass", "src/resources/textures/Backglass.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLBackglassId, 255, 255, 255, 255);

    m_PBTBLStartDoorId = gfxLoadSprite("OpenDoor", "src/resources/textures/startdooropen2.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLStartDoorId, 255, 255, 255, 255);

    // Load doors and set them up to slide left and right
    m_PBTBLLeftDoorId = gfxLoadSprite("LeftDoor", "src/resources/textures/DoorLeft2.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    m_PBTBLeftDoorStartId = gfxInstanceSprite(m_PBTBLLeftDoorId);
    gfxSetXY(m_PBTBLeftDoorStartId, ACTIVEDISPX + 315, ACTIVEDISPY + 112, false); 
    m_PBTBLeftDoorEndId = gfxInstanceSprite(m_PBTBLLeftDoorId);
    gfxSetXY(m_PBTBLeftDoorEndId, ACTIVEDISPX + 90, ACTIVEDISPY + 112, false); 
    // Left door moves left with acceleration, starting slowly and slamming open
    gfxLoadAnimateData(&animateData, m_PBTBLLeftDoorId, m_PBTBLeftDoorStartId, m_PBTBLeftDoorEndId, 
                       ANIMATE_X_MASK, 1.25f, false, GFX_NOLOOP, GFX_ANIM_ACCL,
                       0, -400.0f, 0.0f, 0.0f, 0.0f, true, -50.0f, 0.0f, 0.0f);
    gfxCreateAnimation(animateData, true);

    m_PBTBLRightDoorId = gfxLoadSprite("RightDoor", "src/resources/textures/DoorRight2.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    m_PBTBRightDoorStartId = gfxInstanceSprite(m_PBTBLRightDoorId);
    gfxSetXY(m_PBTBRightDoorStartId, ACTIVEDISPX + 460, ACTIVEDISPY + 112, false);
    m_PBTBRightDoorEndId = gfxInstanceSprite(m_PBTBLRightDoorId);
    gfxSetXY(m_PBTBRightDoorEndId, ACTIVEDISPX + 754, ACTIVEDISPY + 112, false);
    // Right door moves right with acceleration, starting slowly and slamming open
    gfxLoadAnimateData(&animateData, m_PBTBLRightDoorId, m_PBTBRightDoorStartId, m_PBTBRightDoorEndId, 
                       ANIMATE_X_MASK, 1.25f, false, GFX_NOLOOP, GFX_ANIM_ACCL,
                       0, 400.0f, 0.0f, 0.0f, 0.0f, true, 50.0f, 0.0f, 0.0f);
    gfxCreateAnimation(animateData, true);
    
    m_PBTBLDoorDragonId = gfxLoadSprite("DoorDragon", "src/resources/textures/Dragon.bmp", GFX_BMP, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetScaleFactor(m_PBTBLDoorDragonId, 0.8, false);
    m_PBTBLDragonEyesId = gfxLoadSprite("DragonEyes", "src/resources/textures/DragonEyes.bmp", GFX_BMP, GFX_NOMAP, GFX_UPPERLEFT, true, true);

    m_PBTBLDoorDungeonId = gfxLoadSprite("DoorDungeon", "src/resources/textures/Dungeon2.bmp", GFX_BMP, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetScaleFactor(m_PBTBLDoorDungeonId, 0.94, false);

    m_PBTBLFlame1Id = gfxLoadSprite("Flame1", "src/resources/textures/flame1.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, false, true);
    gfxSetColor(m_PBTBLFlame1Id, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame1Id, 1.0, false);

    m_PBTBLFlame1StartId = gfxInstanceSprite(m_PBTBLFlame1Id, -2, -2, 92, 255, 255, 255, 92, 1.05f, -4.0f);

    m_PBTBLFlame1EndId = gfxInstanceSprite(m_PBTBLFlame1Id, 2, 2, 92, 255, 255, 255, 92, 1.25f, 4.0f);

    gfxLoadAnimateData(&animateData, m_PBTBLFlame1Id, m_PBTBLFlame1StartId, m_PBTBLFlame1EndId, 
                       ANIMATE_SCALE_MASK | ANIMATE_X_MASK | ANIMATE_Y_MASK | ANIMATE_ROTATE_MASK, 0.1f, true, GFX_RESTART, GFX_ANIM_JUMPRANDOM,
                       0, 0.0f, 0.0f, 0.0f, 0.6f, true, 0.0f, 0.0f, 0.0f);
    gfxCreateAnimation(animateData, true);

    m_PBTBLFlame2Id = gfxLoadSprite("Flame2", "src/resources/textures/flame2.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, false, true);
    gfxSetColor(m_PBTBLFlame2Id, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame2Id, 1.15, false);

    m_PBTBLFlame2StartId = gfxInstanceSprite(m_PBTBLFlame2Id, -2, -2, 92, 255, 255, 255, 92, 1.05f, -4.0f);

    m_PBTBLFlame2EndId = gfxInstanceSprite(m_PBTBLFlame2Id, 2, 2, 92, 255, 255, 255, 92, 1.25f, 4.0f);

    gfxLoadAnimateData(&animateData, m_PBTBLFlame2Id, m_PBTBLFlame2StartId, m_PBTBLFlame2EndId, 
                       ANIMATE_SCALE_MASK | ANIMATE_X_MASK | ANIMATE_Y_MASK | ANIMATE_ROTATE_MASK, 0.1f, true, GFX_RESTART, GFX_ANIM_JUMPRANDOM,
                       0, 0.0f, 0.0f, 0.0f, 0.6f, true, 0.0f, 0.0f, 0.0f);
    gfxCreateAnimation(animateData, true);
    
    m_PBTBLFlame3Id = gfxLoadSprite("Flame3", "src/resources/textures/flame3.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, false, true);
    gfxSetColor(m_PBTBLFlame3Id, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame3Id, 1.15, false);

    m_PBTBLFlame3StartId = gfxInstanceSprite(m_PBTBLFlame3Id, -2, -2, 92, 255, 255, 255, 92, 1.05f, -4.0f);
    
    m_PBTBLFlame3EndId = gfxInstanceSprite(m_PBTBLFlame3Id, 2, 2, 92, 255, 255, 255, 92, 1.25f, 4.0f);

    gfxLoadAnimateData(&animateData, m_PBTBLFlame3Id, m_PBTBLFlame3StartId, m_PBTBLFlame3EndId, 
                       ANIMATE_SCALE_MASK | ANIMATE_X_MASK | ANIMATE_Y_MASK | ANIMATE_ROTATE_MASK, 0.1f, true, GFX_RESTART, GFX_ANIM_JUMPRANDOM,
                       0, 0.0f, 0.0f, 0.0f, 0.6f, true, 0.0f, 0.0f, 0.0f);
    gfxCreateAnimation(animateData, true);

    // Create the fade animation instances for the text
    m_PBTBLTextStartId = gfxInstanceSprite(m_StartMenuFontId);
    gfxSetColor(m_PBTBLTextStartId, 0, 0, 0, 0);
    m_PBTBLTextEndId = gfxInstanceSprite(m_StartMenuFontId);
    gfxSetColor(m_PBTBLTextEndId, 255, 255, 255, 255);
    gfxLoadAnimateDataShort(&animateData, m_StartMenuFontId, m_PBTBLTextStartId, m_PBTBLTextEndId, ANIMATE_COLOR_MASK, 2.0f, true, GFX_NOLOOP, GFX_ANIM_NORMAL);
    gfxCreateAnimation(animateData, true);

    // Note:  So many things to check for loading, it's not worth doing.  Assume the sprites will be loaded.  If texture fails, it will just render incorrectly.

    // Initialize and register the ball ejector device (using example IDs - can be configured per table)
    pbdEjector* ballEjector = new pbdEjector(this, IDI_SENSOR1, IDO_LED1, IDO_BALLEJECT);
    pbeAddDevice(ballEjector);

    m_gameStartLoaded = true;
    return (true);
}

bool PBEngine::pbeRenderGameStart(unsigned long currentTick, unsigned long lastTick){

    static int timeoutTicks, blinkCountTicks;
    static PBTBLScreenState lastScreenState;
    static bool blinkOn;

    if (m_RestartTable) {
        m_RestartTable = false;
        timeoutTicks = 18000;
        blinkCountTicks = 1000;
        blinkOn = true;
        m_PBTBLOpenDoors = false;
        m_PBTBLStartDoorsDone = false;
        m_tableScreenState = PBTBLScreenState::START_START;
        lastScreenState = m_tableScreenState;
    }

    if (!pbeLoadGameStart()) return (false); 
    
    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);

    // Hide the Dungeon behind the door
    gfxRenderSprite(m_PBTBLDoorDungeonId, ACTIVEDISPX+50, ACTIVEDISPY+60);

    // Show the door images - animate if it's opening...
    if (!m_PBTBLOpenDoors) {
        gfxRenderSprite(m_PBTBLLeftDoorId, ACTIVEDISPX + 315, ACTIVEDISPY + 112);
        gfxRenderSprite(m_PBTBLRightDoorId, ACTIVEDISPX + 460, ACTIVEDISPY + 112);
    }
    else{
        gfxAnimateSprite(m_PBTBLLeftDoorId, currentTick);
        gfxRenderSprite(m_PBTBLLeftDoorId);
        gfxAnimateSprite(m_PBTBLRightDoorId, currentTick);
        gfxRenderSprite(m_PBTBLRightDoorId);
    }

    gfxRenderSprite(m_PBTBLStartDoorId, ACTIVEDISPX, ACTIVEDISPY);
    
    gfxSetColor(m_StartMenuFontId, 255 ,165, 0, 255);
    gfxSetScaleFactor(m_StartMenuFontId, 1.25, false);
    
    gfxAnimateSprite(m_PBTBLFlame1Id, currentTick);
    gfxAnimateSprite(m_PBTBLFlame2Id, currentTick);
    gfxAnimateSprite(m_PBTBLFlame3Id, currentTick);

    gfxRenderSprite(m_PBTBLFlame1Id, ACTIVEDISPX + 225, ACTIVEDISPY + 392);
    gfxRenderSprite(m_PBTBLFlame2Id, ACTIVEDISPX + 225, ACTIVEDISPY + 392);
    gfxRenderSprite(m_PBTBLFlame3Id, ACTIVEDISPX + 225, ACTIVEDISPY + 392);

    gfxRenderSprite(m_PBTBLFlame1Id, ACTIVEDISPX + 852, ACTIVEDISPY + 392);
    gfxRenderSprite(m_PBTBLFlame2Id, ACTIVEDISPX + 852, ACTIVEDISPY + 392);
    gfxRenderSprite(m_PBTBLFlame3Id, ACTIVEDISPX + 852, ACTIVEDISPY + 392);

    if (lastScreenState != m_tableScreenState) {
        timeoutTicks = 18000; // Reset the timeout if we change screens
        lastScreenState = m_tableScreenState;
        gfxAnimateRestart(m_StartMenuFontId);

        // Return early if we've restarted the animation here, we'll catch the text on the next pass
        return (true);
    }

    switch (m_tableScreenState) {
        case PBTBLScreenState::START_START:
            gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
            gfxAnimateSprite(m_StartMenuFontId, currentTick);
            gfxSetScaleFactor(m_StartMenuFontId, 0.9, false);
            if (blinkCountTicks <= 0) {
                blinkOn = !blinkOn; 
                if (blinkOn) blinkCountTicks = 2000;
                else blinkCountTicks = 500;
            } else {
                blinkCountTicks -= (currentTick - lastTick);
            }
            if (blinkOn) 
            {
                if (gfxAnimateActive(m_StartMenuFontId)) gfxRenderString(m_StartMenuFontId, "Press Start", (PB_SCREENWIDTH/2) + 30, ACTIVEDISPY + 250, 1, GFX_TEXTCENTER);
                else gfxRenderShadowString(m_StartMenuFontId, "Press Start", (PB_SCREENWIDTH/2) + 30, ACTIVEDISPY + 250, 1, GFX_TEXTCENTER, 0, 0, 0, 255, 3);
            }
            break;

        case PBTBLScreenState::START_INST:
            gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
            gfxAnimateSprite(m_StartMenuFontId, currentTick);
            gfxSetScaleFactor(m_StartMenuFontId, 0.7, false);
            // loop through PBTableInst and print each string
            for (int i = 0; i < PBTABLEINSTSIZE; i++) {
                if (i == 0) {
                    if (gfxAnimateActive(m_StartMenuFontId)) gfxRenderString(m_StartMenuFontId, PBTableInst[i], (PB_SCREENWIDTH/2) + 20, ACTIVEDISPY + 105, 2, GFX_TEXTCENTER);
                    else gfxRenderShadowString (m_StartMenuFontId, PBTableInst[i], (PB_SCREENWIDTH/2) + 20, ACTIVEDISPY + 105, 2, GFX_TEXTCENTER, 0, 0, 0, 255, 3);
                }
                else {
                    if (gfxAnimateActive(m_StartMenuFontId)) gfxRenderString(m_StartMenuFontId, PBTableInst[i],  (PB_SCREENWIDTH/2) - 400, ACTIVEDISPY + 120 + (i * 65), 2, GFX_TEXTLEFT);
                    else gfxRenderShadowString(m_StartMenuFontId, PBTableInst[i], (PB_SCREENWIDTH/2) - 400, ACTIVEDISPY + 120 + (i * 65), 2, GFX_TEXTLEFT, 0, 0, 0, 255, 3); 
                }
                    
            }
            break;
            
        case PBTBLScreenState::START_SCORES:
            gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
            gfxAnimateSprite(m_StartMenuFontId, currentTick);
            gfxSetScaleFactor(m_StartMenuFontId, 0.8, false);

            for (int i = 0; i < NUM_HIGHSCORES + 1; i++) {
                std::string scoreText = std::to_string(m_saveFileData.highScores[i-1].highScore);

                if (i == 0){
                    if (gfxAnimateActive(m_StartMenuFontId)) gfxRenderString(m_StartMenuFontId, "High Scores", (PB_SCREENWIDTH/2) + 20, ACTIVEDISPY + 120, 10, GFX_TEXTCENTER);
                    else gfxRenderShadowString(m_StartMenuFontId, "High Scores", (PB_SCREENWIDTH/2) +  20, ACTIVEDISPY + 120, 10, GFX_TEXTCENTER, 0, 0, 0, 255, 3);
                }
                else if (gfxAnimateActive(m_StartMenuFontId)) 
                {
                    gfxRenderString(m_StartMenuFontId, m_saveFileData.highScores[i-1].playerInitials, (PB_SCREENWIDTH/2) - 220, ACTIVEDISPY + 140 + (i * 75), 10, GFX_TEXTLEFT);
                    gfxRenderString(m_StartMenuFontId, scoreText, (PB_SCREENWIDTH/2) + 220, ACTIVEDISPY + 140 + (i * 75), 3, GFX_TEXTRIGHT);
                }
                else {
                    gfxRenderShadowString(m_StartMenuFontId, m_saveFileData.highScores[i-1].playerInitials, (PB_SCREENWIDTH/2) - 220, ACTIVEDISPY + 140 + (i * 75), 10, GFX_TEXTLEFT, 0, 0, 0, 255, 3);
                    gfxRenderShadowString(m_StartMenuFontId, scoreText, (PB_SCREENWIDTH/2) + 220, ACTIVEDISPY + 140 + (i * 75), 3, GFX_TEXTRIGHT, 0, 0, 0, 255, 3);
                }
            }
            break;

            case PBTBLScreenState::START_OPENDOOR:

                if (!m_PBTBLOpenDoors){
                    gfxAnimateRestart(m_PBTBLLeftDoorId);
                    gfxAnimateRestart(m_PBTBLRightDoorId);
                    m_PBTBLOpenDoors = true;
                }
                
                // Check if door animations are complete and transition to main screen
                if ((!gfxAnimateActive(m_PBTBLLeftDoorId)) && (!gfxAnimateActive(m_PBTBLRightDoorId))) {
                    // Transition to main screen and enable player 1 automatically
                    m_tableState = PBTableState::PBTBL_MAINSCREEN;
                    m_currentPlayer = 0;
                    m_playerStates[0].reset(m_saveFileData.ballsPerGame);
                    m_playerStates[0].enabled = true;
                    // Make sure other players are disabled
                    for (int i = 1; i < 4; i++) {
                        m_playerStates[i].reset(m_saveFileData.ballsPerGame);
                        m_playerStates[i].enabled = false;
                    }
                    
                    // Initialize main score fade-in animation
                    m_mainScoreAnimStartTick = currentTick;
                    m_mainScoreAnimActive = true;
                    
                    // Reset all secondary score slot animations
                    for (int i = 0; i < 3; i++) {
                        m_secondaryScoreAnims[i].reset();
                    }
                }
            break;

        default:
            m_tableScreenState = PBTBLScreenState::START_START;
        break;
    }

    // If timeout happens, switch back to "Press Start"
    if ((timeoutTicks > 0) && (m_tableScreenState != PBTBLScreenState::START_START) && (m_tableScreenState != PBTBLScreenState::START_OPENDOOR)) {
        timeoutTicks -= (currentTick - lastTick);
        if (timeoutTicks <= 0) {
            m_tableScreenState = PBTBLScreenState::START_START;
        }
    }

    return (true);
}

bool PBEngine::pbeLoadMainScreen(){
    
    if (m_mainScreenLoaded) return (true);

    // Main screen uses the same font as the start menu
    // The font should already be loaded from pbeLoadGameStart
    
    // Load the main screen background
    m_PBTBLMainScreenBGId = gfxLoadSprite("MainScreenBG", "src/resources/textures/MainScreenBG.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLMainScreenBGId, 255, 255, 255, 255);
    
    m_mainScreenLoaded = true;
    return (true);
}

bool PBEngine::pbeLoadReset(){
    
    if (m_resetLoaded) return (true);
    
    // Reset screen uses the same font as the start menu
    // The font should already be loaded from pbeLoadGameStart
    
    // Create a solid color sprite (like the title bar in console state)
    m_PBTBLResetSpriteId = gfxLoadSprite("ResetBG", "", GFX_NONE, GFX_NOMAP, GFX_UPPERLEFT, false, false);
    gfxSetColor(m_PBTBLResetSpriteId, 0, 0, 0, 255); // Solid black
    gfxSetWH(m_PBTBLResetSpriteId, 700, 200); // Set width and height
    
    if (m_PBTBLResetSpriteId == NOSPRITE) return (false);
    
    m_resetLoaded = true;
    return (true);
}

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
    m_soundSystem.pbsPlayEffect(EFFECTCLICK);
    
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
    return m_playerStates[m_currentPlayer].screenState;
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

void PBEngine::pbeRenderPlayerScores(unsigned long currentTick, unsigned long lastTick){
    // Calculate fade-in alpha for main score
    unsigned int mainAlpha = 255;
    if (m_mainScoreAnimActive) {
        float elapsedSec = (currentTick - m_mainScoreAnimStartTick) / 1000.0f;
        float animDuration = 1.5f;
        
        if (elapsedSec >= animDuration) {
            m_mainScoreAnimActive = false;
            mainAlpha = 255;
        } else {
            // Linear fade from 0 to 255
            float progress = elapsedSec / animDuration;
            mainAlpha = static_cast<unsigned int>(progress * 255.0f);
        }
    }
    
    // Render the current player's score in the center (smaller white text)
    gfxSetColor(m_StartMenuFontId, 255, 255, 255, mainAlpha);
    gfxSetScaleFactor(m_StartMenuFontId, 0.8, false);
    
    // Render "Player X" label above the score
    std::string playerLabel = "Player " + std::to_string(m_currentPlayer + 1);
    gfxRenderString(m_StartMenuFontId, playerLabel, (PB_SCREENWIDTH/2), ACTIVEDISPY + 280, 5, GFX_TEXTCENTER);
    
    // Render the current player's score with commas
    std::string scoreText = formatScoreWithCommas(getCurrentPlayerScore());
    gfxSetScaleFactor(m_StartMenuFontId, 1.2, false);
    gfxRenderString(m_StartMenuFontId, scoreText, (PB_SCREENWIDTH/2), ACTIVEDISPY + 370, 5, GFX_TEXTCENTER);
    
    // Render other player scores at the bottom (small grey text)
    gfxSetColor(m_StartMenuFontId, 128, 128, 128, 255); // Light grey color for visibility
    gfxSetScaleFactor(m_StartMenuFontId, 0.375, false); // 25% smaller than 0.5
    
    // Render the other player scores at fixed positions based on thirds of active region
    // Active region is approximately 1100 pixels wide
    int activeWidth = 1024;
    int thirdWidth = activeWidth / 3;
    
    int leftX = ACTIVEDISPX + 10;                    // 10 pixels from start (far left)
    int middleX = ACTIVEDISPX + thirdWidth + 10;     // Start of 2nd third + 10 pixels
    int rightX = ACTIVEDISPX + (2 * thirdWidth) + 10; // Start of 3rd third + 10 pixels
    int positions[3] = {leftX, middleX, rightX};
    
    int slotIndex = 0;
    for (int i = 0; i < 4; i++) {
        // Skip current player and disabled players
        if (i == m_currentPlayer || !m_playerStates[i].enabled) continue;
        
        // Only render up to 3 secondary scores
        if (slotIndex >= 3) break;
        
        // Calculate Y offset for scroll-up animation
        int yOffset = 0;
        if (m_secondaryScoreAnims[slotIndex].animationActive) {
            float elapsedSec = (currentTick - m_secondaryScoreAnims[slotIndex].animStartTick) / 1000.0f;
            
            if (elapsedSec >= m_secondaryScoreAnims[slotIndex].animDurationSec) {
                m_secondaryScoreAnims[slotIndex].animationActive = false;
                m_secondaryScoreAnims[slotIndex].currentYOffset = 0;
            } else {
                // Linear interpolation from starting offset to 0
                float progress = elapsedSec / m_secondaryScoreAnims[slotIndex].animDurationSec;
                m_secondaryScoreAnims[slotIndex].currentYOffset = (int)(50 * (1.0f - progress));
            }
            
            yOffset = m_secondaryScoreAnims[slotIndex].currentYOffset;
        }
        
        // Render "PX: score" format, left-justified at the position
        std::string playerScoreText = "P" + std::to_string(i + 1) + ": " + formatScoreWithCommas(m_playerStates[i].score);
        gfxRenderString(m_StartMenuFontId, playerScoreText, positions[slotIndex], ACTIVEDISPY + 735 + yOffset, 3, GFX_TEXTLEFT);
        
        slotIndex++;
    }
}

bool PBEngine::pbeRenderMainScreen(unsigned long currentTick, unsigned long lastTick){
    
    if (!pbeLoadMainScreen()) return (false);
    
    // Clear to black background
    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
    
    // Render the main screen background
    gfxRenderSprite(m_PBTBLMainScreenBGId, ACTIVEDISPX, ACTIVEDISPY);
    
    // Render all player scores
    pbeRenderPlayerScores(currentTick, lastTick);
    
    // Render resource numbers on the right side
    // Position calculations based on the new horizontal layout
    // Icons are at x positions: 724, 809, 894 (from script output)
    // Resource icons are in bottom row at y=400 (center), so text goes below at around y=490
    int resourceIconX1 = ACTIVEDISPX + 724;  // Treasure chest
    int resourceIconX2 = ACTIVEDISPX + 809;  // Flaming sword  
    int resourceIconX3 = ACTIVEDISPX + 894;  // Shield
    int resourceTextY = ACTIVEDISPY + 490;   // Below the icons
    
    // Gold resource (treasure chest) - using golden color
    gfxSetColor(m_StartMenuFontId, 255, 215, 0, 255);
    gfxSetScaleFactor(m_StartMenuFontId, 0.6, false);
    gfxRenderString(m_StartMenuFontId, "999", resourceIconX1, resourceTextY, 3, GFX_TEXTCENTER);
    
    // Sword resource - using red/orange color
    gfxSetColor(m_StartMenuFontId, 255, 100, 50, 255);
    gfxSetScaleFactor(m_StartMenuFontId, 0.6, false);
    gfxRenderString(m_StartMenuFontId, "15", resourceIconX2, resourceTextY, 3, GFX_TEXTCENTER);
    
    // Shield resource - using bright blue color
    gfxSetColor(m_StartMenuFontId, 150, 200, 255, 255);
    gfxSetScaleFactor(m_StartMenuFontId, 0.6, false);
    gfxRenderString(m_StartMenuFontId, "8", resourceIconX3, resourceTextY, 3, GFX_TEXTCENTER);
    
    
    return (true);
}

bool PBEngine::pbeRenderReset(unsigned long currentTick, unsigned long lastTick){
    
    if (!pbeLoadReset()) return (false);
    
    // Center position in active area
    // fix cernter x to use full screen width
    int centerX = (PB_SCREENWIDTH / 2);
    int centerY = ACTIVEDISPY + 384; // Center of active area (768 / 2)
    
    // Render the black background sprite centered
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

bool PBEngine::pbeRenderGameScreen(unsigned long currentTick, unsigned long lastTick){
    
    bool success = false;

    switch (m_tableState) {
        case PBTableState::PBTBL_START: success = pbeRenderGameStart(currentTick, lastTick); break;
        case PBTableState::PBTBL_MAINSCREEN: success = pbeRenderMainScreen(currentTick, lastTick); break;
        case PBTableState::PBTBL_RESET: success = pbeRenderReset(currentTick, lastTick); break;
        default: success = false; break;
    }

    // Render the backglass all the time (this will cover any other sprites and hide anything that's off the mini-screen)
    // It's got a transparent mini screen which wil show the mini render.
    gfxRenderSprite(m_PBTBLBackglassId, 0, 0);

    return (success);
}

// Main State Loop for Pinball Table

void PBEngine::pbeUpdateGameState(stInputMessage inputMessage){
    
    // Check for reset button press first, regardless of current state (unless already in reset)
    if (m_tableState != PBTableState::PBTBL_RESET) {
        if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON && inputMessage.inputId == IDI_RESET) {
            if (!m_ResetButtonPressed) {
                // First time reset is pressed - save current state and enter reset mode
                m_StateBeforeReset = m_tableState;
                m_ResetButtonPressed = true;
                m_tableState = PBTableState::PBTBL_RESET;
                return; // Exit early, we're now in reset state
            }
        }
    }
    
    switch (m_tableState) {
        case PBTableState::PBTBL_START: {

            // Handle button presses during start screen
            if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
                if (inputMessage.inputId == IDI_START) {
                    // Start button opens the doors
                    m_tableScreenState = PBTBLScreenState::START_OPENDOOR;
                }
                else {
                    // Other buttons cycle through info screens (only when doors aren't opening)
                    if (!m_PBTBLOpenDoors) {
                        switch (m_tableScreenState) {
                            case PBTBLScreenState::START_START: m_tableScreenState = PBTBLScreenState::START_INST; break;
                            case PBTBLScreenState::START_INST: m_tableScreenState = PBTBLScreenState::START_SCORES; break;
                            case PBTBLScreenState::START_SCORES: m_tableScreenState = PBTBLScreenState::START_START; break;
                            default: break;
                        }
                    }
                }             
            }
            break;
        }
        case PBTableState::PBTBL_MAINSCREEN: {
            // Handle start button press to add players
            if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
                if (inputMessage.inputId == IDI_START) {
                    pbeTryAddPlayer();
                }
                // Handle activate buttons to add score
                else if (inputMessage.inputId == IDI_LEFTACTIVATE) {
                    addPlayerScore(100);
                }
                else if (inputMessage.inputId == IDI_RIGHTACTIVATE) {
                    addPlayerScore(10000);
                }
            }
            break;
        }
        case PBTableState::PBTBL_STDPLAY: {
            // Check for input messages and update the game state
            int temp = 0;

            break;
        }
        case PBTableState::PBTBL_RESET: {
            // Handle reset state inputs
            if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
                if (inputMessage.inputId == IDI_RESET) {
                    // Reset pressed again - clean up and return to main menu
                    m_ResetButtonPressed = false;
                    m_GameStarted = false; // Reset game started flag
                    
                    // Reload all resources by resetting load states
                    pbeEngineReload();
                    pbeTableReload();
                    
                    m_mainState = PB_STARTMENU; // Return to main menu in Pinball_Engine
                    m_tableState = PBTableState::PBTBL_START; // Reset table state
                }
                else if (inputMessage.inputId == IDI_START || 
                         inputMessage.inputId == IDI_LEFTACTIVATE || 
                         inputMessage.inputId == IDI_RIGHTACTIVATE ||
                         inputMessage.inputId == IDI_LEFTFLIPPER ||
                         inputMessage.inputId == IDI_RIGHTFLIPPER) {
                    // Any other button pressed - cancel reset and return to previous state
                    m_ResetButtonPressed = false;
                    m_tableState = m_StateBeforeReset;
                }
            }
            break;
        }
        
        default: break;
    }
}

// Reload function to reset all table screen load states
void PBEngine::pbeTableReload() {
    m_gameStartLoaded = false;
    m_mainScreenLoaded = false;
    m_resetLoaded = false;
    m_RestartTable = true;    
}


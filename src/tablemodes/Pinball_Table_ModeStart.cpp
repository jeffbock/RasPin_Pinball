// Pinball_Table_ModeStart.cpp:  PBTBL_START mode implementation
//   Handles the attract/start screen with doors, instructions, high scores,
//   and the door-opening transition to gameplay.

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "Pinball_Engine.h"
#include "Pinball_Table.h"
#include "Pinball_TableStr.h"
#include "PBDevice.h"

// ========================================================================
// PBTBL_START: Load Function
// ========================================================================

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
                       0, -150.0f, 0.0f, 0.0f, 0.0f, true, -25.0f, 0.0f, 0.0f);
    gfxCreateAnimation(animateData, true);

    m_PBTBLRightDoorId = gfxLoadSprite("RightDoor", "src/resources/textures/DoorRight2.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    m_PBTBRightDoorStartId = gfxInstanceSprite(m_PBTBLRightDoorId);
    gfxSetXY(m_PBTBRightDoorStartId, ACTIVEDISPX + 460, ACTIVEDISPY + 112, false);
    m_PBTBRightDoorEndId = gfxInstanceSprite(m_PBTBLRightDoorId);
    gfxSetXY(m_PBTBRightDoorEndId, ACTIVEDISPX + 754, ACTIVEDISPY + 112, false);
    // Right door moves right with acceleration, starting slowly and slamming open
    gfxLoadAnimateData(&animateData, m_PBTBLRightDoorId, m_PBTBRightDoorStartId, m_PBTBRightDoorEndId, 
                       ANIMATE_X_MASK, 1.25f, false, GFX_NOLOOP, GFX_ANIM_ACCL,
                       0, 150.0f, 0.0f, 0.0f, 0.0f, true, 25.0f, 0.0f, 0.0f);
    gfxCreateAnimation(animateData, true);

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

    m_gameStartLoaded = true;
    return (true);
}

// ========================================================================
// PBTBL_START: Render Function
// ========================================================================

bool PBEngine::pbeRenderGameStart(unsigned long currentTick, unsigned long lastTick){

    static int timeoutTicks, blinkCountTicks, torchId;
    static PBTBLStartScreenState lastScreenState;
    static bool blinkOn;

    if (m_RestartTable) {
        m_RestartTable = false;
        timeoutTicks = 18000;
        blinkCountTicks = 1000;
        blinkOn = true;
        m_PBTBLOpenDoors = false;
        m_PBTBLStartDoorsDone = false;
        m_tableSubScreenState = static_cast<int>(PBTBLStartScreenState::START_START);
        lastScreenState = static_cast<PBTBLStartScreenState>(m_tableSubScreenState);

        // Play torch sound loop and door theme music
        torchId = m_soundSystem.pbsPlayEffect(SOUNDTORCHES, true);
        m_soundSystem.pbsPlayMusic(SOUNDDOORTHEME);
    }

    if (!pbeLoadGameStart()) {
        pbeSendConsole("ERROR: Failed to load game start screen resources");
        return (false); 
    } 
    
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

    PBTBLStartScreenState currentStartState = static_cast<PBTBLStartScreenState>(m_tableSubScreenState);

    if (lastScreenState != currentStartState) {
        timeoutTicks = 18000; // Reset the timeout if we change screens
        lastScreenState = currentStartState;
        gfxAnimateRestart(m_StartMenuFontId);

        // Return early if we've restarted the animation here, we'll catch the text on the next pass
        return (true);
    }

    switch (currentStartState) {
        case PBTBLStartScreenState::START_START:
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

        case PBTBLStartScreenState::START_INST:
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
            
        case PBTBLStartScreenState::START_SCORES:
            gfxAnimateSprite(m_StartMenuFontId, currentTick);
            gfxSetScaleFactor(m_StartMenuFontId, 0.8, false);

            // Render Grand Champion section (first high score)
            {
                // "Grand Champion" title in gold
                if (gfxAnimateActive(m_StartMenuFontId)) {
                    gfxRenderString(m_StartMenuFontId, "Grand Champion", (PB_SCREENWIDTH/2) + 20, ACTIVEDISPY + 55, 10, GFX_TEXTCENTER);
                } else {
                    gfxSetColor(m_StartMenuFontId, 255, 215, 0, 255);
                    gfxRenderShadowString(m_StartMenuFontId, "Grand Champion", (PB_SCREENWIDTH/2) + 20, ACTIVEDISPY + 55, 10, GFX_TEXTCENTER, 0, 0, 0, 255, 3);
                }

                // Grand Champion initials in gold
                if (gfxAnimateActive(m_StartMenuFontId)) {
                    gfxRenderString(m_StartMenuFontId, m_saveFileData.highScores[0].playerInitials, (PB_SCREENWIDTH/2) + 20, ACTIVEDISPY + 125, 10, GFX_TEXTCENTER);
                } else {
                    gfxSetColor(m_StartMenuFontId, 255, 215, 0, 255);
                    gfxRenderShadowString(m_StartMenuFontId, m_saveFileData.highScores[0].playerInitials, (PB_SCREENWIDTH/2) + 20, ACTIVEDISPY + 125, 10, GFX_TEXTCENTER, 0, 0, 0, 255, 3);
                }
                
                // Grand Champion score on next line in gold
                std::string gcScoreText = std::to_string(m_saveFileData.highScores[0].highScore);
                if (gfxAnimateActive(m_StartMenuFontId)) {
                    gfxRenderString(m_StartMenuFontId, gcScoreText, (PB_SCREENWIDTH/2) + 20, ACTIVEDISPY + 190, 10, GFX_TEXTCENTER);
                } else {
                    gfxSetColor(m_StartMenuFontId, 255, 215, 0, 255);
                    gfxRenderShadowString(m_StartMenuFontId, gcScoreText, (PB_SCREENWIDTH/2) + 20, ACTIVEDISPY + 190, 10, GFX_TEXTCENTER, 0, 0, 0, 255, 3);
                }
            }

            // Render remaining high scores (2-10) with numbered format - reduced size to fit all entries
            gfxSetScaleFactor(m_StartMenuFontId, 0.55, false);
            for (int i = 1; i < NUM_HIGHSCORES; i++) {
                std::string scoreText = std::to_string(m_saveFileData.highScores[i].highScore);
                std::string rankText = std::to_string(i + 1) + ":";
                std::string initialsText = m_saveFileData.highScores[i].playerInitials;
                int yPos = ACTIVEDISPY + 280 + ((i - 1) * 50);

                if (gfxAnimateActive(m_StartMenuFontId)) {
                    gfxRenderString(m_StartMenuFontId, rankText, (PB_SCREENWIDTH/2) - 195, yPos, 10, GFX_TEXTRIGHT);
                    gfxRenderString(m_StartMenuFontId, initialsText, (PB_SCREENWIDTH/2) - 175, yPos, 10, GFX_TEXTLEFT);
                    gfxRenderString(m_StartMenuFontId, scoreText, (PB_SCREENWIDTH/2) + 220, yPos, 3, GFX_TEXTRIGHT);
                } else {
                    gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
                    gfxRenderShadowString(m_StartMenuFontId, rankText, (PB_SCREENWIDTH/2) - 195, yPos, 10, GFX_TEXTRIGHT, 0, 0, 0, 255, 3);
                    gfxRenderShadowString(m_StartMenuFontId, initialsText, (PB_SCREENWIDTH/2) - 175, yPos, 10, GFX_TEXTLEFT, 0, 0, 0, 255, 3);
                    gfxRenderShadowString(m_StartMenuFontId, scoreText, (PB_SCREENWIDTH/2) + 220, yPos, 3, GFX_TEXTRIGHT, 0, 0, 0, 255, 3);
                }
            }
            break;

            case PBTBLStartScreenState::START_OPENDOOR:

                if (!m_PBTBLOpenDoors){
                    gfxAnimateRestart(m_PBTBLLeftDoorId);
                    gfxAnimateRestart(m_PBTBLRightDoorId);
                    m_PBTBLOpenDoors = true;

                    // Play the door sound effect
                    m_soundSystem.pbsPlayEffect(SOUNDDOORCLOSE, false);
                }
                
                // Check if door animations are complete and transition to main screen
                if ((!gfxAnimateActive(m_PBTBLLeftDoorId)) && (!gfxAnimateActive(m_PBTBLRightDoorId))) {
                    // Transition to main screen and enable player 1 automatically
                    m_tableState = PBTableState::PBTBL_MAIN;
                    m_tableSubScreenState = static_cast<int>(PBTBLMainScreenState::MAIN_NORMAL);
                    m_currentPlayer = 0;
                    m_playerStates[0].reset(m_saveFileData.ballsPerGame);
                    m_playerStates[0].enabled = true;
                    m_playerStates[0].inGame  = true;
                    // Make sure other players are disabled
                    for (int i = 1; i < 4; i++) {
                        m_playerStates[i].reset(m_saveFileData.ballsPerGame);
                        m_playerStates[i].enabled = false;
                        m_playerStates[i].inGame  = false;
                    }
                    
                    // Initialize main score fade-in animation
                    m_mainScoreAnimStartTick = currentTick;
                    m_mainScoreAnimActive = true;
                    
                    // Reset all secondary score slot animations
                    for (int i = 0; i < 3; i++) {
                        m_secondaryScoreAnims[i].reset();
                    }
                    
                    // Initialize screen manager with default normal main screen (priority 0 = persistent)
                    pbeClearScreenRequests();
                    pbeRequestScreen(PBTableState::PBTBL_MAIN, static_cast<int>(PBTBLMainScreenState::MAIN_NORMAL),
                                     ScreenPriority::PRIORITY_LOW, 0, true);

                    // Enable auto-outputs (flippers / slings) and start first ball eject
                    SetAutoOutputEnable(true);
                    m_leftInlaneLEDOn  = false;
                    m_rightInlaneLEDOn = false;
                    if (m_hopperDevice) {
                        m_hopperDevice->pbdEnable(true);
                        m_hopperDevice->pdbStartRun();
                    }

                    // Stop the torch sound effect
                    m_soundSystem.pbsStopEffect(torchId);
                }
            break;

        default:
            m_tableSubScreenState = static_cast<int>(PBTBLStartScreenState::START_START);
        break;
    }

    // If timeout happens, switch back to "Press Start"
    currentStartState = static_cast<PBTBLStartScreenState>(m_tableSubScreenState);
    if ((timeoutTicks > 0) && (currentStartState != PBTBLStartScreenState::START_START) && (currentStartState != PBTBLStartScreenState::START_OPENDOOR)) {
        timeoutTicks -= (currentTick - lastTick);
        if (timeoutTicks <= 0) {
            m_tableSubScreenState = static_cast<int>(PBTBLStartScreenState::START_START);
        }
    }

    return (true);
}

// ========================================================================
// PBTBL_START: State Update Function
// ========================================================================

void PBEngine::pbeUpdateStateStart(stInputMessage inputMessage){
    // Handle button presses during start screen
    if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
        if (inputMessage.inputId == IDI_START) {
            // Start button opens the doors
            m_tableSubScreenState = static_cast<int>(PBTBLStartScreenState::START_OPENDOOR);
        }
        else {
            // Other buttons cycle through info screens (only when doors aren't opening)
            if (!m_PBTBLOpenDoors) {
                PBTBLStartScreenState currentStartState = static_cast<PBTBLStartScreenState>(m_tableSubScreenState);
                switch (currentStartState) {
                    case PBTBLStartScreenState::START_START: m_tableSubScreenState = static_cast<int>(PBTBLStartScreenState::START_INST); break;
                    case PBTBLStartScreenState::START_INST: m_tableSubScreenState = static_cast<int>(PBTBLStartScreenState::START_SCORES); break;
                    case PBTBLStartScreenState::START_SCORES: m_tableSubScreenState = static_cast<int>(PBTBLStartScreenState::START_START); break;
                    default: break;
                }
            }
        }             
    }
}

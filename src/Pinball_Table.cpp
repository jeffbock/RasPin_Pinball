// Pinball_Table.cpp:  Class functions and code for the specific pinball table

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "Pinball_Engine.h"
#include "Pinball_Table.h"
#include "Pinball_TableStr.h"
#include "PBSequences.h"
#include "PBDevice.h"
#include <algorithm>  // For std::sort in screen manager

// PBEgine Class Functions for the main pinball game

// Main load function for the pinball game start screen
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

// Renders the start screen w/ instructions, high scores and start prompt
bool PBEngine::pbeRenderGameStart(unsigned long currentTick, unsigned long lastTick){

    static int timeoutTicks, blinkCountTicks, torchId;
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
            gfxAnimateSprite(m_StartMenuFontId, currentTick);
            gfxSetScaleFactor(m_StartMenuFontId, 0.8, false);

            // Render Grand Champion section (first high score)
            {
                // "Grand Champion" title in gold
                if (gfxAnimateActive(m_StartMenuFontId)) {
                    gfxRenderString(m_StartMenuFontId, "Grand Champion", (PB_SCREENWIDTH/2) + 20, ACTIVEDISPY + 105, 10, GFX_TEXTCENTER);
                } else {
                    gfxSetColor(m_StartMenuFontId, 255, 215, 0, 255);
                    gfxRenderShadowString(m_StartMenuFontId, "Grand Champion", (PB_SCREENWIDTH/2) + 20, ACTIVEDISPY + 105, 10, GFX_TEXTCENTER, 0, 0, 0, 255, 3);
                }

                // Grand Champion initials in gold
                if (gfxAnimateActive(m_StartMenuFontId)) {
                    gfxRenderString(m_StartMenuFontId, m_saveFileData.highScores[0].playerInitials, (PB_SCREENWIDTH/2) + 20, ACTIVEDISPY + 175, 10, GFX_TEXTCENTER);
                } else {
                    gfxSetColor(m_StartMenuFontId, 255, 215, 0, 255);
                    gfxRenderShadowString(m_StartMenuFontId, m_saveFileData.highScores[0].playerInitials, (PB_SCREENWIDTH/2) + 20, ACTIVEDISPY + 175, 10, GFX_TEXTCENTER, 0, 0, 0, 255, 3);
                }
                
                // Grand Champion score on next line in gold
                std::string gcScoreText = std::to_string(m_saveFileData.highScores[0].highScore);
                if (gfxAnimateActive(m_StartMenuFontId)) {
                    gfxRenderString(m_StartMenuFontId, gcScoreText, (PB_SCREENWIDTH/2) + 20, ACTIVEDISPY + 240, 10, GFX_TEXTCENTER);
                } else {
                    gfxSetColor(m_StartMenuFontId, 255, 215, 0, 255);
                    gfxRenderShadowString(m_StartMenuFontId, gcScoreText, (PB_SCREENWIDTH/2) + 20, ACTIVEDISPY + 240, 10, GFX_TEXTCENTER, 0, 0, 0, 255, 3);
                }
            }

            // Render remaining high scores (2-5) with numbered format - more space after Grand Champion
            for (int i = 1; i < NUM_HIGHSCORES; i++) {
                std::string scoreText = std::to_string(m_saveFileData.highScores[i].highScore);
                std::string numberedInitials = std::to_string(i + 1) + ": " + m_saveFileData.highScores[i].playerInitials;
                int yPos = ACTIVEDISPY + 325 + ((i - 1) * 70);

                if (gfxAnimateActive(m_StartMenuFontId)) {
                    gfxRenderString(m_StartMenuFontId, numberedInitials, (PB_SCREENWIDTH/2) - 220, yPos, 10, GFX_TEXTLEFT);
                    gfxRenderString(m_StartMenuFontId, scoreText, (PB_SCREENWIDTH/2) + 220, yPos, 3, GFX_TEXTRIGHT);
                } else {
                    gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
                    gfxRenderShadowString(m_StartMenuFontId, numberedInitials, (PB_SCREENWIDTH/2) - 220, yPos, 10, GFX_TEXTLEFT, 0, 0, 0, 255, 3);
                    gfxRenderShadowString(m_StartMenuFontId, scoreText, (PB_SCREENWIDTH/2) + 220, yPos, 3, GFX_TEXTRIGHT, 0, 0, 0, 255, 3);
                }
            }
            break;

            case PBTBLScreenState::START_OPENDOOR:

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

                    // Stop the torch sound effect
                    m_soundSystem.pbsStopEffect(torchId);
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

// Load resources used by the main screen of the pinball game
bool PBEngine::pbeLoadMainScreen(){
    
    if (m_mainScreenLoaded) return (true);

    // Load the main screen background
    m_PBTBLMainScreenBGId = gfxLoadSprite("MainScreenBG", "src/resources/textures/MainScreenBG.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLMainScreenBGId, 255, 255, 255, 255);
    
    // Load the 256 sprites
    m_PBTBLCharacterCircle256Id = gfxLoadSprite("CharacterCircle256", "src/resources/textures/CharacterCircle256.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLCharacterCircle256Id, 255, 255, 255, 255);
    gfxSetScaleFactor(m_PBTBLCharacterCircle256Id, 0.6, false);
    
    m_PBTBLDungeon256Id = gfxLoadSprite("Dungeon256", "src/resources/textures/Dungeon256.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLDungeon256Id, 255, 255, 255, 255);
    gfxSetScaleFactor(m_PBTBLDungeon256Id, 0.42, false);
    
    m_PBTBLShield256Id = gfxLoadSprite("Shield256", "src/resources/textures/Shield256.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLShield256Id, 255, 255, 255, 255);
    gfxSetScaleFactor(m_PBTBLShield256Id, 0.42, false);
    
    m_PBTBLSword256Id = gfxLoadSprite("Sword256", "src/resources/textures/Sword256.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLSword256Id, 255, 255, 255, 255);
    gfxSetScaleFactor(m_PBTBLSword256Id, 0.42, false);
    
    m_PBTBLTreasure256Id = gfxLoadSprite("Treasure256", "src/resources/textures/Treasure256.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLTreasure256Id, 255, 255, 255, 255);
    gfxSetScaleFactor(m_PBTBLTreasure256Id, 0.42, false);
    
    // Load the headshot sprites
    m_PBTBLArcherHeadshot256Id = gfxLoadSprite("ArcherHeadshot256", "src/resources/textures/ArcherHeadshot256.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLArcherHeadshot256Id, 255, 255, 255, 255);
    gfxSetScaleFactor(m_PBTBLArcherHeadshot256Id, 0.5, false);
    
    m_PBTBLKnightHeadshot256Id = gfxLoadSprite("KnightHeadshot256", "src/resources/textures/KnightHeadshot256.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLKnightHeadshot256Id, 255, 255, 255, 255);
    gfxSetScaleFactor(m_PBTBLKnightHeadshot256Id, 0.5, false);
    
    m_PBTBLWolfHeadshot256Id = gfxLoadSprite("WolfHeadshot256", "src/resources/textures/WolfHeadshot256.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLWolfHeadshot256Id, 255, 255, 255, 255);
    gfxSetScaleFactor(m_PBTBLWolfHeadshot256Id, 0.5, false);
    
    // Start the main screen music
    m_soundSystem.pbsPlayMusic(SOUNDMAINTHEME);

    m_mainScreenLoaded = true;
    return (true);
}

// Organizes and calls all the sub-render routines for the main screen
bool PBEngine::pbeRenderMainScreen(unsigned long currentTick, unsigned long lastTick){
    
    if (!pbeLoadMainScreen()) {
        pbeSendConsole("ERROR: Failed to load main screen resources");
        return (false);
    }

    // Clear to black background
    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
    
    // Render the main screen background
    gfxRenderSprite(m_PBTBLMainScreenBGId, ACTIVEDISPX, ACTIVEDISPY);
    
    // Render all player scores
    pbeRenderPlayerScores(currentTick, lastTick);

    // Render status text with fade effects
    pbeRenderStatusText(currentTick, lastTick);

    // Render status icons and values
    pbeRenderStatus(currentTick, lastTick);
    
    return (true);
}

// Support render routines for the main screen
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
    gfxSetScaleFactor(m_StartMenuFontId, 0.6, false);
    
    // Render "Player X" label above the score
    std::string playerLabel = "Player " + std::to_string(m_currentPlayer + 1);
    gfxRenderString(m_StartMenuFontId, playerLabel, (ACTIVEDISPX+(1024/3)), ACTIVEDISPY + 280, 5, GFX_TEXTCENTER);
    
    // Calculate animated score for main player
    unsigned long displayScore = m_playerStates[m_currentPlayer].score;
    
    if (m_playerStates[m_currentPlayer].score != m_playerStates[m_currentPlayer].inProgressScore) {
        // Check if score changed during animation - if so, update inProgressScore to current display value and restart
        if (m_playerStates[m_currentPlayer].score != m_playerStates[m_currentPlayer].previousScore) {
            // Calculate current display score before restarting
            if (m_playerStates[m_currentPlayer].scoreUpdateStartTick > 0) {
                unsigned long elapsedMS = currentTick - m_playerStates[m_currentPlayer].scoreUpdateStartTick;
                if (elapsedMS < UPDATESCOREMS) {
                    float progress = (float)elapsedMS / (float)UPDATESCOREMS;
                    long scoreDiff = m_playerStates[m_currentPlayer].previousScore - m_playerStates[m_currentPlayer].inProgressScore;
                    displayScore = m_playerStates[m_currentPlayer].inProgressScore + (unsigned long)(scoreDiff * progress);
                    displayScore = ((displayScore + 9) / 10) * 10;
                    // Set inProgressScore to current display value so score never goes down
                    m_playerStates[m_currentPlayer].inProgressScore = displayScore;
                }
            }
            // Restart animation from current position to new score
            m_playerStates[m_currentPlayer].scoreUpdateStartTick = currentTick;
            m_playerStates[m_currentPlayer].previousScore = m_playerStates[m_currentPlayer].score;
        }
        
        // Initialize animation start time if not set
        if (m_playerStates[m_currentPlayer].scoreUpdateStartTick == 0) {
            m_playerStates[m_currentPlayer].scoreUpdateStartTick = currentTick;
        }
        
        // Calculate time elapsed since animation started
        unsigned long elapsedMS = currentTick - m_playerStates[m_currentPlayer].scoreUpdateStartTick;
        
        if (elapsedMS >= UPDATESCOREMS) {
            // Animation complete, show final score
            displayScore = m_playerStates[m_currentPlayer].score;
            m_playerStates[m_currentPlayer].inProgressScore = m_playerStates[m_currentPlayer].score;
            m_playerStates[m_currentPlayer].previousScore = m_playerStates[m_currentPlayer].score;
            m_playerStates[m_currentPlayer].scoreUpdateStartTick = 0;
        } else {
            // Calculate interpolated score based on elapsed time
            float progress = (float)elapsedMS / (float)UPDATESCOREMS;
            long scoreDiff = m_playerStates[m_currentPlayer].score - m_playerStates[m_currentPlayer].inProgressScore;
            displayScore = m_playerStates[m_currentPlayer].inProgressScore + (unsigned long)(scoreDiff * progress);
            
            // Round up to nearest 10
            displayScore = ((displayScore + 9) / 10) * 10;
        }
    } else {
        // Scores are equal, reset animation tracking
        m_playerStates[m_currentPlayer].previousScore = m_playerStates[m_currentPlayer].score;
        m_playerStates[m_currentPlayer].scoreUpdateStartTick = 0;
    }
    
    // Render the current player's score with commas
    std::string scoreText = formatScoreWithCommas(displayScore);
    gfxSetScaleFactor(m_StartMenuFontId, 1.2, false);
    gfxRenderString(m_StartMenuFontId, scoreText, (ACTIVEDISPX+(1024/3)), ACTIVEDISPY + 350, 5, GFX_TEXTCENTER);
    
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
        gfxRenderString(m_StartMenuFontId, playerScoreText, positions[slotIndex], ACTIVEDISPY + 725 + yOffset, 3, GFX_TEXTLEFT);
        
        slotIndex++;
    }
}

void PBEngine::pbeSetStatusText(int index, const std::string& text){
    if (index >= 0 && index <= 1) {
        m_statusText[index] = text;
    }
}

void PBEngine::pbeRenderStatusText(unsigned long currentTick, unsigned long lastTick){
    // Calculate position: right justified to (active width - 1/3 active width)
    // Active width is 1024, so 1/3 is ~341, right edge is at 1024 - 341 = 683
    int rightX = ACTIVEDISPX + 683;
    int yPos = ACTIVEDISPY + 660; // Same vertical position as Ball text
    
    // Render the current ball indicator in lower left (same color as main score)
    gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
    gfxSetScaleFactor(m_StartMenuFontId, 0.35, false);
    std::string ballText = "Ball: " + std::to_string(m_playerStates[m_currentPlayer].currentBall);
    gfxRenderString(m_StartMenuFontId, ballText, ACTIVEDISPX + 10, yPos, 3, GFX_TEXTLEFT);
    
    // Check if both strings are empty - skip rendering entirely
    if (m_statusText[0].empty() && m_statusText[1].empty()) {
        return;
    }
    
    // Initialize fade start time on first call
    if (m_statusTextFadeStart == 0) {
        m_statusTextFadeStart = currentTick;
    }
    
    // Check if we're switching text
    if (m_currentActiveText != m_previousActiveText) {
        // Only switch if the new text is not empty
        if (m_statusText[m_currentActiveText].empty()) {
            // New text is empty, revert to previous
            m_currentActiveText = m_previousActiveText;
        } else if (m_statustextFadeIn) {
            // Text changed - start fade out of old text if we were fading in
            m_statustextFadeIn = false;
            m_statusTextFadeStart = currentTick;
        }
    }
    
    // Calculate fade alpha
    unsigned int alpha = 0;
    float fadeDuration = 500.0f; // 500ms fade time
    float elapsedSec = (currentTick - m_statusTextFadeStart) / 1000.0f;
    float elapsedMs = elapsedSec * 1000.0f;
    
    // Determine which text to render based on fade state
    int textToRender = m_statustextFadeIn ? m_currentActiveText : m_previousActiveText;
    
    if (!m_statustextFadeIn) {
        // Fading out
        if (elapsedMs >= fadeDuration) {
            // Fade out complete, start fading in new text
            alpha = 0;
            m_statustextFadeIn = true;
            m_previousActiveText = m_currentActiveText;
            m_statusTextFadeStart = currentTick;
        } else {
            // Linear fade from 255 to 0
            float progress = elapsedMs / fadeDuration;
            alpha = static_cast<unsigned int>(255.0f * (1.0f - progress));
        }
    } else {
        // Fading in
        if (elapsedMs >= fadeDuration) {
            // Fade in complete
            alpha = 255;
            
            // Start display timer if not already started
            if (m_statusTextDisplayStart == 0) {
                m_statusTextDisplayStart = currentTick;
            }
            
            // Check if other text entry is not empty and display duration has elapsed
            int otherTextIndex = (m_currentActiveText == 0) ? 1 : 0;
            if (!m_statusText[otherTextIndex].empty()) {
                float displayDuration = 4000.0f; // 3 seconds display time
                float displayElapsedMs = (currentTick - m_statusTextDisplayStart);
                
                if (displayElapsedMs >= displayDuration) {
                    // Switch to other text (will trigger fade out on next frame)
                    m_currentActiveText = otherTextIndex;
                    m_statusTextDisplayStart = 0;
                }
            }
        } else {
            // Linear fade from 0 to 255
            float progress = elapsedMs / fadeDuration;
            alpha = static_cast<unsigned int>(255.0f * progress);
        }
    }
    
    // Render the current status text
    gfxSetColor(m_StartMenuFontId, 255, 255, 255, alpha);
    gfxSetScaleFactor(m_StartMenuFontId, 0.35, false);
    gfxRenderString(m_StartMenuFontId, m_statusText[textToRender], rightX, yPos, 3, GFX_TEXTRIGHT);
}

bool PBEngine::pbeRenderStatus(unsigned long currentTick, unsigned long lastTick){
    
    // Static position variables for character circles
    static const int circle1X = ACTIVEDISPX + 700;
    static const int circle1Y = ACTIVEDISPY + 20;
    static const int circle2X = ACTIVEDISPX + 860;
    static const int circle2Y = ACTIVEDISPY + 20;
    static const int circle3X = ACTIVEDISPX + 780;
    static const int circle3Y = ACTIVEDISPY + 170;
    
    // Static position variables for resource icons
    static const int treasureX = ACTIVEDISPX + 720;
    static const int treasureY = ACTIVEDISPY + 350;
    static const int swordX = ACTIVEDISPX + 685;
    static const int swordY = ACTIVEDISPY + 467;
    static const int shieldX = ACTIVEDISPX + 845;
    static const int shieldY = ACTIVEDISPY + 467;
    static const int dungeonX = ACTIVEDISPX + 720;
    static const int dungeonY = ACTIVEDISPY + 580;
    
    // Get current player's game state
    pbGameState& playerState = m_playerStates[m_currentPlayer];
    
    // Render character headshots in the center of each circle
    // Headshots are 256x256, scaled down by another 10% from 0.45 to 0.405 (~104px)
    
    // Ranger (Archer) - upper left circle (Shidea) - right 30px, down 15px
    gfxSetScaleFactor(m_PBTBLArcherHeadshot256Id, 0.405, false);
    if (playerState.rangerJoined) {
        // Full color
        gfxSetColor(m_PBTBLArcherHeadshot256Id, 255, 255, 255, 255);
    } else {
        // 1/3 grayscale (85, 85, 85) at 50% transparency
        gfxSetColor(m_PBTBLArcherHeadshot256Id, 85, 85, 85, 128);
    }
    gfxRenderSprite(m_PBTBLArcherHeadshot256Id, circle1X + 37, circle1Y + 22);
    
    // Priest (Wolf) - upper right circle (Kahriel) - right 20px, down 10px
    gfxSetScaleFactor(m_PBTBLWolfHeadshot256Id, 0.405, false);
    if (playerState.priestJoined) {
        // Full color
        gfxSetColor(m_PBTBLWolfHeadshot256Id, 255, 255, 255, 255);
    } else {
        // 1/3 grayscale (85, 85, 85) at 50% transparency
        gfxSetColor(m_PBTBLWolfHeadshot256Id, 85, 85, 85, 128);
    }
    gfxRenderSprite(m_PBTBLWolfHeadshot256Id, circle2X + 27, circle2Y + 27);
    
    // Knight - lower middle circle (Caiphos) - right 25px, down 20px
    gfxSetScaleFactor(m_PBTBLKnightHeadshot256Id, 0.38, false);
    if (playerState.knightJoined) {
        // Full color
        gfxSetColor(m_PBTBLKnightHeadshot256Id, 255, 255, 255, 255);
    } else {
        // 1/3 grayscale (85, 85, 85) at 50% transparency
        gfxSetColor(m_PBTBLKnightHeadshot256Id, 85, 85, 85, 128);
    }
    gfxRenderSprite(m_PBTBLKnightHeadshot256Id, circle3X + 25, circle3Y + 31);
    
    // Render character circles
    gfxRenderSprite(m_PBTBLCharacterCircle256Id, circle1X, circle1Y);
    gfxRenderSprite(m_PBTBLCharacterCircle256Id, circle2X, circle2Y);
    gfxRenderSprite(m_PBTBLCharacterCircle256Id, circle3X, circle3Y);
    
    // Render level text at bottom of each circle
    gfxSetScaleFactor(m_StartMenuFontId, 0.3, false);
    
    // Ranger level - below and to the left of circle1
    std::string rangerLevelText = "Level: " + std::to_string(playerState.rangerLevel);
    if (playerState.rangerJoined) {
        // White color with black shadow
        gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
        gfxRenderShadowString(m_StartMenuFontId, rangerLevelText, circle1X + 14, circle1Y + 145, 3, GFX_TEXTLEFT, 0, 0, 0, 255, 1);
    }
    // Priest level - below and to the right of circle2
    std::string priestLevelText = "Level: " + std::to_string(playerState.priestLevel);
    if (playerState.priestJoined) {
        // White color with black shadow
        gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
        gfxRenderShadowString(m_StartMenuFontId, priestLevelText, circle2X + 128 + 10, circle2Y + 145, 3, GFX_TEXTRIGHT, 0, 0, 0, 255, 1);
    }
    // Knight level - bottom of circle3
    std::string knightLevelText = "Level: " + std::to_string(playerState.knightLevel);
    if (playerState.knightJoined) {
        // White color with black shadow
        gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
        gfxRenderShadowString(m_StartMenuFontId, knightLevelText, circle3X + 78, circle3Y - 15, 3, GFX_TEXTCENTER, 0, 0, 0, 255, 1);
    }
    // Render resource icons
    gfxRenderSprite(m_PBTBLTreasure256Id, treasureX, treasureY);
    gfxRenderSprite(m_PBTBLSword256Id, swordX, swordY);
    gfxRenderSprite(m_PBTBLShield256Id, shieldX, shieldY);
    gfxRenderSprite(m_PBTBLDungeon256Id, dungeonX, dungeonY);
    
    // Render character names with gold color and shadow
    gfxSetColor(m_StartMenuFontId, 235, 176, 20, 255);
    gfxSetScaleFactor(m_StartMenuFontId, 0.3, false);
    
    // "Shidea" above top left circle (center of circle is at circle1X + 64)
    gfxRenderShadowString(m_StartMenuFontId, "Shidea", circle1X + 77, circle1Y - 15, 4, GFX_TEXTCENTER, 150, 100, 0, 255, 2);
    
    // "Kahriel" above top right circle (center of circle is at circle2X + 64)
    gfxRenderShadowString(m_StartMenuFontId, "Kahriel", circle2X + 78, circle2Y - 15, 4, GFX_TEXTCENTER, 150, 100, 0, 255, 2);
    
    // "Caiphos" below middle circle (bottom is at circle3Y + 128)
    gfxRenderShadowString(m_StartMenuFontId, "Caiphos", circle3X + 76, circle3Y + 142, 4, GFX_TEXTCENTER, 150, 100, 0, 255, 2);
    
    // Render resource values to the right of icons
    // Icon text scaled to 0.7 (2x bigger than 0.35)
    gfxSetScaleFactor(m_StartMenuFontId, 0.7, false);
    
    // Gold value (bright gold color)
    gfxSetColor(m_StartMenuFontId, 255, 215, 0, 255);
    std::string goldText = std::to_string(playerState.goldValue);
    gfxRenderShadowString(m_StartMenuFontId, goldText, treasureX + 115, treasureY + 30, 3, GFX_TEXTLEFT, 180, 140, 0, 255, 2);
    
    // Attack value (fire red/orange color)
    gfxSetColor(m_StartMenuFontId, 255, 80, 20, 255);
    std::string attackText = std::to_string(playerState.attackValue);
    gfxRenderShadowString(m_StartMenuFontId, attackText, swordX + 105, swordY + 30, 3, GFX_TEXTLEFT, 180, 40, 10, 255, 2);
    
    // Defense value (steel blue-grey color)
    gfxSetColor(m_StartMenuFontId, 43, 66, 69, 255);
    std::string defenseText = std::to_string(playerState.defenseValue);
    gfxRenderShadowString(m_StartMenuFontId, defenseText, shieldX + 105, shieldY + 30, 3, GFX_TEXTLEFT, 20, 30, 32, 255, 2);
    
    // Dungeon floor and level (darker stone grey color)
    // Dungeon text scaled to 0.525 (1.5x bigger than 0.35)
    gfxSetScaleFactor(m_StartMenuFontId, 0.525, false);
    gfxSetColor(m_StartMenuFontId, 81, 79, 122, 255);
    std::string floorText = "Floor: " + std::to_string(playerState.dungeonFloor);
    std::string levelText = "Level: " + std::to_string(playerState.dungeonLevel);
    gfxRenderShadowString(m_StartMenuFontId, floorText, dungeonX + 123, dungeonY + 15, 3, GFX_TEXTLEFT, 27, 24, 48, 255, 2);
    gfxRenderShadowString(m_StartMenuFontId, levelText, dungeonX + 123, dungeonY + 60, 3, GFX_TEXTLEFT, 27, 24, 48, 255, 2);
    
    return (true);
}

// Load and render fucntions for the reset screen
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

bool PBEngine::pbeRenderReset(unsigned long currentTick, unsigned long lastTick){
    
    if (!pbeLoadReset()) {
        pbeSendConsole("ERROR: Failed to load reset screen resources");
        return (false);
    }
    
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

// Various Helper and intialization functions used throught the game

bool PBEngine::pbeTableInit(){
    
    // Initialize and register the ball ejector device (using example IDs - can be configured per table)
    pbdEjector* ballEjector = new pbdEjector(this, IDI_IO0P07_EJECTSW2, IDO_LED0P08_LSlingLED, IDO_IO2P08_EJECT);
    pbeAddDevice(ballEjector);
    
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

// Load function for the init screen
bool PBEngine::pbeLoadInitScreen(){
    
    if (m_initScreenLoaded) return (true);
    
    // Initialize table devices and state
    pbeTableInit();
    
    m_initScreenLoaded = true;
    return true;
}


// Render function for the init screen
bool PBEngine::pbeRenderInitScreen(unsigned long currentTick, unsigned long lastTick){
    
    if (!pbeLoadInitScreen()) {
        pbeSendConsole("ERROR: Failed to load init screen resources");
        return (false);
    }
    
    // Transition to start screen
    m_tableState = PBTableState::PBTBL_START;
    return true;
}

// Main render selection function for the pinball table
bool PBEngine::pbeRenderGameScreen(unsigned long currentTick, unsigned long lastTick){
    
    bool success = false;

    switch (m_tableState) {
        case PBTableState::PBTBL_INIT: success = pbeRenderInitScreen(currentTick, lastTick); break;
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

//
// Main State Loop for Pinball Table
//
void PBEngine::pbeUpdateGameState(stInputMessage inputMessage){
    
    // Check for reset button press first, regardless of current state (unless already in reset)
    if (m_tableState != PBTableState::PBTBL_RESET) {
        if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON && inputMessage.inputId == IDI_RPIOP24_RESET) {
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
        case PBTableState::PBTBL_INIT: {
            // Nothing right now, it's all taken care in the init render function
        break;
        }
        case PBTableState::PBTBL_START: {

            // Handle button presses during start screen
            if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
                if (inputMessage.inputId == IDI_RPIOP06_START) {
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
                if (inputMessage.inputId == IDI_RPIOP06_START) {
                    if (pbeTryAddPlayer()){
                        // Play the sword cut sound
                        m_soundSystem.pbsPlayEffect(SOUNDSWORDCUT);
                    }
                }
                // Handle activate buttons to add score
                else if (inputMessage.inputId == IDI_RPIOP05_LACTIVATE) {
                    addPlayerScore(100);
                }
                else if (inputMessage.inputId == IDI_RPIOP22_RACTIVATE) {
                    addPlayerScore(10000);
                }
                // Handle flipper buttons to enable/disable all characters
                else if (inputMessage.inputId == IDI_RPIOP27_LFLIP) {
                    // Enable all characters
                    m_playerStates[m_currentPlayer].knightJoined = true;
                    m_playerStates[m_currentPlayer].priestJoined = true;
                    m_playerStates[m_currentPlayer].rangerJoined = true;
                }
                else if (inputMessage.inputId == IDI_RPIOP17_RFLIP) {
                    // Disable all characters
                    m_playerStates[m_currentPlayer].knightJoined = false;
                    m_playerStates[m_currentPlayer].priestJoined = false;
                    m_playerStates[m_currentPlayer].rangerJoined = false;
                }
            }
            
            // Update mode system
            pbeUpdateModeSystem(inputMessage, GetTickCountGfx());
            
            break;
        }
        case PBTableState::PBTBL_STDPLAY: {
            // Standard play mode - delegates to mode system
            pbeUpdateModeSystem(inputMessage, GetTickCountGfx());

            break;
        }
        case PBTableState::PBTBL_RESET: {
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
                }
            }
            break;
        }
        
        default: break;
    }
}

// Reload function to reset all table screen load states
void PBEngine::pbeTableReload() {
    m_initScreenLoaded = false;
    m_gameStartLoaded = false;
    m_mainScreenLoaded = false;
    m_resetLoaded = false;
    m_RestartTable = true;    
}

// ========================================================================
// MODE SYSTEM IMPLEMENTATION
// ========================================================================

// Helper function to update mode system - reduces code duplication
void PBEngine::pbeUpdateModeSystem(stInputMessage inputMessage, unsigned long currentTick) {
    // Update mode system state
    pbeUpdateModeState(currentTick);
    
    // Update screen manager
    pbeUpdateScreenManager(currentTick);
    
    // Route input to current mode's update function
    ModeState& modeState = m_playerStates[m_currentPlayer].modeState;
    switch (modeState.currentMode) {
        case PBTableMode::MODE_NORMAL_PLAY:
            pbeUpdateNormalPlayMode(inputMessage, currentTick);
            break;
        case PBTableMode::MODE_MULTIBALL:
            pbeUpdateMultiballMode(inputMessage, currentTick);
            break;
        default:
            break;
    }
}

// Main mode state update function - called each frame to update current mode
void PBEngine::pbeUpdateModeState(unsigned long currentTick) {
    // Get current player's mode state
    ModeState& modeState = m_playerStates[m_currentPlayer].modeState;
    
    // Check if we should transition to a different mode
    if (pbeCheckModeTransition(currentTick)) {
        return; // Mode transition occurred, state will be updated next frame
    }
    
    // Update the current mode's state machine
    switch (modeState.currentMode) {
        case PBTableMode::MODE_NORMAL_PLAY: {
            // Update normal play mode sub-states
            switch (modeState.normalPlayState) {
                case PBNormalPlayState::NORMAL_IDLE:
                    // Check if ball became active
                    // In a real implementation, you'd check ball switches/sensors
                    // For now, we'll just remain in idle
                    break;
                    
                case PBNormalPlayState::NORMAL_ACTIVE:
                    // Ball is in play - normal gameplay logic here
                    // Check for mode qualification conditions
                    if (pbeCheckMultiballQualified()) {
                        pbeSendConsole("Multiball qualified!");
                        modeState.multiballQualified = true;
                    }
                    break;
                    
                case PBNormalPlayState::NORMAL_DRAIN:
                    // Ball drained - handle end of ball
                    // Transition back to idle after a delay
                    if (currentTick - modeState.normalPlayStateStartTick > 2000) {
                        modeState.normalPlayState = PBNormalPlayState::NORMAL_IDLE;
                        modeState.normalPlayStateStartTick = currentTick;
                    }
                    break;
                    
                default:
                    break;
            }
            break;
        }
        
        case PBTableMode::MODE_MULTIBALL: {
            // Update multiball mode sub-states
            switch (modeState.multiballState) {
                case PBMultiballState::MULTIBALL_START:
                    // Starting multiball sequence
                    pbeSendConsole("Multiball starting...");
                    
                    // Transition to active after start sequence
                    if (currentTick - modeState.multiballStateStartTick > 3000) {
                        modeState.multiballState = PBMultiballState::MULTIBALL_ACTIVE;
                        modeState.multiballStateStartTick = currentTick;
                        modeState.multiballCount = 2; // Start with 2 balls
                        
                        // Request high-priority screen
                        pbeRequestScreen(100, ScreenPriority::PRIORITY_HIGH, 5000, false);
                    }
                    break;
                    
                case PBMultiballState::MULTIBALL_ACTIVE:
                    // Multiball gameplay active
                    // In a real implementation, track balls in play
                    // For now, we'll end multiball after 20 seconds
                    if (currentTick - modeState.multiballStateStartTick > 20000) {
                        modeState.multiballState = PBMultiballState::MULTIBALL_ENDING;
                        modeState.multiballStateStartTick = currentTick;
                        pbeSendConsole("Multiball ending...");
                    }
                    break;
                    
                case PBMultiballState::MULTIBALL_ENDING:
                    // Transitioning back to normal play
                    if (currentTick - modeState.multiballStateStartTick > 2000) {
                        // Exit multiball mode and return to normal play
                        pbeExitMode(PBTableMode::MODE_MULTIBALL, currentTick);
                        pbeEnterMode(PBTableMode::MODE_NORMAL_PLAY, currentTick);
                    }
                    break;
                    
                default:
                    break;
            }
            break;
        }
        
        default:
            break;
    }
}

// Check if conditions are met to transition to a different mode
bool PBEngine::pbeCheckModeTransition(unsigned long currentTick) {
    ModeState& modeState = m_playerStates[m_currentPlayer].modeState;
    
    // Check for mode transitions based on current mode
    switch (modeState.currentMode) {
        case PBTableMode::MODE_NORMAL_PLAY: {
            // Check if multiball should start
            if (modeState.multiballQualified && 
                modeState.normalPlayState == PBNormalPlayState::NORMAL_ACTIVE) {
                
                // For this example, we'll trigger multiball when qualified
                // In a real game, you might need a specific switch hit
                pbeSendConsole("Transitioning to multiball mode!");
                pbeExitMode(PBTableMode::MODE_NORMAL_PLAY, currentTick);
                pbeEnterMode(PBTableMode::MODE_MULTIBALL, currentTick);
                return true;
            }
            break;
        }
        
        case PBTableMode::MODE_MULTIBALL: {
            // Multiball handles its own exit in pbeUpdateModeState
            break;
        }
        
        default:
            break;
    }
    
    return false;
}

// Enter a new mode
void PBEngine::pbeEnterMode(PBTableMode newMode, unsigned long currentTick) {
    ModeState& modeState = m_playerStates[m_currentPlayer].modeState;
    
    pbeSendConsole("Entering mode: " + std::to_string(static_cast<int>(newMode)));
    
    // Save previous mode
    modeState.previousMode = modeState.currentMode;
    modeState.currentMode = newMode;
    modeState.modeTransitionActive = true;
    modeState.modeTransitionStartTick = currentTick;
    
    // Initialize mode-specific state
    switch (newMode) {
        case PBTableMode::MODE_NORMAL_PLAY:
            modeState.normalPlayState = PBNormalPlayState::NORMAL_ACTIVE;
            modeState.normalPlayStateStartTick = currentTick;
            
            // Request normal play screen
            pbeRequestScreen(1, ScreenPriority::PRIORITY_LOW, 0, true);
            break;
            
        case PBTableMode::MODE_MULTIBALL:
            modeState.multiballState = PBMultiballState::MULTIBALL_START;
            modeState.multiballStateStartTick = currentTick;
            modeState.multiballCount = 1;
            
            // Request multiball intro screen
            pbeRequestScreen(200, ScreenPriority::PRIORITY_HIGH, 3000, false);
            break;
            
        default:
            break;
    }
}

// Exit a mode
void PBEngine::pbeExitMode(PBTableMode exitingMode, unsigned long currentTick) {
    ModeState& modeState = m_playerStates[m_currentPlayer].modeState;
    
    pbeSendConsole("Exiting mode: " + std::to_string(static_cast<int>(exitingMode)));
    
    // Clean up mode-specific state
    switch (exitingMode) {
        case PBTableMode::MODE_NORMAL_PLAY:
            // No special cleanup needed
            break;
            
        case PBTableMode::MODE_MULTIBALL:
            // Reset multiball state
            modeState.multiballQualified = false;
            modeState.multiballCount = 1;
            break;
            
        default:
            break;
    }
}

// Update normal play mode with input
void PBEngine::pbeUpdateNormalPlayMode(stInputMessage inputMessage, unsigned long currentTick) {
    ModeState& modeState = m_playerStates[m_currentPlayer].modeState;
    
    // Handle inputs specific to normal play mode
    if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
        // Example: Left activate button changes state in normal play
        if (inputMessage.inputId == IDI_RPIOP05_LACTIVATE) {
            if (modeState.normalPlayState == PBNormalPlayState::NORMAL_IDLE) {
                modeState.normalPlayState = PBNormalPlayState::NORMAL_ACTIVE;
                modeState.normalPlayStateStartTick = currentTick;
                pbeSendConsole("Normal play: Ball became active");
            }
        }
        // Right activate button drains ball
        else if (inputMessage.inputId == IDI_RPIOP22_RACTIVATE) {
            if (modeState.normalPlayState == PBNormalPlayState::NORMAL_ACTIVE) {
                modeState.normalPlayState = PBNormalPlayState::NORMAL_DRAIN;
                modeState.normalPlayStateStartTick = currentTick;
                pbeSendConsole("Normal play: Ball drained");
            }
        }
    }
}

// Update multiball mode with input
void PBEngine::pbeUpdateMultiballMode(stInputMessage inputMessage, unsigned long currentTick) {
    ModeState& modeState = m_playerStates[m_currentPlayer].modeState;
    
    // Handle inputs specific to multiball mode
    if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
        // Example: Add scoring in multiball
        if (inputMessage.inputId == IDI_RPIOP05_LACTIVATE) {
            addPlayerScore(5000); // Higher scoring in multiball
            pbeSendConsole("Multiball jackpot!");
        }
    }
}

// Check if multiball is qualified
bool PBEngine::pbeCheckMultiballQualified() {
    // Example qualification: Score over 50,000 points
    // In a real game, this would check specific targets, combos, etc.
    return (m_playerStates[m_currentPlayer].score > 50000);
}

// ========================================================================
// SCREEN MANAGER IMPLEMENTATION
// ========================================================================

// Request a screen to be displayed
void PBEngine::pbeRequestScreen(int screenId, ScreenPriority priority, 
                                unsigned long durationMs, bool canBePreempted) {
    ScreenRequest request;
    request.screenId = screenId;
    request.priority = priority;
    request.durationMs = durationMs;
    request.requestTick = GetTickCountGfx();
    request.canBePreempted = canBePreempted;
    
    // Add to queue (we'll sort by priority in update function)
    m_screenQueue.push_back(request);
    
    pbeSendConsole("Screen request: ID=" + std::to_string(screenId) + 
                   " Priority=" + std::to_string(static_cast<int>(priority)));
}

// Update screen manager - determines which screen should be displayed
void PBEngine::pbeUpdateScreenManager(unsigned long currentTick) {
    if (m_screenQueue.empty()) {
        return; // No screen requests
    }
    
    // Sort queue by priority (highest first)
    std::sort(m_screenQueue.begin(), m_screenQueue.end(), 
        [](const ScreenRequest& a, const ScreenRequest& b) {
            return static_cast<int>(a.priority) > static_cast<int>(b.priority);
        });
    
    // Get highest priority request
    ScreenRequest& topRequest = m_screenQueue.front();
    
    // Check if we should change the displayed screen
    bool shouldChange = false;
    
    if (m_currentDisplayedScreen != topRequest.screenId) {
        // Different screen requested
        if (m_currentDisplayedScreen == -1) {
            // No screen currently displayed
            shouldChange = true;
        } else {
            // Check if current screen can be preempted
            // Find current screen in queue
            bool currentCanBePreempted = true;
            for (const auto& req : m_screenQueue) {
                if (req.screenId == m_currentDisplayedScreen) {
                    currentCanBePreempted = req.canBePreempted;
                    break;
                }
            }
            
            if (currentCanBePreempted && 
                static_cast<int>(topRequest.priority) > 
                static_cast<int>(ScreenPriority::PRIORITY_LOW)) {
                shouldChange = true;
            }
        }
    }
    
    if (shouldChange) {
        m_currentDisplayedScreen = topRequest.screenId;
        m_currentScreenStartTick = currentTick;
        pbeSendConsole("Screen changed to: " + std::to_string(m_currentDisplayedScreen));
    }
    
    // Remove expired screens from queue
    auto it = m_screenQueue.begin();
    while (it != m_screenQueue.end()) {
        bool expired = false;
        
        // Check if duration has elapsed (if duration is specified)
        if (it->durationMs > 0) {
            if (currentTick - it->requestTick > it->durationMs) {
                expired = true;
            }
        }
        
        if (expired) {
            if (it->screenId == m_currentDisplayedScreen) {
                m_currentDisplayedScreen = -1; // Clear current screen
            }
            it = m_screenQueue.erase(it);
        } else {
            ++it;
        }
    }
}

// Clear all screen requests
void PBEngine::pbeClearScreenRequests() {
    m_screenQueue.clear();
    m_currentDisplayedScreen = -1;
    m_currentScreenStartTick = 0;
}

// Get the current screen that should be displayed
int PBEngine::pbeGetCurrentScreen() {
    return m_currentDisplayedScreen;
}



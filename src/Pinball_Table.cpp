
// Pinball_Table.cpp:  Class functions and code for the specific pinball table

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "Pinball_Table.h"
#include "Pinball_TableStr.h"

// PBEgine Class Fucntions for the main pinball game

bool PBEngine::pbeLoadGameStart(bool forceReload){
    
    static bool gameStartLoaded = false;
    if (forceReload) gameStartLoaded = false;
    if (gameStartLoaded) return (true);

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
    gfxLoadAnimateData(&animateData, m_PBTBLLeftDoorId, m_PBTBLeftDoorStartId, m_PBTBLeftDoorEndId, 0, ANIMATE_X_MASK, 1.25f, 0.0f, false, true, GFX_NOLOOP);
    gfxCreateAnimation(animateData, true);

    m_PBTBLRightDoorId = gfxLoadSprite("RightDoor", "src/resources/textures/DoorRight2.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    m_PBTBRightDoorStartId = gfxInstanceSprite(m_PBTBLRightDoorId);
    gfxSetXY(m_PBTBRightDoorStartId, ACTIVEDISPX + 460, ACTIVEDISPY + 112, false);
    m_PBTBRightDoorEndId = gfxInstanceSprite(m_PBTBLRightDoorId);
    gfxSetXY(m_PBTBRightDoorEndId, ACTIVEDISPX + 754, ACTIVEDISPY + 112, false);
    gfxLoadAnimateData(&animateData, m_PBTBLRightDoorId, m_PBTBRightDoorStartId, m_PBTBRightDoorEndId, 0, ANIMATE_X_MASK, 1.25f, 0.0f, false, true, GFX_NOLOOP);
    gfxCreateAnimation(animateData, true);
    
    m_PBTBLDoorDragonId = gfxLoadSprite("DoorDragon", "src/resources/textures/Dragon.bmp", GFX_BMP, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetScaleFactor(m_PBTBLDoorDragonId, 0.8, false);
    m_PBTBLDragonEyesId = gfxLoadSprite("DragonEyes", "src/resources/textures/DragonEyes.bmp", GFX_BMP, GFX_NOMAP, GFX_UPPERLEFT, true, true);

    m_PBTBLDoorDungeonId = gfxLoadSprite("DoorDungeon", "src/resources/textures/Dungeon2.bmp", GFX_BMP, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetScaleFactor(m_PBTBLDoorDungeonId, 0.94, false);

    m_PBTBLFlame1Id = gfxLoadSprite("Flame1", "src/resources/textures/flame1.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, false, true);
    gfxSetColor(m_PBTBLFlame1Id, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame1Id, 1.0, false);

    m_PBTBLFlame1StartId = gfxInstanceSprite(m_PBTBLFlame1Id);
    gfxSetColor(m_PBTBLFlame1StartId, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame1StartId, 1.1, false);

    m_PBTBLFlame1EndId = gfxInstanceSprite(m_PBTBLFlame1Id);
    gfxSetColor(m_PBTBLFlame1EndId, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame1EndId, 1.2, false);

    gfxLoadAnimateData(&animateData, m_PBTBLFlame1Id, m_PBTBLFlame1StartId, m_PBTBLFlame1EndId, 0, ANIMATE_SCALE_MASK, 1.0f, 0.0f, true, true, GFX_REVERSE);
    gfxCreateAnimation(animateData, true);

    m_PBTBLFlame2Id = gfxLoadSprite("Flame2", "src/resources/textures/flame2.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, false, true);
    gfxSetColor(m_PBTBLFlame2Id, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame2Id, 1.15, false);

    m_PBTBLFlame2StartId = gfxInstanceSprite(m_PBTBLFlame2Id);
    gfxSetColor(m_PBTBLFlame2StartId, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame2StartId, 1.1, false);

    m_PBTBLFlame2EndId = gfxInstanceSprite(m_PBTBLFlame2Id);
    gfxSetColor(m_PBTBLFlame2EndId, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame2EndId, 1.2, false);

    gfxLoadAnimateData(&animateData, m_PBTBLFlame2Id, m_PBTBLFlame2StartId, m_PBTBLFlame2EndId, 0, ANIMATE_SCALE_MASK, 0.9f, 0.0f, true, true, GFX_REVERSE);
    gfxCreateAnimation(animateData, true);
    
    m_PBTBLFlame3Id = gfxLoadSprite("Flame3", "src/resources/textures/flame3.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, false, true);
    gfxSetColor(m_PBTBLFlame3Id, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame3Id, 1.15, false);

    m_PBTBLFlame3StartId = gfxInstanceSprite(m_PBTBLFlame3Id);
    gfxSetColor(m_PBTBLFlame3StartId, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame3StartId, 1.1, false);
    
    m_PBTBLFlame3EndId = gfxInstanceSprite(m_PBTBLFlame3Id);
    gfxSetColor(m_PBTBLFlame3EndId, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame3EndId, 1.2, false);

    gfxLoadAnimateData(&animateData, m_PBTBLFlame3Id, m_PBTBLFlame3StartId, m_PBTBLFlame3EndId, 0, ANIMATE_SCALE_MASK, 1.1f, 0.0f, true, true, GFX_REVERSE);
    gfxCreateAnimation(animateData, true);

    // Create the fade animation instances for the text
    m_PBTBLTextStartId = gfxInstanceSprite(m_StartMenuFontId);
    gfxSetColor(m_PBTBLTextStartId, 0, 0, 0, 0);
    m_PBTBLTextEndId = gfxInstanceSprite(m_StartMenuFontId);
    gfxSetColor(m_PBTBLTextEndId, 255, 255, 255, 255);
    gfxLoadAnimateData(&animateData, m_StartMenuFontId, m_PBTBLTextStartId, m_PBTBLTextEndId, 0, ANIMATE_COLOR_MASK, 2.0f, 0.0f, true, true, GFX_NOLOOP);
    gfxCreateAnimation(animateData, true);

    // Note:  So many things to check for loading, it's not worth doing.  Assume the sprites will be loaded.  If texture fails, it will just render incorrectly.

    gameStartLoaded = true;

    return (gameStartLoaded);
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
        lastScreenState = m_tableScreenState;
    }

    if (!pbeLoadGameStart (false)) return (false); 
    
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

bool PBEngine::pbeRenderGameScreen(unsigned long currentTick, unsigned long lastTick){
    
    bool success = false;

    switch (m_tableState) {
        case PBTableState::PBTBL_START: success = pbeRenderGameStart(currentTick, lastTick); break;
        default: success = false; break;
    }

    // Render the backglass all the time (this will cover any other sprites and hide anything that's off the mini-screen)
    // It's got a transparent mini screen which wil show the mini render.
    gfxRenderSprite(m_PBTBLBackglassId, 0, 0);

    return (success);
}

// Main State Loop for Pinball Table

void PBEngine::pbeUpdateGameState(stInputMessage inputMessage){
    
    switch (m_tableState) {
        case PBTableState::PBTBL_START: {

            if (m_PBTBLOpenDoors) {
                if ((!gfxAnimateActive(m_PBTBLLeftDoorId)) && (!gfxAnimateActive(m_PBTBLRightDoorId))) m_tableState = PBTableState::PBTBL_STDPLAY; 
            } 
            else {
                if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
                    if (inputMessage.inputId != IDI_START) {
                        switch (m_tableScreenState) {
                            case PBTBLScreenState::START_START: m_tableScreenState = PBTBLScreenState::START_INST; break;
                            case PBTBLScreenState::START_INST: m_tableScreenState = PBTBLScreenState::START_SCORES; break;
                            case PBTBLScreenState::START_SCORES: m_tableScreenState = PBTBLScreenState::START_START; break;
                            case PBTBLScreenState::START_OPENDOOR: m_tableScreenState = PBTBLScreenState::START_OPENDOOR; break;
                            default: break;
                        }
                    }
                    else {
                        m_tableScreenState = PBTBLScreenState::START_OPENDOOR;
                    }             
                }
            }
            break;
        }
        case PBTableState::PBTBL_STDPLAY: {
            // Check for input messages and update the game state
            int temp = 0;

            break;
        }
        
        default: break;
    }
}


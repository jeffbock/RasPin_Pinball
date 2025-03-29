
#include "PInball_Table.h"
#include "PInball_TableStr.h"

// PBEgine Class Fucntions for the main pinball game

bool PBEngine::pbeLoadGameStart(){
    
    // Scene is loaded but maybe the texures are not
    if (m_PBTBLStartLoaded) 
    {
        // Check that the textures are loaded
        if (!gfxTextureLoaded(m_PBTBLStartDoorId)) {
            if (!gfxReloadTexture(m_PBTBLStartDoorId)) return (false);
        }
        if (!gfxTextureLoaded(m_PBTBLLeftDoorId)) {
            if (!gfxReloadTexture(m_PBTBLLeftDoorId)) return (false);
        }
        if (!gfxTextureLoaded(m_PBTBLRightDoorId)) {
            if (!gfxReloadTexture(m_PBTBLRightDoorId)) return (false);
        }
        if (!gfxTextureLoaded(m_PBTBLDoorDragonId)) {
            if (!gfxReloadTexture(m_PBTBLDoorDragonId)) return (false);
        }
        if (!gfxTextureLoaded(m_PBTBLDragonEyesId)) {
            if (!gfxReloadTexture(m_PBTBLDragonEyesId)) return (false);
        }
        if (!gfxTextureLoaded(m_PBTBLFlame1Id)) {
            if (!gfxReloadTexture(m_PBTBLFlame1Id)) return (false);
        }
        if (!gfxTextureLoaded(m_PBTBLFlame2Id)) {
            if (!gfxReloadTexture(m_PBTBLFlame2Id)) return (false);
        }
        if (!gfxTextureLoaded(m_PBTBLFlame3Id)) {
            if (!gfxReloadTexture(m_PBTBLFlame3Id)) return (false);
        }
        return (true);
    }

    stAnimateData animateData;

    m_PBTBLStartDoorId = gfxLoadSprite("OpenDoor", "src/resources/textures/startdooropen.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLStartDoorId, 255, 255, 255, 255);

    // Load doors and set them up to slide left and right
    m_PBTBLLeftDoorId = gfxLoadSprite("LeftDoor", "src/resources/textures/DoorLeft.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    m_PBTBLeftDoorStartId = gfxInstanceSprite(m_PBTBLLeftDoorId);
    gfxSetXY(m_PBTBLeftDoorStartId, 247, 72, false); 
    m_PBTBLeftDoorEndId = gfxInstanceSprite(m_PBTBLLeftDoorId);
    gfxSetXY(m_PBTBLeftDoorEndId, 77, 72, false); 
    gfxLoadAnimateData(&animateData, m_PBTBLLeftDoorId, m_PBTBLeftDoorStartId, m_PBTBLeftDoorEndId, 0, ANIMATE_X_MASK, 1.25f, 0.0f, false, true, GFX_NOLOOP);
    gfxCreateAnimation(animateData, true);

    m_PBTBLRightDoorId = gfxLoadSprite("RightDoor", "src/resources/textures/DoorRight.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    m_PBTBRightDoorStartId = gfxInstanceSprite(m_PBTBLRightDoorId);
    gfxSetXY(m_PBTBRightDoorStartId, 360, 72, false);
    m_PBTBRightDoorEndId = gfxInstanceSprite(m_PBTBLRightDoorId);
    gfxSetXY(m_PBTBRightDoorEndId, 600, 72, false);
    gfxLoadAnimateData(&animateData, m_PBTBLRightDoorId, m_PBTBRightDoorStartId, m_PBTBRightDoorEndId, 0, ANIMATE_X_MASK, 1.25f, 0.0f, false, true, GFX_NOLOOP);
    gfxCreateAnimation(animateData, true);
    
    m_PBTBLDoorDragonId = gfxLoadSprite("DoorDragon", "src/resources/textures/Dragon.bmp", GFX_BMP, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetScaleFactor(m_PBTBLDoorDragonId, 0.8, false);
    m_PBTBLDragonEyesId = gfxLoadSprite("DragonEyes", "src/resources/textures/DragonEyes.bmp", GFX_BMP, GFX_NOMAP, GFX_UPPERLEFT, true, true);

    m_PBTBLDoorDungeonId = gfxLoadSprite("DoorDungeon", "src/resources/textures/Dungeon.bmp", GFX_BMP, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetScaleFactor(m_PBTBLDoorDungeonId, 0.8, false);

    m_PBTBLFlame1Id = gfxLoadSprite("Flame1", "src/resources/textures/flame1.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, false, true);
    gfxSetColor(m_PBTBLFlame1Id, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame1Id, 0.6, false);

    m_PBTBLFlame1StartId = gfxInstanceSprite(m_PBTBLFlame1Id);
    gfxSetColor(m_PBTBLFlame1StartId, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame1StartId, 0.70, false);

    m_PBTBLFlame1EndId = gfxInstanceSprite(m_PBTBLFlame1Id);
    gfxSetColor(m_PBTBLFlame1EndId, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame1EndId, 0.8, false);

    gfxLoadAnimateData(&animateData, m_PBTBLFlame1Id, m_PBTBLFlame1StartId, m_PBTBLFlame1EndId, 0, ANIMATE_SCALE_MASK, 1.0f, 0.0f, true, true, GFX_REVERSE);
    gfxCreateAnimation(animateData, true);

    m_PBTBLFlame2Id = gfxLoadSprite("Flame2", "src/resources/textures/flame2.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, false, true);
    gfxSetColor(m_PBTBLFlame2Id, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame2Id, 0.75, false);

    m_PBTBLFlame2StartId = gfxInstanceSprite(m_PBTBLFlame2Id);
    gfxSetColor(m_PBTBLFlame2StartId, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame2StartId, 0.70, false);

    m_PBTBLFlame2EndId = gfxInstanceSprite(m_PBTBLFlame2Id);
    gfxSetColor(m_PBTBLFlame2EndId, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame2EndId, 0.8, false);

    gfxLoadAnimateData(&animateData, m_PBTBLFlame2Id, m_PBTBLFlame2StartId, m_PBTBLFlame2EndId, 0, ANIMATE_SCALE_MASK, 0.9f, 0.0f, true, true, GFX_REVERSE);
    gfxCreateAnimation(animateData, true);
    
    m_PBTBLFlame3Id = gfxLoadSprite("Flame3", "src/resources/textures/flame3.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, false, true);
    gfxSetColor(m_PBTBLFlame3Id, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame3Id, 0.75, false);

    m_PBTBLFlame3StartId = gfxInstanceSprite(m_PBTBLFlame3Id);
    gfxSetColor(m_PBTBLFlame3StartId, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame3StartId, 0.70, false);
    
    m_PBTBLFlame3EndId = gfxInstanceSprite(m_PBTBLFlame3Id);
    gfxSetColor(m_PBTBLFlame3EndId, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame3EndId, 0.8, false);

    gfxLoadAnimateData(&animateData, m_PBTBLFlame3Id, m_PBTBLFlame3StartId, m_PBTBLFlame3EndId, 0, ANIMATE_SCALE_MASK, 1.1f, 0.0f, true, true, GFX_REVERSE);
    gfxCreateAnimation(animateData, true);

    // Create the fade animation instances for the text
    m_PBTBLTextStartId = gfxInstanceSprite(m_StartMenuFontId);
    gfxSetColor(m_PBTBLTextStartId, 0, 0, 0, 0);
    m_PBTBLTextEndId = gfxInstanceSprite(m_StartMenuFontId);
    gfxSetColor(m_PBTBLTextEndId, 255, 255, 255, 255);
    gfxLoadAnimateData(&animateData, m_StartMenuFontId, m_PBTBLTextStartId, m_PBTBLTextEndId, 0, ANIMATE_COLOR_MASK, 2.0f, 0.0f, true, true, GFX_NOLOOP);
    gfxCreateAnimation(animateData, true);

    if (m_PBTBLStartDoorId == NOSPRITE || m_PBTBLFlame1Id == NOSPRITE || m_PBTBLFlame2Id == NOSPRITE ||  
        m_PBTBLFlame3Id == NOSPRITE) return (false);

    m_PBTBLStartLoaded = true;

    return (m_PBTBLStartLoaded);
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

    if (!pbeLoadGameStart ()) return (false); 
    
    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);

    // Hide the dragon behind the doors
    gfxRenderSprite(m_PBTBLDoorDungeonId, 215, 80);

    // Show the door image with the flames - animate if it's opening...
    if (!m_PBTBLOpenDoors) {
        gfxRenderSprite(m_PBTBLLeftDoorId, 247, 72);
        gfxRenderSprite(m_PBTBLRightDoorId, 360, 72);
    }
    else{
        gfxAnimateSprite(m_PBTBLLeftDoorId, currentTick);
        gfxAnimateSprite(m_PBTBLRightDoorId, currentTick);
        gfxRenderSprite(m_PBTBLLeftDoorId);
        gfxRenderSprite(m_PBTBLRightDoorId);
    }

    gfxRenderSprite(m_PBTBLStartDoorId, 0, 0);
    
    gfxSetColor(m_StartMenuFontId, 255 ,165, 0, 255);
    gfxSetScaleFactor(m_StartMenuFontId, 1.25, false);
    if (!m_PBTBLOpenDoors) {
        gfxRenderShadowString (m_StartMenuFontId, "Dragons of Destiny", (PB_SCREENWIDTH/2) + 20, 5, 2, GFX_TEXTCENTER, 0, 0, 0, 255, 3);
        gfxSetScaleFactor(m_StartMenuFontId, 0.5, false);
        gfxRenderShadowString(m_StartMenuFontId, "Designed by Jeff Bock", (PB_SCREENWIDTH/2) + 20, 80, 2, GFX_TEXTCENTER, 0, 0, 0, 255, 3);
    }
    
    gfxAnimateSprite(m_PBTBLFlame1Id, currentTick);
    gfxAnimateSprite(m_PBTBLFlame2Id, currentTick);
    gfxAnimateSprite(m_PBTBLFlame3Id, currentTick);

    gfxRenderSprite(m_PBTBLFlame1Id, 178, 240);
    gfxRenderSprite(m_PBTBLFlame2Id, 178, 240);
    gfxRenderSprite(m_PBTBLFlame3Id, 178, 240);        

    gfxRenderSprite(m_PBTBLFlame1Id, 665, 240);
    gfxRenderSprite(m_PBTBLFlame2Id, 665, 240);
    gfxRenderSprite(m_PBTBLFlame3Id, 665, 240);

    if (lastScreenState != m_tableScreenState) {
        timeoutTicks = 18000; // Reset the timeout if we change screens
        lastScreenState = m_tableScreenState;
        gfxAnimateRestart(m_StartMenuFontId);
    }

    switch (m_tableScreenState) {
        case START_START:
            gfxAnimateSprite(m_StartMenuFontId, currentTick);
            gfxSetScaleFactor(m_StartMenuFontId, 0.8, false);
            gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
            if (blinkCountTicks <= 0) {
                blinkOn = !blinkOn; 
                if (blinkOn) blinkCountTicks = 2000;
                else blinkCountTicks = 500;
            } else {
                blinkCountTicks -= (currentTick - lastTick);
            }
            if (blinkOn) gfxRenderShadowString(m_StartMenuFontId, "Press Start", (PB_SCREENWIDTH/2) + 20, 200, 1, GFX_TEXTCENTER, 0, 0, 0, 255, 3);
            break;

        case START_INST:
            gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
            gfxAnimateSprite(m_StartMenuFontId, currentTick);
            gfxSetScaleFactor(m_StartMenuFontId, 0.5, false);
            // loop through PBTableInst and print each string
            for (int i = 0; i < PBTABLEINSTSIZE; i++) {
                if (i == 0) {
                    if (gfxAnimateActive(m_StartMenuFontId)) gfxRenderString(m_StartMenuFontId, PBTableInst[i], (PB_SCREENWIDTH/2) + 20, 130 + (i * 34), 2, GFX_TEXTCENTER);
                    else gfxRenderShadowString (m_StartMenuFontId, PBTableInst[i], (PB_SCREENWIDTH/2) + 20, 130 + (i * 34), 2, GFX_TEXTCENTER, 0, 0, 0, 255, 3);
                }
                else {
                    if (gfxAnimateActive(m_StartMenuFontId)) gfxRenderString(m_StartMenuFontId, PBTableInst[i], 220, 130 + (i * 34), 2, GFX_TEXTLEFT);
                    else gfxRenderShadowString (m_StartMenuFontId, PBTableInst[i], 220, 130 + (i * 34), 2, GFX_TEXTLEFT, 0, 0, 0, 255, 3);
                }
            }
            break;
            
        case START_SCORES:
            gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
            gfxAnimateSprite(m_StartMenuFontId, currentTick);
            gfxSetScaleFactor(m_StartMenuFontId, 0.8, false);

            for (int i = 0; i < NUM_HIGHSCORES + 1; i++) {
                std::string scoreText = std::to_string(m_saveFileData.highScores[i-1].highScore);

                if (i == 0){
                    if (gfxAnimateActive(m_StartMenuFontId)) gfxRenderString(m_StartMenuFontId, "High Scores", (PB_SCREENWIDTH/2) + 20, 130 + (i * 50), 10, GFX_TEXTCENTER);
                    else gfxRenderShadowString(m_StartMenuFontId, "High Scores", (PB_SCREENWIDTH/2) + 20, 130 + (i * 50), 10, GFX_TEXTCENTER, 0, 0, 0, 255, 3);
                }
                else if (gfxAnimateActive(m_StartMenuFontId)) 
                {
                    gfxRenderString(m_StartMenuFontId, m_saveFileData.highScores[i-1].playerInitials, (PB_SCREENWIDTH/2) - 140, 140 + (i * 50), 10, GFX_TEXTLEFT);
                    gfxRenderString(m_StartMenuFontId, scoreText, (PB_SCREENWIDTH/2) + 150, 140 + (i * 50), 3, GFX_TEXTRIGHT);
                }
                else {
                    gfxRenderShadowString(m_StartMenuFontId, m_saveFileData.highScores[i-1].playerInitials, (PB_SCREENWIDTH/2) - 140, 140 + (i * 50), 10, GFX_TEXTLEFT, 0, 0, 0, 255, 3);
                    gfxRenderShadowString(m_StartMenuFontId, scoreText, (PB_SCREENWIDTH/2) + 150, 140 + (i * 50), 3, GFX_TEXTRIGHT, 0, 0, 0, 255, 3);
                }
            }
            break;

            case START_OPENDOOR:

                if (!m_PBTBLOpenDoors){
                    gfxAnimateRestart(m_PBTBLLeftDoorId);
                    gfxAnimateRestart(m_PBTBLRightDoorId);
                    m_PBTBLOpenDoors = true;
                }
            break;

        default:
            m_tableScreenState = START_START;
        break;
    }

    // If timeout happens, switch back to "Press Start"
    if ((timeoutTicks > 0) && (m_tableScreenState != START_START) && (m_tableScreenState != START_OPENDOOR)) {
        timeoutTicks -= (currentTick - lastTick);
        if (timeoutTicks <= 0) {
            m_tableScreenState = START_START;
        }
    }

    return (true);
}

bool PBEngine::pbeRenderGameScreen(unsigned long currentTick, unsigned long lastTick){
    
    switch (m_tableState) {
        case PBTBL_START: return pbeRenderGameStart(currentTick, lastTick); break;
        default: return (false); break;
    }

    return (false);
}

// Main State Loop for Pinball Table

void PBEngine::pbeUpdateGameState(stInputMessage inputMessage){
    
    switch (m_tableState) {
        case PBTBL_START: {

            if (m_PBTBLOpenDoors) {
                if ((!gfxAnimateActive(m_PBTBLLeftDoorId)) && (!gfxAnimateActive(m_PBTBLRightDoorId))) m_tableState = PBTBL_STDPLAY; 
            } 
            else {
                if (inputMessage.inputType == PB_INPUT_BUTTON && inputMessage.inputState == PB_ON) {
                    if (inputMessage.inputId != IDI_START) {
                        switch (m_tableScreenState) {
                            case START_START: m_tableScreenState = START_INST; break;
                            case START_INST: m_tableScreenState = START_SCORES; break;
                            case START_SCORES: m_tableScreenState = START_START; break;
                            case START_OPENDOOR: m_tableScreenState = START_OPENDOOR; break;
                            default: break;
                        }
                    }
                    else {
                        m_tableScreenState = START_OPENDOOR;
                    }             
                }
            }
            break;
        }
        case PBTBL_STDPLAY: {
            // Check for input messages and update the game state
            int temp = 0;

            break;
        }
        
        default: break;
    }
}


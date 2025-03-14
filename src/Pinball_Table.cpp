
#include "PInball_Table.h"

// PBEgine Class Fucntions for the main pinball game

bool PBEngine::pbeLoadPlayGame(){
    
    // Scene is loaded but maybe the texures are not
    if (m_PBTBLStartLoaded) 
    {
        // Check that the textures are loaded
        if (!gfxTextureLoaded(m_PBTBLStartDoorId)) {
            if (!gfxReloadTexture(m_PBTBLStartDoorId)) return (false);
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

    m_PBTBLStartDoorId = gfxLoadSprite("Door", "src/resources/textures/startdoor.bmp", GFX_BMP, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLStartDoorId, 255, 255, 255, 192);

    m_PBTBLFlame1Id = gfxLoadSprite("Flame1", "src/resources/textures/flame1.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, false, true);
    gfxSetColor(m_PBTBLFlame1Id, 255, 255, 255, 92);
    gfxSetScaleFactor(m_PBTBLFlame1Id, 0.75, false);

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
    gfxLoadAnimateData(&animateData, m_StartMenuFontId, m_PBTBLTextStartId, m_PBTBLTextEndId, 0, ANIMATE_COLOR_MASK, 3.0f, 0.0f, true, true, GFX_NOLOOP);
    gfxCreateAnimation(animateData, true);

    if (m_PBTBLStartDoorId == NOSPRITE || m_PBTBLFlame1Id == NOSPRITE || m_PBTBLFlame2Id == NOSPRITE ||  
        m_PBTBLFlame3Id == NOSPRITE) return (false);

    m_PBTBLStartLoaded = true;

    return (m_PBTBLStartLoaded);
}

bool PBEngine::pbeRenderPlayGame(unsigned long currentTick, unsigned long lastTick){

    static int timeoutTicks, blinkCountTicks;
    static PBTBLScreenState lastScreenState;
    static bool blinkOn;

    if (m_RestartTable) {
        m_RestartTable = false;
        timeoutTicks = 10000;
        blinkCountTicks = 1000;
        blinkOn = true;
        lastScreenState = m_tableScreenState;
    }

    if (!pbeLoadScreen (PB_PLAYGAME)) return (false); 
    
    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);

    // Show the door image with the flames
    gfxRenderSprite(m_PBTBLStartDoorId, 0, 0);

    gfxSetColor(m_StartMenuFontId, 255 ,165, 0, 255);
    gfxSetScaleFactor(m_StartMenuFontId, 1.25, false);
    gfxRenderShadowString(m_StartMenuFontId, "Dragons of Destiny", (PB_SCREENWIDTH/2) + 20, 5, 2, GFX_TEXTCENTER, 0, 0, 0, 255, 3);
    gfxSetScaleFactor(m_StartMenuFontId, 0.5, false);
    gfxRenderShadowString(m_StartMenuFontId, "Designed by Jeff Bock", (PB_SCREENWIDTH/2) + 20, 80, 2, GFX_TEXTCENTER, 0, 0, 0, 255, 3);
    
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
        timeoutTicks = 10000; // Reset the timeout if we change screens
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
            if (gfxAnimateActive(m_StartMenuFontId)) gfxRenderString(m_StartMenuFontId, "Instructions", (PB_SCREENWIDTH/2) + 20, 200, 1, GFX_TEXTCENTER);
            else gfxRenderShadowString (m_StartMenuFontId, "Instructions", (PB_SCREENWIDTH/2) + 20, 200, 1, GFX_TEXTCENTER, 0, 0, 0, 255, 3);
            break;

        case START_SCORES:
            gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
            gfxAnimateSprite(m_StartMenuFontId, currentTick);
            gfxSetScaleFactor(m_StartMenuFontId, 0.8, false);
            if (gfxAnimateActive(m_StartMenuFontId)) gfxRenderString(m_StartMenuFontId, "Scores", (PB_SCREENWIDTH/2) + 20, 200, 1, GFX_TEXTCENTER);
            else gfxRenderShadowString (m_StartMenuFontId, "Scores", (PB_SCREENWIDTH/2) + 20, 200, 1, GFX_TEXTCENTER, 0, 0, 0, 255, 3);
            break;

        default:
            m_tableScreenState = START_START;
        break;
    }

    if (timeoutTicks > 0) {
        timeoutTicks -= (currentTick - lastTick);
        if (timeoutTicks <= 0) {
            m_tableScreenState = START_INST;
        }
    }

    return (true);
}


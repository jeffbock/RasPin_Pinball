
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

    m_PBTBLStartDoorId = gfxLoadSprite("Door", "src/resources/textures/startdoor.bmp", GFX_BMP, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLStartDoorId, 255, 255, 255, 200);

    m_PBTBLFlame1Id = gfxLoadSprite("Flame1", "src/resources/textures/flame1.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, false, true);
    gfxSetColor(m_PBTBLFlame1Id, 255, 255, 255, 128);
    gfxSetScaleFactor(m_PBTBLFlame1Id, 1.0, false);

    m_PBTBLFlame1StartId = gfxInstanceSprite(m_PBTBLFlame1Id);
    gfxSetColor(m_PBTBLFlame1StartId, 255, 255, 255, 128);
    gfxSetScaleFactor(m_PBTBLFlame1StartId, 1.1, false);

    m_PBTBLFlame1EndId = gfxInstanceSprite(m_PBTBLFlame1Id);
    gfxSetColor(m_PBTBLFlame1EndId, 255, 255, 255, 128);
    gfxSetScaleFactor(m_PBTBLFlame1EndId, 1.3, false);

    m_PBTBLFlame2Id = gfxLoadSprite("Flame2", "src/resources/textures/flame2.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, false, true);
    gfxSetColor(m_PBTBLFlame1Id, 255, 255, 255, 128);
    gfxSetScaleFactor(m_PBTBLFlame1Id, 1.0, false);

    m_PBTBLFlame2StartId = gfxInstanceSprite(m_PBTBLFlame2Id);
    gfxSetColor(m_PBTBLFlame2StartId, 255, 255, 255, 128);
    gfxSetScaleFactor(m_PBTBLFlame2StartId, 0.9, false);

    m_PBTBLFlame2EndId = gfxInstanceSprite(m_PBTBLFlame2Id);
    gfxSetColor(m_PBTBLFlame2EndId, 255, 255, 255, 128);
    gfxSetScaleFactor(m_PBTBLFlame2EndId, 1.4, false);
    
    m_PBTBLFlame3Id = gfxLoadSprite("Flame3", "src/resources/textures/flame3.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, false, true);
    gfxSetColor(m_PBTBLFlame1Id, 255, 255, 255, 128);
    gfxSetScaleFactor(m_PBTBLFlame1Id, 1.0, false);

    m_PBTBLFlame3StartId = gfxInstanceSprite(m_PBTBLFlame3Id);
    gfxSetColor(m_PBTBLFlame3StartId, 255, 255, 255, 128);
    gfxSetScaleFactor(m_PBTBLFlame3StartId, 0.85, false);
    
    m_PBTBLFlame3EndId = gfxInstanceSprite(m_PBTBLFlame3Id);
    gfxSetColor(m_PBTBLFlame3EndId, 255, 255, 255, 128);
    gfxSetScaleFactor(m_PBTBLFlame3EndId, 1.25, false);

    if (m_PBTBLStartDoorId == NOSPRITE || m_PBTBLFlame1Id == NOSPRITE || m_PBTBLFlame2Id == NOSPRITE ||  
        m_PBTBLFlame3Id == NOSPRITE) return (false);

    m_PBDefaultBackgroundLoaded = true;

    return (m_PBTBLStartLoaded);
}

bool PBEngine::pbeRenderPlayGame(unsigned long currentTick, unsigned long lastTick){

    if (!pbeLoadScreen (PB_PLAYGAME)) return (false); 
    
       // Show the door image
       gfxRenderSprite(m_PBTBLStartDoorId, 0, 0);

    return (true);
}



// PInball:  A complete pinball framework for building 1/2 scale phyical pinball machines with Raspberry Pi
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#include "PInball.h"

 // Global pinball engine object
PBEngine g_PBEngine;

// Special startup code depending on the platform - add more as needed
// PBInitRender: Intialize the specific window / rendering needed for the platform
// PBProcessInput: Process input for the specifc platform (simulator will need to get input from keystrokes)

// Windows startup and render code
#ifdef EXE_MODE_WINDOWS
#include "PBWinRender.h"

HWND g_WHND = NULL;

// Init the render system for Windows
bool PBInitRender (long width, long height) {

g_WHND = PBInitWinRender (width, height);
if (g_WHND == NULL) return (false);

// For windows, OGLNativeWindows type is HWND
if (!g_PBEngine.oglInit (width, height, g_WHND)) return (false);
if (!g_PBEngine.gfxInit()) return (false);

return (true);
}

// Process input for Windows
// Returns true as long as the application should continue running
// This will be need to be expanding to take input for the windows simluator
bool PBProcessInput() {

    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return (false);
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    
    return (true);
}

#endif

// Raspeberry Pi startup and render code
#ifdef EXE_MODE_RASPI
#include "PBRasPiRender.h"
bool PBInitRender (long width, long height) {

return true;

}
#endif

// End the platform specific code and functions

// Class functions for PBEngine
 PBEngine::PBEngine() {

    m_mainState = PB_BOOTUP;

    m_PBBootupLoaded = false;
    m_PBStartMenuLoaded = false;
    m_PBPlayGameLoaded = false;
    m_PBTestModeLoaded = false;
    m_PBBenchmarkLoaded = false;
    m_PBCreditsLoaded = false;

    m_BootUpConsoleId = NOSPRITE; 
    m_BootUpStarsId = NOSPRITE;
    m_BootUpStarsId2 = NOSPRITE;
    m_BootUpStarsId3 = NOSPRITE;
    m_BootUpStarsId4 = NOSPRITE;

    m_systemFontId = NOSPRITE;

 }

 PBEngine::~PBEngine(){

    // Code later...

}

// Load the screen based on the main state of the game 
bool PBEngine::pbeLoadScreen (PBMainState state){
    switch (state) {
        case PB_BOOTUP: return (pbeLoadBootUp()); break;
        case PB_STARTMENU: return (pbeLoadStartMenu()); break;
        case PB_PLAYGAME: return (pbeLoadPlayGame()); break;
        case PB_TESTMODE: return (pbeLoadTestMode()); break;
        case PB_BENCHMARK: return (pbeLoadBenchmark()); break;
        case PB_CREDITS: return (pbeLoadCredits()); break;
        default: return (false); break;
    }
     
    return (false);
}

// Render the screen based on the main state of the game
bool PBEngine::pbeRenderScreen(unsigned long currentTick, unsigned long lastTick){
    
    switch (m_mainState) {
        case PB_BOOTUP: return pbeRenderBootScreen(currentTick, lastTick); break;
        case PB_STARTMENU: return pbeRenderStartMenu(currentTick, lastTick); break;
        case PB_PLAYGAME: return pbeRenderPlayGame(currentTick, lastTick); break;
        case PB_TESTMODE: return pbeRenderTestMode(currentTick, lastTick); break;
        case PB_BENCHMARK: return pbeRenderBenchmark(currentTick, lastTick); break;
        case PB_CREDITS: return pbeRenderCredits(currentTick, lastTick); break;
        default: return (false); break;
    }

    return (false);
}

// Load reasources for the boot up screen
bool PBEngine::pbeLoadBootUp(){
    if (m_PBBootupLoaded) return (true);

    // Load the bootup screen
    m_BootUpConsoleId = g_PBEngine.gfxLoadSprite("Console", "resources/textures/console.bmp", GFX_BMP, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    g_PBEngine.gfxSetColor(m_BootUpConsoleId, 255, 255, 255, 255);

    m_BootUpStarsId = g_PBEngine.gfxLoadSprite("Stars", "resources/textures/stars.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, true, true);
    g_PBEngine.gfxSetColor(m_BootUpStarsId, 24, 0, 210, 96);
    g_PBEngine.gfxSetScaleFactor(m_BootUpStarsId, 2.0, false);

    m_BootUpStarsId2 = g_PBEngine.gfxInstanceSprite(m_BootUpStarsId);
    g_PBEngine.gfxSetColor(m_BootUpStarsId2, 24, 0, 210, 96);
    g_PBEngine.gfxSetScaleFactor(m_BootUpStarsId2, 0.75, false);

    m_BootUpStarsId3 = g_PBEngine.gfxInstanceSprite(m_BootUpStarsId);
    g_PBEngine.gfxSetColor(m_BootUpStarsId3, 24, 0, 210, 96);
    g_PBEngine.gfxSetScaleFactor(m_BootUpStarsId3, 0.20, false);
    
    m_BootUpStarsId4 = g_PBEngine.gfxInstanceSprite(m_BootUpStarsId);
    g_PBEngine.gfxSetColor(m_BootUpStarsId4, 24, 0, 210, 96);
    g_PBEngine.gfxSetScaleFactor(m_BootUpStarsId4, 0.05, false);

    if (m_BootUpConsoleId == NOSPRITE || m_BootUpStarsId == NOSPRITE || m_BootUpStarsId2 == NOSPRITE ||  m_BootUpStarsId3 == NOSPRITE ||  m_BootUpStarsId4 == NOSPRITE) return (false);

    m_PBBootupLoaded = true;

    return (m_PBBootupLoaded);
}

// Render the bootup screen
bool PBEngine::pbeRenderBootScreen(unsigned long currentTick, unsigned long lastTick){

   if (!g_PBEngine.pbeLoadScreen (PB_BOOTUP)) return (false); 

   float degreesPerTick = -0.001f, tickDiff = 0.0f;
   float degreesPerTick2 = -0.005f;
   float degreesPerTick3 = -0.025f;
   float degreesPerTick4 = -0.75f;
            
   tickDiff = (float)(currentTick - lastTick);

   g_PBEngine.gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
         
   // Show the console background - it's a full screen image
   g_PBEngine.gfxRenderSprite(m_BootUpConsoleId, 0, 0);

   // Show the rotating stars - tunnel-like effect
   g_PBEngine.gfxSetRotateDegrees(m_BootUpStarsId, (degreesPerTick * (float) tickDiff), true);
   g_PBEngine.gfxRenderSprite(m_BootUpStarsId, 400, 240);

   g_PBEngine.gfxSetRotateDegrees(m_BootUpStarsId2, (degreesPerTick2 * (float) tickDiff), true);
   g_PBEngine.gfxRenderSprite(m_BootUpStarsId2, 400, 215);

   g_PBEngine.gfxSetRotateDegrees(m_BootUpStarsId3, (degreesPerTick3 * (float) tickDiff), true);
   g_PBEngine.gfxRenderSprite(m_BootUpStarsId3, 400, 190);

   g_PBEngine.gfxSetRotateDegrees(m_BootUpStarsId4, (degreesPerTick4 * (float) tickDiff), true);
   g_PBEngine.gfxRenderSprite(m_BootUpStarsId4, 400, 165);

   // g_PBEngine.gfxRenderString (m_systemFontId, "This is the first text!!!", 10, 10);

   return (true);
}

bool PBEngine::pbeRenderStartMenu(unsigned long currentTick, unsigned long lastTick){
   return (false);   
}

bool PBEngine::pbeRenderPlayGame(unsigned long currentTick, unsigned long lastTick){
    return (false);   
}

bool PBEngine::pbeRenderTestMode(unsigned long currentTick, unsigned long lastTick){
    return (false);   
}

bool PBEngine::pbeRenderBenchmark(unsigned long currentTick, unsigned long lastTick){
    return (false);   
}

bool PBEngine::pbeRenderCredits(unsigned long currentTick, unsigned long lastTick){
    return (false);   
}
 
bool PBEngine::pbeLoadStartMenu(){
    return (false);
}

bool PBEngine::pbeLoadPlayGame(){
    return (false);
}

bool PBEngine::pbeLoadTestMode(){
    return (false);
}

bool PBEngine::pbeLoadBenchmark(){
    return (false);
}

bool PBEngine::pbeLoadCredits(){
    return (false);
}

// Main program start
int main(int argc, char const *argv[])
{
    bool isBlack = true;
    
    // Initialize the platform specific render system
    if (!PBInitRender (PB_SCREENWIDTH, PB_SCREENHEIGHT)) return (false);

    // Main loop for the pinball game                                
    unsigned long currentTick = GetTickCount64();
    unsigned long lastTick = currentTick;

    while (true) {
        if (!PBProcessInput()) return (0);

        currentTick = GetTickCount64();

        g_PBEngine.pbeRenderScreen(currentTick, lastTick);   
        g_PBEngine.gfxSwap();

        lastTick = currentTick;
    }

    return 0;
}

// Sample code for using the schrift library
/*

// Load the font
struct SFT sft;
sft = (struct SFT){
    .xScale = 16,
    .yScale = 16,
    .flags = SFT_DOWNWARD_Y,
    .font = schrift_load("path/to/font.ttf")
};

// Render the Glyph
struct SFT_Glyph glyph;
schrift_lookup(&sft, 'A', &glyph);

// Get the bitmap of the glyph
struct SFT_Image image;
schrift_render(&sft, &glyph, &image);

*/
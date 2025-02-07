
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

    // Code later...

 }

 PBEngine::~PBEngine(){

    // Code later...

 }

// Main program start
int main(int argc, char const *argv[])
{
    bool isBlack = true;
    
    // Initialize the platform specific render system
    if (!PBInitRender (PB_SCREENWIDTH, PB_SCREENHEIGHT)) return (false);

    // Create a background sprites for the bootup screen
    unsigned int backgroudId = g_PBEngine.gfxCreateSprite("Console", "resources/textures/console.bmp", GFX_BMP, GFX_UPPERLEFT, 0.2f, false, 1.0f, 0, false);
    // unsigned int ballId = g_PBEngine.gfxCreateSprite("Pinball", "resources/textures/pinball.bmp", GFX_BMP, GFX_CENTER, 1.0f, false, 1.0f, 0, false);

    // Main loop for the pinball game

    while (true) {
        if (!PBProcessInput()) return (0);

         g_PBEngine.gfxClear(0.0f, 0.0f, 0.0f, 0.0f, false);

        // Show the console background
        g_PBEngine.gfxRenderSprite(backgroudId, 0, 0);
        // g_PBEngine.gfxRenderSprite(ballId, 400, 240);

        g_PBEngine.gfxSwap();

        Sleep(100); // Sleep to limit the frame rate to ~10 FPS
    }

    // create a new instance of the pinball class
    //PInball pinball;

    // run the pinball game
    //pinball.run();

    // return 0 to indicate success
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
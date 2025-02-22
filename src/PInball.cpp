
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

// Tranlates windows keys into PBEngine input messages
void PBWinSimInput(char character, PBInputState inputState, stInputMessage* inputMessage){

    inputMessage->inputType = PB_INPUT_BUTTON;
    inputMessage->inputId = 1;
    inputMessage->inputState = inputState;
    inputMessage->instanceTick = GetTickCount64();

}

// Process input for Windows
// Returns true as long as the application should continue running
// This will be need to be expanding to take input for the windows simluator
bool PBProcessInput() {

    // Process Windows Messages
    MSG msg;
    
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return (false);

        TranslateMessage(&msg);
        DispatchMessage(&msg);

        // Handle WM_KEYDOWN message
        if ((msg.message == WM_KEYDOWN) || (msg.message == WM_KEYUP)) {
            // Get the virtual key code
            WPARAM key = msg.wParam;

            // Convert the virtual key code to a character
            char character = MapVirtualKey(key, MAPVK_VK_TO_CHAR);

            // Simulate receiving a message
            stInputMessage inputMessage;
            // Translate input keys to PBMessages and place on message queue
            if (msg.message == WM_KEYDOWN) PBWinSimInput(character, PB_ON, &inputMessage);
            if (msg.message == WM_KEYUP) PBWinSimInput(character, PB_OFF, &inputMessage);

            g_PBEngine.m_inputQueue.push(inputMessage);
        }
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
    m_BootUpTitleBarId = NOSPRITE;

    m_defaultFontSpriteId = NOSPRITE;

    m_maxConsoleLines = 18;
    m_consoleTextHeight = 0;
 }

 PBEngine::~PBEngine(){

    // Code later...

}

// Console functions - basically put strings into the queue
void PBEngine::pbeSendConsole(std::string output){

    m_consoleQueue.push_back(output);

    // If we have too many lines, remove the oldest
    if (m_consoleQueue.size() > m_maxConsoleLines) m_consoleQueue.erase(m_consoleQueue.begin());
}

void PBEngine::pbeClearConsole(){
    while (!m_consoleQueue.empty()) m_consoleQueue.pop_back();
}

void PBEngine::pbeRenderConsole(unsigned int startingX, unsigned int startingY){
    
     // Starting position for rendering
     unsigned int x = startingX;
     unsigned int y = startingY;
 
     // Iterate through the vector and render each string
     for (const auto& line : m_consoleQueue) {
         g_PBEngine.gfxRenderString(m_defaultFontSpriteId, line, x, y, 1);
         y += m_consoleTextHeight + 1; // Move to the next row
     }
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
bool PBEngine::pbeLoadDefaultBackground(){
    if (m_PBDefaultBackgroundLoaded) return (true);

    g_PBEngine.pbeSendConsole("(PI)nball Engine: Loading default background resources");

    m_BootUpConsoleId = g_PBEngine.gfxLoadSprite("Console", "src/resources/textures/console.bmp", GFX_BMP, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    g_PBEngine.gfxSetColor(m_BootUpConsoleId, 255, 255, 255, 96);

    m_BootUpStarsId = g_PBEngine.gfxLoadSprite("Stars", "src/resources/textures/stars.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, true, true);
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

    if (m_BootUpConsoleId == NOSPRITE || m_BootUpStarsId == NOSPRITE || m_BootUpStarsId2 == NOSPRITE ||  
        m_BootUpStarsId3 == NOSPRITE ||  m_BootUpStarsId4 == NOSPRITE ) return (false);

    m_PBDefaultBackgroundLoaded = true;

    return (m_PBDefaultBackgroundLoaded);
}

// Load reasources for the boot up screen
bool PBEngine::pbeLoadBootUp(){
    
    if (!pbeLoadDefaultBackground()) return (false);

    if (m_PBBootupLoaded) return (true);

    g_PBEngine.pbeSendConsole("(PI)nball Engine: Loading boot screen resources");

    // Load the bootup screen items
    
    m_BootUpTitleBarId = g_PBEngine.gfxLoadSprite("Title Bar", "", GFX_NONE, GFX_NOMAP, GFX_UPPERLEFT, false, false);
    g_PBEngine.gfxSetColor(m_BootUpTitleBarId, 0, 0, 255, 255);
    g_PBEngine.gfxSetWH(m_BootUpTitleBarId, 800, 40);

    if (m_BootUpTitleBarId == NOSPRITE) return (false);

    m_PBBootupLoaded = true;

    return (m_PBBootupLoaded);
}

// Render the bootup screen
bool PBEngine::pbeRenderDefaultBackground (unsigned long currentTick, unsigned long lastTick){

    float degreesPerTick = -0.001f, tickDiff = 0.0f;
    float degreesPerTick2 = -0.005f;
    float degreesPerTick3 = -0.025f;
    float degreesPerTick4 = -0.75f;
            
    tickDiff = (float)(currentTick - lastTick);

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

   return (true);

}

// Render the bootup screen
bool PBEngine::pbeRenderBootScreen(unsigned long currentTick, unsigned long lastTick){

   if (!g_PBEngine.pbeLoadScreen (PB_BOOTUP)) return (false); 

   g_PBEngine.gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);

   // Render the default background
   pbeRenderDefaultBackground (currentTick, lastTick);
         
   g_PBEngine.gfxRenderSprite(m_BootUpTitleBarId, 0, 0);
   g_PBEngine.gfxRenderString(m_defaultFontSpriteId, "(PI)nball Engine - Copyright 2024 Jeff Bock", 202, 10, 1);

   g_PBEngine.gfxSetColor(m_defaultFontSpriteId, 256, 256, 256, 256);
   g_PBEngine.pbeRenderConsole(1, 42);

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

void PBEngine::pbeInputLoop(){
    
    // Check the input queue.  If it has a message, pop it and output the character via pbeSendConsole
    while (!m_inputQueue.empty()) {
        stInputMessage inputMessage = m_inputQueue.front();
        m_inputQueue.pop();

        if (inputMessage.inputType == PB_INPUT_BUTTON) {
            if (inputMessage.inputState == PB_ON) {
                std::string temp = "Button " + std::to_string(inputMessage.inputId) + " pressed";
                pbeSendConsole(temp);
            } else {
                std::string temp = "Button " + std::to_string(inputMessage.inputId) + " released";
                pbeSendConsole(temp);
            }
        }
    }
    
    return;
}

// Main program start
int main(int argc, char const *argv[])
{
    bool isBlack = true;
    std::string temp;
    
    g_PBEngine.pbeSendConsole("OpenGL ES: Initialize");
    if (!PBInitRender (PB_SCREENWIDTH, PB_SCREENHEIGHT)) return (false);

    g_PBEngine.pbeSendConsole("OpenGL ES: Successful");

    temp = "Screen Width: " + std::to_string(g_PBEngine.oglGetScreenWidth()) + " Screen Height: " + std::to_string(g_PBEngine.oglGetScreenHeight());
    g_PBEngine.pbeSendConsole(temp);

    g_PBEngine.pbeSendConsole("(PI)nball Engine: Loading system font");

    // Get the system font sprite Id and default height for console
    g_PBEngine.m_defaultFontSpriteId = g_PBEngine.gfxGetSystemFontSpriteId();
    g_PBEngine.m_consoleTextHeight = g_PBEngine.gfxGetTextHeight(g_PBEngine.m_defaultFontSpriteId);

    g_PBEngine.pbeSendConsole("(PI)nball Engine: System font ready");

    // Send a few things to the console
    
    g_PBEngine.pbeSendConsole("(PI)nball Engine: Starting main processing loop");    
    // Main loop for the pinball game                                
    unsigned long currentTick = GetTickCount64();
    unsigned long lastTick = currentTick;

    // Start the input thread
    // std::thread inputThread(&PBProcessInput);

    // The main game engine loop
    while (true) {

        currentTick = GetTickCount64();
        
        // PB Process Input will be a thread in the PiOS side since it won't have windows messages
        // Will need to fix this later...
        PBProcessInput();

        // Process the input message queue
        g_PBEngine.pbeInputLoop();

        g_PBEngine.pbeRenderScreen(currentTick, lastTick);   
        g_PBEngine.gfxSwap();

        lastTick = currentTick;
    }

   // Join the input thread before exiting
   // inputThread.join();

   return 0;
}


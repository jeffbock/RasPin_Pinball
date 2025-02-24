
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
void PBWinSimInput(std::string character, PBPinState inputState, stInputMessage* inputMessage){

    // Find the character in the input definition global
    for (int i = 0; i < NUM_INPUTS; i++) {
        if (g_inputDef[i].simMapKey == character) {
            inputMessage->inputType = g_inputDef[i].inputType;
            inputMessage->inputId = g_inputDef[i].id;
            inputMessage->inputState = inputState;
            inputMessage->instanceTick = GetTickCount64();
            return;
        }
    }
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

            // Convert the virtual key code to a string
            char character = MapVirtualKey(key, MAPVK_VK_TO_CHAR);
            std::string temp = "";
            temp += character;
            
            // Check if the key press is an auto-repeat
            bool isAutoRepeat = (msg.lParam & (1 << 30)) != 0;

            // Simulate receiving a message
            if ((!isAutoRepeat) || (msg.message == WM_KEYUP)) {
                stInputMessage inputMessage;
                // Translate input keys to PBMessages and place on message queue
                if (msg.message == WM_KEYDOWN) PBWinSimInput(temp, PB_ON, &inputMessage);
                if (msg.message == WM_KEYUP) PBWinSimInput(temp, PB_OFF, &inputMessage);

                g_PBEngine.m_inputQueue.push(inputMessage);
            }
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

    m_StartMenuFontId = NOSPRITE;
    m_StartMenuSwordId = NOSPRITE;

    // This size is dependent on the font size and the size of the screen
    m_maxConsoleLines = 18;
    m_consoleTextHeight = 0;

    // Start Menu variables
    m_currentMenuItem = 1;

    m_PassSelfTest = true;
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

    g_PBEngine.pbeSendConsole("(PI)nball Engine: Ready - Press any button to continue");

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

// Menu Screen

bool PBEngine::pbeLoadStartMenu(){

    if (m_PBStartMenuLoaded) return (true);

    // Load the font for the start menu
    m_StartMenuFontId = g_PBEngine.gfxLoadSprite("Start Menu Font", MENUFONT, GFX_PNG, GFX_TEXTMAP, GFX_UPPERLEFT, true, true);
    if (m_StartMenuFontId == NOSPRITE) return (false);

    g_PBEngine.gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);

    m_StartMenuSwordId = g_PBEngine.gfxLoadSprite("Start Menu Sword", MENUSWORD, GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    if (m_StartMenuSwordId == NOSPRITE) return (false);

    g_PBEngine.gfxSetScaleFactor(m_StartMenuSwordId, 0.35, false);
    g_PBEngine.gfxSetColor(m_StartMenuSwordId, 200, 200, 200, 200);

    m_PBStartMenuLoaded = true;

    return (m_PBStartMenuLoaded);
}

bool PBEngine::pbeRenderStartMenu(unsigned long currentTick, unsigned long lastTick){

   if (!g_PBEngine.pbeLoadScreen (PB_STARTMENU)) return (false); 

   g_PBEngine.gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);

    // Render the default background
    pbeRenderDefaultBackground (currentTick, lastTick);
   
    g_PBEngine.gfxSetColor(m_StartMenuFontId, 0, 0, 0, 255);
    g_PBEngine.gfxRenderString(m_StartMenuFontId, MenuTitle, 208, 10, 2);

    g_PBEngine.gfxSetColor(m_StartMenuFontId, 255 ,165, 0, 255);
    g_PBEngine.gfxRenderString(m_StartMenuFontId, MenuTitle, 205, 13, 2);

    unsigned int swordY = 89;
    // Determine where to put the sword cursor and give blue underline to selected text
    switch (m_currentMenuItem) {
        case (1): {
            swordY = 89; 
            g_PBEngine.gfxSetColor(m_StartMenuFontId, 64, 0, 255, 255);
            if (!g_PBEngine.m_PassSelfTest)g_PBEngine.gfxRenderString(m_StartMenuFontId, Menu1Fail, 293, 8, 1);
            else g_PBEngine.gfxRenderString(m_StartMenuFontId, Menu1, 293, 88, 1);
            break;
        }
        case (2): {
            swordY = 154; 
            g_PBEngine.gfxSetColor(m_StartMenuFontId, 64, 0, 255, 255);
            g_PBEngine.gfxRenderString(m_StartMenuFontId, Menu2, 293, 153, 1);
            break;
        }
        case (3): {
            swordY = 219; 
            g_PBEngine.gfxSetColor(m_StartMenuFontId, 64, 0, 255, 255);
            g_PBEngine.gfxRenderString(m_StartMenuFontId, Menu3, 293, 218, 1);
            break;
        }
        case (4): {
            swordY = 284; 
            g_PBEngine.gfxSetColor(m_StartMenuFontId, 64, 0, 255, 255);
            g_PBEngine.gfxRenderString(m_StartMenuFontId, Menu4, 293, 283, 1);
            break;
        }
        case (5): {
            swordY = 349; 
            g_PBEngine.gfxSetColor(m_StartMenuFontId, 64, 0, 255, 255);
            g_PBEngine.gfxRenderString(m_StartMenuFontId, Menu5, 293, 348, 1);
            break;
        }
        case (6): {
            swordY = 414; 
            g_PBEngine.gfxSetColor(m_StartMenuFontId, 64, 0, 255, 255);
            g_PBEngine.gfxRenderString(m_StartMenuFontId, Menu6, 293, 413, 1);
            break;
        }
        default: break;
    }

    g_PBEngine.gfxSetColor(m_StartMenuFontId, 200, 200, 200, 224);
    if (!g_PBEngine.m_PassSelfTest)g_PBEngine.gfxRenderString(m_StartMenuFontId, Menu1Fail, 290, 85, 1);
    else g_PBEngine.gfxRenderString(m_StartMenuFontId, Menu1, 290, 85, 1);
    g_PBEngine.gfxRenderString(m_StartMenuFontId, Menu2, 290, 150, 1);
    g_PBEngine.gfxRenderString(m_StartMenuFontId, Menu3, 290, 215, 1);
    g_PBEngine.gfxRenderString(m_StartMenuFontId, Menu4, 290, 280, 1);
    g_PBEngine.gfxRenderString(m_StartMenuFontId, Menu5, 290, 345, 1);
    g_PBEngine.gfxRenderString(m_StartMenuFontId, Menu6, 290, 410, 1);

    // Add insturctions to the bottom of the screen
    g_PBEngine.gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
    g_PBEngine.gfxRenderString(m_defaultFontSpriteId, "L/R flip = move", 615, 430, 1);
    g_PBEngine.gfxRenderString(m_defaultFontSpriteId, "L/R active = select", 615, 455, 1);

    g_PBEngine.gfxRenderSprite(m_StartMenuSwordId, 240, swordY);
          
    return (true);
}

bool PBEngine::pbeRenderPlayGame(unsigned long currentTick, unsigned long lastTick){
    return (false);   
}

// Test Mode Screren
bool PBEngine::pbeLoadTestMode(){
    // Test mode currently only requires the default background and font
    if (!pbeLoadDefaultBackground()) return (false);

    m_PBTestModeLoaded = true;
    return (m_PBTestModeLoaded);
}

bool PBEngine::pbeRenderTestMode(unsigned long currentTick, unsigned long lastTick){

    if (!g_PBEngine.pbeLoadScreen (PB_TESTMODE)) return (false); 

    g_PBEngine.gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
    
    // Render the default background
    pbeRenderDefaultBackground (currentTick, lastTick);
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    gfxRenderString(m_defaultFontSpriteId, "Test Playfield Inputs/Outputs - Press START to EXIT", 140, 4, 1);

    g_PBEngine.gfxRenderString(m_defaultFontSpriteId, "INPUTS", 10, 30, 1);

    // These input / ouput lists will need to be adjusted if there end up being too many items - add a second row
    // Hopefully can fit  within two columns each, otherwise, need to shrink font or re-think how to display.

    // Go through each of the input defs and print the state
    for (int i = 0; i < NUM_INPUTS; i++) {
        std::string temp = g_inputDef[i].inputName;
        gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
        g_PBEngine.gfxRenderString(m_defaultFontSpriteId, temp, 10, 60 + (i * 22), 1);
        
        // Print the state of the input (and highlight in RED) if ON
        if (g_inputDef[i].lastState == PB_ON) {
            gfxSetColor(m_defaultFontSpriteId, 255,0, 0, 255);
            temp = "ON";
        }
        else {
            temp = "OFF";
            gfxSetColor(m_defaultFontSpriteId, 255,255, 255, 255);
        };
        g_PBEngine.gfxRenderString(m_defaultFontSpriteId, temp, 160, 60 + (i * 22), 1);
    }

    // Go through each of the output defs and print the state
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    g_PBEngine.gfxRenderString(m_defaultFontSpriteId, "OUTPUTS", 410, 30, 1);

    for (int i = 0; i < NUM_OUTPUTS; i++) {
        std::string temp = g_outputDef[i].outputName;
        gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
        g_PBEngine.gfxRenderString(m_defaultFontSpriteId, temp, 410, 60 + (i * 22), 1);
        
        // Print the state of the input (and highlight in RED) if ON
        if (g_outputDef[i].lastState == PB_ON) {
            gfxSetColor(m_defaultFontSpriteId, 255,0, 0, 255);
            temp = "ON";
        }
        else {
            temp = "OFF";
            gfxSetColor(m_defaultFontSpriteId, 255,255, 255, 255);
        };
        g_PBEngine.gfxRenderString(m_defaultFontSpriteId, temp, 560, 60 + (i * 22), 1);
    }
    
    return (true);   
}

bool PBEngine::pbeRenderBenchmark(unsigned long currentTick, unsigned long lastTick){
    return (false);   
}

bool PBEngine::pbeRenderCredits(unsigned long currentTick, unsigned long lastTick){
    return (false);   
}
 
bool PBEngine::pbeLoadPlayGame(){
    return (false);
}

bool PBEngine::pbeLoadBenchmark(){
    return (false);
}

bool PBEngine::pbeLoadCredits(){
    return (false);
}

void PBEngine::pbeUpdateState(stInputMessage inputMessage){
    
    switch (m_mainState) {
        case PB_BOOTUP: {
            // If any button is pressed, move to the start menu
            if (inputMessage.inputType == PB_INPUT_BUTTON && inputMessage.inputState == PB_ON) {
                m_mainState = PB_STARTMENU;
            }
            break;
        }
        case PB_STARTMENU: {
            // If either left button is pressed, subtract 1 from m_currentMenuItem
            if (inputMessage.inputType == PB_INPUT_BUTTON && inputMessage.inputState == PB_ON) {
                if (inputMessage.inputId == IDI_LEFTFLIPPER) {
                    if (m_currentMenuItem > 1) m_currentMenuItem--;
                }
                // If either right button is pressed, add 1 to m_currentMenuItem
                if (inputMessage.inputId == IDI_RIGHTFLIPPER) {
                    if (m_currentMenuItem < 6) m_currentMenuItem++;
                }

                if ((inputMessage.inputId == IDI_RIGHTACTIVATE) || (inputMessage.inputId == IDI_LEFTACTIVATE)) {
                    // Do something based on the menu item
                    switch (m_currentMenuItem) {
                        case (1):  if (m_PassSelfTest) m_mainState = PB_PLAYGAME; break;
                        case (2):  m_mainState = PB_SETTINGS; break;
                        case (3):  m_mainState = PB_TESTMODE; break;
                        case (4):  m_mainState = PB_BENCHMARK; break;
                        case (5):  m_mainState = PB_BOOTUP; break;
                        case (6):  m_mainState = PB_CREDITS; break;
                        default: break;
                    }
                }                
            }
            break;
        }
        case PB_TESTMODE: {
            // If the start button has been pressed, return to the start menu
            if (inputMessage.inputId == IDI_START && inputMessage.inputState == PB_ON) {
                m_mainState = PB_STARTMENU;
            }
            // Otherwise, need to set the variables that tell the game to test the inputs and outputs
            break;
        }
        default: break;
    }
}

// Main program start
int main(int argc, char const *argv[])
{
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
    g_PBEngine.pbeSendConsole("(PI)nball Engine: Starting main processing loop");    
   
    // Check the input definitions and ensure no duplicates
    // Need to change this to a self test function - will need to set up Raspberry I/O and breakout boards
    for (int i = 0; i < NUM_INPUTS; i++) {
        if (i == 0) g_PBEngine.pbeSendConsole("(PI)nball Engine: Total Inputs: " + std::to_string(NUM_INPUTS)); 
        for (int j = i + 1; j < NUM_INPUTS; j++) {
            if (g_inputDef[i].id == g_inputDef[j].id) {
                g_PBEngine.pbeSendConsole("(PI)nball Engine: ERROR: Duplicate input ID: " + std::to_string(g_inputDef[i].id));
                g_PBEngine.m_PassSelfTest = false;
            }
        }
    }

    // Check the output definitions and ensure no duplicates
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        if (i == 0) g_PBEngine.pbeSendConsole("(PI)nball Engine: Total Outputs: " + std::to_string(NUM_OUTPUTS)); 
        for (int j = i + 1; j < NUM_OUTPUTS; j++) {
            if (g_outputDef[i].id == g_outputDef[j].id) {
                g_PBEngine.pbeSendConsole("(PI)nball Engine: ERROR: Duplicate output ID: " + std::to_string(g_outputDef[i].id));
                g_PBEngine.m_PassSelfTest = false;
            }
        }
    }

    // Main loop for the pinball game                                
    unsigned long currentTick = GetTickCount64();
    unsigned long lastTick = currentTick;

    // Start the input thread
    // std::thread inputThread(&PBProcessInput);

    // The main game engine loop
    while (true) {

        currentTick = GetTickCount64();
        stInputMessage inputMessage;
        
        // PB Process Input will be a thread in the PiOS side since it won't have windows messages
        // Will need to fix this later...
        PBProcessInput();

        // Process the input message queue
        if (!g_PBEngine.m_inputQueue.empty()){
            inputMessage = g_PBEngine.m_inputQueue.front();
            g_PBEngine.m_inputQueue.pop();

            // Update the game state based on the input message
            g_PBEngine.pbeUpdateState (inputMessage); 
        }
        
        g_PBEngine.pbeRenderScreen(currentTick, lastTick);   
        g_PBEngine.gfxSwap();

        lastTick = currentTick;
    }

   // Join the input thread before exiting
   // inputThread.join();

   return 0;
}



// PInball:  A complete pinball framework for building 1/2 scale phyical pinball machines with Raspberry Pi

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "PInball.h"
#include "PInballMenus.h"

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
            inputMessage->sentTick = g_PBEngine.GetTickCountGfx();

            // Update the various state items for the input, could be used by the progam later
            if (g_inputDef[i].lastState == inputState) g_inputDef[i].timeInState += (inputMessage->sentTick - g_inputDef[i].lastStateTick);
            else g_inputDef[i].timeInState = 0;
            g_inputDef[i].lastState = inputState;
            g_inputDef[i].lastStateTick = inputMessage->sentTick;

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

bool PBProcessOutput() {
    return (true);
}

#endif

// Raspeberry Pi startup and render code
#ifdef EXE_MODE_RASPI
#include "PBRasPiRender.h"

EGLNativeWindowType g_PiWindow;

bool PBInitRender (long width, long height) {

g_PiWindow = PBInitPiRender (width, height);
if (g_PiWindow == 0) return (false);

// For Rasberry Pi, OGLNativeWindows type is TBD
if (!g_PBEngine.oglInit (width, height, g_PiWindow)) return (false);
if (!g_PBEngine.gfxInit()) return (false);

return true;

}

bool  PBProcessInput() {

    stInputMessage inputMessage;
    // Loop through all the inputs in m_inputMap and, read them, check for state change, and send a message if changed
    for (auto& inputPair : g_PBEngine.m_inputMap) {
        int inputId = inputPair.first;
        cDebounceInput& input = inputPair.second;

        // Read the current state of the input
        int currentState = input.readPin();

        // Check if the state has changed
        if (currentState != g_inputDef[inputId].lastState) {
            // Create an input message
            inputMessage.inputType = g_inputDef[inputId].inputType;
            inputMessage.inputId = g_inputDef[inputId].id;
            if (currentState == 0) {
                inputMessage.inputState = PB_ON;
                g_inputDef[inputId].lastState = PB_ON;
            }
            else{
                inputMessage.inputState = PB_OFF;
                g_inputDef[inputId].lastState = PB_OFF;
            } 
            inputMessage.sentTick = g_PBEngine.GetTickCountGfx();
            
            g_PBEngine.m_inputQueue.push(inputMessage);
        }
    }

    return (true);
}

bool PBProcessOutput() {

    // Pop the message from output queue and process it
    // This is a simple loop for now, but will more complicated later
    // The message queue should actually automatically manage on/off times and control of LEDs without main loop interaction

    // Pop the entry from the m_outputQueue
    while (!g_PBEngine.m_outputQueue.empty())
    {    
        stOutputMessage tempMessage = g_PBEngine.m_outputQueue.front();
        g_PBEngine.m_outputQueue.pop();

        // Process the mesage - we only support the one LED right now
        if (tempMessage.outputId == PB_OUTPUT_LED) {
            // Set the LED state
            if (tempMessage.outputState == PB_ON) {
                // Set the LED on
                digitalWrite(g_outputDef[PB_OUTPUT_LED].pin,HIGH); 
            }
            else {
                // Set the LED off
                 digitalWrite(g_outputDef[PB_OUTPUT_LED].pin,LOW); 
            }
        }
        
    }
    return (true);
}

#endif

// End the platform specific code and functions

// Class functions for PBEngine
 PBEngine::PBEngine() {

    m_mainState = PB_BOOTUP;

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
    m_CurrentMenuItem = 0;
    m_RestartMenu = true;
    m_GameStarted = false;

    // Setting Menu variables - at some points these should saved to a file and loaded from a file
    m_CurrentSettingsItem = 0;
    m_RestartSettings = true;

    // Diagnostics Menu variables
    m_CurrentDiagnosticsItem = 0;
    m_EnableOverlay = false;
    m_RestartDiagnostics = true;

    // Credits screen variables
    m_CreditsScrollY = 480;
    m_TicksPerPixel = 30;
    m_RestartCredits = true;

    // Benchmark variables
    m_TicksPerScene = 10000; m_BenchmarkStartTick = 0;  m_CountDownTicks = 4000; m_BenchmarkDone = false;
    m_RestartBenchmark = true;

    // Test Mode variables
    m_TestMode = PB_TESTINPUT;
    m_LFON = false; m_RFON=false; m_LAON =false; m_RAON = false; 
    m_CurrentOutputItem = 0;
    m_RestartTestMode = true;

    m_PassSelfTest = true;

    /////////////////////
    // Table variables
    /////////////////////
    m_tableState = PBTableState::PBTBL_START; 
    m_tableScreenState = PBTBLScreenState::START_START;

    // Tables start screen variables
    m_PBTBLStartDoorId=0; m_PBTBLFlame1Id=0; m_PBTBLFlame2Id=0; m_PBTBLFlame3Id=0;
    m_RestartTable = true;
 }

 PBEngine::~PBEngine(){

    // Code later...

}

bool PBEngine::pbeLoadSaveFile(bool loadDefaults, bool resetScores){
    
    // Try and load the save file - if it exists, load it, place it in the stSaveFileData structure and return true
    // otherwise load the defaults if not present, or if loadDefaults is true

    bool failed = false;

    std::ifstream saveFile(SAVEFILENAME, std::ios::binary);
    if (saveFile) {
        saveFile.read(reinterpret_cast<char*>(&m_saveFileData), sizeof(stSaveFileData));
        saveFile.close();
    } else failed = true;
        
    if ((loadDefaults) || failed){
        // Set default values for the saveData structure
        m_saveFileData.mainVolume = MAINVOLUME_DEFAULT;
        m_saveFileData.musicVolume = MUSICVOLUME_DEFAULT;
        m_saveFileData.ballsPerGame = BALLSPERGAME_DEFAULT;
        m_saveFileData.difficulty = PB_NORMAL;
    }

    // Set the stHighScoreData array scores to zero and name to "JEF"
    if ((resetScores) || failed) resetHighScores();
    
    return (!failed);
}

void PBEngine::resetHighScores(){
    // Reset the high scores to zero and initials to "JEF"
    for (int i = 0; i < NUM_HIGHSCORES; i++) {
        m_saveFileData.highScores[i].highScore = 0;
        m_saveFileData.highScores[i].playerInitials = "JEF";
    }
}

bool PBEngine::pbeSaveFile(){
    
    // Save the current settings and high scores to the save file, overwriting any previous data
    std::ofstream saveFile(SAVEFILENAME, std::ios::binary);
    if (!saveFile) return (false); // Failed to open the file for writing

    saveFile.write(reinterpret_cast<const char*>(&m_saveFileData), sizeof(stSaveFileData));
    saveFile.close();

    return (true);
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
         gfxRenderString(m_defaultFontSpriteId, line, x, y, 1, GFX_TEXTLEFT);
         y += m_consoleTextHeight + 1; // Move to the next row
     }
}

// Render the screen based on the main state of the game
// Play Game is final state right now for the menu screens.  If pinball ever exits, then we'd need to change this
bool PBEngine::pbeRenderScreen(unsigned long currentTick, unsigned long lastTick){
    
    switch (m_mainState) {
        case PB_BOOTUP: return pbeRenderBootScreen(currentTick, lastTick); break;
        case PB_STARTMENU: return pbeRenderStartMenu(currentTick, lastTick); break;
        case PB_PLAYGAME: return (true); break;
        case PB_TESTMODE: return pbeRenderTestMode(currentTick, lastTick); break;
        case PB_BENCHMARK: return pbeRenderBenchmark(currentTick, lastTick); break;
        case PB_CREDITS: return pbeRenderCredits(currentTick, lastTick); break;
        case PB_SETTINGS: return pbeRenderSettings(currentTick, lastTick); break;
        case PB_DIAGNOSTICS: return pbeRenderDiagnostics(currentTick, lastTick); break;
        default: return (false); break;
    }

    return (false);
}

// Load reasources for the boot up screen
bool PBEngine::pbeLoadDefaultBackground(bool forceReload){
    
    static bool defaultBackgroundLoaded = false;
    if (forceReload) defaultBackgroundLoaded = false;
    if (defaultBackgroundLoaded) return (true);

    pbeSendConsole("(PI)nball Engine: Loading default background resources");

    m_BootUpConsoleId = gfxLoadSprite("Console", "src/resources/textures/ConsoleLarge.bmp", GFX_BMP, GFX_NOMAP, GFX_UPPERLEFT, false, true);
    gfxSetColor(m_BootUpConsoleId, 255, 255, 255, 196);
    gfxSetScaleFactor(m_BootUpConsoleId, 2.0, false);

    m_BootUpStarsId = gfxLoadSprite("Stars", "src/resources/textures/stars.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, false, true);
    gfxSetColor(m_BootUpStarsId, 24, 0, 210, 96);
    gfxSetScaleFactor(m_BootUpStarsId, 4.0, false);

    m_BootUpStarsId2 = gfxInstanceSprite(m_BootUpStarsId);
    gfxSetColor(m_BootUpStarsId2, 24, 0, 210, 96);
    gfxSetScaleFactor(m_BootUpStarsId2, 1.5, false);

    m_BootUpStarsId3 = gfxInstanceSprite(m_BootUpStarsId);
    gfxSetColor(m_BootUpStarsId3, 24, 0, 210, 96);
    gfxSetScaleFactor(m_BootUpStarsId3, 0.4, false);
    
    m_BootUpStarsId4 = gfxInstanceSprite(m_BootUpStarsId);
    gfxSetColor(m_BootUpStarsId4, 24, 0, 210, 96);
    gfxSetScaleFactor(m_BootUpStarsId4, 0.1, false); 

    if (m_BootUpConsoleId == NOSPRITE || m_BootUpStarsId == NOSPRITE || m_BootUpStarsId2 == NOSPRITE ||  
        m_BootUpStarsId3 == NOSPRITE ||  m_BootUpStarsId4 == NOSPRITE ) return (false);

    defaultBackgroundLoaded = true;

    return (defaultBackgroundLoaded);
}

// Load reasources for the boot up screen
bool PBEngine::pbeLoadBootUp(bool forceReload){
    
    static bool bootUpLoaded = false;
    if (forceReload) bootUpLoaded = false;
    if (bootUpLoaded) return (true);

    if (!pbeLoadDefaultBackground(forceReload)) return (false);

    pbeSendConsole("(PI)nball Engine: Loading boot screen resources");
    
    // Load the bootup screen items
    
    m_BootUpTitleBarId = gfxLoadSprite("Title Bar", "", GFX_NONE, GFX_NOMAP, GFX_UPPERLEFT, false, false);
    gfxSetColor(m_BootUpTitleBarId, 0, 0, 255, 255);
    gfxSetWH(m_BootUpTitleBarId, PB_SCREENWIDTH, 40);

    if (m_BootUpTitleBarId == NOSPRITE) return (false);

    pbeSendConsole("(PI)nball Engine: Ready - Press any button to continue");

    bootUpLoaded = true;
    return (bootUpLoaded);
}

// Render the bootup screen
bool PBEngine::pbeRenderDefaultBackground (unsigned long currentTick, unsigned long lastTick){

    float degreesPerTick = -0.001f, tickDiff = 0.0f;
    float degreesPerTick2 = -0.005f;
    float degreesPerTick3 = -0.025f;
    float degreesPerTick4 = -0.075f;
            
    tickDiff = (float)(currentTick - lastTick);

   // Show the console background - it's a full screen image
   gfxRenderSprite(m_BootUpConsoleId, 0, 0);

   // Show the rotating stars - tunnel-like effect
   gfxSetRotateDegrees(m_BootUpStarsId, (degreesPerTick * (float) tickDiff), true);
   gfxRenderSprite(m_BootUpStarsId, PB_SCREENWIDTH/2 - 15, (PB_SCREENHEIGHT / 2) + 190
);

   gfxSetRotateDegrees(m_BootUpStarsId2, (degreesPerTick2 * (float) tickDiff), true);
   gfxRenderSprite(m_BootUpStarsId2, PB_SCREENWIDTH/2 - 15, (PB_SCREENHEIGHT / 2) + 175);

   gfxSetRotateDegrees(m_BootUpStarsId3, (degreesPerTick3 * (float) tickDiff), true);
   gfxRenderSprite(m_BootUpStarsId3, PB_SCREENWIDTH/2 - 15, (PB_SCREENHEIGHT / 2) + 140);

   gfxSetRotateDegrees(m_BootUpStarsId4, (degreesPerTick4 * (float) tickDiff), true);
   gfxRenderSprite(m_BootUpStarsId4, PB_SCREENWIDTH/2 - 15, (PB_SCREENHEIGHT / 2) + 115);

   return (true);
}

// Render the bootup screen
bool PBEngine::pbeRenderBootScreen(unsigned long currentTick, unsigned long lastTick){
        
    if (!pbeLoadBootUp(false)) return (false);

    if (m_RestartBootUp) {
        m_RestartBootUp = false;
    }

    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);

    // Render the default background
    pbeRenderDefaultBackground (currentTick, lastTick);
         
    gfxRenderSprite(m_BootUpTitleBarId, 0, 0);
    gfxRenderShadowString(m_defaultFontSpriteId, "(PI)nball Engine - Copyright 2025 Jeff Bock", (PB_SCREENWIDTH / 2), 10, 1, GFX_TEXTCENTER, 0, 0, 0, 255, 2);

    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);   
    pbeRenderConsole(1, 42);

    return (true);
}

// Function generically renders a menu with a cursor at an x/y location, simplifies the use of creating new menus

bool PBEngine::pbeRenderGenericMenu(unsigned int cursorSprite, unsigned int fontSprite, unsigned int selectedItem, 
                                    int x, int y, int lineSpacing, std::map<unsigned int, std::string>* menuItems,
                                    bool useShadow, bool useCursor, unsigned int redShadow, 
                                    unsigned int greenShadow, unsigned int blueShadow, unsigned int alphaShadow, unsigned int shadowOffset){

    // Check that the cursor sprite and font sprite are valid, and fontSprite is actually a font
    if ((gfxIsSprite(cursorSprite) == false) && (useCursor ==  true)) return (false);
    if (gfxIsFontSprite(fontSprite) == false) return (false);

    unsigned int cursorX = x, cursorY = y;
    unsigned int menuX = x, menuY = y;
    
    // Get the cursor and font scale factors
    float cursorScale = 0; 
    int cursorWidth = 0, cursorHeight  = 0; 
    unsigned int cursorCenterOffset = 0;

    // If using the cursor, get the metrics, if not using, then scale factors will be 0, but they will not be used anywhere
    if (useCursor) {
        cursorScale = gfxGetScaleFactor(cursorSprite);
        cursorHeight = gfxGetBaseHeight(cursorSprite);
        cursorWidth = gfxGetBaseWidth(cursorSprite);    

         // Calculate the scaled width and height of the cursor
        cursorWidth = (int)((float)cursorWidth * cursorScale);
        cursorHeight = (int)((float)cursorHeight * cursorScale);
    }
    
    // Calculate the scaled max height of the font
    float fontScale = gfxGetScaleFactor(fontSprite);
    int fontHeight  = gfxGetTextHeight(fontSprite);
    fontHeight = (int)((float)fontHeight * fontScale);

    cursorCenterOffset = (fontHeight - cursorHeight) / 2;

    // Get the count of items in the menu, then loop through each of the items and render them.  If the item is selected, render the cursor and make use the shadow text
    unsigned int itemIndex = 0;

    for (auto& item : *menuItems) {
        // Get the item text and calculate the width
        std::string itemText = item.second;
        unsigned int itemWidth = gfxStringWidth(fontSprite, itemText, 1);

        // Calculate the x position of the menu item
        menuY = y + (itemIndex * (fontHeight + lineSpacing));

        // Render the menu item with shadow depending on the selected item
        if (selectedItem == item.first) {
            if (useShadow) gfxRenderShadowString(fontSprite, itemText, menuX + cursorWidth + CURSOR_TO_MENU_SPACING, menuY, 1, GFX_TEXTLEFT, redShadow, greenShadow, blueShadow, alphaShadow, shadowOffset);
            else gfxRenderString(fontSprite, itemText, menuX + cursorWidth + CURSOR_TO_MENU_SPACING, menuY, 1, GFX_TEXTLEFT);

            if (useCursor) gfxRenderSprite (cursorSprite, cursorX, menuY + cursorCenterOffset);
        } else {
            gfxRenderString(fontSprite, itemText, menuX + cursorWidth + CURSOR_TO_MENU_SPACING, menuY, 1, GFX_TEXTLEFT);
        }
        itemIndex++;
    }
    
    return (true);
}

// Main Menu Screen Setup

bool PBEngine::pbeLoadStartMenu(bool forceReload){

    static bool startMenuLoaded = false;
    if (forceReload) startMenuLoaded = false;
    if (startMenuLoaded) return (true);

    // Load the font for the start menu
    m_StartMenuFontId = gfxLoadSprite("Start Menu Font", MENUFONT, GFX_PNG, GFX_TEXTMAP, GFX_UPPERLEFT, true, true);
    if (m_StartMenuFontId == NOSPRITE) return (false);

    gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);

    m_StartMenuSwordId = gfxLoadSprite("Start Menu Sword", MENUSWORD, GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, false, true);
    if (m_StartMenuSwordId == NOSPRITE) return (false);

    gfxSetScaleFactor(m_StartMenuSwordId, 0.35, false);
    gfxSetColor(m_StartMenuSwordId, 200, 200, 200, 200);

    startMenuLoaded = true;

    return (startMenuLoaded);
}

// Renders the main menu
bool PBEngine::pbeRenderStartMenu(unsigned long currentTick, unsigned long lastTick){

   if (!pbeLoadStartMenu (false)) return (false); 

   if (m_RestartMenu) {
        m_CurrentSettingsItem = 0; 
        m_RestartMenu = false;
        gfxSetScaleFactor(m_StartMenuSwordId, 0.9, false);
        gfxSetRotateDegrees(m_StartMenuSwordId, 0.0f, false);
    } 
    
    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);

    // Render the default background
    pbeRenderDefaultBackground (currentTick, lastTick);

    int tempX = PB_SCREENWIDTH / 2;

    gfxSetColor(m_StartMenuFontId, 255 ,165, 0, 255);
    gfxSetScaleFactor(m_StartMenuFontId, 2.0, false);
    gfxRenderShadowString(m_StartMenuFontId, MenuTitle, tempX, 15, 2, GFX_TEXTCENTER, 0, 0, 0, 255, 6);
    gfxSetScaleFactor(m_StartMenuFontId, 1.5, false);

    gfxSetColor(m_StartMenuFontId, 255 ,255, 255, 255);

    // Render the menu items with shadow depending on the selected item
    pbeRenderGenericMenu(m_StartMenuSwordId, m_StartMenuFontId, m_CurrentMenuItem, 620, 260, 25, &g_mainMenu, true, true, 64, 0, 255, 255, 8);

    // Add insturctions to the bottom of the screen - calculate the x position based on string length
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "L/R flip = move", PB_SCREENWIDTH - 200, PB_SCREENHEIGHT - 50, 1, GFX_TEXTLEFT, 0,0,0,255,2);
    gfxRenderShadowString(m_defaultFontSpriteId, "L/R active = select", PB_SCREENWIDTH - 200, PB_SCREENHEIGHT - 25, 1, GFX_TEXTLEFT, 0,0,0,255,2);

    return (true);
}

// Test Mode Screen
bool PBEngine::pbeLoadTestMode(bool forceReload){
    // Test mode currently only requires the default background and font
    if (!pbeLoadDefaultBackground(false)) return (false);

    return (true);
}

bool PBEngine::pbeRenderTestMode(unsigned long currentTick, unsigned long lastTick){

    if (!pbeLoadTestMode (false)) return (false); 

    if (m_RestartTestMode) {
        m_LFON = false; m_RFON=false; m_LAON =false; m_RAON = false;
        m_CurrentOutputItem = 0;
        m_TestMode = PB_TESTINPUT;
        m_RestartTestMode = false;
    }

    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
    
    // Render the default background
    pbeRenderDefaultBackground (currentTick, lastTick);
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "Test Playfield I/O: [LF+RF] Toggle I/O, [LA+RA] Exit", (PB_SCREENWIDTH /2), 4, 1, GFX_TEXTCENTER, 0, 0, 0, 255, 2);

    int limit = 0;
    std::string temp;

    if (m_TestMode == PB_TESTINPUT) limit = NUM_INPUTS;
    else limit = NUM_OUTPUTS;

    gfxSetColor(m_defaultFontSpriteId, 255,255, 255, 255);

    if (m_TestMode == PB_TESTINPUT) gfxRenderShadowString(m_defaultFontSpriteId, "INPUTS", 10, 30, 1, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    else gfxRenderShadowString(m_defaultFontSpriteId, "OUTPUTS", 10, 30, 1, GFX_TEXTLEFT, 0, 0, 0, 255, 2);  
    
    for (int i = 0; i < limit; i++) {
        #ifdef EXE_MODE_WINDOWS
            if (m_TestMode == PB_TESTINPUT)  temp = g_inputDef[i].inputName + "(" + g_inputDef[i].simMapKey + "): ";
            else temp = g_outputDef[i].outputName + ": ";
        #endif
        #ifdef EXE_MODE_RASPI
            if (m_TestMode == PB_TESTINPUT) temp = g_inputDef[i].inputName + ": ";
            else temp = g_outputDef[i].outputName + ": ";
        #endif
        
        if ((i == m_CurrentOutputItem) && (m_TestMode == PB_TESTOUTPUT)) gfxSetColor (m_defaultFontSpriteId, 255, 0, 0, 255);
        else gfxSetColor (m_defaultFontSpriteId, 255, 255, 255, 255);

        int itemp =  ((i % 19) * 40);
        gfxRenderString(m_defaultFontSpriteId, temp, 10 + ((i / 19) * 220), 60 + ((i % 19) * 22), 1, GFX_TEXTLEFT);
        
        // Print the state of the input (and highlight in RED) if ON
        if (((g_inputDef[i].lastState == PB_ON) && (m_TestMode == PB_TESTINPUT)) || 
            ((g_outputDef[i].lastState == PB_ON) && (m_TestMode == PB_TESTOUTPUT))) {
            gfxSetColor(m_defaultFontSpriteId, 255,0, 0, 255);
            temp = "ON";
        }
        else {
            gfxSetColor(m_defaultFontSpriteId, 255,255, 255, 255);
            temp = "OFF";
        };
        gfxRenderString(m_defaultFontSpriteId, temp, 180 + ((i / 19) * 220), 60 + ((i % 19) * 22), 1, GFX_TEXTLEFT);
    }
    
    return (true);   
}

// Settings Menu Screen

bool PBEngine::pbeLoadSettings(bool forceReload){

    if (!pbeLoadStartMenu (false)) return (false); 

    return (true);
}

bool PBEngine::pbeRenderSettings(unsigned long currentTick, unsigned long lastTick){

    if (!pbeLoadSettings(false)) return (false); 

    std::map<unsigned int, std::string> tempMenu = g_settingsMenu;

    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
 
    // Render the default background
    pbeRenderDefaultBackground (currentTick, lastTick);
 
    gfxSetColor(m_StartMenuFontId, 255 ,165, 0, 255);
    gfxSetScaleFactor(m_StartMenuFontId, 2.0, false);
    gfxRenderShadowString(m_StartMenuFontId, MenuSettings, (PB_SCREENWIDTH/2), 15, 2, GFX_TEXTCENTER, 0, 0, 0, 255, 6);
    gfxSetScaleFactor(m_StartMenuFontId, 1.5, false);
    gfxSetColor(m_StartMenuFontId, 255 ,255, 255, 255);

    gfxSetScaleFactor(m_StartMenuSwordId, 0.9, false);
    gfxSetRotateDegrees(m_StartMenuSwordId, 0.0f, false);

    // Add the extra data to the menu strings before displaying
    tempMenu[0] += std::to_string(m_saveFileData.mainVolume);
    tempMenu[1] += std::to_string(m_saveFileData.musicVolume);
    tempMenu[2] += std::to_string(m_saveFileData.ballsPerGame);
    switch (m_saveFileData.difficulty) {
        case PB_EASY: tempMenu[3] += "Easy"; break;
        case PB_NORMAL: tempMenu[3] += "Normal"; break;
        case PB_HARD: tempMenu[3] += "Hard"; break;
        case PB_EPIC: tempMenu[3] += "Epic"; break;
    }
    
    // Render the menu items with shadow depending on the selected item
    pbeRenderGenericMenu(m_StartMenuSwordId, m_StartMenuFontId, m_CurrentSettingsItem, (PB_SCREENWIDTH/2) - 470, 250, 15, &tempMenu, true, true, 64, 0, 255, 255, 8);

    // Add insturctions how to exit
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "Start = exit", PB_SCREENWIDTH - 100, PB_SCREENHEIGHT - 25, 1, GFX_TEXTLEFT, 0,0,0,255,2);
        
     return (true);
}

bool PBEngine::pbeLoadDiagnostics(bool forceReload){
    if (!pbeLoadStartMenu(false)) return (false); 

    return (true);
}

bool PBEngine::pbeRenderDiagnostics(unsigned long currentTick, unsigned long lastTick){

    if (!pbeLoadDiagnostics(false)) return (false); 

    std::map<unsigned int, std::string> tempMenu = g_diagnosticsMenu;

    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
 
    // Render the default background
    pbeRenderDefaultBackground (currentTick, lastTick);
 
    gfxSetColor(m_StartMenuFontId, 255 ,165, 0, 255);
    gfxSetScaleFactor(m_StartMenuFontId, 2.0, false);
    gfxRenderShadowString(m_StartMenuFontId, MenuDiagnostics, (PB_SCREENWIDTH/2), 5, 2, GFX_TEXTCENTER, 0, 0, 0, 255, 6);
    gfxSetScaleFactor(m_StartMenuFontId, 1.5, false);
    gfxSetColor(m_StartMenuFontId, 255 ,255, 255, 255);

    gfxSetScaleFactor(m_StartMenuSwordId, 0.9, false);
    gfxSetRotateDegrees(m_StartMenuSwordId, 0.0f, false);

    // Add the extra data to the menu strings before displaying
    if (m_EnableOverlay) tempMenu[2] += PB_OVERLAY_ON_TEXT;
    else tempMenu[2] += PB_OVERLAY_OFF_TEXT;
        
    // Render the menu items with shadow depending on the selected item
    pbeRenderGenericMenu(m_StartMenuSwordId, m_StartMenuFontId, m_CurrentDiagnosticsItem, (PB_SCREENWIDTH/2) - 500, 250, 25, &tempMenu, true, true, 64, 0, 255, 255, 8);

    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "Start = exit", PB_SCREENWIDTH - 100, PB_SCREENHEIGHT - 25, 1, GFX_TEXTLEFT, 0,0,0,255,2);
        
     return (true);
}

// Credits Screen processing

bool PBEngine::pbeLoadCredits(bool forceReload){

    if (!pbeLoadDefaultBackground(false)) return (false);
    return (true);
}

bool PBEngine::pbeRenderCredits(unsigned long currentTick, unsigned long lastTick){

    if (!pbeLoadCredits (false)) return (false);

    if (m_RestartCredits) {
        m_RestartCredits = false;
        m_StartTick = GetTickCountGfx(); 
    }

    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
 
    // Render the default background
    pbeRenderDefaultBackground (currentTick, lastTick);

    int pixelShiftY = ((currentTick - m_StartTick) / m_TicksPerPixel);
    int tempX = PB_SCREENWIDTH / 2;

    // Once we fix the ability to render to negative coordinates, we can remove this and let it scroll off the screen
    if (pixelShiftY < (PB_SCREENHEIGHT * 2)) {

        m_CreditsScrollY = PB_SCREENHEIGHT - pixelShiftY;
        int spacing = 45;

        gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
        gfxSetScaleFactor(m_defaultFontSpriteId, 1.5, false);
        gfxRenderShadowString(m_defaultFontSpriteId, "Credits", tempX, m_CreditsScrollY, 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "Dragons of Destiny Pinball", tempX, m_CreditsScrollY + (1*spacing), 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "Designed and Programmed by: Jeffrey Bock", tempX, m_CreditsScrollY + (2*spacing), 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "Additional design and 3D printing: Tremayne Bock", tempX, m_CreditsScrollY + (3*spacing), 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "Using Rasberry Pi (PI)nball Engine", tempX, m_CreditsScrollY + (4*spacing), 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "Full code and 3D models available at:", tempX, m_CreditsScrollY + (5*spacing), 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "https://github.com/jeffbock/PInball", tempX, m_CreditsScrollY + (6*spacing), 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "Thanks to Kim, Ally, Katie and Ruth for inspiration", tempX, m_CreditsScrollY + (7*spacing), 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, " ", tempX, m_CreditsScrollY + (8*spacing), 1, GFX_TEXTCENTER, 0,0,0,0,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "Thanks to the following Open Source libraries", tempX, m_CreditsScrollY + (9*spacing) +2, 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "STB Single Header: http://nothings.org/stb", tempX, m_CreditsScrollY + (10*spacing) +2, 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "JSON.hpp https://github.com/nlohmann/json", tempX, m_CreditsScrollY + (11*spacing) +2, 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "WiringPi https://github.com/WiringPi/WiringPi", tempX, m_CreditsScrollY + (12*spacing) +2, 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "Developed using AI and Microsoft Copilot tools", tempX, m_CreditsScrollY + (13*spacing) +2, 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxSetScaleFactor(m_defaultFontSpriteId, 1.0, false);
    }

    return (true);   
}
 
// Benchmark Screen
bool PBEngine::pbeLoadBenchmark(bool forceReload){

    // Benchmark will just use default font and the start menu items
    if (!pbeLoadStartMenu (false)) return (false); 
    return (true);
}

bool PBEngine::pbeRenderBenchmark(unsigned long currentTick, unsigned long lastTick){

    static unsigned int FPSSwap, smallSpriteCount, spriteTransformCount, bigSpriteCount;
    
    if (!pbeLoadBenchmark (false)) return (false); 

    if (m_RestartBenchmark) {
        m_BenchmarkStartTick =  GetTickCountGfx(); 
        m_BenchmarkDone = false;
        m_RestartBenchmark = false;
        FPSSwap = 0; smallSpriteCount = 0; spriteTransformCount = 0; bigSpriteCount = 0;
        m_TicksPerScene = 4000; m_CountDownTicks = 4000;
    }

    unsigned int elapsedTime = currentTick - m_BenchmarkStartTick;

    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);

    gfxAnimateSprite(m_aniId, currentTick);
    gfxRenderSprite(m_aniId);

    std::string temp;
    int tempX = PB_SCREENWIDTH / 2;
    int x, y;

    if (elapsedTime < m_CountDownTicks) {
        
        temp = "Benchmark Starting in " + std::to_string((m_CountDownTicks - elapsedTime) / 1000);
        gfxRenderShadowString(m_defaultFontSpriteId, temp, tempX, 200, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);
        gfxRenderShadowString(m_defaultFontSpriteId, "System will be unresponsive", tempX, 225, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);
        return (true);
    }
    
    // Clear and Swap rate (may be limited by monitor refresh rate)
    if (elapsedTime < (m_TicksPerScene + m_CountDownTicks)) {
        while ((GetTickCountGfx() - currentTick) < 1000) {
            gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
            temp = "Clear and Swap Test: Swap " + std::to_string(FPSSwap);
            gfxRenderShadowString(m_defaultFontSpriteId, temp , tempX, 200, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);
            FPSSwap++;
            // gfxSwap();
        }
        return (true);
    }

    // Small, untransformed sprites per second (with a clear)
    if (elapsedTime < ((m_TicksPerScene *2) + m_CountDownTicks)) {
        gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
        gfxSetScaleFactor(m_StartMenuSwordId, 0.10, false);
        while ((GetTickCountGfx() - currentTick) < 1000) {
            // Get and random X and Y value, within the screen bounds
            x = rand() % PB_SCREENWIDTH;
            y = rand() % PB_SCREENHEIGHT;
            gfxRenderSprite(m_StartMenuSwordId, x, y);
            smallSpriteCount++;
        }
        gfxRenderShadowString(m_defaultFontSpriteId, "Small Sprite Test", tempX, 200, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);
        // gfxSwap();
        return (true);
    }

    // Big, untransformed sprites per second (with a clear)
    if (elapsedTime < ((m_TicksPerScene *3) + m_CountDownTicks)) {
        gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
        // gfxSetScaleFactor(m_StartMenuSwordId, 3.0f, false);
        
        while ((GetTickCountGfx() - currentTick) < 1000 ){
            // Get a ramdom value from -ScreenWidth  to +ScreenWidth and -ScreenHeight to +ScreenHeight
            x = rand() % (PB_SCREENWIDTH * 2) - PB_SCREENWIDTH;
            y = rand() % (PB_SCREENHEIGHT * 2) - PB_SCREENHEIGHT;
            gfxRenderSprite(m_BootUpConsoleId, x, y);
            bigSpriteCount++;
        }

        gfxRenderShadowString(m_defaultFontSpriteId, "Large Sprite Test", tempX, 200, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);
        // gfxSwap();
        return (true);
    }

    // Full Transformed, random untransformed sprites per second (with a clear)
    if (elapsedTime < ((m_TicksPerScene *4) + m_CountDownTicks)) {
        gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
        
        while ((GetTickCountGfx() - currentTick) < 250) {
            // Get and random X and Y value, within the screen bounds
            x = rand() % PB_SCREENWIDTH;
            y = rand() % PB_SCREENHEIGHT;
            // Get a random scale and rotation value
            float scale = (rand() % 100) / 100.0f;
            float rotation = (rand() % 360);
            gfxSetScaleFactor(m_StartMenuSwordId, scale, false);
            gfxSetRotateDegrees(m_StartMenuSwordId, rotation, false);
            gfxRenderSprite(m_StartMenuSwordId, x, y);
            spriteTransformCount++;
        }
        gfxRenderShadowString(m_defaultFontSpriteId, "Transformed Sprite Test", tempX, 200, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);
        // gfxSwap();
        return (true);
        // Print the final results when done
    }
    
    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
    temp = "Benchmark Complete - Results";
    gfxRenderShadowString(m_defaultFontSpriteId, temp, tempX, 180, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);
    temp = "Clear + Swap Rate: " + std::to_string(FPSSwap/(m_TicksPerScene/1000)) + " FPS";
    gfxRenderShadowString(m_defaultFontSpriteId, temp, tempX, 230, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);
    temp = "Small Sprite Rate: " + std::to_string(smallSpriteCount/((m_TicksPerScene))) + "k SPS";
    gfxRenderShadowString(m_defaultFontSpriteId, temp, tempX, 255, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);
    temp = "Large Sprite Rate: " + std::to_string(bigSpriteCount/((m_TicksPerScene))) + "k SPS";
    gfxRenderShadowString(m_defaultFontSpriteId, temp, tempX, 280, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);
    temp = "Transformed Sprite Rate: " + std::to_string(spriteTransformCount/((m_TicksPerScene))) + "k SPS";
    gfxRenderShadowString(m_defaultFontSpriteId, temp, tempX, 305, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);

    m_BenchmarkDone = true;
    return (true);   
}

// Texture management functions
void PBEngine::pbeReleaseMenuTextures(){

    gfxUnloadTexture(m_BootUpConsoleId);
    gfxUnloadTexture(m_BootUpStarsId);
    gfxUnloadTexture(m_StartMenuSwordId);
}

void PBEngine::pbeUpdateState(stInputMessage inputMessage){
    
    switch (m_mainState) {
        case PB_BOOTUP: {
            // If any button is pressed, move to the start menu
            if (inputMessage.inputType == PB_INPUT_BUTTON && inputMessage.inputState == PB_ON) {
                m_mainState = PB_STARTMENU;
                m_RestartMenu = true;
            }
            break;
        }
        case PB_STARTMENU: {
            // If either left button is pressed, subtract 1 from m_currentMenuItem
            if (inputMessage.inputType == PB_INPUT_BUTTON && inputMessage.inputState == PB_ON) {
                if (inputMessage.inputId == IDI_LEFTFLIPPER) {
                    // Get the current menu item count from g_mainMenu
                    if (m_CurrentMenuItem > 0) m_CurrentMenuItem--;
                }
                // If either right button is pressed, add 1 to m_currentMenuItem
                if (inputMessage.inputId == IDI_RIGHTFLIPPER) {
                    int temp = g_mainMenu.size();
                    if (m_CurrentMenuItem < (temp -1)) m_CurrentMenuItem++;
                }

                if ((inputMessage.inputId == IDI_RIGHTACTIVATE) || (inputMessage.inputId == IDI_LEFTACTIVATE)) {
                    // Do something based on the menu item
                    switch (m_CurrentMenuItem) {
                        case (0):  if (m_PassSelfTest) m_mainState = PB_PLAYGAME; break;
                        case (1):  m_mainState = PB_SETTINGS; m_RestartSettings = true; break;
                        case (2):  m_mainState = PB_DIAGNOSTICS; m_RestartDiagnostics = true; break;
                        case (3):  m_mainState = PB_CREDITS; m_RestartCredits = true; break;
                        default: break;
                    }
                }                
            }
            break;
        }
        case PB_DIAGNOSTICS: {

            if (inputMessage.inputType == PB_INPUT_BUTTON && inputMessage.inputState == PB_ON) {
                if (inputMessage.inputId == IDI_LEFTFLIPPER) {
                    // Get the current menu item count from g_mainMenu
                    if (m_CurrentDiagnosticsItem > 0) m_CurrentDiagnosticsItem--;
                }
                // If either right button is pressed, add 1 to m_currentMenuItem
                if (inputMessage.inputId == IDI_RIGHTFLIPPER) {
                    int temp = g_diagnosticsMenu.size();
                    if (m_CurrentDiagnosticsItem < (temp -1)) m_CurrentDiagnosticsItem++;
                }
            }

            if (((inputMessage.inputId == IDI_RIGHTACTIVATE) || (inputMessage.inputId == IDI_LEFTACTIVATE)) && inputMessage.inputState == PB_ON){
                switch (m_CurrentDiagnosticsItem) {
                    case (0): m_mainState = PB_TESTMODE; m_RestartTestMode = true; break;
                    case (1): m_mainState = PB_BENCHMARK; m_RestartBenchmark = true; break;
                    case (2): if ((inputMessage.inputId == IDI_RIGHTACTIVATE) || (inputMessage.inputId == IDI_LEFTACTIVATE)) {
                        if (m_EnableOverlay) m_EnableOverlay = false;
                        else m_EnableOverlay = true;
                    }
                    default: break;
                }
            }

            if (inputMessage.inputId == IDI_START) {
                // Save the values to the settings file and exit the screen
                m_mainState = PB_STARTMENU;
                m_RestartMenu = true;
            }

            break;
        }

        case PB_TESTMODE: {
            
            // Check the mode and send output message if needed
            if (m_TestMode == PB_TESTOUTPUT) {
                // Send the output message to the output queue - this will be connected to HW
                if (inputMessage.inputType == PB_INPUT_BUTTON && inputMessage.inputState == PB_ON) {
                    if (inputMessage.inputId == IDI_LEFTFLIPPER) {
                        if (m_CurrentOutputItem > 0) m_CurrentOutputItem--;
                    }
                    // If either right button is pressed, add 1 to m_currentMenuItem
                    if (inputMessage.inputId == IDI_RIGHTFLIPPER) {
                        if (m_CurrentOutputItem < (NUM_OUTPUTS -1)) m_CurrentOutputItem++;
                    }
                }

                if ((inputMessage.inputId == IDI_RIGHTACTIVATE) || (inputMessage.inputId == IDI_LEFTACTIVATE)) {
                    if (g_outputDef[m_CurrentOutputItem].lastState == PB_ON) g_outputDef[m_CurrentOutputItem].lastState = PB_OFF;
                    else g_outputDef[m_CurrentOutputItem].lastState = PB_ON;

                     // Send the message to the output queue
                    stOutputMessage outputMessage;        
                    outputMessage.outputType = g_outputDef[m_CurrentOutputItem].outputType;
                    outputMessage.outputId = g_outputDef[m_CurrentOutputItem].id;
                    outputMessage.outputState = g_outputDef[m_CurrentOutputItem].lastState;
                    outputMessage.sentTick = GetTickCountGfx();
                    m_outputQueue.push(outputMessage);
                    }
            }
            
            // If the start button has been pressed, return to the start menu
            if (inputMessage.inputId == IDI_LEFTFLIPPER) {
                if (inputMessage.inputState == PB_ON) m_LFON = true;
                else m_LFON = false;
            }
            if (inputMessage.inputId == IDI_RIGHTFLIPPER) {
                if (inputMessage.inputState == PB_ON) m_RFON = true;
                else m_RFON = false;
            }
            if (inputMessage.inputId == IDI_LEFTACTIVATE) {
                if (inputMessage.inputState == PB_ON) m_LAON = true;
                else m_LAON = false;
            }
            if (inputMessage.inputId == IDI_RIGHTACTIVATE) {
                if (inputMessage.inputState == PB_ON) m_RAON = true;
                else m_RAON = false;
            }

            // If both left and right flippers are pressed, toggle the test mode
            if (m_LFON && m_RFON) {
                if (m_TestMode == PB_TESTINPUT) m_TestMode = PB_TESTOUTPUT;
                else m_TestMode = PB_TESTINPUT;
                m_LFON = false; m_RFON=false; m_LAON =false; m_RAON = false;
            }
            // If both left and right activations are pressed, exit test mode
            if (m_LAON && m_RAON) {
                m_mainState = PB_DIAGNOSTICS;
                m_RestartDiagnostics = true;
            }
            // Send the output message to the output queue - this will be connected to HW


            break;
        }
        case PB_SETTINGS: {
            if (inputMessage.inputType == PB_INPUT_BUTTON && inputMessage.inputState == PB_ON) {
                if (inputMessage.inputId == IDI_LEFTFLIPPER) {
                    if (m_CurrentSettingsItem > 0) m_CurrentSettingsItem--;
                }
                // If either right button is pressed, add 1 to m_currentMenuItem
                if (inputMessage.inputId == IDI_RIGHTFLIPPER) {
                    int temp = g_settingsMenu.size();
                    if (m_CurrentSettingsItem < (temp -1)) m_CurrentSettingsItem++;
                }
                if (inputMessage.inputId == IDI_START) {
                    // Save the values to the settings file and exit the screen
                    pbeSaveFile();
                    m_mainState = PB_STARTMENU;
                    m_RestartMenu = true;
                }
            }

            if (((inputMessage.inputId == IDI_RIGHTACTIVATE) || (inputMessage.inputId == IDI_LEFTACTIVATE)) && inputMessage.inputState == PB_ON){
                switch (m_CurrentSettingsItem) {
                    case (0): {
                        if (inputMessage.inputId == IDI_RIGHTACTIVATE) {
                            if (m_saveFileData.mainVolume < 10) m_saveFileData.mainVolume++;
                        }
                        if (inputMessage.inputId == IDI_LEFTACTIVATE) {
                            if (m_saveFileData.mainVolume > 0) m_saveFileData.mainVolume--;
                        }
                        break;
                    }
                    case (1): {
                        if (inputMessage.inputId == IDI_RIGHTACTIVATE) {
                            if (m_saveFileData.musicVolume < 10) m_saveFileData.musicVolume++;
                        }
                        if (inputMessage.inputId == IDI_LEFTACTIVATE) {
                            if (m_saveFileData.musicVolume > 0) m_saveFileData.musicVolume--;
                        }
                        break;
                    }
                    case (2): {
                        if (inputMessage.inputId == IDI_RIGHTACTIVATE) {
                            if (m_saveFileData.ballsPerGame < 9) m_saveFileData.ballsPerGame++;
                        }
                        if (inputMessage.inputId == IDI_LEFTACTIVATE) {
                            if (m_saveFileData.ballsPerGame > 1) m_saveFileData.ballsPerGame--;
                        }
                        break;
                    }
                    case (3): {
                        if (inputMessage.inputId == IDI_RIGHTACTIVATE) {
                            switch (m_saveFileData.difficulty) {
                                case PB_EASY: m_saveFileData.difficulty = PB_NORMAL; break;
                                case PB_NORMAL: m_saveFileData.difficulty = PB_HARD; break;
                                case PB_HARD: m_saveFileData.difficulty = PB_EPIC; break;
                                case PB_EPIC: m_saveFileData.difficulty = PB_EPIC; break;
                            }
                        }
                        if (inputMessage.inputId == IDI_LEFTACTIVATE) {
                            switch (m_saveFileData.difficulty) {
                                case PB_EASY: m_saveFileData.difficulty = PB_EASY; break;
                                case PB_NORMAL: m_saveFileData.difficulty = PB_EASY; break;
                                case PB_HARD: m_saveFileData.difficulty = PB_NORMAL; break;
                                case PB_EPIC: m_saveFileData.difficulty = PB_HARD; break;
                            }
                        }
                        break;
                    }
                    case (4): {
                        if ((inputMessage.inputId == IDI_RIGHTACTIVATE) || (inputMessage.inputId == IDI_LEFTACTIVATE)) {
                            resetHighScores();
                        }
                    }
                    default: break;
                }
            }
            break;
        }
        case PB_CREDITS: {
            if (inputMessage.inputType == PB_INPUT_BUTTON && inputMessage.inputState == PB_ON) {
                m_mainState = PB_STARTMENU;
                m_RestartMenu = true;
            }
        break;
        }
        case PB_BENCHMARK: {
            if (m_BenchmarkDone && (inputMessage.inputType == PB_INPUT_BUTTON && inputMessage.inputState == PB_ON)) {
                m_mainState = PB_DIAGNOSTICS;
                m_RestartDiagnostics = true;
            }
        break;
        }

        case PB_PLAYGAME: {
            pbeReleaseMenuTextures();
            m_GameStarted = true;
            m_RestartTable = true;
            m_mainState = PB_PLAYGAME;
        break;
        }

        default: break;
    }
}

// Function checks the input / output structures for errors and if Raspberry Pi, sets up the GPIO pins
bool PBEngine::pbeSetupIO()
{
    // Check the input definitions and ensure no duplicates
    // Need to change this to a self test function - will need to set up Raspberry I/O and breakout boards
    for (int i = 0; i < NUM_INPUTS; i++) {
        if (i == 0) g_PBEngine.pbeSendConsole("(PI)nball Engine: Total Inputs: " + std::to_string(NUM_INPUTS)); 
        for (int j = i + 1; j < NUM_INPUTS; j++) {
            if (g_inputDef[i].id == g_inputDef[j].id) {
                g_PBEngine.pbeSendConsole("(PI)nball Engine: ERROR: Duplicate input ID: " + std::to_string(g_inputDef[i].id));
                g_PBEngine.m_PassSelfTest = false;
            }
            // Check that the board type and pin number are unique
            if (g_inputDef[i].boardType == g_inputDef[j].boardType && g_inputDef[i].pin == g_inputDef[j].pin) {
                g_PBEngine.pbeSendConsole("(PI)nball Engine: ERROR: Duplicate input board/pin: " + std::to_string(g_inputDef[i].id));
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
            // Check that the board type and pin number are unique
            if (g_outputDef[i].boardType == g_outputDef[j].boardType && g_outputDef[i].pin == g_outputDef[j].pin) {
                g_PBEngine.pbeSendConsole("(PI)nball Engine: ERROR: Duplicate output board/pin: " + std::to_string(g_outputDef[i].id));
                g_PBEngine.m_PassSelfTest = false;
            }
        }
    }

    // Set up the GPIO and I2C for Raspberry Pi if in EXE_MODE_RASPI
    #ifdef EXE_MODE_RASPI

    wiringPiSetupPinType(WPI_PIN_BCM);

    // Setup the devices on the I2C bus (currently only the amplifier)
    // TODO:  Connect this to the master volume control at some pont.. probably need some I2C write function
    // TODO:  This will need to be refactored to when more GPIO I2C devices are added
    int fd = wiringPiI2CSetup(PB_I2C_AMPLIFIER);
    uint8_t data = 0x20;

    if (fd > 0) wiringPiI2CRawWrite (fd, &data, 1);
    else (g_PBEngine.m_PassSelfTest = false);

    // Loop through each of the inputs and program the GPIOs and setup the debounce class for each input
    for (int i = 0; i < NUM_INPUTS; i++) {
        if (g_inputDef[i].boardType == PB_RASPI){
            cDebounceInput debounceInput(g_inputDef[i].pin, g_inputDef[i].debounceTimeMS, true, true);
            g_PBEngine.m_inputMap.emplace(g_inputDef[i].id, debounceInput);
        }
    }

    // Repeat for outputs
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        if (g_outputDef[i].boardType == PB_RASPI){
            pinMode(g_outputDef[i].pin, OUTPUT);
            digitalWrite(g_outputDef[i].pin,LOW); 
        }
    }

    #endif // EXE_MODE_RASPI
    
    return (g_PBEngine.m_PassSelfTest);
}

// Main program start!!   
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

    // Setup the inputs and outputs
    g_PBEngine.pbeSendConsole("(PI)nball Engine: Setting up I/O");
    g_PBEngine.pbeSetupIO();
    
    // Load the saved values for settings and high scores
    if (!g_PBEngine.pbeLoadSaveFile(false, false)) {
        std::string temp2 = SAVEFILENAME;
        std::string temp = "(PI)nball Engine: ERROR Using settings defaults, failed: " + temp2;
        g_PBEngine.pbeSendConsole(temp);
        g_PBEngine.pbeSaveFile ();
    }
    else g_PBEngine.pbeSendConsole("(PI)nball Engine: Loaded settings and score file"); 

    g_PBEngine.pbeSendConsole("(PI)nball Engine: Starting main processing loop");    
   
    // Main loop for the pinball game                                
    unsigned long currentTick = g_PBEngine.GetTickCountGfx();
    unsigned long lastTick = currentTick;

    // Start the input thread
    // std::thread inputThread(&PBProcessInput);

    // The main game engine loop
    while (true) {

        currentTick = g_PBEngine.GetTickCountGfx();
        stInputMessage inputMessage;
        
        // PB Process Input and Output will be a thread in the PiOS side since it won't have windows messages
        // Will need to fix this later...
        PBProcessInput();
        PBProcessOutput();

        // Process all the input message queue and update the game state
        if (!g_PBEngine.m_inputQueue.empty()){
            inputMessage = g_PBEngine.m_inputQueue.front();
            g_PBEngine.m_inputQueue.pop();

            // Update the game state based on the input message
            if (!g_PBEngine.m_GameStarted) g_PBEngine.pbeUpdateState (inputMessage); 
            else g_PBEngine.pbeUpdateGameState (inputMessage);
        }
        
        if (!g_PBEngine.m_GameStarted)g_PBEngine.pbeRenderScreen(currentTick, lastTick);
        else g_PBEngine.pbeRenderGameScreen(currentTick, lastTick);

        g_PBEngine.gfxSwap();

        lastTick = currentTick;
    }

   // Join the input thread before exiting
   // inputThread.join();

   return 0;
}


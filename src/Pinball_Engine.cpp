// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "Pinball_Engine.h"
#include "Pinball.h"
#include "PBSequences.h"
#include "PBDevice.h"

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
    m_maxConsoleLines = 40;
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
    m_ShowFPS = false;
    m_RenderFPS = 0;

    // Credits screen variables
    m_CreditsScrollY = 480;
    m_TicksPerPixel = 30;
    m_RestartCredits = true;

    // Benchmark variables
    m_TicksPerScene = 10000; m_BenchmarkStartTick = 0;  m_CountDownTicks = 4000; m_BenchmarkDone = false;
    m_RestartBenchmark = true;

    // Test Sandbox variables
    m_RestartTestSandbox = true;
    m_sandboxVideoPlayer = nullptr;
    m_sandboxVideoSpriteId = NOSPRITE;
    m_sandboxVideoLoaded = false;
    m_videoFadeStartTick = 0;
    m_videoFadingIn = false;
    m_videoFadingOut = false;
    m_videoFadeDurationSec = 2.0f;  // 2 second fade in/out

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
    
    // Auto output control - default to disable since the menus launch first
    m_autoOutputEnable = false;
 }

 PBEngine::~PBEngine(){

    // Clean up video player
    if (m_sandboxVideoPlayer) {
        delete m_sandboxVideoPlayer;
        m_sandboxVideoPlayer = nullptr;
    }
    
    // Clean up all registered devices
    ClearDevices();

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
        case PB_TESTSANDBOX: return pbeRenderTestSandbox(currentTick, lastTick); break;
        default: return (false); break;
    }

    return (false);
}

// Load reasources for the boot up screen
bool PBEngine::pbeLoadDefaultBackground(bool forceReload){
    
    static bool defaultBackgroundLoaded = false;
    if (forceReload) defaultBackgroundLoaded = false;
    if (defaultBackgroundLoaded) return (true);

    pbeSendConsole("RasPin: Loading default background resources");

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

    pbeSendConsole("RasPin: Loading boot screen resources");
    
    // Load the bootup screen items
    
    m_BootUpTitleBarId = gfxLoadSprite("Title Bar", "", GFX_NONE, GFX_NOMAP, GFX_UPPERLEFT, false, false);
    gfxSetColor(m_BootUpTitleBarId, 0, 0, 255, 255);
    gfxSetWH(m_BootUpTitleBarId, PB_SCREENWIDTH, 40);

    if (m_BootUpTitleBarId == NOSPRITE) return (false);

    pbeSendConsole("RasPin: Ready - Press any button to continue");

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
    gfxRenderShadowString(m_defaultFontSpriteId, "RasPin - Copyright 2025 Jeff Bock", (PB_SCREENWIDTH / 2), 10, 1, GFX_TEXTCENTER, 0, 0, 0, 255, 2);

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
        gfxRenderString(m_defaultFontSpriteId, temp, 10 + ((i / 24) * 220), 60 + ((i % 24) * 26), 1, GFX_TEXTLEFT);
        
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
        gfxRenderString(m_defaultFontSpriteId, temp, 210 + ((i / 24) * 220), 60 + ((i % 24) * 26), 1, GFX_TEXTLEFT);
    }
    
    return (true);   
}

// I/O Overlay Rendering - Shows current state of all inputs and outputs

bool PBEngine::pbeRenderOverlay(unsigned long currentTick, unsigned long lastTick){
    
    // Uses the same resources as test mode
    if (!pbeLoadTestMode (false)) return (false); 

    // Set up transparent background for overlay
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    
    // INPUTS Section
    gfxSetColor(m_defaultFontSpriteId, 0, 255, 255, 255);  // Cyan for inputs header
    gfxRenderShadowString(m_defaultFontSpriteId, "INPUTS", 20, 5, 0.4, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    
    // Render inputs in one column (48 items will fit with 40% scale)
    for (int i = 0; i < NUM_INPUTS; i++) {
        std::string temp = g_inputDef[i].inputName + ": ";
        
        int x = 20;                   // Single column
        int y = 25 + (i * 20);        // Start below header at top of screen
        
        // Render input name
        gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
        gfxRenderShadowString(m_defaultFontSpriteId, temp, x, y, 0.4, GFX_TEXTLEFT, 0, 0, 0, 255, 1);
        
        // Render state with appropriate color
        std::string stateText;
        switch (g_inputDef[i].lastState) {
            case PB_ON:
                stateText = "ON";
                gfxSetColor(m_defaultFontSpriteId, 0, 255, 0, 255);  // Green for ON
                break;
            case PB_OFF:
                stateText = "OFF";
                gfxSetColor(m_defaultFontSpriteId, 128, 128, 128, 255);  // Gray for OFF
                break;
            default:
                stateText = "UNK";
                gfxSetColor(m_defaultFontSpriteId, 255, 0, 255, 255);  // Magenta for unknown
                break;
        }
        gfxRenderShadowString(m_defaultFontSpriteId, stateText, x + 180, y, 0.4, GFX_TEXTLEFT, 0, 0, 0, 255, 1);
    }
    
    // OUTPUTS Section - Position moved 225 pixels further right total
    int outputStartX = PB_SCREENWIDTH - 255;  // Moved 225 pixels right total (was -480, now -255)
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 0, 255);  // Yellow for outputs header
    gfxRenderShadowString(m_defaultFontSpriteId, "OUTPUTS", outputStartX, 5, 0.4, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    
    // Render outputs in one column (48 items will fit with 40% scale)
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        std::string temp = g_outputDef[i].outputName + ": ";
        
        int x = outputStartX;         // Single column
        int y = 25 + (i * 20);        // Start below header at top of screen
        
        // Render output name
        gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
        gfxRenderShadowString(m_defaultFontSpriteId, temp, x, y, 0.4, GFX_TEXTLEFT, 0, 0, 0, 255, 1);
        
        // Render state with appropriate color
        std::string stateText;
        switch (g_outputDef[i].lastState) {
            case PB_ON:
                stateText = "ON";
                gfxSetColor(m_defaultFontSpriteId, 0, 255, 0, 255);  // Green for ON
                break;
            case PB_OFF:
                stateText = "OFF";
                gfxSetColor(m_defaultFontSpriteId, 128, 128, 128, 255);  // Gray for OFF
                break;
            case PB_BLINK:
                stateText = "BLNK";
                gfxSetColor(m_defaultFontSpriteId, 255, 255, 0, 255);  // Yellow for BLINK
                break;
            case PB_BRIGHTNESS:
                stateText = "BRGT";
                gfxSetColor(m_defaultFontSpriteId, 255, 128, 0, 255);  // Orange for BRIGHTNESS
                break;
            default:
                stateText = "UNK";
                gfxSetColor(m_defaultFontSpriteId, 255, 0, 255, 255);  // Magenta for unknown
                break;
        }
        gfxRenderShadowString(m_defaultFontSpriteId, stateText, x + 180, y, 0.4, GFX_TEXTLEFT, 0, 0, 0, 255, 1);
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
    gfxRenderShadowString(m_defaultFontSpriteId, "Start = exit", PB_SCREENWIDTH - 130, PB_SCREENHEIGHT - 25, 1, GFX_TEXTLEFT, 0,0,0,255,2);
        
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
    if (m_EnableOverlay) tempMenu[2] += PB_ON_TEXT;
    else tempMenu[2] += PB_OFF_TEXT;
    
    if (m_ShowFPS) tempMenu[3] += PB_ON_TEXT;
    else tempMenu[3] += PB_OFF_TEXT;
        
    // Render the menu items with shadow depending on the selected item
    pbeRenderGenericMenu(m_StartMenuSwordId, m_StartMenuFontId, m_CurrentDiagnosticsItem, (PB_SCREENWIDTH/2) - 500, 250, 25, &tempMenu, true, true, 64, 0, 255, 255, 8);

    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "Start = exit", PB_SCREENWIDTH - 130, PB_SCREENHEIGHT - 25, 1, GFX_TEXTLEFT, 0,0,0,255,2);
        
     return (true);
}

// Test Sandbox Screen

bool PBEngine::pbeLoadTestSandbox(bool forceReload){
    if (!pbeLoadStartMenu(false)) return (false);
    
    // Initialize video player if not already created
    if (!m_sandboxVideoPlayer && !forceReload) {
        m_sandboxVideoPlayer = new PBVideoPlayer(this, &m_soundSystem);
        
        // Load the video with 720p dimensions (1280x720)
        // Center it horizontally and position below the text
        // With 75% scale: 1280 * 0.75 = 960 width, 720 * 0.75 = 540 height
        int scaledWidth = (int)(1280 * 0.75f);
        int scaledHeight = (int)(720 * 0.75f);
        
        // Center horizontally and position below text (text ends around y=250)
        int videoX = (PB_SCREENWIDTH - scaledWidth) / 2;
        int videoY = 480;  // Below the button descriptions (pushed down 200px)
        
        m_sandboxVideoSpriteId = m_sandboxVideoPlayer->pbvpLoadVideo(
            "src/resources/videos/darktown_sound_h264.mp4",
            videoX,
            videoY,
            false  // Don't keep resident
        );
        
        if (m_sandboxVideoSpriteId != NOSPRITE) {
            // Scale the video sprite to 75%
            m_sandboxVideoPlayer->pbvpSetScaleFactor(0.75f);
            
            // Start with alpha at 0 for fade in
            gfxSetTextureAlpha(m_sandboxVideoSpriteId, 0.0f);
            
            m_sandboxVideoLoaded = true;
        } else {
            m_sandboxVideoLoaded = false;
        }
    }
    
    return (true);
}

bool PBEngine::pbeRenderTestSandbox(unsigned long currentTick, unsigned long lastTick){
    
    if (!pbeLoadTestSandbox(false)) return (false);
    
    if (m_RestartTestSandbox) {
        m_RestartTestSandbox = false;
        
        // Clean up any existing video player when restarting sandbox
        if (m_sandboxVideoPlayer) {
            m_sandboxVideoPlayer->pbvpStop();
            m_sandboxVideoPlayer->pbvpUnloadVideo();
            delete m_sandboxVideoPlayer;
            m_sandboxVideoPlayer = nullptr;
            m_sandboxVideoSpriteId = NOSPRITE;
            m_sandboxVideoLoaded = false;
        }
    }

    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
    
    // Render the default background
    pbeRenderDefaultBackground(currentTick, lastTick);
    
    // Render title
    gfxSetColor(m_StartMenuFontId, 255, 165, 0, 255);
    gfxSetScaleFactor(m_StartMenuFontId, 2.0, false);
    gfxRenderShadowString(m_StartMenuFontId, "Test Sandbox", (PB_SCREENWIDTH/2), 15, 2, GFX_TEXTCENTER, 0, 0, 0, 255, 6);
    gfxSetScaleFactor(m_StartMenuFontId, 1.0, false);
    
    // Render button descriptions in the middle of the screen
    int centerX = PB_SCREENWIDTH / 2;
    int startY = 200;
    int lineSpacing = 50;
    
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    gfxSetScaleFactor(m_defaultFontSpriteId, 1.2, false);
    
    // Left Flipper - Bright RED (lighter, more vibrant)
    std::string lfState = m_LFON ? " (ON)" : " (OFF)";
    gfxSetColor(m_defaultFontSpriteId, 255, 64, 64, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "Left Flipper" + lfState + ":", centerX - 200, startY, 1, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "Sequence Test", centerX + 50, startY, 1, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    
    // Right Flipper - Bright RED (lighter, more vibrant)
    std::string rfState = m_RFON ? " (ON)" : " (OFF)";
    gfxSetColor(m_defaultFontSpriteId, 255, 64, 64, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "Right Flipper" + rfState + ":", centerX - 200, startY + lineSpacing, 1, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "Force LED Send (R-G-B) Test", centerX + 50, startY + lineSpacing, 1, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    
    // Left Activate - Bright Cyan-Blue (like blue LED light)
    std::string laState = m_LAON ? " (ON)" : " (OFF)";
    gfxSetColor(m_defaultFontSpriteId, 64, 192, 255, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "Left Activate" + laState + ":", centerX - 200, startY + (2 * lineSpacing), 1, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "Video Playback Test", centerX + 50, startY + (2 * lineSpacing), 1, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    
    // Right Activate - Bright Cyan-Blue (like blue LED light)
    std::string raState = m_RAON ? " (ON)" : " (OFF)";
    gfxSetColor(m_defaultFontSpriteId, 64, 192, 255, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "Right Activate" + raState + ":", centerX - 200, startY + (3 * lineSpacing), 1, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "Test 4", centerX + 50, startY + (3 * lineSpacing), 1, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    
    // Reset scale
    gfxSetScaleFactor(m_defaultFontSpriteId, 1.0, false);
    
    // Update and render video if loaded and playing
    if (m_sandboxVideoLoaded && m_sandboxVideoPlayer) {
        pbvPlaybackState videoState = m_sandboxVideoPlayer->pbvpGetPlaybackState();
        
        if (videoState == PBV_PLAYING) {
            // Update video frame (this decodes next frame, updates texture, and plays audio)
            m_sandboxVideoPlayer->pbvpUpdate(currentTick);
            
            // Track current video alpha for text rendering
            float currentVideoAlpha = 1.0f;
            
            // Handle fade in
            if (m_videoFadingIn) {
                float fadeElapsed = (currentTick - m_videoFadeStartTick) / 1000.0f;
                float fadeProgress = fadeElapsed / m_videoFadeDurationSec;
                
                if (fadeProgress >= 1.0f) {
                    // Fade in complete
                    currentVideoAlpha = 1.0f;
                    gfxSetTextureAlpha(m_sandboxVideoSpriteId, currentVideoAlpha);
                    m_videoFadingIn = false;
                } else {
                    // Fade in progress
                    currentVideoAlpha = fadeProgress;
                    gfxSetTextureAlpha(m_sandboxVideoSpriteId, currentVideoAlpha);
                }
            }
            
            // Handle fade out
            if (m_videoFadingOut) {
                float fadeElapsed = (currentTick - m_videoFadeStartTick) / 1000.0f;
                float fadeProgress = fadeElapsed / m_videoFadeDurationSec;
                
                if (fadeProgress >= 1.0f) {
                    // Fade out complete - stop the video
                    currentVideoAlpha = 0.0f;
                    gfxSetTextureAlpha(m_sandboxVideoSpriteId, currentVideoAlpha);
                    m_videoFadingOut = false;
                    m_sandboxVideoPlayer->pbvpStop();
                } else {
                    // Fade out progress (1.0 to 0.0)
                    currentVideoAlpha = 1.0f - fadeProgress;
                    gfxSetTextureAlpha(m_sandboxVideoSpriteId, currentVideoAlpha);
                }
            }
            
            // Render the video sprite
            m_sandboxVideoPlayer->pbvpRender();
            
            // Render video title over the video at the top, matching video alpha
            // Video is at Y=480, so position title just below that
            unsigned int textAlpha = (unsigned int)(currentVideoAlpha * 255.0f);
            gfxSetColor(m_StartMenuFontId, 139, 0, 0, textAlpha);  // Blood red with matching alpha
            gfxSetScaleFactor(m_StartMenuFontId, 0.75, false);
            gfxRenderShadowString(m_StartMenuFontId, "Town of Darkside", (PB_SCREENWIDTH/2), 495, 2, GFX_TEXTCENTER, 0, 0, 0, textAlpha, 2);
            gfxSetScaleFactor(m_StartMenuFontId, 1.0, false);
        }
    }
    
    // Add instructions to exit
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "Start = exit", PB_SCREENWIDTH - 130, PB_SCREENHEIGHT - 25, 1, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    
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
        gfxRenderShadowString(m_defaultFontSpriteId, "Using RasPin Pinball Engine", tempX, m_CreditsScrollY + (4*spacing), 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "Full code and 3D models available at:", tempX, m_CreditsScrollY + (5*spacing), 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "https://github.com/jeffbock/RasPin_Pinball", tempX, m_CreditsScrollY + (6*spacing), 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "Thanks to Kim, Ally, Katie and Ruth for inspiration", tempX, m_CreditsScrollY + (7*spacing), 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, " ", tempX, m_CreditsScrollY + (8*spacing), 1, GFX_TEXTCENTER, 0,0,0,0,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "Using the these excellent open source libraries", tempX, m_CreditsScrollY + (9*spacing) +2, 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "STB Single Header: http://nothings.org/stb", tempX, m_CreditsScrollY + (10*spacing) +2, 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "JSON.hpp https://github.com/nlohmann/json", tempX, m_CreditsScrollY + (11*spacing) +2, 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "WiringPi https://github.com/WiringPi/WiringPi", tempX, m_CreditsScrollY + (12*spacing) +2, 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "FFmpeg https://github.com/BtbN/FFmpeg-Builds", tempX, m_CreditsScrollY + (13*spacing) +2, 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "Developed using AI and Microsoft Copilot tools", tempX, m_CreditsScrollY + (14*spacing) +2, 1, GFX_TEXTCENTER, 0,0,0,255,2);
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
    static unsigned int msForSwapTest, msForSmallSprite, msForTransformSprite, msForBigSprite;
    unsigned int msRender = 25;
    
    if (!pbeLoadBenchmark (false)) return (false); 

    if (m_RestartBenchmark) {
        m_BenchmarkStartTick =  GetTickCountGfx(); 
        m_BenchmarkDone = false;
        m_RestartBenchmark = false;
        FPSSwap = 0; smallSpriteCount = 0; spriteTransformCount = 0; bigSpriteCount = 0;
        msForSwapTest = 0; msForSmallSprite = 0; msForTransformSprite = 0; msForBigSprite = 0;
        m_TicksPerScene = 3000; m_CountDownTicks = 4000;
        return (true);
    }

    unsigned long elapsedTime = (currentTick - m_BenchmarkStartTick);

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
        while ((GetTickCountGfx() - currentTick) < msRender) {
            gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
            temp = "Clear and Swap Test: Swap " + std::to_string(FPSSwap);
            gfxRenderShadowString(m_defaultFontSpriteId, temp , tempX, 200, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);
            FPSSwap++;
        }

        msForSwapTest += GetTickCountGfx() - currentTick;
        return (true);
    }

    // Small, untransformed sprites per second (with a clear)
    if (elapsedTime < ((m_TicksPerScene *2) + m_CountDownTicks)) {
        gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
        gfxSetScaleFactor(m_StartMenuSwordId, 0.10, false);
        while ((GetTickCountGfx() - currentTick) < msRender) {
            // Get and random X and Y value, within the screen bounds
            x = rand() % PB_SCREENWIDTH;
            y = rand() % PB_SCREENHEIGHT;
            gfxRenderSprite(m_StartMenuSwordId, x, y);
            smallSpriteCount++;
        }

        msForSmallSprite += GetTickCountGfx() - currentTick;
        gfxRenderShadowString(m_defaultFontSpriteId, "Small Sprite Test", tempX, 200, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);
        return (true);
    }

    // Big, untransformed sprites per second (with a clear)
    if (elapsedTime < ((m_TicksPerScene *3) + m_CountDownTicks)) {
        gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
        
        while ((GetTickCountGfx() - currentTick) < msRender ){
            // Get a ramdom value from -ScreenWidth  to +ScreenWidth and -ScreenHeight to +ScreenHeight
            x = rand() % (PB_SCREENWIDTH * 2) - PB_SCREENWIDTH;
            y = rand() % (PB_SCREENHEIGHT * 2) - PB_SCREENHEIGHT;
            gfxRenderSprite(m_BootUpConsoleId, x, y);
            bigSpriteCount++;
        }

        msForBigSprite += GetTickCountGfx() - currentTick;
        gfxRenderShadowString(m_defaultFontSpriteId, "Large Sprite Test", tempX, 200, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);
        return (true);
    }

    // Full Transformed, random untransformed sprites per second (with a clear)
    if (elapsedTime < ((m_TicksPerScene *4) + m_CountDownTicks)) {
        gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
        
        while ((GetTickCountGfx() - currentTick) < msRender) {
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

        msForTransformSprite += GetTickCountGfx() - currentTick;
        gfxRenderShadowString(m_defaultFontSpriteId, "Transformed Sprite Test", tempX, 200, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);
        return (true);
        // Print the final results when done
    }
    
    if (elapsedTime >= ((m_TicksPerScene *4) + m_CountDownTicks)) {

        gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
        temp = "Benchmark Complete - Results";
        gfxRenderShadowString(m_defaultFontSpriteId, temp, tempX, 180, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);
        temp = "Clear + Swap Rate: " + std::to_string(FPSSwap/(msForSwapTest/1000)) + " FPS";
        gfxRenderShadowString(m_defaultFontSpriteId, temp, tempX, 230, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);
        temp = "Small Sprite Rate: " + std::to_string(smallSpriteCount/((msForSmallSprite))) + "k SPS";
        gfxRenderShadowString(m_defaultFontSpriteId, temp, tempX, 255, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);
        temp = "Large Sprite Rate: " + std::to_string(bigSpriteCount/((msForBigSprite))) + "k SPS";
        gfxRenderShadowString(m_defaultFontSpriteId, temp, tempX, 280, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);
        temp = "Transformed Sprite Rate: " + std::to_string(spriteTransformCount/((msForTransformSprite))) + "k SPS";
        gfxRenderShadowString(m_defaultFontSpriteId, temp, tempX, 305, 1, GFX_TEXTCENTER, 0, 0, 255, 255, 2);

        m_BenchmarkDone = true;
    }

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
            if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
                m_mainState = PB_STARTMENU;
                m_RestartMenu = true;
            }
            break;
        }
        case PB_STARTMENU: {
            // If either left button is pressed, subtract 1 from m_currentMenuItem
            if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
                if (inputMessage.inputId == IDI_LEFTFLIPPER) {
                    // Get the current menu item count from g_mainMenu
                    if (m_CurrentMenuItem > 0) {
                        m_CurrentMenuItem--;
                        g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTSWORDHIT);
                    }
                }
                // If either right button is pressed, add 1 to m_currentMenuItem
                if (inputMessage.inputId == IDI_RIGHTFLIPPER) {
                    int temp = g_mainMenu.size();
                    if (m_CurrentMenuItem < (temp -1)) {
                        m_CurrentMenuItem++;
                        g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTSWORDHIT);
                    }
                }

                if ((inputMessage.inputId == IDI_RIGHTACTIVATE) || (inputMessage.inputId == IDI_LEFTACTIVATE)) {
                    // Do something based on the menu item
                    switch (m_CurrentMenuItem) {
                        case (0):  if (m_PassSelfTest) m_mainState = PB_PLAYGAME; break;
                        case (1):  m_mainState = PB_SETTINGS; m_RestartSettings = true; break;
                        case (2):  m_mainState = PB_DIAGNOSTICS; m_RestartDiagnostics = true; break;
                        case (3):  m_mainState = PB_CREDITS; m_RestartCredits = true; break;
                        #if ENABLE_TEST_SANDBOX
                        case (4):  
                            m_mainState = PB_TESTSANDBOX; 
                            m_RestartTestSandbox = true; 
                            // Pause background music when entering sandbox
                            m_soundSystem.pbsPauseMusic();
                            break;
                        #endif
                        default: break;
                    }
                }
            }
            break;
        }
        case PB_DIAGNOSTICS: {

            if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
                if (inputMessage.inputId == IDI_LEFTFLIPPER) {
                    // Get the current menu item count from g_mainMenu
                    if (m_CurrentDiagnosticsItem > 0) {
                        m_CurrentDiagnosticsItem--;
                        g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTSWORDHIT);
                    }
                }
                // If either right button is pressed, add 1 to m_currentMenuItem
                if (inputMessage.inputId == IDI_RIGHTFLIPPER) {
                    int temp = g_diagnosticsMenu.size();
                    if (m_CurrentDiagnosticsItem < (temp -1)) {
                        m_CurrentDiagnosticsItem++;
                        g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTSWORDHIT);
                    }
                }
            }

            if (((inputMessage.inputId == IDI_RIGHTACTIVATE) || (inputMessage.inputId == IDI_LEFTACTIVATE)) && inputMessage.inputState == PB_ON){
                switch (m_CurrentDiagnosticsItem) {
                    case (0): m_mainState = PB_TESTMODE; m_RestartTestMode = true; m_EnableOverlay = false; break;
                    case (1): m_mainState = PB_BENCHMARK; m_RestartBenchmark = true; break;
                    case (2): if ((inputMessage.inputId == IDI_RIGHTACTIVATE) || (inputMessage.inputId == IDI_LEFTACTIVATE)) {
                        if (m_EnableOverlay) m_EnableOverlay = false;
                        else m_EnableOverlay = true;
                        g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTCLICK);
                    }
                    break;
                    case (3): if ((inputMessage.inputId == IDI_RIGHTACTIVATE) || (inputMessage.inputId == IDI_LEFTACTIVATE)) {
                        if (m_ShowFPS) m_ShowFPS = false;
                        else m_ShowFPS = true;
                        g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTCLICK);
                    }
                    break;
                    case (4): if ((inputMessage.inputId == IDI_RIGHTACTIVATE) || (inputMessage.inputId == IDI_LEFTACTIVATE)) {
                        m_mainState = PB_BOOTUP;
                        g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTCLICK);
                    }
                    break;
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
            
            // Record state of flipper and activation buttons
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

            // Check the mode and send output message if needed
            if (m_TestMode == PB_TESTOUTPUT) {
                // Send the output message to the output queue - this will be connected to HW
                if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON && (!m_LAON && !m_RAON)) {
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

                     // Send the message to the output queue using SendOutputMsg function
                    SendOutputMsg(g_outputDef[m_CurrentOutputItem].outputMsg, 
                                g_outputDef[m_CurrentOutputItem].id, 
                                g_outputDef[m_CurrentOutputItem].lastState, 
                                false);
                    }
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
            if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
                if (inputMessage.inputId == IDI_LEFTFLIPPER) {
                    if (m_CurrentSettingsItem > 0) {
                        m_CurrentSettingsItem--;
                        g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTSWORDHIT);
                    }
                }
                // If either right button is pressed, add 1 to m_currentMenuItem
                if (inputMessage.inputId == IDI_RIGHTFLIPPER) {
                    int temp = g_settingsMenu.size();
                    if (m_CurrentSettingsItem < (temp -1)) {
                        m_CurrentSettingsItem++;
                        g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTSWORDHIT);
                    }
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
                            if (m_saveFileData.mainVolume < 10) {
                                m_saveFileData.mainVolume++;
                                m_ampDriver.SetVolume(m_saveFileData.mainVolume * 10);  // Convert 0-10 to 0-100%
                                g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTCLICK);
                            }
                        }
                        if (inputMessage.inputId == IDI_LEFTACTIVATE) {
                            if (m_saveFileData.mainVolume > 0) {
                                m_saveFileData.mainVolume--;
                                m_ampDriver.SetVolume(m_saveFileData.mainVolume * 10);  // Convert 0-10 to 0-100%
                                g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTCLICK);
                            }
                        }
                        break;
                    }
                    case (1): {
                        if (inputMessage.inputId == IDI_RIGHTACTIVATE) {
                            if (m_saveFileData.musicVolume < 10) m_saveFileData.musicVolume++;
                            g_PBEngine.m_soundSystem.pbsSetMusicVolume(m_saveFileData.musicVolume * 10);
                            g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTCLICK);
                        }
                        if (inputMessage.inputId == IDI_LEFTACTIVATE) {
                            if (m_saveFileData.musicVolume > 0) m_saveFileData.musicVolume--;
                            g_PBEngine.m_soundSystem.pbsSetMusicVolume(m_saveFileData.musicVolume * 10);
                            g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTCLICK);
                        }
                        break;
                    }
                    case (2): {
                        if (inputMessage.inputId == IDI_RIGHTACTIVATE) {
                            if (m_saveFileData.ballsPerGame < 9) {
                                m_saveFileData.ballsPerGame++;
                                g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTCLICK);
                            }
                        }
                        if (inputMessage.inputId == IDI_LEFTACTIVATE) {
                            if (m_saveFileData.ballsPerGame > 1) {
                                m_saveFileData.ballsPerGame--;
                                g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTCLICK);
                            }
                        }
                        break;
                    }
                    case (3): {
                        if (inputMessage.inputId == IDI_RIGHTACTIVATE) {
                            switch (m_saveFileData.difficulty) {
                                case PB_EASY: m_saveFileData.difficulty = PB_NORMAL; g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTCLICK);break;
                                case PB_NORMAL: m_saveFileData.difficulty = PB_HARD; g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTCLICK);break;
                                case PB_HARD: m_saveFileData.difficulty = PB_EPIC; g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTCLICK);break;
                                case PB_EPIC: m_saveFileData.difficulty = PB_EPIC; break;
                            }
                        }
                        if (inputMessage.inputId == IDI_LEFTACTIVATE) {
                            switch (m_saveFileData.difficulty) {
                                case PB_EASY: m_saveFileData.difficulty = PB_EASY; break;
                                case PB_NORMAL: m_saveFileData.difficulty = PB_EASY; g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTCLICK);break;
                                case PB_HARD: m_saveFileData.difficulty = PB_NORMAL; g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTCLICK);break;
                                case PB_EPIC: m_saveFileData.difficulty = PB_HARD; g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTCLICK);break;
                            }
                        }
                        break;
                    }
                    case (4): {
                        if ((inputMessage.inputId == IDI_RIGHTACTIVATE) || (inputMessage.inputId == IDI_LEFTACTIVATE)) {
                            resetHighScores();
                            g_PBEngine.m_soundSystem.pbsPlayEffect(EFFECTCLICK);
                        }
                    }
                    default: break;
                }
            }
            break;
        }
        case PB_CREDITS: {
            if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
                m_mainState = PB_STARTMENU;
                m_RestartMenu = true;
            }
        break;
        }
        case PB_BENCHMARK: {
            if (m_BenchmarkDone && (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON)) {
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

        case PB_TESTSANDBOX: {
            // Track button states for display
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
            
            if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
                
                // Start button exits to Start Menu
                if (inputMessage.inputId == IDI_START) {
                    // Clean up video player before exiting
                    if (m_sandboxVideoPlayer) {
                        m_sandboxVideoPlayer->pbvpStop();
                        m_sandboxVideoPlayer->pbvpUnloadVideo();
                        delete m_sandboxVideoPlayer;
                        m_sandboxVideoPlayer = nullptr;
                        m_sandboxVideoSpriteId = NOSPRITE;
                        m_sandboxVideoLoaded = false;
                    }
                    
                    // Resume background music when exiting sandbox
                    m_soundSystem.pbsResumeMusic();
                    
                    m_mainState = PB_STARTMENU;
                    m_RestartMenu = true;
                }
                
                // Left Flipper - Test 1
                if (inputMessage.inputId == IDI_LEFTFLIPPER) {
                    static int testCount = 0;

                    if (testCount % 3 == 0) {  
                        // Sending Test Messages
                        SendOutputMsg(PB_OMSG_GENERIC_IO, IDO_SLINGSHOT, PB_ON, true);
                        SendOutputMsg(PB_OMSG_GENERIC_IO, IDO_POPBUMPER, PB_ON, true);
                        SendOutputMsg(PB_OMSG_GENERIC_IO, IDO_BALLEJECT, PB_ON, true);

                        SendRGBMsg(IDO_LED2, IDO_LED3, IDO_LED4, PB_LEDCYAN, PB_ON, false);
                        SendRGBMsg(IDO_LED5, IDO_LED6, IDO_LED7, PB_LEDYELLOW, PB_ON, false);
                        SendRGBMsg(IDO_LED8, IDO_LED9, IDO_LED10, PB_LEDPURPLE, PB_ON, false);
                    }
                    else if (testCount % 3 == 1) {
                        // Turn on RGB color cycle sequence
                        SendSeqMsg(&PBSeq_RGBColorCycle, PBSeq_RGBColorCycleMask, PB_LOOP, PB_ON);
                    }
                    else {
                        // Turn off RGB color cycle sequence
                        SendSeqMsg(&PBSeq_RGBColorCycle, PBSeq_RGBColorCycleMask, PB_LOOP, PB_OFF);
                    }       
                    
                    testCount++;
                }
                
                // Right Flipper - Test 2
                if (inputMessage.inputId == IDI_RIGHTFLIPPER) {
                    static int testQCount = 0;

                    if (testQCount % 3 == 0) {  
                        // Sending Test Messages - these can be used to test out the pending message queue while sequences are running
                        SendRGBMsg(IDO_LED2, IDO_LED3, IDO_LED4, PB_LEDRED, PB_ON, false);
                        SendRGBMsg(IDO_LED5, IDO_LED6, IDO_LED7, PB_LEDRED, PB_ON, false);
                        SendRGBMsg(IDO_LED8, IDO_LED9, IDO_LED10, PB_LEDRED, PB_ON, false);
                    }
                    else if (testQCount % 3 == 1) {
                        SendRGBMsg(IDO_LED2, IDO_LED3, IDO_LED4, PB_LEDGREEN, PB_ON, false);
                        SendRGBMsg(IDO_LED5, IDO_LED6, IDO_LED7, PB_LEDGREEN, PB_ON, false);
                        SendRGBMsg(IDO_LED8, IDO_LED9, IDO_LED10, PB_LEDGREEN, PB_ON, false);
                    }
                    else {
                        SendRGBMsg(IDO_LED2, IDO_LED3, IDO_LED4, PB_LEDBLUE, PB_ON, false);
                        SendRGBMsg(IDO_LED5, IDO_LED6, IDO_LED7, PB_LEDBLUE, PB_ON, false);
                        SendRGBMsg(IDO_LED8, IDO_LED9, IDO_LED10, PB_LEDBLUE, PB_ON, false);
                    }       
                    
                    testQCount++;
                }
                
                // Left Activate - Test 3 - Video Playback Test (Toggle fade in/out)
                if (inputMessage.inputId == IDI_LEFTACTIVATE) {
                    if (m_sandboxVideoLoaded && m_sandboxVideoPlayer) {
                        pbvPlaybackState videoState = m_sandboxVideoPlayer->pbvpGetPlaybackState();
                        
                        if (videoState == PBV_STOPPED || videoState == PBV_FINISHED) {
                            // Start video playback with fade in
                            m_sandboxVideoPlayer->pbvpSetLooping(true);  // Enable looping
                            gfxSetTextureAlpha(m_sandboxVideoSpriteId, 0.0f);    // Start fully transparent
                            m_sandboxVideoPlayer->pbvpPlay();
                            
                            // Initialize fade in
                            m_videoFadingIn = true;
                            m_videoFadingOut = false;
                            m_videoFadeStartTick = GetTickCountGfx();
                        } else if (videoState == PBV_PLAYING) {
                            // Toggle between fade in and fade out
                            if (!m_videoFadingIn && !m_videoFadingOut) {
                                // Currently fully visible, start fade out
                                m_videoFadingOut = true;
                                m_videoFadingIn = false;
                                m_videoFadeStartTick = GetTickCountGfx();
                            } else if (m_videoFadingOut) {
                                // Currently fading out, reverse to fade in
                                m_videoFadingIn = true;
                                m_videoFadingOut = false;
                                m_videoFadeStartTick = GetTickCountGfx();
                            } else if (m_videoFadingIn) {
                                // Currently fading in, reverse to fade out
                                m_videoFadingOut = true;
                                m_videoFadingIn = false;
                                m_videoFadeStartTick = GetTickCountGfx();
                            }
                        }
                    }
                }
                
                // Right Activate - Test 4
                if (inputMessage.inputId == IDI_RIGHTACTIVATE) {
                    // To be defined
                }
            }
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
        if (i == 0) g_PBEngine.pbeSendConsole("RasPin: Total Inputs: " + std::to_string(NUM_INPUTS)); 
        for (int j = i + 1; j < NUM_INPUTS; j++) {
            if (g_inputDef[i].id == g_inputDef[j].id) {
                g_PBEngine.pbeSendConsole("RasPin: ERROR: Duplicate input ID: " + std::to_string(g_inputDef[i].id));
                g_PBEngine.m_PassSelfTest = false;
            }
            // Check that the board type and pin number are unique
            if (g_inputDef[i].boardType == g_inputDef[j].boardType && 
                g_inputDef[i].boardIndex == g_inputDef[j].boardIndex && 
                g_inputDef[i].pin == g_inputDef[j].pin) {
                g_PBEngine.pbeSendConsole("RasPin: ERROR: Duplicate input board/board index/pin: " + std::to_string(g_inputDef[i].id));
                g_PBEngine.m_PassSelfTest = false;
            }
        }
    }

    // Check the output definitions and ensure no duplicates
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        if (i == 0) g_PBEngine.pbeSendConsole("RasPin: Total Outputs: " + std::to_string(NUM_OUTPUTS)); 
        for (int j = i + 1; j < NUM_OUTPUTS; j++) {
            if (g_outputDef[i].id == g_outputDef[j].id) {
                g_PBEngine.pbeSendConsole("RasPin: ERROR: Duplicate output ID: " + std::to_string(g_outputDef[i].id));
                g_PBEngine.m_PassSelfTest = false;
            }
            // Check that the board type and pin number are unique
            if (g_outputDef[i].boardType == g_outputDef[j].boardType && 
                g_outputDef[i].boardIndex == g_outputDef[j].boardIndex && 
                g_outputDef[i].pin == g_outputDef[j].pin) {
                g_PBEngine.pbeSendConsole("RasPin: ERROR: Duplicate output board/board index/pin: " + std::to_string(g_outputDef[i].id));
                g_PBEngine.m_PassSelfTest = false;
            }
        }
    }

    // Loop through each of the inputs and program the GPIOs and setup the debounce class for each input
     g_PBEngine.pbeSendConsole("RasPin: Intializing Inputs");

    #ifdef EXE_MODE_RASPI
    wiringPiSetupPinType(WPI_PIN_BCM);
    #endif // EXE_MODE_RASPI

    // Set up inputs
    for (int i = 0; i < NUM_INPUTS; i++) {
        if (g_inputDef[i].boardType == PB_RASPI){
            #ifdef EXE_MODE_RASPI
                cDebounceInput debounceInput(g_inputDef[i].pin, g_inputDef[i].debounceTimeMS, true, true);
                g_PBEngine.m_inputPiMap.emplace(g_inputDef[i].id, debounceInput);
            #endif
        }
        else if (g_inputDef[i].boardType == PB_IO) {
            // Configure the pin as input on the appropriate IO chip
            if (g_inputDef[i].boardIndex < NUM_IO_CHIPS) {
                g_PBEngine.m_IOChip[g_inputDef[i].boardIndex].ConfigurePin(g_inputDef[i].pin, PB_INPUT);
                g_PBEngine.m_IOChip[g_inputDef[i].boardIndex].SetPinDebounceTime(g_inputDef[i].pin, g_inputDef[i].debounceTimeMS);
            }
        }
    }

    // Repeat for outputs - key point - "ON" is always active LOW output by design.
     g_PBEngine.pbeSendConsole("RasPin: Intializing Outputs");

    for (int i = 0; i < NUM_OUTPUTS; i++) {
        if (g_outputDef[i].boardType == PB_RASPI){
            #ifdef EXE_MODE_RASPI
                pinMode(g_outputDef[i].pin, OUTPUT);
                if (g_outputDef[i].lastState == PB_ON) {
                    digitalWrite(g_outputDef[i].pin,LOW);
                } else {
                    digitalWrite(g_outputDef[i].pin,HIGH);
                }
            #endif
        }
        else if (g_outputDef[i].boardType == PB_IO) {
            // Configure the pin as output on the appropriate IO chip
            if (g_outputDef[i].boardIndex < NUM_IO_CHIPS) {
                g_PBEngine.m_IOChip[g_outputDef[i].boardIndex].ConfigurePin(g_outputDef[i].pin, PB_OUTPUT);    
                g_PBEngine.m_IOChip[g_outputDef[i].boardIndex].StageOutputPin(g_outputDef[i].pin, g_outputDef[i].lastState);  // Initialize to HIGH
            }
        }
        else if (g_outputDef[i].boardType == PB_LED) {
            // Configure the LED on the appropriate LED chip
            if (g_outputDef[i].boardIndex < NUM_LED_CHIPS) {
                if (g_outputDef[i].lastState == PB_ON)
                    g_PBEngine.m_LEDChip[g_outputDef[i].boardIndex].StageLEDControl(true, g_outputDef[i].pin, LEDOn);  // Initialize to ON
                else
                g_PBEngine.m_LEDChip[g_outputDef[i].boardIndex].StageLEDControl(false, g_outputDef[i].pin, LEDOff);  // Initialize to OFF
            }
        }
    }

    // Send all staged changes to IO and LED chips
    g_PBEngine.pbeSendConsole("RasPin: Sending programmed outputs to pins (LED and IO)");

#ifdef EXE_MODE_RASPI
    SendAllStagedIO();
    SendAllStagedLED();
#endif

    // Hardware validation checks (only do this for actual Raspberry Pi HW)

    #ifdef EXE_MODE_RASPI
    g_PBEngine.pbeSendConsole("RasPin: Verifying HW LED and IO Setup");
    
    // Check LEDDriver MODE1 registers - bit 4 should be 0 (normal operation)
    for (int i = 0; i < NUM_LED_CHIPS; i++) {
        uint8_t mode1 = g_PBEngine.m_LEDChip[i].ReadModeRegister(1);
        if ((mode1 & 0x10) != 0) {  // Check bit 4
            uint8_t address = g_PBEngine.m_LEDChip[i].GetAddress();
            g_PBEngine.pbeSendConsole("RasPin: ERROR: LED chip " + std::to_string(i) + " (address 0x" + std::to_string(address) + ") not detected");
            g_PBEngine.m_PassSelfTest = false;
        }
    }
    
    // Check IODriver POLARITY_PORT0 registers - should return 0x00 (normal polarity)
    for (int i = 0; i < NUM_IO_CHIPS; i++) {
        uint8_t polarity0 = g_PBEngine.m_IOChip[i].ReadPolarityPort(0);
        if (polarity0 != 0x00) {  // Should be 0x00, not 0xFF
            uint8_t address = g_PBEngine.m_IOChip[i].GetAddress();
            g_PBEngine.pbeSendConsole("RasPin: ERROR: IO chip " + std::to_string(i) + " (address 0x" + std::to_string(address) + ") not detected");
            g_PBEngine.m_PassSelfTest = false;
        }
    }
    #endif // EXE_MODE_RASPI

    // Setup and verify the amplifier
    g_PBEngine.pbeSendConsole("RasPin: Initializing amplifier");
    g_PBEngine.m_ampDriver.SetVolume(0);  
    
    if (!g_PBEngine.m_ampDriver.IsConnected()) {
        uint8_t address = g_PBEngine.m_ampDriver.GetAddress();
        g_PBEngine.pbeSendConsole("RasPin: ERROR: Amplifier (address 0x" + std::to_string(address) + ") not detected");
        g_PBEngine.m_PassSelfTest = false;
    } 
    
    return (g_PBEngine.m_PassSelfTest);
}

// Function to create and queue an output message
void PBEngine::SendOutputMsg(PBOutputMsg outputMsg, unsigned int outputId, PBPinState outputState, bool usePulse, stOutputOptions* options)
{
    stOutputMessage outputMessage;
    outputMessage.outputMsg = outputMsg;
    outputMessage.outputId = outputId;
    outputMessage.outputState = outputState;
    outputMessage.usePulse = usePulse;
    outputMessage.sentTick = GetTickCountGfx();
    outputMessage.options = options;
    
    // Lock the output queue mutex and add the message
    // std::lock_guard<std::mutex> lock(m_outputQMutex);
    m_outputQueue.push(outputMessage);
}

// Function to send RGB color messages to three separate LED outputs - makes it RGB LEDs to be a single call
// Specify which output IDs to use for the outputs and then set them by color enum
void PBEngine::SendRGBMsg(unsigned int redId, unsigned int greenId, unsigned int blueId, PBLEDColor color, PBPinState outputState, bool usePulse, stOutputOptions* options)
{
    // Determine the state for each color channel based on the requested color
    PBPinState redState = PB_OFF;
    PBPinState greenState = PB_OFF;
    PBPinState blueState = PB_OFF;
    
    // Set the appropriate color channels based on the color enum
    switch (color) {
        case PB_LEDRED:     redState = outputState; break;
        case PB_LEDGREEN:   greenState = outputState; break;
        case PB_LEDBLUE:    blueState = outputState; break;
        case PB_LEDWHITE:   redState = greenState = blueState = outputState; break;
        case PB_LEDPURPLE:  redState = blueState = outputState; break;
        case PB_LEDYELLOW:  redState = greenState = outputState; break;
        case PB_LEDCYAN:    greenState = blueState = outputState; break;
        default:            break; // Unknown color, turn all off
    }
    
    // If outputState is PB_OFF, override all colors to be off regardless of enum
    if (outputState == PB_OFF) {
        redState = greenState = blueState = PB_OFF;
    }
    
    // Send messages to each color channel
    // Red channel
    SendOutputMsg(PB_OMSG_LED, redId, redState, usePulse, options);
    
    // Green channel  
    SendOutputMsg(PB_OMSG_LED, greenId, greenState, usePulse, options);
    
    // Blue channel
    SendOutputMsg(PB_OMSG_LED, blueId, blueState, usePulse, options);
}

void PBEngine::SendSeqMsg(const LEDSequence* sequence, const uint16_t* mask, PBSequenceLoopMode loopMode, PBPinState outputState)
{
    if (outputState == PB_ON) {
        // Set up sequence options
        stOutputOptions seqOptions;
        seqOptions.loopMode = loopMode;
        seqOptions.setLEDSequence = sequence;
        
        // Copy the mask for all LED chips
        for (int i = 0; i < NUM_LED_CHIPS; i++) {
            seqOptions.activeLEDMask[i] = mask[i];
        }
        
        // Send the sequence start message
        SendOutputMsg(PB_OMSG_LED_SEQUENCE, 0, PB_ON, false, &seqOptions);
    }
    else {
        // Send the sequence stop message
        SendOutputMsg(PB_OMSG_LED_SEQUENCE, 0, PB_OFF, false);
    }
}

// Function to set or unset autoOutput for an input by array index
bool PBEngine::SetAutoOutput(unsigned int id, bool autoOutputEnabled)
{
    if (id < NUM_INPUTS) {
        g_inputDef[id].autoOutput = autoOutputEnabled;
        return true;  // Valid index, updated
    }
    return false;  // Invalid index
}
//==============================================================================
// Device Management Functions
//==============================================================================

// Add a device to the device vector
void PBEngine::AddDevice(PBDevice* device) {
    if (device != nullptr) {
        m_devices.push_back(device);
    }
}

// Clear all devices and free memory
void PBEngine::ClearDevices() {
    for (auto device : m_devices) {
        if (device != nullptr) {
            delete device;
        }
    }
    m_devices.clear();
}

// Execute all registered devices
void PBEngine::ExecuteDevices() {
    for (auto device : m_devices) {
        if (device != nullptr) {
            device->pbdExecute();
        }
    }
}

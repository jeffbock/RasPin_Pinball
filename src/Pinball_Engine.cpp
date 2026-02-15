// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "Pinball_Engine.h"
#include "Pinball.h"
#include "PBSequences.h"
#include "PBDevice.h"
#include <cmath>

// Define NeoPixel global arrays (declared as extern in Pinball_Engine.h)
stNeoPixelNode* g_NeoPixelNodeArray[2];
stNeoPixelNode g_NeoPixelNodes0[g_NeoPixelSize[0]];
stNeoPixelNode g_NeoPixelNodes1[g_NeoPixelSize[1]];

// Define NeoPixel SPI buffer arrays (declared as extern in Pinball_Engine.h)
unsigned char* g_NeoPixelSPIBufferArray[2];
unsigned char g_NeoPixelSPIBuffer0[g_NeoPixelSPIBufferSize[0]];
unsigned char g_NeoPixelSPIBuffer1[g_NeoPixelSPIBufferSize[1]];

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
    m_maxConsoleLines = 256;
    m_consoleTextHeight = 0;
    m_consoleStartLine = 0;

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
    m_videoFadeDurationSec = 2.0f;
    m_sandboxEjector = nullptr;
    
    // Initialize NeoPixel animation variables
    m_sandboxNeoPixelAnimActive = false;
    m_sandboxNeoPixelStepMode = false;  
    m_sandboxNeoPixelPosition = 0;
    m_sandboxNeoPixelMovingUp = true;
    m_sandboxNeoPixelMaxPosition = 0;
    m_sandboxNeoPixelAnimMode = 0;  // Default to original step animation

    // Simple Flip Mode variables
    m_RestartSimpleFlipMode = true;
    m_OverlayWasOnBeforeSimpleFlip = false;

    // Test Mode variables
    m_TestMode = PB_TESTINPUT;
    m_LFON = false; m_RFON=false; m_LAON =false; m_RAON = false; 
    m_CurrentOutputItem = 0;
    m_RestartTestMode = true;

    m_PassSelfTest = true;
    m_RestartBootUp = true;

    /////////////////////
    // Table variables
    /////////////////////
    m_tableState = PBTableState::PBTBL_INIT; 
    m_tableSubScreenState = static_cast<int>(PBTBLStartScreenState::START_START);

    // Tables start screen variables
    m_PBTBLStartDoorId=0; m_PBTBLFlame1Id=0; m_PBTBLFlame2Id=0; m_PBTBLFlame3Id=0;
    m_PBTBLMainScreenBGId=0;
    m_PBTBLResetSpriteId=0;
    m_RestartTable = true;
    
    // Extra ball video variables
    m_extraBallVideoPlayer = nullptr;
    m_extraBallVideoSpriteId = NOSPRITE;
    m_extraBallVideoLoaded = false;
    
    // Reset state initialization
    m_ResetButtonPressed = false;
    m_StateBeforeReset = PBTableState::PBTBL_START;
    m_ScreenBeforeResetState = PBTableState::PBTBL_END;
    m_ScreenBeforeResetSubState = -1;
    m_hasScreenBeforeReset = false;
    
    // Multi-player game state initialization
    m_currentPlayer = 0;
    for (int i = 0; i < 4; i++) {
        m_playerStates[i].reset(3); // Will be updated from save data when game starts
    }
    
    // Main score animation initialization
    m_mainScoreAnimStartTick = 0;
    m_mainScoreAnimActive = false;
    
    // Secondary score slot animation initialization
    for (int i = 0; i < 3; i++) {
        m_secondaryScoreAnims[i].reset();
    }
    
    // Status text animation initialization
    m_statustextFadeIn = true;
    m_statusTextFadeStart = 0;
    m_statusTextDisplayStart = 0;
    m_statusText[0] = "Welcome to Dragons of Destiny Pinball";
    m_statusText[1] = "Collect gold with the bumpers, hire heroes at the Inn";
    m_currentActiveText = 0;
    m_previousActiveText = 0;
    
    // NeoPixel sequence map is initialized on-demand when NeoPixel drivers are created
    
    // Initialize watchdog timer (timerId = 0)
    m_watchdogTimer.timerId = WATCHDOGTIMER_ID;
    m_watchdogTimer.durationMS = 0;
    m_watchdogTimer.startTickMS = 0;
    m_watchdogTimer.expireTickMS = 0;
    
    // Initialize load state tracking
    m_defaultBackgroundLoaded = false;
    m_bootUpLoaded = false;
    m_startMenuLoaded = false;
    m_gameStartLoaded = false;
    m_mainScreenLoaded = false;
    m_resetLoaded = false;
    
    // Auto output control - default to disable since the menus launch first
    m_autoOutputEnable = false;
    
    // Screen manager initialization
    m_currentDisplayedTableState = PBTableState::PBTBL_END;
    m_currentDisplayedSubScreen = -1;
    m_currentScreenStartTick = 0;
 }

 PBEngine::~PBEngine(){

    // Clean up video player
    if (m_sandboxVideoPlayer) {
        delete m_sandboxVideoPlayer;
        m_sandboxVideoPlayer = nullptr;
    }
    
    // Clean up extra ball video player
    if (m_extraBallVideoPlayer) {
        delete m_extraBallVideoPlayer;
        m_extraBallVideoPlayer = nullptr;
    }
    
    // Clean up all registered devices
    pbeClearDevices();

}

bool PBEngine::pbeLoadSaveFile(bool loadDefaults, bool resetScores){
    
    // Try and load the save file - if it exists, load it, place it in the stSaveFileData structure and return true
    // otherwise load the defaults if not present, or if loadDefaults is true

    bool failed = false;
    bool osMismatch = false;

    std::ifstream saveFile(SAVEFILENAME, std::ios::binary);
    if (saveFile.is_open() && saveFile.good()) {
        saveFile.read(reinterpret_cast<char*>(&m_saveFileData), sizeof(stSaveFileData));
        if (!saveFile.good()) {
            failed = true;
        } else {
            // Check if OS matches
            #ifdef EXE_MODE_WINDOWS
            bool currentIsWindows = true;
            #else
            bool currentIsWindows = false;
            #endif
            
            if (m_saveFileData.isWindows != currentIsWindows) {
                osMismatch = true;
                failed = true;
            }
        }
        saveFile.close();
        
        // If OS mismatch or failure detected, delete the file and reinitialize the structure
        if (osMismatch || failed) {
            if (osMismatch) {
                std::remove(SAVEFILENAME);
                pbeSendConsole("RasPin: WARNING: Save file created on different OS - file deleted and will be recreated");
            }
            
            // Reinitialize m_saveFileData with a fresh structure to avoid corrupted std::string objects
            // This is critical because std::string is not POD and binary loading corrupts internal pointers
            m_saveFileData = stSaveFileData();
        }
    } else {
        failed = true;
    }
        
    if ((loadDefaults) || failed){
        // Set default values for the saveData structure
        #ifdef EXE_MODE_WINDOWS
        m_saveFileData.isWindows = true;
        #else
        m_saveFileData.isWindows = false;
        #endif
        m_saveFileData.mainVolume = MAINVOLUME_DEFAULT;
        m_saveFileData.musicVolume = MUSICVOLUME_DEFAULT;
        m_saveFileData.ballsPerGame = BALLSPERGAME_DEFAULT;
        m_saveFileData.difficulty = PB_NORMAL;
    }

    // Set the stHighScoreData array scores to zero and name to "JEF"
    if ((resetScores) || failed) resetHighScores();
    
    // If we had to recreate the structure, save it immediately to create a valid file
    if (failed) {
        pbeSaveFile();
        pbeSendConsole("RasPin: Save file recreated with default values");
    }
    
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
    if (!saveFile) {
        pbeSendConsole("ERROR: Failed to open save file for writing");
        return (false); // Failed to open the file for writing
    }

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

// Helper function to calculate max console lines that fit on screen from a starting Y position
unsigned int PBEngine::pbeGetMaxConsoleLines(unsigned int startingY) {
    // Guard against uninitialized text height - if 0, font isn't loaded yet
    if (m_consoleTextHeight == 0) return 0;
    
    // Line height is text height plus 1 pixel spacing
    unsigned int lineHeight = m_consoleTextHeight + 1;
    
    unsigned int availableHeight = PB_SCREENHEIGHT - startingY;
    return availableHeight / lineHeight;
}

void PBEngine::pbeRenderConsole(unsigned int startingX, unsigned int startingY, unsigned int startLine){
    
     // Starting position for rendering
     unsigned int x = startingX;
     unsigned int y = startingY;
 
     // Calculate how many lines can fit on the screen
     unsigned int lineHeight = m_consoleTextHeight + 1;
     unsigned int maxLinesOnScreen = pbeGetMaxConsoleLines(startingY);
     
     // Get the total number of console lines
     unsigned int totalLines = (unsigned int)m_consoleQueue.size();
     
     // Determine the actual start line to render from
     unsigned int actualStartLine = 0;
     
     if (totalLines <= maxLinesOnScreen) {
         // If text lines <= lines that fit on screen, render from line 0 regardless of requested start
         actualStartLine = 0;
     } else {
         // Calculate the last possible start line that keeps last line at bottom
         unsigned int maxStartLine = totalLines - maxLinesOnScreen;
         
         // If requested start line would leave empty space at bottom, adjust to maxStartLine
         if (startLine > maxStartLine) {
             actualStartLine = maxStartLine;
         } else {
             actualStartLine = startLine;
         }
     }
 
     // Iterate through the vector starting from actualStartLine and render each string
     unsigned int lineIndex = 0;
     for (unsigned int i = actualStartLine; i < totalLines && lineIndex < maxLinesOnScreen; i++, lineIndex++) {
         gfxRenderString(m_defaultFontSpriteId, m_consoleQueue[i], x, y, 1, GFX_TEXTLEFT);
         y += lineHeight; // Move to the next row
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
        case PB_SIMPLEFLIPMODE: return pbeRenderSimpleFlipMode(currentTick, lastTick); break;
        default:
            pbeSendConsole("ERROR: Unknown main state in pbeRenderScreen");
            return (false);
    }

    return (false);
}

// Load reasources for the boot up screen
bool PBEngine::pbeLoadDefaultBackground(){
    
    if (m_defaultBackgroundLoaded) return (true);

    pbeSendConsole("RasPin: Loading default background resources");

    m_BootUpConsoleId = gfxLoadSprite("Console", "src/resources/textures/Console.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, false, true);
    gfxSetColor(m_BootUpConsoleId, 255, 255, 255, 196);
    gfxSetScaleFactor(m_BootUpConsoleId, 1.05, false);

    m_BootUpStarsId = gfxLoadSprite("Stars", "src/resources/textures/stars.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, false, true);
    gfxSetColor(m_BootUpStarsId,  9, 28, 42, 128);
    gfxSetScaleFactor(m_BootUpStarsId, 4.0, false);

    m_BootUpStarsId2 = gfxInstanceSprite(m_BootUpStarsId);
    gfxSetColor(m_BootUpStarsId2, 9, 28, 42, 128);
    gfxSetScaleFactor(m_BootUpStarsId2, 1.5, false);

    m_BootUpStarsId3 = gfxInstanceSprite(m_BootUpStarsId);
    gfxSetColor(m_BootUpStarsId3,  9, 28, 42, 128);
    gfxSetScaleFactor(m_BootUpStarsId3, 0.4, false);
    
    m_BootUpStarsId4 = gfxInstanceSprite(m_BootUpStarsId);
    gfxSetColor(m_BootUpStarsId4,  9, 28, 42, 128);
    gfxSetScaleFactor(m_BootUpStarsId4, 0.1, false); 

    if (m_BootUpConsoleId == NOSPRITE || m_BootUpStarsId == NOSPRITE || m_BootUpStarsId2 == NOSPRITE ||  
        m_BootUpStarsId3 == NOSPRITE ||  m_BootUpStarsId4 == NOSPRITE ) {
        return (false);
    }

    m_defaultBackgroundLoaded = true;

    return (true);
}

// Load reasources for the boot up screen
bool PBEngine::pbeLoadBootUp(){
    
    if (m_bootUpLoaded) return (true);

    if (!pbeLoadDefaultBackground()) return (false);

    pbeSendConsole("RasPin: Loading boot screen resources");
    
    // Load the bootup screen items
    
    m_BootUpTitleBarId = gfxLoadSprite("Title Bar", "", GFX_NONE, GFX_NOMAP, GFX_UPPERLEFT, false, false);
    gfxSetColor(m_BootUpTitleBarId, 0, 0, 255, 255);
    gfxSetWH(m_BootUpTitleBarId, PB_SCREENWIDTH, 40);

    if (m_BootUpTitleBarId == NOSPRITE) return (false);

    // Start the menu music
    g_PBEngine.m_soundSystem.pbsPlayMusic(SOUNDMENUTHEME);

    m_bootUpLoaded = true;
    return (true);
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
   gfxRenderSprite(m_BootUpStarsId, PB_SCREENWIDTH/2 + 42, (PB_SCREENHEIGHT / 2) + 220);

   gfxSetRotateDegrees(m_BootUpStarsId2, (degreesPerTick2 * (float) tickDiff), true);
   gfxRenderSprite(m_BootUpStarsId2, PB_SCREENWIDTH/2 + 42, (PB_SCREENHEIGHT / 2) + 205);

   gfxSetRotateDegrees(m_BootUpStarsId3, (degreesPerTick3 * (float) tickDiff), true);
   gfxRenderSprite(m_BootUpStarsId3, PB_SCREENWIDTH/2 + 42, (PB_SCREENHEIGHT / 2) + 170);

   gfxSetRotateDegrees(m_BootUpStarsId4, (degreesPerTick4 * (float) tickDiff), true);
   gfxRenderSprite(m_BootUpStarsId4, PB_SCREENWIDTH/2 + 42, (PB_SCREENHEIGHT / 2) + 145);

   return (true);
}

// Render the bootup screen
bool PBEngine::pbeRenderBootScreen(unsigned long currentTick, unsigned long lastTick){
        
    if (!pbeLoadBootUp()) {
        pbeSendConsole("ERROR: Failed to load boot screen resources");
        return (false);
    }

    if (m_RestartBootUp) {
        m_RestartBootUp = false;
                
        // Initialize console start line following render rules
        unsigned int maxLinesOnScreen = pbeGetMaxConsoleLines(CONSOLE_START_Y);
        unsigned int totalLines = (unsigned int)m_consoleQueue.size();
        
        if (totalLines <= maxLinesOnScreen) {
            m_consoleStartLine = 0;
        } else {
            m_consoleStartLine = totalLines - maxLinesOnScreen;
        }
    }

    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);

    // Render the default background
    pbeRenderDefaultBackground (currentTick, lastTick);
         
    gfxRenderSprite(m_BootUpTitleBarId, 0, 0);
    gfxRenderShadowString(m_defaultFontSpriteId, "RasPin - Copyright 2025 Jeff Bock (Right/Left to scroll, Start = Exit)", (PB_SCREENWIDTH / 2), 10, 1, GFX_TEXTCENTER, 0, 0, 0, 255, 2);

    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);   
    pbeRenderConsole(1, CONSOLE_START_Y, m_consoleStartLine);

    return (true);
}

// Function generically renders a menu with a cursor at an x/y location, simplifies the use of creating new menus

bool PBEngine::pbeRenderGenericMenu(unsigned int cursorSprite, unsigned int fontSprite, unsigned int selectedItem, 
                                    int x, int y, int lineSpacing, std::map<unsigned int, std::string>* menuItems,
                                    bool useShadow, bool useCursor, unsigned int redShadow, 
                                    unsigned int greenShadow, unsigned int blueShadow, unsigned int alphaShadow, unsigned int shadowOffset,
                                    unsigned int disabledItemsMask){

    // Check that the cursor sprite and font sprite are valid, and fontSprite is actually a font
    if ((gfxIsSprite(cursorSprite) == false) && (useCursor ==  true)) {
        pbeSendConsole("ERROR: Invalid cursor sprite in pbeRenderGenericMenu");
        return (false);
    }
    if (gfxIsFontSprite(fontSprite) == false) {
        pbeSendConsole("ERROR: Invalid font sprite in pbeRenderGenericMenu");
        return (false);
    }

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

        // Check if this item is disabled (bit is set in the mask)
        bool isDisabled = (disabledItemsMask & (1 << item.first)) != 0;

        // Render the menu item with shadow depending on the selected item
        if (selectedItem == item.first) {
            // If disabled, render in grey even for selected items
            if (isDisabled) {
                gfxSetColor(fontSprite, 128, 128, 128, 255);
            }
            
            if (useShadow && !isDisabled) {
                gfxRenderShadowString(fontSprite, itemText, menuX + cursorWidth + CURSOR_TO_MENU_SPACING, menuY, 1, GFX_TEXTLEFT, redShadow, greenShadow, blueShadow, alphaShadow, shadowOffset);
            } else {
                gfxRenderString(fontSprite, itemText, menuX + cursorWidth + CURSOR_TO_MENU_SPACING, menuY, 1, GFX_TEXTLEFT);
            }

            // Restore original color if it was changed
            if (isDisabled) {
                gfxSetColor(fontSprite, 255, 255, 255, 255);
            }

            if (useCursor) gfxRenderSprite (cursorSprite, cursorX, menuY + cursorCenterOffset);
        } else {
            // If disabled, render in grey (128, 128, 128)
            if (isDisabled) {
                gfxSetColor(fontSprite, 128, 128, 128, 255);
            }
            gfxRenderString(fontSprite, itemText, menuX + cursorWidth + CURSOR_TO_MENU_SPACING, menuY, 1, GFX_TEXTLEFT);
            // Restore original color if it was changed
            if (isDisabled) {
                gfxSetColor(fontSprite, 255, 255, 255, 255);
            }
        }
        itemIndex++;
    }
    
    return (true);
}

// Main Menu Screen Setup

bool PBEngine::pbeLoadStartMenu(){

    if (m_startMenuLoaded) return (true);

    // Load the font for the start menu
    m_StartMenuFontId = gfxLoadSprite("Start Menu Font", MENUFONT, GFX_PNG, GFX_TEXTMAP, GFX_UPPERLEFT, true, true);
    if (m_StartMenuFontId == NOSPRITE) return (false);


    gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);

    m_StartMenuSwordId = gfxLoadSprite("Start Menu Sword", MENUSWORD, GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, false, true);
    if (m_StartMenuSwordId == NOSPRITE) return (false);

    gfxSetScaleFactor(m_StartMenuSwordId, 0.35, false);
    gfxSetColor(m_StartMenuSwordId, 200, 200, 200, 200);

    m_startMenuLoaded = true;
    return (true);
}

// Renders the main menu
bool PBEngine::pbeRenderStartMenu(unsigned long currentTick, unsigned long lastTick){

   if (!pbeLoadStartMenu()) {
       pbeSendConsole("ERROR: Failed to load start menu resources");
       return (false); 
   } 

   if (m_RestartMenu) {
        m_CurrentSettingsItem = 0; 
        m_RestartMenu = false;
        gfxSetScaleFactor(m_StartMenuSwordId, 0.9, false);
        gfxSetRotateDegrees(m_StartMenuSwordId, 0.0f, false);
        // Restart the menu music when returning to main menu
        m_soundSystem.pbsPlayMusic(SOUNDMENUTHEME);
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
    // If self-test failed, disable "Play Pinball" menu item (index 0)
    unsigned int disabledItemsMask = m_PassSelfTest ? 0x0 : (1 << 0);  // Bit 0 = "Play Pinball"
    pbeRenderGenericMenu(m_StartMenuSwordId, m_StartMenuFontId, m_CurrentMenuItem, 620, 260, 25, &g_mainMenu, true, true, 64, 0, 255, 255, 8, disabledItemsMask);

    // Add insturctions to the bottom of the screen - calculate the x position based on string length
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "L/R flip = move", PB_SCREENWIDTH - 200, PB_SCREENHEIGHT - 50, 1, GFX_TEXTLEFT, 0,0,0,255,2);
    gfxRenderShadowString(m_defaultFontSpriteId, "L/R active = select", PB_SCREENWIDTH - 200, PB_SCREENHEIGHT - 25, 1, GFX_TEXTLEFT, 0,0,0,255,2);

    return (true);
}

// Test Mode Screen
bool PBEngine::pbeLoadTestMode(){
    // Test mode currently only requires the default background and font
    if (!pbeLoadDefaultBackground()) {
        pbeSendConsole("ERROR: Failed to load default background in pbeLoadTestMode");
        return (false);
    }

    return (true);
}

bool PBEngine::pbeRenderTestMode(unsigned long currentTick, unsigned long lastTick){

    if (!pbeLoadTestMode()) {
        pbeSendConsole("ERROR: Failed to load test mode resources");
        return (false); 
    } 

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

// Helper functions to convert enums to strings for overlay display

std::string PBEngine::MainStateToString(PBMainState state) {
    switch (state) {
        case PB_BOOTUP: return "BOOTUP";
        case PB_STARTMENU: return "STARTMENU";
        case PB_PLAYGAME: return "PLAYGAME";
        case PB_TESTMODE: return "TESTMODE";
        case PB_BENCHMARK: return "BENCHMARK";
        case PB_CREDITS: return "CREDITS";
        case PB_SETTINGS: return "SETTINGS";
        case PB_DIAGNOSTICS: return "DIAGNOSTICS";
        case PB_TESTSANDBOX: return "TESTSANDBOX";
        case PB_SIMPLEFLIPMODE: return "SIMPLEFLIPMODE";
        default: return "UNKNOWN";
    }
}

std::string PBEngine::TableStateToString(PBTableState state) {
    switch (state) {
        case PBTableState::PBTBL_INIT: return "INIT";
        case PBTableState::PBTBL_START: return "START";
        case PBTableState::PBTBL_MAIN: return "MAIN";
        case PBTableState::PBTBL_RESET: return "RESET";
        case PBTableState::PBTBL_END: return "END";
        default: return "UNKNOWN";
    }
}

std::string PBEngine::TableScreenStateToString(PBTableState tableState, int subScreenState) {
    switch (tableState) {
        case PBTableState::PBTBL_INIT:
            return "INIT (No Sub States)";
        case PBTableState::PBTBL_RESET:
            return "RESET (No Sub States)";
        case PBTableState::PBTBL_START: {
            PBTBLStartScreenState startState = static_cast<PBTBLStartScreenState>(subScreenState);
            switch (startState) {
                case PBTBLStartScreenState::START_START: return "START_START";
                case PBTBLStartScreenState::START_INST: return "START_INST";
                case PBTBLStartScreenState::START_SCORES: return "START_SCORES";
                case PBTBLStartScreenState::START_OPENDOOR: return "START_OPENDOOR";
                case PBTBLStartScreenState::START_END: return "START_END";
                default: return "START_UNKNOWN";
            }
        }
        case PBTableState::PBTBL_MAIN: {
            PBTBLMainScreenState mainState = static_cast<PBTBLMainScreenState>(subScreenState);
            switch (mainState) {
                case PBTBLMainScreenState::MAIN_NORMAL: return "MAIN_NORMAL";
                case PBTBLMainScreenState::MAIN_EXTRABALL: return "MAIN_EXTRABALL";
                case PBTBLMainScreenState::MAIN_END: return "MAIN_END";
                default: return "MAIN_UNKNOWN";
            }
        }
        case PBTableState::PBTBL_END:
            return "END (No Sub States)";
        default:
            return "UNKNOWN";
    }
}

// I/O Overlay Rendering - Shows current state of all inputs and outputs

bool PBEngine::pbeRenderOverlay(unsigned long currentTick, unsigned long lastTick){
    
    // Uses the same resources as test mode
    if (!pbeLoadTestMode()) {
        pbeSendConsole("ERROR: Failed to load test mode resources for overlay selection");
        return (false); 
    } 

    // Set up transparent background for overlay
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    
    // STATE DISPLAY Section - Display at top of screen
    PBTableState overlayTableState = (m_currentDisplayedTableState != PBTableState::PBTBL_END) ? m_currentDisplayedTableState : m_tableState;
    int overlaySubState = (m_currentDisplayedSubScreen >= 0) ? m_currentDisplayedSubScreen : m_tableSubScreenState;
    std::string stateDisplay = "MainState: " + MainStateToString(m_mainState) + 
                                "  TableState: " + TableStateToString(m_tableState) + 
                                "  ScreenState: " + TableScreenStateToString(overlayTableState, overlaySubState);
    
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 0, 255);  // Yellow for state display
    gfxRenderShadowString(m_defaultFontSpriteId, stateDisplay, (PB_SCREENWIDTH / 2), 5, 0.4, GFX_TEXTCENTER, 0, 0, 0, 255, 2);
    
    // INPUTS Section
    gfxSetColor(m_defaultFontSpriteId, 0, 255, 255, 255);  // Cyan for inputs header
    gfxRenderShadowString(m_defaultFontSpriteId, "INPUTS", 20, 25, 0.4, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    
    // Render inputs in one column (48 items will fit with 40% scale)
    for (int i = 0; i < NUM_INPUTS; i++) {
        std::string temp = g_inputDef[i].inputName + ": ";
        
        int x = 20;                   // Single column
        int y = 45 + (i * 20);        // Start below header with proper spacing
        
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
        gfxRenderShadowString(m_defaultFontSpriteId, stateText, x + 200, y, 0.4, GFX_TEXTLEFT, 0, 0, 0, 255, 1);
    }
    
    // OUTPUTS Section - Position moved 225 pixels further right total
    int outputStartX = PB_SCREENWIDTH - 280;  // Moved 225 pixels right total (was -480, now -255)
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 0, 255);  // Yellow for outputs header
    gfxRenderShadowString(m_defaultFontSpriteId, "OUTPUTS", outputStartX, 25, 0.4, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    
    // Render outputs in one column (48 items will fit with 40% scale)
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        std::string temp = g_outputDef[i].outputName + ": ";
        
        int x = outputStartX;         // Single column
        int y = 45 + (i * 20);        // Start below header with proper spacing
        
        // Render output name
        gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
        gfxRenderShadowString(m_defaultFontSpriteId, temp, x, y, 0.4, GFX_TEXTLEFT, 0, 0, 0, 255, 1);
        
        // Render state with appropriate color
        std::string stateText;
        // Check if this is a NeoPixel output
        if (g_outputDef[i].boardType == PB_NEOPIXEL) {
            stateText = "NeoPixel";
            gfxSetColor(m_defaultFontSpriteId, 128, 128, 128, 255);  // Gray for NeoPixel
        } else {
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
        }
        gfxRenderShadowString(m_defaultFontSpriteId, stateText, x + 200, y, 0.4, GFX_TEXTLEFT, 0, 0, 0, 255, 1);
    }
    
    return (true);   
}

// Settings Menu Screen

bool PBEngine::pbeLoadSettings(){

    if (!pbeLoadStartMenu()) {
        pbeSendConsole("ERROR: Failed to load start menu resources in pbeLoadSettings");
        return (false); 
    }

    return (true);
}

bool PBEngine::pbeRenderSettings(unsigned long currentTick, unsigned long lastTick){

    if (!pbeLoadSettings()) {
        pbeSendConsole("ERROR: Failed to load settings screen resources");
        return (false); 
    } 

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

bool PBEngine::pbeLoadDiagnostics(){
    if (!pbeLoadStartMenu()) {
        pbeSendConsole("ERROR: Failed to load start menu resources in pbeLoadDiagnostics");
        return (false); 
    }

    return (true);
}

bool PBEngine::pbeRenderDiagnostics(unsigned long currentTick, unsigned long lastTick){

    if (!pbeLoadDiagnostics()) {
        pbeSendConsole("ERROR: Failed to load diagnostics screen resources");
        return (false); 
    } 

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

bool PBEngine::pbeLoadTestSandbox(){
    if (!pbeLoadStartMenu()) {
        pbeSendConsole("ERROR: Failed to load start menu resources in pbeLoadTestSandbox");
        return (false);
    }
    
    // Create and register sandbox ejector device if not already created
    if (!m_sandboxEjector) {
        m_sandboxEjector = new pbdEjector(this, IDI_IO0P07_EJECTSW2, IDO_IOD0P08_LSLING, IDO_IO2P08_EJECT);
        pbeAddDevice(m_sandboxEjector);
    }
    
    // Initialize video player if not already created
    if (!m_sandboxVideoPlayer) {
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
    
    // Note: Sandbox NeoPixel test code removed - use NeoPixel drivers from g_outputDef instead
    // The NeoPixel drivers are now created automatically during pbeSetupIO() based on g_outputDef
    // and use pre-allocated arrays from g_NeoPixelNodeArray to avoid dynamic memory allocation
    
    return (true);
}

bool PBEngine::pbeRenderTestSandbox(unsigned long currentTick, unsigned long lastTick){
    
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
    
    if (!pbeLoadTestSandbox()) {
        pbeSendConsole("ERROR: Failed to load test sandbox resources");
        return (false);
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
    std::string neoPixelMode = m_sandboxNeoPixelStepMode ? "NeoPixel Test (Step Mode)" : "NeoPixel Test (Timer Mode)";
    std::string animMode;
    switch(m_sandboxNeoPixelAnimMode) {
        case 0: animMode = "Original Step"; break;
        case 1: animMode = "Gradual Fade"; break;
        case 2: animMode = "Sweep From Ends"; break;
        case 3: animMode = "Toggle"; break;
        case 4: animMode = "Split Toggle"; break;
        case 5: animMode = "Strobe"; break;
        case 6: animMode = "Snake"; break;
        default: animMode = "Unknown"; break;
    }
    std::string neoPixelInfo = neoPixelMode + " - " + animMode;
    gfxRenderShadowString(m_defaultFontSpriteId, "Toggle NeoPixel Anim", centerX + 50, startY + lineSpacing, 1, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    
    // Reset Button - Show mode cycling
    gfxSetColor(m_defaultFontSpriteId, 255, 200, 0, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "Reset Button:", centerX - 180, startY + (1.6 * lineSpacing), 1, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "Cycle Mode: " + animMode, centerX + 50, startY + (1.6 * lineSpacing), 1, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    
    // Left Activate - Bright Cyan-Blue (like blue LED light)
    std::string laState = m_LAON ? " (ON)" : " (OFF)";
    gfxSetColor(m_defaultFontSpriteId, 64, 192, 255, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "Left Activate" + laState + ":", centerX - 200, startY + (2.5 * lineSpacing), 1, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "Video Playback Test", centerX + 50, startY + (2.5 * lineSpacing), 1, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    
    // Right Activate - Bright Cyan-Blue (like blue LED light)
    std::string raState = m_RAON ? " (ON)" : " (OFF)";
    gfxSetColor(m_defaultFontSpriteId, 64, 192, 255, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "Right Activate" + raState + ":", centerX - 200, startY + (3.5 * lineSpacing), 1, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    std::string ejectorStatus = (m_sandboxEjector && m_sandboxEjector->pdbIsRunning()) ? "RUNNING" : "STOPPED";
    gfxRenderShadowString(m_defaultFontSpriteId, "Ejector Test - " + ejectorStatus, centerX + 50, startY + (3.5 * lineSpacing), 1, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    
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

bool PBEngine::pbeLoadCredits(){

    if (!pbeLoadDefaultBackground()) {
        pbeSendConsole("ERROR: Failed to load default background in pbeLoadCredits");
        return (false);
    }
    return (true);
}

bool PBEngine::pbeRenderCredits(unsigned long currentTick, unsigned long lastTick){

    if (!pbeLoadCredits()) {
        pbeSendConsole("ERROR: Failed to load credits screen resources");
        return (false);
    }

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
        gfxRenderShadowString(m_defaultFontSpriteId, "Designed, Art and Programmed by: Jeffrey Bock", tempX, m_CreditsScrollY + (2*spacing), 1, GFX_TEXTCENTER, 0,0,0,255,2);
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
        gfxRenderShadowString(m_defaultFontSpriteId, "SDL https://github.com/libsdl-org/SDL", tempX, m_CreditsScrollY + (14*spacing) +2, 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "SDL Mixer https://github.com/libsdl-org/SDL_mixer", tempX, m_CreditsScrollY + (15*spacing) +2, 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "Characters developed at https://www.heroforge.com/", tempX, m_CreditsScrollY + (16*spacing) +2, 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "Various images, sounds and music from https://pixabay.com/", tempX, m_CreditsScrollY + (17*spacing) +2, 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxRenderShadowString(m_defaultFontSpriteId, "Microsoft Copilot AI tools utilized with art and code ", tempX, m_CreditsScrollY + (18*spacing) +2, 1, GFX_TEXTCENTER, 0,0,0,255,2);
        gfxSetScaleFactor(m_defaultFontSpriteId, 1.0, false);
    }

    return (true);   
}
 
// Simple Flip Mode Screen

bool PBEngine::pbeLoadSimpleFlipMode(){
    // Simple Flip Mode uses test mode resources
    if (!pbeLoadTestMode()) {
        pbeSendConsole("ERROR: Failed to load test mode resources in pbeLoadSimpleFlipMode");
        return (false);
    }
    
    return (true);
}

bool PBEngine::pbeRenderSimpleFlipMode(unsigned long currentTick, unsigned long lastTick){
    
    if (!pbeLoadSimpleFlipMode()) {
        pbeSendConsole("ERROR: Failed to load simple flip mode resources");
        return (false);
    }
    
    if (m_RestartSimpleFlipMode) {
        m_RestartSimpleFlipMode = false;
        
        // Remember if overlay was already on before entering
        m_OverlayWasOnBeforeSimpleFlip = m_EnableOverlay;
        
        // Enable overlay for this mode
        if (!m_EnableOverlay) {
            m_EnableOverlay = true;
        }
        
        // Enable auto outputs
        SetAutoOutputEnable(true);
    }
    
    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
    
    // Render the default background
    pbeRenderDefaultBackground(currentTick, lastTick);
    
    // Render title (matching other menu title formats)
    gfxSetColor(m_StartMenuFontId, 255, 165, 0, 255);
    gfxSetScaleFactor(m_StartMenuFontId, 2.0, false);
    gfxRenderShadowString(m_StartMenuFontId, "Simple Flip Mode", (PB_SCREENWIDTH/2), 15, 2, GFX_TEXTCENTER, 0, 0, 0, 255, 6);
    gfxSetScaleFactor(m_StartMenuFontId, 1.5, false);
    
    // Render exit instructions in lower right
    gfxSetColor(m_defaultFontSpriteId, 255, 255, 255, 255);
    gfxRenderShadowString(m_defaultFontSpriteId, "Start = exit", PB_SCREENWIDTH - 150, PB_SCREENHEIGHT - 25, 1, GFX_TEXTLEFT, 0, 0, 0, 255, 2);
    
    return (true);
}

// Benchmark Screen
bool PBEngine::pbeLoadBenchmark(){

    // Benchmark will just use default font and the start menu items
    if (!pbeLoadStartMenu()) {
        pbeSendConsole("ERROR: Failed to load start menu resources in pbeLoadBenchmark");
        return (false); 
    }
    return (true);
}

bool PBEngine::pbeRenderBenchmark(unsigned long currentTick, unsigned long lastTick){

    static unsigned int FPSSwap, smallSpriteCount, spriteTransformCount, bigSpriteCount;
    static unsigned int msForSwapTest, msForSmallSprite, msForTransformSprite, msForBigSprite;
    unsigned int msRender = 25;
    
    if (!pbeLoadBenchmark()) {
        pbeSendConsole("ERROR: Failed to load benchmark resources");
        return (false); 
    } 

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

// Helper function to force a state update by adding an empty message to the input queue
// This is useful when a forced state update is needed in the state update code that's not tied to rendering (which happens every frame)
void PBEngine::pbeForceUpdateState() {
    stInputMessage emptyMessage;
    emptyMessage.inputMsg = PB_IMSG_EMPTY;
    emptyMessage.inputId = 0;
    emptyMessage.inputState = PB_ON;
    m_inputQueue.push(emptyMessage);
}

void PBEngine::pbeUpdateState(stInputMessage inputMessage){
    
    // Handle timer messages
    if (inputMessage.inputMsg == PB_IMSG_TIMER) {
        if (inputMessage.inputId == WATCHDOGTIMER_ID) {
            pbeSendConsole("Watchdog timer (ID=0) fired!");
        } else if (inputMessage.inputId == 200) {
            pbeSendConsole("Normal timer (ID=200) fired!");
        }
    }
    
    switch (m_mainState) {
        case PB_BOOTUP: {
            // Handle button presses in BOOTUP state
            if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
                // Only start button exits to start menu
                if (inputMessage.inputId == IDI_RPIOP06_START) {
                    m_mainState = PB_STARTMENU;
                    m_RestartMenu = true;
                }
                // Left flipper or left activate: scroll console one line earlier (up)
                else if (inputMessage.inputId == IDI_RPIOP27_LFLIP || inputMessage.inputId == IDI_RPIOP05_LACTIVATE) {
                    if (m_consoleStartLine > 0) {
                        m_consoleStartLine--;
                    }
                }
                // Right flipper or right activate: scroll console one line later (down)
                else if (inputMessage.inputId == IDI_RPIOP17_RFLIP || inputMessage.inputId == IDI_RPIOP22_RACTIVATE) {
                    // Calculate the maximum start line using helper function
                    unsigned int maxLinesOnScreen = pbeGetMaxConsoleLines(CONSOLE_START_Y);
                    unsigned int totalLines = (unsigned int)m_consoleQueue.size();
                    
                    if (totalLines > maxLinesOnScreen) {
                        unsigned int maxStartLine = totalLines - maxLinesOnScreen;
                        if (m_consoleStartLine < maxStartLine) {
                            m_consoleStartLine++;
                        }
                    }
                }
            }
            break;
        }
        case PB_STARTMENU: {
            // If either left button is pressed, subtract 1 from m_currentMenuItem
            if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
                if (inputMessage.inputId == IDI_RPIOP27_LFLIP) {
                    // Get the current menu item count from g_mainMenu
                    if (m_CurrentMenuItem > 0) {
                        m_CurrentMenuItem--;
                        g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDSWORDCUT);
                    }
                }
                // If either right button is pressed, add 1 to m_currentMenuItem
                if (inputMessage.inputId == IDI_RPIOP17_RFLIP) {
                    int temp = g_mainMenu.size();
                    if (m_CurrentMenuItem < (temp -1)) {
                        m_CurrentMenuItem++;
                        g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDSWORDCUT);
                    }
                }

                if ((inputMessage.inputId == IDI_RPIOP22_RACTIVATE) || (inputMessage.inputId == IDI_RPIOP05_LACTIVATE)) {
                    // Do something based on the menu item
                    switch (m_CurrentMenuItem) {
                        case (0):  if (m_PassSelfTest) m_mainState = PB_PLAYGAME; break;
                        case (1):  m_mainState = PB_SETTINGS; m_RestartSettings = true; break;
                        case (2):  m_mainState = PB_DIAGNOSTICS; m_RestartDiagnostics = true; break;
                        case (3):  m_mainState = PB_CREDITS; m_RestartCredits = true; break;
                        #if ENABLE_TEST_SANDBOX == 1
                        case (4):  
                            m_mainState = PB_TESTSANDBOX; 
                            m_RestartTestSandbox = true; 
                            // Pause background music when entering sandbox
                            m_soundSystem.pbsPauseMusic();
                            break;
                        #elif ENABLE_TEST_SANDBOX == 2
                        case (4):
                            m_mainState = PB_SIMPLEFLIPMODE;
                            m_RestartSimpleFlipMode = true;
                            // Don't pause music for simple flip mode
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
                if (inputMessage.inputId == IDI_RPIOP27_LFLIP) {
                    // Get the current menu item count from g_mainMenu
                    if (m_CurrentDiagnosticsItem > 0) {
                        m_CurrentDiagnosticsItem--;
                        g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDSWORDCUT);
                    }
                }
                // If either right button is pressed, add 1 to m_currentMenuItem
                if (inputMessage.inputId == IDI_RPIOP17_RFLIP) {
                    int temp = g_diagnosticsMenu.size();
                    if (m_CurrentDiagnosticsItem < (temp -1)) {
                        m_CurrentDiagnosticsItem++;
                        g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDSWORDCUT);
                    }
                }
            }

            if (((inputMessage.inputId == IDI_RPIOP22_RACTIVATE) || (inputMessage.inputId == IDI_RPIOP05_LACTIVATE)) && inputMessage.inputState == PB_ON){
                switch (m_CurrentDiagnosticsItem) {
                    case (0): m_mainState = PB_TESTMODE; m_RestartTestMode = true; m_EnableOverlay = false; break;
                    case (1): m_mainState = PB_BENCHMARK; m_RestartBenchmark = true; break;
                    case (2): if ((inputMessage.inputId == IDI_RPIOP22_RACTIVATE) || (inputMessage.inputId == IDI_RPIOP05_LACTIVATE)) {
                        if (m_EnableOverlay) m_EnableOverlay = false;
                        else m_EnableOverlay = true;
                        g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDCLICK);
                    }
                    break;
                    case (3): if ((inputMessage.inputId == IDI_RPIOP22_RACTIVATE) || (inputMessage.inputId == IDI_RPIOP05_LACTIVATE)) {
                        if (m_ShowFPS) m_ShowFPS = false;
                        else m_ShowFPS = true;
                        g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDCLICK);
                    }
                    break;
                    case (4): if ((inputMessage.inputId == IDI_RPIOP22_RACTIVATE) || (inputMessage.inputId == IDI_RPIOP05_LACTIVATE)) {
                        m_mainState = PB_BOOTUP;
                        m_RestartBootUp = true;
                        g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDCLICK);
                    }
                    break;
                    default: break;
                }
            }

            if (inputMessage.inputId == IDI_RPIOP06_START) {
                // Save the values to the settings file and exit the screen
                m_mainState = PB_STARTMENU;
                m_RestartMenu = true;
            }

            break;
        }

        case PB_TESTMODE: {
            
            // Record state of flipper and activation buttons
            if (inputMessage.inputId == IDI_RPIOP27_LFLIP) {
                if (inputMessage.inputState == PB_ON) m_LFON = true;
                else m_LFON = false;
            }
            if (inputMessage.inputId == IDI_RPIOP17_RFLIP) {
                if (inputMessage.inputState == PB_ON) m_RFON = true;
                else m_RFON = false;
            }
            if (inputMessage.inputId == IDI_RPIOP05_LACTIVATE) {
                if (inputMessage.inputState == PB_ON) m_LAON = true;
                else m_LAON = false;
            }
            if (inputMessage.inputId == IDI_RPIOP22_RACTIVATE) {
                if (inputMessage.inputState == PB_ON) m_RAON = true;
                else m_RAON = false;
            }

            // Check the mode and send output message if needed
            if (m_TestMode == PB_TESTOUTPUT) {
                // Send the output message to the output queue - this will be connected to HW
                if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON && (!m_LAON && !m_RAON)) {
                    if (inputMessage.inputId == IDI_RPIOP27_LFLIP) {
                        if (m_CurrentOutputItem > 0) m_CurrentOutputItem--;
                    }
                    // If either right button is pressed, add 1 to m_currentMenuItem
                    if (inputMessage.inputId == IDI_RPIOP17_RFLIP) {
                        if (m_CurrentOutputItem < (NUM_OUTPUTS -1)) m_CurrentOutputItem++;
                    }
                }

                if ((inputMessage.inputId == IDI_RPIOP22_RACTIVATE) || (inputMessage.inputId == IDI_RPIOP05_LACTIVATE)) {
                    // Handle NeoPixel outputs: press = white, release = black
                    if (g_outputDef[m_CurrentOutputItem].boardType == PB_NEOPIXEL) {
                        if (inputMessage.inputState == PB_ON) {
                            SendNeoPixelAllMsg(m_CurrentOutputItem, 255, 255, 255, 255);
                            g_outputDef[m_CurrentOutputItem].lastState = PB_ON;
                        } else {
                            SendNeoPixelAllMsg(m_CurrentOutputItem, 0, 0, 0, 255);
                            g_outputDef[m_CurrentOutputItem].lastState = PB_OFF;
                        }
                    } else if (inputMessage.inputState == PB_ON) {
                        // Regular outputs - toggle state and send message on press
                        if (g_outputDef[m_CurrentOutputItem].lastState == PB_ON) g_outputDef[m_CurrentOutputItem].lastState = PB_OFF;
                        else g_outputDef[m_CurrentOutputItem].lastState = PB_ON;

                        // Send the message to the output queue using SendOutputMsg function
                        SendOutputMsg(g_outputDef[m_CurrentOutputItem].outputMsg, 
                                    m_CurrentOutputItem,  // Use array index as ID
                                    g_outputDef[m_CurrentOutputItem].lastState, 
                                    false);
                    }
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
                if (inputMessage.inputId == IDI_RPIOP27_LFLIP) {
                    if (m_CurrentSettingsItem > 0) {
                        m_CurrentSettingsItem--;
                        g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDSWORDCUT);
                    }
                }
                // If either right button is pressed, add 1 to m_currentMenuItem
                if (inputMessage.inputId == IDI_RPIOP17_RFLIP) {
                    int temp = g_settingsMenu.size();
                    if (m_CurrentSettingsItem < (temp -1)) {
                        m_CurrentSettingsItem++;
                        g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDSWORDCUT);
                    }
                }
                if (inputMessage.inputId == IDI_RPIOP06_START) {
                    // Save the values to the settings file and exit the screen
                    pbeSaveFile();
                    m_mainState = PB_STARTMENU;
                    m_RestartMenu = true;
                }
            }

            if (((inputMessage.inputId == IDI_RPIOP22_RACTIVATE) || (inputMessage.inputId == IDI_RPIOP05_LACTIVATE)) && inputMessage.inputState == PB_ON){
                switch (m_CurrentSettingsItem) {
                    case (0): {
                        if (inputMessage.inputId == IDI_RPIOP22_RACTIVATE) {
                            if (m_saveFileData.mainVolume < 10) {
                                m_saveFileData.mainVolume++;
                                m_ampDriver.SetVolume(m_saveFileData.mainVolume * 10);  // Convert 0-10 to 0-100%
                                g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDCLICK);
                            }
                        }
                        if (inputMessage.inputId == IDI_RPIOP05_LACTIVATE) {
                            if (m_saveFileData.mainVolume > 0) {
                                m_saveFileData.mainVolume--;
                                m_ampDriver.SetVolume(m_saveFileData.mainVolume * 10);  // Convert 0-10 to 0-100%
                                g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDCLICK);
                            }
                        }
                        break;
                    }
                    case (1): {
                        if (inputMessage.inputId == IDI_RPIOP22_RACTIVATE) {
                            if (m_saveFileData.musicVolume < 10) m_saveFileData.musicVolume++;
                            g_PBEngine.m_soundSystem.pbsSetMusicVolume(m_saveFileData.musicVolume * 10);
                            g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDCLICK);
                        }
                        if (inputMessage.inputId == IDI_RPIOP05_LACTIVATE) {
                            if (m_saveFileData.musicVolume > 0) m_saveFileData.musicVolume--;
                            g_PBEngine.m_soundSystem.pbsSetMusicVolume(m_saveFileData.musicVolume * 10);
                            g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDCLICK);
                        }
                        break;
                    }
                    case (2): {
                        if (inputMessage.inputId == IDI_RPIOP22_RACTIVATE) {
                            if (m_saveFileData.ballsPerGame < 9) {
                                m_saveFileData.ballsPerGame++;
                                g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDCLICK);
                            }
                        }
                        if (inputMessage.inputId == IDI_RPIOP05_LACTIVATE) {
                            if (m_saveFileData.ballsPerGame > 1) {
                                m_saveFileData.ballsPerGame--;
                                g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDCLICK);
                            }
                        }
                        break;
                    }
                    case (3): {
                        if (inputMessage.inputId == IDI_RPIOP22_RACTIVATE) {
                            switch (m_saveFileData.difficulty) {
                                case PB_EASY: m_saveFileData.difficulty = PB_NORMAL; g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDCLICK);break;
                                case PB_NORMAL: m_saveFileData.difficulty = PB_HARD; g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDCLICK);break;
                                case PB_HARD: m_saveFileData.difficulty = PB_EPIC; g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDCLICK);break;
                                case PB_EPIC: m_saveFileData.difficulty = PB_EPIC; break;
                            }
                        }
                        if (inputMessage.inputId == IDI_RPIOP05_LACTIVATE) {
                            switch (m_saveFileData.difficulty) {
                                case PB_EASY: m_saveFileData.difficulty = PB_EASY; break;
                                case PB_NORMAL: m_saveFileData.difficulty = PB_EASY; g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDCLICK);break;
                                case PB_HARD: m_saveFileData.difficulty = PB_NORMAL; g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDCLICK);break;
                                case PB_EPIC: m_saveFileData.difficulty = PB_HARD; g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDCLICK);break;
                            }
                        }
                        break;
                    }
                    case (4): {
                        if ((inputMessage.inputId == IDI_RPIOP22_RACTIVATE) || (inputMessage.inputId == IDI_RPIOP05_LACTIVATE)) {
                            resetHighScores();
                            g_PBEngine.m_soundSystem.pbsPlayEffect(SOUNDCLICK);
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
            // Track button states for display (only for button messages)
            if (inputMessage.inputMsg == PB_IMSG_BUTTON) {
                if (inputMessage.inputId == IDI_RPIOP27_LFLIP) {
                    if (inputMessage.inputState == PB_ON) m_LFON = true;
                    else m_LFON = false;
                }
                if (inputMessage.inputId == IDI_RPIOP17_RFLIP) {
                    if (inputMessage.inputState == PB_ON) m_RFON = true;
                    else m_RFON = false;
                }
                if (inputMessage.inputId == IDI_RPIOP05_LACTIVATE) {
                    if (inputMessage.inputState == PB_ON) m_LAON = true;
                    else m_LAON = false;
                }
                if (inputMessage.inputId == IDI_RPIOP22_RACTIVATE) {
                    if (inputMessage.inputState == PB_ON) m_RAON = true;
                    else m_RAON = false;
                }
            }
            
            // Handle NeoPixel animation timer (only active in timer mode)
            if (inputMessage.inputMsg == PB_IMSG_TIMER && 
                inputMessage.inputId == SANDBOX_NEOPIXEL_TIMER_ID &&
                m_sandboxNeoPixelAnimActive &&
                !m_sandboxNeoPixelStepMode) {  // Only process in timer mode
                
                // Call the appropriate animation function based on mode
                switch(m_sandboxNeoPixelAnimMode) {
                    case 0:
                        // Original step animation
                        sandboxNeoPixelStep();
                        break;
                    case 1:
                        // Gradual fade: Red to Blue over 3 seconds, bidirectional loop
                        neoPixelGradualFade(255, 0, 0,  // Start: Red
                                          0, 0, 255,    // End: Blue
                                          IDO_RPIOP10_NEOPIXEL0, 
                                          100,          // Update every 50ms
                                          3000,         // 3 second duration
                                          false,        // Don't restart (use loop instead)
                                          true);        // Loop bidirectionally
                        break;
                    case 2:
                        // Sweep from ends: Green to Purple over 4 seconds, bidirectional loop
                        neoPixelSweepFromEnds(0, 255, 0,  // Start: Green
                                            128, 0, 128,  // End: Purple
                                            IDO_RPIOP10_NEOPIXEL0,
                                            100,          // Update every 100ms
                                            4000,         // 4 second duration
                                            false,        // Don't restart (use loop instead)
                                            true);        // Loop bidirectionally
                        break;
                    case 3:
                        // Toggle: Cyan, Yellow, Green, Red rotating pattern
                        neoPixelToggle(0, 255, 255,    // Color 1: Cyan
                                      255, 255, 0,     // Color 2: Yellow
                                      0, 255, 0,       // Color 3: Green
                                      255, 0, 0,       // Color 4: Red
                                      0x0F,            // Mask: all 4 colors active (0b1111)
                                      IDO_RPIOP10_NEOPIXEL0,
                                      200,             // Shift every 200ms
                                      0,               // Duration ignored when looping
                                      true);           // Loop continuously
                        break;
                    case 4:
                        // Split Toggle: Purple and Orange, swapping halves
                        neoPixelSplitToggle(128, 0, 128,  // Start: Purple
                                           255, 165, 0,   // End: Orange
                                           IDO_RPIOP10_NEOPIXEL0,
                                           400,           // Toggle every 400ms
                                           0,             // Duration ignored when looping
                                           true);         // Loop continuously
                        break;
                    case 5:
                        // Strobe: White light with 2000ms off time, accelerating over 20 seconds
                        neoPixelStrobe(255, 255, 255,  // White color
                                      IDO_RPIOP10_NEOPIXEL0,
                                      2000,            // 2000ms off time (5ms on is fixed)
                                      20000,           // 20 second overall duration with acceleration
                                      true);           // Loop continuously
                        break;
                    case 6:
                        // Snake: Blue base with red snake, length 10, wrap mode enabled, 100ms step time
                        neoPixelSnake(0, 0, 64,        // Base: Dark Blue
                                     255, 0, 0,        // Snake: Red
                                     10,               // Snake length: 10 pixels
                                     true,             // Wrap mode: enabled
                                     IDO_RPIOP10_NEOPIXEL0,
                                     100);             // Step every 100ms
                        break;
                }
                
                // Set next timer
                pbeSetTimer(SANDBOX_NEOPIXEL_TIMER_ID, SANDBOX_NEOPIXEL_TIMER_INTERVAL_MS);
            }
            
            if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
                
                // Start button exits to Start Menu
                if (inputMessage.inputId == IDI_RPIOP06_START) {
                    // Clean up video player before exiting
                    if (m_sandboxVideoPlayer) {
                        m_sandboxVideoPlayer->pbvpStop();
                        m_sandboxVideoPlayer->pbvpUnloadVideo();
                        delete m_sandboxVideoPlayer;
                        m_sandboxVideoPlayer = nullptr;
                        m_sandboxVideoSpriteId = NOSPRITE;
                        m_sandboxVideoLoaded = false;
                    }
                    
                    // Clean up sandbox ejector device before exiting
                    if (m_sandboxEjector) {
                        m_sandboxEjector->pbdEnable(false);
                        m_sandboxEjector->pbdInit();
                        m_sandboxEjector = nullptr;  // Will be deleted by pbeClearDevices
                    }
                    
                    // Clean up NeoPixel animation state and timer
                    if (m_sandboxNeoPixelAnimActive) {
                        pbeTimerStop(SANDBOX_NEOPIXEL_TIMER_ID);
                        m_sandboxNeoPixelAnimActive = false;
                    }
                    
                    // Clear all devices when exiting sandbox
                    pbeClearDevices();
                    
                    // Resume background music when exiting sandbox
                    m_soundSystem.pbsResumeMusic();
                    
                    m_mainState = PB_STARTMENU;
                    m_RestartMenu = true;
                }
                
                // Left Flipper - Test 1
                if (inputMessage.inputId == IDI_RPIOP27_LFLIP) {
                    static int testCount = 0;

                    if (testCount % 3 == 0) {  
                        // Sending Test Messages
                        SendOutputMsg(PB_OMSG_GENERIC_IO, IDO_IOD0P08_LSLING, PB_ON, true);
                        SendOutputMsg(PB_OMSG_GENERIC_IO, IDO_IO1P08_POP, PB_ON, true);
                        SendOutputMsg(PB_OMSG_GENERIC_IO, IDO_IO2P08_EJECT, PB_ON, true);

                        // Note: RGB LED test code commented out - these IDs refer to non-existent hardware in current config
                        // SendRGBMsg(IDO_LED2, IDO_LED3, IDO_LED4, PB_LEDCYAN, PB_ON, false);
                        // SendRGBMsg(IDO_LED5, IDO_LED6, IDO_LED7, PB_LEDYELLOW, PB_ON, false);
                        // SendRGBMsg(IDO_LED8, IDO_LED9, IDO_LED10, PB_LEDPURPLE, PB_ON, false);
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
                
                // Reset Button - Cycle through NeoPixel animation modes
                if (inputMessage.inputId == IDI_RPIOP24_RESET) {
                    // Stop current animation if active
                    if (m_sandboxNeoPixelAnimActive) {
                        SendNeoPixelAllMsg(IDO_RPIOP10_NEOPIXEL0, PB_LEDBLUE, 32);
                        m_sandboxNeoPixelAnimActive = false;
                        pbeTimerStop(SANDBOX_NEOPIXEL_TIMER_ID);
                    }
                    
                    // Cycle to next mode
                    m_sandboxNeoPixelAnimMode = (m_sandboxNeoPixelAnimMode + 1) % 7;
                }
                
                // Right Flipper - NeoPixel Animation Test
                if (inputMessage.inputId == IDI_RPIOP17_RFLIP) {
                    if (!m_sandboxNeoPixelAnimActive) {
                        // Start animation
                        m_sandboxNeoPixelAnimActive = true;
                        
                        // Initialize based on mode
                        if (m_sandboxNeoPixelAnimMode == 0) {
                            // Original step animation needs initialization
                            SendNeoPixelAllMsg(IDO_RPIOP10_NEOPIXEL0, PB_LEDBLUE, 32);  // Dark blue (brightness 32)
                            m_sandboxNeoPixelPosition = -1;  // Start at position -1
                            m_sandboxNeoPixelMovingUp = true;
                            
                            // Get the number of LEDs in the NeoPixel chain
                            if (m_NeoPixelDriverMap.find(0) != m_NeoPixelDriverMap.end()) {
                                unsigned int numLEDs = m_NeoPixelDriverMap.at(0).GetNumLEDs();
                                m_sandboxNeoPixelMaxPosition = (numLEDs >= 1) ? (numLEDs - 1) : 0;
                            } else {
                                m_sandboxNeoPixelMaxPosition = 0;
                            }
                        }
                        // New animation modes handle their own initialization
                        
                        // In step mode, do first step. In timer mode, start timer.
                        if (m_sandboxNeoPixelStepMode) {
                            // Step mode: perform first animation step immediately
                            switch(m_sandboxNeoPixelAnimMode) {
                                case 0: sandboxNeoPixelStep(); break;
                                case 1: 
                                    neoPixelGradualFade(255, 0, 0, 0, 0, 255, IDO_RPIOP10_NEOPIXEL0, 50, 3000, true);
                                    break;
                                case 2:
                                    neoPixelSweepFromEnds(0, 255, 0, 128, 0, 128, IDO_RPIOP10_NEOPIXEL0, 50, 4000, true);
                                    break;
                                case 5:
                                    neoPixelStrobe(255, 255, 255, IDO_RPIOP10_NEOPIXEL0, 2000, 0, true, true);
                                    break;
                                case 6:
                                    neoPixelSnake(0, 0, 64, 255, 0, 0, 10, true, IDO_RPIOP10_NEOPIXEL0, 100);
                                    break;
                            }
                        } else {
                            // Timer mode: start timer for animation
                            // For strobe mode, reset state when starting
                            if (m_sandboxNeoPixelAnimMode == 5) {
                                neoPixelStrobe(255, 255, 255, IDO_RPIOP10_NEOPIXEL0, 2000, 20000, true, true);
                            }
                            pbeSetTimer(SANDBOX_NEOPIXEL_TIMER_ID, SANDBOX_NEOPIXEL_TIMER_INTERVAL_MS);
                        }
                    } else {
                        // Animation already active
                        if (m_sandboxNeoPixelStepMode) {
                            // Step mode: advance one step
                            switch(m_sandboxNeoPixelAnimMode) {
                                case 0: sandboxNeoPixelStep(); break;
                                case 1:
                                    neoPixelGradualFade(255, 0, 0, 0, 0, 255, IDO_RPIOP10_NEOPIXEL0, 50, 3000, true);
                                    break;
                                case 2:
                                    neoPixelSweepFromEnds(0, 255, 0, 128, 0, 128, IDO_RPIOP10_NEOPIXEL0, 50, 4000, true);
                                    break;
                                case 5:
                                    neoPixelStrobe(255, 255, 255, IDO_RPIOP10_NEOPIXEL0, 95, 0, true);
                                    break;
                                case 6:
                                    neoPixelSnake(0, 0, 64, 255, 0, 0, 10, true, IDO_RPIOP10_NEOPIXEL0, 100);
                                    break;
                            }
                        } else {
                            // Timer mode: toggle off (stop animation)
                            SendNeoPixelAllMsg(IDO_RPIOP10_NEOPIXEL0, PB_LEDBLUE, 32);
                            m_sandboxNeoPixelAnimActive = false;
                            pbeTimerStop(SANDBOX_NEOPIXEL_TIMER_ID);
                        }
                    }
                }
                
                // Left Activate - Test 3 - Video Playback Test (Toggle fade in/out)
                if (inputMessage.inputId == IDI_RPIOP05_LACTIVATE) {
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
                
                // Right Activate - Test 4 - Ejector Device Test
                if (inputMessage.inputId == IDI_RPIOP22_RACTIVATE) {
                    if (m_sandboxEjector) {
                        if (m_sandboxEjector->pdbIsRunning()) {
                            // If running, stop and reset
                            m_sandboxEjector->pbdEnable(false);
                            m_sandboxEjector->pbdInit();
                        } else {
                            // If not running, start the ejector
                            m_sandboxEjector->pdbStartRun();
                        }
                    }
                }
            }
        break;
        }
        
        case PB_SIMPLEFLIPMODE: {
            // Simple Flip Mode - Just handle Start button to exit
            if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
                if (inputMessage.inputId == IDI_RPIOP06_START) {
                    // Disable auto outputs
                    SetAutoOutputEnable(false);
                    
                    // Restore overlay state to what it was before entering
                    if (!m_OverlayWasOnBeforeSimpleFlip) {
                        m_EnableOverlay = false;
                    }
                    
                    // Return to main menu
                    m_mainState = PB_STARTMENU;
                    m_RestartMenu = true;
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
    // Validation checks for input/output definitions
    // These checks verify the configuration is valid after initialization
    g_PBEngine.pbeSendConsole("RasPin: Validating I/O configuration");
    g_PBEngine.pbeSendConsole("RasPin: Total Inputs: " + std::to_string(NUM_INPUTS));
    g_PBEngine.pbeSendConsole("RasPin: Total Outputs: " + std::to_string(NUM_OUTPUTS));
    
    // Check for duplicate board/pin assignments in inputs
    for (int i = 0; i < NUM_INPUTS; i++) {
        for (int j = i + 1; j < NUM_INPUTS; j++) {
            if (g_inputDef[i].boardType == g_inputDef[j].boardType && 
                g_inputDef[i].boardIndex == g_inputDef[j].boardIndex && 
                g_inputDef[i].pin == g_inputDef[j].pin) {
                std::string boardName = (g_inputDef[i].boardType == PB_RASPI) ? "RASPI" : 
                                       (g_inputDef[i].boardType == PB_IO) ? "IO" : "UNKNOWN";
                g_PBEngine.pbeSendConsole("RasPin: ERROR: Duplicate input pin assignment - " + 
                    g_inputDef[i].inputName + " and " + g_inputDef[j].inputName + 
                    " both use " + boardName + " board " + std::to_string(g_inputDef[i].boardIndex) + 
                    " pin " + std::to_string(g_inputDef[i].pin));
                g_PBEngine.m_PassSelfTest = false;
            }
        }
    }
    
    // Check for invalid board type configurations in inputs
    for (int i = 0; i < NUM_INPUTS; i++) {
        // LED boards cannot be inputs
        if (g_inputDef[i].boardType == PB_LED) {
            g_PBEngine.pbeSendConsole("RasPin: ERROR: Input " + g_inputDef[i].inputName + 
                " (index " + std::to_string(i) + ") is configured as PB_LED - LED boards are output-only!");
            g_PBEngine.m_PassSelfTest = false;
        }
        // NeoPixel boards cannot be inputs
        if (g_inputDef[i].boardType == PB_NEOPIXEL) {
            g_PBEngine.pbeSendConsole("RasPin: ERROR: Input " + g_inputDef[i].inputName + 
                " (index " + std::to_string(i) + ") is configured as PB_NEOPIXEL - NeoPixels are output-only!");
            g_PBEngine.m_PassSelfTest = false;
        }
    }

    // Check for duplicate board/pin assignments in outputs
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        for (int j = i + 1; j < NUM_OUTPUTS; j++) {
            if (g_outputDef[i].boardType == g_outputDef[j].boardType && 
                g_outputDef[i].boardIndex == g_outputDef[j].boardIndex && 
                g_outputDef[i].pin == g_outputDef[j].pin) {
                std::string boardName = (g_outputDef[i].boardType == PB_RASPI) ? "RASPI" : 
                                       (g_outputDef[i].boardType == PB_IO) ? "IO" :
                                       (g_outputDef[i].boardType == PB_LED) ? "LED" :
                                       (g_outputDef[i].boardType == PB_NEOPIXEL) ? "NEOPIXEL" : "UNKNOWN";
                g_PBEngine.pbeSendConsole("RasPin: ERROR: Duplicate output pin assignment - " + 
                    g_outputDef[i].outputName + " and " + g_outputDef[j].outputName + 
                    " both use " + boardName + " board " + std::to_string(g_outputDef[i].boardIndex) + 
                    " pin " + std::to_string(g_outputDef[i].pin));
                g_PBEngine.m_PassSelfTest = false;
            }
        }
    }
    
    // Check for conflicting pin usage between inputs and outputs on same board/pin
    for (int i = 0; i < NUM_INPUTS; i++) {
        for (int j = 0; j < NUM_OUTPUTS; j++) {
            // Only check boards that support both input and output (RASPI and IO)
            if ((g_inputDef[i].boardType == PB_RASPI || g_inputDef[i].boardType == PB_IO) &&
                g_inputDef[i].boardType == g_outputDef[j].boardType &&
                g_inputDef[i].boardIndex == g_outputDef[j].boardIndex &&
                g_inputDef[i].pin == g_outputDef[j].pin) {
                std::string boardName = (g_inputDef[i].boardType == PB_RASPI) ? "RASPI" : "IO";
                g_PBEngine.pbeSendConsole("RasPin: ERROR: Pin conflict - Input " + g_inputDef[i].inputName + 
                    " and output " + g_outputDef[j].outputName + " both use " + boardName + 
                    " board " + std::to_string(g_inputDef[i].boardIndex) + 
                    " pin " + std::to_string(g_inputDef[i].pin) + 
                    " - a pin cannot be both input and output!");
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
                g_PBEngine.m_inputPiMap.emplace(i, debounceInput);  // Use array index as ID
            #endif
        }
        else if (g_inputDef[i].boardType == PB_IO) {
            // Configure the pin as input on the appropriate IO chip
            if (g_inputDef[i].boardIndex < NUM_IO_CHIPS) {
                g_PBEngine.m_IOChip[g_inputDef[i].boardIndex].ConfigurePin(g_inputDef[i].pin, PB_INPUT);
                g_PBEngine.m_IOChip[g_inputDef[i].boardIndex].SetPinDebounceTime(g_inputDef[i].pin, g_inputDef[i].debounceTimeMS);
            } else {
                g_PBEngine.pbeSendConsole("RasPin: ERROR: Input " + g_inputDef[i].inputName + 
                    " references IO board " + std::to_string(g_inputDef[i].boardIndex) + 
                    " which exceeds NUM_IO_CHIPS (" + std::to_string(NUM_IO_CHIPS) + ")");
                g_PBEngine.m_PassSelfTest = false;
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
            } else {
                g_PBEngine.pbeSendConsole("RasPin: ERROR: Output " + g_outputDef[i].outputName + 
                    " references IO board " + std::to_string(g_outputDef[i].boardIndex) + 
                    " which exceeds NUM_IO_CHIPS (" + std::to_string(NUM_IO_CHIPS) + ")");
                g_PBEngine.m_PassSelfTest = false;
            }
        }
        else if (g_outputDef[i].boardType == PB_LED) {
            // Configure the LED on the appropriate LED chip
            if (g_outputDef[i].boardIndex < NUM_LED_CHIPS) {
                if (g_outputDef[i].lastState == PB_ON)
                    g_PBEngine.m_LEDChip[g_outputDef[i].boardIndex].StageLEDControl(true, g_outputDef[i].pin, LEDOn);  // Initialize to ON
                else
                g_PBEngine.m_LEDChip[g_outputDef[i].boardIndex].StageLEDControl(false, g_outputDef[i].pin, LEDOff);  // Initialize to OFF
            } else {
                g_PBEngine.pbeSendConsole("RasPin: ERROR: Output " + g_outputDef[i].outputName + 
                    " references LED board " + std::to_string(g_outputDef[i].boardIndex) + 
                    " which exceeds NUM_LED_CHIPS (" + std::to_string(NUM_LED_CHIPS) + ")");
                g_PBEngine.m_PassSelfTest = false;
            }
        }
        else if (g_outputDef[i].boardType == PB_NEOPIXEL) {
            // Initialize NeoPixel driver if not already created for this boardIndex
            int boardIndex = g_outputDef[i].boardIndex;
            if (g_PBEngine.m_NeoPixelDriverMap.find(boardIndex) == g_PBEngine.m_NeoPixelDriverMap.end()) {
                // Create NeoPixel driver with index and SPI buffer - it will look up pin and LED count automatically
                g_PBEngine.m_NeoPixelDriverMap.emplace(std::piecewise_construct,
                    std::forward_as_tuple(boardIndex),
                    std::forward_as_tuple(boardIndex, g_NeoPixelSPIBufferArray[boardIndex]));
                
                #ifdef EXE_MODE_RASPI
                // Initialize GPIO for this NeoPixel driver
                g_PBEngine.m_NeoPixelDriverMap.at(boardIndex).InitializeGPIO();
                // Stage initial black (off) state for all LEDs
                g_PBEngine.m_NeoPixelDriverMap.at(boardIndex).StageNeoPixelAll(0, 0, 0);  
                #endif
            }
        }
    }

    // Send all staged changes to IO and LED chips
    g_PBEngine.pbeSendConsole("RasPin: Sending programmed outputs to pins (LED and IO)");

    #ifdef EXE_MODE_RASPI
        SendAllStagedIO();
        SendAllStagedLED();
        SendAllStagedNeoPixels();
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
    
    // Copy options data if provided, otherwise mark as not having options
    if (options != nullptr) {
        outputMessage.optionsCopy = *options;  // Copy the data
        outputMessage.options = &outputMessage.optionsCopy;  // Point to our copy
        outputMessage.hasOptions = true;
    } else {
        outputMessage.options = nullptr;
        outputMessage.hasOptions = false;
    }
    
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

// Send NeoPixel message to set all LEDs in a chain to specific RGB values
void PBEngine::SendNeoPixelAllMsg(unsigned int neoPixelId, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness)
{
    // Set up NeoPixel options with RGB values and brightness
    stOutputOptions neoOptions;
    neoOptions.neoPixelRed = red;
    neoOptions.neoPixelGreen = green;
    neoOptions.neoPixelBlue = blue;
    neoOptions.brightness = brightness;
    neoOptions.neoPixelIndex = ALLNEOPIXELS;  // ALLNEOPIXELS means all pixels
    
    // Send the message - the processing will set all LEDs to these RGB values scaled by brightness
    // NeoPixels are always "on" when a message is sent - use black (0,0,0) to turn off
    SendOutputMsg(PB_OMSG_NEOPIXEL, neoPixelId, PB_ON, false, &neoOptions);
}

// Helper function to convert PBLEDColor enum to RGB values
void PBEngine::ConvertColorToRGB(PBLEDColor color, uint8_t& red, uint8_t& green, uint8_t& blue)
{
    // Initialize to 0 (off/black)
    red = green = blue = 0;
    
    // Map color enum to full brightness RGB (0 or 255)
    // Note: Use a color enum with all zeros (or call the RGB version with 0,0,0) to turn off
    switch (color) {
        case PB_LEDRED:     red = 255; break;
        case PB_LEDGREEN:   green = 255; break;
        case PB_LEDBLUE:    blue = 255; break;
        case PB_LEDWHITE:   red = green = blue = 255; break;
        case PB_LEDPURPLE:  red = blue = 255; break;
        case PB_LEDYELLOW:  red = green = 255; break;
        case PB_LEDCYAN:    green = blue = 255; break;
        default:            break; // Unknown color, all stay at 0 (off/black)
    }
}

// Send NeoPixel message to set all LEDs in a chain to a specific color (enum)
void PBEngine::SendNeoPixelAllMsg(unsigned int neoPixelId, PBLEDColor color, uint8_t brightness)
{
    // Convert color enum to RGB values
    uint8_t red = 0, green = 0, blue = 0;
    ConvertColorToRGB(color, red, green, blue);
    
    // Call the RGB version with brightness
    SendNeoPixelAllMsg(neoPixelId, red, green, blue, brightness);
}

// Send NeoPixel message to set a single LED in a chain to specific RGB values
void PBEngine::SendNeoPixelSingleMsg(unsigned int neoPixelId, unsigned int pixelIndex, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness)
{
    // Set up NeoPixel options with RGB values, brightness, and pixel index
    stOutputOptions neoOptions;
    neoOptions.neoPixelRed = red;
    neoOptions.neoPixelGreen = green;
    neoOptions.neoPixelBlue = blue;
    neoOptions.brightness = brightness;
    neoOptions.neoPixelIndex = pixelIndex;  // Use 0-based index directly (9999 means all pixels)
    
    // Send the message - the processing will set the specific LED to these RGB values scaled by brightness
    SendOutputMsg(PB_OMSG_NEOPIXEL, neoPixelId, PB_ON, false, &neoOptions);
}

// Send NeoPixel message to set a single LED in a chain to a specific color (enum)
void PBEngine::SendNeoPixelSingleMsg(unsigned int neoPixelId, unsigned int pixelIndex, PBLEDColor color, uint8_t brightness)
{
    // Convert color enum to RGB values
    uint8_t red = 0, green = 0, blue = 0;
    ConvertColorToRGB(color, red, green, blue);
    
    // Call the RGB version with brightness
    SendNeoPixelSingleMsg(neoPixelId, pixelIndex, red, green, blue, brightness);
}

// Function to set or unset autoOutput for an input by ID
bool PBEngine::SetAutoOutput(unsigned int id, bool autoOutputEnabled)
{
    // id is the array index - no bounds check needed, arrays are pre-validated
    g_inputDef[id].autoOutput = autoOutputEnabled;
    return true;
}
//==============================================================================
// Device Management Functions
//==============================================================================

// Add a device to the device vector
void PBEngine::pbeAddDevice(PBDevice* device) {
    if (device != nullptr) {
        m_devices.push_back(device);
    }
}

// Clear all devices and free memory
void PBEngine::pbeClearDevices() {
    for (auto device : m_devices) {
        if (device != nullptr) {
            delete device;
        }
    }
    m_devices.clear();
}

// Execute all registered devices
void PBEngine::pbeExecuteDevices() {
    for (auto device : m_devices) {
        if (device != nullptr) {
            device->pbdExecute();
        }
    }
}

//==============================================================================
// Timer Functions
//==============================================================================

// Set the watchdog timer with a duration in milliseconds
// When the timer expires, an input message with PB_IMSG_TIMER will be sent
// with the inputId set to WATCHDOGTIMER_ID (0)
// This is a dedicated timer that doesn't use the timer queue
bool PBEngine::pbeSetWatchdogTimer(unsigned int timerValueMS) {
    unsigned long currentTick = GetTickCountGfx();
    
    m_watchdogTimer.timerId = WATCHDOGTIMER_ID;
    m_watchdogTimer.durationMS = timerValueMS;
    m_watchdogTimer.startTickMS = currentTick;
    m_watchdogTimer.expireTickMS = currentTick + timerValueMS;
    
    return true;
}

// Set a timer with a user-supplied timer ID and duration in milliseconds
// When the timer expires, an input message with PB_IMSG_TIMER will be sent
// with the inputId set to the user-supplied timerId
// Returns true if timer was added successfully, false if limit reached or timerId is 0
bool PBEngine::pbeSetTimer(unsigned int timerId, unsigned int timerValueMS, bool repeat) {
    // Timer ID 0 is reserved for the watchdog timer
    if (timerId == WATCHDOGTIMER_ID) {
        return false;
    }
    
    // Check if we've reached the maximum number of active timers
    if (m_timerQueue.size() >= MAX_TIMERS) {
        return false;
    }
    
    unsigned long currentTick = GetTickCountGfx();
    
    stTimerEntry timerEntry;
    timerEntry.timerId = timerId;
    timerEntry.durationMS = timerValueMS;
    timerEntry.startTickMS = currentTick;
    timerEntry.expireTickMS = currentTick + timerValueMS;
    timerEntry.repeat = repeat;
    
    // Note: Currently single-threaded, mutex locking not needed
    // std::lock_guard<std::mutex> lock(m_timerQMutex);
    m_timerQueue.push(timerEntry);
    return true;
}

// Process all timers in the queue, check for expired timers,
// and send input messages for any that have expired
// Note: Uses a simple queue and temporary copy pattern for simplicity.
// For typical pinball games, the number of active timers is small (< 10),
// so the O(n) processing is negligible. If many simultaneous timers are
// needed, consider using std::priority_queue ordered by expireTickMS.
void PBEngine::pbeProcessTimers() {
    unsigned long currentTick = GetTickCountGfx();
    
    // Check watchdog timer first (timerId = 0)
    if (m_watchdogTimer.durationMS > 0 && currentTick >= m_watchdogTimer.expireTickMS) {
        // Watchdog timer has expired - send an input message
        stInputMessage inputMessage;
        inputMessage.inputMsg = PB_IMSG_TIMER;
        inputMessage.inputId = WATCHDOGTIMER_ID;
        inputMessage.inputState = PB_ON;
        inputMessage.sentTick = currentTick;
        
        m_inputQueue.push(inputMessage);
        
        // Clear the watchdog timer after it fires
        m_watchdogTimer.durationMS = 0;
        m_watchdogTimer.expireTickMS = 0;
    }
    
    // Early exit if queue is empty
    if (m_timerQueue.empty()) {
        return;
    }
    
    // Create a temporary queue to hold non-expired timers
    std::queue<stTimerEntry> remainingTimers;
    
    // Note: Currently single-threaded, mutex locking not needed
    // std::lock_guard<std::mutex> lock(m_timerQMutex);
    
    // Process all timers in the queue
    while (!m_timerQueue.empty()) {
        stTimerEntry timerEntry = m_timerQueue.front();
        m_timerQueue.pop();
        
        // Check if this timer has expired
        if (currentTick >= timerEntry.expireTickMS) {
            // Timer has expired - send an input message with the timer ID
            stInputMessage inputMessage;
            inputMessage.inputMsg = PB_IMSG_TIMER;
            inputMessage.inputId = timerEntry.timerId;
            // Use PB_ON to indicate the timer has fired (consistent with other input messages)
            inputMessage.inputState = PB_ON;
            inputMessage.sentTick = currentTick;
            
            // Add the timer expiration message to the input queue
            m_inputQueue.push(inputMessage);
            
            // If this is a repeat timer, restart it
            if (timerEntry.repeat) {
                timerEntry.startTickMS = currentTick;
                timerEntry.expireTickMS = currentTick + timerEntry.durationMS;
                remainingTimers.push(timerEntry);
            }
        } else {
            // Timer has not expired - keep it in the queue
            remainingTimers.push(timerEntry);
        }
    }
    
    // Swap the remaining timers back into the main timer queue
    m_timerQueue = std::move(remainingTimers);
}

// Check if a timer with the given ID exists and is not expired
// Returns true if the timer is active (exists and not expired), false otherwise
bool PBEngine::pbeTimerActive(unsigned int timerId) {
    unsigned long currentTick = GetTickCountGfx();
    
    // Check watchdog timer first (timerId = 0)
    if (timerId == WATCHDOGTIMER_ID) {
        return (m_watchdogTimer.durationMS > 0 && currentTick < m_watchdogTimer.expireTickMS);
    }
    
    // Early exit if queue is empty
    if (m_timerQueue.empty()) {
        return false;
    }
    
    // Create a temporary queue to iterate through all timers
    std::queue<stTimerEntry> tempQueue;
    bool found = false;
    
    // Note: Currently single-threaded, mutex locking not needed
    // std::lock_guard<std::mutex> lock(m_timerQMutex);
    
    // Search through all timers in the queue
    while (!m_timerQueue.empty()) {
        stTimerEntry timerEntry = m_timerQueue.front();
        m_timerQueue.pop();
        tempQueue.push(timerEntry);
        
        // Check if this is the timer we're looking for and it hasn't expired
        if (timerEntry.timerId == timerId && currentTick < timerEntry.expireTickMS) {
            found = true;
        }
    }
    
    // Restore the timer queue
    m_timerQueue = std::move(tempQueue);
    
    return found;
}

// Remove a timer with the given ID from the queue if it exists
void PBEngine::pbeTimerStop(unsigned int timerId) {
    // Handle watchdog timer first (timerId = 0)
    if (timerId == WATCHDOGTIMER_ID) {
        m_watchdogTimer.durationMS = 0;
        m_watchdogTimer.expireTickMS = 0;
        return;
    }
    
    // Early exit if queue is empty
    if (m_timerQueue.empty()) {
        return;
    }
    
    // Create a temporary queue to hold timers that should remain
    std::queue<stTimerEntry> remainingTimers;
    
    // Note: Currently single-threaded, mutex locking not needed
    // std::lock_guard<std::mutex> lock(m_timerQMutex);
    
    // Process all timers in the queue
    while (!m_timerQueue.empty()) {
        stTimerEntry timerEntry = m_timerQueue.front();
        m_timerQueue.pop();
        
        // Keep the timer if it's not the one we're looking for
        if (timerEntry.timerId != timerId) {
            remainingTimers.push(timerEntry);
        }
    }
    
    // Swap the remaining timers back into the main timer queue
    m_timerQueue = std::move(remainingTimers);
}

// Stop all timers, and optionally stop the watchdog timer
void PBEngine::pbeTimerStopAll(bool stopWatchdog) {
    // Clear the entire timer queue
    while (!m_timerQueue.empty()) {
        m_timerQueue.pop();
    }
    
    // Stop the watchdog timer if requested
    if (stopWatchdog) {
        m_watchdogTimer.durationMS = 0;
        m_watchdogTimer.expireTickMS = 0;
    }
}

// Reload function to reset all engine screen load states
void PBEngine::pbeEngineReload() {
    m_defaultBackgroundLoaded = false;
    m_bootUpLoaded = false;
    m_startMenuLoaded = false;
    m_RestartMenu = true;
}

// Sandbox NeoPixel step function - advances the animation by one step
void PBEngine::sandboxNeoPixelStep() {
    // Get LED count for bounds checking
    unsigned int numLEDs = 0;

    if (m_NeoPixelDriverMap.find(0) != m_NeoPixelDriverMap.end()) {
        numLEDs = m_NeoPixelDriverMap.at(0).GetNumLEDs();
    }
    
    // Clear previous pixels to dark blue (0-based indexing with bounds checking)
    // When moving forward, clear pixels behind. When moving backward, clear pixels ahead.
    if (numLEDs > 0) {
        int clearPos = m_sandboxNeoPixelPosition;
        
        // Clear the 3-pixel trail
        if (clearPos - 1 >= 0 && clearPos - 1 < (int)numLEDs) {
            SendNeoPixelSingleMsg(IDO_RPIOP10_NEOPIXEL0, clearPos - 1, PB_LEDBLUE, 32);
        }
        if (clearPos >= 0 && clearPos < (int)numLEDs) {
            SendNeoPixelSingleMsg(IDO_RPIOP10_NEOPIXEL0, clearPos, PB_LEDBLUE, 32);
        }
        if (clearPos + 1 >= 0 && clearPos + 1 < (int)numLEDs) {
            SendNeoPixelSingleMsg(IDO_RPIOP10_NEOPIXEL0, clearPos + 1, PB_LEDBLUE, 32);
        }
    }

    // Update the positon for the next rendering
    if (m_sandboxNeoPixelMovingUp) {
        m_sandboxNeoPixelPosition++;
        if (m_sandboxNeoPixelPosition >= m_sandboxNeoPixelMaxPosition) {
            m_sandboxNeoPixelMovingUp = false;
            m_sandboxNeoPixelPosition = m_sandboxNeoPixelMaxPosition;  // Clamp to max
        }
    } else {
        m_sandboxNeoPixelPosition--;
        if (m_sandboxNeoPixelPosition <= 0) {
            m_sandboxNeoPixelMovingUp = true;
            m_sandboxNeoPixelPosition = 0;  // Clamp to min
        }
    }
    
    // Set current pixels (position is the 0-based LED index for the center red pixel)
    if (numLEDs > 0) {
        int idx1 = m_sandboxNeoPixelPosition - 1;  // Left purple
        int idx2 = m_sandboxNeoPixelPosition;       // Center red
        int idx3 = m_sandboxNeoPixelPosition + 1;   // Right purple
        
        if (idx1 >= 0 && idx1 < (int)numLEDs) {
            SendNeoPixelSingleMsg(IDO_RPIOP10_NEOPIXEL0, idx1, PB_LEDPURPLE, 255);  // Purple
        }
        if (idx2 >= 0 && idx2 < (int)numLEDs) {
            SendNeoPixelSingleMsg(IDO_RPIOP10_NEOPIXEL0, idx2, PB_LEDRED, 255);     // Red
        }
        if (idx3 >= 0 && idx3 < (int)numLEDs) {
            SendNeoPixelSingleMsg(IDO_RPIOP10_NEOPIXEL0, idx3, PB_LEDPURPLE, 255);  // Purple
        }
    }
    
}

// NeoPixel Gradual Fade - All pixels fade from start color to end color over time
// Parameters:
//   startR, startG, startB: Starting color (RGB 0-255)
//   endR, endG, endB: Ending color (RGB 0-255)
//   neoPixelId: Which NeoPixel output pin/chain to control
//   stepTimeMS: Minimum time in milliseconds between updates
//   totalDurationMS: Total duration for the full transition
//   restart: If true, restart the animation when complete
//   loop: If true, fade back to start color when done (bidirectional). If false, stop at end color.
void PBEngine::neoPixelGradualFade(uint8_t startR, uint8_t startG, uint8_t startB,
                                   uint8_t endR, uint8_t endG, uint8_t endB,
                                   unsigned int neoPixelId, unsigned int stepTimeMS,
                                   unsigned int totalDurationMS, bool restart, bool loop)
{
    // Structure to hold state for each neoPixelId
    struct FadeState {
        bool initialized;
        unsigned long startTimeMS;
        unsigned long lastStepTimeMS;
        bool fadingForward;  // True = start to end, False = end to start
    };
    
    // Static map to track state per neoPixelId
    static std::map<unsigned int, FadeState> stateMap;
    
    // Get current time
    unsigned long currentTimeMS = GetTickCountGfx();
    
    // Get or create state for this neoPixelId
    FadeState& state = stateMap[neoPixelId];
    
    // Initialize or reset if this is the first call
    if (!state.initialized) {
        state.initialized = true;
        state.startTimeMS = currentTimeMS;
        state.lastStepTimeMS = currentTimeMS;
        
        // Initialize all pixels to start color
        SendNeoPixelAllMsg(neoPixelId, startR, startG, startB, 255);
        return;
    }
    
    // Check if step time has elapsed
    unsigned long stepElapsedMS = currentTimeMS - state.lastStepTimeMS;
    if (stepElapsedMS < stepTimeMS) {
        return; // Not time for next step yet
    }
    
    // Update last step time
    state.lastStepTimeMS = currentTimeMS;
    
    // Calculate elapsed time since start
    unsigned long elapsedMS = currentTimeMS - state.startTimeMS;
    
    // Check if animation is complete
    if (elapsedMS >= totalDurationMS) {
        if (loop) {
            // Loop mode: reverse direction and continue
            state.fadingForward = !state.fadingForward;
            state.startTimeMS = currentTimeMS;
            elapsedMS = 0;
        } else if (restart) {
            // Restart mode: jump back to beginning
            state.startTimeMS = currentTimeMS;
            state.fadingForward = true;
            elapsedMS = 0;
        } else {
            // One-shot mode: stop at final color
            SendNeoPixelAllMsg(neoPixelId, endR, endG, endB, 255);
            return;
        }
    }
    
    // Calculate interpolation factor (0.0 to 1.0)
    float progress = (float)elapsedMS / (float)totalDurationMS;
    
    // Interpolate color values based on fade direction
    uint8_t currentR, currentG, currentB;
    if (state.fadingForward) {
        // Fading from start to end
        currentR = (uint8_t)(startR + (endR - startR) * progress);
        currentG = (uint8_t)(startG + (endG - startG) * progress);
        currentB = (uint8_t)(startB + (endB - startB) * progress);
    } else {
        // Fading from end to start
        currentR = (uint8_t)(endR + (startR - endR) * progress);
        currentG = (uint8_t)(endG + (startG - endG) * progress);
        currentB = (uint8_t)(endB + (startB - endB) * progress);
    }
    
    // Update all pixels with interpolated color
    SendNeoPixelAllMsg(neoPixelId, currentR, currentG, currentB, 255);
}

// NeoPixel Sweep From Ends - Color changes symmetrically from both ends toward middle
// with gradient effect based on distance from center
// Parameters:
//   startR, startG, startB: Starting color (RGB 0-255)
//   endR, endG, endB: Ending color (RGB 0-255)
//   neoPixelId: Which NeoPixel output pin/chain to control
//   stepTimeMS: Minimum time in milliseconds between pixel updates
//   totalDurationMS: Total duration for the full transition
//   restart: If true, restart the animation when complete
//   loop: If true, sweep back from center to ends when done (bidirectional). If false, stop at end color.
void PBEngine::neoPixelSweepFromEnds(uint8_t startR, uint8_t startG, uint8_t startB,
                                     uint8_t endR, uint8_t endG, uint8_t endB,
                                     unsigned int neoPixelId, unsigned int stepTimeMS,
                                     unsigned int totalDurationMS, bool restart, bool loop)
{
    // Constants for numerical stability and animation thresholds
    static const float EPSILON = 0.0001f;
    static const float COMPLETION_THRESHOLD = 0.999f;
    
    // Structure to hold state for each neoPixelId
    struct SweepState {
        bool initialized;
        unsigned long startTimeMS;
        unsigned long lastStepTimeMS;
        bool sweepingForward;  // True = ends to center, False = center to ends
    };
    
    // Static map to track state per neoPixelId
    static std::map<unsigned int, SweepState> stateMap;
    
    // Get current time
    unsigned long currentTimeMS = GetTickCountGfx();
    
    // Get or create state for this neoPixelId
    SweepState& state = stateMap[neoPixelId];
    
    // Initialize or reset if this is the first call
    if (!state.initialized) {
        state.initialized = true;
        state.startTimeMS = currentTimeMS;
        state.lastStepTimeMS = currentTimeMS;
        
        // Initialize all pixels to start color
        SendNeoPixelAllMsg(neoPixelId, startR, startG, startB, 255);
        return;
    }
    
    // Check if step time has elapsed
    unsigned long stepElapsedMS = currentTimeMS - state.lastStepTimeMS;
    if (stepElapsedMS < stepTimeMS) {
        return; // Not time for next step yet
    }
    
    // Update last step time
    state.lastStepTimeMS = currentTimeMS;
    
    // Get LED count - use neoPixelId as array index to get board index
    unsigned int numLEDs = 0;
    int boardIndex = -1;
    
    // Get board index directly from neoPixelId (array index - no bounds check needed)
    if (g_outputDef[neoPixelId].boardType == PB_NEOPIXEL) {
        boardIndex = g_outputDef[neoPixelId].boardIndex;
    }
    
    // Get LED count using board index
    if (boardIndex >= 0 && m_NeoPixelDriverMap.find(boardIndex) != m_NeoPixelDriverMap.end()) {
        numLEDs = m_NeoPixelDriverMap.at(boardIndex).GetNumLEDs();
    }
    
    if (numLEDs == 0) {
        return; // No LEDs to control
    }
    
    // Calculate elapsed time since start
    unsigned long elapsedMS = currentTimeMS - state.startTimeMS;
    
    // Check if animation is complete
    if (elapsedMS >= totalDurationMS) {
        if (loop) {
            // Loop mode: reverse direction and continue
            state.sweepingForward = !state.sweepingForward;
            state.startTimeMS = currentTimeMS;
            elapsedMS = 0;
        } else if (restart) {
            // Restart mode: jump back to beginning
            state.startTimeMS = currentTimeMS;
            state.sweepingForward = true;
            elapsedMS = 0;
        } else {
            // One-shot mode: keep all pixels at final color, don't reset
            // Don't send message - pixels are already at their final state from last update
            return;
        }
    }
    
    // Calculate overall progress (0.0 to 1.0)
    float overallProgress = (float)elapsedMS / (float)totalDurationMS;
    
    // Calculate center position (works for both even and odd number of LEDs)
    float center = (float)(numLEDs - 1) / 2.0f;
    
    // Update each pixel with gradient based on distance from center
    for (unsigned int i = 0; i < numLEDs; i++) {
        // Calculate distance from center (0.0 to 1.0)
        // Pixels at the ends (0 and numLEDs-1) have distance 1.0
        // Pixels at the center have distance 0.0
        float distanceFromCenter = fabs((float)i - center) / (center + EPSILON); // Add small value to avoid division by zero
        
        // Calculate pixel progress based on overall time and distance from center
        // When sweeping forward: outer pixels (ends) progress faster
        // When sweeping backward: inner pixels (center) progress faster
        float pixelProgress = 0.0f;
        
        if (overallProgress > 0.0f) {
            float threshold, progressRange;
            
            if (state.sweepingForward) {
                // Sweeping from ends to center
                // Pixels at ends start immediately (threshold = 0)
                // Pixels at center start later (threshold closer to 1)
                threshold = 1.0f - distanceFromCenter;
                progressRange = distanceFromCenter;
            } else {
                // Sweeping from center to ends (reverse)
                // Pixels at center start immediately (threshold = 0)
                // Pixels at ends start later (threshold closer to 1)
                threshold = distanceFromCenter;
                progressRange = 1.0f - distanceFromCenter;
            }
            
            if (overallProgress >= threshold) {
                // This pixel should be changing now
                // Map the remaining time to this pixel's progress
                if (progressRange > EPSILON) {
                    pixelProgress = (overallProgress - threshold) / progressRange;
                    if (pixelProgress > 1.0f) pixelProgress = 1.0f;
                } else {
                    // Edge case for center/end pixels
                    pixelProgress = (overallProgress >= COMPLETION_THRESHOLD) ? 1.0f : 0.0f;
                }
            }
        }
        
        // Interpolate color for this pixel based on sweep direction
        uint8_t currentR, currentG, currentB;
        if (state.sweepingForward) {
            // Sweeping from ends (start color) to center (end color)
            currentR = (uint8_t)(startR + (endR - startR) * pixelProgress);
            currentG = (uint8_t)(startG + (endG - startG) * pixelProgress);
            currentB = (uint8_t)(startB + (endB - startB) * pixelProgress);
        } else {
            // Sweeping from center (end color) back to ends (start color)
            currentR = (uint8_t)(endR + (startR - endR) * pixelProgress);
            currentG = (uint8_t)(endG + (startG - endG) * pixelProgress);
            currentB = (uint8_t)(endB + (startB - endB) * pixelProgress);
        }
        
        
        SendNeoPixelSingleMsg(neoPixelId, i, currentR, currentG, currentB, 255);
    }
}

// NeoPixel Toggle - Pixels cycle through up to 4 colors, shifting on each step
// Parameters:
//   color1R/G/B through color4R/G/B: Four colors to cycle (RGB 0-255)
//   colorMask: Bitmask to select active colors (bit 0-3 for colors 1-4)
//              Example: 0x0F (0b1111) = all 4 colors, 0x03 (0b0011) = only colors 1-2
//   neoPixelId: Which NeoPixel output pin/chain to control
//   stepTimeMS: Time in milliseconds between shifts
//   totalDurationMS: Total duration (ignored if loop=true)
//   loop: If true, rotate continuously. If false, stop after totalDurationMS.
void PBEngine::neoPixelToggle(uint8_t color1R, uint8_t color1G, uint8_t color1B,
                              uint8_t color2R, uint8_t color2G, uint8_t color2B,
                              uint8_t color3R, uint8_t color3G, uint8_t color3B,
                              uint8_t color4R, uint8_t color4G, uint8_t color4B,
                              uint8_t colorMask,
                              unsigned int neoPixelId, unsigned int stepTimeMS,
                              unsigned int totalDurationMS, bool loop)
{
    // Structure to hold state for each neoPixelId
    struct ToggleState {
        bool initialized;
        unsigned long startTimeMS;
        unsigned long lastStepTimeMS;
        int shiftOffset;  // Current rotation offset (0-3)
    };
    
    // Static map to track state per neoPixelId
    static std::map<unsigned int, ToggleState> stateMap;
    
    // Get current time
    unsigned long currentTimeMS = GetTickCountGfx();
    
    // Get or create state for this neoPixelId
    ToggleState& state = stateMap[neoPixelId];
    
    // Build color array based on mask
    struct Color { uint8_t r, g, b; };
    Color colors[4];
    int activeColors[4];
    int numActiveColors = 0;
    
    // Populate color array
    colors[0] = {color1R, color1G, color1B};
    colors[1] = {color2R, color2G, color2B};
    colors[2] = {color3R, color3G, color3B};
    colors[3] = {color4R, color4G, color4B};
    
    // Build list of active colors based on mask
    for (int i = 0; i < 4; i++) {
        if (colorMask & (1 << i)) {
            activeColors[numActiveColors++] = i;
        }
    }
    
    // Need at least one active color
    if (numActiveColors == 0) {
        return;
    }
    
    // Initialize or reset if this is the first call
    if (!state.initialized) {
        state.initialized = true;
        state.startTimeMS = currentTimeMS;
        state.lastStepTimeMS = currentTimeMS;
        state.shiftOffset = 0;
        
        // Get LED count - need to find the board index from the output ID
        unsigned int numLEDs = 0;
        int boardIndex = -1;
        
        // Get board index directly from neoPixelId (array index - no bounds check needed)
        if (g_outputDef[neoPixelId].boardType == PB_NEOPIXEL) {
            boardIndex = g_outputDef[neoPixelId].boardIndex;
        }
        
        // Get LED count using board index
        if (boardIndex >= 0 && m_NeoPixelDriverMap.find(boardIndex) != m_NeoPixelDriverMap.end()) {
            numLEDs = m_NeoPixelDriverMap.at(boardIndex).GetNumLEDs();
        }
        
        // Initialize pixels with color pattern
        for (unsigned int i = 0; i < numLEDs; i++) {
            int colorIndex = activeColors[i % numActiveColors];
            SendNeoPixelSingleMsg(neoPixelId, i, colors[colorIndex].r, colors[colorIndex].g, colors[colorIndex].b, 255);
        }
        return;
    }
    
    // Check if step time has elapsed
    unsigned long stepElapsedMS = currentTimeMS - state.lastStepTimeMS;
    if (stepElapsedMS < stepTimeMS) {
        return; // Not time for next step yet
    }
    
    // Update last step time
    state.lastStepTimeMS = currentTimeMS;
    
    // Check if animation should stop (only if not looping)
    if (!loop) {
        unsigned long totalElapsedMS = currentTimeMS - state.startTimeMS;
        if (totalElapsedMS >= totalDurationMS) {
            // Animation complete, keep current state
            return;
        }
    }
    
    // Increment shift offset (rotates colors)
    state.shiftOffset = (state.shiftOffset + 1) % numActiveColors;
    
    // Get LED count
    unsigned int numLEDs = 0;
    int boardIndex = -1;
    
    // Get board index directly from neoPixelId (array index - no bounds check needed)
    if (g_outputDef[neoPixelId].boardType == PB_NEOPIXEL) {
        boardIndex = g_outputDef[neoPixelId].boardIndex;
    }
    
    // Get LED count using board index
    if (boardIndex >= 0 && m_NeoPixelDriverMap.find(boardIndex) != m_NeoPixelDriverMap.end()) {
        numLEDs = m_NeoPixelDriverMap.at(boardIndex).GetNumLEDs();
    }
    
    if (numLEDs == 0) {
        return; // No LEDs to control
    }
    
    // Update all pixels with shifted pattern
    for (unsigned int i = 0; i < numLEDs; i++) {
        // Calculate which color this pixel gets after rotation
        int patternIndex = (i + state.shiftOffset) % numActiveColors;
        int colorIndex = activeColors[patternIndex];
        SendNeoPixelSingleMsg(neoPixelId, i, colors[colorIndex].r, colors[colorIndex].g, colors[colorIndex].b, 255);
    }
}

// NeoPixel Split Toggle - First half one color, second half another, swap on each step
// Parameters:
//   startR, startG, startB: First color (RGB 0-255)
//   endR, endG, endB: Second color (RGB 0-255)
//   neoPixelId: Which NeoPixel output pin/chain to control
//   stepTimeMS: Time in milliseconds between swaps
//   totalDurationMS: Total duration (ignored if loop=true)
//   loop: If true, toggle continuously. If false, stop after totalDurationMS.
void PBEngine::neoPixelSplitToggle(uint8_t startR, uint8_t startG, uint8_t startB,
                                   uint8_t endR, uint8_t endG, uint8_t endB,
                                   unsigned int neoPixelId, unsigned int stepTimeMS,
                                   unsigned int totalDurationMS, bool loop)
{
    // Structure to hold state for each neoPixelId
    struct SplitToggleState {
        bool initialized;
        unsigned long startTimeMS;
        unsigned long lastStepTimeMS;
        bool swapped;  // True when colors are swapped
    };
    
    // Static map to track state per neoPixelId
    static std::map<unsigned int, SplitToggleState> stateMap;
    
    // Get current time
    unsigned long currentTimeMS = GetTickCountGfx();
    
    // Get or create state for this neoPixelId
    SplitToggleState& state = stateMap[neoPixelId];
    
    // Get LED count - use neoPixelId as array index to get board index
    unsigned int numLEDs = 0;
    int boardIndex = -1;
    
    // Get board index directly from neoPixelId (array index - no bounds check needed)
    if (g_outputDef[neoPixelId].boardType == PB_NEOPIXEL) {
        boardIndex = g_outputDef[neoPixelId].boardIndex;
    }
    
    // Get LED count using board index
    if (boardIndex >= 0 && m_NeoPixelDriverMap.find(boardIndex) != m_NeoPixelDriverMap.end()) {
        numLEDs = m_NeoPixelDriverMap.at(boardIndex).GetNumLEDs();
    }
    
    if (numLEDs == 0) {
        return; // No LEDs to control
    }
    
    // Initialize or reset if this is the first call
    if (!state.initialized) {
        state.initialized = true;
        state.startTimeMS = currentTimeMS;
        state.lastStepTimeMS = currentTimeMS;
        state.swapped = false;
        
        // Initialize pixels with split pattern
        int midPoint = numLEDs / 2;
        
        for (unsigned int i = 0; i < numLEDs; i++) {
            if (i < midPoint) {
                // First half: start color
                SendNeoPixelSingleMsg(neoPixelId, i, startR, startG, startB, 255);
            } else if (i > midPoint || numLEDs % 2 == 0) {
                // Second half: end color (skip middle pixel if odd count)
                SendNeoPixelSingleMsg(neoPixelId, i, endR, endG, endB, 255);
            } else {
                // Middle pixel (only if odd number of LEDs): blend the two colors
                uint8_t blendR = (startR + endR) / 2;
                uint8_t blendG = (startG + endG) / 2;
                uint8_t blendB = (startB + endB) / 2;
                SendNeoPixelSingleMsg(neoPixelId, i, blendR, blendG, blendB, 255);
            }
        }
        return;
    }
    
    // Check if step time has elapsed
    unsigned long stepElapsedMS = currentTimeMS - state.lastStepTimeMS;
    if (stepElapsedMS < stepTimeMS) {
        return; // Not time for next step yet
    }
    
    // Update last step time
    state.lastStepTimeMS = currentTimeMS;
    
    // Check if animation should stop (only if not looping)
    if (!loop) {
        unsigned long totalElapsedMS = currentTimeMS - state.startTimeMS;
        if (totalElapsedMS >= totalDurationMS) {
            // Animation complete, keep current state
            return;
        }
    }
    
    // Toggle the swap state
    state.swapped = !state.swapped;
    
    // Update all pixels with swapped pattern
    int midPoint = numLEDs / 2;
    
    for (unsigned int i = 0; i < numLEDs; i++) {
        if (i < midPoint) {
            // First half
            if (state.swapped) {
                SendNeoPixelSingleMsg(neoPixelId, i, endR, endG, endB, 255);
            } else {
                SendNeoPixelSingleMsg(neoPixelId, i, startR, startG, startB, 255);
            }
        } else if (i > midPoint || numLEDs % 2 == 0) {
            // Second half
            if (state.swapped) {
                SendNeoPixelSingleMsg(neoPixelId, i, startR, startG, startB, 255);
            } else {
                SendNeoPixelSingleMsg(neoPixelId, i, endR, endG, endB, 255);
            }
        } else {
            // Middle pixel (only if odd number of LEDs): always blend the two colors
            uint8_t blendR = (startR + endR) / 2;
            uint8_t blendG = (startG + endG) / 2;
            uint8_t blendB = (startB + endB) / 2;
            SendNeoPixelSingleMsg(neoPixelId, i, blendR, blendG, blendB, 255);
        }
    }
}

// NeoPixel Strobe - Flashes between black and a specified color
// Parameters:
//   colorR, colorG, colorB: The color to strobe (RGB 0-255)
//   neoPixelId: Which NeoPixel output pin/chain to control
//   timeOffMS: Base off time in milliseconds (on time is fixed at 5ms)
//   timeOverallMS: If non-zero, scales off time based on remaining time and stops when elapsed
//   loop: If true, strobe continuously. If false, stop after one cycle.
//   resetState: If true, resets the strobe state to start from the beginning
void PBEngine::neoPixelStrobe(uint8_t colorR, uint8_t colorG, uint8_t colorB,
                              unsigned int neoPixelId, unsigned int timeOffMS,
                              unsigned int timeOverallMS, bool loop,
                              bool resetState)
{
    // Structure to hold state for each neoPixelId
    struct StrobeState {
        bool initialized;
        unsigned long startTimeMS;      // Start time for overall duration tracking
        unsigned long lastToggleTimeMS; // Last time we toggled color
        bool colorOn;                   // True when color is on, false when black
    };
    
    // Static map to track state per neoPixelId
    static std::map<unsigned int, StrobeState> stateMap;
    
    // Get current time
    unsigned long currentTimeMS = GetTickCountGfx();
    
    // Get or create state for this neoPixelId
    StrobeState& state = stateMap[neoPixelId];
    
    // Reset state if requested
    if (resetState) {
        state.initialized = false;
    }
    
    // Initialize or reset if this is the first call
    if (!state.initialized) {
        state.initialized = true;
        state.startTimeMS = currentTimeMS;
        state.lastToggleTimeMS = currentTimeMS;
        state.colorOn = true;  // Start with color on
        
        // Initialize all pixels to the specified color
        SendNeoPixelAllMsg(neoPixelId, colorR, colorG, colorB, 255);
        return;
    }
    
    // Define the on time as 5ms
    const unsigned int onTimeMS = 5;
    
    // Calculate current off time based on timeOverallMS
    unsigned int currentOffTimeMS = timeOffMS;
    
    if (timeOverallMS > 0) {
        // Calculate elapsed time since start
        unsigned long elapsedTotalMS = currentTimeMS - state.startTimeMS;
        
        // Check if overall time has expired
        if (elapsedTotalMS >= timeOverallMS) {
            // Time is up - set all pixels to the on color and stop
            SendNeoPixelAllMsg(neoPixelId, colorR, colorG, colorB, 255);
            return;
        }
        
        // Calculate remaining time as a fraction (0.0 to 1.0)
        float remainingFraction = (float)(timeOverallMS - elapsedTotalMS) / (float)timeOverallMS;
        
        // Scale the off time by remaining fraction
        currentOffTimeMS = (unsigned int)(timeOffMS * remainingFraction);
        
        // Ensure minimum off time of 1ms to avoid divide-by-zero or too-fast strobing
        if (currentOffTimeMS < 1) {
            currentOffTimeMS = 1;
        }
    }
    
    // Calculate elapsed time since last toggle
    unsigned long elapsedMS = currentTimeMS - state.lastToggleTimeMS;
    
    // Check if it's time to toggle
    bool shouldToggle = false;
    if (state.colorOn && elapsedMS >= onTimeMS) {
        // Color is on and on time has elapsed, switch to black
        shouldToggle = true;
    } else if (!state.colorOn && elapsedMS >= currentOffTimeMS) {
        // Color is off and off time has elapsed
        if (loop || timeOverallMS > 0) {
            // In loop mode or with overall time, switch back to color
            shouldToggle = true;
        } else {
            // In one-shot mode, stay black
            return;
        }
    }
    
    if (shouldToggle) {
        // Toggle the state
        state.colorOn = !state.colorOn;
        state.lastToggleTimeMS = currentTimeMS;
        
        // Update all pixels
        if (state.colorOn) {
            SendNeoPixelAllMsg(neoPixelId, colorR, colorG, colorB, 255);
        } else {
            SendNeoPixelAllMsg(neoPixelId, 0, 0, 0, 255);  // Black
        }
    }
}

// NeoPixel Snake - A moving snake of pixels runs along the LED string
// Parameters:
//   baseR, baseG, baseB: Base color for non-snake pixels (RGB 0-255)
//   snakeR, snakeG, snakeB: Snake color for the moving snake (RGB 0-255)
//   snakeLength: Length of the snake in pixels
//   wrapMode: If true, snake wraps around. If false, snake exits before restarting
//   neoPixelId: Which NeoPixel output pin/chain to control
//   stepTimeMS: Time in milliseconds between snake movements
void PBEngine::neoPixelSnake(uint8_t baseR, uint8_t baseG, uint8_t baseB,
                             uint8_t snakeR, uint8_t snakeG, uint8_t snakeB,
                             unsigned int snakeLength, bool wrapMode,
                             unsigned int neoPixelId, unsigned int stepTimeMS)
{
    // Structure to hold state for each neoPixelId
    struct SnakeState {
        bool initialized;
        unsigned long lastStepTimeMS;
        int headPosition;  // Current position of the snake head (-snakeLength to numLEDs-1 for no wrap, can be higher for wrap)
    };
    
    // Static map to track state per neoPixelId
    static std::map<unsigned int, SnakeState> stateMap;
    
    // Get current time
    unsigned long currentTimeMS = GetTickCountGfx();
    
    // Get or create state for this neoPixelId
    SnakeState& state = stateMap[neoPixelId];
    
    // Get LED count - use neoPixelId as array index to get board index
    unsigned int numLEDs = 0;
    int boardIndex = -1;
    
    // Get board index directly from neoPixelId (array index - no bounds check needed)
    if (g_outputDef[neoPixelId].boardType == PB_NEOPIXEL) {
        boardIndex = g_outputDef[neoPixelId].boardIndex;
    }
    
    // Get LED count using board index
    if (boardIndex >= 0 && m_NeoPixelDriverMap.find(boardIndex) != m_NeoPixelDriverMap.end()) {
        numLEDs = m_NeoPixelDriverMap.at(boardIndex).GetNumLEDs();
    }
    
    if (numLEDs == 0) {
        return; // No LEDs to control
    }
    
    // Initialize or reset if this is the first call
    if (!state.initialized) {
        state.initialized = true;
        state.lastStepTimeMS = currentTimeMS;
        state.headPosition = 0;  // Start snake head at position 0
        
        // Initialize all pixels to base color
        SendNeoPixelAllMsg(neoPixelId, baseR, baseG, baseB, 255);
        return;
    }
    
    // Check if step time has elapsed
    unsigned long stepElapsedMS = currentTimeMS - state.lastStepTimeMS;
    if (stepElapsedMS < stepTimeMS) {
        return; // Not time for next step yet
    }
    
    // Update last step time
    state.lastStepTimeMS = currentTimeMS;
    
    // Move the snake head forward by one position
    state.headPosition++;
    
    // Handle wrap mode vs no-wrap mode
    if (wrapMode) {
        // Wrap mode: snake wraps around continuously
        // If snake is longer than the string, wait until tail has moved at least one pixel past start
        if (snakeLength > numLEDs) {
            // Reset when the tail has moved past position 0 by at least 1 pixel
            // tailPosition = headPosition - snakeLength + 1
            // We want tailPosition > 0, so headPosition > snakeLength - 1
            if (state.headPosition >= (int)snakeLength) {
                state.headPosition = 0;  // Restart from beginning
            }
        } else {
            // Normal wrap: restart when head completes the full cycle
            if (state.headPosition >= (int)numLEDs) {
                state.headPosition = 0;
            }
        }
    } else {
        // No-wrap mode: don't show head at first pixel again until tail exits
        // The snake fully exits when tailPosition >= numLEDs
        // tailPosition = headPosition - snakeLength + 1
        // We want tailPosition >= numLEDs, so headPosition >= numLEDs + snakeLength - 1
        if (state.headPosition >= (int)(numLEDs + snakeLength)) {
            state.headPosition = 0;  // Restart from beginning
        }
    }
    
    // Update all pixels based on snake position
    for (unsigned int i = 0; i < numLEDs; i++) {
        // Calculate if this pixel is part of the snake
        // Snake occupies positions from (headPosition - snakeLength + 1) to headPosition
        int tailPosition = state.headPosition - (int)snakeLength + 1;
        
        bool isSnake = false;
        
        if (wrapMode) {
            // In wrap mode, handle wrapping for both head and tail
            for (int j = 0; j < (int)snakeLength; j++) {
                int snakePos = state.headPosition - j;
                // Wrap snake position to valid range [0, numLEDs-1]
                while (snakePos < 0) snakePos += numLEDs;
                while (snakePos >= (int)numLEDs) snakePos -= numLEDs;
                
                if (snakePos == (int)i) {
                    isSnake = true;
                    break;
                }
            }
        } else {
            // In no-wrap mode, only show snake if within valid bounds
            if ((int)i >= tailPosition && (int)i <= state.headPosition) {
                isSnake = true;
            }
        }
        
        // Set pixel color based on whether it's part of the snake
        if (isSnake) {
            SendNeoPixelSingleMsg(neoPixelId, i, snakeR, snakeG, snakeB, 255);
        } else {
            SendNeoPixelSingleMsg(neoPixelId, i, baseR, baseG, baseB, 255);
        }
    }
}






// PInballMenus.h:  Menus used by the main pinball engine.
// This file can only be inlcuded in PInball.cpp, and not in any other file.

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef PInballMenus_h
#define PInballMenus_h

#include <map>
#include <string>

std::map<unsigned int, std::string>  g_mainMenu = {
    {0, "Play Pinball"},
    {1, "Settings"},
    {2, "Test Mode"},
    {3, "Benchmark"},
    {4, "Console"},
    {5, "Credits"}
};

// Make a global variable same as above, but for the settings menu
std::map<unsigned int, std::string>  g_settingsMenu = {
    {0, "Main Volume: "},
    {1, "Music Volume: "},
    {2, "Balls Per Game: "},
    {3, "Difficulty: "},
    {4, "Reset High Scores"}
};

#define PB_EASY_TEXT "Easy"
#define PB_NORMAL_TEXT "Normal"
#define PB_HARD_TEXT "Hard"
#define PB_EPIC_TEXT "Epic"

#define MenuTitle "Dragons of Destiny"
#define Menu1 "Play Pinball"
#define Menu1Fail "Play Pinball (Disabled)"
#define Menu2 "Settings"
#define Menu3 "Test Mode"
#define Menu4 "Benchmark"
#define Menu5 "Console"
#define Menu6 "Credits"

#define MenuSettingsTitle "Settings"
#define MenuSettings1 "Main Volume: "
#define MenuSettings2 "Music Volume: "
#define MenuSettings3 "Balls Per Game: "
#define MenuSettings4 "Difficulty: "
#define MenuSettings5 "Reset High Scores"

#define NUM_SETTINGS 5

#define CURSOR_TO_MENU_SPACING 8

#endif // PInballMenus_h  
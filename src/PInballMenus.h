
// PInballMenus.h:  Menus used by the main pinball engine.
// This file can only be inlcuded in PInball.cpp, and not in any other file.

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef PInballMenus_h
#define PInballMenus_h

#include <map>
#include <string>

// Details for the main menu
#define MenuTitle "Dragons of Destiny"
std::map<unsigned int, std::string>  g_mainMenu = {
    {0, "Play Pinball"},
    {1, "Settings"},
    {2, "Test Mode"},
    {3, "Benchmark"},
    {4, "Console"},
    {5, "Credits"}
};

// Details for the settings menu
#define MenuSettingsTitle "Settings"
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

// #define NUM_SETTINGS 5

#define CURSOR_TO_MENU_SPACING 8

#endif // PInballMenus_h  
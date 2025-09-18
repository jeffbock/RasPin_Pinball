
// PinballMenus.h:  Menus used by the main pinball engine.
// This file can only be inlcuded in Pinball.cpp, and not in any other file.

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef PinballMenus_h
#define PinballMenus_h

#include <map>
#include <string>

// Details for the main menu
#define MenuTitle "Dragons of Destiny"
extern std::map<unsigned int, std::string> g_mainMenu;

// Details for the settings menu
#define MenuSettings "Settings"
extern std::map<unsigned int, std::string> g_settingsMenu;

#define PB_EASY_TEXT "Easy"
#define PB_NORMAL_TEXT "Normal"
#define PB_HARD_TEXT "Hard"
#define PB_EPIC_TEXT "Epic"

// Details for the diagnotic menu
#define MenuDiagnostics "Diagnostics"
extern std::map<unsigned int, std::string> g_diagnosticsMenu;

#define PB_ON_TEXT "On"
#define PB_OFF_TEXT "Off"

// #define NUM_SETTINGS 5

#define CURSOR_TO_MENU_SPACING 8

#endif // PinballMenus_h  
// PinballMenus.cpp: Implementation of global menu definitions
// 
// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "PBBuildSwitch.h"
#include "PinballMenus.h"

// Global menu definitions
std::map<unsigned int, std::string> g_mainMenu = {
    {0, "Play Pinball"},
    {1, "Settings"},
    {2, "Diagnostics"},
    {3, "Credits"},
    #if ENABLE_TEST_SANDBOX
    {4, "Test Sandbox"}
    #endif
};

std::map<unsigned int, std::string> g_settingsMenu = {
    {0, "Main Volume: "},
    {1, "Music Volume: "},
    {2, "Balls Per Game: "},
    {3, "Difficulty: "},
    {4, "Reset High Scores"}
};

std::map<unsigned int, std::string> g_diagnosticsMenu = {
    {0, "Test Input/Output"},
    {1, "Run Benchmark"},
    {2, "I/O Overlay: "},
    {3, "Show FPS: "},
    {4, "Show Console"}
};
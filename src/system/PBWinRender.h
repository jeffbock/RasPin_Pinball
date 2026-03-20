// Window and render code for Windows
// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

// Note: This file is not built during the RasPi build flow

#ifndef PBWinRender_h
#define PBWinRender_h

#include <windows.h>

HWND PBInitWinRender (long width, long height);

#endif // PBWinRender_h
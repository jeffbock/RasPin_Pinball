// Window and render code for Windows
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#ifndef PBWinRender_h
#define PBWinRender_h

#ifdef EXE_MODE_WINDOWS
#include <windows.h>

HWND PBInitWinRender (long width, long height);
#endif // EXE_MODE_WINDOWS

#endif // PBWinRender_h
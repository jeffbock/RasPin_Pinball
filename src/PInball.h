// PInball:  A complete pinball framework for building 1/2 scale phyical pinball machines with Raspberry Pi
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#ifndef PInball_h
#define PInball_h

// Choose the EXE mode for the code - Windows is used for developent / debug, Rasberry Pi is used for the final build
// Comment out this line if building for Rasberry Pi
#define EXE_MODE_WINDOWS

#ifdef EXE_MODE_WINDOWS
// Windows specific includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "PBWinRender.h"

#else
#define EXE_MODE_RASPI
// Rasberry PI specific includes
// Comment out define below if you want to build / use a PiOS as the simulator instead of windows
// #define RASPI_SIMULATOR
#endif  // Platform include selection

// Includes for all platforms
#include "include_ogl/egl.h"
#include "include_ogl/gl31.h"
#include "schrift.h"
#include "PBOGLES.h"
#include "PBGfx.h"
#include <iostream>
#include <stdio.h>

// This must be set to whatever actual screen size is being use for Rasbeery Pi
#define PB_SCREENWIDTH 800
#define PB_SCREENHEIGHT 480

bool PBInitRender (long width, long height);

// Make new class named PBGame that inheritis from PBOGLES with just essential functions
class PBEngine : public PBGfx {
public:
    PBEngine();
    ~PBEngine();
    // bool init (long width, long height, NativeWindowType nativeWindow);
    // bool clear (long color, bool doFlip);
    // void run();
};

#endif // PInball_h
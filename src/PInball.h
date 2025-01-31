// Main Header file for Pinball

#ifndef PInball_h
#define PInball_h

// Choose the EXE mode for the code - Windows is used for developent / debug, Rasberry Pi is used for the final build
// Comment out this line if building for Rasberry Pi
#define EXE_MODE_WINDOWS

#ifdef EXE_MODE_WINDOWS
// Windows specific includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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
#include <stdio.h>

#endif // PInball_h
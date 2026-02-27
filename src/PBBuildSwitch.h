// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef PBBuildSwitch_h
#define PBBuildSwitch_h

// This header file only serves as the build switch between Windows and PiOS.  Uncomment the appropriate linefor the desired platform.

#define EXE_MODE_WINDOWS
// #define EXE_MODE_RASPI

// Enable/Disable Test Sandbox menu item
// 0 = disabled (no menu item)
// 1 = enabled as "Test Sandbox"
// 2 = enabled as "Simple Flip Mode"
#define ENABLE_TEST_SANDBOX 1

// Enable hardware video decoding on Raspberry Pi (requires V4L2 M2M support)
// Set to 1 to enable hardware decode, 0 for software decode
#define ENABLE_HW_VIDEO_DECODE 0

#endif
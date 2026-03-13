// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef PBBuildSwitch_h
#define PBBuildSwitch_h

// This header file only serves as the build switch between Windows and PiOS.  Uncomment the appropriate linefor the desired platform.

#define EXE_MODE_WINDOWS
// #define EXE_MODE_DEBIAN
// #define EXE_MODE_RASPI

#if (defined(EXE_MODE_WINDOWS) + defined(EXE_MODE_DEBIAN) + defined(EXE_MODE_RASPI)) != 1
#error "Exactly one EXE_MODE_* define must be enabled"
#endif

// Enable/Disable Test Sandbox menu item
// 0 = disabled (no menu item)
// 1 = enabled as "Test Sandbox"
// 2 = enabled as "Simple Flip Mode"
#define ENABLE_TEST_SANDBOX 0

// Enable hardware video decoding on Raspberry Pi (requires V4L2 M2M support)
// Set to 1 to enable hardware decode, 0 for software decode
#define ENABLE_HW_VIDEO_DECODE 0

#if defined(EXE_MODE_DEBIAN) || defined(EXE_MODE_WINDOWS)
// Uncomment to run the simulator window at 1/4 the virtual screen area
// (half width, half height).  Useful for RDP / remote-desktop performance.
// Comment out to use full native resolution.  Has no effect on Raspberry Pi.
#define SIMULATOR_SMALL_WINDOW
#endif

#endif
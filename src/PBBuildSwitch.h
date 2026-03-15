// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef PBBuildSwitch_h
#define PBBuildSwitch_h

// =======================================================================================
// SECTION 1: PLATFORM SELECTION (EXE_MODE_*) and Enable HW Mode (ENABLE_PINBALL_HARDWARE)
// =======================================================================================
// Selects the target operating system / environment.  Exactly one must be set.
//
// Build tasks in tasks.json pass -DEXE_MODE_* (GCC/Clang) or /DEXE_MODE_* (MSVC)
// which take priority over the manual selection below.
// Only change the manual selection when building outside of VS Code tasks.
//
//   EXE_MODE_WINDOWS  - Windows PC.  Simulator mode only (no pinball hardware).
//   EXE_MODE_DEBIAN   - Debian/Ubuntu Linux.  Simulator mode only.
//   EXE_MODE_RASPI    - Raspberry Pi OS.  Supports both simulator and hardware
//                       mode (see ENABLE_PINBALL_HARDWARE below).
//
// Define ENABLE_PINBALL_HARDWARE to enable full physical pinball machine support.
// This activates wiringPi GPIO/I2C/SPI communication with the hardware breakout
// board, debounce inputs, output coils, LEDs, and NeoPixels.
//
// Only valid when EXE_MODE_RASPI is selected.  Defining it with any other
// EXE_MODE will cause a compile error.
//
// When NOT defined (the default), the build targets SIMULATOR MODE regardless of
// platform:
//   - Windowed X11 (Linux) or Win32 window with keyboard input.
//   - No physical I/O hardware required.
//   - SIMULATOR_SMALL_WINDOW option is available (see Section 3).

#if !defined(EXE_MODE_WINDOWS) && !defined(EXE_MODE_DEBIAN) && !defined(EXE_MODE_RASPI)
#define EXE_MODE_WINDOWS
//#define EXE_MODE_DEBIAN
// #define EXE_MODE_RASPI
#endif

#if (defined(EXE_MODE_WINDOWS) + defined(EXE_MODE_DEBIAN) + defined(EXE_MODE_RASPI)) != 1
#error "Exactly one EXE_MODE_* define must be enabled"
#endif

// To build for a real pinball machine, uncomment the line below:
// #define ENABLE_PINBALL_HARDWARE

#if defined(ENABLE_PINBALL_HARDWARE) && !defined(EXE_MODE_RASPI)
#error "ENABLE_PINBALL_HARDWARE requires EXE_MODE_RASPI"
#endif

// =============================================================================
// SECTION 2: SIMULATOR OPTIONS
// =============================================================================
// These options apply whenever ENABLE_PINBALL_HARDWARE is NOT defined, i.e.
// any Windows, Debian, or Raspberry Pi simulator build.

// Uncomment to run the simulator window at 1/4 the virtual screen area
// (half width, half height).  Useful for RDP / remote-desktop performance.
// Comment out to use full native resolution.
#ifndef ENABLE_PINBALL_HARDWARE
// #define SIMULATOR_SMALL_WINDOW
#endif

// =============================================================================
// SECTION 3: MAIN MENU OPTION
// =============================================================================
// Controls which optional item appears as the 5th entry in the main menu.
//   0 = High Scores  (default)
//   1 = Test Sandbox (developer hardware/feature test)
//   2 = Simple Flip Mode (basic flipper I/O test with overlay)
#define MAIN_MENU_OPTION 0

// =============================================================================
// SECTION 5: VIDEO DECODE
// =============================================================================
// Enable hardware video decoding (requires V4L2 M2M support).
// This is independent of ENABLE_PINBALL_HARDWARE — simulators may also have
// hardware decode capability depending on the host platform.
// Set to 1 to enable hardware decode, 0 for software decode.
// BUG / CONFIG ISSUE:  This doesn't seem to work for Raspberry Pi, where it's really needed. Might work later w/ OS updates.
#define ENABLE_HW_VIDEO_DECODE 0

#endif
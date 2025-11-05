// Pinball:  A complete pinball framework for building 1/2 scale phyical pinball machines with Raspberry Pi

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef Pinball_h
#define Pinball_h

// Define WIN32_LEAN_AND_MEAN before any other includes to avoid redefinition warnings
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

// Choose the EXE mode for the code - Windows is used for developent / debug, Rasberry Pi is used for the final build
// Change the PBBuildSwitch.h to change the build mode
#include "PBBuildSwitch.h"

#ifdef EXE_MODE_WINDOWS
// Windows specific includes
#include <windows.h>
#include "PBWinRender.h"
#endif

#ifdef EXE_MODE_RASPI
// Rasberry PI specific includes
#include "wiringPi.h"
#include "wiringPiI2C.h"
#include "PBDebounce.h"

#endif  // Platform include selection

// Includes for all platforms
#include <egl.h>
#include <gl31.h>
#include "PBOGLES.h"
#include "PBGfx.h"
#include "Pinball_IO.h"
#include "Pinball_Table.h"
#include "PBSound.h"
#include <iostream>
#include <stdio.h>
#include <vector>
#include <queue>
#include <map>
#include <chrono>
// Multi-threading includes for potential future use
//#include <thread>
//#include <mutex>
//#include <condition_variable>
#include "PBDebounce.h"
#include "PinballMenus.h"
#include "Pinball_Engine.h"

// Version Information
#define PB_VERSION_MAJOR 0
#define PB_VERSION_MINOR 5  
#define PB_VERSION_BUILD 170

// This must be set to whatever actual screen size is being use for Rasbeery Pi
#define PB_SCREENWIDTH 1920
#define PB_SCREENHEIGHT 1080

#define MAX_DEFERRED_LED_QUEUE  100  // Maximum size of the deferred LED message queue

// FPS limit for the game rendering
#define PB_FPSLIMIT 30
#define PB_MS_PER_FRAME (PB_FPSLIMIT == 0 ? 0 : (1000 / PB_FPSLIMIT))

#define MENUFONT "src/resources/fonts/Baldur_96_768.png"
#define MENUSWORD "src/resources/textures/MenuSword.png"
#define SAVEFILENAME "src/resources/savefile.bin"

// Sound files
#define MUSICFANTASY "src/resources/sound/fantasymusic.mp3"
#define EFFECTSWORDHIT "src/resources/sound/swordcut.mp3"
#define EFFECTCLICK "src/resources/sound/click.mp3"

bool PBInitRender (long width, long height);

// Version display function
void ShowVersion();

// Forward declarations for structures
struct stInputMessage;
struct stOutputMessage;
struct stOutputDef;

// Forward declarations for functions defined in Pinball.cpp

// Windows-specific simulator input function
#ifdef EXE_MODE_WINDOWS
void PBWinSimInput(std::string character, PBPinState inputState, stInputMessage* inputMessage);
#endif

// Platform-specific I/O processing functions - these change depending on the EXE_MODE
bool PBProcessInput();
bool PBProcessOutput();
bool PBProcessIO();

#ifdef EXE_MODE_RASPI
// Output processing utility functions - Used only in Raspberry Pi Mode
int FindOutputDefIndex(unsigned int outputId);
void SendAllStagedIO();
void SendAllStagedLED();
void ProcessLEDSequenceMessage(const stOutputMessage& message);
void ProcessIOOutputMessage(const stOutputMessage& message, stOutputDef& outputDef);
void ProcessLEDOutputMessage(const stOutputMessage& message, stOutputDef& outputDef, bool skipSequenceCheck = false);
void ProcessLEDConfigMessage(const stOutputMessage& message, stOutputDef& outputDef);
void ProcessActivePulseOutputs();
void ProcessActiveLEDSequence();
void HandleLEDSequenceBoundaries();
void EndLEDSequence();
void ProcessDeferredLEDQueue();
#endif

#endif // Pinball_h
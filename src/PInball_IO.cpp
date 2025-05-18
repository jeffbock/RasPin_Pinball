// PInball_IO.cpp:  Shared input / output structures used the pinball engine
// These need to be defined by the user for the specific table

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

// NOTE:  This IO file is to be used with the basic PInball breakout box setup, (see HW schematics /hw/SimpleBreakoutSchematic.png)
// All active connections use the GPIO pins on the Raspberry Pi directily, rather than I2C and expansion ICs

#include "PInball_IO.h"

// Output definitions
stOutputDef g_outputDef[] = {
    {"Jet Bumper", PB_OUTPUT_JETBUMPER, IDO_JETBUMPER, 1, PB_OUTPUT1, PB_OFF},
    {"Pop Bumper", PB_OUTPUT_POPBUMPER, IDO_POPBUMPER, 2, PB_OUTPUT1, PB_OFF},
    {"LED", PB_OUTPUT_LED, IDO_LED1, 23, PB_RASPI, PB_OFF}
};

// Input definitions
stInputDef g_inputDef[] = {
    {"Left Flipper", "A", PB_INPUT_BUTTON, IDI_LEFTFLIPPER, 27, PB_RASPI, PB_OFF, 0, 0, 4},
    {"Right Flipper", "D", PB_INPUT_BUTTON, IDI_RIGHTFLIPPER, 17, PB_RASPI, PB_OFF, 0, 0, 4},
    {"Left Activate", "Q", PB_INPUT_BUTTON, IDI_LEFTACTIVATE, 5, PB_RASPI, PB_OFF, 0, 0, 4},
    {"Right Activate", "E", PB_INPUT_BUTTON, IDI_RIGHTACTIVATE,22, PB_RASPI, PB_OFF, 0, 0, 4},
    {"Start", "Z", PB_INPUT_BUTTON, IDI_START, 6, PB_RASPI, PB_OFF, 0, 0, 4}
};
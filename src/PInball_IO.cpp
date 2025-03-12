// PInball_IO.cpp:  Shared input / output structures used the pinball engine
// These need to be defined by the user for the specific table

#include "PInball_IO.h"

// Output definitions
stOutputDef g_outputDef[] = {
    {"Jet Bumper", PB_OUTPUT_JETBUMPER, IDO_JETBUMPER, 1, PB_OUTPUT1, PB_OFF},
    {"Pop Bumper", PB_OUTPUT_POPBUMPER, IDO_POPBUMPER, 2, PB_OUTPUT1, PB_OFF}
};

// Input definitions
stInputDef g_inputDef[] = {
    {"Left Flipper", "A", PB_INPUT_BUTTON, IDI_LEFTFLIPPER, 1, PB_INPUT1, PB_OFF, 0, 0, 10},
    {"Right Flipper", "D", PB_INPUT_BUTTON, IDI_RIGHTFLIPPER, 2, PB_INPUT1, PB_OFF, 0, 0, 10},
    {"Left Activate", "Q", PB_INPUT_BUTTON, IDI_LEFTACTIVATE, 3, PB_INPUT1, PB_OFF, 0, 0, 10},
    {"Right Activate", "E", PB_INPUT_BUTTON, IDI_RIGHTACTIVATE, 4, PB_INPUT1, PB_OFF, 0, 0, 10},
    {"Start", "Z", PB_INPUT_BUTTON, IDI_START, 5, PB_INPUT1, PB_OFF, 0, 0, 100}
};
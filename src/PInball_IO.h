// PInball_IO.h:  Header file to define all the input sensors and buttons in the pinball machine
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#ifndef PInball_IO_h
#define PInball_IO_h

#include <string>


// Geneic IO definitions
enum PBPinState {
    PB_ON = 0,
    PB_OFF = 1,
};

enum PBBoardType {
    PB_RASPI = 0,
    PB_INPUT1 = 1,
    PB_OUTPUT1 = 2,
    PB_OUTPUT2 = 3,
    PB_NOBOARD
};

// Input message structs and types
enum PBInputType {
    PB_INPUT_BUTTON = 0,
    PB_INPUT_BALLSENSOR = 1,
    PB_INPUT_TARGET = 2,
    PB_INPUT_JETBUMPER = 3,
    PB_INPUT_POPBUMPER = 4,
};

struct stInputDef{
    std::string inputName; 
    std::string simMapKey;
    PBInputType inputType; 
    unsigned int id;
    unsigned int pin;
    PBBoardType boardType;
    PBPinState lastState;
    unsigned long lastStateTick;
    unsigned long timeInState;
    unsigned long debounceTimeMS;
};

// The actual definition of the input items - the user of the library will need to define these for the specific table
// The IDs need to be unique, and will be checked during initilization - if there are duplicates, the engine will not start

#define IDI_LEFTFLIPPER 0
#define IDI_RIGHTFLIPPER 1
#define IDI_LEFTACTIVATE 2
#define IDI_RIGHTACTIVATE 3
#define IDI_START 4

stInputDef g_inputDef[] = {
    {"Left Flipper", "A", PB_INPUT_BUTTON, IDI_LEFTFLIPPER, 1, PB_INPUT1, PB_OFF, 0, 0, 10},
    {"Right Flipper", "D", PB_INPUT_BUTTON, IDI_RIGHTFLIPPER, 2, PB_INPUT1, PB_OFF, 0, 0, 10},
    {"Left Activate", "Q", PB_INPUT_BUTTON, IDI_LEFTACTIVATE, 3, PB_INPUT1, PB_OFF, 0, 0, 10},
    {"Right Activate", "E", PB_INPUT_BUTTON, 3, IDI_RIGHTACTIVATE, PB_INPUT1, PB_OFF, 0, 0, 10},
    {"Start", "Z", PB_INPUT_BUTTON, IDI_START, 5, PB_INPUT1, PB_OFF, 0, 0, 100}
};

#define NUM_INPUTS (sizeof(g_inputDef) / sizeof(stInputDef))

// Output defintions
// Input message structs and types
enum PBOutputType {
    PB_OUTPUT_JETBUMPER = 1,
    PB_OUTPUT_POPBUMPER = 2,
};

// Placeholder - there are probably items to add here
struct stOutputDef{
    std::string outputName; 
    std::string simMapKey;
    PBOutputType outputType; 
    unsigned int id;
    unsigned int pin;
    PBBoardType boardType;
};

#define IDO_JETBUMPER 0
#define IDO_POPBUMPER 1

stOutputDef g_outputDef[] = {
    {"Jet Bumper", "J", PB_OUTPUT_JETBUMPER, IDO_JETBUMPER, 1, PB_OUTPUT1},
    {"Pop Bumper", "P", PB_OUTPUT_POPBUMPER, IDO_POPBUMPER, 2, PB_OUTPUT1}
};

#define NUM_OUTPUTS (sizeof(g_outputDef) / sizeof(stOutputDef))

#endif // PInball_IO.h
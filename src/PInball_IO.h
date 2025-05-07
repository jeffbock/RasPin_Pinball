// PInball_IO.h:  Header file to define all the input sensors and buttons in the pinball machine
// These need to be defined by the user for the specific table
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#ifndef PInball_IO_h
#define PInball_IO_h

#include <string>

#define PB_I2C_AMPLIFIER 0x4B

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

// Output defintions
// Input message structs and types
enum PBOutputType {
    PB_OUTPUT_JETBUMPER = 1,
    PB_OUTPUT_POPBUMPER = 2,
    PB_OUTPUT_LED = 3,
};

// Placeholder - there are probably items to add here
struct stOutputDef{
    std::string outputName; 
    PBOutputType outputType; 
    unsigned int id;
    unsigned int pin;
    PBBoardType boardType;
    PBPinState lastState;
};

// The actual definition of the input items - the user of the library will need to define these for the specific table
// The IDs need to be unique, and will be checked during initilization - if there are duplicates, the engine will not start

// The actual definition of the output items - the user of the library will need to define these for the specific table

#define IDO_JETBUMPER 0
#define IDO_POPBUMPER 1
#define IDO_LED1 2
#define NUM_OUTPUTS 3

#define IDI_LEFTFLIPPER 0
#define IDI_RIGHTFLIPPER 1
#define IDI_LEFTACTIVATE 2
#define IDI_RIGHTACTIVATE 3
#define IDI_START 4
#define NUM_INPUTS 5

// Declare the shared variables for input / output structures.
extern stInputDef g_inputDef[];
extern stOutputDef g_outputDef[];

#endif
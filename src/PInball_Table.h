// PInball_Table.h:  Header file for the specific functions needed for the pinball table.  
//                   This includes further PBEngine Class functions and any support functions unique to the table
//                   These need to be defined by the user for the specific table
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#ifndef PInball_Table_h
#define PInball_Table_h

#include "PInball.h"

enum PBTableState {
    PBTBL_START = 0,
    PBTBL_STDPLAY = 1,
    PBTBL_END
};

enum PBTBLScreenState {
    START_START = 0,
    START_INST = 1,
    START_SCORES = 2,
    START_OPENDOOR = 3,
    START_END
};

#endif // PInball_Table_h
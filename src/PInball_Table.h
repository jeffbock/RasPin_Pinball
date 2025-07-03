// PInball_Table.h:  Header file for the specific functions needed for the pinball table.  
//                   This includes further PBEngine Class functions and any support functions unique to the table
//                   These need to be defined by the user for the specific table

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef PInball_Table_h
#define PInball_Table_h

#include "PInball.h"

enum class PBTableState {
    PBTBL_START = 0,
    PBTBL_STDPLAY = 1,
    PBTBL_END
};

enum class PBTBLScreenState {
    START_START = 0,
    START_INST = 1,
    START_SCORES = 2,
    START_OPENDOOR = 3,
    START_END
};

#define ACTIVEDISPX 448
#define ACTIVEDISPY 268

#endif // PInball_Table_h
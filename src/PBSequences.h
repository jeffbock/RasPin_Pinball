// PBSequences.h: LED sequence definitions for pinball machine
// Contains pre-defined LED sequences that can be used for animations and effects

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef PBSequences_h
#define PBSequences_h

#include "Pinball_Engine.h"

// Sequence array declarations
extern const LEDSequence PBSeq_AllChipsTest;
extern const uint16_t PBSeq_AllChipsTestMask[NUM_LED_CHIPS];

extern const LEDSequence PBSeq_LastThreeTest;
extern const uint16_t PBSeq_LastThreeTestMask[NUM_LED_CHIPS];

extern const LEDSequence PBSeq_RGBColorCycle;
extern const uint16_t PBSeq_RGBColorCycleMask[NUM_LED_CHIPS];

#endif // PBSequences_h

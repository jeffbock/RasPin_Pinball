// PBSequences.cpp: LED sequence definitions for pinball machine
// Contains pre-defined LED sequences that can be used for animations and effects

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "PBSequences.h"

// Static array of steps for AllChipsTest sequence
static const stLEDSequence PBSeq_AllChipsTest_Steps[] = {
    {{0x00FF, 0x0000, 0x0000}, 500, 0},  // Chip 0, LEDs 0-7
    {{0xFF00, 0x0000, 0x0000}, 500, 0},  // Chip 0, LEDs 8-15
    {{0x0000, 0x00FF, 0x0000}, 500, 0},  // Chip 1, LEDs 0-7
    {{0x0000, 0xFF00, 0x0000}, 500, 0},  // Chip 1, LEDs 8-15
    {{0x0000, 0x0000, 0x00FF}, 500, 0},  // Chip 2, LEDs 0-7
    {{0x0000, 0x0000, 0xFF00}, 500, 0}   // Chip 2, LEDs 8-15
};

// Sequence data structure (compile-time initialized)
const LEDSequence PBSeq_AllChipsTest = {
    PBSeq_AllChipsTest_Steps,
    sizeof(PBSeq_AllChipsTest_Steps) / sizeof(PBSeq_AllChipsTest_Steps[0])
};

const uint16_t PBSeq_AllChipsTestMask[NUM_LED_CHIPS] = {0xFFFF, 0xFFFF, 0xFFFF};

// Static array of steps for LastThreeTest sequence (LEDs 8,9,10 on each chip)
static const stLEDSequence PBSeq_LastThreeTest_Steps[] = {
    {{0x0700, 0x0000, 0x0000}, 500, 250},  // Chip 0, LEDs 8-10 on for 500ms, off for 250ms
    {{0x0000, 0x0700, 0x0000}, 500, 250},  // Chip 1, LEDs 8-10 on for 500ms, off for 250ms
    {{0x0000, 0x0000, 0x0700}, 500, 250}   // Chip 2, LEDs 8-10 on for 500ms, off for 250ms
};

// Sequence data structure (compile-time initialized)
const LEDSequence PBSeq_LastThreeTest = {
    PBSeq_LastThreeTest_Steps,
    sizeof(PBSeq_LastThreeTest_Steps) / sizeof(PBSeq_LastThreeTest_Steps[0])
};

const uint16_t PBSeq_LastThreeTestMask[NUM_LED_CHIPS] = {0x0700, 0x0700, 0x0700};

// Static array of steps for RGB Color Cycle sequence (LEDs 8=Red, 9=Green, 10=Blue on each chip)
static const stLEDSequence PBSeq_RGBColorCycle_Steps[] = {
    // Single chip color progression - same color per set (150ms on, 50ms off)
    {{0x0100, 0x0000, 0x0000}, 150, 50},   // Chip 0: Red
    {{0x0000, 0x0100, 0x0000}, 150, 50},   // Chip 1: Red  
    {{0x0000, 0x0000, 0x0100}, 150, 50},   // Chip 2: Red
    {{0x0200, 0x0000, 0x0000}, 150, 50},   // Chip 0: Green
    {{0x0000, 0x0200, 0x0000}, 150, 50},   // Chip 1: Green
    {{0x0000, 0x0000, 0x0200}, 150, 50},   // Chip 2: Green
    {{0x0400, 0x0000, 0x0000}, 150, 50},   // Chip 0: Blue
    {{0x0000, 0x0400, 0x0000}, 150, 50},   // Chip 1: Blue
    {{0x0000, 0x0000, 0x0400}, 150, 50},   // Chip 2: Blue
    {{0x0300, 0x0000, 0x0000}, 150, 50},   // Chip 0: Yellow (Red+Green)
    {{0x0000, 0x0300, 0x0000}, 150, 50},   // Chip 1: Yellow (Red+Green)
    {{0x0000, 0x0000, 0x0300}, 150, 50},   // Chip 2: Yellow (Red+Green)
    {{0x0500, 0x0000, 0x0000}, 150, 50},   // Chip 0: Magenta (Red+Blue)
    {{0x0000, 0x0500, 0x0000}, 150, 50},   // Chip 1: Magenta (Red+Blue)
    {{0x0000, 0x0000, 0x0500}, 150, 50},   // Chip 2: Magenta (Red+Blue)
    {{0x0600, 0x0000, 0x0000}, 150, 50},   // Chip 0: Cyan (Green+Blue)
    {{0x0000, 0x0600, 0x0000}, 150, 50},   // Chip 1: Cyan (Green+Blue)
    {{0x0000, 0x0000, 0x0600}, 150, 50},   // Chip 2: Cyan (Green+Blue)
    {{0x0700, 0x0000, 0x0000}, 150, 50},   // Chip 0: White (All colors)
    {{0x0000, 0x0700, 0x0000}, 150, 50},   // Chip 1: White (All colors)
    {{0x0000, 0x0000, 0x0700}, 150, 50},   // Chip 2: White (All colors)
    
    // All chips synchronized with middle LED different (100ms on, no off time)
    {{0x0100, 0x0100, 0x0100}, 100, 0},    // All chips: Red
    {{0x0200, 0x0200, 0x0200}, 100, 0},    // All chips: Green
    {{0x0400, 0x0400, 0x0400}, 100, 0},    // All chips: Blue
    {{0x0300, 0x0300, 0x0300}, 100, 0},    // All chips: Yellow
    {{0x0500, 0x0500, 0x0500}, 100, 0},    // All chips: Magenta
    {{0x0600, 0x0600, 0x0600}, 100, 0},    // All chips: Cyan
    {{0x0700, 0x0700, 0x0700}, 100, 0},    // All chips: White
    
    // All chips with middle LED different color combinations
    {{0x0100, 0x0200, 0x0100}, 100, 0},    // Red with Green middle
    {{0x0100, 0x0400, 0x0100}, 100, 0},    // Red with Blue middle
    {{0x0200, 0x0100, 0x0200}, 100, 0},    // Green with Red middle
    {{0x0200, 0x0400, 0x0200}, 100, 0},    // Green with Blue middle
    {{0x0400, 0x0100, 0x0400}, 100, 0},    // Blue with Red middle
    {{0x0400, 0x0200, 0x0400}, 100, 0},    // Blue with Green middle
    {{0x0300, 0x0400, 0x0300}, 100, 0},    // Yellow with Blue middle
    {{0x0300, 0x0500, 0x0300}, 100, 0},    // Yellow with Magenta middle
    {{0x0500, 0x0200, 0x0500}, 100, 0},    // Magenta with Green middle
    {{0x0500, 0x0600, 0x0500}, 100, 0},    // Magenta with Cyan middle
    {{0x0600, 0x0100, 0x0600}, 100, 0},    // Cyan with Red middle
    {{0x0600, 0x0500, 0x0600}, 100, 0},    // Cyan with Magenta middle
    {{0x0700, 0x0100, 0x0700}, 100, 0},    // White with Red middle
    {{0x0700, 0x0200, 0x0700}, 100, 0},    // White with Green middle
    {{0x0700, 0x0400, 0x0700}, 100, 0}     // White with Blue middle
};

// Sequence data structure (compile-time initialized)
const LEDSequence PBSeq_RGBColorCycle = {
    PBSeq_RGBColorCycle_Steps,
    sizeof(PBSeq_RGBColorCycle_Steps) / sizeof(PBSeq_RGBColorCycle_Steps[0])
};

const uint16_t PBSeq_RGBColorCycleMask[NUM_LED_CHIPS] = {0x0700, 0x0700, 0x0700};

/*
 * TEMPLATE FOR CREATING NEW SEQUENCES
 * Copy and paste the block below to create a new sequence:
 * 
 * // Static array of steps for YourSequenceName
 * static const stLEDSequence YourSequenceName_Steps[] = {
 *     {{chip0, chip1, chip2}, onMS, offMS},  // Step 1: Description
 *     {{chip0, chip1, chip2}, onMS, offMS},  // Step 2: Description
 *     // Add more steps as needed...
 * };
 * 
 * // Sequence data structure (compile-time initialized)
 * const LEDSequence YourSequenceName = {
 *     YourSequenceName_Steps,
 *     sizeof(YourSequenceName_Steps) / sizeof(YourSequenceName_Steps[0])
 * };
 * 
 * const uint16_t YourSequenceNameMask[NUM_LED_CHIPS] = {chip0mask, chip1mask, chip2mask};
 * 
 * NOTES:
 * - LEDOnBits: Which LEDs to turn on (1=on, 0=off) for each chip
 * - onDurationMS: How long this step lasts (milliseconds)
 * - offDurationMS: Gap before next step (usually 0 for smooth transitions)
 * - Each chip has 16 LEDs (0x0000 to 0xFFFF)
 * - Use hex values: 0x00FF = LEDs 0-7, 0xFF00 = LEDs 8-15, 0xFFFF = all 16 LEDs
 * - activeLEDMask is now set in stOutputOptions when triggering the sequence
 * - The mask constant defines which LEDs the sequence can control
 * - Static arrays eliminate heap allocation and are more efficient for embedded systems
 */

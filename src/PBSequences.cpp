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

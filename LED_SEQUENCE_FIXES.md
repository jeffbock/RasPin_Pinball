# LED Sequence Fixes

## Overview
This document describes the two timing issues identified in the LED sequence management system and the fixes applied to resolve them.

## Issues Fixed

### 1. State Save Timing Issue
**Location**: `ProcessLEDSequenceMessage()` function in `Pinball.cpp`

**Problem**: 
- When starting a sequence, the code read the hardware state before ensuring all pending staged changes were sent to hardware
- The original order was:
  1. Set activeLEDMask
  2. Call `SendAllStagedLED()` 
  3. Read and save hardware state (could be stale if changes were staged after step 2)

**Fix**:
Reordered operations to ensure hardware state is flushed before reading:
```cpp
// NEW ORDER:
1. Set activeLEDMask
2. Call SendAllStagedLED()        // Flush all pending changes FIRST
3. Read and save hardware state   // Now guaranteed to be actual HW state
4. Clear active LEDs and stage to OFF
```

**Impact**: Ensures saved LED state accurately reflects the actual hardware state before sequence starts.

---

### 2. Premature State Restoration
**Location**: `EndLEDSequence()` function in `Pinball.cpp`

**Problem**: 
- When ending a sequence, the code restored the previous LED state without first sending the sequence's final staged values
- This meant the final sequence step might never reach the hardware

**Fix**:
Added `SendAllStagedLED()` call at the beginning of `EndLEDSequence()`:
```cpp
void EndLEDSequence() {
    g_PBEngine.m_LEDSequenceInfo.sequenceEnabled = false;
    
    // Send sequence's final state to hardware FIRST
    SendAllStagedLED();
    
    // Then restore previous LED state
    // ... restoration code ...
    
    // Send restored state to hardware
    SendAllStagedLED();
}
```

**Impact**: Ensures the sequence completes properly with its final state sent to hardware before restoration begins.

---

## Testing Recommendations

### Test Case 1: State Save Accuracy
```cpp
// Set LEDs to a specific state
SendRGBMsg(IDO_LED2, IDO_LED3, IDO_LED4, PB_LEDBLUE, PB_ON, false);
// Start sequence on those LEDs
SendSeqMsg(&PBSeq_RGBColorCycle, PBSeq_RGBColorCycleMask, PB_LOOP, PB_ON);
// Stop sequence
SendSeqMsg(&PBSeq_RGBColorCycle, PBSeq_RGBColorCycleMask, PB_LOOP, PB_OFF);
// Verify: LEDs return to blue state (not some intermediate state)
```

### Test Case 2: Sequence Completion
```cpp
// Start a sequence that has a distinct final step
SendSeqMsg(&PBSeq_AllChipsTest, PBSeq_AllChipsTestMask, PB_NOLOOP, PB_ON);
// Wait for sequence to complete
// Verify: Final step of sequence is visible before state restoration
```

## Summary

Both fixes ensure proper timing and ordering of hardware updates:
1. State is saved AFTER flushing pending changes
2. Sequence completes (final state sent) BEFORE restoration begins

These changes are minimal and focused on ensuring the correct order of operations without adding any multi-threading constructs (no mutexes, atomic operations, etc.).

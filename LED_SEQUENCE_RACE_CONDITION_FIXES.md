# LED Sequence Race Condition Fixes

## Overview
This document describes the race conditions identified in the LED sequence management system and the fixes applied to resolve them. These issues could cause partially incorrect LED states during sequence start/stop transitions and when processing deferred messages.

## Race Conditions Identified and Fixed

### 1. State Save Timing Issue (CRITICAL)
**Location**: `ProcessLEDSequenceMessage()` function in `Pinball.cpp`

**Problem**: 
- When starting a sequence, the code called `SendAllStagedLED()` to flush staged values
- Then it immediately read the hardware state with `ReadLEDControl(CurrentHW, ...)`
- However, between these two operations, new LED messages could be staged but not sent
- This meant the saved state might not reflect the actual hardware state
- Result: Incorrect state restoration when sequence ends

**Fix**:
```cpp
// OLD ORDER (INCORRECT):
1. Set activeLEDMask
2. SendAllStagedLED()  
3. Read and save HW state
4. Stage LEDs to OFF
5. Enable sequence

// NEW ORDER (CORRECT):
1. Clear stale deferred messages
2. Set activeLEDMask
3. SendAllStagedLED()        // Flush all pending changes
4. Read and save HW state    // Now guaranteed to be actual HW state
5. Stage LEDs to OFF
6. Enable sequence           // Enable LAST to prevent race
```

**Impact**: Ensures saved LED state accurately reflects hardware state before sequence starts.

---

### 2. Missing Mutex Protection on Deferred Queue (CRITICAL)
**Location**: Multiple functions in `Pinball.cpp`

**Problem**:
- The `m_deferredQueue` had a mutex declared (`m_deferredQMutex`) but it wasn't being used
- Multiple threads could access the queue simultaneously:
  - Main output thread adding deferred messages
  - Processing thread reading deferred messages
- Result: Queue corruption, lost messages, or crashes

**Fix**:
```cpp
// Added mutex protection in three locations:

// 1. When deferring messages (ProcessLEDOutputMessage)
std::lock_guard<std::mutex> lock(g_PBEngine.m_deferredQMutex);
if (g_PBEngine.m_deferredQueue.size() < MAX_DEFERRED_LED_QUEUE) {
    g_PBEngine.m_deferredQueue.push(message);
}

// 2. When processing deferred messages (ProcessDeferredLEDQueue)
std::lock_guard<std::mutex> lock(g_PBEngine.m_deferredQMutex);
while (!g_PBEngine.m_deferredQueue.empty()) {
    // ... process messages
}

// 3. When clearing deferred messages (ProcessLEDSequenceMessage)
std::lock_guard<std::mutex> lock(g_PBEngine.m_deferredQMutex);
while (!g_PBEngine.m_deferredQueue.empty()) {
    g_PBEngine.m_deferredQueue.pop();
}
```

**Impact**: Prevents queue corruption and ensures thread-safe access to deferred messages.

**Additional Improvements**:
- Queue clearing optimized using swap() for O(1) performance instead of O(n) loop
- Added warning logging when deferred queue is full and messages are dropped
- Static counter tracks dropped messages and logs every 100th occurrence to avoid console spam

---

### 3. State Restoration Before Final Sequence State Sent (MAJOR)
**Location**: `EndLEDSequence()` function in `Pinball.cpp`

**Problem**:
- When sequence ended, it immediately disabled the sequence flag
- Then it restored saved LED state
- Then it sent staged values to hardware
- However, the final sequence step might have staged values that weren't sent yet
- Result: Final sequence state never reached hardware before restoration

**Fix**:
```cpp
void EndLEDSequence() {
    // 1. Disable sequence FIRST (prevents new deferrals)
    g_PBEngine.m_LEDSequenceInfo.sequenceEnabled = false;
    
    // 2. Send sequence's final staged state to HW
    SendAllStagedLED();
    
    // 3. Stage restored values
    // ... restore saved state ...
    
    // 4. Send restored state to HW
    SendAllStagedLED();
}
```

**Impact**: Ensures sequence completes properly before state restoration begins.

---

### 4. Stale Deferred Messages from Previous Sequence (MAJOR)
**Location**: `ProcessLEDSequenceMessage()` function in `Pinball.cpp`

**Problem**:
- When starting a new sequence, old deferred messages from a previous sequence might still be in the queue
- These stale messages would be processed when the new sequence ended
- Result: Unexpected LED behavior from old messages being applied

**Fix**:
```cpp
// At start of new sequence, clear all stale deferred messages
{
    std::lock_guard<std::mutex> lock(g_PBEngine.m_deferredQMutex);
    while (!g_PBEngine.m_deferredQueue.empty()) {
        g_PBEngine.m_deferredQueue.pop();
    }
}
```

**Impact**: Each sequence starts with a clean slate, no interference from previous sequences.

---

### 5. Sequence Enable Flag Check Race (MINOR)
**Location**: `PBProcessOutput()` function in `Pinball.cpp`

**Problem**:
- The code checked `sequenceEnabled` flag directly in if statement
- Flag could change between check and action
- Result: Could process wrong branch (sequence vs deferred)

**Fix**:
```cpp
// Read flag once atomically
bool sequenceCurrentlyEnabled = g_PBEngine.m_LEDSequenceInfo.sequenceEnabled;

if (sequenceCurrentlyEnabled) {
    ProcessActiveLEDSequence();
} else {
    ProcessDeferredLEDQueue();
}
```

**Impact**: Ensures consistent processing based on sequence state at time of check.

---

### 6. Thread-Safe Atomic Flag Operations (CRITICAL)
**Location**: Multiple files - header and implementation

**Problem**:
- The `sequenceEnabled` flag was a plain `bool` without synchronization
- Multiple threads could read/write this flag simultaneously
- Compiler optimizations or CPU caching could cause threads to see stale values
- Result: Thread checking flag might not see recent enable/disable changes

**Fix**:
```cpp
// Changed flag type in header:
std::atomic<bool> sequenceEnabled;

// All writes use release memory order:
g_PBEngine.m_LEDSequenceInfo.sequenceEnabled.store(true, std::memory_order_release);
g_PBEngine.m_LEDSequenceInfo.sequenceEnabled.store(false, std::memory_order_release);

// All reads use acquire memory order:
bool enabled = g_PBEngine.m_LEDSequenceInfo.sequenceEnabled.load(std::memory_order_acquire);
```

**Memory Ordering Explanation**:
- `memory_order_release`: When writing, ensures all previous writes in this thread are visible to other threads before the flag change
- `memory_order_acquire`: When reading, ensures we see all writes that happened before the flag was set
- This creates a happens-before relationship that prevents data races

**Impact**: Guarantees that when a thread sees the sequence enabled/disabled, it also sees all the related state changes (masks, saved values, etc.)

---

## Testing Recommendations

### Test Case 1: Rapid Sequence Start/Stop
```cpp
// Start sequence
SendSeqMsg(&PBSeq_RGBColorCycle, PBSeq_RGBColorCycleMask, PB_LOOP, PB_ON);
// Immediately stop
SendSeqMsg(&PBSeq_RGBColorCycle, PBSeq_RGBColorCycleMask, PB_LOOP, PB_OFF);
// Verify: LEDs return to pre-sequence state
```

### Test Case 2: LED Messages During Sequence
```cpp
// Start sequence
SendSeqMsg(&PBSeq_RGBColorCycle, PBSeq_RGBColorCycleMask, PB_LOOP, PB_ON);
// Send LED messages that should be deferred
SendRGBMsg(IDO_LED2, IDO_LED3, IDO_LED4, PB_LEDRED, PB_ON, false);
// Stop sequence
SendSeqMsg(&PBSeq_RGBColorCycle, PBSeq_RGBColorCycleMask, PB_LOOP, PB_OFF);
// Verify: Deferred message is applied correctly
```

### Test Case 3: Back-to-Back Sequences
```cpp
// Start first sequence
SendSeqMsg(&PBSeq_AllChipsTest, PBSeq_AllChipsTestMask, PB_LOOP, PB_ON);
// Send LED message (should be deferred)
SendRGBMsg(IDO_LED5, IDO_LED6, IDO_LED7, PB_LEDGREEN, PB_ON, false);
// Start second sequence (should clear deferred queue)
SendSeqMsg(&PBSeq_RGBColorCycle, PBSeq_RGBColorCycleMask, PB_LOOP, PB_ON);
// Stop second sequence
SendSeqMsg(&PBSeq_RGBColorCycle, PBSeq_RGBColorCycleMask, PB_LOOP, PB_OFF);
// Verify: First sequence's deferred message is NOT applied
```

### Test Case 4: Pulse Outputs During Sequence
```cpp
// Set LEDs to specific state
SendRGBMsg(IDO_LED2, IDO_LED3, IDO_LED4, PB_LEDBLUE, PB_ON, false);
// Start sequence on those LEDs
SendSeqMsg(&PBSeq_RGBColorCycle, PBSeq_RGBColorCycleMask, PB_LOOP, PB_ON);
// Stop sequence
SendSeqMsg(&PBSeq_RGBColorCycle, PBSeq_RGBColorCycleMask, PB_LOOP, PB_OFF);
// Verify: LEDs return to blue state
```

## Summary of Changes

Changes were made to three files:

### `/home/runner/work/RasPin_Pinball/RasPin_Pinball/src/Pinball.cpp`:
1. **ProcessLEDSequenceMessage()**: Reordered operations to save state correctly, added deferred queue clearing with efficient swap, moved `sequenceEnabled = true` to end with release memory order
2. **ProcessLEDOutputMessage()**: Added mutex lock when adding to deferred queue, added logging for dropped messages
3. **EndLEDSequence()**: Added SendAllStagedLED() before restoration, use release memory order for flag
4. **ProcessDeferredLEDQueue()**: Added mutex lock when processing queue
5. **PBProcessOutput()**: Made sequenceEnabled check atomic with acquire memory order

### `/home/runner/work/RasPin_Pinball/RasPin_Pinball/src/Pinball_Engine.h`:
1. Changed `sequenceEnabled` from `bool` to `std::atomic<bool>` for thread-safe access

### `/home/runner/work/RasPin_Pinball/RasPin_Pinball/src/Pinball_Engine.cpp`:
1. Added explicit initialization of `m_LEDSequenceInfo` in PBEngine constructor with proper atomic initialization

## Benefits

- **Correctness**: LED state is now accurately saved and restored
- **Thread Safety**: Deferred queue is protected from concurrent access
- **Reliability**: Sequences complete properly before state changes
- **Predictability**: No stale messages affect new sequences
- **Maintainability**: Clear comments explain critical timing requirements

## Backward Compatibility

All changes are internal to the LED sequence processing logic. The public API remains unchanged:
- `SendSeqMsg()` function signature unchanged
- `SendRGBMsg()` function signature unchanged  
- Sequence definition structures unchanged
- Existing sequences continue to work as expected

No changes required to existing code that uses LED sequences.

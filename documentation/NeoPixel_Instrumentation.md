# NeoPixel Timing Instrumentation

## Overview

The NeoPixel timing instrumentation feature provides detailed diagnostics for WS2812B LED strip communication. It captures high-resolution timing data for each bit transmitted, allowing developers to verify that timing requirements are being met and diagnose any timing issues.

## Purpose

WS2812B LEDs require precise timing for reliable operation:
- **Bit 1**: 800ns high, 450ns low (±150ns tolerance)
- **Bit 0**: 400ns high, 850ns low (±150ns tolerance)

This instrumentation helps:
1. Verify timing accuracy when using `clock_gettime()` method
2. Diagnose timing issues caused by system load or scheduling
3. Determine if real-time priority is needed
4. Validate that the NeoPixel communication is working correctly

## Data Structures

### `stNeoPixelBitTiming`
Tracks timing data for a single bit transmission:
```cpp
struct stNeoPixelBitTiming {
    uint32_t highTimeNs;      // Time pin was high in nanoseconds
    uint32_t lowTimeNs;       // Time pin was low in nanoseconds
    bool meetsSpec;           // True if timing meets WS2812B specification
    bool bitValue;            // The actual bit value sent (0 or 1)
};
```

### `stNeoPixelInstrumentation`
Complete instrumentation data for up to 32 bits (4 bytes):
```cpp
struct stNeoPixelInstrumentation {
    stNeoPixelBitTiming bitTimings[NEOPIXEL_INSTRUMENTATION_BITS];  // 32 bits
    unsigned int numBitsCaptured;     // Number of bits captured (0-32)
    uint32_t capturedBytes;           // The actual byte values captured
    bool instrumentationEnabled;      // Flag to enable/disable instrumentation
    unsigned int byteSequenceNumber;  // Sequential byte counter
    uint64_t totalTransmissionTimeNs; // Total time for all captured bits
};
```

## API Methods

### `void EnableInstrumentation(bool enable)`
Enable or disable timing instrumentation.
- When enabled, resets all instrumentation data
- When disabled, stops capturing timing data
- **Note**: Instrumentation adds overhead and should only be enabled for diagnostics

### `bool IsInstrumentationEnabled() const`
Check if instrumentation is currently enabled.

### `void ResetInstrumentation()`
Clear all captured instrumentation data and reset counters.

### `const stNeoPixelInstrumentation& GetInstrumentationData() const`
Retrieve the current instrumentation data for analysis.

## Usage Example

```cpp
#ifdef EXE_MODE_RASPI

// Assuming driver is a NeoPixelDriver instance
NeoPixelDriver& driver = g_PBEngine.m_NeoPixelDriverMap.at(0);

// 1. Enable instrumentation
driver.EnableInstrumentation(true);

// 2. Stage data to send (first 32 bits will be captured)
driver.StageNeoPixel(0, 255, 128, 64);  // Red, Green, Blue

// 3. Send the data (timing is captured during transmission)
driver.SendStagedNeoPixels();

// 4. Retrieve and analyze the data
const stNeoPixelInstrumentation& data = driver.GetInstrumentationData();

printf("Bits captured: %u\n", data.numBitsCaptured);
printf("Total time: %llu ns\n", data.totalTransmissionTimeNs);

// Check each bit
for (unsigned int i = 0; i < data.numBitsCaptured; i++) {
    const stNeoPixelBitTiming& bit = data.bitTimings[i];
    printf("Bit %u: value=%d, high=%uns, low=%uns, spec=%s\n",
           i, bit.bitValue, bit.highTimeNs, bit.lowTimeNs,
           bit.meetsSpec ? "PASS" : "FAIL");
}

// 5. Reset for next test
driver.ResetInstrumentation();

// 6. Disable when done
driver.EnableInstrumentation(false);

#endif
```

## How It Works

When instrumentation is enabled:

1. **Bit Transmission**: For each bit sent via `SendByte()`:
   - Captures the start time before setting pin HIGH
   - Captures the end time after the HIGH period
   - Calculates actual HIGH duration in nanoseconds
   - Captures the start time before setting pin LOW
   - Captures the end time after the LOW period
   - Calculates actual LOW duration in nanoseconds

2. **Specification Check**: Each bit timing is compared against WS2812B specifications:
   - For bit 1: HIGH must be 650-950ns, LOW must be 300-600ns
   - For bit 0: HIGH must be 250-550ns, LOW must be 700-1000ns
   - Sets `meetsSpec` flag based on whether timing is within tolerance

3. **Data Capture**: Stores timing data for up to 32 bits:
   - Typically captures first 32 bits of transmission (4 bytes)
   - Useful for capturing timing of a complete 32-bit value or one LED (24 bits)
   - Tracks byte sequence number and total transmission time

4. **Metadata**: Also captures:
   - The actual byte values sent (`capturedBytes`)
   - Sequential byte number (`byteSequenceNumber`)
   - Total time for all captured bits (`totalTransmissionTimeNs`)

## Timing Method Compatibility

**Currently implemented for:**
- ✅ `NEOPIXEL_TIMING_CLOCKGETTIME` - Full instrumentation support

**Not implemented for:**
- ❌ `NEOPIXEL_TIMING_NOP` - No instrumentation (timing is NOP-based, adding timing measurements would affect accuracy)

The instrumentation is specifically designed for the `clock_gettime()` timing method because:
1. This method already uses system time calls
2. Additional timing measurements don't significantly affect accuracy
3. The NOP method requires calibration and timing measurements would interfere

## Performance Impact

**When enabled:**
- Additional `clock_gettime()` calls per bit (4 extra calls)
- Minimal impact since `clock_gettime()` is already used for timing
- Total overhead: ~200-400ns per bit (acceptable for diagnostics)
- Should NOT be used in production - for diagnostics only

**When disabled:**
- Zero overhead - instrumentation code is skipped
- Normal NeoPixel transmission performance

## Interpreting Results

### Successful Timing
```
Bit 0: value=1, high=820ns, low=460ns, spec=PASS
Bit 1: value=0, high=410ns, low=870ns, spec=PASS
```
All bits meet specification - timing is good.

### Failed Timing
```
Bit 0: value=1, high=1200ns, low=600ns, spec=FAIL
```
High time exceeded specification - possible causes:
- System scheduling delay during transmission
- High CPU load
- Not running with real-time priority
- Consider enabling `NEOPIXEL_USE_RT_PRIORITY`

### Borderline Timing
```
Bit 0: value=1, high=640ns, low=290ns, spec=FAIL
```
Close to specification but outside tolerance - may work but not reliable:
- Try enabling real-time priority
- Reduce system load
- Consider switching to NOP-based timing

## Integration with Diagnostic System

The instrumentation can be integrated into the engine's diagnostic/sandbox mode:

```cpp
// In Pinball_Engine.cpp, add to diagnostic menu or sandbox mode
void PBEngine::RunNeoPixelDiagnostics() {
    if (m_NeoPixelDriverMap.find(0) != m_NeoPixelDriverMap.end()) {
        NeoPixelDriver& driver = m_NeoPixelDriverMap.at(0);
        
        driver.EnableInstrumentation(true);
        
        // Test pattern
        driver.StageNeoPixel(0, 255, 0, 0);  // Red
        driver.SendStagedNeoPixels();
        
        // Analyze and display results
        const stNeoPixelInstrumentation& data = driver.GetInstrumentationData();
        
        // Display on screen or log to file
        DisplayInstrumentationResults(data);
        
        driver.EnableInstrumentation(false);
    }
}
```

## Limitations

1. **Captures only first 32 bits**: If sending multiple LEDs, only the first 32 bits (from first ~1.33 LEDs) are captured
2. **Not for production use**: Instrumentation should be disabled in production code
3. **Clock-based method only**: Only works with `NEOPIXEL_TIMING_CLOCKGETTIME`
4. **Single transmission**: Data is overwritten on each `SendStagedNeoPixels()` call

## Troubleshooting

**No bits captured:**
- Check that `EnableInstrumentation(true)` was called
- Verify `SendStagedNeoPixels()` was called after staging data

**All bits fail specification:**
- System may be under heavy load
- Try enabling real-time priority with `NEOPIXEL_USE_RT_PRIORITY`
- Consider running with `sudo` for RT priority

**Inconsistent results:**
- Normal variation due to scheduling
- Run multiple tests to see patterns
- Use real-time priority for more consistent results

## Future Enhancements

Possible improvements:
1. Configurable capture size (more or fewer than 32 bits)
2. Continuous monitoring mode with statistics
3. Export to file for analysis
4. Graphical timing display in diagnostics menu
5. Automatic detection of timing issues

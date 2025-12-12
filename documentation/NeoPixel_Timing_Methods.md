# NeoPixel Timing Methods

## Overview

The NeoPixel driver supports multiple timing methods for WS2812B LED communication. Each method has different characteristics, advantages, and requirements. This document explains each method and helps you choose the best one for your application.

## Timing Methods

### 1. NEOPIXEL_TIMING_CLOCKGETTIME (Default)

**Description**: Uses `clock_gettime()` system calls for precise timing delays.

**How it works**:
- Calls `clock_gettime(CLOCK_MONOTONIC)` to measure time
- Busy-waits in loops checking elapsed time
- Switches GPIO pin high/low based on timing

**Advantages**:
- ✅ Portable - works on any Raspberry Pi model
- ✅ No hardware dependencies
- ✅ CPU frequency independent
- ✅ Supports instrumentation for diagnostics

**Disadvantages**:
- ❌ System call overhead (~200-400ns per call)
- ❌ Affected by system load and scheduling
- ❌ May require real-time priority for consistency
- ❌ Less reliable under high CPU load

**When to use**:
- Default method for most applications
- When you need timing diagnostics
- When SPI/PWM hardware is unavailable
- For prototyping and development

**Configuration**:
```cpp
driver.SetTimingMethod(NEOPIXEL_TIMING_CLOCKGETTIME);
```

---

### 2. NEOPIXEL_TIMING_NOP

**Description**: Uses assembly NOP instructions for timing delays.

**How it works**:
- Executes calibrated NOP (no-operation) instruction loops
- Timing based on CPU clock cycles
- More deterministic than system calls

**Advantages**:
- ✅ More deterministic than clock_gettime
- ✅ No system call overhead
- ✅ Faster execution

**Disadvantages**:
- ❌ CPU frequency dependent (requires calibration)
- ❌ Calibrated for specific CPU (currently Pi 5 @ 2.4GHz)
- ❌ May need adjustment for different Pi models
- ❌ No instrumentation support

**When to use**:
- When you need more consistent timing
- On known hardware (Raspberry Pi 5)
- When system load is affecting clock_gettime
- For production deployments on specific hardware

**Configuration**:
```cpp
driver.SetTimingMethod(NEOPIXEL_TIMING_NOP);
```

**Calibration Notes**:
- Default calibration is for Raspberry Pi 5 @ 2.4GHz
- May need oscilloscope verification for other Pi models
- Adjust NOP counts in SendByte() for your CPU frequency

---

### 3. NEOPIXEL_TIMING_SPI ⭐ (Recommended)

**Description**: Uses SPI hardware to generate WS2812B timing signals.

**How it works**:
- Automatically detects if output pin is SPI-capable (GPIO 10 or 20)
- Configures SPI at 2.4 MHz (3x WS2812B 800kHz rate)
- Encodes each WS2812B bit as 3 SPI bits:
  - Bit 1: `110` pattern (high for ~0.8µs, low for ~0.4µs)
  - Bit 0: `100` pattern (high for ~0.4µs, low for ~0.8µs)
- SPI hardware handles all timing automatically
- Uses MOSI (Master Out, Slave In) pin to send data TO NeoPixels

**Advantages**:
- ✅ **Most reliable** - hardware-based timing
- ✅ Zero CPU overhead during transmission
- ✅ Not affected by system load or scheduling
- ✅ Consistent timing regardless of conditions
- ✅ No calibration needed
- ✅ Automatic pin validation and channel selection

**Disadvantages**:
- ❌ Requires SPI-capable pin (GPIO 10 for SPI0 or GPIO 20 for SPI1)
- ❌ Automatically falls back to clock_gettime if pin is not SPI-capable
- ❌ 3x bandwidth requirement (24 SPI bits per 8 WS2812B bits)
- ❌ No instrumentation support

**When to use**:
- **Best choice for production** deployments
- When reliability is critical
- When you have SPI pins available (GPIO 10 or 20)
- For long LED chains
- When system is under variable load

**Configuration**:
```cpp
// The driver automatically detects SPI capability based on output pin
driver.SetTimingMethod(NEOPIXEL_TIMING_SPI);

// Pin configuration in g_outputDef must use SPI MOSI pin:
// - SPI0 MOSI: GPIO 10 (Physical Pin 19)
// - SPI1 MOSI: GPIO 20 (Physical Pin 38)
// If pin is not SPI-capable, automatically falls back to NEOPIXEL_TIMING_CLOCKGETTIME
```

**Why MOSI (not MISO)?**
- MOSI = Master Out, Slave In (Pi sends data TO NeoPixels)
- MISO = Master In, Slave Out (Pi receives data FROM devices)
- NeoPixels only receive data, they don't send data back
- Therefore, we use the MOSI pin for one-way communication

**Hardware Requirements**:
- SPI-capable GPIO pin (MOSI)
- SPI must be enabled in `/boot/config.txt`:
OR
- Use the Raspberry Pi Configuration tool (under Preferences) to enable SPI

  ```
  dtparam=spi=on
  ```

---

### 4. NEOPIXEL_TIMING_PWM 🚧 (Experimental)

**Description**: Uses hardware PWM with duty cycle control for timing.

**How it works**:
- Automatically detects if output pin is PWM-capable (GPIO 12, 13, 18, or 19)
- Configures PWM at 800kHz base frequency
- Uses duty cycle to control high/low times:
  - Bit 1: 70% duty cycle (~0.875µs high, 0.375µs low)
  - Bit 0: 30% duty cycle (~0.375µs high, 0.875µs low)
- **Current implementation**: Uses software delays between PWM writes
- **Future enhancement**: DMA-based queueing for zero-CPU operation

**Advantages**:
- ✅ Hardware PWM timing control
- ✅ Precise duty cycle generation
- ✅ Not affected by system load
- ✅ Automatic pin validation and channel selection
- ✅ Suitable for experimentation

**Disadvantages**:
- ⚠️ **Current limitation**: Software delays reduce performance benefit
- ❌ Requires PWM-capable GPIO pin (12, 13, 18, or 19)
- ❌ Automatically falls back to clock_gettime if pin is not PWM-capable
- ❌ May conflict with other PWM uses (e.g., audio)
- ❌ No instrumentation support
- 🚧 DMA implementation needed for full performance

**When to use**:
- Experimental and development purposes
- When testing PWM-based approaches
- When you have PWM-capable pins available (GPIO 12, 13, 18, or 19)
- When you plan to implement DMA queueing

**Configuration**:
```cpp
// The driver automatically detects PWM capability based on output pin
driver.SetTimingMethod(NEOPIXEL_TIMING_PWM);

// Pin configuration in g_outputDef must use PWM-capable pin:
// - PWM0: GPIO 12 (Physical Pin 32) or GPIO 18 (Physical Pin 12)
// - PWM1: GPIO 13 (Physical Pin 33) or GPIO 19 (Physical Pin 35)
// If pin is not PWM-capable, automatically falls back to NEOPIXEL_TIMING_CLOCKGETTIME
```

**Future Improvements**:
The current implementation uses `delayMicroseconds()` between bit writes, which limits performance. A full DMA implementation would:
- Queue all bit patterns in memory
- Use DMA to transfer patterns to PWM controller
- Achieve true zero-CPU transmission
- Support high frame rates and multiple chains

**Configuration**:
```cpp
driver.SetTimingMethod(NEOPIXEL_TIMING_PWM);

// Use PWM-capable GPIO pin:
// - PWM0: GPIO 12 (Pin 32), GPIO 18 (Pin 12)
// - PWM1: GPIO 13 (Pin 33), GPIO 19 (Pin 35)
```

**Hardware Requirements**:
- PWM-capable GPIO pin
- No conflicting PWM usage (e.g., analog audio output)

---

## Comparison Matrix

| Feature | clock_gettime | NOP | SPI ⭐ | PWM 🚧 |
|---------|---------------|-----|---------|---------|
| Reliability | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |
| CPU Usage | High | Medium | Very Low | Medium* |
| Portability | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ |
| System Load Impact | High | Medium | None | Low* |
| Calibration Needed | No | Yes | No | No |
| Hardware Required | None | None | SPI | PWM |
| Instrumentation | ✅ | ❌ | ❌ | ❌ |
| Production Ready | ⚠️ | ⚠️ | ✅ | 🚧 |

**Legend**: ⭐ = Rating, ✅ = Supported, ❌ = Not Supported, ⚠️ = Conditional, 🚧 = Experimental  
***Note**: Current PWM implementation uses software delays; DMA version would achieve "Very Low" CPU usage

---

## Choosing the Right Method

### For Development & Debugging
→ Use **NEOPIXEL_TIMING_CLOCKGETTIME**
- Supports instrumentation for diagnostics
- Easy to debug timing issues
- No hardware setup required

### For Production (SPI Available)
→ Use **NEOPIXEL_TIMING_SPI** ⭐
- Most reliable hardware-based method
- Zero timing issues
- Works under any system load

### For Experimentation
→ Use **NEOPIXEL_TIMING_PWM** 🚧
- Test PWM-based approach
- Hardware timing with duty cycle control
- Foundation for future DMA implementation

### For Specific Hardware Only
→ Use **NEOPIXEL_TIMING_NOP**
- When SPI/PWM unavailable
- Calibrated for your specific Pi model
- Better than clock_gettime under load

---

## Example: Switching Timing Methods

```cpp
#include "Pinball_IO.h"

void setupNeoPixels() {
    // Get NeoPixel driver
    NeoPixelDriver& driver = g_PBEngine.m_NeoPixelDriverMap.at(0);
    
    #ifdef USE_SPI_METHOD
        // Production: Use SPI for reliability (recommended)
        driver.SetTimingMethod(NEOPIXEL_TIMING_SPI);
        printf("Using SPI timing (most reliable)\n");
    #elif defined(USE_PWM_METHOD)
        // Experimental: Use PWM (basic implementation)
        driver.SetTimingMethod(NEOPIXEL_TIMING_PWM);
        printf("Using PWM timing (experimental)\n");
    #elif defined(USE_NOP_METHOD)
        // Calibrated: Use NOP for Pi 5
        driver.SetTimingMethod(NEOPIXEL_TIMING_NOP);
        printf("Using NOP timing (calibrated)\n");
    #else
        // Default: Use clock_gettime with instrumentation
        driver.SetTimingMethod(NEOPIXEL_TIMING_CLOCKGETTIME);
        driver.EnableInstrumentation(true);
        printf("Using clock_gettime with diagnostics\n");
    #endif
}
```

---

## Troubleshooting

### "Flickering or wrong colors with clock_gettime"
- System load too high
- **Solution**: Try SPI or PWM method
- **Alternative**: Enable real-time priority (`NEOPIXEL_USE_RT_PRIORITY`)

### "NOP timing not working on my Pi"
- Calibration is for Pi 5 @ 2.4GHz
- **Solution**: Use SPI/PWM or recalibrate NOP counts
- **Check**: Verify CPU frequency with `cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq`

### "SPI initialization failed"
- SPI not enabled in system
- **Solution**: Enable SPI in `/boot/config.txt`: `dtparam=spi=on`
- **Solution**: Reboot after enabling
- **Verify**: Check `/dev/spidev0.0` exists

### "PWM not working"
- GPIO pin doesn't support PWM
- **Solution**: Use PWM-capable pin (GPIO 12, 13, 18, 19)
- **Check**: PWM not used by other features (e.g., audio)

---

## Hardware Pin Reference

### SPI Pins (Raspberry Pi)
| SPI Channel | MOSI Pin | GPIO | Physical Pin |
|-------------|----------|------|--------------|
| SPI0 | MOSI0 | GPIO 10 | Pin 19 |
| SPI1 | MOSI1 | GPIO 20 | Pin 38 |

### PWM Pins (Raspberry Pi)
| PWM Channel | GPIO Options | Physical Pins |
|-------------|--------------|---------------|
| PWM0 | GPIO 12, GPIO 18 | Pin 32, Pin 12 |
| PWM1 | GPIO 13, GPIO 19 | Pin 33, Pin 35 |

**Note**: Check your specific Raspberry Pi model's pinout for exact pin locations.

---

## Performance Benchmarks

Approximate CPU overhead per LED (3 bytes = 24 bits):

| Method | CPU Time | System Calls | Best For |
|--------|----------|--------------|----------|
| clock_gettime | ~50-80µs | ~96 | Debugging |
| NOP | ~30µs | 0 | Calibrated HW |
| SPI | ~5µs | 1 | Reliability ⭐ |
| PWM (current) | ~35µs | ~24 | Experimental 🚧 |
| PWM (DMA)* | <1µs | 1 (DMA) | Future |

**Test conditions**: Raspberry Pi 5, 10 LEDs, minimal system load  
***Future DMA implementation** would achieve <1µs performance

---

## Additional Resources

- [WS2812B Datasheet](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf)
- [wiringPi Documentation](http://wiringpi.com/)
- [Raspberry Pi SPI Documentation](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#serial-peripheral-interface-spi)
- [Raspberry Pi PWM Documentation](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#pulse-width-modulation)

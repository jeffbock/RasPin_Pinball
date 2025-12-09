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
- Configures SPI at 2.4 MHz (3x WS2812B 800kHz rate)
- Encodes each WS2812B bit as 3 SPI bits:
  - Bit 1: `110` pattern (high for ~0.8µs, low for ~0.4µs)
  - Bit 0: `100` pattern (high for ~0.4µs, low for ~0.8µs)
- SPI hardware handles all timing automatically

**Advantages**:
- ✅ **Most reliable** - hardware-based timing
- ✅ Zero CPU overhead during transmission
- ✅ Not affected by system load or scheduling
- ✅ Consistent timing regardless of conditions
- ✅ No calibration needed

**Disadvantages**:
- ❌ Requires SPI channel (SPI0 or SPI1)
- ❌ Pin must be SPI-capable (check Pi pinout)
- ❌ 3x bandwidth requirement (24 SPI bits per 8 WS2812B bits)
- ❌ No instrumentation support

**When to use**:
- **Best choice for production** deployments
- When reliability is critical
- When you have SPI pins available
- For long LED chains
- When system is under variable load

**Configuration**:
```cpp
// Use SPI channel 0 (default)
driver.SetTimingMethod(NEOPIXEL_TIMING_SPI);

// Connect NeoPixel data line to SPI MOSI pin:
// - SPI0: GPIO 10 (Pin 19)
// - SPI1: GPIO 20 (Pin 38)
```

**Hardware Requirements**:
- SPI-capable GPIO pin (MOSI)
- SPI must be enabled in `/boot/config.txt`:
  ```
  dtparam=spi=on
  ```

---

### 4. NEOPIXEL_TIMING_PWM 🚀 (Best Performance)

**Description**: Uses hardware PWM with DMA for precise timing control.

**How it works**:
- Configures PWM at 800kHz base frequency
- Uses duty cycle to control high/low times:
  - Bit 1: 70% duty cycle (~0.8µs high, 0.45µs low)
  - Bit 0: 30% duty cycle (~0.4µs high, 0.85µs low)
- DMA transfers data to PWM controller

**Advantages**:
- ✅ **Best performance** - lowest CPU usage
- ✅ DMA-based - CPU can do other work
- ✅ Precise hardware timing
- ✅ Not affected by system load
- ✅ Suitable for high frame rates

**Disadvantages**:
- ❌ Requires PWM-capable GPIO pin
- ❌ Complex setup and configuration
- ❌ May conflict with other PWM uses (e.g., audio)
- ❌ Pin-specific (only certain GPIOs support PWM)
- ❌ No instrumentation support

**When to use**:
- High-performance applications
- Real-time LED animations
- When CPU resources are limited
- Multiple simultaneous LED chains

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

| Feature | clock_gettime | NOP | SPI ⭐ | PWM 🚀 |
|---------|---------------|-----|---------|---------|
| Reliability | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| CPU Usage | High | Medium | Very Low | Lowest |
| Portability | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ |
| System Load Impact | High | Medium | None | None |
| Calibration Needed | No | Yes | No | No |
| Hardware Required | None | None | SPI | PWM |
| Instrumentation | ✅ | ❌ | ❌ | ❌ |
| Production Ready | ⚠️ | ⚠️ | ✅ | ✅ |

**Legend**: ⭐ = Rating, ✅ = Supported, ❌ = Not Supported, ⚠️ = Conditional

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

### For High Performance
→ Use **NEOPIXEL_TIMING_PWM** 🚀
- Lowest CPU usage
- DMA-based for best performance
- Suitable for complex animations

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
        // Production: Use SPI for reliability
        driver.SetTimingMethod(NEOPIXEL_TIMING_SPI);
        printf("Using SPI timing (most reliable)\n");
    #elif defined(USE_PWM_METHOD)
        // High performance: Use PWM with DMA
        driver.SetTimingMethod(NEOPIXEL_TIMING_PWM);
        printf("Using PWM timing (best performance)\n");
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
| SPI | ~5µs | 1 | Reliability |
| PWM | <1µs | 1 (DMA) | Performance |

**Test conditions**: Raspberry Pi 5, 10 LEDs, minimal system load

---

## Additional Resources

- [WS2812B Datasheet](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf)
- [wiringPi Documentation](http://wiringpi.com/)
- [Raspberry Pi SPI Documentation](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#serial-peripheral-interface-spi)
- [Raspberry Pi PWM Documentation](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#pulse-width-modulation)

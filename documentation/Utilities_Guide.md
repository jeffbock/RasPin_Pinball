# RasPin Utilities Guide

## Overview

The RasPin framework includes several command-line utilities to help with font generation, hardware diagnostics, and amplifier control. This guide covers all available utilities, their purposes, and how to use them.

---

## Utility Summary

| Utility | Platform Support | Description |
|---------|------------------|-------------|
| **FontGen** | Windows & Raspberry Pi | Converts TrueType fonts to texture atlases for text rendering |
| **pblistdevices** | Raspberry Pi only | Scans I2C bus and lists all connected hardware devices |
| **pbsetamp** | Raspberry Pi only | Controls MAX9744 amplifier volume settings |

---

# FontGen - Font Generation Utility

**Platform:** Windows & Raspberry Pi

**Purpose:** Converts TrueType (.ttf) font files into texture atlases and UV mapping files for use in the RasPin Pinball engine.

## What FontGen Creates

FontGen takes a TrueType font file and produces:

1. **PNG Texture File** - A texture atlas containing all printable ASCII characters (space through tilde, characters 32-126)
2. **JSON Mapping File** - UV coordinates and dimensions for each character in the texture

### File Naming Convention

Both output files use the pattern: `<font_name>_<font_size>_<texture_size>.<extension>`

**Example:**
```
Input:  Baldur.ttf
Output: Baldur_96_768.png    (texture atlas image)
        Baldur_96_768.json   (character UV mapping)
```

## Building FontGen

FontGen is built separately from the main pinball engine.

### Windows Build

**VS Code Task:** `Windows: FontGen Build`

Or manually:
```bash
cl.exe /Zi /EHsc /nologo ^
  /Fewinbuild/FontGen.exe ^
  src/3rdparty/stb_truetype.cpp ^
  src/3rdparty/stb_image_write.cpp ^
  src/PButils/FontGen.cpp
```

**Output:** `winbuild/FontGen.exe`

### Raspberry Pi Build

**VS Code Task:** `Raspberry Pi: FontGen Build`

Or manually:
```bash
g++ -g -o raspibuild/FontGen \
  src/3rdparty/stb_truetype.cpp \
  src/3rdparty/stb_image_write.cpp \
  src/PButils/FontGen.cpp \
  -lpthread
```

**Output:** `raspibuild/FontGen`

## Using FontGen

### Command Syntax

```bash
FontGen <font_file.ttf> <font_size> [<buffer_size>] [<V_pixel_bump>]
```

### Parameters

| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `font_file.ttf` | Yes | - | Path to TrueType font file |
| `font_size` | Yes | - | Font size in pixels (height) |
| `buffer_size` | No | 256 | Texture atlas size (256, 512, 768, 1024, etc.) |
| `V_pixel_bump` | No | 2 | Vertical UV adjustment in pixels to prevent clipping |

### Examples

**Basic Usage - Default 256x256 Texture:**
```bash
FontGen Arial.ttf 24
# Creates: Arial_24_256.png
#          Arial_24_256.json
```

**Larger Font with 512x512 Texture:**
```bash
FontGen Baldur.ttf 60 512
# Creates: Baldur_60_512.png
#          Baldur_60_512.json
```

**Large Display Font with 768x768 Texture:**
```bash
FontGen Baldur.ttf 96 768
# Creates: Baldur_96_768.png
#          Baldur_96_768.json
```

### Choosing Parameters

**Font Size:**
- **24-32px**: Small UI text, debug info, console
- **48-60px**: Medium text, menus, game messages
- **72-96px**: Large text, titles, scores
- **100+px**: Extra large display text, attract mode

**Buffer Size:**
- **256**: Sufficient for small fonts (24-32px)
- **512**: Good for medium fonts (48-60px)
- **768**: Needed for large fonts (72-96px)
- **1024**: Very large fonts or extensive character sets

**Rule of Thumb:** Buffer size should be ~8-10 times the font size

## Using Generated Fonts in the Engine

### Load Font in Code

Use `gfxLoadSprite()` with `GFX_TEXTMAP` type:

```cpp
unsigned int myFontId = gfxLoadSprite(
    "MyCustomFont",                              // Sprite name
    "src/resources/fonts/MyFont_48_512.png",    // PNG path
    GFX_PNG,                                     // Texture type
    GFX_TEXTMAP,                                 // MAP TYPE - this is key!
    GFX_UPPERLEFT,                               // Alignment
    true,                                        // Keep resident
    true                                         // Use texture
);
```

### Render Text

```cpp
// Set color and size
gfxSetColor(myFontId, 255, 255, 255, 255);      // White
gfxSetScaleFactor(myFontId, 1.5, false);        // 150% size

// Render string
gfxRenderString(myFontId, "HELLO WORLD", 
               100, 200,        // x, y position
               2,               // character spacing
               GFX_TEXTCENTER); // justification
```

---

# pblistdevices - I2C Device Scanner

**Platform:** Raspberry Pi only (requires real hardware)

**Purpose:** Scans the I2C bus and lists all connected RasPin hardware devices including IO expanders, LED drivers, and amplifiers.

## Building pblistdevices

### Raspberry Pi Build

**VS Code Task:** `Raspberry Pi: pblistdevices Build`

Or manually:
```bash
g++ -g -o raspibuild/pblistdevices \
  src/PButils/pblistdevices.cpp \
  -lwiringPi \
  -lpthread
```

**Output:** `raspibuild/pblistdevices`

## Using pblistdevices

### Command Syntax

```bash
./pblistdevices
```

No arguments required - simply run the utility.

### Example Output

```
Scanning I2C bus for devices...
================================

Amplifier (MAX9744) addresses:
0x4b - Amplifier: Active
0x4c - Amplifier: Empty
0x4d - Amplifier: Empty

LED Expander (TLC59116) addresses:
0x60 - LED Expander: Active
0x61 - LED Expander: Active
0x62 - LED Expander: Active
0x63 - LED Expander: Empty
...
0x68 - LED Expander: Skipped (All-Call address)
...

IO Expander (TCA9555) addresses:
0x20 - IO Expander: Active
0x21 - IO Expander: Active
0x22 - IO Expander: Active
0x23 - IO Expander: Empty
...

Summary:
--------
Amplifiers: 1 (0x4b)
LED Expanders: 3 (0x60, 0x61, 0x62)
IO Expanders: 3 (0x20, 0x21, 0x22)
```

## Device Address Ranges

The utility scans the following I2C address ranges:

| Device Type | Address Range | Count |
|-------------|---------------|-------|
| **MAX9744 Amplifier** | 0x4B - 0x4D | 3 possible addresses |
| **TLC59116 LED Driver** | 0x60 - 0x6F | 16 possible addresses (0x68 skipped) |
| **TCA9555 IO Expander** | 0x20 - 0x27 | 8 possible addresses |

## Use Cases

- **Initial Hardware Setup:** Verify all devices are connected and responding
- **Troubleshooting:** Identify missing or non-responsive devices
- **Address Verification:** Confirm I2C addresses match your code configuration
- **Hardware Debugging:** Detect I2C communication issues

---

# pbsetamp - Amplifier Volume Control

**Platform:** Raspberry Pi only (requires real hardware)

**Purpose:** Controls the volume level of MAX9744 amplifiers via I2C commands.

## Building pbsetamp

### Raspberry Pi Build

**VS Code Task:** `Raspberry Pi: pbsetamp Build`

Or manually:
```bash
g++ -g -o raspibuild/pbsetamp \
  src/PButils/pbsetamp.cpp \
  -lwiringPi \
  -lpthread
```

**Output:** `raspibuild/pbsetamp`

## Using pbsetamp

### Command Syntax

```bash
pbsetamp [--address <addr>] <volume>
```

### Parameters

| Parameter | Required | Description |
|-----------|----------|-------------|
| `--address <addr>` | No | I2C address: 0x4B (default), 0x4C, or 0x4D |
| `<volume>` | Yes | Volume level: 0-63 (0 = mute, 63 = max) |

### Examples

**Set Volume Using Default Address (0x4B):**
```bash
pbsetamp 32
# Output: Successfully set amplifier volume to: 32 (0x20) at address 0x4b
```

**Set Volume with Specific Address:**
```bash
pbsetamp --address 0x4C 45
# Output: Successfully set amplifier volume to: 45 (0x2d) at address 0x4c
```

**Mute Amplifier:**
```bash
pbsetamp 0
# Output: Successfully set amplifier volume to: 0 (0x00) at address 0x4b
```

**Maximum Volume:**
```bash
pbsetamp 63
# Output: Successfully set amplifier volume to: 63 (0x3f) at address 0x4b
```

## Volume Guidelines

| Volume Level | Percentage | Description |
|--------------|------------|-------------|
| 0 | 0% | Muted |
| 16 | 25% | Low volume |
| 32 | 50% | Medium volume |
| 48 | 75% | High volume |
| 63 | 100% | Maximum volume |

**Note:** Volume is linear from 0-63. The MAX9744 amplifier supports 64 discrete volume levels.

## Multiple Amplifiers

If you have multiple MAX9744 amplifiers with different I2C addresses, control them individually:

```bash
# Set volume for amplifier at 0x4B
pbsetamp --address 0x4B 32

# Set volume for amplifier at 0x4C
pbsetamp --address 0x4C 40

# Set volume for amplifier at 0x4D
pbsetamp --address 0x4D 28
```

## Use Cases

- **Testing:** Set volume levels during hardware testing
- **Calibration:** Fine-tune amplifier volume for your speaker setup
- **Scripting:** Include in startup scripts to set initial volume
- **Troubleshooting:** Verify amplifier I2C communication
- **Volume Adjustment:** Quick volume changes without recompiling code

## Error Handling

The utility provides clear error messages:

```bash
# Invalid volume
pbsetamp 100
# Error: Volume must be between 0 and 63

# Invalid address
pbsetamp --address 0xFF 32
# Error: I2C address must be 0x4B, 0x4C, or 0x4D

# Device not found
pbsetamp --address 0x4C 32
# Error: Failed to open I2C bus to MAX9744 amplifier at address 0x4c
```

## Running at Boot with Cron

### Setting Amplifier to Mute on Boot

It's highly recommended to configure pbsetamp to run at boot and set the amplifier volume to 0 (mute). This is a safety feature that prevents unexpected loud audio output when the system starts.

**Why this is useful:**
- **Prevents Speaker Damage:** Avoids sudden loud audio bursts that could damage speakers
- **Hearing Protection:** Protects users from unexpected loud sounds during startup
- **Safe Testing:** Allows you to power on the system safely, then manually adjust volume as needed
- **Development Safety:** During development, prevents audio surprises when repeatedly rebooting
- **Graceful Startup:** The pinball engine can then set the volume to the desired level after initialization

### Adding to Crontab

**Step 1: Edit your crontab**
```bash
crontab -e
```

**Step 2: Add the following line to run pbsetamp at boot**
```bash
@reboot /home/pi/PInballProj/PInball/raspibuild/pbsetamp 0
```

**Or with a specific I2C address:**
```bash
@reboot /home/pi/PInballProj/PInball/raspibuild/pbsetamp --address 0x4B 0
```

**Step 3: Save and exit the editor**

**Step 4: Verify cron job was added**
```bash
crontab -l
```

### Multiple Amplifiers at Boot

If you have multiple amplifiers, mute them all:

```bash
@reboot /home/pi/PInballProj/PInball/raspibuild/pbsetamp --address 0x4B 0
@reboot /home/pi/PInballProj/PInball/raspibuild/pbsetamp --address 0x4C 0
@reboot /home/pi/PInballProj/PInball/raspibuild/pbsetamp --address 0x4D 0
```

### Complete Boot Sequence Example

**Crontab entry:**
```bash
# Mute amplifier on boot for safety
@reboot /home/pi/PInballProj/PInball/raspibuild/pbsetamp 0

# Optional: Start pinball engine after boot (delay to allow system initialization)
@reboot sleep 10 && /home/pi/PInballProj/PInball/raspibuild/Pinball
```

**Then in your pinball engine code**, set the volume to the desired level after initialization:
```cpp
// In Pinball.cpp or similar initialization code
// After hardware is initialized, set volume from saved settings
g_PBEngine.pbsSetAmpVolume(g_PBEngine.m_saveFileData.mainVolume);
```

This approach ensures the system always starts muted, then the engine sets the appropriate volume level once it's ready.

### Troubleshooting Cron Jobs

**Check if cron service is running:**
```bash
sudo systemctl status cron
```

**View cron logs:**
```bash
grep CRON /var/log/syslog
```

**Test the command manually first:**
```bash
/home/pi/PInballProj/PInball/raspibuild/pbsetamp 0
```

---

## Building All Utilities

### Build All at Once

**Windows:**
```bash
# Build only FontGen (pblistdevices and pbsetamp require wiringPi)
```
**VS Code Task:** `Windows: Build All PBUtils`

**Raspberry Pi:**
```bash
# Build all three utilities
```
**VS Code Task:** `Raspberry Pi: Build All PBUtils`

### Individual Build Commands

**Windows:**
```bash
# Only FontGen is available on Windows
cd winbuild
FontGen.exe
```

**Raspberry Pi:**
```bash
cd raspibuild
./FontGen
./pblistdevices
./pbsetamp
```

---

## Best Practices

### FontGen
- Generate multiple font sizes for different uses (small, medium, large)
- Use appropriate buffer sizes to avoid "not enough space" errors
- Keep font files organized in `src/resources/fonts/`
- Always use `GFX_TEXTMAP` when loading fonts in code

### pblistdevices
- Run after initial hardware setup to verify all connections
- Run before starting the pinball engine if devices aren't responding
- Use output to confirm I2C addresses match your code configuration
- Include in hardware documentation for your specific machine

### pbsetamp
- Set reasonable default volumes in code (around 30-40 for safety)
- Test volume levels before connecting speakers
- Document the optimal volume setting for your amplifier/speaker combination
- Consider using in startup scripts for consistent volume on boot

---

## Troubleshooting

### FontGen Issues

**"Not enough space in the texture buffer"**
- Solution: Increase buffer size parameter

**Top of characters clipped**
- Solution: Increase V pixel bump parameter

**Font renders as white boxes**
- Solution: Use `GFX_TEXTMAP` when loading the font sprite

### pblistdevices Issues

**"No devices found"**
- Check I2C is enabled on Raspberry Pi (`sudo raspi-config`)
- Verify hardware connections and power
- Check for address conflicts

**Devices show "Empty (no response)"**
- Verify device power supply
- Check I2C pull-up resistors
- Test with multimeter for proper voltage levels

### pbsetamp Issues

**"Failed to open I2C bus"**
- Verify amplifier is powered and connected
- Check I2C address matches hardware configuration (AD pin setting)
- Use `pblistdevices` to verify amplifier is detected

**No audio output after setting volume**
- Check amplifier enable pin (should be HIGH)
- Verify audio input connections
- Test with different volume levels

---

## Integration with Main Engine

All utilities are designed to work alongside the main RasPin engine:

**FontGen:**
- Generate fonts once, use forever in your game
- Fonts referenced in code via sprite loading

**pblistdevices:**
- Run independently for diagnostics
- Can be used in shell scripts for pre-flight checks

**pbsetamp:**
- Complements in-game volume control
- Useful for initial hardware setup
- Can set volume before engine starts

---

## See Also

- **[Game Creation API](Game_Creation_API.md)** - Complete guide to PBGfx class and text rendering
- **[I/O Processing API](IO_Processing_API.md)** - Hardware I2C communication details
- **[How To Build](HowToBuild.md)** - Build instructions for all utilities
- **[RasPin Overview](RasPin_Overview.md)** - Framework architecture overview

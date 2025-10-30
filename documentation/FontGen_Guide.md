# FontGen Utility Guide

## Overview

FontGen is a command-line utility that converts TrueType (.ttf) font files into texture atlases and UV mapping files for use in the RasPin Pinball engine. It generates two files that work together to enable text rendering with custom fonts.

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

---

## Building FontGen

FontGen is built separately from the main pinball engine.

### Windows Build

**VS Code Task:** `Windows: FontGen Build`

Or manually:
```bash
cl.exe /Zi /EHsc /nologo ^
  /Fewinbuild/FontGen.exe ^
  src/stb_truetype.cpp ^
  src/stb_image_write.cpp ^
  src/FontGen.cpp
```

**Output:** `winbuild/FontGen.exe`

### Raspberry Pi Build

**VS Code Task:** `Raspberry Pi: FontGen Build`

Or manually:
```bash
g++ -g -o rpi_build/FontGen \
  src/stb_truetype.cpp \
  src/stb_image_write.cpp \
  src/FontGen.cpp \
  -lpthread
```

**Output:** `rpi_build/FontGen`

---

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

**Font with Custom V Bump (for fonts with tall ascenders):**
```bash
FontGen FancyFont.ttf 48 512 4
# Creates: FancyFont_48_512.png
#          FancyFont_48_512.json
# Uses 4-pixel V bump instead of default 2
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

**Rule of Thumb:** 
- Buffer size should be ~8-10 times the font size
- If you get "Not enough space in texture buffer" error, increase buffer size

**V Pixel Bump:**
- **2 (default)**: Works for most fonts
- **3-5**: Use for fonts with tall letters that clip at the top
- Increase if you see top of letters cut off when rendered

---

## Output Files

### PNG Texture Atlas

The PNG file contains all 95 printable ASCII characters rendered in white with alpha channel:
- Characters 32-126 (space through tilde)
- RGBA format (32-bit)
- Characters packed efficiently in rows
- Background is transparent (alpha = 0)
- Foreground is white (RGB = 255,255,255)

**Texture Layout:**
```
[sp][!]["][#][$][%][&]['][(][)][*][+][,][-][.][/][0][1][2]...
[continuing across rows until character 126 (~)]
```

### JSON UV Mapping File

The JSON file maps each character to its location in the texture atlas.

**Format:**
```json
{
    "A": {
        "height": 59,
        "u1": 0.125,
        "u2": 0.187,
        "v1": 0.111,
        "v2": 0.211,
        "width": 32
    },
    "B": {
        ...
    }
}
```

**Fields:**
- `u1`, `v1` - Top-left UV coordinates (0.0 to 1.0)
- `u2`, `v2` - Bottom-right UV coordinates (0.0 to 1.0)
- `width` - Character width in pixels
- `height` - Character height in pixels (same for all characters)

---

## Using Generated Fonts in the Engine

### Step 1: Generate Font Files

```bash
# Generate your custom font
FontGen src/resources/fonts/MyFont.ttf 48 512

# Move output files to fonts directory
# (or generate directly in the fonts directory)
```

### Step 2: Load Font in Code

Use `gfxLoadSprite()` with `GFX_TEXTMAP` type:

```cpp
// In your load function
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

**Important:** The engine automatically looks for the `.json` file with the same base name as the `.png` file.

### Step 3: Render Text

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

### Step 4: Text with Shadow

```cpp
gfxRenderShadowString(myFontId, "GAME OVER", 
                     PB_SCREENWIDTH/2, 300,  // x, y
                     2,                       // spacing
                     GFX_TEXTCENTER,          // justify
                     0, 0, 0,                 // shadow RGB (black)
                     255,                     // shadow alpha
                     4);                      // shadow offset pixels
```

---

## Complete Example Workflow

### 1. Find or Download a Font

```
Place MyGame.ttf in src/resources/fonts/
```

### 2. Generate Font Files

```bash
# Windows
cd winbuild
FontGen.exe ../src/resources/fonts/MyGame.ttf 60 512

# Raspberry Pi
cd rpi_build
./FontGen ../src/resources/fonts/MyGame.ttf 60 512

# Output:
# src/resources/fonts/MyGame_60_512.png
# src/resources/fonts/MyGame_60_512.json
```

### 3. Add to Your Game

**In your header (.h):**
```cpp
class PBEngine : public PBGfx {
public:
    unsigned int m_myGameFontId;  // Font sprite ID
};
```

**In your load function (.cpp):**
```cpp
bool PBEngine::pbeLoadMyScreen(bool forceReload) {
    static bool loaded = false;
    if (forceReload) loaded = false;
    if (loaded) return true;
    
    // Load custom font
    m_myGameFontId = gfxLoadSprite(
        "MyGameFont",
        "src/resources/fonts/MyGame_60_512.png",
        GFX_PNG, 
        GFX_TEXTMAP,      // Critical: must be GFX_TEXTMAP
        GFX_UPPERLEFT, 
        true, 
        true
    );
    
    loaded = true;
    return loaded;
}
```

**In your render function:**
```cpp
bool PBEngine::pbeRenderMyScreen(unsigned long currentTick, unsigned long lastTick) {
    if (!pbeLoadMyScreen(false)) return false;
    
    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
    
    // Render title
    gfxSetColor(m_myGameFontId, 255, 215, 0, 255);  // Gold
    gfxSetScaleFactor(m_myGameFontId, 2.0, false);
    gfxRenderShadowString(m_myGameFontId, "MY AWESOME GAME", 
                         PB_SCREENWIDTH/2, 50, 2, GFX_TEXTCENTER,
                         0, 0, 0, 255, 5);
    
    // Render score
    gfxSetColor(m_myGameFontId, 255, 255, 255, 255);  // White
    gfxSetScaleFactor(m_myGameFontId, 1.0, false);
    std::string score = "Score: " + std::to_string(currentScore);
    gfxRenderString(m_myGameFontId, score, 50, 100, 1, GFX_TEXTLEFT);
    
    return true;
}
```

---

## Existing Fonts in the Framework

The framework includes several pre-generated fonts in `src/resources/fonts/`:

| Font File | Generated Files | Description |
|-----------|----------------|-------------|
| `Ubuntu-Regular.ttf` | `Ubuntu-Regular_24_256.png/json` | System default font (24px) |
| `Baldur.ttf` | `Baldur_60_512.png/json` | Medieval style, medium (60px) |
| `Baldur.ttf` | `Baldur_96_768.png/json` | Medieval style, large (96px) |
| `IMMORTAL.ttf` | `IMMORTAL_60_512.png/json` | Decorative font (60px) |

**Usage Example:**
```cpp
#define MENUFONT "src/resources/fonts/Baldur_96_768.png"

// In code:
m_menuFontId = gfxLoadSprite("MenuFont", MENUFONT, 
                             GFX_PNG, GFX_TEXTMAP, GFX_UPPERLEFT, 
                             true, true);
```

---

## Troubleshooting

### "Not enough space in the texture buffer"

**Problem:** All characters don't fit in the texture.

**Solution:** Increase buffer size:
```bash
# Try doubling the buffer size
FontGen MyFont.ttf 60 1024  # instead of 512
```

### Top of Characters Clipped

**Problem:** Tall letters (like 'b', 'h', 'd') have tops cut off.

**Solution:** Increase V pixel bump:
```bash
FontGen MyFont.ttf 60 512 5  # instead of default 2
```

### Characters Look Blurry

**Problem:** Font size too small for the texture.

**Solution:** Use appropriate texture size for font:
```bash
# For 24px font, 256 is fine
FontGen MyFont.ttf 24 256

# For 96px font, need larger texture
FontGen MyFont.ttf 96 768
```

### JSON File Not Found Error

**Problem:** Engine can't find the `.json` file.

**Cause:** The `.png` and `.json` files must be in the same directory with the same base name.

**Solution:** 
- Ensure both files are in `src/resources/fonts/`
- Verify names match exactly: `MyFont_60_512.png` and `MyFont_60_512.json`

### Font Renders as White Boxes

**Problem:** Font sprite loads but shows white rectangles instead of text.

**Cause:** Font loaded without `GFX_TEXTMAP` flag.

**Solution:** Use `GFX_TEXTMAP` when loading:
```cpp
gfxLoadSprite(name, path, GFX_PNG, GFX_TEXTMAP, ...);
//                                  ^^^^^^^^^^^
```

---

## Technical Details

### Character Set

FontGen generates exactly 95 printable ASCII characters:
- Space (32) through Tilde (126)
- No extended ASCII or Unicode characters
- No control characters

### Texture Format

- **Image Format:** PNG with alpha channel
- **Color Space:** RGBA (32-bit)
- **Character Color:** White (255, 255, 255)
- **Background:** Transparent (alpha = 0)
- **Channel Usage:** Color can be changed in engine, alpha provides character shape

### UV Coordinate System

- **Origin:** Top-left (0.0, 0.0)
- **Range:** 0.0 to 1.0 for both U and V
- **V Direction:** Increases downward
- **Precision:** Float values in JSON

### Font Metrics

FontGen uses TrueType font metrics:
- **Ascent:** Distance from baseline to top of tallest character
- **Descent:** Distance from baseline to bottom of lowest character
- **Line Gap:** Additional spacing between lines
- **Scale:** Calculated to match requested pixel height

---

## Best Practices

### Organizing Fonts

```
src/resources/fonts/
├── Arial.ttf                    (source font)
├── Arial_24_256.png             (small size)
├── Arial_24_256.json
├── Arial_48_512.png             (medium size)
├── Arial_48_512.json
└── Arial_72_768.png             (large size)
    Arial_72_768.json
```

### Font Size Guidelines

**Create multiple sizes for different uses:**
- **Small (24-32px):** Debug, console, small labels
- **Medium (48-60px):** Menu text, game messages
- **Large (72-96px):** Titles, scores, important text

### Performance Considerations

- **Keep Resident:** Always use `keepResident = true` for fonts
- **Preload:** Load fonts in initial load function, not during gameplay
- **Reuse:** Use one font for multiple text strings
- **Limit Count:** Don't load excessive fonts (3-5 is typical)

### Naming Convention

Use descriptive, consistent names:
```cpp
m_systemFontId      // Small system font
m_menuFontId        // Menu display font
m_scoreFontId       // Score/number font
m_titleFontId       // Large title font
```

---

## Integration with Engine

### How the Engine Uses Font Files

1. **Load Sprite:** Engine loads PNG as texture
2. **Load JSON:** Automatically loads matching `.json` file
3. **Parse UV Data:** Reads character UV coordinates into `m_textMapList`
4. **Set Space Width:** Adjusts space character width to match 'j' width
5. **Ready to Render:** Font available for `gfxRenderString()` calls

### Internal Data Structure

```cpp
// Engine internal (you don't access this directly)
std::map<unsigned int, std::map<std::string, stTextMapData>> m_textMapList;
//       sprite ID          character    UV data

struct stTextMapData {
    unsigned int width;   // Character width
    unsigned int height;  // Character height
    float U1, V1;        // Top-left UV
    float U2, V2;        // Bottom-right UV
};
```

### Rendering Pipeline

```
User Code: gfxRenderString(fontId, "Hello", ...)
    ↓
Engine: For each character 'H', 'e', 'l', 'l', 'o':
    ↓
    1. Look up UV coords in m_textMapList[fontId]["H"]
    ↓
    2. Render quad with character's UV coordinates
    ↓
    3. Advance X position by character width + spacing
    ↓
Final Result: "Hello" rendered on screen
```

---

## See Also

- **Game_Creation_API.md** - Complete guide to PBGfx class and text rendering
- **PBEngine_API.md** - Core engine functions
- **HowToBuild.md** - Build instructions for FontGen and main engine
- **UsersGuide.md** - Complete framework documentation

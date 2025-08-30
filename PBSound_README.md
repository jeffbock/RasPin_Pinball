# PBSound - Audio System for PInball

## Overview

PBSound is a C++ class designed to provide audio playback functionality for the PInball project using SDL_mixer on Raspberry Pi 5. It supports playing MP3 files for both background music and sound effects with reasonable quality while maintaining low memory usage.

## Features

- **Cross-platform compatibility**: Compiles on both Windows and Raspberry Pi, with platform-specific functionality
- **Background music**: Loop MP3 files for continuous background music
- **Sound effects**: Play up to 4 simultaneous sound effects
- **Volume control**: Separate volume controls for master and music levels (0-100%)
- **Effect management**: Track and query the status of playing effects
- **Memory efficient**: Uses SDL_mixer's optimized audio format and caching system

## Platform Support

- **Raspberry Pi 5** (EXE_MODE_RASPI): Full functionality with SDL_mixer
- **Windows** (EXE_MODE_WINDOWS): Stub implementation for compilation compatibility

## Dependencies

### Raspberry Pi 5
To use PBSound on Raspberry Pi, you need to install SDL2 and SDL_mixer:

```bash
sudo apt update
sudo apt install libsdl2-dev libsdl2-mixer-dev
```

### Build Configuration
Make sure to set the correct build mode in `PBBuildSwitch.h`:

For Raspberry Pi:
```cpp
// #define EXE_MODE_WINDOWS
#define EXE_MODE_RASPI
```

For Windows:
```cpp
#define EXE_MODE_WINDOWS
// #define EXE_MODE_RASPI
```

## Audio Format

The system is configured with the following audio settings for optimal performance:
- **Sample Rate**: 44.1 kHz
- **Format**: 16-bit signed
- **Channels**: Stereo
- **Buffer Size**: 1024 bytes

This provides good quality audio while keeping memory usage reasonable.

## API Reference

### Constructor/Destructor
```cpp
PBSound();          // Constructor
~PBSound();         // Destructor (automatically calls pbsShutdown)
```

### Initialization
```cpp
bool pbsInitialize();  // Initialize SDL and audio system
void pbsShutdown();    // Clean up and shutdown (called automatically by destructor)
```

### Music Control
```cpp
bool pbsPlayMusic(const std::string& mp3FilePath);  // Play looping background music
void pbsStopMusic();                                 // Stop current music
```

### Sound Effects
```cpp
int pbsPlayEffect(const std::string& mp3FilePath);  // Play effect once (returns effect ID 1-4, 0 = fail)
bool pbsIsEffectPlaying(int effectId);              // Check if effect is still playing
void pbsStopEffect(int effectId);                   // Stop specific effect
void pbsStopAllEffects();                           // Stop all effects
```

### Volume Control
```cpp
void pbsSetMasterVolume(int volume);   // Set overall volume (0-100%)
void pbsSetMusicVolume(int volume);    // Set music volume (0-100%)
int pbsGetMasterVolume() const;        // Get current master volume
int pbsGetMusicVolume() const;         // Get current music volume
```

## Usage Example

```cpp
#include "PBSound.h"

int main() {
    PBSound sound;
    
    // Initialize
    if (!sound.pbsInitialize()) {
        return -1;  // Failed to initialize
    }
    
    // Set volumes
    sound.pbsSetMasterVolume(80);  // 80% master volume
    sound.pbsSetMusicVolume(60);   // 60% music volume
    
    // Play background music
    if (sound.pbsPlayMusic("background.mp3")) {
        // Music started successfully
    }
    
    // Play sound effects
    int shootEffect = sound.pbsPlayEffect("shoot.mp3");
    if (shootEffect > 0) {
        // Effect is playing with ID 'shootEffect'
        
        // Check if still playing
        if (sound.pbsIsEffectPlaying(shootEffect)) {
            // Effect is still playing
        }
    }
    
    // Stop specific effect
    sound.pbsStopEffect(shootEffect);
    
    // Stop music
    sound.pbsStopMusic();
    
    // Cleanup is automatic when object goes out of scope
    return 0;
}
```

## Effect Slot Management

The system can play up to 4 sound effects simultaneously. When `pbsPlayEffect()` is called:

1. Returns effect ID 1-4 if successful
2. Returns 0 if all 4 slots are occupied or if there's an error
3. Effect slots are automatically freed when effects finish playing
4. You can manually stop effects using `stopEffect()` or `stopAllEffects()`

## Audio File Caching

The system automatically caches loaded sound effects to improve performance:
- Effects are loaded once and reused for subsequent plays
- Cache is automatically cleared during shutdown
- Only effects are cached; music files are loaded fresh each time

## Error Handling

- All functions return appropriate error codes or boolean values
- On Windows, functions return failure values but don't crash
- Check return values to handle errors gracefully

## Memory Considerations

- Sound effects are cached in memory for performance
- Music files are streamed, not fully loaded into memory
- SDL_mixer handles audio format conversion automatically
- Consider the total size of cached effects when planning memory usage

## Integration Notes

- Include `PBSound.h` in your project
- Add `PBSound.cpp` to your build system
- Link with SDL2 and SDL_mixer libraries on Raspberry Pi
- Ensure proper cleanup by calling `pbsShutdown()` or relying on destructor

## Troubleshooting

### Common Issues

1. **Initialization fails**: Check if SDL2 and SDL_mixer are properly installed
2. **No audio output**: Verify audio device configuration and volume settings
3. **Effects fail to play**: Check file paths and ensure MP3 files are valid
4. **Compilation errors**: Verify `PBBuildSwitch.h` is configured correctly

### Build System Integration

Add to your Raspberry Pi build task:
```bash
-lSDL2 -lSDL2_mixer
```

For the PInball project, update the Raspberry Pi build task to include:
```bash
"${workspaceFolder}/src/PBSound.cpp",
```
And add the SDL libraries to the link flags.

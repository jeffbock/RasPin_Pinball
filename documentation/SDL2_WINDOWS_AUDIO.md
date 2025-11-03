# SDL2 Audio Support for Windows (Simulation Mode)

## Overview
This document describes how to enable audio playback on Windows builds using SDL2, matching the existing Raspberry Pi implementation.

## Current Situation
- `PBSound` class is fully implemented with SDL2_mixer for Raspberry Pi
- All methods return stubs/false on Windows due to `#ifdef EXE_MODE_RASPI` guards
- Code is already well-structured and ready to work cross-platform
- Windows builds are currently silent (no audio output)

## Recommended Solution: Enable SDL2 for Windows Build

### Why This Approach?
- ✅ **Minimal code changes** - Only need to modify preprocessor guards (~10 lines)
- ✅ **Reuses existing, working code** - No new implementation needed
- ✅ **Same library on both platforms** - SDL2 is cross-platform
- ✅ **Same behavior** - Identical API on Windows and Raspberry Pi
- ✅ **Low risk** - SDL2 is mature, stable, and designed for games
- ✅ **Easy to test and debug** - Well-documented library
- ✅ **Handles complexity** - Threading, timing, mixing all built-in

### Implementation Steps

#### 1. Download SDL2 Libraries for Windows

**SDL2 Development Libraries:**
- Visit: https://github.com/libsdl-org/SDL/releases
- Download: `SDL2-devel-2.x.x-VC.zip` (Visual C++ version)
- Extract the archive

**SDL2_mixer Development Libraries:**
- Visit: https://github.com/libsdl-org/SDL_mixer/releases
- Download: `SDL2_mixer-devel-2.x.x-VC.zip` (Visual C++ version)
- Extract the archive

#### 2. Install Libraries

**Headers:**
Copy from extracted SDL2 package:
- `SDL2-x.x.x/include/*.h` → `src/include_ogl_win/SDL2/`

Copy from extracted SDL2_mixer package:
- `SDL2_mixer-x.x.x/include/*.h` → `src/include_ogl_win/SDL2/`

**Libraries (64-bit):**
Copy from SDL2 package:
- `SDL2-x.x.x/lib/x64/SDL2.lib` → `src/lib_ogl_win/`
- `SDL2-x.x.x/lib/x64/SDL2main.lib` → `src/lib_ogl_win/`

Copy from SDL2_mixer package:
- `SDL2_mixer-x.x.x/lib/x64/SDL2_mixer.lib` → `src/lib_ogl_win/`

**Runtime DLLs:**
Copy from SDL2 package:
- `SDL2-x.x.x/lib/x64/SDL2.dll` → `winbuild/`

Copy from SDL2_mixer package:
- `SDL2_mixer-x.x.x/lib/x64/SDL2_mixer.dll` → `winbuild/`
- Also copy any additional DLLs for codecs (libFLAC, libmpg123, etc.)

#### 3. Modify Code Files

**File: `src/PBSound.h`**

Change the SDL2 includes section (around line 17):
```cpp
// FROM:
#ifdef EXE_MODE_RASPI
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <map>
#endif

// TO:
#if defined(EXE_MODE_RASPI) || defined(EXE_MODE_WINDOWS)
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <map>
#endif
```

Change all member variables section (around line 74):
```cpp
// FROM:
#ifdef EXE_MODE_RASPI
    Mix_Music* currentMusic;
    // ... rest of private members
#endif

// TO:
#if defined(EXE_MODE_RASPI) || defined(EXE_MODE_WINDOWS)
    Mix_Music* currentMusic;
    // ... rest of private members
#endif
```

**File: `src/PBSound.cpp`**

Replace **ALL** occurrences of `#ifdef EXE_MODE_RASPI` with:
```cpp
#if defined(EXE_MODE_RASPI) || defined(EXE_MODE_WINDOWS)
```

This includes:
- Line 8: Static instance declaration
- Line 12: Constructor implementation
- Line 32: pbsInitialize()
- Line 68: pbsShutdown()
- Line 101: pbsPlayMusic()
- Line 134: pbsStopMusic()
- Line 142: pbsPauseMusic()
- Line 150: pbsResumeMusic()
- Line 158: pbsPlayEffect()
- Line 197: pbsIsEffectPlaying()
- Line 215: pbsStopEffect()
- Line 232: pbsStopAllEffects()
- Line 250: pbsSetMasterVolume()
- Line 265: pbsSetMusicVolume()
- Line 280: Helper functions section
- Line 383: Video audio functions
- Line 474: Static callback function

**Count:** Approximately 20-25 occurrences to change.

#### 4. Update Build Task

**File: `.vscode/tasks.json`**

Update the "Windows: Full Pinball Build" task to include SDL2 libraries:

In the `"args"` array, add after the OpenGL libraries:
```json
"SDL2.lib",
"SDL2main.lib",
"SDL2_mixer.lib",
```

The libraries should be added in the link section, after:
```json
"libegl.dll.lib",
"libglesv2.dll.lib",
```

#### 5. Update Release Build Task (Optional)

If you want sound in release builds, also update:
- "Windows: Full Pinball Build (Release)" task with the same SDL2 libraries

### Alternative: Selective Windows Support (Lower Risk)

If you want to enable only music and effects (not video audio), you can use a more selective approach:

1. Use `#if defined(EXE_MODE_RASPI) || defined(EXE_MODE_WINDOWS)` for basic functions
2. Keep video audio methods under `#ifdef EXE_MODE_RASPI` only:
   - `pbsStartVideoAudioStream()`
   - `pbsStopVideoAudio()`
   - `pbsRestartVideoAudioStream()`
   - `pbsIsVideoAudioPlaying()`
   - `channelFinishedCallback()`
   - `handleChannelFinished()`

This reduces risk since video audio streaming is more complex.

### Testing

After implementation, test:

1. **Initialization:**
   ```cpp
   bool success = g_PBEngine.m_soundSystem.pbsInitialize();
   // Should return true if SDL2 is properly installed
   ```

2. **Background Music:**
   ```cpp
   g_PBEngine.m_soundSystem.pbsPlayMusic("src/resources/sound/music.mp3");
   ```

3. **Sound Effects:**
   ```cpp
   int effectId = g_PBEngine.m_soundSystem.pbsPlayEffect("src/resources/sound/effect.mp3");
   ```

4. **Volume Control:**
   ```cpp
   g_PBEngine.m_soundSystem.pbsSetMasterVolume(50);  // 50%
   g_PBEngine.m_soundSystem.pbsSetMusicVolume(75);   // 75%
   ```

### Troubleshooting

**Build Errors:**
- Verify SDL2 headers are in `src/include_ogl_win/SDL2/`
- Verify .lib files are in `src/lib_ogl_win/`
- Check that library names match exactly in tasks.json

**Runtime Errors:**
- Ensure SDL2.dll and SDL2_mixer.dll are in the same folder as Pinball.exe
- Check that DLLs match architecture (x64 for 64-bit builds)
- Verify audio files exist and are accessible

**No Sound Output:**
- Check Windows audio mixer settings
- Verify audio device is not muted
- Check that pbsInitialize() returns true
- Test with known-good MP3/WAV files

### Estimated Effort
- **Library download/setup:** 1-2 hours
- **Code modifications:** 30 minutes
- **Build task updates:** 15 minutes
- **Testing:** 30 minutes
- **Total:** 2.5-3 hours

### File Size Impact
- SDL2.dll: ~1.5 MB
- SDL2_mixer.dll: ~500 KB
- Codec DLLs: ~500 KB - 1 MB
- **Total distribution size increase:** ~2-3 MB

### Dependencies Added
- SDL2 (version 2.x or later)
- SDL2_mixer (version 2.x or later)
- Optional codec libraries (included with SDL2_mixer)

### Maintenance Notes
- SDL2 APIs are stable; version updates rarely break compatibility
- Same codebase for Windows and Raspberry Pi reduces maintenance
- No platform-specific code needed in game logic
- Volume and playback control work identically on both platforms

### Future Enhancements
Once SDL2 is working on Windows, consider:
- Adding support for more audio formats (OGG, FLAC)
- Implementing 3D positional audio
- Adding audio effects (reverb, echo)
- Creating audio configuration UI in settings

## Alternative Approaches (Not Recommended)

### Windows Audio Session API (WASAPI)
- **Effort:** Very high (~40-80 hours)
- **Risk:** High (new implementation, different paradigm)
- **Maintenance:** Requires maintaining two separate implementations
- **Benefit:** No external dependencies, but not worth the cost

### DirectSound
- **Status:** Deprecated by Microsoft
- **Effort:** High
- **Risk:** Medium-high
- **Not recommended:** Legacy API, limited support

### Windows Media Foundation
- **Effort:** High (~30-50 hours)
- **Risk:** Medium-high
- **Complexity:** More complex than SDL2
- **Not recommended:** Overkill for game audio needs

## Conclusion

Enabling SDL2 on Windows is the clear winner:
- Minimal code changes
- Low risk
- Reuses proven, working code
- Cross-platform consistency
- Easy to maintain

The only downside is adding 2-3 MB of DLLs to the distribution, which is negligible for modern systems.

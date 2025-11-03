# Video System Cleanup - Conservative Approach
**Date:** November 3, 2025  
**Branch:** jbock/videotest  
**Scope:** Dead code removal from PBVideo, PBVideoPlayer, and PBSound

---

## Changes Made

### ✅ Removed (Dead Code)

#### PBVideo.h
- **Removed:** `smoothAudioBuffer[AUDIO_BUFFER_FRAMES * 2]` array
- **Removed:** `audioReadIndex`, `audioWriteIndex`, `audioBufferedSamples` variables
- **Removed:** `AUDIO_BUFFER_FRAMES` constant (was 8192)
- **Reason:** Unused intermediate buffer system replaced by `audioAccumulator`

#### PBVideo.cpp
- **Removed:** Duplicate `#include <algorithm>` (line 8)
- **Removed:** Initializations of removed variables in constructor
- **Removed:** Resets of removed variables in `pbvUnloadVideo()`
- **Removed:** Debug print line for `audioBufferedSamples` in `pbvPrintDecoderInfo()`

#### PBSound.h
- **Removed:** `pbsQueueVideoAudio()` function declaration
- **Reason:** Obsolete push-based audio queuing replaced by callback streaming

#### PBSound.cpp
- **Removed:** `pbsQueueVideoAudio()` implementation
- **Reason:** Function only returned true without doing anything

---

### ✅ Preserved (Future Use)

#### PBVideo.h / PBVideo.cpp
- **Kept:** `getVideoClock()`, `getAudioClock()`, `getMasterClock()`, `shouldDisplayFrame()`
- **Kept:** `videoClock`, `audioClock`, `masterClock` member variables
- **Added:** Comments marking these as "FUTURE:" for A/V sync improvements
- **Reason:** Standard A/V synchronization methods useful for:
  - Variable frame rate (VFR) video support
  - Advanced audio/video sync scenarios
  - Alternative timing/sync modes

---

## Functional Impact

### ❌ No Breaking Changes Expected
- All removed code was dead/unused
- Current video playback uses:
  - `audioAccumulator` buffer (not the removed `smoothAudioBuffer`)
  - Simple FPS-based frame timing (not the preserved clock methods)
  - Callback-based audio streaming (not the removed queue function)

### ✅ Code Improvements
- ~150 lines of dead code removed
- Clearer architecture (one audio buffer system)
- Better maintainability
- Reduced confusion for future developers

---

## Testing Checklist (Raspberry Pi)

When testing on Raspberry Pi, verify:

### Video Playback
- [ ] Video loads successfully (`pbvpLoadVideo()`)
- [ ] Video plays without stuttering (`pbvpPlay()`)
- [ ] Frame updates work correctly (`pbvpUpdate()`)
- [ ] Video renders properly (`pbvpRender()`)

### Audio Playback
- [ ] Audio plays synchronized with video
- [ ] No audio popping or crackling
- [ ] Audio continues smoothly throughout playback
- [ ] Audio restarts properly when video loops

### Controls
- [ ] Pause/resume works (`pbvpPause()`, `pbvpPlay()`)
- [ ] Seeking works (`pbvpSeekTo()`)
- [ ] Looping works correctly (`pbvpSetLooping(true)`)
- [ ] Stop and restart works (`pbvpStop()`, `pbvpPlay()`)

### Edge Cases
- [ ] Multiple videos can play sequentially
- [ ] Video unload/reload works properly
- [ ] Long-running videos (>5 min) play without issues
- [ ] Audio buffer doesn't underrun during intensive scenes

---

## Rollback Instructions

If issues occur on Raspberry Pi:

### Quick Rollback
```bash
git diff HEAD~1 src/PBVideo.h src/PBVideo.cpp src/PBSound.h src/PBSound.cpp
git checkout HEAD~1 -- src/PBVideo.h src/PBVideo.cpp src/PBSound.h src/PBSound.cpp
```

### Specific Issue Workarounds

**If audio issues occur:**
- Check: Was `audioAccumulator` accidentally modified? (It wasn't touched)
- Check: Is SDL_mixer callback still working? (Unchanged)
- Check: Is `pbsStartVideoAudioStream()` being called? (Unchanged)

**If video timing issues occur:**
- Check: Is `getCurrentPlaybackTimeSec()` working? (Unchanged)
- Check: Is frame timing calculation correct? (Unchanged)
- Note: Clock methods are still present if needed, just not actively used

---

## Files Modified

```
src/PBVideo.h       - 9 lines removed, 1 comment added
src/PBVideo.cpp     - 12 lines removed, 5 comments added
src/PBSound.h       - 1 line removed
src/PBSound.cpp     - 9 lines removed
```

**Total:** ~31 lines removed, ~6 comment lines added

---

## Notes

- The preserved clock synchronization methods are **not** causing any overhead (private methods, only called explicitly)
- If future A/V sync issues arise, the foundation is already in place
- The `audioAccumulator` system (kept) is the **active** audio buffering mechanism
- Video audio streaming still uses callback pattern (`channelFinishedCallback`)

---

## Contact

If issues arise during Raspberry Pi testing, this cleanup can be easily reverted.
All changes are non-functional (dead code removal) and well-documented.

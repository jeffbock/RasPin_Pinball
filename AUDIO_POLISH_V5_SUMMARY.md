# Audio Polish v5 - v0.5.157

## Issues Addressed
1. **Tiny popping/crackling** - Small audio gaps or buffer underruns
2. **Audio not resetting on loop** - Old audio continued playing when video looped

## Solutions Implemented

### 1. Increased Audio Buffer (Prevents Popping)

**Problem:** Buffer at 50% capacity caused occasional underruns leading to pops/clicks

**Solution:** Increased buffer fill from 50% to 75%
- **pbvPlay()**: Pre-fills to 75% before starting playback
- **pbvUpdateFrame()**: Maintains 75% capacity during playback
- Result: More headroom prevents audio starvation

**Code Changes:**
```cpp
// Before: 50% buffer
while (audioAccumulatorIndex < AUDIO_ACCUMULATOR_SIZE * 0.5f)

// After: 75% buffer  
while (audioAccumulatorIndex < AUDIO_ACCUMULATOR_SIZE * 0.75f)
```

### 2. Audio Stream Restart on Loop (Clean Transitions)

**Problem:** When video looped, audio accumulator was cleared but streaming callback kept pulling old data, causing audio to not reset properly

**Solution:** Detect loop and restart audio stream
1. Added `justLooped` flag to PBVideo
2. Set flag when video loops in `pbvUpdateFrame()`
3. Added `pbvDidJustLoop()` method to check and clear flag
4. Added `pbsRestartVideoAudioStream()` to stop and restart stream
5. PBVideoPlayer checks for loop and restarts audio

**Flow:**
```
Video loops:
  → seekToFrame(0.0f) clears accumulator
  → justLooped = true
  
PBVideoPlayer.pbvpUpdate():
  → pbvDidJustLoop() returns true
  → pbsRestartVideoAudioStream()
      → Stop current stream (clears chunk, halts channel)
      → 5ms delay for clean stop
      → Start new stream (pulls fresh audio from reset buffer)
```

### Benefits

✅ **Smoother Audio** - 75% buffer provides more cushion against underruns  
✅ **No More Pops** - Larger buffer prevents audio starvation gaps  
✅ **Clean Loops** - Audio properly resets when video loops  
✅ **Seamless Transitions** - Short delay ensures clean stream restart  

### Technical Details

**Buffer Size:** 88200 samples (1 second at 44.1kHz stereo)
- Previous fill: 44100 samples (0.5 seconds)
- New fill: 66150 samples (0.75 seconds)
- Extra cushion: 22050 samples (~500ms)

**Restart Timing:** 5ms delay between stop and restart ensures:
- SDL_mixer fully releases channel 4
- Old chunk memory is freed
- Clean state for new stream

## Testing Notes

This build should eliminate the remaining pops and ensure audio restarts cleanly on each loop. The increased buffer provides more tolerance for timing variations and system load.

Build: v0.5.157
Date: November 2, 2025

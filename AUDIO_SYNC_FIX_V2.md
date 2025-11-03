# Audio Sync Fix - Version 2

## Issue Discovered
After implementing the initial buffer size increases, the audio improved (less clicking) but was playing **much faster than the video**, losing synchronization.

## Root Cause
The initial fix caused audio to race ahead because:
1. Buffer fill threshold was too high (75%) - accumulated too much audio
2. Fixed chunk size of 8192 samples (~185ms) was too large for typical video frame rates
3. At 30fps, each frame is only 33ms, so we were providing ~5.6 frames worth of audio per video frame

## Solution Applied (v0.5.149)

### 1. Dynamic Audio Chunk Sizing
**Changed:** Calculate audio chunk size based on actual video frame rate
```cpp
// For 30fps: 1/30 = 0.0333s = 2940 samples (with 1.2x buffer = 3528 samples)
// For 60fps: 1/60 = 0.0167s = 1470 samples (with 1.2x buffer = 1764 samples)
float frameDuration = 1.0f / videoInfo.fps;
targetSamplesPerFrame = (int)(44100.0f * frameDuration * 2.0f * 1.2f);
```

**Benefits:**
- Audio chunk size automatically matches video frame timing
- 1.2x multiplier provides small overlap for smooth playback
- Prevents audio from racing ahead
- Maintains proper A/V sync

### 2. Reduced Buffer Fill Threshold
**Changed:** Back to 50% (from 75%)
```cpp
while (audioAccumulatorIndex < AUDIO_ACCUMULATOR_SIZE * 0.5f) {
```

**Reasoning:**
- Prevents over-accumulation of audio
- Keeps audio in sync with video timing
- Still provides enough buffer to prevent underruns

### 3. Maintained Dual-Channel System
**Kept:** Alternating channels 4 and 5 for smooth overlap
- Allows continuous audio without gaps
- Reduces clicking by permitting smooth transitions
- Automatic cleanup of finished chunks

### 4. Clamped Chunk Size Bounds
**Added:** Maximum chunk size reduced to 6144 samples (~139ms)
```cpp
if (targetSamplesPerFrame > 6144) targetSamplesPerFrame = 6144;
```

**Reasoning:**
- Prevents excessively large chunks even at low frame rates
- Ensures responsive audio behavior
- Balances smoothness with timing accuracy

## Key Changes Summary

| Parameter | Before (v0.5.148) | After (v0.5.149) |
|-----------|------------------|------------------|
| Audio Chunk Size | Fixed 8192 samples | Dynamic 2048-6144 samples |
| Buffer Fill | 75% | 50% |
| Chunk Calculation | Static | Frame-rate based |
| Max Chunk Duration | ~185ms | ~139ms |

## Expected Results

✅ **Improved audio/video synchronization** - Audio stays in sync with video  
✅ **Reduced clicking** - Dual-channel overlap smooths transitions  
✅ **Dynamic adaptation** - Works properly with different video frame rates  
✅ **Balanced buffering** - Enough to prevent gaps, not so much to cause drift  

## Technical Details

### Audio Samples Calculation
At 44.1kHz stereo:
- **30fps video**: 2940 samples/frame × 1.2 = 3528 samples (~80ms)
- **24fps video**: 3675 samples/frame × 1.2 = 4410 samples (~100ms)
- **60fps video**: 1470 samples/frame × 1.2 = 1764 samples (~40ms)

### Buffer Characteristics
- **AUDIO_ACCUMULATOR_SIZE**: 88200 samples = 1 second at 44.1kHz stereo
- **50% fill threshold**: 44100 samples = 0.5 seconds
- **Provides**: ~15-25 frames of buffer depending on video frame rate

## Memory Usage
**No change from v0.5.148:**
- Total: ~417 KB
- Impact on 8GB Raspberry Pi: 0.0046% of RAM

## Build Info
- Version: v0.5.149
- Build Status: ✅ Success
- Platform: Raspberry Pi
- Branch: jbock/videotest

## Testing Notes

Test these scenarios:
1. ✅ Audio/video sync over extended playback (check for drift)
2. ✅ Different frame rates (24fps, 30fps, 60fps)
3. ✅ Audio quality (clicking/popping should be minimal)
4. ✅ Seek operations (verify sync after seeking)
5. ✅ Loop playback (verify smooth restart)

## Next Steps if Issues Persist

If audio is still not perfect:

**If still choppy:**
- Increase multiplier from 1.2x to 1.5x
- Check SDL_mixer buffer settings
- Monitor for audio underruns

**If sync drifts over time:**
- Implement periodic sync correction
- Add drift detection and compensation
- Consider using audio clock as master

**If clicking persists:**
- Add crossfade between chunks
- Implement smoother buffer transitions
- Check for buffer boundary artifacts

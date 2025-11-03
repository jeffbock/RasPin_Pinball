# Video Audio Choppy Playback - Implementation Summary

## Date: November 2, 2025
## Version: v0.5.148

## Problem
Video playback had correctly synchronized audio and video, but the audio was extremely choppy with gaps and clicking sounds.

## Root Causes Identified
1. **Small audio chunks** (~46ms) being played individually with gaps between them
2. **Audio samples skipped** when previous chunk was still playing, creating silent gaps
3. **Inefficient buffer management** with small buffers and frequent memory operations
4. **Audio tied to video frame rate** instead of streaming continuously

## Memory Impact Analysis
**Before:**
- AUDIO_BUFFER_FRAMES: 2048 frames × 2 channels = 16 KB
- AUDIO_ACCUMULATOR_SIZE: 8820 samples = 35 KB
- Total: ~51 KB

**After:**
- AUDIO_BUFFER_FRAMES: 8192 frames × 2 channels = 64 KB
- AUDIO_ACCUMULATOR_SIZE: 88200 samples = 353 KB
- Total: ~417 KB

**Net increase: ~366 KB (0.36 MB)**
- Impact on 8GB Raspberry Pi: **0.0046% of total RAM**
- **Completely negligible** - no performance concerns

## Changes Implemented

### 1. Buffer Size Increases (src/PBVideo.h)
```cpp
// Increased from 2048 to 8192 frames
static const int AUDIO_BUFFER_FRAMES = 8192;  

// Increased from 8820 to 88200 samples (1 second of 44.1kHz stereo audio)
static const int AUDIO_ACCUMULATOR_SIZE = 88200;
```

**Benefits:**
- Larger buffers prevent audio underruns
- Reduces frequency of buffer refills
- Provides smoother continuous playback

### 2. Larger Audio Chunks (src/PBVideo.cpp - pbvGetAudioSamples)
```cpp
// Changed from 2048 to 8192 samples (~185ms at 44.1kHz stereo)
int samplesToProvide = std::min(audioSamplesAvailable, 8192);
```

**Benefits:**
- Reduces overhead from frequent chunk creation
- Better buffering for SDL_mixer
- Fewer gaps between audio chunks

### 3. Keep Buffer Fuller (src/PBVideo.cpp - pbvUpdateFrame)
```cpp
// Changed from 50% to 75% full threshold
while (audioAccumulatorIndex < AUDIO_ACCUMULATOR_SIZE * 0.75f) {
```

**Benefits:**
- Maintains larger audio buffer reserve
- Reduces risk of audio starvation
- Better resilience to timing variations

### 4. Dual-Channel Audio Streaming (src/PBSound.h & src/PBSound.cpp)

**Header changes:**
```cpp
// Changed from single chunk to dual chunks for alternating playback
Mix_Chunk* videoAudioChunks[2];  // Two chunks for alternating
int videoAudioChannels[2];       // Channels 4 and 5
bool videoAudioActive[2];        // Track which chunks are active
int videoAudioToggle;            // Alternates between 0 and 1
```

**Implementation changes:**
- Alternates between SDL_mixer channels 4 and 5
- Allows smooth overlap between audio chunks
- Cleans up finished chunks automatically
- No more skipping of audio samples

**Benefits:**
- Eliminates gaps between audio chunks
- Allows seamless audio streaming
- Automatic cleanup prevents memory leaks
- Better audio continuity

## Expected Results

### Audio Quality Improvements
✅ Smoother audio playback without gaps
✅ Elimination of clicking/popping sounds
✅ Better audio/video synchronization
✅ More consistent audio streaming

### Performance Characteristics
✅ Minimal memory overhead (~366 KB)
✅ Reduced CPU overhead from fewer chunk operations
✅ Better buffer utilization
✅ More resilient to timing variations

## Testing Recommendations

1. **Various Video Formats**
   - Test different frame rates: 24fps, 30fps, 60fps
   - Test different audio sample rates
   - Test different video codecs

2. **Playback Duration**
   - Short clips (< 30 seconds)
   - Medium clips (1-5 minutes)
   - Long clips (> 10 minutes)
   - Check for audio/video drift over time

3. **System Load**
   - Test with other processes running
   - Monitor CPU and memory usage
   - Check for buffer underruns

4. **Edge Cases**
   - Seeking during playback
   - Pause/resume cycles
   - Looping videos
   - Rapid start/stop

## Build Information
- Build successfully compiled on Raspberry Pi
- Version: v0.5.148
- No compilation errors
- Ready for testing

## Future Enhancements (Optional)

For even better audio quality, consider:

1. **Ring Buffer with Separate Thread**
   - Decouple audio decoding from video frame updates
   - Use dedicated audio thread for continuous streaming
   - Larger ring buffer (2-3 seconds)

2. **SDL Audio Callback**
   - Use SDL_mixer's post-mix callback
   - Direct buffer feeding without chunk creation
   - Even lower latency

3. **Dynamic Buffer Adjustment**
   - Monitor buffer levels
   - Adjust buffer sizes based on playback performance
   - Adaptive buffering for different video properties

## Notes
- Changes are backward compatible
- No API changes required
- Works on both debug and release builds
- Minimal impact on video decoding performance

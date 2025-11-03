# Video Audio Choppy Playback - Proposed Fixes

## Problem Analysis

The current video audio playback is choppy due to several architectural issues:

### Issue 1: Small Chunk Playback with Gaps
- Audio samples are provided in small chunks (max 2048 samples ~= 46ms)
- Each chunk is played as a separate Mix_Chunk on channel 4
- The code skips audio if previous chunk is still playing: `return false` when `Mix_Playing(videoAudioChannel)`
- This creates gaps in audio and clicking sounds

### Issue 2: Inefficient Audio Buffer Management
- `pbvGetAudioSamples()` copies and shifts samples in the accumulator every call
- Only provides 2048 samples at a time, forcing frequent calls
- `memmove()` operations on every audio retrieval are expensive

### Issue 3: Poor Audio/Video Coupling
- Audio is requested once per video frame update
- Video frame rate (e.g., 30fps = 33ms) doesn't align with audio buffer needs
- Audio needs continuous streaming independent of video timing

## Proposed Solutions

### Solution 1: Queue-Based Audio Streaming (RECOMMENDED)
Instead of playing small chunks, use SDL_mixer's queue system or implement a larger circular buffer:

**Changes needed:**
1. Increase audio accumulator size significantly (e.g., 88200 samples = 1 second at 44.1kHz stereo)
2. Keep a larger audio buffer and only queue audio when buffer gets low
3. Use Mix_SetPostMix() callback or a dedicated audio thread for smooth playback
4. Decouple audio decoding from video frame updates

**Benefits:**
- Smooth continuous audio without gaps
- Better audio/video sync
- Reduced overhead from frequent chunk creation

### Solution 2: Increase Chunk Size and Overlap
A simpler fix that requires minimal changes:

**Changes needed:**
1. Increase audio chunk size to 8192 or 16384 samples (185ms - 370ms)
2. Queue next chunk before current one finishes (overlap buffering)
3. Use Mix_SetPostMix or check remaining audio time instead of just `Mix_Playing()`

**Benefits:**
- Easier to implement
- Reduces chunk creation overhead
- Better overlap handling

### Solution 3: Continuous Audio Buffer with Callback
Use SDL_mixer's audio callback system for continuous streaming:

**Changes needed:**
1. Create a large ring buffer for audio samples
2. Implement SDL audio callback that fills from ring buffer
3. Video decoder continuously fills ring buffer in background
4. Remove per-frame audio chunking

**Benefits:**
- Professional audio streaming approach
- Eliminates gaps completely
- True continuous audio

## Recommended Implementation: Solution 2 (Quick Fix)

Here are the specific code changes needed:

### Change 1: Increase audio buffer constants in PBVideo.h
```cpp
// Current values
#define AUDIO_BUFFER_SIZE 8192          // Increase to 32768
#define AUDIO_ACCUMULATOR_SIZE 16384    // Increase to 131072 (3 seconds of stereo audio)
#define AUDIO_BUFFER_FRAMES 1024        // Increase to 4096
```

### Change 2: Modify pbvGetAudioSamples() to provide larger chunks
```cpp
// In pbvGetAudioSamples(), change:
int samplesToProvide = std::min(audioSamplesAvailable, 2048);  
// To:
int samplesToProvide = std::min(audioSamplesAvailable, 8192); // ~185ms chunks
```

### Change 3: Fill audio buffer more aggressively in pbvUpdateFrame()
```cpp
// Change from:
while (audioAccumulatorIndex < AUDIO_ACCUMULATOR_SIZE * 0.5f) {
// To:
while (audioAccumulatorIndex < AUDIO_ACCUMULATOR_SIZE * 0.75f) { // Keep buffer fuller
```

### Change 4: Fix audio chunk queueing in PBSound.cpp
```cpp
bool PBSound::pbsPlayVideoAudio(const float* audioSamples, int numSamples, int sampleRate) {
    // Instead of skipping if playing, check if we're near the end
    if (videoAudioActive && videoAudioChannel != -1) {
        if (Mix_Playing(videoAudioChannel)) {
            // Check if current chunk is almost done (less than 50ms left)
            // If so, allow queueing next chunk for smooth transition
            // For now, we'll just not skip - let it queue on next available slot
            // This might cause slight overlap but prevents gaps
        } else {
            // Previous chunk finished, clean it up
            pbsStopVideoAudio();
        }
    }
    
    // ... rest of function
}
```

### Change 5: Consider using multiple audio channels for overlap
```cpp
// Use 2 channels alternating (channel 4 and 5) to allow overlap
// Modify pbsPlayVideoAudio to alternate between channels:
videoAudioChannel = Mix_PlayChannel((videoAudioToggle ? 4 : 5), videoAudioChunk, 0);
videoAudioToggle = !videoAudioToggle;
```

## Long-term Solution: Implement Solution 1 or 3

For professional-quality audio, eventually implement a proper audio streaming system with:
- Large ring buffer (1-2 seconds of audio)
- Separate audio decoding thread or async decoding
- SDL_mixer callback for continuous buffer filling
- Proper audio/video drift correction

## Testing

After implementing fixes, test with:
1. Various video formats (different frame rates: 24, 30, 60 fps)
2. Different audio sample rates
3. Monitor audio buffer levels
4. Check for audio/video sync drift over time
5. Test on both Windows and Raspberry Pi

## Expected Results

After implementing Solution 2:
- Smoother audio with minimal gaps
- Reduced clicking/popping sounds
- Better audio/video synchronization
- Slightly higher memory usage (acceptable tradeoff)

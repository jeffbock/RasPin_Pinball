# Audio Streaming Redesign v4 - v0.5.155

## Problem Addressed
User reported: "Audio is very choppy" despite perfect video synchronization.

## Root Cause Analysis
Previous attempts used manual dual-channel management with per-frame audio queueing:
- 46ms chunks queued every 33ms (30fps) caused channel saturation
- Both channels became busy on frame 3, forcing audio skips
- Manual chunk management fought against SDL_mixer's design

## Solution: SDL_mixer Callback-Based Streaming

### Key Architectural Changes:

1. **Automatic Callback System**
   - Uses `Mix_ChannelFinished(callback)` to queue next chunk automatically
   - When current chunk finishes playing, SDL_mixer calls our callback
   - Callback pulls next chunk from video buffer and plays it immediately
   - Creates seamless continuous audio stream

2. **Single Dedicated Channel**
   - Switched from dual-channel alternating (channels 4 & 5) to single channel (4)
   - No more manual channel management or toggle logic
   - SDL_mixer handles all timing internally

3. **Pre-filled Audio Buffer**
   - `pbvPlay()` now pre-fills audio buffer before starting playback
   - Ensures audio data is ready when streaming starts
   - Prevents "no audio available" on first callback

4. **Fixed Chunk Size**
   - Uses consistent 2048 sample chunks (46ms at 44.1kHz)
   - Works for any video FPS - audio runs independently
   - SDL_mixer handles timing automatically

### Implementation Details:

**PBSound.h:**
- Added `pbsStartVideoAudioStream()` - initializes streaming
- Added `pbsSetVideoAudioProvider(PBVideo*)` - sets video source
- Added static callback `channelFinishedCallback()`
- Added `handleChannelFinished()` - pulls and queues next chunk
- Removed dual-channel arrays and toggle logic

**PBSound.cpp:**
- `pbsInitialize()` registers `Mix_ChannelFinished()` callback
- `pbsStartVideoAudioStream()` pre-queues first chunk manually
- `handleChannelFinished()` automatically queues subsequent chunks
- Callback pulls audio via `videoProvider->pbvGetAudioSamples()`

**PBVideo.h/cpp:**
- Added `pbvGetAudioSamples(float* buffer, int requestedSamples)` overload
- `pbvPlay()` now pre-fills audio buffer to 50% capacity
- Ensures audio data ready before streaming starts

**PBVideoPlayer.cpp:**
- `pbvpPlay()` calls `pbsSetVideoAudioProvider()` and `pbsStartVideoAudioStream()`
- Removed per-frame audio queueing from update loop
- Audio now handled automatically by SDL callback

### How It Works:

```
1. User calls pbvpPlay()
   └─> pbvPlay() pre-fills audio buffer
   └─> pbsSetVideoAudioProvider(&m_video)
   └─> pbsStartVideoAudioStream()
       └─> Manually queues first 2048-sample chunk
       └─> Mix_PlayChannel(4, chunk, 0)

2. SDL_mixer plays chunk on channel 4 (46ms duration)

3. Chunk finishes playing
   └─> SDL_mixer automatically calls channelFinishedCallback()
       └─> handleChannelFinished(4)
           └─> Pull next 2048 samples from videoProvider
           └─> Create new chunk
           └─> Mix_PlayChannel(4, chunk, 0)
           
4. Process repeats seamlessly
   └─> Continuous audio with no gaps!
```

### Benefits:

✅ **Smooth Audio** - SDL_mixer handles timing like it does for music  
✅ **No Manual Management** - Callback does everything automatically  
✅ **No Gaps** - Next chunk queued instantly when previous finishes  
✅ **FPS Independent** - Works at any video frame rate  
✅ **Simpler Code** - Removed complex dual-channel logic  
✅ **Proven Approach** - Same method used for smooth music playback  

## Testing Notes:

This version uses the same streaming approach that makes music playback smooth in SDL_mixer. Audio should now play continuously without choppiness, regardless of video frame rate.

Build: v0.5.155
Date: November 2, 2025

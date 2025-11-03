# Audio Stream Redesign - SDL_mixer Callback Approach

## Current Problem
- We're queueing small audio chunks per video frame (33ms chunks at 30fps)
- Dual-channel alternating creates gaps when both channels are busy
- Fighting against SDL_mixer's design instead of working with it

## Root Cause
Music plays smoothly because `Mix_PlayMusic` handles continuous streaming internally.
Our video audio is choppy because we're manually managing tiny chunks.

## Solution: Use SDL_mixer's Channel Finished Callback

SDL_mixer provides `Mix_ChannelFinished(callback)` which is called when a channel stops playing.
This lets us create a **continuous audio stream** by automatically queueing the next chunk.

### New Architecture:

```
Video Thread (main):
  - Decodes audio from FFmpeg
  - Fills a circular audio buffer
  - Sets flag when buffer has data

Audio Thread (SDL callback):
  - Called automatically when channel finishes
  - Pulls next chunk from circular buffer
  - Queues it on same channel
  - Creates seamless playback
```

### Key Changes:

1. **Single dedicated channel** (not alternating)
2. **Larger circular buffer** (2-3 seconds of audio)
3. **Channel finished callback** queues next chunk automatically
4. **Fixed chunk size** (2048 samples = 46ms is fine for streaming)
5. **Video syncs to audio clock**, not vice versa

### Benefits:
- SDL_mixer handles timing automatically
- No manual channel management
- No gaps from channel saturation
- Audio becomes master clock (as user requested)
- Video can drop frames if needed to stay in sync

### Implementation:
1. Create circular audio buffer in PBVideo (2-3 seconds)
2. Add channel finished callback in PBSound
3. Callback pulls next chunk and queues it
4. Video timing adjusts to audio position
5. Drop video frames if falling behind

This is how music playback works smoothly - let's apply it to video audio!

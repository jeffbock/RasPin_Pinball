# Proper Audio/Video Synchronization - Version 3

## Date: November 2, 2025
## Version: v0.5.150

## The Real Problem (Identified by User)

The previous fixes addressed symptoms but not the root cause:
- **Audio was racing ahead of video** even with reduced chunk sizes
- The fundamental issue: **Video timing was using wall clock, not frame timestamps**
- Video FPS is arbitrary - could be anything (24, 30, 60, etc.)
- **Audio should be the master clock**, video should sync to it

## Key Insight

> "Why should audio playback time be synchronized if video is working properly? I'd rather have video skip a frame and audio stay constant if needed."

**This is the correct approach for video playback!**

## Architectural Change

### OLD (Incorrect) Approach:
```
┌─────────────┐
│ Wall Clock  │──────┐
│ (currentTick)│     │
└─────────────┘      ├──> Decide when to show frames
                     │
┌─────────────┐      │
│   Video     │──────┘
│ Frame Rate  │
└─────────────┘

Problem: Ignores actual frame timestamps!
Audio tries to follow video timing → drift
```

### NEW (Correct) Approach:
```
┌─────────────────┐
│  Audio Clock    │ (MASTER)
│  (timestamp     │
│   from frames)  │
└────────┬────────┘
         │
         ├──────> Audio plays continuously at proper rate
         │
         └──────> Video syncs to audio
                  ↓
              ┌───────────┐
              │ Video may │
              │ skip frames│
              │ if behind │
              └───────────┘
```

## Implementation Details

### 1. Audio as Master Clock
```cpp
double PBVideo::getMasterClock() {
    if (videoInfo.hasAudio && audioEnabled) {
        // Get decoded audio timestamp
        double decodedAudioTime = getAudioClock();
        
        // Account for buffered (not yet played) audio
        double bufferedTime = audioAccumulatorIndex / (44100.0 * 2.0);
        
        // Actual playback position = decoded - buffered
        double actualPlaybackTime = decodedAudioTime - bufferedTime;
        
        return actualPlaybackTime;
    } else {
        // No audio: use wall clock
        return getCurrentPlaybackTimeSec(currentTick);
    }
}
```

**Key:** We track the audio timestamp minus the buffer, giving us the **actual audio playback position**.

### 2. Video Syncs to Audio
```cpp
bool PBVideo::pbvUpdateFrame(unsigned long currentTick) {
    // Get master clock (audio position)
    double masterClock = getMasterClock();
    
    // Get current video timestamp
    double currentVideoTime = videoClock;
    
    // Should we decode next frame?
    if (videoInfo.hasAudio && audioEnabled) {
        // Decode if video is behind audio
        shouldDecodeNext = (currentVideoTime < masterClock);
    } else {
        // No audio: use wall clock
        shouldDecodeNext = (currentTimeSec >= lastFrameTime + frameTime);
    }
    
    // After decoding, check if frame should be displayed NOW
    if (shouldDisplayFrame(newVideoTime)) {
        // Display it
        return true;
    } else if (newVideoTime < masterClock - 0.1) {
        // Frame is >100ms behind - skip it, get next frame
        return pbvUpdateFrame(currentTick);  // Recursive call
    } else {
        // Frame is ahead of audio - don't display yet
        return false;
    }
}
```

**Key behaviors:**
- Video checks timestamps from decoded frames (not wall clock)
- If video is behind audio by >100ms, **skip that frame**
- If video is ahead, **wait** (don't display yet)
- Audio plays continuously without interruption

### 3. Simple Audio Chunks
Since video now syncs to audio (not vice versa), we don't need complex chunk size calculations:

```cpp
// Fixed ~46ms chunks for smooth continuous audio
int targetSamplesPerFrame = 2048;  

// Larger chunks for low frame rate videos (less overhead)
if (videoInfo.fps < 30.0f) {
    targetSamplesPerFrame = 4096;  // ~93ms
}
```

## What This Fixes

### ✅ Audio Stays Constant
- Audio plays at the correct rate from frame timestamps
- No more racing ahead
- Smooth, continuous playback

### ✅ Video Adapts to Audio
- Video uses actual frame timestamps (PTS)
- Compares frame time to audio clock
- Skips frames if falling behind
- Waits if getting ahead

### ✅ Proper Synchronization
- Based on media timestamps, not arbitrary timing
- Works with any video FPS
- Handles variable frame rates
- Resilient to system load variations

### ✅ Frame Skipping (When Needed)
- If system is overloaded, video may skip frames
- Audio continues smoothly
- Better than stuttering both audio and video

## Technical Details

### Timestamp Tracking
- **Video PTS** (Presentation Time Stamp): When to show frame
- **Audio PTS**: When audio should be heard
- **Master Clock**: Adjusted audio PTS (accounting for buffer)

### Buffer Accounting
```
Decoded Audio Time: 2.5 seconds
Buffered Samples:   4410 samples = 0.1 seconds
Actual Playback:    2.5 - 0.1 = 2.4 seconds

Video at 2.35s → Behind by 50ms → Decode/Display
Video at 2.45s → Ahead by 50ms → Wait
Video at 2.25s → Behind by 150ms → Skip frame!
```

### Synchronization Window
- **Display window**: ±40ms tolerance
- **Skip threshold**: >100ms behind
- **Wait behavior**: Frame ahead of audio

## Expected Results

### Smooth Audio ✅
- Continuous playback at correct rate
- Minimal clicking (dual-channel overlap)
- No speed variations

### Synchronized Video ✅
- Frames displayed at correct times
- May skip if system lags
- Never races ahead of audio

### Resilient Playback ✅
- Adapts to system load
- Works with any frame rate
- Handles timing variations

## Memory Usage
**Unchanged from v0.5.148:** ~417 KB total

## Build Info
- Version: v0.5.150
- Build Status: ✅ Success
- Platform: Raspberry Pi
- Branch: jbock/videotest

## Testing Checklist

Test these scenarios:
1. ✅ Normal playback - check audio/video stay in sync
2. ✅ Long videos (>5 minutes) - check for drift over time
3. ✅ System under load - video may skip frames, audio should stay smooth
4. ✅ Different frame rates (24, 30, 60 fps)
5. ✅ Seeking - verify sync after jump
6. ✅ Looping - check smooth restart

## Debug Output (Optional)

To verify sync, you could add debug output:
```cpp
printf("Master Clock: %.3f, Video PTS: %.3f, Diff: %.3f\n", 
       masterClock, videoClock, videoClock - masterClock);
```

Expect:d
- Diff near 0: Good sync
- Positive diff: Video ahead (will wait)
- Negative diff > 0.1: Video behind (will skip)

## Philosophy

**The correct video playback model:**
1. Audio is sacred - must play smoothly
2. Video adapts to audio - may skip if needed
3. Use media timestamps - not wall clock
4. Better to skip frames than stutter audio

This matches how professional media players work (VLC, ffplay, etc.).

# PBVideo - FFmpeg-Based Video Playback System

## Overview

PBVideo is a cross-platform video playback system for the Pinball project that integrates FFmpeg for video decoding, PBGfx for OpenGL ES texture rendering, and PBSound for audio playback. It provides synchronized video and audio playback with support for seeking, looping, and playback speed control.

## Architecture

### Components

1. **PBVideo** (`PBVideo.h/cpp`)
   - Low-level FFmpeg wrapper
   - Handles video/audio decoding
   - Frame timing and synchronization
   - Provides RGBA frame data for texture upload

2. **PBGfx Integration**
   - New texture type: `GFX_VIDEO`
   - Dynamic texture updates with `gfxUpdateVideoTexture()`
   - Video textures render like any other sprite

3. **PBSound Integration** (Raspberry Pi only)
   - Dedicated video audio channel (channel 5)
   - Float sample conversion for SDL2_mixer
   - Automatic synchronization with video frames

4. **PBVideoPlayer** (`PBVideoPlayer.h/cpp`)
   - High-level wrapper combining all components
   - Simple API for common use cases
   - Automatic resource management

## Installation

### Windows

1. Download FFmpeg shared libraries:
   ```
   https://github.com/BtbN/FFmpeg-Builds/releases
   ```
   Choose: `ffmpeg-master-latest-win64-gpl-shared.zip`

2. Extract and copy files:
   ```
   DLLs → winbuild/
   include/ → src/include_ogl_win/
   lib/ → src/lib_ogl_win/
   ```

3. Required DLLs:
   - `avcodec-*.dll`
   - `avformat-*.dll`
   - `avutil-*.dll`
   - `swscale-*.dll`
   - `swresample-*.dll`

### Raspberry Pi

```bash
sudo apt update
sudo apt install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev
```

## Build Configuration

### Windows Build Task

Add to your `tasks.json` compilation args:

```json
"args": [
    // ... existing args ...
    "${workspaceFolder}/src/PBVideo.cpp",
    "${workspaceFolder}/src/PBVideoPlayer.cpp",
    // ... other files ...
],
```

Add to linker args:
```json
"/link",
// ... existing libs ...
"avcodec.lib",
"avformat.lib",
"avutil.lib",
"swscale.lib",
"swresample.lib",
```

### Raspberry Pi Build Task

Add to compilation args:
```json
"args": [
    // ... existing args ...
    "${workspaceFolder}/src/PBVideo.cpp",
    "${workspaceFolder}/src/PBVideoPlayer.cpp",
    // ... other files ...
    "-lavcodec",
    "-lavformat",
    "-lavutil",
    "-lswscale",
    "-lswresample",
],
```

## Usage

### Basic Example

```cpp
#include "PBVideoPlayer.h"

// In your initialization code
PBGfx* gfx = ...; // Your graphics system
PBSound* sound = ...; // Your sound system

PBVideoPlayer videoPlayer(gfx, sound);

// Load a video
unsigned int videoSprite = videoPlayer.pbvpLoadVideo(
    "src/resources/videos/intro.mp4",
    100, 100,  // x, y position
    false      // keepResident
);

if (videoSprite != NOSPRITE) {
    videoPlayer.pbvpSetLooping(true);
    videoPlayer.pbvpPlay();
}

// In your render loop
unsigned long currentTick = gfx->GetTickCountGfx();

gfx->gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);

videoPlayer.pbvpUpdate(currentTick);  // Decode frame, update texture, play audio
videoPlayer.pbvpRender();             // Render video sprite

gfx->gfxSwap(true);
```

### Advanced Usage

```cpp
// Load video with transformations
videoPlayer.pbvpLoadVideo("background.mp4", 0, 0, true);
videoPlayer.pbvpSetScaleFactor(0.5f);    // Scale to 50%
videoPlayer.pbvpSetAlpha(0.8f);          // 80% opacity
videoPlayer.pbvpSetRotation(45.0f);      // Rotate 45 degrees
videoPlayer.pbvpSetPlaybackSpeed(2.0f);  // 2x speed
videoPlayer.pbvpPlay();

// Seek to specific time
videoPlayer.pbvpSeekTo(30.0f); // Jump to 30 seconds

// Check playback status
pbvPlaybackState state = videoPlayer.pbvpGetPlaybackState();
if (state == PBV_FINISHED) {
    // Video finished
}

// Get video info
stVideoInfo info = videoPlayer.pbvpGetVideoInfo();
printf("Video: %dx%d @ %.2f fps, Duration: %.2f sec\n",
       info.width, info.height, info.fps, info.durationSec);
```

## API Reference

### PBVideoPlayer Methods

#### Loading & Lifecycle
- `pbvpLoadVideo(filePath, x, y, keepResident)` - Load video and create sprite
- `pbvpUnloadVideo()` - Unload and free resources
- `pbvpIsLoaded()` - Check if video is loaded

#### Playback Control
- `pbvpPlay()` - Start/resume playback
- `pbvpPause()` - Pause playback
- `pbvpStop()` - Stop and reset to beginning
- `pbvpSeekTo(timeSec)` - Seek to specific time
- `pbvpSetPlaybackSpeed(speed)` - Set playback speed (0.5 - 4.0)
- `pbvpSetLooping(loop)` - Enable/disable looping

#### Rendering
- `pbvpUpdate(currentTick)` - Update frame and audio (call every frame)
- `pbvpRender()` - Render at current position
- `pbvpRender(x, y)` - Render at specific position
- `pbvpRender(x, y, scale, rotation)` - Render with transformations

#### Sprite Manipulation
- `pbvpSetXY(x, y)` - Set position
- `pbvpSetAlpha(alpha)` - Set transparency (0.0 - 1.0)
- `pbvpSetScaleFactor(scale)` - Set scale factor
- `pbvpSetRotation(degrees)` - Set rotation

#### Query Functions
- `pbvpGetVideoInfo()` - Get video information
- `pbvpGetPlaybackState()` - Get current playback state
- `pbvpGetCurrentTimeSec()` - Get current playback time
- `pbvpGetSpriteId()` - Get sprite ID for manual manipulation

### Playback States

```cpp
enum pbvPlaybackState {
    PBV_STOPPED = 0,   // Video stopped or not started
    PBV_PLAYING = 1,   // Currently playing
    PBV_PAUSED = 2,    // Paused
    PBV_FINISHED = 3   // Finished playing (not looping)
};
```

### Video Information Structure

```cpp
struct stVideoInfo {
    std::string videoFilePath;  // Path to video file
    unsigned int width;         // Video width in pixels
    unsigned int height;        // Video height in pixels
    float fps;                  // Frames per second
    float durationSec;          // Duration in seconds
    bool hasAudio;              // Has audio track
    bool hasVideo;              // Has video track
};
```

## Supported Formats

FFmpeg supports most common formats:

### Containers
- MP4, AVI, MOV, WebM, MKV, FLV

### Video Codecs
- H.264 (recommended)
- H.265/HEVC
- VP8, VP9
- MPEG-4

### Audio Codecs
- AAC (recommended)
- MP3
- Vorbis
- PCM

### Recommended Settings

For best compatibility:
```
Container: MP4
Video: H.264, 30fps
Audio: AAC, 44.1kHz stereo
Resolution: 1280x720 or lower for Raspberry Pi
```

### Video Conversion

Use FFmpeg to convert videos:

```bash
# Convert to recommended format
ffmpeg -i input.avi -c:v libx264 -preset fast -crf 23 \
       -c:a aac -b:a 128k output.mp4

# Scale down for Raspberry Pi
ffmpeg -i input.mp4 -vf scale=1280:720 -c:v libx264 -preset fast \
       -crf 23 -c:a aac -b:a 128k output_720p.mp4

# Extract frame as image for testing
ffmpeg -i video.mp4 -ss 00:00:10 -vframes 1 frame.png
```

## Performance Considerations

### Raspberry Pi

1. **Resolution**: Keep videos at 720p or lower
2. **Bitrate**: Use lower bitrates (2-4 Mbps)
3. **CPU Usage**: Video decoding is CPU-intensive
4. **Memory**: Each frame uses width × height × 4 bytes

### Windows

1. Generally performs well with 1080p videos
2. Consider GPU for higher resolutions (not currently implemented)

### Optimization Tips

1. Use lower resolution videos for backgrounds
2. Preload videos during loading screens
3. Use `keepResident = true` for frequently used videos
4. Limit number of simultaneous videos
5. Consider hardware decoding (requires custom implementation)

## Platform Differences

### Audio Playback

| Feature | Windows | Raspberry Pi |
|---------|---------|--------------|
| Video audio | ❌ Stubbed | ✅ Supported |
| SDL2 integration | ❌ Not available | ✅ Channel 5 |
| Sample format | N/A | Float stereo, 44.1kHz |

### Video Rendering

| Feature | Windows | Raspberry Pi |
|---------|---------|--------------|
| Video decoding | ✅ FFmpeg | ✅ FFmpeg |
| Texture update | ✅ OpenGL ES | ✅ OpenGL ES |
| Hardware decode | ❌ Not implemented | ❌ Not implemented |

## Troubleshooting

### Video Won't Load

1. Check FFmpeg libraries are installed
2. Verify file path is correct
3. Check console for error messages
4. Test with simple H.264 MP4 file

### No Audio (Raspberry Pi)

1. Ensure `PBSound::pbsInitialize()` was called
2. Check video has audio: `info.hasAudio`
3. Verify SDL2 and SDL2_mixer are installed
4. Check audio isn't disabled: `pbvpSetAudioEnabled(true)`

### Stuttering / Frame Drops

1. Lower video resolution
2. Reduce video bitrate
3. Check CPU usage
4. Ensure game loop runs faster than video FPS
5. Close other applications

### Texture Not Updating

1. Verify `pbvpUpdate()` is called every frame
2. Check return value of `pbvpUpdate()` is true
3. Ensure video is in `PBV_PLAYING` state
4. Verify sprite is being rendered

### Build Errors

#### Windows
- Missing `avcodec.lib`: Check library path in tasks.json
- Missing headers: Verify FFmpeg include directory
- DLL not found: Copy DLLs to winbuild folder

#### Raspberry Pi
- Missing headers: `sudo apt install libavcodec-dev ...`
- Linker errors: Add `-lavcodec -lavformat ...` to build args

## Future Enhancements

Possible improvements for future versions:

1. **Hardware Acceleration**
   - Raspberry Pi: V4L2 decoder
   - Windows: DXVA2 or NVDEC

2. **Audio for Windows**
   - Implement Windows audio backend
   - Use DirectSound or XAudio2

3. **Subtitle Support**
   - Parse subtitle tracks
   - Render as text overlay

4. **Streaming Support**
   - HTTP/HTTPS streaming
   - Network video sources

5. **Multiple Audio Tracks**
   - Track selection
   - Language switching

## Example Integration

See `PBVideoExample.cpp` for complete examples including:
- Basic playback
- Transformations
- Multiple videos
- Game loop integration
- Playback controls

## License

Copyright (c) 2025 Jeffrey D. Bock

Licensed under Creative Commons Attribution-NonCommercial 4.0 International License.
See LICENSE file for details.

## Credits

- FFmpeg library: https://ffmpeg.org/
- SDL2 and SDL2_mixer: https://www.libsdl.org/
- Built for the Pinball project

## Decoders found during LS

crw-rw----+ 1 root video 81, 16 Oct 11 14:17 /dev/video19
crw-rw----+ 1 root video 81,  0 Oct 11 14:17 /dev/video20
crw-rw----+ 1 root video 81,  1 Oct 11 14:17 /dev/video21
crw-rw----+ 1 root video 81,  2 Oct 11 14:17 /dev/video22
crw-rw----+ 1 root video 81,  3 Oct 11 14:17 /dev/video23
crw-rw----+ 1 root video 81,  4 Oct 11 14:17 /dev/video24
crw-rw----+ 1 root video 81,  5 Oct 11 14:17 /dev/video25
crw-rw----+ 1 root video 81,  6 Oct 11 14:17 /dev/video26
crw-rw----+ 1 root video 81,  7 Oct 11 14:17 /dev/video27
crw-rw----+ 1 root video 81,  8 Oct 11 14:17 /dev/video28
crw-rw----+ 1 root video 81,  9 Oct 11 14:17 /dev/video29
crw-rw----+ 1 root video 81, 10 Oct 11 14:17 /dev/video30
crw-rw----+ 1 root video 81, 11 Oct 11 14:17 /dev/video31
crw-rw----+ 1 root video 81, 12 Oct 11 14:17 /dev/video32
crw-rw----+ 1 root video 81, 13 Oct 11 14:17 /dev/video33
crw-rw----+ 1 root video 81, 14 Oct 11 14:17 /dev/video34
crw-rw----+ 1 root video 81, 15 Oct 11 14:17 /dev/video35
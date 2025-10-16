// Example usage of PBVideoPlayer
// Copyright (c) 2025 Jeffrey D. Bock
// This file demonstrates how to integrate video playback into your pinball application

/*

SETUP INSTRUCTIONS:

1. Install FFmpeg libraries:

   WINDOWS:
   - Download FFmpeg shared libraries from https://github.com/BtbN/FFmpeg-Builds/releases
   - Extract and copy the following DLLs to your winbuild folder:
     * avcodec-*.dll
     * avformat-*.dll
     * avutil-*.dll
     * swscale-*.dll
     * swresample-*.dll
   - Copy include files to src/include_ogl_win/libav*/
   - Copy lib files to src/lib_ogl_win/

   RASPBERRY PI:
   ```bash
   sudo apt update
   sudo apt install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev
   ```

2. Add to your build tasks:
   
   Windows (tasks.json):
   Add these source files to the compilation list:
     ${workspaceFolder}/src/PBVideo.cpp
     ${workspaceFolder}/src/PBVideoPlayer.cpp
   
   Add these libraries to the linker:
     avcodec.lib avformat.lib avutil.lib swscale.lib swresample.lib

   Raspberry Pi (tasks.json):
   Add these source files to the compilation list:
     ${workspaceFolder}/src/PBVideo.cpp
     ${workspaceFolder}/src/PBVideoPlayer.cpp
   
   Add these flags to compilation:
     -lavcodec -lavformat -lavutil -lswscale -lswresample

===================================================================================
BASIC USAGE EXAMPLE:
===================================================================================
*/

#include "PBVideoPlayer.h"
#include "PBGfx.h"
#include "PBSound.h"

void BasicVideoExample(PBGfx* gfx, PBSound* sound) {
    
    // Create a video player
    PBVideoPlayer videoPlayer(gfx, sound);
    
    // Load a video file and create a sprite at position (100, 100)
    unsigned int videoSprite = videoPlayer.pbvpLoadVideo("src/resources/videos/intro.mp4", 100, 100, false);
    
    if (videoSprite == NOSPRITE) {
        // Failed to load video
        return;
    }
    
    // Get video information
    stVideoInfo info = videoPlayer.pbvpGetVideoInfo();
    // info.width, info.height, info.fps, info.durationSec, info.hasAudio, info.hasVideo
    
    // Start playback
    videoPlayer.pbvpPlay();
    
    // Set looping (optional)
    videoPlayer.pbvpSetLooping(true);
    
    // Main render loop
    while (videoPlayer.pbvpGetPlaybackState() == PBV_PLAYING) {
        
        // Clear screen
        gfx->gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
        
        // Update video (decodes frame, updates texture, plays audio)
        unsigned long currentTick = gfx->GetTickCountGfx();
        videoPlayer.pbvpUpdate(currentTick);
        
        // Render the video
        videoPlayer.pbvpRender();
        
        // Swap buffers
        gfx->gfxSwap(true);
    }
    
    // Stop and cleanup (automatic on destruction, but can be explicit)
    videoPlayer.pbvpStop();
    videoPlayer.pbvpUnloadVideo();
}

/*
===================================================================================
ADVANCED USAGE EXAMPLE - Video with transformations
===================================================================================
*/

void AdvancedVideoExample(PBGfx* gfx, PBSound* sound) {
    
    PBVideoPlayer videoPlayer(gfx, sound);
    
    // Load video
    unsigned int videoSprite = videoPlayer.pbvpLoadVideo("src/resources/videos/background.mp4", 0, 0, false);
    
    if (videoSprite == NOSPRITE) return;
    
    // Configure video
    videoPlayer.pbvpSetLooping(true);        // Loop continuously
    videoPlayer.pbvpSetPlaybackSpeed(1.0f);  // Normal speed
    videoPlayer.pbvpSetScaleFactor(0.5f);    // Scale to 50%
    videoPlayer.pbvpSetAlpha(0.8f);          // 80% opacity
    
    // Start playback
    videoPlayer.pbvpPlay();
    
    // Render loop
    unsigned long startTime = gfx->GetTickCountGfx();
    
    while (true) {
        unsigned long currentTick = gfx->GetTickCountGfx();
        float elapsedSec = (currentTick - startTime) / 1000.0f;
        
        gfx->gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
        
        // Update video
        videoPlayer.pbvpUpdate(currentTick);
        
        // Rotate the video over time
        float rotation = elapsedSec * 30.0f; // 30 degrees per second
        videoPlayer.pbvpSetRotation(rotation);
        
        // Render with current transformations
        videoPlayer.pbvpRender(400, 300, 0.5f, rotation);
        
        gfx->gfxSwap(true);
        
        // Exit after 10 seconds for demo
        if (elapsedSec > 10.0f) break;
    }
}

/*
===================================================================================
INTEGRATION WITH GAME LOOP
===================================================================================
*/

class PinballGame {
private:
    PBGfx* gfx;
    PBSound* sound;
    PBVideoPlayer* attractModeVideo;
    
public:
    void InitAttractMode() {
        // Create video player for attract mode
        attractModeVideo = new PBVideoPlayer(gfx, sound);
        
        // Load attract mode video
        attractModeVideo->pbvpLoadVideo("src/resources/videos/attract.mp4", 0, 0, true);
        attractModeVideo->pbvpSetLooping(true);
        attractModeVideo->pbvpPlay();
    }
    
    void RenderAttractMode() {
        unsigned long currentTick = gfx->GetTickCountGfx();
        
        // Clear and render background
        gfx->gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
        
        // Update and render video
        if (attractModeVideo) {
            attractModeVideo->pbvpUpdate(currentTick);
            attractModeVideo->pbvpRender();
        }
        
        // Render UI overlays on top of video
        // ... your UI code ...
        
        gfx->gfxSwap(true);
    }
    
    void CleanupAttractMode() {
        if (attractModeVideo) {
            delete attractModeVideo;
            attractModeVideo = nullptr;
        }
    }
};

/*
===================================================================================
VIDEO PLAYBACK CONTROLS
===================================================================================
*/

void VideoControlsExample(PBVideoPlayer* videoPlayer, PBGfx* gfx) {
    
    unsigned long currentTick = gfx->GetTickCountGfx();
    
    // Check playback state
    pbvPlaybackState state = videoPlayer->pbvpGetPlaybackState();
    
    if (state == PBV_PLAYING) {
        // Video is playing
        
        // Get current playback time
        float currentTime = videoPlayer->pbvpGetCurrentTimeSec();
        float duration = videoPlayer->pbvpGetVideoInfo().durationSec;
        float progress = currentTime / duration;
        
        // Seek to 50% of video
        if (progress > 0.5f) {
            videoPlayer->pbvpSeekTo(0.0f); // Jump back to start
        }
        
    } else if (state == PBV_PAUSED) {
        // Video is paused
        videoPlayer->pbvpPlay(); // Resume
        
    } else if (state == PBV_FINISHED) {
        // Video finished
        videoPlayer->pbvpSeekTo(0.0f); // Reset to beginning
        videoPlayer->pbvpPlay();       // Restart
    }
}

/*
===================================================================================
MULTIPLE VIDEOS
===================================================================================
*/

void MultipleVideosExample(PBGfx* gfx, PBSound* sound) {
    
    // Create multiple video players
    PBVideoPlayer background(gfx, sound);
    PBVideoPlayer overlay(gfx, nullptr); // No audio for overlay
    
    // Load videos
    background.pbvpLoadVideo("src/resources/videos/background.mp4", 0, 0, true);
    background.pbvpSetLooping(true);
    background.pbvpPlay();
    
    overlay.pbvpLoadVideo("src/resources/videos/overlay.mp4", 200, 150, false);
    overlay.pbvpSetAudioEnabled(false);
    overlay.pbvpSetAlpha(0.7f);
    overlay.pbvpPlay();
    
    // Render loop
    while (background.pbvpGetPlaybackState() == PBV_PLAYING) {
        unsigned long currentTick = gfx->GetTickCountGfx();
        
        gfx->gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
        
        // Update and render background
        background.pbvpUpdate(currentTick);
        background.pbvpRender();
        
        // Update and render overlay
        overlay.pbvpUpdate(currentTick);
        overlay.pbvpRender();
        
        gfx->gfxSwap(true);
    }
}

/*
===================================================================================
SUPPORTED VIDEO FORMATS
===================================================================================

FFmpeg supports most common video formats:
- MP4 (H.264, H.265)
- AVI
- MOV
- WebM
- MKV
- FLV

Recommended format for best compatibility:
- Container: MP4
- Video codec: H.264
- Audio codec: AAC or MP3
- Resolution: Match your display or scale down
- Frame rate: 30 or 60 fps

Convert videos using FFmpeg command line:
ffmpeg -i input.avi -c:v libx264 -preset fast -crf 23 -c:a aac -b:a 128k output.mp4

===================================================================================
PERFORMANCE NOTES
===================================================================================

1. Video decoding is CPU-intensive. On Raspberry Pi, use lower resolutions (720p or less)

2. Hardware acceleration is not enabled by default. For Raspberry Pi, consider using
   H.264 hardware decoder by modifying PBVideo.cpp to use V4L2 codecs

3. Audio synchronization happens automatically. The video player uses the system tick
   count to ensure frames are displayed at the correct time

4. For smooth playback, ensure your game loop runs at least as fast as the video FPS

5. Memory usage: Each video frame uses width * height * 4 bytes (RGBA)
   Example: 1920x1080 = ~8MB per frame

===================================================================================
TROUBLESHOOTING
===================================================================================

Video won't load:
- Check FFmpeg libraries are installed
- Verify video file path is correct
- Check console output for FFmpeg error messages

No audio:
- Ensure PBSound is initialized (pbsInitialize())
- Check if video has audio track (info.hasAudio)
- Windows: Audio is stubbed out, only works on Raspberry Pi

Video stuttering:
- Lower video resolution
- Reduce playback speed
- Check CPU usage
- Enable hardware decoding (requires code modification)

Texture not updating:
- Ensure you're calling pbvpUpdate() every frame
- Check that pbvpUpdate() returns true
- Verify the sprite is being rendered

*/

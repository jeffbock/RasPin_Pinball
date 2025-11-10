// PBVideoPlayer - High-level video player that integrates PBVideo, PBGfx, and PBSound
// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef PBVideoPlayer_h
#define PBVideoPlayer_h

#include "PBVideo.h"
#include "PBGfx.h"
#include "PBSound.h"
#include <string>

// Forward declaration
class PBEngine;

// Video player wrapper that combines video, graphics, and sound
class PBVideoPlayer {
public:
    PBVideoPlayer(PBGfx* gfx, PBSound* sound);
    ~PBVideoPlayer();
    
    // Load a video and create a sprite for it
    // Returns sprite ID on success, NOSPRITE on failure
    unsigned int pbvpLoadVideo(const std::string& videoFilePath, int x, int y, bool keepResident = false);
    
    // Unload the video and free all resources
    void pbvpUnloadVideo();
    
    // Start or resume playback
    bool pbvpPlay();
    
    // Pause playback
    void pbvpPause();
    
    // Stop playback and reset to beginning
    void pbvpStop();
    
    // Update and render the video (call this every frame)
    // Pass in the current tick count from your game loop
    // Returns true if playing, false if stopped/finished
    bool pbvpUpdate(unsigned long currentTick);
    
    // Render the video sprite at its current position
    bool pbvpRender();
    
    // Render the video sprite at a specific position
    bool pbvpRender(int x, int y);
    
    // Render with scale and rotation
    bool pbvpRender(int x, int y, float scaleFactor, float rotateDegrees);
    
    // Query functions
    unsigned int pbvpGetSpriteId() const { return videoSpriteId; }
    stVideoInfo pbvpGetVideoInfo() const;
    pbvPlaybackState pbvpGetPlaybackState() const;
    float pbvpGetCurrentTimeSec() const;
    bool pbvpIsLoaded() const;
    
    // Control functions
    bool pbvpSeekTo(float timeSec);
    void pbvpSetPlaybackSpeed(float speed);
    void pbvpSetLooping(bool loop);
    void pbvpSetAudioEnabled(bool enabled);
    
    // Sprite manipulation (forwards to PBGfx)
    void pbvpSetXY(int x, int y);
    void pbvpSetAlpha(float alpha);
    void pbvpSetScaleFactor(float scale);
    void pbvpSetRotation(float degrees);
    
private:
    PBGfx* m_gfx;
    PBSound* m_sound;
    PBVideo m_video;
    
    unsigned int videoSpriteId;
    bool videoLoaded;
    bool audioEnabled;
};

#endif // PBVideoPlayer_h

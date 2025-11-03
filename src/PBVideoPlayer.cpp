// PBVideoPlayer - High-level video player implementation
// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "PBVideoPlayer.h"
#include <sstream>

PBVideoPlayer::PBVideoPlayer(PBGfx* gfx, PBSound* sound) {
    m_gfx = gfx;
    m_sound = sound;
    videoSpriteId = NOSPRITE;
    videoLoaded = false;
    audioEnabled = true;
    
    // Initialize video system
    m_video.pbvInitialize();
}

PBVideoPlayer::~PBVideoPlayer() {
    pbvpUnloadVideo();
    m_video.pbvShutdown();
}

unsigned int PBVideoPlayer::pbvpLoadVideo(const std::string& videoFilePath, int x, int y, bool keepResident) {
    // Unload any existing video
    pbvpUnloadVideo();
    
    // Load the video file
    if (!m_video.pbvLoadVideo(videoFilePath)) {
        return NOSPRITE;
    }
    
    // Get video information
    stVideoInfo info = m_video.pbvGetVideoInfo();
    
    if (!info.hasVideo) {
        m_video.pbvUnloadVideo();
        return NOSPRITE;
    }
    
    // Create a video sprite with dimensions matching the video
    // The "filename" for video textures is in the format "widthxheight"
    std::ostringstream dimensions;
    dimensions << info.width << "x" << info.height;
    
    videoSpriteId = m_gfx->gfxLoadSprite(
        "VideoSprite_" + videoFilePath,
        dimensions.str(),
        GFX_VIDEO,
        GFX_NOMAP,
        GFX_UPPERLEFT,
        keepResident,
        true
    );
    
    if (videoSpriteId == NOSPRITE) {
        m_video.pbvUnloadVideo();
        return NOSPRITE;
    }
    
    // Set initial position
    m_gfx->gfxSetXY(videoSpriteId, x, y, false);
    
    videoLoaded = true;
    audioEnabled = info.hasAudio;
    
    return videoSpriteId;
}

void PBVideoPlayer::pbvpUnloadVideo() {
    if (videoLoaded) {
        pbvpStop();
        
        if (videoSpriteId != NOSPRITE) {
            m_gfx->gfxUnloadTexture(videoSpriteId);
            videoSpriteId = NOSPRITE;
        }
        
        m_video.pbvUnloadVideo();
        videoLoaded = false;
    }
}

bool PBVideoPlayer::pbvpPlay() {
    if (!videoLoaded) {
        return false;
    }
    
    // Start the video
    bool success = m_video.pbvPlay();
    
    // Start audio streaming if we have sound and audio is enabled
    if (success && m_sound && audioEnabled) {
        m_sound->pbsSetVideoAudioProvider(&m_video);
        m_sound->pbsStartVideoAudioStream();
    }
    
    return success;
}

void PBVideoPlayer::pbvpPause() {
    if (videoLoaded) {
        m_video.pbvPause();
    }
}

void PBVideoPlayer::pbvpStop() {
    if (videoLoaded) {
        m_video.pbvStop();
        
        // Stop video audio if playing
        if (m_sound && audioEnabled) {
            m_sound->pbsStopVideoAudio();
        }
    }
}

bool PBVideoPlayer::pbvpUpdate(unsigned long currentTick) {
    if (!videoLoaded) {
        return false;
    }
    
    pbvPlaybackState state = m_video.pbvGetPlaybackState();
    
    // Check if we're still playing
    if (state != PBV_PLAYING) {
        return false;
    }
    
    // Update video frame
    bool newFrame = m_video.pbvUpdateFrame(currentTick);
    
    // Check if video just looped - restart audio stream for clean loop
    if (m_sound && audioEnabled && m_video.pbvDidJustLoop()) {
        m_sound->pbsRestartVideoAudioStream();
    }
    
    if (newFrame) {
        // Get the new frame data
        unsigned int frameWidth, frameHeight;
        const uint8_t* frameData = m_video.pbvGetFrameData(&frameWidth, &frameHeight);
        
        if (frameData) {
            // Update the video texture
            m_gfx->gfxUpdateVideoTexture(videoSpriteId, frameData, frameWidth, frameHeight);
        }
        
        // Note: Audio is now handled automatically via SDL_mixer callback streaming
        // No need to queue audio per-frame anymore
    }
    
    return true;
}

bool PBVideoPlayer::pbvpRender() {
    if (!videoLoaded || videoSpriteId == NOSPRITE) {
        return false;
    }
    
    return m_gfx->gfxRenderSprite(videoSpriteId);
}

bool PBVideoPlayer::pbvpRender(int x, int y) {
    if (!videoLoaded || videoSpriteId == NOSPRITE) {
        return false;
    }
    
    return m_gfx->gfxRenderSprite(videoSpriteId, x, y);
}

bool PBVideoPlayer::pbvpRender(int x, int y, float scaleFactor, float rotateDegrees) {
    if (!videoLoaded || videoSpriteId == NOSPRITE) {
        return false;
    }
    
    return m_gfx->gfxRenderSprite(videoSpriteId, x, y, scaleFactor, rotateDegrees);
}

stVideoInfo PBVideoPlayer::pbvpGetVideoInfo() const {
    return m_video.pbvGetVideoInfo();
}

pbvPlaybackState PBVideoPlayer::pbvpGetPlaybackState() const {
    return m_video.pbvGetPlaybackState();
}

float PBVideoPlayer::pbvpGetCurrentTimeSec() const {
    return m_video.pbvGetCurrentTimeSec();
}

bool PBVideoPlayer::pbvpIsLoaded() const {
    return videoLoaded;
}

bool PBVideoPlayer::pbvpSeekTo(float timeSec) {
    if (!videoLoaded) {
        return false;
    }
    
    return m_video.pbvSeekTo(timeSec);
}

void PBVideoPlayer::pbvpSetPlaybackSpeed(float speed) {
    m_video.pbvSetPlaybackSpeed(speed);
}

void PBVideoPlayer::pbvpSetLooping(bool loop) {
    m_video.pbvSetLooping(loop);
}

void PBVideoPlayer::pbvpSetAudioEnabled(bool enabled) {
    audioEnabled = enabled;
    m_video.pbvSetAudioEnabled(enabled);
    
    if (!enabled && m_sound) {
        m_sound->pbsStopVideoAudio();
    }
}

void PBVideoPlayer::pbvpSetXY(int x, int y) {
    if (videoSpriteId != NOSPRITE) {
        m_gfx->gfxSetXY(videoSpriteId, x, y, false);
    }
}

void PBVideoPlayer::pbvpSetAlpha(float alpha) {
    if (videoSpriteId != NOSPRITE) {
        m_gfx->gfxSetTextureAlpha(videoSpriteId, alpha);
    }
}

void PBVideoPlayer::pbvpSetScaleFactor(float scale) {
    if (videoSpriteId != NOSPRITE) {
        m_gfx->gfxSetScaleFactor(videoSpriteId, scale, false);
    }
}

void PBVideoPlayer::pbvpSetRotation(float degrees) {
    if (videoSpriteId != NOSPRITE) {
        m_gfx->gfxSetRotateDegrees(videoSpriteId, degrees, false);
    }
}

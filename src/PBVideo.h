// PBVideo - FFmpeg-based video playback with OpenGL ES texture rendering
// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef PBVideo_h
#define PBVideo_h

#include "PBBuildSwitch.h"
#include <string>
#include <cstdint>

// Forward declarations for FFmpeg structures to avoid including FFmpeg headers in this header
extern "C" {
    struct AVFormatContext;
    struct AVCodecContext;
    struct AVFrame;
    struct AVPacket;
    struct SwsContext;
    struct SwrContext;
}

// Video playback states
enum pbvPlaybackState {
    PBV_STOPPED = 0,
    PBV_PLAYING = 1,
    PBV_PAUSED = 2,
    PBV_FINISHED = 3
};

// Video information structure
struct stVideoInfo {
    std::string videoFilePath;
    unsigned int width;
    unsigned int height;
    float fps;
    float durationSec;
    bool hasAudio;
    bool hasVideo;
};

class PBVideo {
public:
    PBVideo();
    ~PBVideo();
    
    // Initialize video system
    bool pbvInitialize();
    
    // Cleanup and shutdown
    void pbvShutdown();
    
    // Load a video file (prepares for playback but doesn't start)
    bool pbvLoadVideo(const std::string& videoFilePath);
    
    // Unload current video and free resources
    void pbvUnloadVideo();
    
    // Start or resume playback
    bool pbvPlay();
    
    // Pause playback
    void pbvPause();
    
    // Stop playback and reset to beginning
    void pbvStop();
    
    // Update video frame based on current time (call this every frame)
    // Returns true if a new frame was decoded
    bool pbvUpdateFrame(unsigned long currentTick);
    
    // Get current video frame data (RGBA format for texture upload)
    // Returns pointer to frame buffer or nullptr if no frame available
    // frameWidth and frameHeight will be set to actual frame dimensions
    const uint8_t* pbvGetFrameData(unsigned int* frameWidth, unsigned int* frameHeight);
    
    // Get audio samples for current frame (for integration with PBSound)
    // Returns pointer to audio buffer (stereo float format) and number of samples
    // This is designed to be called once per video frame update
    const float* pbvGetAudioSamples(int* numSamples);
    
    // Query functions
    stVideoInfo pbvGetVideoInfo() const;
    pbvPlaybackState pbvGetPlaybackState() const;
    float pbvGetCurrentTimeSec() const;
    float pbvGetDurationSec() const;
    bool pbvIsLoaded() const;
    
    // Seek to specific time (in seconds)
    bool pbvSeekTo(float timeSec);
    
    // Set playback speed (1.0 = normal, 2.0 = 2x speed, 0.5 = half speed)
    void pbvSetPlaybackSpeed(float speed);
    float pbvGetPlaybackSpeed() const { return playbackSpeed; }
    
    // Enable/disable audio playback
    void pbvSetAudioEnabled(bool enabled);
    bool pbvIsAudioEnabled() const { return audioEnabled; }
    
    // Set looping behavior
    void pbvSetLooping(bool loop);
    bool pbvIsLooping() const { return looping; }
    
private:
    // FFmpeg context pointers
    AVFormatContext* formatContext;
    AVCodecContext* videoCodecContext;
    AVCodecContext* audioCodecContext;
    SwsContext* swsContext;        // For video color conversion
    SwrContext* swrContext;        // For audio resampling
    
    AVFrame* videoFrame;
    AVFrame* videoFrameRGB;
    AVFrame* audioFrame;
    AVPacket* packet;
    
    // Video stream information
    int videoStreamIndex;
    int audioStreamIndex;
    stVideoInfo videoInfo;
    
    // Playback state
    pbvPlaybackState playbackState;
    bool initialized;
    bool videoLoaded;
    bool looping;
    bool audioEnabled;
    float playbackSpeed;
    
    // Timing information
    unsigned long startTick;       // Tick when playback started
    unsigned long pauseTick;       // Tick when playback was paused
    unsigned long pauseDuration;   // Total time spent paused
    float lastFrameTimeSec;        // Time of last decoded frame
    
    // Frame buffers
    uint8_t* frameBuffer;          // RGBA frame data for texture upload
    unsigned int frameBufferSize;
    bool newFrameAvailable;
    
    // Audio buffers
    float* audioBuffer;            // Stereo float audio buffer
    int audioBufferSize;
    int audioSamplesAvailable;
    
    // Internal helper methods
    bool openVideoFile(const std::string& filePath);
    bool findStreamInfo();
    bool openCodecs();
    void closeCodecs();
    void freeBuffers();
    bool decodeNextVideoFrame();
    bool decodeNextAudioFrame();
    void convertFrameToRGBA();
    void convertAudioToFloat();
    float getCurrentPlaybackTimeSec(unsigned long currentTick) const;
    bool seekToFrame(float timeSec);
};

#endif // PBVideo_h

// PBVideo - FFmpeg-based video playback implementation
// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "PBVideo.h"
#include "PBBuildSwitch.h"
#include <cstring>
#include <cmath>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
}

PBVideo::PBVideo() {
    formatContext = nullptr;
    videoCodecContext = nullptr;
    audioCodecContext = nullptr;
    swsContext = nullptr;
    swrContext = nullptr;
    videoFrame = nullptr;
    videoFrameRGB = nullptr;
    audioFrame = nullptr;
    packet = nullptr;
    
    videoStreamIndex = -1;
    audioStreamIndex = -1;
    
    playbackState = PBV_STOPPED;
    initialized = false;
    videoLoaded = false;
    looping = false;
    audioEnabled = true;
    playbackSpeed = 1.0f;
    
    startTick = 0;
    pauseTick = 0;
    pauseDuration = 0;
    lastFrameTimeSec = 0.0f;
    
    frameBuffer = nullptr;
    frameBufferSize = 0;
    newFrameAvailable = false;
    
    audioBuffer = nullptr;
    audioBufferSize = 0;
    audioSamplesAvailable = 0;
    
    videoInfo = {"", 0, 0, 0.0f, 0.0f, false, false};
}

PBVideo::~PBVideo() {
    pbvShutdown();
}

bool PBVideo::pbvInitialize() {
    if (initialized) {
        return true;
    }
    
    // FFmpeg is available on both Windows and Raspberry Pi
    // Note: FFmpeg libraries must be installed on the system
    
    initialized = true;
    return true;
}

void PBVideo::pbvShutdown() {
    if (!initialized) {
        return;
    }
    
    pbvUnloadVideo();
    initialized = false;
}

bool PBVideo::pbvLoadVideo(const std::string& videoFilePath) {
    if (!initialized) {
        return false;
    }
    
    // Unload any existing video
    pbvUnloadVideo();
    
    // Open the video file
    if (!openVideoFile(videoFilePath)) {
        return false;
    }
    
    // Find stream information
    if (!findStreamInfo()) {
        pbvUnloadVideo();
        return false;
    }
    
    // Open codecs
    if (!openCodecs()) {
        pbvUnloadVideo();
        return false;
    }
    
    // Store video information
    videoInfo.videoFilePath = videoFilePath;
    videoInfo.hasVideo = (videoStreamIndex >= 0);
    videoInfo.hasAudio = (audioStreamIndex >= 0);
    
    if (videoInfo.hasVideo) {
        videoInfo.width = videoCodecContext->width;
        videoInfo.height = videoCodecContext->height;
        
        // Calculate FPS
        AVRational frameRate = formatContext->streams[videoStreamIndex]->avg_frame_rate;
        if (frameRate.den != 0) {
            videoInfo.fps = (float)frameRate.num / (float)frameRate.den;
        } else {
            videoInfo.fps = 30.0f; // Default fallback
        }
        
        // Calculate duration
        int64_t duration = formatContext->streams[videoStreamIndex]->duration;
        AVRational timeBase = formatContext->streams[videoStreamIndex]->time_base;
        if (duration != AV_NOPTS_VALUE) {
            videoInfo.durationSec = (float)duration * timeBase.num / timeBase.den;
        } else if (formatContext->duration != AV_NOPTS_VALUE) {
            videoInfo.durationSec = (float)formatContext->duration / AV_TIME_BASE;
        } else {
            videoInfo.durationSec = 0.0f;
        }
        
        // Allocate frame buffer for RGBA conversion
        frameBufferSize = videoInfo.width * videoInfo.height * 4; // RGBA
        frameBuffer = new uint8_t[frameBufferSize];
        memset(frameBuffer, 0, frameBufferSize);
    }
    
    if (videoInfo.hasAudio) {
        // Allocate audio buffer (estimate 1 second of audio at 48kHz stereo)
        audioBufferSize = 48000 * 2; // stereo
        audioBuffer = new float[audioBufferSize];
        memset(audioBuffer, 0, audioBufferSize * sizeof(float));
    }
    
    videoLoaded = true;
    playbackState = PBV_STOPPED;
    
    return true;
}

void PBVideo::pbvUnloadVideo() {
    pbvStop();
    
    closeCodecs();
    
    if (formatContext) {
        avformat_close_input(&formatContext);
        formatContext = nullptr;
    }
    
    freeBuffers();
    
    videoLoaded = false;
    videoStreamIndex = -1;
    audioStreamIndex = -1;
    videoInfo = {"", 0, 0, 0.0f, 0.0f, false, false};
}

bool PBVideo::pbvPlay() {
    if (!videoLoaded || playbackState == PBV_FINISHED) {
        return false;
    }
    
    if (playbackState == PBV_PAUSED) {
        // Resume from pause
        pauseDuration += (0 - pauseTick); // Add time spent paused
        playbackState = PBV_PLAYING;
    } else if (playbackState == PBV_STOPPED) {
        // Start from beginning
        startTick = 0; // Will be set on first update
        pauseDuration = 0;
        lastFrameTimeSec = 0.0f;
        playbackState = PBV_PLAYING;
    }
    
    return true;
}

void PBVideo::pbvPause() {
    if (playbackState == PBV_PLAYING) {
        pauseTick = 0; // Will be captured on next update
        playbackState = PBV_PAUSED;
    }
}

void PBVideo::pbvStop() {
    playbackState = PBV_STOPPED;
    startTick = 0;
    pauseTick = 0;
    pauseDuration = 0;
    lastFrameTimeSec = 0.0f;
    newFrameAvailable = false;
    audioSamplesAvailable = 0;
    
    // Seek back to beginning if video is loaded
    if (videoLoaded) {
        seekToFrame(0.0f);
    }
}

bool PBVideo::pbvUpdateFrame(unsigned long currentTick) {
    if (!videoLoaded || playbackState != PBV_PLAYING) {
        return false;
    }
    
    // Initialize start tick on first update
    if (startTick == 0) {
        startTick = currentTick;
    }
    
    // Calculate current playback time
    float currentTimeSec = getCurrentPlaybackTimeSec(currentTick);
    
    // Check if we need to decode a new frame
    if (!videoInfo.hasVideo) {
        return false;
    }
    
    // Calculate the time per frame
    float frameTime = 1.0f / videoInfo.fps;
    
    // Check if it's time for the next frame
    if (currentTimeSec >= lastFrameTimeSec + frameTime) {
        // Decode next video frame
        bool success = decodeNextVideoFrame();
        
        if (success) {
            lastFrameTimeSec = currentTimeSec;
            newFrameAvailable = true;
            
            // Also decode audio if available and enabled
            if (videoInfo.hasAudio && audioEnabled) {
                decodeNextAudioFrame();
            }
            
            return true;
        } else {
            // End of video
            if (looping) {
                // Loop back to beginning
                seekToFrame(0.0f);
                startTick = currentTick;
                pauseDuration = 0;
                lastFrameTimeSec = 0.0f;
                return pbvUpdateFrame(currentTick);
            } else {
                playbackState = PBV_FINISHED;
                return false;
            }
        }
    }
    
    return false;
}

const uint8_t* PBVideo::pbvGetFrameData(unsigned int* frameWidth, unsigned int* frameHeight) {
    if (!videoLoaded || !videoInfo.hasVideo || !newFrameAvailable) {
        *frameWidth = 0;
        *frameHeight = 0;
        return nullptr;
    }
    
    *frameWidth = videoInfo.width;
    *frameHeight = videoInfo.height;
    return frameBuffer;
}

const float* PBVideo::pbvGetAudioSamples(int* numSamples) {
    if (!videoLoaded || !videoInfo.hasAudio || !audioEnabled) {
        *numSamples = 0;
        return nullptr;
    }
    
    *numSamples = audioSamplesAvailable;
    return audioBuffer;
}

stVideoInfo PBVideo::pbvGetVideoInfo() const {
    return videoInfo;
}

pbvPlaybackState PBVideo::pbvGetPlaybackState() const {
    return playbackState;
}

float PBVideo::pbvGetCurrentTimeSec() const {
    return lastFrameTimeSec;
}

float PBVideo::pbvGetDurationSec() const {
    return videoInfo.durationSec;
}

bool PBVideo::pbvIsLoaded() const {
    return videoLoaded;
}

bool PBVideo::pbvSeekTo(float timeSec) {
    if (!videoLoaded) {
        return false;
    }
    
    bool wasPlaying = (playbackState == PBV_PLAYING);
    pbvStop();
    
    if (!seekToFrame(timeSec)) {
        return false;
    }
    
    lastFrameTimeSec = timeSec;
    
    if (wasPlaying) {
        pbvPlay();
    }
    
    return true;
}

void PBVideo::pbvSetPlaybackSpeed(float speed) {
    if (speed > 0.0f && speed <= 4.0f) { // Limit to reasonable range
        playbackSpeed = speed;
    }
}

void PBVideo::pbvSetAudioEnabled(bool enabled) {
    audioEnabled = enabled;
}

void PBVideo::pbvSetLooping(bool loop) {
    looping = loop;
}

// Private helper methods

bool PBVideo::openVideoFile(const std::string& filePath) {
    formatContext = nullptr;
    if (avformat_open_input(&formatContext, filePath.c_str(), nullptr, nullptr) != 0) {
        return false;
    }
    return true;
}

bool PBVideo::findStreamInfo() {
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        return false;
    }
    
    // Find video and audio streams
    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
        AVCodecParameters* codecParams = formatContext->streams[i]->codecpar;
        
        if (codecParams->codec_type == AVMEDIA_TYPE_VIDEO && videoStreamIndex < 0) {
            videoStreamIndex = i;
        } else if (codecParams->codec_type == AVMEDIA_TYPE_AUDIO && audioStreamIndex < 0) {
            audioStreamIndex = i;
        }
    }
    
    return (videoStreamIndex >= 0); // At minimum we need video
}

bool PBVideo::openCodecs() {
    // Open video codec
    if (videoStreamIndex >= 0) {
        AVCodecParameters* codecParams = formatContext->streams[videoStreamIndex]->codecpar;
        const AVCodec* codec = nullptr;
        
        // Use software decoder for better compatibility
        // Hardware decoders (V4L2 M2M) can be problematic on some Raspberry Pi configurations
        codec = avcodec_find_decoder(codecParams->codec_id);
        
        if (!codec) {
            return false;
        }
        
        videoCodecContext = avcodec_alloc_context3(codec);
        if (!videoCodecContext) {
            return false;
        }
        
        if (avcodec_parameters_to_context(videoCodecContext, codecParams) < 0) {
            return false;
        }
        
        if (avcodec_open2(videoCodecContext, codec, nullptr) < 0) {
            return false;
        }
        
        // Allocate video frames
        videoFrame = av_frame_alloc();
        videoFrameRGB = av_frame_alloc();
        
        if (!videoFrame || !videoFrameRGB) {
            return false;
        }
        
        // Allocate buffer for RGB frame
        int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, 
                                                 videoCodecContext->width,
                                                 videoCodecContext->height, 1);
        uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
        
        av_image_fill_arrays(videoFrameRGB->data, videoFrameRGB->linesize, buffer,
                            AV_PIX_FMT_RGBA, videoCodecContext->width, 
                            videoCodecContext->height, 1);
        
        // Initialize SWS context for color conversion
        swsContext = sws_getContext(videoCodecContext->width, videoCodecContext->height,
                                    videoCodecContext->pix_fmt,
                                    videoCodecContext->width, videoCodecContext->height,
                                    AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);
        
        if (!swsContext) {
            return false;
        }
    }
    
    // Open audio codec
    if (audioStreamIndex >= 0) {
        AVCodecParameters* codecParams = formatContext->streams[audioStreamIndex]->codecpar;
        const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
        
        if (!codec) {
            // Audio is optional, so don't fail
            audioStreamIndex = -1;
        } else {
            audioCodecContext = avcodec_alloc_context3(codec);
            if (audioCodecContext) {
                if (avcodec_parameters_to_context(audioCodecContext, codecParams) >= 0) {
                    if (avcodec_open2(audioCodecContext, codec, nullptr) >= 0) {
                        audioFrame = av_frame_alloc();
                        
                        // Initialize SWR context for audio resampling to stereo float
                        swrContext = swr_alloc();
                        if (swrContext) {
                            // Use newer FFmpeg API for channel layout
                            AVChannelLayout in_ch_layout = audioCodecContext->ch_layout;
                            AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_STEREO;
                            
                            av_opt_set_chlayout(swrContext, "in_chlayout", &in_ch_layout, 0);
                            av_opt_set_int(swrContext, "in_sample_rate", audioCodecContext->sample_rate, 0);
                            av_opt_set_sample_fmt(swrContext, "in_sample_fmt", audioCodecContext->sample_fmt, 0);
                            
                            av_opt_set_chlayout(swrContext, "out_chlayout", &out_ch_layout, 0);
                            av_opt_set_int(swrContext, "out_sample_rate", 44100, 0); // Match SDL2_mixer default
                            av_opt_set_sample_fmt(swrContext, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
                            
                            if (swr_init(swrContext) < 0) {
                                swr_free(&swrContext);
                                swrContext = nullptr;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Allocate packet
    packet = av_packet_alloc();
    if (!packet) {
        return false;
    }
    
    return true;
}

void PBVideo::closeCodecs() {
    if (videoCodecContext) {
        avcodec_free_context(&videoCodecContext);
        videoCodecContext = nullptr;
    }
    
    if (audioCodecContext) {
        avcodec_free_context(&audioCodecContext);
        audioCodecContext = nullptr;
    }
    
    if (swsContext) {
        sws_freeContext(swsContext);
        swsContext = nullptr;
    }
    
    if (swrContext) {
        swr_free(&swrContext);
        swrContext = nullptr;
    }
    
    if (videoFrame) {
        av_frame_free(&videoFrame);
        videoFrame = nullptr;
    }
    
    if (videoFrameRGB) {
        if (videoFrameRGB->data[0]) {
            av_free(videoFrameRGB->data[0]);
        }
        av_frame_free(&videoFrameRGB);
        videoFrameRGB = nullptr;
    }
    
    if (audioFrame) {
        av_frame_free(&audioFrame);
        audioFrame = nullptr;
    }
    
    if (packet) {
        av_packet_free(&packet);
        packet = nullptr;
    }
}

void PBVideo::freeBuffers() {
    if (frameBuffer) {
        delete[] frameBuffer;
        frameBuffer = nullptr;
        frameBufferSize = 0;
    }
    
    if (audioBuffer) {
        delete[] audioBuffer;
        audioBuffer = nullptr;
        audioBufferSize = 0;
    }
}

bool PBVideo::decodeNextVideoFrame() {
    if (!videoCodecContext || videoStreamIndex < 0) {
        return false;
    }
    
    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex) {
            // Send packet to decoder
            if (avcodec_send_packet(videoCodecContext, packet) < 0) {
                av_packet_unref(packet);
                continue;
            }
            
            // Receive frame from decoder
            if (avcodec_receive_frame(videoCodecContext, videoFrame) == 0) {
                // Convert frame to RGBA
                convertFrameToRGBA();
                av_packet_unref(packet);
                return true;
            }
        }
        av_packet_unref(packet);
    }
    
    return false; // End of stream
}

bool PBVideo::decodeNextAudioFrame() {
    if (!audioCodecContext || audioStreamIndex < 0 || !swrContext) {
        return false;
    }
    
    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == audioStreamIndex) {
            // Send packet to decoder
            if (avcodec_send_packet(audioCodecContext, packet) < 0) {
                av_packet_unref(packet);
                continue;
            }
            
            // Receive frame from decoder
            if (avcodec_receive_frame(audioCodecContext, audioFrame) == 0) {
                // Convert audio to float stereo
                convertAudioToFloat();
                av_packet_unref(packet);
                return true;
            }
        }
        av_packet_unref(packet);
    }
    
    return false; // End of stream
}

void PBVideo::convertFrameToRGBA() {
    if (!videoFrame || !videoFrameRGB || !swsContext || !frameBuffer) {
        return;
    }
    
    // Scale and convert color space
    sws_scale(swsContext, videoFrame->data, videoFrame->linesize, 0,
              videoCodecContext->height, videoFrameRGB->data, videoFrameRGB->linesize);
    
    // Copy to our frame buffer
    memcpy(frameBuffer, videoFrameRGB->data[0], frameBufferSize);
}

void PBVideo::convertAudioToFloat() {
    if (!audioFrame || !swrContext || !audioBuffer) {
        return;
    }
    
    // Resample audio to stereo float format
    uint8_t* output = (uint8_t*)audioBuffer;
    int outSamples = swr_convert(swrContext, &output, audioBufferSize / 2,
                                 (const uint8_t**)audioFrame->data, audioFrame->nb_samples);
    
    if (outSamples > 0) {
        audioSamplesAvailable = outSamples;
    } else {
        audioSamplesAvailable = 0;
    }
}

float PBVideo::getCurrentPlaybackTimeSec(unsigned long currentTick) const {
    if (startTick == 0) {
        return 0.0f;
    }
    
    float elapsedSec = (float)(currentTick - startTick - pauseDuration) / 1000.0f;
    return elapsedSec * playbackSpeed;
}

bool PBVideo::seekToFrame(float timeSec) {
    if (!formatContext || videoStreamIndex < 0) {
        return false;
    }
    
    // Convert time to stream time base
    AVRational timeBase = formatContext->streams[videoStreamIndex]->time_base;
    int64_t seekTarget = (int64_t)(timeSec / timeBase.num * timeBase.den);
    
    // Seek to keyframe
    if (av_seek_frame(formatContext, videoStreamIndex, seekTarget, AVSEEK_FLAG_BACKWARD) < 0) {
        return false;
    }
    
    // Flush codec buffers
    if (videoCodecContext) {
        avcodec_flush_buffers(videoCodecContext);
    }
    if (audioCodecContext) {
        avcodec_flush_buffers(audioCodecContext);
    }
    
    return true;
}

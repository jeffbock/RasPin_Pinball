// PBVideo - FFmpeg-based video playback implementation
// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "PBVideo.h"
#include "PBBuildSwitch.h"
#include <cstring>
#include <cmath>
#include <algorithm>

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
    justLooped = false;
    
    startTick = 0;
    pauseTick = 0;
    pauseDuration = 0;
    lastFrameTimeSec = 0.0f;
    videoTimeBase = 0.0;
    audioTimeBase = 0.0;
    masterClock = 0.0;
    videoClock = 0.0;
    audioClock = 0.0;
    
    frameBuffer = nullptr;
    frameBufferSize = 0;
    newFrameAvailable = false;
    
    audioBuffer = nullptr;
    audioBufferSize = 0;
    audioSamplesAvailable = 0;
    audioAccumulatorIndex = 0;
    
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
    
    // Store time base information for proper synchronization
    if (videoStreamIndex >= 0) {
        AVRational timeBase = formatContext->streams[videoStreamIndex]->time_base;
        videoTimeBase = (double)timeBase.num / (double)timeBase.den;
    }
    
    if (audioStreamIndex >= 0) {
        AVRational timeBase = formatContext->streams[audioStreamIndex]->time_base;
        audioTimeBase = (double)timeBase.num / (double)timeBase.den;
    }
    
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
    
    clearPacketQueues();
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
    videoTimeBase = 0.0;
    audioTimeBase = 0.0;
    masterClock = 0.0;
    videoClock = 0.0;
    audioClock = 0.0;
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
        
        // Pre-fill audio buffer for streaming to work immediately
        // Fill to 75% to prevent initial popping
        if (videoInfo.hasAudio && audioEnabled) {
            while (audioAccumulatorIndex < AUDIO_ACCUMULATOR_SIZE * 0.75f) {
                if (!decodeNextAudioFrame()) {
                    break;
                }
            }
            audioSamplesAvailable = audioAccumulatorIndex;
        }
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
    audioAccumulatorIndex = 0;
    
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
    
    // Fill packet queues if they're getting low
    if (videoPacketQueue.size() < 5 || audioPacketQueue.size() < 5) {
        fillPacketQueues();
    }
    
    // Always try to keep audio buffer filled independently of video
    if (videoInfo.hasAudio && audioEnabled) {
        // Keep buffer at 75% capacity for smooth streaming without gaps
        // Higher buffer prevents popping/crackling from underruns
        while (audioAccumulatorIndex < AUDIO_ACCUMULATOR_SIZE * 0.75f) {
            if (!decodeNextAudioFrame()) {
                break; // No more audio frames available
            }
        }
        // Make available samples accessible
        audioSamplesAvailable = audioAccumulatorIndex;
    }
    
    // Check if we need to decode a new frame
    if (!videoInfo.hasVideo) {
        return false;
    }
    
    // Calculate current playback time based on system clock
    float currentTimeSec = getCurrentPlaybackTimeSec(currentTick);
    
    // Calculate the time per frame
    float frameTime = 1.0f / videoInfo.fps;
    
    // Check if it's time for the next frame (small tolerance to prevent frame rushing)
    if (currentTimeSec >= lastFrameTimeSec + frameTime - 0.001f) { // 1ms tolerance
        // Decode next video frame
        bool success = decodeNextVideoFrame();
        
        if (success) {
            lastFrameTimeSec = currentTimeSec;
            newFrameAvailable = true;
            return true;
        } else {
            // End of video
            if (looping) {
                // Loop back to beginning
                seekToFrame(0.0f);
                startTick = currentTick;
                pauseDuration = 0;
                lastFrameTimeSec = 0.0f;
                audioAccumulatorIndex = 0;
                videoClock = 0.0;
                audioClock = 0.0;
                justLooped = true;  // Signal that we looped
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

// Overload for buffer-based retrieval (used by streaming callback)
int PBVideo::pbvGetAudioSamples(float* buffer, int requestedSamples) {
    if (!videoLoaded || !videoInfo.hasAudio || !audioEnabled || audioSamplesAvailable == 0 || !buffer) {
        return 0;
    }
    
    int samplesToProvide = std::min(audioSamplesAvailable, requestedSamples * 2); // stereo
    
    // Copy samples to provided buffer
    memcpy(buffer, audioAccumulator, samplesToProvide * sizeof(float));
    
    // Shift remaining samples to front of accumulator
    int remainingSamples = audioAccumulatorIndex - samplesToProvide;
    if (remainingSamples > 0) {
        memmove(audioAccumulator, audioAccumulator + samplesToProvide, 
                remainingSamples * sizeof(float));
    }
    audioAccumulatorIndex = remainingSamples;
    audioSamplesAvailable = audioAccumulatorIndex;
    
    return samplesToProvide / 2;  // Return mono sample count
}

// Original version for compatibility
const float* PBVideo::pbvGetAudioSamples(int* numSamples) {
    if (!videoLoaded || !videoInfo.hasAudio || !audioEnabled || audioSamplesAvailable == 0) {
        *numSamples = 0;
        return nullptr;
    }
    
    // Calculate chunk size to match video frame rate
    // Smaller chunks finish faster, allowing dual channels to alternate smoothly
    int targetSamplesPerFrame = 1470; // Default: ~33ms (good for 30fps)
    
    if (videoInfo.fps > 0.0f) {
        // Calculate samples for one frame duration
        float frameDuration = 1.0f / videoInfo.fps; // seconds
        targetSamplesPerFrame = (int)(44100.0f * frameDuration * 2.0f); // stereo samples
        
        // Clamp to reasonable range
        if (targetSamplesPerFrame < 882) targetSamplesPerFrame = 882;    // Min ~20ms
        if (targetSamplesPerFrame > 4410) targetSamplesPerFrame = 4410;  // Max ~100ms
    }
    
    int samplesToProvide = std::min(audioSamplesAvailable, targetSamplesPerFrame);
    
    // Copy samples to audio buffer for retrieval
    for (int i = 0; i < samplesToProvide && i < audioBufferSize; i++) {
        audioBuffer[i] = audioAccumulator[i];
    }
    
    // Shift remaining samples to front of accumulator (avoid gaps)
    int remainingSamples = audioAccumulatorIndex - samplesToProvide;
    if (remainingSamples > 0) {
        memmove(audioAccumulator, audioAccumulator + samplesToProvide, 
                remainingSamples * sizeof(float));
    }
    audioAccumulatorIndex = remainingSamples;
    audioSamplesAvailable = audioAccumulatorIndex;
    
    *numSamples = samplesToProvide;
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

bool PBVideo::pbvDidJustLoop() {
    bool result = justLooped;
    justLooped = false;  // Clear flag after reading
    return result;
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
        
#if defined(EXE_MODE_RASPI) && ENABLE_HW_VIDEO_DECODE
        // Try hardware decoder first on Raspberry Pi
        if (codecParams->codec_id == AV_CODEC_ID_H264) {
            codec = avcodec_find_decoder_by_name("h264_v4l2m2m");
            if (codec) {
                printf("PBVideo: Using H.264 hardware decoder (V4L2 M2M)\n");
            } else {
                printf("PBVideo: H.264 hardware decoder not found, falling back to software\n");
                codec = avcodec_find_decoder(codecParams->codec_id);
            }
        } else if (codecParams->codec_id == AV_CODEC_ID_HEVC) {
            codec = avcodec_find_decoder_by_name("hevc_v4l2m2m");
            if (codec) {
                printf("PBVideo: Using HEVC hardware decoder (V4L2 M2M)\n");
            } else {
                printf("PBVideo: HEVC hardware decoder not found, falling back to software\n");
                codec = avcodec_find_decoder(codecParams->codec_id);
            }
        } else {
            // Use software decoder for other codecs
            codec = avcodec_find_decoder(codecParams->codec_id);
            if (codec) {
                printf("PBVideo: Using software decoder for codec: %s\n", codec->name);
            }
        }
#else
        // Use software decoder for better compatibility on Windows
        // or when hardware decode is disabled
        codec = avcodec_find_decoder(codecParams->codec_id);
        if (codec) {
            printf("PBVideo: Using software decoder: %s\n", codec->name);
        }
#endif
        
        if (!codec) {
            printf("PBVideo: No decoder found for codec ID: %d\n", codecParams->codec_id);
            return false;
        }
        
        videoCodecContext = avcodec_alloc_context3(codec);
        if (!videoCodecContext) {
            printf("PBVideo: Failed to allocate codec context\n");
            return false;
        }
        
        if (avcodec_parameters_to_context(videoCodecContext, codecParams) < 0) {
            printf("PBVideo: Failed to copy codec parameters to context\n");
            return false;
        }
        
#if defined(EXE_MODE_RASPI) && ENABLE_HW_VIDEO_DECODE
        // Configure hardware decoder specific options
        if (strstr(codec->name, "v4l2m2m") != nullptr) {
            // On Raspberry Pi 5+, let FFmpeg auto-detect the device
            // (the older /dev/video10 path is only for Pi 4 and earlier)
            // Just configure buffer counts for better performance
            av_opt_set_int(videoCodecContext->priv_data, "num_output_buffers", 16, 0);
            av_opt_set_int(videoCodecContext->priv_data, "num_capture_buffers", 16, 0);
            
            printf("PBVideo: Configured V4L2 M2M decoder (auto-detect device)\n");
        }
#endif
        
        if (avcodec_open2(videoCodecContext, codec, nullptr) < 0) {
#if defined(EXE_MODE_RASPI) && ENABLE_HW_VIDEO_DECODE
            // If hardware decoder fails, try software decoder as fallback
            if (strstr(codec->name, "v4l2m2m") != nullptr) {
                printf("PBVideo: Hardware decoder failed, falling back to software\n");
                
                // Clean up failed hardware context
                avcodec_free_context(&videoCodecContext);
                
                // Try software decoder
                codec = avcodec_find_decoder(codecParams->codec_id);
                if (codec) {
                    videoCodecContext = avcodec_alloc_context3(codec);
                    if (videoCodecContext) {
                        if (avcodec_parameters_to_context(videoCodecContext, codecParams) >= 0) {
                            if (avcodec_open2(videoCodecContext, codec, nullptr) >= 0) {
                                printf("PBVideo: Software decoder fallback successful\n");
                            } else {
                                printf("PBVideo: Software decoder fallback failed\n");
                                return false;
                            }
                        } else {
                            printf("PBVideo: Failed to copy parameters for software fallback\n");
                            return false;
                        }
                    } else {
                        printf("PBVideo: Failed to allocate context for software fallback\n");
                        return false;
                    }
                } else {
                    printf("PBVideo: No software decoder available for fallback\n");
                    return false;
                }
            } else {
                printf("PBVideo: Software decoder failed to open\n");
                return false;
            }
#else
            printf("PBVideo: Failed to open codec\n");
            return false;
#endif
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
        // Use SWS_FAST_BILINEAR for better performance, especially with software decode
        swsContext = sws_getContext(videoCodecContext->width, videoCodecContext->height,
                                    videoCodecContext->pix_fmt,
                                    videoCodecContext->width, videoCodecContext->height,
                                    AV_PIX_FMT_RGBA, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
        
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

void PBVideo::clearPacketQueues() {
    // Free all packets in video queue
    while (!videoPacketQueue.empty()) {
        AVPacket* pkt = videoPacketQueue.front();
        videoPacketQueue.pop();
        av_packet_free(&pkt);
    }
    
    // Free all packets in audio queue
    while (!audioPacketQueue.empty()) {
        AVPacket* pkt = audioPacketQueue.front();
        audioPacketQueue.pop();
        av_packet_free(&pkt);
    }
}

bool PBVideo::fillPacketQueues() {
    if (!formatContext) {
        return false;
    }
    
    // Read packets and distribute to appropriate queues
    // Stop after reading 10 packets to avoid blocking too long
    int packetsRead = 0;
    while (packetsRead < 10) {
        AVPacket* pkt = av_packet_alloc();
        if (!pkt) {
            break;
        }
        
        int result = av_read_frame(formatContext, pkt);
        if (result < 0) {
            av_packet_free(&pkt);
            return false; // End of stream or error
        }
        
        // Route packet to appropriate queue
        if (pkt->stream_index == videoStreamIndex) {
            videoPacketQueue.push(pkt);
        } else if (pkt->stream_index == audioStreamIndex) {
            audioPacketQueue.push(pkt);
        } else {
            // Unknown stream, discard
            av_packet_free(&pkt);
        }
        
        packetsRead++;
    }
    
    return true;
}

bool PBVideo::decodeNextVideoFrame() {
    if (!videoCodecContext || videoStreamIndex < 0) {
        return false;
    }
    
    // Fill queue if empty
    if (videoPacketQueue.empty()) {
        fillPacketQueues();
    }
    
    // Try to decode from queued packets
    while (!videoPacketQueue.empty()) {
        AVPacket* pkt = videoPacketQueue.front();
        videoPacketQueue.pop();
        
        // Send packet to decoder
        if (avcodec_send_packet(videoCodecContext, pkt) < 0) {
            av_packet_free(&pkt);
            continue;
        }
        
        // Receive frame from decoder
        int ret = avcodec_receive_frame(videoCodecContext, videoFrame);
        av_packet_free(&pkt);
        
        if (ret == 0) {
            // Successfully decoded a frame
            // FUTURE: Update video clock for A/V sync improvements
            videoClock = getVideoClock();
            convertFrameToRGBA();
            return true;
        }
    }
    
    return false; // No more frames available
}

bool PBVideo::decodeNextAudioFrame() {
    if (!audioCodecContext || audioStreamIndex < 0 || !swrContext) {
        return false;
    }
    
    // Fill queue if empty
    if (audioPacketQueue.empty()) {
        fillPacketQueues();
    }
    
    // Try to decode from queued packets
    while (!audioPacketQueue.empty()) {
        AVPacket* pkt = audioPacketQueue.front();
        audioPacketQueue.pop();
        
        // Send packet to decoder
        if (avcodec_send_packet(audioCodecContext, pkt) < 0) {
            av_packet_free(&pkt);
            continue;
        }
        
        // Receive frame from decoder
        int ret = avcodec_receive_frame(audioCodecContext, audioFrame);
        av_packet_free(&pkt);
        
        if (ret == 0) {
            // Successfully decoded a frame
            // FUTURE: Update audio clock for A/V sync improvements
            audioClock = getAudioClock();
            convertAudioToFloat();
            return true;
        }
    }
    
    return false; // No more frames available
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
    if (!audioFrame || !swrContext) {
        return;
    }
    
    // Temporary buffer for resampled audio
    float tempBuffer[8192]; // Large enough for typical audio frame
    uint8_t* output = (uint8_t*)tempBuffer;
    
    // Resample audio to stereo float format at 44.1kHz
    int outSamples = swr_convert(swrContext, &output, 4096,
                                 (const uint8_t**)audioFrame->data, audioFrame->nb_samples);
    
    if (outSamples > 0) {
        // Accumulate samples into accumulator buffer
        // outSamples is in frames (each frame = 2 samples for stereo)
        int totalSamples = outSamples * 2; // Convert frames to samples (stereo)
        
        for (int i = 0; i < totalSamples && audioAccumulatorIndex < AUDIO_ACCUMULATOR_SIZE; i++) {
            audioAccumulator[audioAccumulatorIndex++] = tempBuffer[i];
        }
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
    
    // Clear packet queues since we seeked
    clearPacketQueues();
    
    // Flush codec buffers
    if (videoCodecContext) {
        avcodec_flush_buffers(videoCodecContext);
    }
    if (audioCodecContext) {
        avcodec_flush_buffers(audioCodecContext);
    }
    
    // Reset audio accumulator
    audioAccumulatorIndex = 0;
    audioSamplesAvailable = 0;
    
    return true;
}

// FUTURE: Synchronization methods for advanced A/V sync
// These methods implement timestamp-based synchronization for improved
// audio/video sync with variable frame rate content or complex timing scenarios.
// Currently using simpler frame-rate based timing, but these are preserved
// for potential future enhancements.
double PBVideo::getVideoClock() {
    if (!videoFrame || videoStreamIndex < 0) {
        return 0.0;
    }
    
    // Get the presentation timestamp of the current video frame
    if (videoFrame->pts != AV_NOPTS_VALUE) {
        return videoFrame->pts * videoTimeBase;
    } else if (videoFrame->pkt_dts != AV_NOPTS_VALUE) {
        return videoFrame->pkt_dts * videoTimeBase;
    }
    
    return videoClock;
}

double PBVideo::getAudioClock() {
    if (!audioFrame || audioStreamIndex < 0) {
        return 0.0;
    }
    
    // Get the presentation timestamp of the current audio frame
    if (audioFrame->pts != AV_NOPTS_VALUE) {
        return audioFrame->pts * audioTimeBase;
    } else if (audioFrame->pkt_dts != AV_NOPTS_VALUE) {
        return audioFrame->pkt_dts * audioTimeBase;
    }
    
    return audioClock;
}

double PBVideo::getMasterClock() {
    // Use system time as the master clock
    // This keeps playback smooth and predictable
    // Audio and video will both sync to wall clock time
    return (double)getCurrentPlaybackTimeSec(0);
}

bool PBVideo::shouldDisplayFrame(double frameTime) {
    // Get current master clock time
    double masterTime = getMasterClock();
    
    // Calculate timing difference
    double timeDiff = frameTime - masterTime;
    
    // Display frame if it's within reasonable timing window
    // Allow some tolerance for timing variations
    return (timeDiff >= -0.04 && timeDiff <= 0.04); // 40ms tolerance
}

// Debug and monitoring functions
bool PBVideo::pbvIsUsingHardwareDecoder() const {
    if (!videoCodecContext) {
        return false;
    }
    
    const char* codecName = videoCodecContext->codec->name;
    return (strstr(codecName, "v4l2m2m") != nullptr || 
            strstr(codecName, "nvenc") != nullptr ||
            strstr(codecName, "vaapi") != nullptr ||
            strstr(codecName, "qsv") != nullptr);
}

void PBVideo::pbvPrintDecoderInfo() const {
    printf("=== PBVideo Decoder Information ===\n");
    
    if (videoCodecContext) {
        printf("Video Codec: %s\n", videoCodecContext->codec->long_name);
        printf("Video Decoder: %s\n", videoCodecContext->codec->name);
        printf("Hardware Acceleration: %s\n", pbvIsUsingHardwareDecoder() ? "YES" : "NO");
        printf("Video Resolution: %dx%d\n", videoCodecContext->width, videoCodecContext->height);
        printf("Video Pixel Format: %s\n", av_get_pix_fmt_name(videoCodecContext->pix_fmt));
        printf("Video Time Base: %.6f\n", videoTimeBase);
    } else {
        printf("No video codec loaded\n");
    }
    
    if (audioCodecContext) {
        printf("Audio Codec: %s\n", audioCodecContext->codec->long_name);
        printf("Audio Decoder: %s\n", audioCodecContext->codec->name);
        printf("Audio Sample Rate: %d Hz\n", audioCodecContext->sample_rate);
        printf("Audio Channels: %d\n", audioCodecContext->ch_layout.nb_channels);
        printf("Audio Time Base: %.6f\n", audioTimeBase);
    } else {
        printf("No audio codec loaded\n");
    }
    
    printf("Playback State: ");
    switch (playbackState) {
        case PBV_STOPPED: printf("STOPPED\n"); break;
        case PBV_PLAYING: printf("PLAYING\n"); break;
        case PBV_PAUSED: printf("PAUSED\n"); break;
        case PBV_FINISHED: printf("FINISHED\n"); break;
    }
    
    printf("Video Queue Size: %zu packets\n", videoPacketQueue.size());
    printf("Audio Queue Size: %zu packets\n", audioPacketQueue.size());
    printf("===================================\n");
}

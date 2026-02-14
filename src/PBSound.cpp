// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "PBSound.h"
#include "PBVideo.h"

#ifdef EXE_MODE_RASPI
PBSound* PBSound::instance = nullptr;
#endif

PBSound::PBSound() : initialized(false), masterVolume(100), musicVolume(100), videoVolume(100) {
#ifdef EXE_MODE_RASPI
    currentMusic = nullptr;
    for (int i = 0; i < 4; i++) {
        effectSlots[i] = nullptr;
        effectChannels[i] = -1;
        effectActive[i] = false;
        effectLoop[i] = false;
        effectFilePath[i] = "";
    }
    // Initialize video audio streaming system with double-buffering
    videoAudioChunk = nullptr;
    videoAudioChunkPending = nullptr;
    pendingChunkReady = false;
    videoAudioStreaming = false;
    videoProvider = nullptr;
    instance = this;  // Set singleton for callback
#endif
}

PBSound::~PBSound() {
    pbsShutdown();
}

bool PBSound::pbsInitialize() {
#ifdef EXE_MODE_RASPI
    if (initialized) {
        return true;
    }
    
    // Initialize SDL audio subsystem
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        return false;
    }
    
    // Initialize SDL_mixer with optimized quality settings for reduced popping/skips
    // 44.1kHz, 16-bit signed, stereo, 4096 sample frame buffer
    // Previous: 2048 sample frames (~11.6ms at 44.1kHz) - sometimes caused audio pops
    // New: 4096 sample frames (~23.2ms at 44.1kHz) - smoother streaming with larger headroom
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
        SDL_Quit();
        return false;
    }
    
    // Allocate 5 mixing channels: 4 for effects, 1 reserved for video audio
    Mix_AllocateChannels(5);
    
    // Register channel finished callback for continuous audio streaming
    Mix_ChannelFinished(channelFinishedCallback);
    
    // Set initial volumes
    Mix_VolumeMusic(convertVolumeToSDL(musicVolume));
    Mix_Volume(-1, convertVolumeToSDL(masterVolume));
    
    initialized = true;
    return true;
#else
    // Windows stub - just return false
    return false;
#endif
}

void PBSound::pbsShutdown() {
#ifdef EXE_MODE_RASPI
    if (!initialized) {
        return;
    }
    
    // Stop and free music
    if (currentMusic) {
        Mix_HaltMusic();
        Mix_FreeMusic(currentMusic);
        currentMusic = nullptr;
    }
    
    // Stop all effects and free chunks
    pbsStopAllEffects();
    
    // Stop video audio
    pbsStopVideoAudio();
    
    // Free cached effects
    for (auto& pair : loadedEffects) {
        Mix_FreeChunk(pair.second);
    }
    loadedEffects.clear();
    
    // Close SDL_mixer and SDL
    Mix_CloseAudio();
    SDL_Quit();
    
    initialized = false;
#endif
}

bool PBSound::pbsPlayMusic(const std::string& mp3FilePath) {
#ifdef EXE_MODE_RASPI
    if (!initialized) {
        return false;
    }
    
    // Stop current music if playing
    if (currentMusic) {
        Mix_HaltMusic();
        Mix_FreeMusic(currentMusic);
        currentMusic = nullptr;
    }
    
    // Load new music
    currentMusic = Mix_LoadMUS(mp3FilePath.c_str());
    if (!currentMusic) {
        return false;
    }
    
    // Play music with infinite loops (-1)
    if (Mix_PlayMusic(currentMusic, -1) == -1) {
        Mix_FreeMusic(currentMusic);
        currentMusic = nullptr;
        return false;
    }
    
    return true;
#else
    // Windows stub
    return false;
#endif
}

void PBSound::pbsStopMusic() {
#ifdef EXE_MODE_RASPI
    if (initialized && currentMusic) {
        Mix_HaltMusic();
    }
#endif
}

void PBSound::pbsPauseMusic() {
#ifdef EXE_MODE_RASPI
    if (initialized && currentMusic && Mix_PlayingMusic()) {
        Mix_PauseMusic();
    }
#endif
}

void PBSound::pbsResumeMusic() {
#ifdef EXE_MODE_RASPI
    if (initialized && currentMusic && Mix_PausedMusic()) {
        Mix_ResumeMusic();
    }
#endif
}

int PBSound::pbsPlayEffect(const std::string& mp3FilePath, bool loop) {
#ifdef EXE_MODE_RASPI
    if (!initialized) {
        return 0;
    }
    
    // Update effect status first
    updateEffectStatus();
    
    // Find a free effect slot
    int slotIndex = findFreeEffectSlot();
    if (slotIndex == -1) {
        return 0; // No free slots
    }
    
    // Load the effect (use cache if available)
    Mix_Chunk* effect = loadEffect(mp3FilePath);
    if (!effect) {
        return 0;
    }
    
    // Play the effect - use SDL_mixer's loop parameter
    // 0 = play once, -1 = loop infinitely
    int channel = Mix_PlayChannel(-1, effect, loop ? -1 : 0);
    if (channel == -1) {
        return 0;
    }
    
    // Store effect information
    effectSlots[slotIndex] = effect;
    effectChannels[slotIndex] = channel;
    effectActive[slotIndex] = true;
    effectLoop[slotIndex] = loop;
    effectFilePath[slotIndex] = mp3FilePath;
    
    return slotIndex + 1; // Return 1-based effect ID
#else
    // Windows stub
    return 0;
#endif
}

bool PBSound::pbsIsEffectPlaying(int effectId) {
#ifdef EXE_MODE_RASPI
    if (!initialized || effectId < 1 || effectId > 4) {
        return false;
    }
    
    int slotIndex = effectId - 1;
    
    // Update status first
    updateEffectStatus();
    
    return effectActive[slotIndex];
#else
    // Windows stub
    return false;
#endif
}

void PBSound::pbsStopEffect(int effectId) {
#ifdef EXE_MODE_RASPI
    if (!initialized || effectId < 1 || effectId > 4) {
        return;
    }
    
    int slotIndex = effectId - 1;
    
    if (effectActive[slotIndex] && effectChannels[slotIndex] != -1) {
        Mix_HaltChannel(effectChannels[slotIndex]);
        effectActive[slotIndex] = false;
        effectChannels[slotIndex] = -1;
        effectSlots[slotIndex] = nullptr;
        effectLoop[slotIndex] = false;
        effectFilePath[slotIndex] = "";
    }
#endif
}

void PBSound::pbsStopAllEffects() {
#ifdef EXE_MODE_RASPI
    if (!initialized) {
        return;
    }
    
    // Halt all channels
    Mix_HaltChannel(-1);
    
    // Reset all effect slots
    for (int i = 0; i < 4; i++) {
        effectSlots[i] = nullptr;
        effectChannels[i] = -1;
        effectActive[i] = false;
        effectLoop[i] = false;
        effectFilePath[i] = "";
    }
#endif
}

void PBSound::pbsSetMasterVolume(int volume) {
    // Clamp volume to 0-100 range
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
    masterVolume = volume;
    
#ifdef EXE_MODE_RASPI
    if (initialized) {
        Mix_Volume(-1, convertVolumeToSDL(masterVolume));
    }
#endif
}

void PBSound::pbsSetMusicVolume(int volume) {
    // Clamp volume to 0-100 range
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
    musicVolume = volume;
    
#ifdef EXE_MODE_RASPI
    if (initialized) {
        Mix_VolumeMusic(convertVolumeToSDL(musicVolume));
    }
#endif
}

void PBSound::pbsSetVideoVolume(int volume) {
    // Clamp volume to 0-100 range
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
    videoVolume = volume;
    
#ifdef EXE_MODE_RASPI
    if (initialized) {
        Mix_Volume(VIDEO_AUDIO_CHANNEL, convertVolumeToSDL(videoVolume));
    }
#endif
}

#ifdef EXE_MODE_RASPI
int PBSound::findFreeEffectSlot() {
    for (int i = 0; i < 4; i++) {
        if (!effectActive[i]) {
            return i;
        }
    }
    return -1; // No free slots
}

void PBSound::updateEffectStatus() {
    for (int i = 0; i < 4; i++) {
        if (effectActive[i] && effectChannels[i] != -1) {
            // Check if channel is still playing
            if (!Mix_Playing(effectChannels[i])) {
                // Check if this effect should loop
                if (effectLoop[i] && !effectFilePath[i].empty()) {
                    // Restart the looping effect
                    Mix_Chunk* effect = loadEffect(effectFilePath[i]);
                    if (effect) {
                        int channel = Mix_PlayChannel(-1, effect, 0);
                        if (channel != -1) {
                            // Update channel info but keep loop settings
                            effectSlots[i] = effect;
                            effectChannels[i] = channel;
                            // effectActive, effectLoop, and effectFilePath remain unchanged
                            continue; // Skip the cleanup below
                        }
                    }
                }
                // Effect finished and not looping (or loop restart failed)
                effectActive[i] = false;
                effectChannels[i] = -1;
                effectSlots[i] = nullptr;
                effectLoop[i] = false;
                effectFilePath[i] = "";
            }
        }
    }
}

Mix_Chunk* PBSound::loadEffect(const std::string& filePath) {
    // Check if already loaded in cache
    auto it = loadedEffects.find(filePath);
    if (it != loadedEffects.end()) {
        return it->second;
    }
    
    // Load new effect
    Mix_Chunk* effect = Mix_LoadWAV(filePath.c_str());
    if (effect) {
        // Add to cache
        loadedEffects[filePath] = effect;
    }
    
    return effect;
}

int PBSound::convertVolumeToSDL(int percentage) {
    // Convert 0-100% to SDL's 0-128 range
    return (percentage * MIX_MAX_VOLUME) / 100;
}

Mix_Chunk* PBSound::createAudioChunkFromSamples(const float* audioSamples, int numSamples, int sampleRate) {
    if (!audioSamples || numSamples <= 0) {
        return nullptr;
    }
    
    // numSamples is total samples (already accounts for stereo channels)
    // For stereo: numSamples = frames * 2
    // Each sample converts to Sint16, so buffer size = numSamples * sizeof(Sint16)
    int bufferSize = numSamples * sizeof(Sint16);
    Sint16* buffer = new Sint16[numSamples];
    
    for (int i = 0; i < numSamples; i++) {
        // Clamp and convert float [-1.0, 1.0] to Sint16 [-32768, 32767]
        float sample = audioSamples[i];
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        buffer[i] = (Sint16)(sample * 32767.0f);
    }
    
    // Create Mix_Chunk from buffer
    Mix_Chunk* chunk = new Mix_Chunk;
    chunk->allocated = 1;
    chunk->abuf = (Uint8*)buffer;
    chunk->alen = bufferSize;
    chunk->volume = MIX_MAX_VOLUME;
    
    return chunk;
}
#endif

// Video audio streaming functions (stubs for Windows, implementation for Raspberry Pi)

void PBSound::pbsSetVideoAudioProvider(PBVideo* provider) {
#ifdef EXE_MODE_RASPI
    videoProvider = provider;
#endif
}

bool PBSound::pbsStartVideoAudioStream() {
#ifdef EXE_MODE_RASPI
    if (!initialized || !videoProvider) {
        return false;
    }
    
    videoAudioStreaming = true;
    pendingChunkReady = false;
    
    // Use larger chunk size for smoother streaming
    // 4096 sample frames (stereo pairs) = ~93ms at 44.1kHz
    // This reduces callback frequency and the chance of buffer underruns that cause pops/clicks
    const int STREAM_CHUNK_SIZE = 4096;  // Sample frames (stereo pairs)
    float audioSamples[STREAM_CHUNK_SIZE * 2];  // stereo (L+R for each frame)
    
    int samplesRead = videoProvider->pbvGetAudioSamples(audioSamples, STREAM_CHUNK_SIZE);
    if (samplesRead > 0) {
        // samplesRead is mono count, but we need total stereo samples for createAudioChunkFromSamples
        videoAudioChunk = createAudioChunkFromSamples(audioSamples, samplesRead * 2, 44100);
        if (videoAudioChunk) {
            int result = Mix_PlayChannel(VIDEO_AUDIO_CHANNEL, videoAudioChunk, 0);
            if (result == -1) {
                // Failed to play - clean up
                if (videoAudioChunk->abuf) {
                    delete[] (Sint16*)videoAudioChunk->abuf;
                }
                delete videoAudioChunk;
                videoAudioChunk = nullptr;
                videoAudioStreaming = false;
                return false;
            }
            // Pre-buffer the next chunk immediately to ensure seamless playback
            preparePendingAudioChunk();
            return true;
        }
    }
    
    // No audio available
    videoAudioStreaming = false;
    return false;
#else
    return false;
#endif
}

void PBSound::pbsStopVideoAudio() {
#ifdef EXE_MODE_RASPI
    if (!initialized) {
        return;
    }
    
    videoAudioStreaming = false;
    pendingChunkReady = false;
    
    // Stop channel 4
    Mix_HaltChannel(VIDEO_AUDIO_CHANNEL);
    
    // Clean up current chunk
    if (videoAudioChunk) {
        if (videoAudioChunk->abuf) {
            delete[] (Sint16*)videoAudioChunk->abuf;
        }
        delete videoAudioChunk;
        videoAudioChunk = nullptr;
    }
    
    // Clean up pending chunk if any
    if (videoAudioChunkPending) {
        if (videoAudioChunkPending->abuf) {
            delete[] (Sint16*)videoAudioChunkPending->abuf;
        }
        delete videoAudioChunkPending;
        videoAudioChunkPending = nullptr;
    }
#endif
}

void PBSound::pbsRestartVideoAudioStream() {
#ifdef EXE_MODE_RASPI
    // Stop current stream and restart
    pbsStopVideoAudio();
    
    // Small delay to ensure channel is fully stopped and hardware buffer clears
    // Increased from 5ms to 10ms for more reliable cleanup
    SDL_Delay(10);
    
    // Restart streaming
    pbsStartVideoAudioStream();
#endif
}

bool PBSound::pbsIsVideoAudioPlaying() {
#ifdef EXE_MODE_RASPI
    if (!initialized) {
        return false;
    }
    
    return videoAudioStreaming && Mix_Playing(VIDEO_AUDIO_CHANNEL);
#else
    return false;
#endif
}

// Static callback function - called by SDL_mixer when a channel finishes
#ifdef EXE_MODE_RASPI
void PBSound::channelFinishedCallback(int channel) {
    if (instance) {
        instance->handleChannelFinished(channel);
    }
}

void PBSound::handleChannelFinished(int channel) {
    // Only handle our video audio channel
    if (channel != VIDEO_AUDIO_CHANNEL || !videoAudioStreaming || !videoProvider) {
        return;
    }
    
    // Clean up previous chunk
    if (videoAudioChunk) {
        if (videoAudioChunk->abuf) {
            delete[] (Sint16*)videoAudioChunk->abuf;
        }
        delete videoAudioChunk;
        videoAudioChunk = nullptr;
    }
    
    // Double-buffering: Use pre-prepared pending chunk if available (reduces latency)
    // This eliminates the delay of creating chunks during the callback
    if (pendingChunkReady && videoAudioChunkPending) {
        videoAudioChunk = videoAudioChunkPending;
        videoAudioChunkPending = nullptr;
        pendingChunkReady = false;
        
        int result = Mix_PlayChannel(VIDEO_AUDIO_CHANNEL, videoAudioChunk, 0);
        if (result == -1) {
            // Failed to play - clean up and stop streaming
            if (videoAudioChunk->abuf) {
                delete[] (Sint16*)videoAudioChunk->abuf;
            }
            delete videoAudioChunk;
            videoAudioChunk = nullptr;
            videoAudioStreaming = false;
            return;
        }
        
        // Immediately prepare the next chunk for seamless playback
        preparePendingAudioChunk();
        return;
    }
    
    // Fallback: Pull next audio chunk from video provider directly
    // 4096 sample frames (stereo pairs) = ~93ms at 44.1kHz
    // Larger chunks reduce callback frequency and minimize underrun risk
    const int STREAM_CHUNK_SIZE = 4096;  // Sample frames (stereo pairs)
    float audioSamples[STREAM_CHUNK_SIZE * 2];  // stereo (L+R for each frame)
    
    int samplesRead = videoProvider->pbvGetAudioSamples(audioSamples, STREAM_CHUNK_SIZE);
    if (samplesRead > 0) {
        // samplesRead is mono count, but we need total stereo samples for createAudioChunkFromSamples
        videoAudioChunk = createAudioChunkFromSamples(audioSamples, samplesRead * 2, 44100);
        if (videoAudioChunk) {
            int result = Mix_PlayChannel(VIDEO_AUDIO_CHANNEL, videoAudioChunk, 0);
            if (result == -1) {
                // Failed to play - clean up and stop streaming
                if (videoAudioChunk->abuf) {
                    delete[] (Sint16*)videoAudioChunk->abuf;
                }
                delete videoAudioChunk;
                videoAudioChunk = nullptr;
                videoAudioStreaming = false;
            } else {
                // Successfully started playing, prepare next chunk
                preparePendingAudioChunk();
            }
        } else {
            videoAudioStreaming = false;
        }
    } else {
        // No more audio available, stop streaming
        videoAudioStreaming = false;
    }
}

void PBSound::preparePendingAudioChunk() {
    // Pre-buffer the next audio chunk to eliminate delay during callback
    // This double-buffering technique prevents gaps between chunks that cause pops
    if (!videoProvider || !videoAudioStreaming || pendingChunkReady) {
        return;
    }
    
    const int STREAM_CHUNK_SIZE = 4096;  // Sample frames (stereo pairs)
    float audioSamples[STREAM_CHUNK_SIZE * 2];  // stereo (L+R for each frame)
    
    int samplesRead = videoProvider->pbvGetAudioSamples(audioSamples, STREAM_CHUNK_SIZE);
    if (samplesRead > 0) {
        videoAudioChunkPending = createAudioChunkFromSamples(audioSamples, samplesRead * 2, 44100);
        if (videoAudioChunkPending) {
            pendingChunkReady = true;
        }
    }
}
#endif

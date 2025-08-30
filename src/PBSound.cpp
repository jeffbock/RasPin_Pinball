// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "PBSound.h"

PBSound::PBSound() : initialized(false), masterVolume(100), musicVolume(100) {
#ifdef EXE_MODE_RASPI
    currentMusic = nullptr;
    for (int i = 0; i < 4; i++) {
        effectSlots[i] = nullptr;
        effectChannels[i] = -1;
        effectActive[i] = false;
    }
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
    
    // Initialize SDL_mixer with reasonable quality settings
    // 44.1kHz, 16-bit signed, stereo, 1024 byte chunks
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
        SDL_Quit();
        return false;
    }
    
    // Allocate 4 mixing channels for effects
    Mix_AllocateChannels(4);
    
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

int PBSound::pbsPlayEffect(const std::string& mp3FilePath) {
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
    
    // Play the effect
    int channel = Mix_PlayChannel(-1, effect, 0);
    if (channel == -1) {
        return 0;
    }
    
    // Store effect information
    effectSlots[slotIndex] = effect;
    effectChannels[slotIndex] = channel;
    effectActive[slotIndex] = true;
    
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
                effectActive[i] = false;
                effectChannels[i] = -1;
                effectSlots[i] = nullptr;
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
#endif

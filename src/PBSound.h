// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

/*
To use PBSound on Raspberry Pi, you need to install SDL2 and SDL_mixer:

    ```bash
    sudo apt update
    sudo apt install libsdl2-dev libsdl2-mixer-dev

    ```
*/

#ifndef PBSound_h
#define PBSound_h

#include "PBBuildSwitch.h"
#include <string>

#ifdef EXE_MODE_RASPI
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <map>
#endif

class PBSound {
public:
    PBSound();
    ~PBSound();
    
    // Initialize the sound system
    bool pbsInitialize();
    
    // Cleanup and shutdown
    void pbsShutdown();
    
    // Play background music (looping)
    bool pbsPlayMusic(const std::string& mp3FilePath);
    
    // Stop current music
    void pbsStopMusic();
    
    // Pause current music
    void pbsPauseMusic();
    
    // Resume paused music
    void pbsResumeMusic();
    
    // Play sound effect (returns effect ID 1-4, or 0 on failure)
    int pbsPlayEffect(const std::string& mp3FilePath, bool loop = false);
    
    // Check if effect is still playing (true = still playing, false = completed/not found)
    bool pbsIsEffectPlaying(int effectId);
    
    // Stop a specific effect
    void pbsStopEffect(int effectId);
    
    // Stop all effects
    void pbsStopAllEffects();
    
    // Video audio streaming functions (Raspberry Pi only, channel 4 reserved for video)
    bool pbsStartVideoAudioStream();  // Initialize the streaming system
    void pbsStopVideoAudio();
    void pbsRestartVideoAudioStream();  // Restart stream (for looping video)
    bool pbsIsVideoAudioPlaying();
    void pbsSetVideoAudioProvider(class PBVideo* provider);  // Set the video object for callbacks
    
    // Set overall volume (0-100%)
    void pbsSetMasterVolume(int volume);
    
    // Set music volume (0-100%)
    void pbsSetMusicVolume(int volume);
    
    // Get current volume levels
    int pbsGetMasterVolume() const { return masterVolume; }
    int pbsGetMusicVolume() const { return musicVolume; }
    
private:
    bool initialized;
    int masterVolume;  // 0-100%
    int musicVolume;   // 0-100%
    
#ifdef EXE_MODE_RASPI
    Mix_Music* currentMusic;
    Mix_Chunk* effectSlots[4];  // Up to 4 simultaneous effects
    int effectChannels[4];      // Channel assignments for effects
    bool effectActive[4];       // Track which effects are active
    bool effectLoop[4];         // Track which effects should loop
    std::string effectFilePath[4];  // Track file path for each effect (for restarting loops)
    std::map<std::string, Mix_Chunk*> loadedEffects;  // Cache for loaded effects
    
    // Video audio streaming system (dedicated channel 4)
    static const int VIDEO_AUDIO_CHANNEL = 4;
    Mix_Chunk* videoAudioChunk;          // Single chunk for streaming
    bool videoAudioStreaming;            // Is stream active?
    class PBVideo* videoProvider;        // Reference to video for pulling audio
    
    // Internal helper methods
    int findFreeEffectSlot();
    void updateEffectStatus();
    Mix_Chunk* loadEffect(const std::string& filePath);
    int convertVolumeToSDL(int percentage);
    Mix_Chunk* createAudioChunkFromSamples(const float* audioSamples, int numSamples, int sampleRate);
    
    // Static callback for SDL_mixer channel finished
    static void channelFinishedCallback(int channel);
    static PBSound* instance;  // Singleton for callback access
    void handleChannelFinished(int channel);
#endif
};

#endif // PBSound_h

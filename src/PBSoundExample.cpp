// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

/*
 * PBSound Usage Example
 * 
 * This file demonstrates how to use the PBSound class for audio playback.
 * The class is designed to work with SDL_mixer on Raspberry Pi 5 for MP3 playback.
 * 
 * On Windows, the class will compile but all functions will return failure values.
 * On Raspberry Pi (when EXE_MODE_RASPI is defined), full functionality is available.
 */

#include "PBSound.h"
#include <iostream>
#include <thread>
#include <chrono>

void demonstratePBSound() {
    PBSound sound;
    
    // Initialize the sound system
    if (!sound.pbsInitialize()) {
        std::cout << "Failed to initialize sound system" << std::endl;
        return;
    }
    
    std::cout << "Sound system initialized successfully" << std::endl;
    
    // Set volume levels
    sound.pbsSetMasterVolume(80);  // 80% master volume
    sound.pbsSetMusicVolume(60);   // 60% music volume
    
    std::cout << "Master Volume: " << sound.pbsGetMasterVolume() << "%" << std::endl;
    std::cout << "Music Volume: " << sound.pbsGetMusicVolume() << "%" << std::endl;
    
    // Play background music (looping)
    // Note: Replace with actual path to your MP3 file
    if (sound.pbsPlayMusic("/path/to/background_music.mp3")) {
        std::cout << "Background music started" << std::endl;
    } else {
        std::cout << "Failed to start background music" << std::endl;
    }
    
    // Wait a bit for music to start
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Play some sound effects
    std::cout << "Playing sound effects..." << std::endl;
    
    int effect1 = sound.pbsPlayEffect("/path/to/effect1.mp3");
    int effect2 = sound.pbsPlayEffect("/path/to/effect2.mp3");
    int effect3 = sound.pbsPlayEffect("/path/to/effect3.mp3");
    
    if (effect1 > 0) {
        std::cout << "Effect 1 playing with ID: " << effect1 << std::endl;
    }
    if (effect2 > 0) {
        std::cout << "Effect 2 playing with ID: " << effect2 << std::endl;
    }
    if (effect3 > 0) {
        std::cout << "Effect 3 playing with ID: " << effect3 << std::endl;
    }
    
    // Monitor effect playback
    for (int i = 0; i < 10; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        if (effect1 > 0 && sound.pbsIsEffectPlaying(effect1)) {
            std::cout << "Effect " << effect1 << " still playing..." << std::endl;
        }
        if (effect2 > 0 && sound.pbsIsEffectPlaying(effect2)) {
            std::cout << "Effect " << effect2 << " still playing..." << std::endl;
        }
        if (effect3 > 0 && sound.pbsIsEffectPlaying(effect3)) {
            std::cout << "Effect " << effect3 << " still playing..." << std::endl;
        }
    }
    
    // Try to play more effects than available slots
    std::cout << "Testing effect slot limits..." << std::endl;
    int effect4 = sound.pbsPlayEffect("/path/to/effect4.mp3");
    int effect5 = sound.pbsPlayEffect("/path/to/effect5.mp3");  // This should fail if 4 effects are playing
    
    if (effect4 > 0) {
        std::cout << "Effect 4 playing with ID: " << effect4 << std::endl;
    } else {
        std::cout << "Effect 4 failed to play (slot limit reached)" << std::endl;
    }
    
    if (effect5 > 0) {
        std::cout << "Effect 5 playing with ID: " << effect5 << std::endl;
    } else {
        std::cout << "Effect 5 failed to play (slot limit reached)" << std::endl;
    }
    
    // Stop a specific effect
    if (effect1 > 0) {
        sound.pbsStopEffect(effect1);
        std::cout << "Stopped effect " << effect1 << std::endl;
    }
    
    // Wait a bit more
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Stop all effects
    sound.pbsStopAllEffects();
    std::cout << "All effects stopped" << std::endl;
    
    // Stop music
    sound.pbsStopMusic();
    std::cout << "Music stopped" << std::endl;
    
    // Cleanup is automatic when sound object goes out of scope
    std::cout << "Sound system demo complete" << std::endl;
}

/*
 * Example of integrating PBSound into a game loop:
 */
void gameLoopExample() {
    PBSound gameSound;
    
    if (!gameSound.pbsInitialize()) {
        // Handle initialization failure
        return;
    }
    
    // Start background music
    gameSound.pbsPlayMusic("/path/to/game_music.mp3");
    gameSound.pbsSetMusicVolume(70);
    
    // Game loop
    bool gameRunning = true;
    int frameCount = 0;
    
    while (gameRunning && frameCount < 100) {  // Simplified loop
        // Simulate game events that trigger sound effects
        if (frameCount == 10) {
            // Player shoots
            int shootEffect = gameSound.pbsPlayEffect("/path/to/shoot.mp3");
            std::cout << "Shoot effect ID: " << shootEffect << std::endl;
        }
        
        if (frameCount == 30) {
            // Enemy hit
            gameSound.pbsPlayEffect("/path/to/hit.mp3");
        }
        
        if (frameCount == 50) {
            // Explosion
            gameSound.pbsPlayEffect("/path/to/explosion.mp3");
        }
        
        if (frameCount == 80) {
            // Lower music volume during intense action
            gameSound.pbsSetMusicVolume(30);
        }
        
        if (frameCount == 90) {
            // Restore music volume
            gameSound.pbsSetMusicVolume(70);
        }
        
        // Simulate frame delay
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        frameCount++;
    }
    
    // Cleanup is automatic
}

/*
 * Uncomment the main function below to run the examples:
 */
/*
int main() {
    std::cout << "PBSound Demonstration" << std::endl;
    std::cout << "=====================" << std::endl;
    
    demonstratePBSound();
    
    std::cout << std::endl << "Game Loop Example" << std::endl;
    std::cout << "=================" << std::endl;
    
    gameLoopExample();
    
    return 0;
}
*/

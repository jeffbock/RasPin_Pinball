# PBSound API Reference

## Overview

`PBSound` provides table music, sound effects, volume controls, and the audio
stream used by `PBVideoPlayer`. `PBEngine` owns the table sound system as
`m_soundSystem`; initialize it during platform startup before playing audio.

On Raspberry Pi and Debian builds, playback uses SDL2 and SDL_mixer. Windows
simulator builds provide the same API but do not output audio.

## Setup

Raspberry Pi and Debian builds need SDL2 and SDL_mixer development packages:

```bash
sudo apt update
sudo apt install libsdl2-dev libsdl2-mixer-dev
```

```cpp
bool pbsInitialize();
void pbsShutdown();
```

Call `pbsInitialize()` once at startup. `pbsShutdown()` stops music, effects,
and video audio, then releases the audio system.

## Background Music

```cpp
bool pbsPlayMusic(const std::string& mp3FilePath);
void pbsStopMusic();
void pbsPauseMusic();
void pbsResumeMusic();
```

`pbsPlayMusic()` replaces the current music and loops the requested file. It
returns `false` when the file cannot be loaded or playback cannot start.

```cpp
m_soundSystem.pbsSetMusicVolume(70);
m_soundSystem.pbsPlayMusic("src/user/resources/sound/theme.mp3");
```

## Sound Effects

```cpp
int  pbsPlayEffect(const std::string& mp3FilePath, bool loop = false);
bool pbsIsEffectPlaying(int effectId);
void pbsStopEffect(int effectId);
void pbsStopAllEffects();
```

The sound system supports up to four simultaneous effect slots. `pbsPlayEffect`
returns an ID in the range 1-4, or 0 if no slot is available or the effect
cannot start. Retain the ID only when the effect must be queried or stopped.

```cpp
int swordEffect = m_soundSystem.pbsPlayEffect(
    "src/user/resources/sound/swordcut.mp3");

if (swordEffect != 0 && m_soundSystem.pbsIsEffectPlaying(swordEffect)) {
    // The effect is still active.
}
```

## Volume

All volume values are percentages from 0 to 100.

```cpp
void pbsSetMasterVolume(int volume);
void pbsSetMusicVolume(int volume);
void pbsSetVideoVolume(int volume);

int pbsGetMasterVolume() const;
int pbsGetMusicVolume() const;
int pbsGetVideoVolume() const;
```

Use music volume to balance the continuous soundtrack against effects. Video
audio has its own level so cutscenes can be adjusted independently.

## Video Audio Integration

These functions are used by `PBVideoPlayer`; table code should normally use the
video-player API instead of calling them directly.

```cpp
bool pbsStartVideoAudioStream();
void pbsStopVideoAudio();
void pbsRestartVideoAudioStream();
bool pbsIsVideoAudioPlaying();
void pbsSetVideoAudioProvider(PBVideo* provider);
```

Video audio uses its own mixer channel and does not consume an effect slot.

## Best Practices

- Initialize audio once and reuse `m_soundSystem` for the lifetime of the table.
- Start music when entering a long-lived mode, then stop or replace it on exit.
- Check the effect ID only when a later action depends on completion.
- Keep effect files short and avoid relying on more than four simultaneous effects.
- Use [PBVideoPlayer API](PBVideo_API.md) for synchronized video audio.
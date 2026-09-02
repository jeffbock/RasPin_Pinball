# PBVideoPlayer API Reference

## Overview

`PBVideoPlayer` is the table-facing video API. It combines FFmpeg decoding,
`PBGfx` texture rendering, and `PBSound` audio streaming. A loaded video is
rendered as a sprite, so it can be positioned, scaled, rotated, and faded.

Use one `PBVideoPlayer` per independently playing video. The lower-level
`PBVideo` decoder is owned internally and normally does not need to be used by
table code.

## Creating A Player

```cpp
PBVideoPlayer(PBGfx* gfx, PBSound* sound);
```

`PBEngine` inherits from `PBGfx` and owns `m_soundSystem`, so an engine member
can be created with `this` and `&m_soundSystem`.

```cpp
m_introVideoPlayer = new PBVideoPlayer(this, &m_soundSystem);
```

Destroy the player when its owning engine is destroyed. Before replacing a
loaded video, stop and unload it.

## Loading And Playback

```cpp
unsigned int pbvpLoadVideo(const std::string& videoFilePath,
                           int x, int y, bool keepResident = false);
void pbvpUnloadVideo();
bool pbvpPlay();
void pbvpPause();
void pbvpStop();
```

`pbvpLoadVideo()` prepares a video and creates its `GFX_VIDEO` sprite. It
returns the sprite ID, or `NOSPRITE` on failure; loading does not start playback.
`pbvpStop()` resets playback to the beginning. `pbvpUnloadVideo()` stops playback
and frees the decoder and sprite resources.

```cpp
m_introVideoId = m_introVideoPlayer->pbvpLoadVideo(
    "src/user/resources/videos/intro.mp4", 0, 0, false);
if (m_introVideoId == NOSPRITE) return false;

m_introVideoPlayer->pbvpSetLooping(true);
m_introVideoPlayer->pbvpPlay();
```

## Per-Frame Update And Render

```cpp
bool pbvpUpdate(unsigned long currentTick);
bool pbvpRender();
bool pbvpRender(int x, int y);
bool pbvpRender(int x, int y, float scaleFactor, float rotateDegrees);
```

Call `pbvpUpdate(currentTick)` every frame while the video is playing, before
rendering it. It decodes an available frame, updates the texture, and manages
audio. It returns `false` when playback is stopped or finished.

```cpp
if (m_introVideoPlayer->pbvpUpdate(currentTick)) {
    m_introVideoPlayer->pbvpRender();
} else if (m_introVideoPlayer->pbvpGetPlaybackState() == PBV_FINISHED) {
    // Transition to the next screen or restart the video.
}
```

## Presentation Controls

```cpp
void pbvpSetXY(int x, int y);
void pbvpSetAlpha(float alpha);
void pbvpSetScaleFactor(float scale);
void pbvpSetRotation(float degrees);
```

`pbvpSetAlpha()` takes a value from 0.0 (transparent) to 1.0 (opaque). For
broader sprite operations, retrieve the video sprite ID with `pbvpGetSpriteId()`
and use [PBGfx 2D Graphics API](PBGfx_2D_API.md).

## Playback Controls

```cpp
bool pbvpSeekTo(float timeSec);
void pbvpSetPlaybackSpeed(float speed);
void pbvpSetLooping(bool loop);
void pbvpSetAudioEnabled(bool enabled);
void pbvpSetVolume(int volume);
```

Seeking returns `false` if the target time cannot be reached. A speed of `1.0f`
is normal playback. Disabling audio does not stop video frames. Video volume is
separate from music and effect volume.

## Queries

```cpp
unsigned int pbvpGetSpriteId() const;
stVideoInfo pbvpGetVideoInfo() const;
pbvPlaybackState pbvpGetPlaybackState() const;
float pbvpGetCurrentTimeSec() const;
bool pbvpIsLoaded() const;
```

`stVideoInfo` describes the loaded stream:

```cpp
struct stVideoInfo {
    std::string videoFilePath;
    unsigned int width;
    unsigned int height;
    float fps;
    float durationSec;
    bool hasAudio;
    bool hasVideo;
};
```

Playback state is one of `PBV_STOPPED`, `PBV_PLAYING`, `PBV_PAUSED`, or
`PBV_FINISHED`. A non-looping video reaches `PBV_FINISHED` at its end.

## Formats And Performance

FFmpeg supplies container and codec support. MP4 with H.264 video and AAC audio
is the recommended portable format. For Raspberry Pi, use 720p or lower and
keep to one active video whenever possible; decoded RGBA frames require
`width * height * 4` bytes each.

Call `pbvpUnloadVideo()` when changing screens if the video will not be reused.
For a looping attract video, `keepResident = true` avoids repeatedly rebuilding
its texture.

## See Also

- [PBSound API](PBSound_API.md) - Music, effects, and video audio levels
- [PBGfx 2D Graphics API](PBGfx_2D_API.md) - Rendering and video texture sprites
- [Game Screen Creation Guide](Game_Creation_API.md) - Screen lifecycle and ownership
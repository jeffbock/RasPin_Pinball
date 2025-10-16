# Video Resources

This directory contains video files for the pinball game.

## Video Requirements

For best performance and compatibility:
- **Format**: MP4 container with H.264 video codec
- **Resolution**: 720p (1280x720) or lower for Raspberry Pi
- **Frame Rate**: 30 fps recommended
- **Audio**: AAC codec, 44.1kHz stereo

## Current Videos

### darktown.mp4
- Used in: Test Sandbox mode (Left Activate button)
- Expected dimensions: 1280x720 (720p)
- Description: Test video for video playback functionality

## Converting Videos

Use FFmpeg to convert videos to the recommended format:

```bash
# Convert to 720p with H.264 and AAC audio
ffmpeg -i input_video.mp4 -vf scale=1280:720 -c:v libx264 -preset fast -crf 23 -c:a aac -b:a 128k darktown.mp4

# For lower quality / smaller file size
ffmpeg -i input_video.mp4 -vf scale=1280:720 -c:v libx264 -preset fast -crf 28 -c:a aac -b:a 96k darktown.mp4
```

## Notes

- Place video files directly in this directory
- The Test Sandbox mode expects `darktown.mp4` to be present
- Videos are loaded dynamically and not kept in memory when not in use
- On Raspberry Pi, keep video bitrate reasonable (2-4 Mbps) for smooth playback

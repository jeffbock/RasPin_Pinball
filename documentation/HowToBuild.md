# Why Windows and Raspberry Pi?
Despite the power of the Rasberry Pi, Windowss or a full Linux PC remains a faster and more capable development environemnt.  That said, you don't need to use Windows to develop with the RasPin engine - you can exclusively use PiOS.  At some point you must move to PiOS since Windows is limited to basic simulaton capabilities and cannot talk to the PInball hardware.  Thankfull, VS Code makes it pretty easy to switch between the three development environments.

# Build Instructions
The build is set up with VS code, with both Windows and Linux (Raspberry Pi 5 / Debian) build paths.  The assumption is that development and debug is being done on a Windows or Debian simulator machine, while final testing will deploy a build to the Raspberry Pi

While using VS Code, by hitting Ctrl-Shift-P, you can bring up the Task Commands.  The following commands exist:
- Windows / Rasberry Pi:  Full Pinball Build - Build the RasPin engine and executable to run PInball.
- Windows / Rasberry Pi:  Full Pinball Build (Release) - Optimized release build with full compiler optimizations.
- Windows: Copy Runtime DLLs - Copies all required Windows DLLs (FFmpeg and ANGLE) to both `build/windows/debug/` and `build/windows/release/`. Run this once after cloning, or again if DLLs are missing.
- Windows / Rasberry Pi:  FontGen Build - Builds FontGen.cpp, a utility program takes a truetype font, and generate the files needed to use that font in the RasPin.
- Windows / Rasberry Pi:  Single file build - used for testing single files

PiOS uses OGL ES 3.1, so this will be the target for the builds and the main display output of the app will be a full-screen OGL ES rendering surface.

The VS code configuration files (eg: tasks.json, launch.json, settings.json, c_cpp_properties.json) are automatically set up correctly depending on your current OS, and per instructions below.

The GIT repo uses LFS, make sure to install: Windows: winget install -e --id GitHub.GitLFS, RaspPi: sudo apt install git-lfs

**Important Note**:  You must change the #define in PBBuildSwitch.h depending on which build you want. For example,

```
// For Windows development/simulation:
#define EXE_MODE_WINDOWS

// For Raspberry Pi hardware build (with physical pinball hardware):
#define EXE_MODE_RASPI
#define ENABLE_PINBALL_HARDWARE

// For Raspberry Pi in simulator mode (no hardware required):
#define EXE_MODE_RASPI

// For Debian/Linux development/simulation:
#define EXE_MODE_DEBIAN
```

See `PBBuildSwitch.h` for full details including `SIMULATOR_SMALL_WINDOW`, `MAIN_MENU_OPTION`, and `ENABLE_HW_VIDEO_DECODE` switches.

# Building for Windows

> **Note: Windows setup is significantly more complex than Linux.**  On Linux, all required libraries (OpenGL ES, FFmpeg, SDL2, etc.) are installed system-wide via `apt` and are immediately available to the compiler and linker with no extra steps.  On Windows there is no standard package manager for these, so headers, import libraries, and runtime DLLs must be manually sourced, compiled (ANGLE), or downloaded (FFmpeg) and placed in the appropriate directories within the repo.  Budget extra time for the initial Windows setup.

Windows development assumes VS Code and Visual Studio 2022 are already installed. VS 2022 is required to get the MSVC compiler tool chain (cl.exe).  

- Clone ANGLE repo from GitHub: [ANGLE](https://github.com/google/angle)
- Follow the ANGLE Dev Instructions: [ANGLE Setup](https://github.com/google/angle/blob/main/doc/DevSetup.md)
    - Required paths can be added in windows (both system and current user) by going to System->About->Advanced System Settings-> Environment Variables.
    - Be sure the full path location to angle/out/release or angle/out/debug is placed in the system path.  This is so the final built executable will be able to find the DLLs during execution.
    - Follow all required steps (optional not required).  Don't assume all Windows SDK files are installed through your VS Studio installation.  Ignore the steps for UWP apps.
    - Using the "gn" command as detailed will create directories and build settings for the ANGLE build system.
    - autoninja will use the settings created with "gn" and build all files / exes / libs for those settings.

- Clone the PInball repo from GitHub: [PInblall](https://github.com/jeffbock/PInball)

- Copy your desired ANGLE OGL ES (debug or release) libraries into the PInball structure from where you built ANGLE (angle/out/debug or angle/out/release).  The following libraries should be copied to these locations.
    - PInball/src/lib_ogl_win/libEGL.dll.lib
    - PInball/src/lib_ogl_win/libGLESv2.dll.lib
 
-  Important NOTE: You must also likely replace the header files in PInball/src/include_ogl_win with the same files from Angle/include.  These header files must usually be aligned with what was built on the system.

- In the windows search bar, look for "x64 Native Tools Command Prompt for VS2022" and run it.
    - Type "code" and hit enter to launch VS Code.
    - This must be done every time to run / debug in windows as it sets important env variables.

-  The PInball repo shoud be set up to work properly, however, there may be some include and library paths that need adjustment.  
    -  Check c_cpp_properties.json in Pinball/.vscode.  If the versions numbers or location in the include and compiler paths don't match what's on the system, change them to match.
    -  Check tasks.json in Pinball/.vscode.  If the versions numbers or location in the linker LIBPATH for your Windows SDK don't match the system, change them to match.

- PInball should now be set up to build in windows and use VS Code as the primary debugger.
    - Hit Crtl-Shift-P to bring up the task menu, select "Windows: Full Pinball Build".
    - Run the EXE either through debug with F5 or by launching the app outside VS Code.
 
- Note: At some point, the Windows setup may be updated to use the headers / libraries directly from the Angle locations, but it does not currently do that, unlike the Raspberry Pi setup.

### FFMPEG Video Support (Windows)

For Windows the libs and DLLs are already included in the source repo, but can be updated/re-installed as needed.

1. Download FFmpeg shared libraries from:
   ```
   https://github.com/BtbN/FFmpeg-Builds/releases
   ```
   Choose: `ffmpeg-master-latest-win64-gpl-shared.zip`

2. Extract and copy into the repo:
   - `include/` contents → `src/include_ogl_win/` (FFmpeg headers)
   - `lib/` contents → `src/lib_ogl_win/` (FFmpeg `.lib` import libraries)
   - The DLL files themselves are handled separately (see below).

3. Required runtime DLLs — these must be present alongside `Pinball.exe` at runtime:
   - `avcodec-*.dll`
   - `avformat-*.dll`
   - `avutil-*.dll`
   - `swscale-*.dll`
   - `swresample-*.dll`
   - `libegl.dll` and `libglesv2.dll` (from your ANGLE build)
   - Any DLLs that the above depend on (e.g. `avdevice-*.dll`, `avfilter-*.dll`, `postproc-*.dll`)

   **Where to put them:** The **Windows: Copy Runtime DLLs** task (Ctrl-Shift-P) runs `scripts/copy_win_dlls.ps1` automatically as part of every Windows Full Pinball Build. It copies all FFmpeg and ANGLE DLLs from `src/lib_ogl_win/` to both `build/windows/debug/` and `build/windows/release/`. Run it manually if DLLs are ever missing from those folders.

   **If the DLL version numbers change** (e.g. after a FFmpeg update), the `.dll` filenames in `src/lib_ogl_win/` must match what `copy_win_dlls.ps1` looks for — check that script if copies are silently skipped.

# Build Natively on Linux (Raspberry Pi / Debian)

The Linux build supports two targets that share the same source, build tools, and required packages:

| Target | Define | VS Code Task | Hardware Required |
|---|---|---|---|
| **Raspberry Pi 5 (PiOS)** | `EXE_MODE_RASPI` | Raspberry Pi: Full Pinball Build | Optional (see below) |
| **Debian/Ubuntu simulator** | `EXE_MODE_DEBIAN` | DebianOS: Full Pinball Build | None |

Items marked **[Raspberry Pi only]** are not needed for a Debian simulator build.

Install VS Code if not already installed.

### Required Installations

XRandR — multi-monitor support:
- Debian: `sudo apt-get install x11-xserver-utils libxrandr-dev`
- **[Raspberry Pi only]**: `sudo apt-get install x11-xserver-utils`  *(Pi OS includes `libxrandr-dev` by default)*

OpenGL ES — EGL/GLES headers *(Debian/Ubuntu only — Pi OS includes these by default)*:  
`sudo apt install libgles2-mesa-dev libegl1-mesa-dev`

SDL2 / SDL_Mixer — sound library:  
`sudo apt install libsdl2-dev libsdl2-mixer-dev`

FFmpeg — video playback:  
`sudo apt install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev`

WiringPi — GPIO / I2C for pinball hardware **[Raspberry Pi only]**:  
`sudo apt install ./wiringpi-3.0-1.deb`

### Build Configuration

Set the appropriate define in `PBBuildSwitch.h` (or pass it at build time):

```cpp
// Raspberry Pi — simulator or hardware mode:
#define EXE_MODE_RASPI

// Debian/Ubuntu — simulator mode only:
#define EXE_MODE_DEBIAN
```

Then use Ctrl-Shift-P to run the matching VS Code build task.

### Display and Window Notes

Both targets open an 800×480 full-screen X11 rendering surface using OGL ES 3.1.  
`SIMULATOR_SMALL_WINDOW` can be uncommented in `PBBuildSwitch.h` for a half-size window, useful when working over RDP.

**[Raspberry Pi only]** The current tested display configuration is an 800×480 screen on HDMI1 (the game display) and a larger monitor on HDMI2 for VS Code and debugging.

### Pinball Hardware **[Raspberry Pi only]**

To control physical pinball hardware, a breakout box with left/right flippers, activate buttons, and a start button must be wired to the Raspberry Pi GPIOs per the hardware design, and `ENABLE_PINBALL_HARDWARE` must be defined in `PBBuildSwitch.h`.  

Without `ENABLE_PINBALL_HARDWARE`, the Raspberry Pi build runs in **simulator mode** — it opens an X11 window and accepts keyboard input, identical to the Debian simulator.

# Cross Compile in Windows and running on Raspberry Pi 5
As of this writing Raspberry Pi 5 is running the 12.2 of the GNU compiler toolkit.  Your cross compiler tools must match to be able to build in Windows but then run the executable on the Raspberry Pi 5.

However, cross compiling on Windows and deploying to Rasberry Pi has proven difficult due to needed libraries, etc.. and is no longer a goal since the Pi 5 has proven quite capable of running VS Code and compiling locally.  That said, if you have other CPP test programs you'd like to build on Windows and send them to the Pi for execution, use the cross-compile toolkit listed below.

-  Install the Arm64 Windows cross-compile toolkit, version 12.2.Rel1   
 [AArch64 GNU/Linux target (aarch64-none-linux-gnu) 12.2.Rel](https://developer.arm.com/-/media/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-mingw-w64-i686-aarch64-none-linux-gnu.exe?rev=1cb73007050f4e638ba158f2aadcfb81&hash=C2E073917F80FF09C05248CCC5568DDBC99DCC56)

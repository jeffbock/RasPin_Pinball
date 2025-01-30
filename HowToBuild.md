# Build Instructions
The build is set up with VS code, with both Windows and Raspberry Pi 5 build paths.  The assumption is that development and debug is being done on a Windows machine, while final testing will deploy a build to the Raspberry Pi

It is assumed that VS Code is being used as the primary development GUI, so that the development can be portable between Windows and PiOS.

PiOS uses OGL ES 3.1, so this will be the target for the builds and the main display output of the app will be a full-screen OGL ES rendering surface.

# Building for Windows
Windows development assumes VS Code, along with Visual Stuiod 2022 being installed to get the MSVC compiler tool chain (cl.exe).  

- Clone ANGLE repo from GitHub: [ANGLE](https://github.com/google/angle)
- Follow the ANGLE Dev Instructions: [ANGLE Setup](https://github.com/google/angle/blob/main/doc/DevSetup.md)
    - Required paths can be added in windows (both system and current user) by going to System->About->Advanced System Settings-> Environment Variables.
    - Be sure the full path location to angle/out/release or angle/out/debug is placed in the system path.  This is so the final built executable will be able to find the DLLs during execution.
    - Follow all required steps (optional not required).  Don't assume all Windows SDK files are installed through your VS Studio installation.  Ignore the steps for UWP apps.
    - Using the "gn" command as detailed will create directories and build settings for the ANGLE build system.
    - autoninja will use the settings created with "gn" and build all files / exes / libs for those settings.

- Clone the PInball repo from GitHub: [PInblall](https://github.com/jeffbock/PInball)

- Copy your desired ANGLE OGL ES (debug or release) libraries into the PInball structure from where you built ANGLE (angle/out/debug or angle/out/release).  The following libraries shoudl be copied to these locations.
    - PInball/src/lib_ogl/libEGL.dll.lib
    - PInball/src/lib_ogl/libGLESv2.dll.lib

- In the winows search bar, look for "x64 Native Tools Command Prompt for VS2022" and run it.
    - Type "code" and hit enter to launch VS Code.
    - This must be done every time to run / debug in windows as it sets important env variables.

-  The PInball repo shoud be set up to work properly, however, there may be some include and library paths that need adjustment.  
    -  Check c_cpp_properties.json in Pinball/.vscode.  If the versions numbers or location in the include and compiler paths don't match what's on the system, change them to match.
    -  Check tasks.json in Pinball/.vscode.  If the versions numbers or location in the linker LIBPATH for your Windows SDK don't match the system, change them to match.

- PInball should now be set up to build in windows and use VS Code as the primary debugger.
    - To test: Select GLSmall.cpp and hit SHIFT-CTRL-B to build.  This will put an EXE in Pinball/winbuild
    - Run the EXE either through debug with F5 or by launching the app outside VS Code.
    - Note: If you have problems with ANGLE / OGLES libraries while building, try and replace the header files in PInball/src/include_ogl with the same files in Angle/include.  

-  TODO:  Final insructions for how to build all of Windows PInball.

# Cross Compile in Windows and running on Raspberry Pi 5
As of this writing Raspberry Pi 5 is running the 12.2 of the GNU compiler toolkit.  Your cross compiler tools must match to be able to build in Windows but then run the executable on the Raspberry Pi 5.

-  Install the Arm64 Windows cross-compile toolkit, version 12.2.Rel1   

 [AArch64 GNU/Linux target (aarch64-none-linux-gnu) 12.2.Rel](https://developer.arm.com/-/media/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-mingw-w64-i686-aarch64-none-linux-gnu.exe?rev=1cb73007050f4e638ba158f2aadcfb81&hash=C2E073917F80FF09C05248CCC5568DDBC99DCC56)

 -  TODO:  More steps needed but don't know them yet...  need to include delpoying to Pi as well...

 # Build Natively on Raspberry Pi 5
Since VS Code should be able to work directly on PiOS, it should be possible to build directly on Raspberry Pi itself.  

-  TODO:  No idea how this works yet, will get to it eventually.
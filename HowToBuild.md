# Why Windows and Raspberry Pi?
Despite the power of the Rasberry Pi, Windowws remains a faster and more capable development environemnt.  That said, you don't need to use Windows to develop with the PInball engine - you can exclusively use PiOS.  At some point you must move to PiOS since Windows is limited to basic simulaton capabilities and cannot talk to the PInball hardware.  Thankfull, VS Code makes it pretty easy to switch between the two development environments.

# Build Instructions
The build is set up with VS code, with both Windows and Raspberry Pi 5 build paths.  The assumption is that development and debug is being done on a Windows machine, while final testing will deploy a build to the Raspberry Pi

While using VS Code, by hitting Ctrl-Shift-P, you can bring up the Task Commands.  The following commands exist:
Windows / Rasberry Pi:  Full Pinball Build - Build the PInball engine and executable to run PInball.
Windows / Rasberry Pi:  FontGen Build - Builds FontGen.cpp, a utility program takes a truetype font, and generate the files needed to use that font in the PInball engine.
Windows / Rasberry Pi:  Single file build - used for testing single files

PiOS uses OGL ES 3.1, so this will be the target for the builds and the main display output of the app will be a full-screen OGL ES rendering surface.

The VS code configuration files (eg: tasks.json, launch.json, settings.json, c_cpp_properties.json) are automatically set up correctly depending on your current OS, and per instructions below.

# Building for Windows
Windows development assumes VS Code and Visual Studio 2022 are already installed installed. VS 2022 is required to get the MSVC compiler tool chain (cl.exe).  

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
 
-  Imporant NOTE: You must also likely replace the header files in PInball/src/include_ogl with the same files from Angle/include.  These header files must usually be aligned with what was built on the system.

- In the winows search bar, look for "x64 Native Tools Command Prompt for VS2022" and run it.
    - Type "code" and hit enter to launch VS Code.
    - This must be done every time to run / debug in windows as it sets important env variables.

-  The PInball repo shoud be set up to work properly, however, there may be some include and library paths that need adjustment.  
    -  Check c_cpp_properties.json in Pinball/.vscode.  If the versions numbers or location in the include and compiler paths don't match what's on the system, change them to match.
    -  Check tasks.json in Pinball/.vscode.  If the versions numbers or location in the linker LIBPATH for your Windows SDK don't match the system, change them to match.

- PInball should now be set up to build in windows and use VS Code as the primary debugger.
    - Hit Crtl-Shift-P to bring up the task menu, select "Windows: Full Pinball Build".
    - Run the EXE either through debug with F5 or by launching the app outside VS Code.
    
 
- Note: At some point, the Windows setup may be updated to use the headers / libraries directly from the Angle locations, but it does not currently do that, unlike the Raspberry Pi setup.

# Cross Compile in Windows and running on Raspberry Pi 5
As of this writing Raspberry Pi 5 is running the 12.2 of the GNU compiler toolkit.  Your cross compiler tools must match to be able to build in Windows but then run the executable on the Raspberry Pi 5.

However, cross compiling on Windows and deploying to Rasberry Pi has proven difficult due to needed libraries, etc.. and is no longer a goal since the Pi 5 has proven quite capable of running VS Code and compiling locally.  That said, if you have other CPP test programs you'd like to build on Windows and send them to the Pi for execution, use the cross-compile toolkit listed below.

-  Install the Arm64 Windows cross-compile toolkit, version 12.2.Rel1   
 [AArch64 GNU/Linux target (aarch64-none-linux-gnu) 12.2.Rel](https://developer.arm.com/-/media/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-mingw-w64-i686-aarch64-none-linux-gnu.exe?rev=1cb73007050f4e638ba158f2aadcfb81&hash=C2E073917F80FF09C05248CCC5568DDBC99DCC56)

 # Build Natively on Raspberry Pi 5
Since VS Code should be able to work directly on PiOS, it should be possible to build directly on Raspberry Pi itself.  

-  TODO:  No idea how this works yet, will get to it eventually.

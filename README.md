# RasPin Pinball Framework
A half-scale pinball homebrew project using Raspberry Pi hardware.  The goal is to fully develop a low cost system of SW and HW using Raspberry Pi and various easy to source HW pieces with typical modern pinball machnine capabilities.  Developed from scratch to learn and develop the entire architecture.  Compared to other pinball frameworks, RasPin is intended to be unique to Raspberry Pi, low cost, easy to implement, small and efficient and really intended for the hobbyist who wants a hand in every piece of the machine they want to build.

<img width="400" alt="RasPinMenu" src="https://github.com/user-attachments/assets/80699574-9bf8-4ebb-9889-d9ec4445964f" />
<img width="400" alt="RasPinDiag" src="https://github.com/user-attachments/assets/78baad46-6c7f-4157-8284-754867c45b45" />
<img width="400" alt="RasPinStart" src="https://github.com/user-attachments/assets/6ad23313-8f80-4c98-af07-982c786c6f57" />
<img width="363" alt="RasPin" src="https://github.com/user-attachments/assets/a15879aa-0a3b-4ca7-8cfd-c8a9cbe4cf9a" />

# Current Features - This is changing constantly as work continues...
- Intended to be a hobby level, low cost system for creating full feature, half scale pinball machines with Raspberry Pi hardware
- Cross platform VS Code environment for Raspberry Pi (full HW) and Windows (simulation / fast development)
- Supports a primary Pinball HDMI screen, plus 2nd monitor support for ease of debug / development
- Built on OpenGL ES for graphics rendering, with sprite, animation and text rendering support
- Video playback through FFMPEG integrated into sprite / rendering system
- Simplified HW architecture for easy debug, understanding and implemention
- Message based input and output processing utilizing Raspberry Pi, and TI I2C IO and LED expanders optimized to decrease latency and minimize HW traffic
- Automatic LED control and sequence animation for dynamic lighting effects
- Easy to use music and sound effect system with multiple channels
- Full setup / control and diagnostics menus and capability, along with straight-forward ability to add / expand for your own personalize machine.
- TODO: Prototype HW (mechanics, flippers, slings, etc..), build a cabinet and implement a full game (currently only game test screens).
- TODO: Develop schematics for the custom expander boards based on TI

# Documentation
- **[RasPin Overview](documentation/RasPin_Overview.md)** - **START HERE** - High-level architecture overview with diagrams and introduction to all documentation
- **[How To Build](documentation/HowToBuild.md)** - Build instructions for Windows and Raspberry Pi

- **API Reference Documentation:**
  - **[PBEngine API](documentation/PBEngine_API.md)** - Core engine class, game state, and output control
  - **[I/O Processing API](documentation/IO_Processing_API.md)** - Input/output message processing and hardware communication
  - **[LED Control API](documentation/LED_Control_API.md)** - LED control, sequences, and animation patterns
  - **[PBDevice API](documentation/PBDevice_API.md)** - Device management framework for complex pinball mechanisms
  - **[Platform Init API](documentation/Platform_Init_API.md)** - Platform initialization, main loop, and configuration
  - **[Game Creation API](documentation/Game_Creation_API.md)** - Graphics (PBGfx), sound (PBSound), video playback (PBVideoPlayer), sprites, animations, and screen management 

- **[Utilities Guide](documentation/Utilities_Guide.md)** - Complete guide for all RasPin utilities:
  - **FontGen** (Windows & Raspberry Pi) - Converts TrueType fonts to texture atlases for text rendering
  - **pblistdevices** (Raspberry Pi only) - Scans I2C bus and lists all connected hardware devices
  - **pbsetamp** (Raspberry Pi only) - Controls MAX9744 amplifier volume settings

# Design and Development guidelines
-  Actual machine based on Raspberry Pi 5 and PiOS for ease of development and debug.  Full power of Linux OS.  
-  Cross platform support via VS Code: Shared code between PiOS and Windows - using Windows for high power development / simulation.
-  Keep the structure and coding straightfoward so that intermediate level coders and utilize the system and build new tables.
-  Modular / HAL based graphics engine - allow for easy upgrade to other APIs if desired (currently OGL 3.1, but maybe Vulkan later?)
-  All IO (sensor/button inputs, lighting and solenoid outputs) routed through Pi controller.
    - Allows for complete decoupling of inputs and outputs - maximum flexibility of how to respond to inputs but design to reduce latency
    - IO expanders (based on TI TCA9555) / driver boards used in conjunction with Pi controller to greatly expand number of elements in the system.
    - LED expander (based on TI TLC59116) with blinking, dimming and group control to offload LED management for core HW as much as possible
    - Modular structure for I/O devices to allow ease of integration of any new controllers.
-  Sound and display using standard HDMI and audio output jacks from the Pi controller.  The code has flexibility in primary screen selection for the table.
    - Can use HDMI speaker sound or external amplifier based on readily avaialble MAX9744 breakout boards.
-  Simple sound usage with WAV / MP3 files for easy to use music and sound effects.
-  Allow for flexibility in configuration, eg: standard menus but SW should be architected to quickly change tables and layouts.
-  Everything should be rougly half-scale compared to a normal pinball machine - which means ~14" W x 28"H play area with a 1/2" ball.
    -  All custom pinball mechanisms will be developed with 3D models for 3D printing, allowing others to easily duplicate the construction.
    -  Cabinet will be developed using bass wood plywood.
-  Focus on architecture and design, utilize AI for speed of development but review all code closely to ensure desired behavior

# About the lead developer
-  I've been a computer engineer in the industry for 30+ years, working for major tech firms, primarily in HW/SW interfacing and testing for graphics and AI drivers.  Creating SW to talk to HW and graphics engines at that level has always been a hobby and pinball seems to be an excellent overall systems and architectural challenge to do all the things that are interesting.

# License Guidelines
-  TL;DR: The SW and all files are open source unless otherwise noted, and available to users for personal but not commercial projects.  Any contributions to the repo become available for all to use.  The original repo owner reserves the rights to use the code and all contributions for commercial use.  Hey - I might want to build and sell a few machines at some point... ;)  If you want to use the code for commercial purposes, then let's talk.

- Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.  See more specific details and other license terms in the root of the project


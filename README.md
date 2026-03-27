# RasPin Pinball Framework
A half-scale pinball homebrew project using Raspberry Pi hardware.  The goal is to fully develop a low cost system of SW and HW using Raspberry Pi and various easy to source HW pieces with typical modern pinball machnine capabilities.  Developed from scratch to learn and develop and entire architecture from top to bottom.  Compared to other pinball frameworks, RasPin is intended to be unique to Raspberry Pi, low cost, easy to implement, small and efficient and really intended for the hobbyist who wants a hand in every piece of the machine they want to build.

<img width="400" alt="Main" src="https://github.com/user-attachments/assets/4cca1044-11f2-49b0-9a04-07f01f3636d4" />
<img width="400" alt="Settings" src="https://github.com/user-attachments/assets/ae7a281d-bd2e-4194-bca6-df9027b6d494" />
<img width="400" alt="StartScreen" src="https://github.com/user-attachments/assets/b516174f-b142-4b0e-9c75-2702062400b2" />
<img width="400"alt="Game" src="https://github.com/user-attachments/assets/429c6e0d-82e5-4d91-80c3-b7df7e8f1e19" />
<img width="400" alt="NeoPixelHW" src="https://github.com/user-attachments/assets/6943b0b3-35fb-4170-a80e-49459dd16450" />
<img width="400" alt="BaseHW" src="https://github.com/user-attachments/assets/989e1e88-8786-43c8-bf0c-c8ffe9036ce2" />

Prototype hardware shown, full machine in development.

# Current RasPin System Features
- Intended to be a hobby level, low cost system for creating full feature, half scale pinball machines with Raspberry Pi hardware
- Cross platform VS Code environment for Linux (Linux via Debian / Raspberry Pi) and Windows - All OSes support simulator mode, while Raspberry Pi supports full pinball hardware.  Other Linux versions would likely work as well, but not tested.
- Pinball HW mode supports a primary Pinball HDMI screen, plus 2nd monitor support for ease of debug / development
- Built on OpenGL ES for graphics rendering, with sprite, 3D (glTF/GLB), animation and text rendering support all in simple screen space.
- Video playback through FFMPEG integrated into sprite / rendering system
- Simplified HW architecture for easy debug, understanding and implemention
- Message based input and output processing utilizing Raspberry Pi, and TI I2C IO and LED expanders optimized to decrease latency and minimize HW traffic
- Timer system with dedicated watchdog timer and user timers for timed game events
- Automatic LED control and sequence animation for dynamic lighting effects
- NeoPixel (SK6812/WS2812B) RGB LED strip support with multiple timing methods (clock_gettime, NOP, SPI, PWM), sequence animation, and full color control
- Easy to use music and sound effect system with multiple channels
- Full setup / control and diagnostics menus and capability, along with straight-forward ability to add / expand for your own personalize machine.
- Mode and gameplay framework with a multi-mode state machine, mode sub-states, screen display priority management with and a structured game flow skeleton ready for building full pinball table rules
- Base system supports "Simple Flip" debug mode for basic checkout.

# Currently working on... (WIP)
- Prototype Cabinet and lower 1/3 playfield.  Cabinet done in 1/4 foamboard, dimensions in HW directory of repo.  Prototyping lanes and flippers via 3D printing, flipper solenoid drivers.  Will be mounted on 1/2 ply.
- FUTURE: Prototype HW (Balleject, targets, pop bumpers, lane / ramp detect, etc..)
- FUTURE: Develop full version 1 whitewood
- FUTURE: Full gameplay / code with version 1 whitewood
- FUTURE: Develop schematics for the custom expander boards based on TI

# Full Game Code Status
- **Dragons of Destiny (DoD)** — fantasy-themed full game now enabled, utilizing all aspects of code: lights, sound, music, devices, 3D graphics, NeoPixels, and more.
- **Attract sequence:** Castle doors → animated torches → "Press Start" blink → cycles through Instructions and High Score screens every 18 s.
- **Rules (1–4 players):**
  - **Pop bumpers:** 3 pops on the upper playfield — each hit scores 50 pts and adds +1 gold; auto-fire solenoid triggers immediately via the auto-output system.
  - **Slingshots:** L/R slings score 1,000 pts each hit; every 10,000-point sling milestone awards an extra ball and activates ball save.
  - **Inn lanes:** 3 lanes at the top of the field; completing all 3 scores 250 pts each, sets Inn Open, and resets lane LEDs. Flippers rotate the lit lane indicator left/right.
  - **Key targets:** 3 standup targets; completing all 3 scores 250 pts each and sets Key Obtained. State tracked and displayed in the status panel.
  - **Inlanes + ball save:** Hitting both inlane sensors lights the SAVE LED and activates a 5-second ball-save timer.
  - **Character recruit & dungeon:** Per-player party members (Knight/Priest/Ranger) and dungeon floor/level tracked and displayed in the status panel. Full recruitment mechanic is planned.
  - **Ball hopper eject device** supported.
- See **[Dragons of Destiny Table Spec](documentation/DoDTable.md)** for full state machine diagrams, I/O map, per-player data structures, and mode-by-mode details.

# Documentation
- **[RasPin Overview](documentation/RasPin_Overview.md)** - **START HERE** - High-level architecture overview with diagrams and introduction to all documentation
- **[How To Build](documentation/HowToBuild.md)** - Build instructions for Windows and Raspberry Pi

- **API Reference Documentation:**
  - **[PBEngine API](documentation/PBEngine_API.md)** - Core engine class, game state, timer system, and output control
  - **[I/O Processing API](documentation/IO_Processing_API.md)** - Input/output message processing and hardware communication
  - **[LED Control API](documentation/LED_Control_API.md)** - LED control, NeoPixel RGB strips, sequences, and animation patterns
  - **[NeoPixel Timing Methods](documentation/NeoPixel_Timing_Methods.md)** - Timing methods for reliable NeoPixel control (clock_gettime, NOP, SPI, PWM)
  - **[NeoPixel Instrumentation](documentation/NeoPixel_Instrumentation.md)** - Diagnostic tools for NeoPixel timing verification
  - **[PBDevice API](documentation/PBDevice_API.md)** - Device management framework for complex pinball mechanisms
  - **[Platform Init API](documentation/Platform_Init_API.md)** - Platform initialization, main loop, and configuration
  - **[Game Creation API](documentation/Game_Creation_API.md)** - Graphics (PBGfx), sound (PBSound), video playback (PBVideoPlayer), sprites, animations, and screen management
  - **[3D Rendering API](documentation/PB3D_API.md)** - 3D model loading (glTF/GLB), instance management, lighting, perspective animation, and coordinate system

- **[Dragons of Destiny Table Spec](documentation/DoDTable.md)** - Full specification for the *Dragons of Destiny* example table: gameplay overview, all state machines with transition diagrams, I/O pin assignments, per-player data structures, screen-manager system, and AI prompt templates for each mode

- **[Utilities Guide](documentation/Utilities_Guide.md)** - Complete guide for all RasPin utilities:
  - **FontGen** (Windows & Raspberry Pi) - Converts TrueType fonts to texture atlases for text rendering
  - **pblistdevices** (Raspberry Pi only) - Scans I2C bus and lists all connected hardware devices
  - **pbsetamp** (Raspberry Pi only) - Controls MAX9744 amplifier volume settings
  - **pblaunch** (Linux / Rasberry Pi Only) - A GUI based utility to find/launch/stop the EXE without the development environment.

# Design and Development guidelines
-  Actual machine based on Raspberry Pi 5 and PiOS for ease of development and debug.  Full power of Linux OS.  
-  Cross platform support via VS Code: Shared code between Linux/PiOS and Windows - using Windows for high power development / simulation, although could do this with a full Linux PC as well.
-  Keep the structure and coding straightfoward so that intermediate level coders and utilize the system and build new tables.
-  Modular / HAL based graphics engine - allow for easy upgrade to other APIs if desired (currently OGL 3.1 ES, but maybe Vulkan later?)
-  All IO (sensor/button inputs, lighting and solenoid outputs) routed through Pi controller.
    - Allows for complete decoupling of inputs and outputs - maximum flexibility of how to respond to inputs but design to reduce latency
    - IO expanders (based on TI TCA9555) / driver boards used in conjunction with Pi controller to greatly expand number of elements in the system.
    - LED expander (based on TI TLC59116) with blinking, dimming and group control to offload LED management for core HW as much as possible
    - NeoPixel support via Raspberry Pi5 SPI pins (support up to two distinct chains)
    - Modular structure for I/O devices to allow ease of integration of any new controllers.
-  Sound and display using standard HDMI and audio output jacks from the Pi controller.  The code has flexibility in primary screen selection for the table.
    - Can use HDMI speaker sound or external amplifier based on readily avaialble MAX9744 breakout boards.
-  Simple sound usage with WAV / MP3 files for easy to use music and sound effects.
-  Graphics (2D, Sprites, 3D) have simplified user interfaces and are all screen coodinate based for easy understanding.
-  Allow for flexibility in configuration, eg: standard menus but SW is architected to quickly change tables and layouts.
-  Everything should be rougly half-scale compared to a normal pinball machine - currently targeing 15" x 24" playfield, with a 5/8" ball.
    -  All custom pinball mechanisms will be developed with 3D models for 3D printing, allowing others to easily duplicate the construction.
    -  Cabinet / backbox will be developed using 1/4" plywood for weight / cost.  Playfield will be 1/2" plywood.
-  Focus on architecture and design, utilize AI for speed of development but review all code closely to ensure desired behavior

# About the lead developer
-  Jeff has been a computer engineer in the industry for 30+ years, working for major tech firms, primarily in HW/SW interfacing and testing for graphics and AI drivers.  Creating SW to talk to HW and graphics engines at that level has always been a hobby and pinball seems to be an excellent overall systems and architectural challenge to do all the things that are interesting.

# License Guidelines
-  TL;DR: The SW and all files are open source unless otherwise noted, and available to users for personal but not commercial projects.  Any contributions to the repo become available for all to use.  The original repo owner reserves the rights to use the code and all contributions for commercial use.  Hey - I might want to build and sell a few machines at some point... ;)  If you want to use the code for commercial purposes, then let's talk.

- Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.  See more specific details and other license terms in the root of the project


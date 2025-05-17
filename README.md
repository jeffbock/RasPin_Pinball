# (PI)nball
Half-scale pinball project using Raspberry Pi.  The goal is to fully develop a system of SW and HW using Raspberry PI and various easy to source HW pieces to develop a half size machine with all the basic pinball requirements.  

<img src="https://github.com/user-attachments/assets/57f19724-b2ef-4bbc-9d49-0114196ab59f" alt="Screenshot 2025-05-13 224816" width="400">
<img src="https://github.com/user-attachments/assets/b3703c41-70b1-40f0-bd77-2578b7076be8" alt="Screenshot 2025-05-13 224901" width="400">  

# Feature Goals
- Standard piball electro-magnetic devices:  Bumpers, Pop-Bumpers, Slingshots, Lane sensors, flippers
- Full lighting and sound effects, including reasonably large display
- Interactive mix between pinball and electronic screen for both physical and video-game like experiences.

# Design and Development guidelines
-  Actual machine based on Raspberry Pi 5 and PiOS for ease of development and debug.
-  Cross platform support: Shared code between PiOS and Windows - Windows for high power development / simulation.
-  Utilize VS Code as standard cross-platform environment
-  Develop core code from scratch - for learning, and ultimate low level control of Pi Controller.
-  OpenGL ES backend sprite engine, built on PInball graphics HAL.  Allows for easy upgrade to different backend later (Vulkan?)
-  All sensor/button inputs, lighting and solenoid outputs routed through Pi controller.
    - Allows for complete decoupling of inputs and outputs - maximum flexibility of how to respond to inputs.
    - IO expanders / driver boards used in conjunction with Pi controller to greatly expand number of elements in the system.
-  Sound and display using standard HDMI and audio output jacks from the Pi controller.
-  Allow for flexibility in configuration, eg: standard menus but SW should be architectued to quickly change tables and layouts.
-  Everything should be rougly half-scale compared to a normal pinball machine - which means ~14" W x 28"H play area with a 1/2" ball.
    -  All custom pinball mechanisms will be developed with 3D models for 3D printing, allowing others to easily duplicate the construction.
    -  Cabinet will be developed using bass wood plywood.

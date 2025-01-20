# (PI)nball
Half-scale pinball project using Raspberry Pi.  The goal is to fully develop a system of SW and HW using Raspberry PI and various easy to source HW pieces to develop a reasonably modern half-size pinball machine.  

# Feature Goals
- Standard piball electro-magnetic devices:  Bumpers, Pop-Bumpers, Lane sensors, flippers, auto-ball ejectors.
- Full lighting and sound effects, including reasonably large display
- Interactive mix between pinball and electronic screen for both physical and video-game like experiences.

# Architecture guidelines
-  Based on Raspberry Pi 5 and PiOS for ease of development and debug.
-  All sensor/button inputs, lighting and solenoid outputs routed through Pi controller.
    - Allows for complete decoupling of inputs and outputs - maximum flexibility of how to respond to inputs.
    - IO expanders / driver boards used in conjunction with Pi controller to greatly expand number of elements in the system.
-  Sound and display using standard HDMI and audio output jacks from the Pi controller.
-  Software-wise, build devices from class components - sensors, solenoids, etc..
-  Allow for flexibility in configuration - utilze JSON files to build HW configuration to quickly change tables and layouts.
-  Everything should be rougly half-scale compared to a normal pinball machine - which means ~14" W x 28"W play area with a 1/2" ball.
    -  All custom pinball mechanisms will be developed with 3D models for 3D printing, allowing others to easily duplicate the construction.
    -  Cabinet will be developed using bass wood plywood.

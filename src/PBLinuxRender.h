// Window and render code for Linux (Raspberry Pi and Debian)

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef PBLinuxRender_h
#define PBLinuxRender_h

#include "PBBuildSwitch.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <iostream>

EGLNativeWindowType PBInitLinuxRender (long width, long height);

#ifdef EXE_MODE_DEBIAN
Display* PBGetLinuxDisplay();
Window PBGetLinuxWindow();
Atom PBGetLinuxWMDeleteWindow();
#endif

#endif // PBLinuxRender_h
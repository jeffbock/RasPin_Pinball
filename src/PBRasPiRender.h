// Window and render code for Rasbery Pi

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef PBRasPiRender_h
#define PBRasPiRender_h

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <iostream>

EGLNativeWindowType PBInitPiRender (long width, long height);

#endif // PBRasPiRender_h
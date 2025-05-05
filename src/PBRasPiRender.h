// Window and render code for Rasbery Pi
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

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
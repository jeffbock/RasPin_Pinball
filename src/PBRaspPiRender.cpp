
// Window and render code for Rasbery Pi
/// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "PBRasPiRender.h"

#ifdef EXE_MODE_DEBIAN
static Display* g_PiDisplay = nullptr;
static Window g_PiWindow = 0;
#endif

EGLNativeWindowType PBInitPiRender (long width, long height) {

    #ifdef EXE_MODE_DEBIAN
    // Disable XIM so compose/dead-key UI does not intercept simulator control keys.
    XSetLocaleModifiers("@im=none");
    #endif

    // Open X11 display
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        return (0);
    }

    // Get the default screen
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

     // Query RandR for monitor information
    XRRScreenResources* screenResources = XRRGetScreenResources(display, root);
    if (!screenResources) {
        XCloseDisplay(display);
        return (0);
    }

    int useMonitor = -1;
    int xPos = 0, yPos = 0;
    XRROutputInfo* outputInfo;

    // Print available monitors
    for (int i = 0; i < screenResources->noutput; ++i) {
        outputInfo = XRRGetOutputInfo(display, screenResources, screenResources->outputs[i]);
        if (outputInfo->connection == RR_Connected) {
            if (outputInfo->crtc) {
                XRRCrtcInfo* crtcInfo = XRRGetCrtcInfo(display, screenResources, outputInfo->crtc);
                if ((crtcInfo->width == width) && (crtcInfo->height == height)) {
                    useMonitor = i;
                    xPos = crtcInfo->x; 
                    yPos = crtcInfo->y; 
                }
                XRRFreeCrtcInfo(crtcInfo);
            }
        }

        // If the desired size monitor was found, break
        XRRFreeOutputInfo(outputInfo);
        if (useMonitor != -1) break;
    }

    // Couldn't find the right size monitor
    if (useMonitor == -1)
    {
        XRRFreeScreenResources(screenResources);
        XCloseDisplay(display);
        return (0);
    }

    // Create a full-screen X11 window
    XSetWindowAttributes attributes;
    attributes.override_redirect = True;
    Window window = XCreateWindow(display, root, xPos, yPos, width, height,0,
                                  CopyFromParent, InputOutput, CopyFromParent, 
                                  CWOverrideRedirect, &attributes);

    if(!window)
    {
        XRRFreeScreenResources(screenResources);
        XCloseDisplay(display);
        return (0);
    }

    // Set the window to full-screen
    Atom wmState = XInternAtom(display, "_NET_WM_STATE", False);
    Atom wmStateFullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    XChangeProperty(display, window, wmState, XA_ATOM, 32,
                    PropModeReplace, (unsigned char*)&wmStateFullscreen, 1);

    #ifdef EXE_MODE_DEBIAN
    XSelectInput(display, window, KeyPressMask | KeyReleaseMask | StructureNotifyMask);
    g_PiDisplay = display;
    g_PiWindow = window;
    #endif

    // Map (show) the window
    XMapWindow(display, window);
    XFlush(display);
    
    return (window);
}

#ifdef EXE_MODE_DEBIAN
Display* PBGetPiDisplay() {
    return g_PiDisplay;
}

Window PBGetPiWindow() {
    return g_PiWindow;
}
#endif


// Window and render code for Rasbery Pi
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#include "PBRasPiRender.h"

EGLNativeWindowType PBInitPiRender (long width, long height) {

    // Open X11 display
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Failed to open X11 display" << std::endl;
        return (0);
    }

    // Get the default screen
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

     // Query RandR for monitor information
    XRRScreenResources* screenResources = XRRGetScreenResources(display, root);
    if (!screenResources) {
        std::cerr << "Failed to get RandR screen resources" << std::endl;
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
        std::cerr << "No monitor found with desired resolution" << std::endl;
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
        std::cerr << "Failed to create base X11 Window" << std::endl;
        XRRFreeScreenResources(screenResources);
        XCloseDisplay(display);
        return (0);
    }

    // Set the window to full-screen
    Atom wmState = XInternAtom(display, "_NET_WM_STATE", False);
    Atom wmStateFullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    XChangeProperty(display, window, wmState, XA_ATOM, 32,
                    PropModeReplace, (unsigned char*)&wmStateFullscreen, 1);

    // Map (show) the window
    XMapWindow(display, window);
    XFlush(display);
    
    return (window);
}

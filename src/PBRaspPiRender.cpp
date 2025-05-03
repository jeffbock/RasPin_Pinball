
// Window and render code for Rasbery Pi
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#include "PBRasPiRender.h"

bool PBInitPiRender (long width, long height) {

    // Open X11 display
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Failed to open X display" << std::endl;
        return (false);
    }

    Window root DefaultRootWindow(display);

    XRRScreenConfiguration* screenConfig = XRRGetScreenInfo(display, root);
    if (!screenConfig){
        std::cerr << "Failed to get the screen configurations from Xrandr" << std::endl;
        return (false);
    }

    

    // Find the right display to use
    int count = ScreenCount(display);
    int screenNum = -1;

    // Look for a screen that matches the pinball width / height
    for (int i = 0; i <= count; i++){
        Screen *screen = ScreenOfDisplay(display, i);
        if ((screen->height == height) && (screen->width == width)) screenNum = i;
    }

    if (screenNum == -1) {
        std::cerr << "No Screen Matching Desired Size" << std::endl;
        return (false);
    }

    Window root = RootWindow(display, screenNum);

    // Create a full-screen X11 window
    Window window = XCreateSimpleWindow(display, root, 0, 0,
                                        width,
                                        height,
                                        0, BlackPixel(display, screenNum),
                                        BlackPixel(display, screenNum));

    // Set the window to full-screen
    Atom wmState = XInternAtom(display, "_NET_WM_STATE", False);
    Atom wmStateFullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    XChangeProperty(display, window, wmState, XA_ATOM, 32,
                    PropModeReplace, (unsigned char*)&wmStateFullscreen, 1);

    // Map (show) the window
    XMapWindow(display, window);

    return (true);
}

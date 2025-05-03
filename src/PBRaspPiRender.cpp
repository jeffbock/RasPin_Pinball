
// Window and render code for Rasbery Pi
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#include "PBRasPiRender.h"

bool PBInitPiRender (long width, long height) {

    // Open X11 display
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Failed to open X display" << std::endl;
        return -1;
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    // Create a full-screen X11 window
    Window window = XCreateSimpleWindow(display, root, 0, 0,
                                        DisplayWidth(display, screen),
                                        DisplayHeight(display, screen),
                                        0, BlackPixel(display, screen),
                                        BlackPixel(display, screen));


    return (true);
}


// Window and render code for Rasbery Pi
/// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "PBRasPiRender.h"

#ifdef EXE_MODE_DEBIAN
#include <array>
#include <cstdlib>
#include <cstring>
#endif

#ifdef EXE_MODE_DEBIAN
static Display* g_PiDisplay = nullptr;
static Window g_PiWindow = 0;
static Atom g_wmDeleteWindow = 0;
#endif

EGLNativeWindowType PBInitPiRender (long width, long height) {

    #ifdef EXE_MODE_DEBIAN
    // Disable XIM so compose/dead-key UI does not intercept simulator control keys.
    XSetLocaleModifiers("@im=none");
    #endif

    // Open X11 display.
    // In some Debian debug/task launches DISPLAY may be unset or point to a
    // non-existent server. Fall back to common local X displays.
    Display* display = XOpenDisplay(nullptr);
#ifdef EXE_MODE_DEBIAN
    if (!display) {
        const char* envDisplay = std::getenv("DISPLAY");
        if (envDisplay != nullptr && std::strlen(envDisplay) > 0) {
            display = XOpenDisplay(envDisplay);
        }

        if (!display) {
            constexpr std::array<const char*, 12> kDisplayFallbacks = {
                ":0", ":0.0", ":1", ":1.0", ":10", ":10.0", ":11", ":11.0", ":12", ":12.0", ":13", ":13.0"
            };
            for (const char* fallbackDisplay : kDisplayFallbacks) {
                if (envDisplay != nullptr && std::strcmp(envDisplay, fallbackDisplay) == 0) {
                    continue;
                }

                display = XOpenDisplay(fallbackDisplay);
                if (display != nullptr) {
                    break;
                }
            }
        }
    }
#endif
    if (!display) {
        return (0);
    }

    // Get the default screen
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);
    Window window = 0;

    #ifdef EXE_MODE_RASPI
    // Query RandR for monitor information to place the fullscreen window on
    // the display that matches the requested pinball dimensions.
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

    // Create a full-screen X11 window for Raspberry Pi hardware display
    XSetWindowAttributes attributes;
    attributes.override_redirect = True;
    window = XCreateWindow(display, root, xPos, yPos, width, height, 0,
                           CopyFromParent, InputOutput, CopyFromParent,
                           CWOverrideRedirect, &attributes);

    if (!window)
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

    XRRFreeScreenResources(screenResources);
    #endif // EXE_MODE_RASPI

    #ifdef EXE_MODE_DEBIAN
    // Create the simulator window.  With SIMULATOR_SMALL_WINDOW the window is 1/4 the virtual
    // screen area (half width, half height) for improved RDP performance; without it the
    // window uses the full virtual resolution.  Game rendering coordinates are unchanged.
    #ifdef SIMULATOR_SMALL_WINDOW
    const long winWidth  = width  / 2;
    const long winHeight = height / 2;
    #else
    const long winWidth  = width;
    const long winHeight = height;
    #endif
    window = XCreateSimpleWindow(display, root, 0, 0,
                                 (unsigned int)winWidth, (unsigned int)winHeight, 0,
                                 BlackPixel(display, screen), BlackPixel(display, screen));

    if (!window)
    {
        XCloseDisplay(display);
        return (0);
    }

    // Set the window title
    XStoreName(display, window, "Pinball Simulator");

    // Prevent user from resizing the window by locking min/max to the (reduced) window
    // dimensions, mirroring the WS_OVERLAPPED | WS_SYSMENU style used by the Windows
    // simulator (no WS_THICKFRAME / WS_MAXIMIZEBOX).
    XSizeHints sizeHints;
    sizeHints.flags     = PMinSize | PMaxSize;
    sizeHints.min_width  = sizeHints.max_width  = (int)winWidth;
    sizeHints.min_height = sizeHints.max_height = (int)winHeight;
    XSetWMNormalHints(display, window, &sizeHints);

    // Register WM_DELETE_WINDOW so clicking the close button delivers a
    // ClientMessage instead of killing the process immediately.
    g_wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &g_wmDeleteWindow, 1);

    // Select keyboard and structure events; focus is handled by the WM normally.
    XSelectInput(display, window, KeyPressMask | KeyReleaseMask | StructureNotifyMask);
    g_PiDisplay = display;
    g_PiWindow  = window;
    #endif // EXE_MODE_DEBIAN

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

Atom PBGetPiWMDeleteWindow() {
    return g_wmDeleteWindow;
}
#endif

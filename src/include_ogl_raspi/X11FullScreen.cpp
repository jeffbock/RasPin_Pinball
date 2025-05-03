#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <iostream>

int main() {
    // Open X11 display
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Failed to open X display" << std::endl;
        return -1;
    }

    // Get the default screen
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    // Query RandR for monitor information
    XRRScreenResources* screenResources = XRRGetScreenResources(display, root);
    if (!screenResources) {
        std::cerr << "Failed to get RandR screen resources" << std::endl;
        XCloseDisplay(display);
        return -1;
    }

    // Print available monitors
    std::cout << "Available monitors:" << std::endl;
    for (int i = 0; i < screenResources->noutput; ++i) {
        XRROutputInfo* outputInfo = XRRGetOutputInfo(display, screenResources, screenResources->outputs[i]);
        if (outputInfo->connection == RR_Connected) {
            std::cout << "Monitor " << i << ": " << outputInfo->name
                      << " (" << outputInfo->mm_width << "mm x " << outputInfo->mm_height << "mm)" << std::endl;

            if (outputInfo->crtc) {
                XRRCrtcInfo* crtcInfo = XRRGetCrtcInfo(display, screenResources, outputInfo->crtc);
                std::cout << "  Position: (" << crtcInfo->x << ", " << crtcInfo->y << ")"
                          << ", Resolution: " << crtcInfo->width << "x" << crtcInfo->height << std::endl;
                XRRFreeCrtcInfo(crtcInfo);
            }
        }
        XRRFreeOutputInfo(outputInfo);
    }

    // Select a specific monitor (e.g., monitor 1)
    int selectedMonitor = 1; // Change this to the desired monitor index
    if (selectedMonitor >= screenResources->noutput) {
        std::cerr << "Invalid monitor index" << std::endl;
        XRRFreeScreenResources(screenResources);
        XCloseDisplay(display);
        return -1;
    }

    XRROutputInfo* selectedOutput = XRRGetOutputInfo(display, screenResources, screenResources->outputs[selectedMonitor]);
    if (!selectedOutput || selectedOutput->connection != RR_Connected || !selectedOutput->crtc) {
        std::cerr << "Selected monitor is not connected or has no CRTC" << std::endl;
        XRRFreeOutputInfo(selectedOutput);
        XRRFreeScreenResources(screenResources);
        XCloseDisplay(display);
        return -1;
    }

    XRRCrtcInfo* crtcInfo = XRRGetCrtcInfo(display, screenResources, selectedOutput->crtc);

    // Create a full-screen X11 window on the selected monitor
    Window window = XCreateSimpleWindow(display, root,
                                        crtcInfo->x, crtcInfo->y,
                                        crtcInfo->width, crtcInfo->height,
                                        0, BlackPixel(display, screen),
                                        BlackPixel(display, screen));

    // Set the window to full-screen
    Atom wmState = XInternAtom(display, "_NET_WM_STATE", False);
    Atom wmStateFullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    XChangeProperty(display, window, wmState, XA_ATOM, 32,
                    PropModeReplace, (unsigned char*)&wmStateFullscreen, 1);

    // Map (show) the window
    XMapWindow(display, window);

    // Initialize EGL
    EGLDisplay eglDisplay = eglGetDisplay((EGLNativeDisplayType)display);
    if (eglDisplay == EGL_NO_DISPLAY) {
        std::cerr << "Failed to get EGL display" << std::endl;
        return -1;
    }

    if (!eglInitialize(eglDisplay, nullptr, nullptr)) {
        std::cerr << "Failed to initialize EGL" << std::endl;
        return -1;
    }

    // Configure EGL
    EGLConfig config;
    EGLint numConfigs;
    EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_NONE
    };
    eglChooseConfig(eglDisplay, attribs, &config, 1, &numConfigs);

    // Create an EGL window surface
    EGLSurface eglSurface = eglCreateWindowSurface(eglDisplay, config, (EGLNativeWindowType)window, nullptr);
    if (eglSurface == EGL_NO_SURFACE) {
        std::cerr << "Failed to create EGL surface" << std::endl;
        return -1;
    }

    // Create an EGL context
    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext eglContext = eglCreateContext(eglDisplay, config, EGL_NO_CONTEXT, contextAttribs);
    if (eglContext == EGL_NO_CONTEXT) {
        std::cerr << "Failed to create EGL context" << std::endl;
        return -1;
    }

    // Make the context current
    eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);

    // OpenGL ES rendering loop
    while (true) {
        // Clear the screen
        glClearColor(0.0f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Swap buffers
        eglSwapBuffers(eglDisplay, eglSurface);

        // Handle X11 events (e.g., close window)
        XEvent event;
        while (XPending(display)) {
            XNextEvent(display, &event);
            if (event.type == KeyPress) {
                goto cleanup; // Exit on key press
            }
        }
    }

cleanup:
    // Cleanup
    eglDestroySurface(eglDisplay, eglSurface);
    eglDestroyContext(eglDisplay, eglContext);
    eglTerminate(eglDisplay);
    XDestroyWindow(display, window);
    XRRFreeCrtcInfo(crtcInfo);
    XRRFreeOutputInfo(selectedOutput);
    XRRFreeScreenResources(screenResources);
    XCloseDisplay(display);

    return 0;
}
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xinerama.h>
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

    // Check for Xinerama support
    if (!XineramaIsActive(display)) {
        std::cerr << "Xinerama is not active on this display" << std::endl;
        XCloseDisplay(display);
        return -1;
    }

    // Get the list of screens
    int numScreens;
    XineramaScreenInfo* screens = XineramaQueryScreens(display, &numScreens);
    if (!screens) {
        std::cerr << "Failed to query Xinerama screens" << std::endl;
        XCloseDisplay(display);
        return -1;
    }

    // Print available screens
    std::cout << "Available screens:" << std::endl;
    for (int i = 0; i < numScreens; ++i) {
        std::cout << "Screen " << i << ": "
                  << "x=" << screens[i].x_org
                  << ", y=" << screens[i].y_org
                  << ", width=" << screens[i].width
                  << ", height=" << screens[i].height
                  << std::endl;
    }

    // Select a specific screen (e.g., screen 1)
    int selectedScreen = 1; // Change this to the desired screen index
    if (selectedScreen >= numScreens) {
        std::cerr << "Invalid screen index" << std::endl;
        XFree(screens);
        XCloseDisplay(display);
        return -1;
    }

    XineramaScreenInfo screen = screens[selectedScreen];

    // Create a full-screen X11 window on the selected screen
    Window root = RootWindow(display, DefaultScreen(display));
    Window window = XCreateSimpleWindow(display, root,
                                        screen.x_org, screen.y_org,
                                        screen.width, screen.height,
                                        0, BlackPixel(display, DefaultScreen(display)),
                                        BlackPixel(display, DefaultScreen(display)));

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
    XFree(screens);
    XCloseDisplay(display);

    return 0;
}
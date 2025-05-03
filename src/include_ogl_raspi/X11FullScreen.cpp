#include <X11/Xlib.h>
#include <X11/Xatom.h>
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

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    // Create a full-screen X11 window
    Window window = XCreateSimpleWindow(display, root, 0, 0,
                                        DisplayWidth(display, screen),
                                        DisplayHeight(display, screen),
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
    XCloseDisplay(display);

    return 0;
}
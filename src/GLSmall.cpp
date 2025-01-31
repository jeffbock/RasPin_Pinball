#include <windows.h>
#include "include_ogl/egl.h"
#include "include_ogl/gl31.h"
#include <stdio.h>

HWND CreateNativeWindow(int width, int height, const char* title) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "ANGLEWindowClass";
    RegisterClass(&wc);

    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindow(
        wc.lpszClassName, title, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, wc.hInstance, nullptr);

    return hwnd;
}

int main(int argc, char const *argv[])
{
    HWND hwnd = CreateNativeWindow(640, 480, "ANGLE Window");
    if (!hwnd) {
        printf("Error: Failed to create window\n");
        return -1;
    }

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        printf("Error: eglGetDisplay() failed\n");
        return -1;
    }

    if (!eglInitialize(display, nullptr, nullptr)) {
        printf("Error: eglInitialize() failed\n");
        return -1;
    }

    EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_NONE
    };
    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig(display, configAttribs, &config, 1, &numConfigs) || numConfigs < 1) {
        printf("Error: eglChooseConfig() failed\n");
        return -1;
    }

    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        printf("Error: eglCreateContext() failed\n");
        return -1;
    }

    EGLSurface surface = eglCreateWindowSurface(display, config, hwnd, nullptr);
    if (surface == EGL_NO_SURFACE) {
        printf("Error: eglCreateWindowSurface() failed\n");
        return -1;
    }

    if (!eglMakeCurrent(display, surface, surface, context)) {
        printf("Error: eglMakeCurrent() failed\n");
        return -1;
    }

    bool isBlack = false;

    while (true) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                return 0;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Just flip the screen back and forth between black and white
        glViewport(0, 0, 640, 480);
        if (!isBlack) {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            isBlack = true;
        } else {
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            isBlack = false;
        }
        glClear(GL_COLOR_BUFFER_BIT);

        if (eglSwapBuffers(display, surface) != EGL_TRUE) {
            printf("Error: eglSwapBuffers() failed\n");
            break;
        }

        Sleep(16); // Sleep to limit the frame rate to ~60 FPS
    }

    return 0;
}

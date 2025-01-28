#include "include_ogl/egl.h"
#include "include_ogl/gl31.h"
#include <stdio.h>

int main(int argc, char const *argv[])
{
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, nullptr, nullptr);

    EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_NONE
    };
    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(display, configAttribs, &config, 1, &numConfigs);

    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);

    EGLint surfaceAttribs[] = {
        EGL_WIDTH, 640,
        EGL_HEIGHT, 480,
        EGL_NONE
    };
    EGLSurface surface = eglCreatePbufferSurface(display, config, surfaceAttribs);
    eglMakeCurrent(display, surface, surface, context);

    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    GLuint renderbuffer;
    glGenRenderbuffers(1, &renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 640, 480);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("Error: Framebuffer is not complete!\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    while (true) 
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glViewport(0, 0, 640, 480);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Add your rendering code here

        eglSwapBuffers(display, surface);
    }

    return 0;
}



// PBOGLES - OpenGL ES 3.1 rendering backend for Windows / Rasberry Pi
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#ifndef PBOGLES_h
#define PBOGLES_h

#include "include_ogl/egl.h"
#include "include_ogl/gl31.h"
#include <stdio.h>
#include <iostream>
#include <fstream>

#define OGLES_BLACKCOLOR 0x0 
#define OGLES_WHITECOLOR 0x1

// Define a class for the OGL ES code
class PBOGLES {

public:

    enum oglTexType {
        OGL_BMP = 0,
        OGL_TEXTURE = 1, 
        OGL_TTEND
    };

    PBOGLES();
    ~PBOGLES();

    bool oglInit (long width, long height, NativeWindowType nativeWindow) ;
    bool oglClear (float red, float blue, float green, float alpha, bool doFlip);
    bool oglSwap ();
    unsigned int oglGetScreenHeight();
    unsigned int oglGetScreenWidth();

protected:
    GLuint oglLoadTexture(const char* filename, oglTexType type, unsigned int* width, unsigned int* height);
    void oglRenderQuad (float X1, float Y1, float X2, float Y2, float scale, unsigned int rotateDegrees,
                        unsigned int textureId, float alpha);

private:
    long m_width;
    long m_height;
    bool m_started;
    
    // OGLES Context variables
    EGLDisplay m_display;
    EGLContext m_context;
    EGLSurface m_surface;
    GLuint     m_shaderProgram;
    // This will likely be a vector or something.. need to manage using and freeing textures
    GLuint     m_texture;

    GLuint oglCompileShader(GLenum type, const char* source);
    GLuint oglCreateProgram(const char* vertexSource, const char* fragmentSource);
    void   oglCreateShaders();
    void   oglCleanup();

    // Shaders used for sprite rendering.  All sprites are quads, with textures and an overall alpha control value
    // Vertex shader source code
    const char* vertexShaderSource = R"(
        attribute vec4 vPosition;
        attribute vec4 vColor;
        attribute vec2 vTexCoord;
        varying vec4 fColor;
        varying vec2 fTexCoord;
        void main() {
            gl_Position = vPosition;
            fColor = vColor;
            fTexCoord = vTexCoord;
        }
    )";

    // Fragment shader source code
    const char* fragmentShaderSource = R"(
        precision mediump float;
        varying vec4 fColor;
        varying vec2 fTexCoord;
        uniform sampler2D uTexture;
        uniform float uAlpha;
        uniform bool useTexture;
        void main() {
            vec4 texColor = useTexture ? texture2D(uTexture, fTexCoord) : vec4(1.0);
            gl_FragColor = texColor * fColor * uAlpha;
        }
    )";
};

#endif // PBOGLES_h

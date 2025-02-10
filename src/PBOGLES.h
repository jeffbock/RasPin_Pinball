// PBOGLES - OpenGL ES 3.1 rendering backend for Windows / Rasberry Pi
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#ifndef PBOGLES_h
#define PBOGLES_h

#include "include_ogl/egl.h"
#include "include_ogl/gl31.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include "stb_image.h"

#define OGLES_BLACKCOLOR 0x0 
#define OGLES_WHITECOLOR 0x1

// Define a class for the OGL ES code
class PBOGLES {

public:

    enum oglTexType {
        OGL_BMP = 0,
        OGL_PNG = 1, 
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
    GLuint oglLoadBMPTexture (const char* filename, unsigned int* width, unsigned int* height);
    GLuint oglLoadPNGTexture (const char* filename, unsigned int* width, unsigned int* height);
    void oglRenderQuad (float* X1, float* Y1, float* X2, float* Y2, float scale, float rotateDegrees,
                        bool useCenter, bool returnBoundingBox, unsigned int textureId, bool useAlpha, float alpha);
    void scaleAndRotateVertices(float* x, float* y, float scale, float rotateDegrees);
    void oglSetQuadColor(float red, float green, float blue, float alpha);

private:
    long m_width;
    long m_height;
    float m_aspectRatio;
    unsigned int m_lastTextureId;
    bool m_started;
    float m_quadRed, m_quadGreen, m_quadBlue, m_quadAlpha;
    
    // OGLES Context variables
    EGLDisplay m_display;
    EGLContext m_context;
    EGLSurface m_surface;

    // Shader variables
    GLuint     m_shaderProgram;
    GLuint     m_texture;
    GLint      m_posAttrib;
    GLint      m_colorAttrib; 
    GLint      m_texCoordAttrib; 
    GLint      m_uTexAlpha;
    GLint      m_useTexture;
    GLint      m_useTexAlpha;

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
        uniform float uTexAlpha;
        uniform bool useTexture;
        uniform bool useTexAlpha;
        void main() {
            vec4 texColor = useTexture ? texture2D(uTexture, fTexCoord) : vec4(1.0);
            texColor.a = useTexAlpha ? uTexAlpha : texColor.a;
            gl_FragColor = texColor * fColor;
        }
    )";
};

#endif // PBOGLES_h

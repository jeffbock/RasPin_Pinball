// Univerasl OGL ES Code for all platforms

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

    PBOGLES();
    ~PBOGLES();

    bool gfxInit (long width, long height, NativeWindowType nativeWindow) ;
    bool gfxClear (long color, bool doFlip);
    void gfxRenderQuad ();
    bool gfxSwap ();

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

    GLuint gfxCompileShader(GLenum type, const char* source);
    GLuint gfxCreateProgram(const char* vertexSource, const char* fragmentSource);
    void   gfxCreateShaders();
    bool   gfxLoadBMPTexture(const char* filename);
    void   gfxCleanup();

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
        void main() {
            vec4 texColor = texture2D(uTexture, fTexCoord);
            gl_FragColor = texColor * fColor * uAlpha;
        }
    )";
};

#endif // PBOGLES_h

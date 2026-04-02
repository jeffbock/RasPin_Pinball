// PBOGLES - OpenGL ES 3.1 rendering backend for Windows / Rasberry Pi

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef PBOGLES_h
#define PBOGLES_h

#include "PBBuildSwitch.h"

// Define WIN32_LEAN_AND_MEAN before including EGL headers to avoid redefinition warnings
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

//#include "include_ogl/egl.h"
//#include "include_ogl/gl31.h"
#include <egl.h>
#include <gl31.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include "3rdparty/stb_image.h"

#define OGLES_BLACKCOLOR 0x0 
#define OGLES_WHITECOLOR 0x1

// Define a class for the OGL ES code
class PBOGLES {

public:

    enum oglTexType {
        OGL_BMP = 0,
        OGL_PNG = 1, 
        OGL_NONE = 2,
        OGL_VIDEO = 3,      // Video texture (dynamically updated)
        OGL_TTEND
    };

    PBOGLES();
    ~PBOGLES();

    bool oglInit (long width, long height, NativeWindowType nativeWindow) ;
    bool oglClear (float red, float blue, float green, float alpha, bool doFlip);
    bool oglSwap (bool flush);
    void oglSetScissor (bool enable, int x1, int y1, int x2, int y2);
    unsigned int oglGetScreenHeight();
    unsigned int oglGetScreenWidth();

protected:
    bool   oglUnloadTexture(GLuint textureId);
    GLuint oglLoadTexture(const char* filename, oglTexType type, unsigned int* width, unsigned int* height);
    GLuint oglLoadBMPTexture (const char* filename, unsigned int* width, unsigned int* height);
    GLuint oglLoadPNGTexture (const char* filename, unsigned int* width, unsigned int* height);
    GLuint oglCreateVideoTexture(unsigned int width, unsigned int height);
    bool   oglUpdateTexture(GLuint textureId, const uint8_t* data, unsigned int width, unsigned int height);
    void   oglRenderQuad (float* X1, float* Y1, float* X2, float* Y2, float U1, float V1, float U2, float V2, 
                          bool useCenter, bool useTexAlpha, float texAlpha, unsigned int textureId, 
                          float vertRed, float vertGreen, float vertBlue, float vertAlpha, 
                          float scale, float rotateDegrees, bool returnBoundingBox);
    void   scaleAndRotateVertices(float* x, float* y, float scale, float rotateDegrees);
    GLuint oglCompileShader(GLenum type, const char* source);
    GLuint oglCreateProgram(const char* vertexSource, const char* fragmentSource);
    void   oglResetTextureCache() { m_lastTextureId = 0; }  // Call after any external glBindTexture to keep 2D cache valid
    void   oglRestore2DState();  // Restore full 2D rendering state after 3D pass (shader, attribs, blend, VBO unbind)

    // -----------------------------------------------------------------------
    // 3D rendering backend — all OpenGL ES calls for the 3D pass live here.
    // Called only from PB3D; PB3D itself makes no direct gl*() calls.
    // -----------------------------------------------------------------------

    // Shader lifecycle
    bool         ogl3dInitShader(const char* vertSrc, const char* fragSrc);
    void         ogl3dDestroyShader();

    // Skinned-mesh shader lifecycle (separate program with bone matrix array)
    bool         ogl3dInitSkinnedShader(const char* vertSrc, const char* fragSrc);
    void         ogl3dDestroySkinnedShader();

    // Mesh GPU resource management (interleaved layout: pos3 + norm3 + uv2 = 8 floats/vertex)
    bool         ogl3dCreateMesh(const float* vertData, size_t vertFloatCount,
                                 const unsigned int* idxData, size_t idxCount,
                                 unsigned int& outVao, unsigned int& outVbo, unsigned int& outEbo);
    void         ogl3dDestroyMesh(unsigned int vao, unsigned int vbo, unsigned int ebo);

    // Skinned mesh GPU resource management
    // Interleaved layout: pos3 + norm3 + uv2 + joints4 + weights4 = 16 floats/vertex
    bool         ogl3dCreateSkinnedMesh(const float* vertData, size_t vertFloatCount,
                                        const unsigned int* idxData, size_t idxCount,
                                        unsigned int& outVao, unsigned int& outVbo, unsigned int& outEbo);

    // Texture GPU resource management
    unsigned int ogl3dCreateTexture(const unsigned char* pixels, int width, int height);
    unsigned int ogl3dCreateFallbackTexture();
    void         ogl3dDestroyTexture(unsigned int texId);

    // Frame pass control
    void         ogl3dBeginPass();         // enable depth test, bind static 3D shader
    void         ogl3dBeginSkinnedPass();  // enable depth test, bind skinned 3D shader

    // Mid-frame shader switch: switch between static and skinned shaders
    // without re-clearing the depth buffer.  Call after ogl3dBeginPass().
    void         ogl3dActivateStaticShader();
    void         ogl3dActivateSkinnedShader();

    // Scene-level uniform upload (light direction/colour/ambient, camera eye)
    void         ogl3dSetSceneUniforms(float lightDirX, float lightDirY, float lightDirZ,
                                       float lightColR, float lightColG, float lightColB,
                                       float ambR,      float ambG,      float ambB,
                                       float eyeX,      float eyeY,      float eyeZ);

    // Scene-level uniforms for the skinned shader pass (same parameters, different program)
    void         ogl3dSetSkinnedSceneUniforms(float lightDirX, float lightDirY, float lightDirZ,
                                              float lightColR, float lightColG, float lightColB,
                                              float ambR,      float ambG,      float ambB,
                                              float eyeX,      float eyeY,      float eyeZ);

    // Per-instance uniform upload (MVP + model matrix + alpha) — static path
    void         ogl3dSetInstanceUniforms(const float mvp[16], const float modelMat[16], float alpha);

    // Per-instance uniform upload for skinned path (adds bone matrices array)
    void         ogl3dSetSkinnedInstanceUniforms(const float mvp[16], const float modelMat[16],
                                                 float alpha,
                                                  const float* boneMatrices, int numBones);
    void         ogl3dSetBlend(bool enable);

    // Draw one mesh primitive (VAO + texture already bound by caller)
    void         ogl3dDrawMeshPrimitive(unsigned int vao, unsigned int textureId, unsigned int indexCount);

private:
    // 2D shader variables
    GLuint     m_shaderProgram;
    GLint      m_posAttrib;
    GLint      m_colorAttrib; 
    GLint      m_texCoordAttrib; 
    long m_width;
    long m_height;
    float m_aspectRatio;
#ifdef SIMULATOR_SMALL_WINDOW
    // Actual surface dimensions (may differ from m_width/m_height when the
    // simulator window is smaller than the virtual game resolution).
    long m_surfaceWidth;
    long m_surfaceHeight;
#endif
    unsigned int m_lastTextureId;
    bool m_started;
    bool m_scissorEnabled;          // cached scissor test state — avoids redundant D3D11 rasterizer state churn on ANGLE
    bool m_depthTestEnabled;        // cached GL_DEPTH_TEST state — avoids redundant D3D11 rasterizer state churn on ANGLE
    bool m_cullFaceEnabled;         // cached GL_CULL_FACE state  — avoids redundant D3D11 rasterizer state churn on ANGLE
    bool m_blendEnabled;            // cached GL_BLEND state      — avoids redundant D3D11 rasterizer state churn on ANGLE
    float m_quadRed, m_quadGreen, m_quadBlue, m_quadAlpha;
    
    // OGLES Context variables
    EGLDisplay m_display;
    EGLContext m_context;
    EGLSurface m_surface;

    // Shader variables (remaining private)
    GLuint     m_texture;
    GLint      m_uTexAlpha;
    GLint      m_useTexture;
    GLint      m_useTexAlpha;

    void   oglCreateShaders();
    void   oglCleanup();

    // 3D shader program and cached uniform / attribute locations
    GLuint m_3dShaderProgram;
    GLint  m_3dMVPUniform,       m_3dModelUniform,      m_3dLightDirUniform;
    GLint  m_3dLightColorUniform, m_3dAmbientUniform,   m_3dCameraEyeUniform, m_3dAlphaUniform;
    GLint  m_3dPosAttrib,         m_3dNormalAttrib,      m_3dTexCoordAttrib;

    // Skinned-mesh shader program and cached uniform / attribute locations
    GLuint m_3dSkinnedShaderProgram;
    GLint  m_3dSk_MVPUniform,    m_3dSk_ModelUniform,   m_3dSk_LightDirUniform;
    GLint  m_3dSk_LightColUniform, m_3dSk_AmbientUniform, m_3dSk_CameraEyeUniform, m_3dSk_AlphaUniform;
    GLint  m_3dSk_BonesUniform;
    GLint  m_3dSk_PosAttrib,     m_3dSk_NormalAttrib,   m_3dSk_TexCoordAttrib;
    GLint  m_3dSk_JointsAttrib,  m_3dSk_WeightsAttrib;

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

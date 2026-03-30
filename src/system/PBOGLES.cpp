
// PBOGLES - OpenGL ES 3.1 rendering backend for Windows / Rasberry Pi
//           
// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "PBOGLES.h"

PBOGLES::PBOGLES() {

    // Key PBOGLES variables
    m_height = 0;
    m_width = 0;
    m_aspectRatio = 0.0f;
    m_lastTextureId = 0;
    m_started = false;
    m_scissorEnabled    = false;
    m_depthTestEnabled  = false;
    m_cullFaceEnabled   = false;
    m_blendEnabled      = false;
#ifdef SIMULATOR_SMALL_WINDOW
    m_surfaceWidth  = 0;
    m_surfaceHeight = 0;
#endif

    // OGL ES variables
    m_display = EGL_NO_DISPLAY;
    m_context = EGL_NO_CONTEXT;
    m_surface = EGL_NO_SURFACE;
    m_shaderProgram = 0;
    m_texture = 0;
    m_posAttrib = 0;
    m_colorAttrib = 0; 
    m_texCoordAttrib = 0; 
    m_uTexAlpha = 0;
    m_useTexture = 0;
    m_useTexAlpha = 0;

    // 3D shader state
    m_3dShaderProgram    = 0;
    m_3dMVPUniform       = -1;
    m_3dModelUniform     = -1;
    m_3dLightDirUniform  = -1;
    m_3dLightColorUniform= -1;
    m_3dAmbientUniform   = -1;
    m_3dCameraEyeUniform = -1;
    m_3dAlphaUniform     = -1;
    m_3dPosAttrib        = -1;
    m_3dNormalAttrib     = -1;
    m_3dTexCoordAttrib   = -1;

    // Skinned 3D shader state
    m_3dSkinnedShaderProgram = 0;
    m_3dSk_MVPUniform      = -1;
    m_3dSk_ModelUniform    = -1;
    m_3dSk_LightDirUniform = -1;
    m_3dSk_LightColUniform = -1;
    m_3dSk_AmbientUniform  = -1;
    m_3dSk_CameraEyeUniform= -1;
    m_3dSk_AlphaUniform    = -1;
    m_3dSk_BonesUniform    = -1;
    m_3dSk_PosAttrib       = -1;
    m_3dSk_NormalAttrib    = -1;
    m_3dSk_TexCoordAttrib  = -1;
    m_3dSk_JointsAttrib    = -1;
    m_3dSk_WeightsAttrib   = -1;
}

PBOGLES::~PBOGLES() {

    // Clean up the EGL context
    oglCleanup();
}

// Clean up the EGL / OGL context
void PBOGLES::oglCleanup() {
    if (m_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (m_surface != EGL_NO_SURFACE) {
            eglDestroySurface(m_display, m_surface);
            m_surface = EGL_NO_SURFACE;
        }
        if (m_context != EGL_NO_CONTEXT) {
            eglDestroyContext(m_display, m_context);
            m_context = EGL_NO_CONTEXT;
        }
        eglTerminate(m_display);
        m_display = EGL_NO_DISPLAY;
    }
}

bool PBOGLES::oglInit(long width, long height, NativeWindowType nativeWindow) {

    // Start the EGL init process
    // These error messages might not show up if we are running on the RasPi
    m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_display == EGL_NO_DISPLAY) {
        std::cout << "Error: eglGetDisplay() failed\n";
        return (false);
    }

    if (!eglInitialize(m_display, nullptr, nullptr)) {
        std::cout << "Error: eglInitialize() failed\n";
        return (false);
    }

    // This might need to change for the RasPi - it probably won't use a window or it will be full screen
    // Current settings - windowed mode, 32 bit color, 8 bit alpha, 8 bit red, 8 bit green, 8 bit blue
    EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_ALPHA_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_NONE
    };

    // Choose the EGL config, currently set up to use the first (and only) one that is returned
    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig(m_display, configAttribs, &config, 1, &numConfigs) || numConfigs < 1) {
        std::cout << "Error: eglChooseConfig() failed\n";
        return (false);
    }

    // Create the context
    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    m_context = eglCreateContext(m_display, config, EGL_NO_CONTEXT, contextAttribs);
    if (m_context == EGL_NO_CONTEXT) {
        std::cout << "Error: eglCreateContext() failed\n";
        return false;
    }

    // Create the surface, attempting to get one with a back buffer and attache it to the native window
    // This might need to change for the RasPi - it probably won't use a window or it will be full screen
    EGLint surfaceAttribs[] = {
        EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
        EGL_NONE
    };
    m_surface = eglCreateWindowSurface(m_display, config, nativeWindow, surfaceAttribs);
    if (m_surface == EGL_NO_SURFACE) {
        std::cout << "Error: eglCreateWindowSurface() failed\n";
        return false;
    }

    // Make the context current
    if (!eglMakeCurrent(m_display, m_surface, m_surface, m_context)) {
        std::cout << "Error: eglMakeCurrent() failed\n";
        return false;
    }

    // Compile the quad shader for the sprite system
    m_shaderProgram = oglCreateProgram(vertexShaderSource, fragmentShaderSource);
    glUseProgram(m_shaderProgram);

    // Set up the shader attributes and input variables
    m_posAttrib = glGetAttribLocation(m_shaderProgram, "vPosition");
    glEnableVertexAttribArray(m_posAttrib);
    m_colorAttrib = glGetAttribLocation(m_shaderProgram, "vColor");
    glEnableVertexAttribArray(m_colorAttrib);
    m_texCoordAttrib = glGetAttribLocation(m_shaderProgram, "vTexCoord");
    glEnableVertexAttribArray(m_texCoordAttrib);
    m_uTexAlpha = glGetUniformLocation(m_shaderProgram, "uTexAlpha");
    m_useTexture = glGetUniformLocation(m_shaderProgram, "useTexture");
    m_useTexAlpha = glGetUniformLocation(m_shaderProgram, "useTexAlpha");

    glEnable(GL_BLEND);
    m_blendEnabled = true;
    // glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glBlendFunc(GL_ONE, GL_ONE);

    // Depth writes are disabled for 2D rendering — the depth buffer is only used during
    // 3D passes. ogl3dBeginPass() re-enables writes before 3D draws.
    glDepthMask(GL_FALSE);
    
    // Set the internal variables and reflect that the basic rendering engine has been intialized
    m_width = width;
    m_height = height;
    m_aspectRatio = (float)height / (float)width;

#ifdef SIMULATOR_SMALL_WINDOW
    // Set the physical surface dimensions used by glViewport and scissor scaling.
    // The simulator window is half width/height so the full NDC scene maps correctly.
    m_surfaceWidth  = width  / 2;
    m_surfaceHeight = height / 2;
#endif

    m_started = true;
    return true;
}

// Clear the back buffere with option to flip
bool PBOGLES::oglClear(float red, float blue, float green, float alpha, bool doFlip) {

#ifdef SIMULATOR_SMALL_WINDOW
        glViewport(0, 0, m_surfaceWidth, m_surfaceHeight);
#else
        glViewport(0, 0, m_width, m_height);
#endif
        glClearColor(red, green, blue, alpha);

        glClear(GL_COLOR_BUFFER_BIT);  // Depth buffer is cleared by ogl3dBeginPass before 3D draws

        if (doFlip) {
            if (oglSwap(false) == false) return (false);
        }

        return (true);
}

// Clear the back buffer with option to flip
bool PBOGLES::oglSwap(bool flush){

    // Add ability to flush pipeline before swap
    // if (flush) glFlush();
    // glFinish(); // This is an alternative to glFlush, but it waits for all commands to complete
    if (flush) glFinish();  
    
    if (eglSwapBuffers(m_display, m_surface) != EGL_TRUE) return (false);
    return (true);
}

// Set or disable scissor test using OpenGL ES 3.1
void PBOGLES::oglSetScissor(bool enable, int x1, int y1, int x2, int y2) {
    if (enable) {
        // Enable scissor test — only call glEnable if not already enabled to avoid
        // redundant D3D11 rasterizer state creation (ANGLE warning #55 SETPRIVATEDATA_CHANGINGPARAMS)
        if (!m_scissorEnabled) {
            glEnable(GL_SCISSOR_TEST);
            m_scissorEnabled = true;
        }
        
        // Convert screen space coordinates to OpenGL scissor coordinates
        // OpenGL scissor uses bottom-left origin, so we need to convert Y coordinate
        // x1, y1 = upper left corner in screen space
        // x2, y2 = lower right corner in screen space
#ifdef SIMULATOR_SMALL_WINDOW
        // Scale scissor coordinates from virtual resolution to the smaller physical
        // surface so the clipping region matches the down-scaled simulator window.
        float scaleX = (float)m_surfaceWidth  / (float)m_width;
        float scaleY = (float)m_surfaceHeight / (float)m_height;
        int x      = (int)(x1       * scaleX);
        int y      = (int)(m_surfaceHeight - y2 * scaleY);  // flip to bottom-left origin
        int width  = (int)((x2 - x1) * scaleX);
        int height = (int)((y2 - y1) * scaleY);
#else
        int x = x1;
        int y = m_height - y2;  // Convert from top-left to bottom-left origin
        int width = x2 - x1;
        int height = y2 - y1;
#endif

        // Set the scissor rectangle
        glScissor(x, y, width, height);
    } else {
        // Disable scissor test — only call glDisable if currently enabled
        if (m_scissorEnabled) {
            glDisable(GL_SCISSOR_TEST);
            m_scissorEnabled = false;
        }
    }
}

// Function to compile shader
GLuint PBOGLES::oglCompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    // Check for compile errors
    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        if (logLen > 1) {
            std::string log(logLen, '\0');
            glGetShaderInfoLog(shader, logLen, nullptr, &log[0]);
            std::cout << "Shader compile error (" << (type == GL_VERTEX_SHADER ? "VERT" : "FRAG") << "):\n" << log << std::endl;
        }
    }
    return shader;
}

// Function to create shader program
GLuint PBOGLES::oglCreateProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = oglCompileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = oglCompileShader(GL_FRAGMENT_SHADER, fragmentSource);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    // Check for link errors
    GLint linkStatus = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE) {
        GLint logLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
        if (logLen > 1) {
            std::string log(logLen, '\0');
            glGetProgramInfoLog(program, logLen, nullptr, &log[0]);
            std::cout << "Shader link error:\n" << log << std::endl;
        }
        glDeleteProgram(program);
        return 0;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

//void   oglRenderQuad (float* X1, float* Y1, float* X2, float* Y2, bool useCenter, bool useTexture, bool useTexAlpha, float texAlpha, unsigned int textureId, 
//    float vertRed, float vertGreen, float vertBlue, float vertAlpha, float scale, float rotateDegrees, bool returnBoundingBox);

// Renderig a quad to the back buffer - this is a lot of parameters....
void PBOGLES::oglRenderQuad (float* X1, float* Y1, float* X2, float* Y2, float U1, float V1, float U2, float V2, 
                             bool useCenter, bool useTexAlpha, float texAlpha, unsigned int textureId,
                             float vertRed, float vertGreen, float vertBlue, float vertAlpha, 
                             float scale, float rotateDegrees, bool returnBoundingBox) {

    // Create the vertices for the quad 
    GLfloat vertices[] = {
    // Pos (3 XYZ)      // Colors (4 RGBA)      // Texture Coords
    *X1,  *Y1, 0.0f,      vertRed, vertGreen, vertBlue, vertAlpha, U1, V2, // Top Left
    *X1,  *Y2, 0.0f,      vertRed, vertGreen, vertBlue, vertAlpha, U1, V1, // Bottom-left
    *X2,  *Y1, 0.0f,      vertRed, vertGreen, vertBlue, vertAlpha, U2, V2, // Top right
    *X2,  *Y2, 0.0f,      vertRed, vertGreen, vertBlue, vertAlpha, U2, V1  // Bottom-right
    };

    //  Transfor the quad if rotateDegrees are not the default (no scale / rotate) values
    if ((scale != 1.0f) || (rotateDegrees != 0.0f)) {
        if (useCenter) {
            // Calculate the center of the quad
            float centerX = (*X1 + *X2) / 2.0f;
            float centerY = (*Y1 + *Y2) / 2.0f;

            // Scale and rotate the quad around the center, using aspect ratios to keep the quad at right angles
            for (int i = 0; i < 4; i++) {
                // Translate to origin
                float x = vertices[i * 9] - centerX;
                float y = (vertices[i * 9 + 1] - centerY) * m_aspectRatio;

                scaleAndRotateVertices(&x, &y, scale, rotateDegrees);

                // Translate back to render location
                vertices[i * 9] = x + centerX;
                vertices[i * 9 + 1] = (y / m_aspectRatio) + centerY;
            }
        } else {
           // Scale and rotate the quad around the upper left corner, using aspect ratios to keep the quad at right angles
            for (int i = 0; i < 4; i++) {
                // Translate to origin
                float x = vertices[i * 9] - vertices[0];
                float y = (vertices[i * 9 + 1] - vertices[1]) * m_aspectRatio;
                
                scaleAndRotateVertices(&x, &y, scale, rotateDegrees);

                // Translate back to render location
                vertices[i * 9] = vertices[0] + x;
                vertices[i * 9 + 1] = vertices[1] + (y / m_aspectRatio);
                
            }
        }
    }

    // if requested, search for the bounding box in the vertices, return then in X1,Y1,X2,Y2
    // The bounding box is defined maximum X,Y values and minimum X,Y values of the quad.
    // This could be more effecient for non-rotated quads, but for now we will just search through the vertices
    if (returnBoundingBox) {
        float fminX = vertices[0];
        float fminY = vertices[1];
        float fmaxX = vertices[0];
        float fmaxY = vertices[1];
        for (int i = 1; i < 4; i++) {
            if (vertices[i * 9] < fminX) fminX = vertices[i * 9];
            if (vertices[i * 9] > fmaxX) fmaxX = vertices[i * 9];
            if (vertices[i * 9 + 1] < fminY) fminY = vertices[i * 9 + 1];
            if (vertices[i * 9 + 1] > fmaxY) fmaxY = vertices[i * 9 + 1];
        }
        *X2 = fminX;
        *Y2 = fminY;
        *X1 = fmaxX;
        *Y1 = fmaxY;
    }

    // Define the indices for the quad
    GLushort indices[] = {
        1, 0, 2,
        1, 2, 3
    };

    // Specify how the data for each attribute is retrieved from the array
    glVertexAttribPointer(m_posAttrib, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), vertices);
    glVertexAttribPointer(m_colorAttrib, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), vertices + 3);
    glVertexAttribPointer(m_texCoordAttrib, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), vertices + 7);

    // Set the input variable for the alpha and enable/bind the texture if needed
    glUniform1f(m_uTexAlpha, texAlpha);
    if (useTexAlpha) glUniform1f(m_useTexAlpha, 1);
    else glUniform1f(m_useTexAlpha, 0);

    if (textureId == 0) {
        glUniform1i(m_useTexture, 0);
        m_lastTextureId = textureId;
    }
    else {
        if (textureId != m_lastTextureId) {
            glUniform1i(m_useTexture, 1);
            glBindTexture(GL_TEXTURE_2D, textureId);
            m_lastTextureId = textureId;
        }
    }
      
     // Finally, draw the quad!
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

void PBOGLES::scaleAndRotateVertices(float* x, float* y, float scale, float rotateDegrees){
    
    // Scale
    if (scale != 1.0f) {
        *x *= scale;
        *y *= scale;
    }

    // Rotate
    if (rotateDegrees != 0.0f) {

        float tempX = *x;
        float tempY = *y;
        float angle = rotateDegrees * 3.14159f / 180.0f;

        *x = tempX * std::cos(angle) - tempY * std::sin(angle);
        *y = tempX * std::sin(angle) + tempY * std::cos(angle);
    }
}

// Delete a texture from a sprite.  You can keep the sprite, but release the texture
bool   PBOGLES::oglUnloadTexture(GLuint textureId){
    glDeleteTextures(1, &textureId);
    return (true);
}

// Function to load a BMP image and place it a texture
GLuint PBOGLES::oglLoadTexture(const char* filename, oglTexType type, unsigned int* width, unsigned int* height) {

    GLuint tempTexture = 0;

    // Picking the loading fucntion depending on the type of texture
    switch (type)
    {
        case OGL_BMP: tempTexture = oglLoadBMPTexture (filename, width, height); break;
        case OGL_PNG: tempTexture = oglLoadPNGTexture (filename, width, height); break;
        case OGL_VIDEO:
            // For video textures, filename contains "widthxheight" format (e.g., "1920x1080")
            // Parse the dimensions and create an empty texture
            {
                int vidWidth = 0, vidHeight = 0;
                if (sscanf(filename, "%dx%d", &vidWidth, &vidHeight) == 2) {
                    tempTexture = oglCreateVideoTexture(vidWidth, vidHeight);
                    *width = vidWidth;
                    *height = vidHeight;
                }
            }
            break;
        default: break;
    }

    // No sprite was created
    return (tempTexture);

}

GLuint PBOGLES::oglLoadBMPTexture (const char* filename, unsigned int* width, unsigned int* height){
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        // Fix this error output l
        std::cout << "Error: Unable to open file " << filename << std::endl;
        return false;
    }

     int tempWidth, tempHeight;

    // Go throught the bitmap header to the width and height
    file.seekg(18);

    file.read(reinterpret_cast<char*>(&tempWidth), 4);
    file.read(reinterpret_cast<char*>(&tempHeight), 4);

    // Calculate the size for data of the bitmap
    int size = 3 * (tempWidth) * (tempHeight);
    unsigned char* data = new unsigned char[size];
    unsigned char* rgbData = new unsigned char[size];
   
    // Go to the image data and put it in the data array
    file.seekg(54);
    file.read(reinterpret_cast<char*>(data), size);
    file.close();

    // Convert BGR to RGB for bitmaps
    for (int i = 0; i < size; i += 3) {
        rgbData[i] = data[i + 2];
        rgbData[i + 1] = data[i + 1];
        rgbData[i + 2] = data[i];
    }

    delete[] data;

    // Reverse the BMP data in place so that it is in the correct order so that top row is first in memory
    for (int i = 0; i < tempHeight / 2; i++) {
        for (int j = 0; j < tempWidth * 3; j++) {
            unsigned char temp = rgbData[i * tempWidth * 3 + j];
            rgbData[i * tempWidth * 3 + j] = rgbData[(tempHeight - i - 1) * tempWidth * 3 + j];
            rgbData[(tempHeight - i - 1) * tempWidth * 3 + j] = temp;
        }
    }

    // Now create and bind the texture to a texture ID
    GLuint texture;

    glGenTextures(1, &texture);
    if (texture == 0) {
        delete[] rgbData;
        return (0);
    }

    // Create the texture.  NOTE:  We don't force power of two textures, nor does the system have ability
    // to assign UV texture coodiates within a texture (sprites always use full texture).  If non-power of two 
    // textures start to be a problem, thenwe will need to add code to handle this and pick sub rectagles within 
    // the texture.

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tempWidth, tempHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, rgbData);
   
    delete[] rgbData; 
    
    *width = tempWidth;
    *height = tempHeight;
    return (texture);
}

unsigned int PBOGLES::oglGetScreenHeight(){
    return m_height;
}
unsigned int PBOGLES::oglGetScreenWidth(){
    return m_width;
}

// Function to load a PNG texture
GLuint PBOGLES::oglLoadPNGTexture (const char* filename, unsigned int* width, unsigned int* height){ 

    int texWidth, texHeight, texChannels;
    unsigned char* data = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!data) {
        std::cout << "Error: Unable to load image " << filename << std::endl;
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    *width = texWidth;
    *height = texHeight;
    return texture;

    return (0);
}

// Create an empty texture for video playback (will be updated dynamically)
GLuint PBOGLES::oglCreateVideoTexture(unsigned int width, unsigned int height) {
    GLuint texture;
    glGenTextures(1, &texture);
    if (texture == 0) return 0;
    
    glBindTexture(GL_TEXTURE_2D, texture);
    
    // Create an empty RGBA texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    
    // Set texture parameters for video (no mipmaps, linear filtering)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    return texture;
}

// Restore full 2D rendering state after a 3D pass.
// Called by PB3D::pb3dEnd() so that the 3D layer never needs to know
// about the internal 2D shader program, attrib layout or GL state.
void PBOGLES::oglRestore2DState() {
    // Disable 3D-specific state
    if (m_depthTestEnabled) { glDisable(GL_DEPTH_TEST); m_depthTestEnabled = false; }
    if (m_cullFaceEnabled)  { glDisable(GL_CULL_FACE);  m_cullFaceEnabled  = false; }
    // Disable depth writes — 2D sprites must not write to the depth buffer.
    // Per the OpenGL ES spec, depth writes occur even when GL_DEPTH_TEST is disabled
    // if glDepthMask is GL_TRUE, causing major unnecessary memory bandwidth.
    glDepthMask(GL_FALSE);

    // Re-enable standard alpha blending for 2D sprites
    if (!m_blendEnabled) { glEnable(GL_BLEND); m_blendEnabled = true; }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Unbind VAO so 2D CPU-pointer vertex calls work correctly
    glBindVertexArray(0);

    // Restore 2D sprite shader program
    glUseProgram(m_shaderProgram);

    // Unbind VBOs: GL_ARRAY_BUFFER is global state — if left bound,
    // oglRenderQuad's CPU vertex pointers are misread as VBO offsets.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Re-enable 2D vertex attrib arrays (may have been disrupted by VAO binding)
    glEnableVertexAttribArray(m_posAttrib);
    glEnableVertexAttribArray(m_colorAttrib);
    glEnableVertexAttribArray(m_texCoordAttrib);

    // Reset 2D texture cache: 3D rendering binds textures outside our tracking.
    oglResetTextureCache();
}

// Update an existing texture with new video frame data
bool PBOGLES::oglUpdateTexture(GLuint textureId, const uint8_t* data, unsigned int width, unsigned int height) {
    if (textureId == 0 || data == nullptr) {
        return false;
    }
    
    glBindTexture(GL_TEXTURE_2D, textureId);
    
    // Update the texture with new RGBA data
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    return true;
}

// ============================================================================
// 3D rendering backend — all OpenGL ES calls for the 3D pass live here.
// PB3D delegates every GL operation to these methods; PB3D itself makes no
// direct gl*() calls so that the OGL layer remains self-contained in PBOGLES.
// ============================================================================

// Compile the 3D shader and cache all uniform / attribute locations.
// Returns true if the shader compiled and the position attribute was found.
bool PBOGLES::ogl3dInitShader(const char* vertSrc, const char* fragSrc) {
    m_3dShaderProgram = oglCreateProgram(vertSrc, fragSrc);
    if (m_3dShaderProgram == 0) return false;

    m_3dMVPUniform        = glGetUniformLocation(m_3dShaderProgram, "uMVP");
    m_3dModelUniform      = glGetUniformLocation(m_3dShaderProgram, "uModel");
    m_3dLightDirUniform   = glGetUniformLocation(m_3dShaderProgram, "uLightDir");
    m_3dLightColorUniform = glGetUniformLocation(m_3dShaderProgram, "uLightColor");
    m_3dAmbientUniform    = glGetUniformLocation(m_3dShaderProgram, "uAmbientColor");
    m_3dCameraEyeUniform  = glGetUniformLocation(m_3dShaderProgram, "uCameraEye");
    m_3dAlphaUniform      = glGetUniformLocation(m_3dShaderProgram, "uAlpha");

    m_3dPosAttrib      = glGetAttribLocation(m_3dShaderProgram, "aPosition");
    m_3dNormalAttrib   = glGetAttribLocation(m_3dShaderProgram, "aNormal");
    m_3dTexCoordAttrib = glGetAttribLocation(m_3dShaderProgram, "aTexCoord");

    return (m_3dPosAttrib >= 0);
}

// Delete the 3D shader program and reset cached locations.
void PBOGLES::ogl3dDestroyShader() {
    if (m_3dShaderProgram) {
        glDeleteProgram(m_3dShaderProgram);
        m_3dShaderProgram    = 0;
        m_3dMVPUniform       = -1;
        m_3dModelUniform     = -1;
        m_3dLightDirUniform  = -1;
        m_3dLightColorUniform= -1;
        m_3dAmbientUniform   = -1;
        m_3dCameraEyeUniform = -1;
        m_3dAlphaUniform     = -1;
        m_3dPosAttrib        = -1;
        m_3dNormalAttrib     = -1;
        m_3dTexCoordAttrib   = -1;
    }
}

// Upload interleaved vertex + index data to the GPU and return opaque handles.
// Vertex layout (stride = 8 floats): [posX posY posZ normX normY normZ u v]
// The caller owns the CPU buffers and may free them after this call returns.
bool PBOGLES::ogl3dCreateMesh(const float* vertData, size_t vertFloatCount,
                               const unsigned int* idxData, size_t idxCount,
                               unsigned int& outVao, unsigned int& outVbo, unsigned int& outEbo) {
    GLuint vao = 0, vbo = 0, ebo = 0;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(vertFloatCount * sizeof(float)), vertData, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(idxCount * sizeof(unsigned int)), idxData, GL_STATIC_DRAW);

    GLsizei stride = 8 * sizeof(float);
    if (m_3dPosAttrib >= 0) {
        glEnableVertexAttribArray(m_3dPosAttrib);
        glVertexAttribPointer(m_3dPosAttrib, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    }
    if (m_3dNormalAttrib >= 0) {
        glEnableVertexAttribArray(m_3dNormalAttrib);
        glVertexAttribPointer(m_3dNormalAttrib, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    }
    if (m_3dTexCoordAttrib >= 0) {
        glEnableVertexAttribArray(m_3dTexCoordAttrib);
        glVertexAttribPointer(m_3dTexCoordAttrib, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    }

    glBindVertexArray(0);
    // Unbind VBOs: GL_ARRAY_BUFFER is global state — if left bound,
    // oglRenderQuad's CPU vertex pointers are misread as VBO offsets.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    outVao = (unsigned int)vao;
    outVbo = (unsigned int)vbo;
    outEbo = (unsigned int)ebo;
    return true;
}

// Release GPU resources for a mesh.
void PBOGLES::ogl3dDestroyMesh(unsigned int vao, unsigned int vbo, unsigned int ebo) {
    GLuint glVao = (GLuint)vao;
    GLuint glVbo = (GLuint)vbo;
    GLuint glEbo = (GLuint)ebo;
    if (glVao) glDeleteVertexArrays(1, &glVao);
    if (glVbo) glDeleteBuffers(1, &glVbo);
    if (glEbo) glDeleteBuffers(1, &glEbo);
}

// Upload an RGBA pixel buffer as a GL texture and return its opaque handle.
unsigned int PBOGLES::ogl3dCreateTexture(const unsigned char* pixels, int width, int height) {
    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return (unsigned int)texId;
}

// Create a 1×1 opaque-white fallback texture used when a model has no material.
unsigned int PBOGLES::ogl3dCreateFallbackTexture() {
    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    const unsigned char white[] = {255, 255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    return (unsigned int)texId;
}

// Delete a 3D texture by its opaque handle.
void PBOGLES::ogl3dDestroyTexture(unsigned int texId) {
    GLuint glTex = (GLuint)texId;
    if (glTex) glDeleteTextures(1, &glTex);
}

// Begin the 3D rendering pass: enable depth testing and bind the 3D shader.
void PBOGLES::ogl3dBeginPass() {
    // Re-enable depth writes and clear before 3D draws.
    // glDepthMask is GL_FALSE during 2D — must set TRUE before glClear(DEPTH) is effective.
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    if (!m_depthTestEnabled) { glEnable(GL_DEPTH_TEST);  m_depthTestEnabled = true; }
    glDepthFunc(GL_LEQUAL);
    // Backface culling disabled — glTF winding-order varies by exporter.
    if (m_cullFaceEnabled)    { glDisable(GL_CULL_FACE); m_cullFaceEnabled  = false; }
    glUseProgram(m_3dShaderProgram);
}

// Upload scene-level uniforms: light direction/colour/ambient and camera eye.
// Call once per frame after ogl3dBeginPass(), before drawing any instances.
void PBOGLES::ogl3dSetSceneUniforms(float lightDirX, float lightDirY, float lightDirZ,
                                     float lightColR, float lightColG, float lightColB,
                                     float ambR,      float ambG,      float ambB,
                                     float eyeX,      float eyeY,      float eyeZ) {
    glUniform3f(m_3dLightDirUniform,   lightDirX, lightDirY, lightDirZ);
    glUniform3f(m_3dLightColorUniform, lightColR, lightColG, lightColB);
    glUniform3f(m_3dAmbientUniform,    ambR,      ambG,      ambB);
    glUniform3f(m_3dCameraEyeUniform,  eyeX,      eyeY,      eyeZ);
}

// Upload scene-level uniforms for the skinned shader pass.
void PBOGLES::ogl3dSetSkinnedSceneUniforms(float lightDirX, float lightDirY, float lightDirZ,
                                            float lightColR, float lightColG, float lightColB,
                                            float ambR,      float ambG,      float ambB,
                                            float eyeX,      float eyeY,      float eyeZ) {
    glUniform3f(m_3dSk_LightDirUniform,  lightDirX, lightDirY, lightDirZ);
    glUniform3f(m_3dSk_LightColUniform,  lightColR, lightColG, lightColB);
    glUniform3f(m_3dSk_AmbientUniform,   ambR,      ambG,      ambB);
    glUniform3f(m_3dSk_CameraEyeUniform, eyeX,      eyeY,      eyeZ);
}

// Upload per-instance uniforms: MVP matrix, model matrix, and alpha.
// Call once per instance before drawing its meshes.
void PBOGLES::ogl3dSetInstanceUniforms(const float mvp[16], const float modelMat[16], float alpha) {
    glUniformMatrix4fv(m_3dMVPUniform,   1, GL_FALSE, mvp);
    glUniformMatrix4fv(m_3dModelUniform, 1, GL_FALSE, modelMat);
    glUniform1f(m_3dAlphaUniform, alpha);
}

// Enable or disable alpha blending for transparent 3D instances.
void PBOGLES::ogl3dSetBlend(bool enable) {
    if (enable) {
        if (!m_blendEnabled) { glEnable(GL_BLEND); m_blendEnabled = true; }
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        if (m_blendEnabled) { glDisable(GL_BLEND); m_blendEnabled = false; }
    }
}

// Draw one mesh primitive using its VAO and texture handle.
// ogl3dSetInstanceUniforms() must be called before the first mesh of each instance.
void PBOGLES::ogl3dDrawMeshPrimitive(unsigned int vao, unsigned int textureId, unsigned int indexCount) {
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray((GLuint)vao);
    glBindTexture(GL_TEXTURE_2D, (GLuint)textureId);
    glDrawElements(GL_TRIANGLES, (GLsizei)indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// ============================================================================
// Skinned-mesh shader support
// ============================================================================

// Compile the skinned 3D shader (vertex shader must include uBones[] array).
bool PBOGLES::ogl3dInitSkinnedShader(const char* vertSrc, const char* fragSrc) {
    m_3dSkinnedShaderProgram = oglCreateProgram(vertSrc, fragSrc);
    if (m_3dSkinnedShaderProgram == 0) return false;

    m_3dSk_MVPUniform       = glGetUniformLocation(m_3dSkinnedShaderProgram, "uMVP");
    m_3dSk_ModelUniform     = glGetUniformLocation(m_3dSkinnedShaderProgram, "uModel");
    m_3dSk_LightDirUniform  = glGetUniformLocation(m_3dSkinnedShaderProgram, "uLightDir");
    m_3dSk_LightColUniform  = glGetUniformLocation(m_3dSkinnedShaderProgram, "uLightColor");
    m_3dSk_AmbientUniform   = glGetUniformLocation(m_3dSkinnedShaderProgram, "uAmbientColor");
    m_3dSk_CameraEyeUniform = glGetUniformLocation(m_3dSkinnedShaderProgram, "uCameraEye");
    m_3dSk_AlphaUniform     = glGetUniformLocation(m_3dSkinnedShaderProgram, "uAlpha");
    m_3dSk_BonesUniform     = glGetUniformLocation(m_3dSkinnedShaderProgram, "uBones");

    m_3dSk_PosAttrib        = glGetAttribLocation(m_3dSkinnedShaderProgram, "aPosition");
    m_3dSk_NormalAttrib     = glGetAttribLocation(m_3dSkinnedShaderProgram, "aNormal");
    m_3dSk_TexCoordAttrib   = glGetAttribLocation(m_3dSkinnedShaderProgram, "aTexCoord");
    m_3dSk_JointsAttrib     = glGetAttribLocation(m_3dSkinnedShaderProgram, "aJoints");
    m_3dSk_WeightsAttrib    = glGetAttribLocation(m_3dSkinnedShaderProgram, "aWeights");

    return (m_3dSk_PosAttrib >= 0);
}

// Delete the skinned 3D shader program and reset cached locations.
void PBOGLES::ogl3dDestroySkinnedShader() {
    if (m_3dSkinnedShaderProgram) {
        glDeleteProgram(m_3dSkinnedShaderProgram);
        m_3dSkinnedShaderProgram = 0;
        m_3dSk_MVPUniform       = -1;
        m_3dSk_ModelUniform     = -1;
        m_3dSk_LightDirUniform  = -1;
        m_3dSk_LightColUniform  = -1;
        m_3dSk_AmbientUniform   = -1;
        m_3dSk_CameraEyeUniform = -1;
        m_3dSk_AlphaUniform     = -1;
        m_3dSk_BonesUniform     = -1;
        m_3dSk_PosAttrib        = -1;
        m_3dSk_NormalAttrib     = -1;
        m_3dSk_TexCoordAttrib   = -1;
        m_3dSk_JointsAttrib     = -1;
        m_3dSk_WeightsAttrib    = -1;
    }
}

// Upload skinned vertex data to the GPU.
// Vertex layout (stride = 16 floats):
//   [posX posY posZ  normX normY normZ  u v  j0 j1 j2 j3  w0 w1 w2 w3]
// The joints (j0..j3) are stored as floats; the shader casts them to int.
bool PBOGLES::ogl3dCreateSkinnedMesh(const float* vertData, size_t vertFloatCount,
                                      const unsigned int* idxData, size_t idxCount,
                                      unsigned int& outVao, unsigned int& outVbo, unsigned int& outEbo) {
    GLuint vao = 0, vbo = 0, ebo = 0;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(vertFloatCount * sizeof(float)), vertData, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(idxCount * sizeof(unsigned int)), idxData, GL_STATIC_DRAW);

    GLsizei stride = 16 * sizeof(float);
    if (m_3dSk_PosAttrib >= 0) {
        glEnableVertexAttribArray(m_3dSk_PosAttrib);
        glVertexAttribPointer(m_3dSk_PosAttrib, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    }
    if (m_3dSk_NormalAttrib >= 0) {
        glEnableVertexAttribArray(m_3dSk_NormalAttrib);
        glVertexAttribPointer(m_3dSk_NormalAttrib, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    }
    if (m_3dSk_TexCoordAttrib >= 0) {
        glEnableVertexAttribArray(m_3dSk_TexCoordAttrib);
        glVertexAttribPointer(m_3dSk_TexCoordAttrib, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    }
    if (m_3dSk_JointsAttrib >= 0) {
        glEnableVertexAttribArray(m_3dSk_JointsAttrib);
        glVertexAttribPointer(m_3dSk_JointsAttrib, 4, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));
    }
    if (m_3dSk_WeightsAttrib >= 0) {
        glEnableVertexAttribArray(m_3dSk_WeightsAttrib);
        glVertexAttribPointer(m_3dSk_WeightsAttrib, 4, GL_FLOAT, GL_FALSE, stride, (void*)(12 * sizeof(float)));
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    outVao = (unsigned int)vao;
    outVbo = (unsigned int)vbo;
    outEbo = (unsigned int)ebo;
    return true;
}

// Begin the skinned 3D rendering pass: enable depth testing, bind skinned shader.
void PBOGLES::ogl3dBeginSkinnedPass() {
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    if (!m_depthTestEnabled) { glEnable(GL_DEPTH_TEST);  m_depthTestEnabled = true; }
    glDepthFunc(GL_LEQUAL);
    if (m_cullFaceEnabled)   { glDisable(GL_CULL_FACE);  m_cullFaceEnabled  = false; }
    glUseProgram(m_3dSkinnedShaderProgram);
}

// Switch between static and skinned shader programs mid-frame without
// re-clearing the depth buffer.  Scene uniforms must already be uploaded.
void PBOGLES::ogl3dActivateStaticShader() {
    glUseProgram(m_3dShaderProgram);
}
void PBOGLES::ogl3dActivateSkinnedShader() {
    if (m_3dSkinnedShaderProgram) {
        glUseProgram(m_3dSkinnedShaderProgram);
    }
}

// Upload per-instance uniforms for the skinned pass (MVP, model, alpha, bone matrices).
void PBOGLES::ogl3dSetSkinnedInstanceUniforms(const float mvp[16], const float modelMat[16],
                                               float alpha,
                                               const float* boneMatrices, int numBones) {
    glUniformMatrix4fv(m_3dSk_MVPUniform,   1, GL_FALSE, mvp);
    glUniformMatrix4fv(m_3dSk_ModelUniform, 1, GL_FALSE, modelMat);
    glUniform1f(m_3dSk_AlphaUniform, alpha);
    if (m_3dSk_BonesUniform >= 0 && boneMatrices && numBones > 0) {
        glUniformMatrix4fv(m_3dSk_BonesUniform, numBones, GL_FALSE, boneMatrices);
    }
}
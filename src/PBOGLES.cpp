
// PBOGLES - OpenGL ES 3.1 rendering backend for Windows / Rasberry Pi
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#include "PBOGLES.h"

PBOGLES::PBOGLES() {

    // Key PBOGLES variables
    m_height = 0;
    m_width = 0;
    m_aspectRatio = 0.0f;
    m_lastTextureId = 0;
    m_started = false;

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
    // glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glBlendFunc(GL_ONE, GL_ONE);
    
    // Set the internal variables and reflect that the basic rendering engine has been intialized
    m_width = width;
    m_height = height;
    m_aspectRatio = (float)height / (float)width;
    m_started = true;
    return true;
}

// Clear the back buffere with option to flip
bool PBOGLES::oglClear(float red, float blue, float green, float alpha, bool doFlip) {

        glViewport(0, 0, m_width, m_height);
        glClearColor(red, green, blue, alpha);

        glClear(GL_COLOR_BUFFER_BIT);

        if (doFlip) {
            if (oglSwap() == false) return (false);
        }

        return (true);
}

// Clear the back buffer with option to flip
bool PBOGLES::oglSwap(){

    if (eglSwapBuffers(m_display, m_surface) != EGL_TRUE) return (false);
    return (true);
}

// Function to compile shader
GLuint PBOGLES::oglCompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
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
    return program;
}

//void   oglRenderQuad (float* X1, float* Y1, float* X2, float* Y2, bool useCenter, bool useTexture, bool useTexAlpha, float texAlpha, unsigned int textureId, 
//    float vertRed, float vertGreen, float vertBlue, float vertAlpha, float scale, float rotateDegrees, bool returnBoundingBox);

// Renderig a quad to the back buffer 
void PBOGLES::oglRenderQuad (float* X1, float* Y1, float* X2, float* Y2, bool useCenter, bool useTexAlpha, float texAlpha, unsigned int textureId,
                             float vertRed, float vertGreen, float vertBlue, float vertAlpha, float scale, float rotateDegrees, bool returnBoundingBox) {

    // Create the vertices for the quad
    GLfloat vertices[] = {
    // Pos (3 XYZ)      // Colors (4 RGBA)      // Texture Coords
    *X1,  *Y1, 0.0f,      vertRed, vertGreen, vertBlue, vertAlpha, 0.0f, 1.0f, // Top Left
    *X1,  *Y2, 0.0f,      vertRed, vertGreen, vertBlue, vertAlpha, 0.0f, 0.0f, // Bottom-left
    *X2,  *Y1, 0.0f,      vertRed, vertGreen, vertBlue, vertAlpha, 1.0f, 1.0f, // Top right
    *X2,  *Y2, 0.0f,      vertRed, vertGreen, vertBlue, vertAlpha, 1.0f, 0.0f  // Bottom-right
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

    if (textureId == 0) glUniform1i(m_useTexture, 0);
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

        *x = tempX * cos(angle) - tempY * sin(angle);
        *y = tempX * sin(angle) + tempY * cos(angle);
    }
}

// Function to load a BMP image and place it a texture
GLuint PBOGLES::oglLoadTexture(const char* filename, oglTexType type, unsigned int* width, unsigned int* height) {

    // Picking the loading fucntion depending on the type of texture
    switch (type)
    {
        case OGL_BMP:
            return (oglLoadBMPTexture (filename, width, height));
        break;

        case OGL_PNG:
            return (oglLoadPNGTexture (filename, width, height));
        break;
    
    default:
        break;
    }

    // No sprite was created
    return (0);

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

    // Now create and bind the texture to a texture ID
    GLuint texture;

    glGenTextures(1, &texture);
    if (texture == 0) return (0);

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
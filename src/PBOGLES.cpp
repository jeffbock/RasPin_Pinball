#include "PBOGLES.h"

PBOGLES::PBOGLES() {

    // Key PBOGLES variables
    m_height = 0;
    m_width = 0;
    m_started = false;

    // OGL ES variables
    m_display = EGL_NO_DISPLAY;
    m_context = EGL_NO_CONTEXT;
    m_surface = EGL_NO_SURFACE;
    m_shaderProgram = 0;
    m_texture = 0;
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

    // Load the test texture - this will get moved...
    oglLoadBMPTexture("resources/textures/godzilla.bmp");

    // St the internal variables and reflect that the basic rendering engine has been intialized
    m_width = width;
    m_height = height;
    m_started = true;
    return true;
}

// Clear the back buffere with option to flip
bool PBOGLES::oglClear(long color, bool doFlip) {

        glViewport(0, 0, m_width, m_height);
        if (color == OGLES_BLACKCOLOR) glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            else if (color == OGLES_WHITECOLOR) glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
                else return (false);
            
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

// Renderig a quad to the back buffer 
void PBOGLES::oglRenderQuad (){

    // Define the quad vertices, colors, and texture coordinates
    GLfloat vertices[] = {
    // Pos (3 XYZ)      // Colors (4 RGB)       // Texture Coords
    -0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, // Top-left
    -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, // Bottom-left
     0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, // Bottom-right
     0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f  // Top-right
    };

    // Define the indices for the quad
    GLushort indices[] = {
        0, 1, 2,
        0, 2, 3
    };

     // Get the attribute locations, enable them
    GLint posAttrib = glGetAttribLocation(m_shaderProgram, "vPosition");
    glEnableVertexAttribArray(posAttrib);
    GLint colorAttrib = glGetAttribLocation(m_shaderProgram, "vColor");
    glEnableVertexAttribArray(colorAttrib);
    GLint texCoordAttrib = glGetAttribLocation(m_shaderProgram, "vTexCoord");
    glEnableVertexAttribArray(texCoordAttrib);

    // Specify how the data for each attribute is retrieved from the array
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), vertices);
    glVertexAttribPointer(colorAttrib, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), vertices + 3);
    glVertexAttribPointer(texCoordAttrib, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), vertices + 7);

    // Get the uniform location for the alpha value
    GLint alphaUniform = glGetUniformLocation(m_shaderProgram, "uAlpha");
    glUniform1f(alphaUniform, 1.0f);

     // Draw the quad
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

// Function to load a BMP image and place it a texture
bool PBOGLES::oglLoadBMPTexture(const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        // Fix this error output l
        std::cout << "Error: Unable to open file " << filename << std::endl;
        return false;
    }

     int width, height;

    // Go throught the bitmap header to the width and height
    file.seekg(18);

    file.read(reinterpret_cast<char*>(&width), 4);
    file.read(reinterpret_cast<char*>(&height), 4);

    // Calculate the size for data of the bitmap
    int size = 3 * (width) * (height);
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

    // TODO:  The texture data needs to be part of a different structure so that it can be managed

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgbData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return true;
}

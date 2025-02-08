
// PBOGLES - OpenGL ES 3.1 rendering backend for Windows / Rasberry Pi
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#include "PBOGLES.h"

PBOGLES::PBOGLES() {

    // Key PBOGLES variables
    m_height = 0;
    m_width = 0;
    m_pixelXYRatio = 0.0f;
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

    // St the internal variables and reflect that the basic rendering engine has been intialized
    m_width = width;
    m_height = height;
    m_pixelXYRatio = (float)height / (float)width;
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

// Renderig a quad to the back buffer 
void PBOGLES::oglRenderQuad (float* X1, float* Y1, float* X2, float* Y2, float scale, unsigned int rotateDegrees,
                             bool useCenter, bool returnBoundingBox, unsigned int textureId, float alpha) {

    // Need to use the X1,Y1 and X2,Y2 to create the quad.
    // The scale is used to scale the quad, the rotateDegrees is used to rotate the quad
    // Define the quad vertices, colors, and texture coordinates
    GLfloat vertices[] = {
    // Pos (3 XYZ)      // Colors (4 RGBA)      // Texture Coords
    *X1,  *Y1, 0.0f,      1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, // Top Left
    *X1,  *Y2, 0.0f,      1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, // Bottom-left
    *X2,  *Y1, 0.0f,      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // Top right
    *X2,  *Y2, 0.0f,      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f  // Bottom-right
    };

    // If the scale or rotateDegrees are not the default values, then scale and rotate the quad
    if ((scale != 1.0f) || (rotateDegrees != 0)) {
        float aspectRatio = m_pixelXYRatio;
        float angle = rotateDegrees * 3.14159f / 180.0f;

        if (useCenter) {
            // Calculate the center of the quad
            float centerX = (*X1 + *X2) / 2.0f;
            float centerY = (*Y1 + *Y2) / 2.0f;

            // Scale and rotate the quad around the center
            for (int i = 0; i < 4; i++) {
                // Translate to origin
                float x = vertices[i * 9] - centerX;
                float y = (vertices[i * 9 + 1] - centerY) * aspectRatio;

                // Scale
                if (scale != 1.0f) {
                    x *= scale;
                    y *= scale;
                }

                // Rotate
                float newX = x * cos(angle) - y * sin(angle);
                float newY = x * sin(angle) + y * cos(angle);
            
                // Translate back
                vertices[i * 9] = newX + centerX;
                vertices[i * 9 + 1] = (newY / aspectRatio) + centerY;
            }
        } else {
            // Scale and rotate the quad from the upper left corner
            // Adjust the vertices for a upper left rendering
            for (int i = 0; i < 4; i++) {
                // Translate to origin
                float x = vertices[i * 9] - vertices[0];
                float y = (vertices[i * 9 + 1] - vertices[1]) * aspectRatio;
                
                // Scale
                x *= scale;
                y *= scale;

                // Rotate
                float newX = x * cos(angle) - y * sin(angle);
                float newY = x * sin(angle) + y * cos(angle);

                // Translate back
                vertices[i * 9] = vertices[0] + newX;
                vertices[i * 9 + 1] = vertices[1] + (newY / aspectRatio);
            }
        }
        // if requested, search for the bounding box in the vertices, return then in X1,Y1,X2,Y2
        // The bounding box is defined maximum X,Y values and minimum X,Y values of the quad.
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
    }

    // Define the indices for the quad
    GLushort indices[] = {
        1, 0, 2,
        1, 2, 3
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
    glUniform1f(alphaUniform, alpha);

    // Set the boolean of the shader function to use the texture
    GLint useTexture = glGetUniformLocation(m_shaderProgram, "useTexture");
    if (textureId == 0) glUniform1i(useTexture, 0);
        else {
            glUniform1i(useTexture, 1);
            glBindTexture(GL_TEXTURE_2D, textureId);
        }
    
     // Draw the quad
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

/*

 if (useCenter) {
            // Scale the quad
            for (int i = 0; i < 4; i++) {
                vertices[i * 9] *= scale;
                vertices[i * 9 + 1] *= scale;
            }

            // Rotate the quad
            float angle = rotateDegrees * 3.14159f / 180.0f;
            for (int i = 0; i < 4; i++) {
                float x = vertices[i * 9];
                float y = vertices[i * 9 + 1] * m_pixelXYRatio;

                // Rotate the quad around the center 
                // Factor in pixel scaling with the aspect ratio (m_pixelXYRatio)
                vertices[i * 9] = (x * cos(angle)) - (y * sin(angle));
                // Adjust the vertice for the aspect ratio
                vertices[i * 9 + 1] = (x * sin(angle) + y * cos(angle)) / m_pixelXYRatio;
                
            }
        }
        else {
            // Scale and rotate the quad from the upper left corner.  Upper left will remain stationary, and the remaining vertices
            // will be scaled and rotated around the upper left corner.
            for (int i = 1; i < 4; i++) {
                vertices[i * 9] = vertices[0] + (vertices[i * 9] - vertices[0]) * scale;
                vertices[i * 9 + 1] = vertices[1] + (vertices[i * 9 + 1] - vertices[1]) * scale;
            }
            // Rotate the quad aroud the upper left corner
            float angle = rotateDegrees * 3.14159f / 180.0f;
            for (int i = 1; i < 4; i++) {
                float x = vertices[i * 9] - vertices[0];
                float y = vertices[i * 9 + 1] - vertices[1] * m_pixelXYRatio;
                vertices[i * 9] = vertices[0] + x * cos(angle) - y * sin(angle);
                vertices[i * 9 + 1] = vertices[1] + (x * sin(angle) + y * cos(angle)) / m_pixelXYRatio;
            }
        }
*/

// Function to load a BMP image and place it a texture
GLuint PBOGLES::oglLoadTexture(const char* filename, oglTexType type, unsigned int* width, unsigned int* height) {
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

/*
// Function to load a PNG texture
bool PBOGLES::oglLoadPNGTexture(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        return false;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        std::cerr << "Error: Unable to create PNG read struct" << std::endl;
        fclose(file);
        return false;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        std::cerr << "Error: Unable to create PNG info struct" << std::endl;
        png_destroy_read_struct(&png, nullptr, nullptr);
        fclose(file);
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        std::cerr << "Error: PNG read error" << std::endl;
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(file);
        return false;
    }

    png_init_io(png, file);
    png_read_info(png, info);

    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    if (bit_depth == 16) {
        png_set_strip_16(png);
    }

    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    }

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(png);
    }

    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    }

    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY) {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }

    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }

    png_read_update_info(png, info);

    png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++) {
        row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png, info));
    }

    png_read_image(png, row_pointers);

    fclose(file);

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, row_pointers[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    for (int y = 0; y < height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);

    png_destroy_read_struct(&png, &info, nullptr);

    return true;
}
*/

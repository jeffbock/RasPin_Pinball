// Univerasl OGL ES Code for all platforms

#ifndef PBOGLES_h
#define PBOGLES_h

#include "include_ogl/egl.h"
#include "include_ogl/gl31.h"
#include <stdio.h>
#include <iostream>

#define OGLES_BLACKCOLOR 0x0 
#define OGLES_WHITECOLOR 0x1

// Define a class for the OGL ES code
class PBOGLES {

public:

    PBOGLES();
    ~PBOGLES();

    bool gfxInit (long width, long height, NativeWindowType nativeWindow) ;
    bool gfxClear (long color, bool doFlip);

private:
    long m_width;
    long m_height;
    bool m_started;
    
    // OGLES Context variables
    EGLDisplay m_display;
    EGLContext m_context;
    EGLSurface m_surface;

    void gfxRenderQuad ();
    void cleanup();
};

#endif // PBOGLES_h

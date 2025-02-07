// Gfx front-end libary based on OpenGL ES 3.1
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#ifndef PBGfx_h
#define PBGfx_h

#include "include_ogl/egl.h"
#include "include_ogl/gl31.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include "PBOGLES.h"

#define NOSPRITE 0

// Define an enum for different texture sources
enum gfxTexType {
    GFX_BMP = 0,
    GFX_TEXTURE = 1, 
    GFX_TTEND
};

enum gfxTexCenter {
    GFX_UPPERLEFT = 0,
    GFX_CENTER = 1, 
    GFX_TCEND
};

struct stBoundingBox {
    unsigned int x1;  // Upper Left
    unsigned int y1;  // Upper Left
    unsigned int x2;  // Lower Right
    unsigned int y2;  // Lower Right
};

// Define a struct for sprite information
struct stSpriteInfo {
    // Input information for sprites, the calling app will supply these values and can change some of them
    std::string spriteName;
    std::string textureFileName;
    gfxTexType textureType;
    gfxTexCenter textureCenter;
    float textureAlpha;
    // TODO:  Set sprite color value (white if no modifcation)
    bool keepResident;
    float scaleFactor;
    int rotateDegrees;
    bool updateBoundingBox;

    // Internal information for sprites, the system will supply these values and the app can query
    unsigned int baseWidth;
    unsigned int baseHeight;
    unsigned int glTextureId;
    stBoundingBox boundingBox;
};

// Define a class for the OGL ES code
class PBGfx : public PBOGLES {

public:
    PBGfx();
    ~PBGfx();

    unsigned int gfxCreateSprite(const std::string& spriteName, const std::string& textureFileName, 
                                 gfxTexType textureType, gfxTexCenter textureCenter, float textureAlpha, 
                                 bool keepResident, float scaleFactor, int rotateDegrees, bool updateBoundingBox);
    unsigned int gfxCreateSprite(stSpriteInfo spriteInfo);
    bool         gfxRenderSprite(unsigned int spriteId, unsigned int x, unsigned int y);
    void         gfxSwap();
    void         gfxClear(float red, float blue, float green, float alpha, bool doFlip);
    unsigned int gfxSetScaleFactor(unsigned int spriteId, float scaleFactor, bool addFactor);
    unsigned int gfxSetRotateDegrees(unsigned int spriteId, int rotateDegrees, bool addDegrees);
    unsigned int gfxGetBaseHeight(unsigned int spriteId);
    unsigned int gfxGetBaseWidth(unsigned int spriteId);
    stBoundingBox gfxGetBoundingBox(unsigned int spriteId);

    /*  Functions to add
    
    UpdateBoundingBox
    Create a font sprite type
    Write text from font sprite at location x,y (from a string / string array?)
    Create system font(s)
    
    */

private:
    unsigned int gfxSysCreateSprite(stSpriteInfo spriteInfo, bool bSystem);

    // User sprites start at 100. System sprites will use lower numbers
    unsigned int nextSystemSpriteId = 1;
    unsigned int nextUserSpriteId = 100;
    std::map<unsigned int, stSpriteInfo> spriteMap;
};

#endif // PBGfx_h

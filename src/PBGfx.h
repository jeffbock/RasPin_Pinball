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
#include <cmath>
#include "json.hpp"
#include "PBOGLES.h"
 
#define NOSPRITE 0
#define SYSTEMFONTSPRITE "resources/fonts/Ubuntu-Regular_32_256.png"

using json = nlohmann::json;

// Define an enum for different texture file sources
enum gfxTexType {
    GFX_BMP = 0,
    GFX_PNG = 1, 
    GFX_TTEND
};

// Alignment of where the X/Y coordinates are for the sprite
enum gfxTexCenter {
    GFX_UPPERLEFT = 0,
    GFX_CENTER = 1, 
    GFX_TCEND
};

// Enum to allow sprites to have a multiple UV maps, useful for text and animated sprites
enum gfxSpriteMap {
    GFX_NOMAP = 0,
    GFX_TEXTMAP = 1, 
    GFX_SPRITEMAP = 2, 
    GFX_SMEND
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
    gfxSpriteMap mapType;
    gfxTexCenter textureCenter;
    bool keepResident;
    bool useTexture;
    
    // Internal information for sprites, the system will supply these values and the app can query
    unsigned int baseWidth;
    unsigned int baseHeight;
    unsigned int glTextureId;
    bool isLoaded;
};

// Holds the current rendering information of the sprite instance - all things that can change or be animated
struct stSpriteInstance {
    unsigned int parentSpriteId;
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
    float u1,v1,u2,v2;
    float textureAlpha;
    float vertRed, vertGreen, vertBlue, vertAlpha;
    float scaleFactor;
    float rotateDegrees;
    bool updateBoundingBox;

    // These two will never be set by calling app
    unsigned int glTextureId;
    stBoundingBox boundingBox;
};

struct stTextJson {
    unsigned int width;
    unsigned int height;
    float U1, V1, U2, V2;
};

// Define a class for the OGL ES code
class PBGfx : public PBOGLES {

public:
    PBGfx();
    ~PBGfx();

    // Initialization function
    bool gfxInit ();

    // Sprite creation
    unsigned int gfxLoadSprite(const std::string& spriteName, const std::string& textureFileName, gfxTexType textureType,
                               gfxSpriteMap mapType, gfxTexCenter textureCenter, bool keepResident, bool useTexture);
    unsigned int gfxLoadSprite(stSpriteInfo spriteInfo);
    
    unsigned int gfxInstanceSprite (unsigned int parentSpriteId, unsigned int x, unsigned int y, unsigned int textureAlpha, 
                                    unsigned int vertRed, unsigned int vertGreen, unsigned int vertBlue, unsigned int vertAlpha, float scaleFactor, float rotateDegrees);
    unsigned int gfxInstanceSprite (unsigned int parentSpriteId, stSpriteInstance instance);
    unsigned int gfxInstanceSprite (unsigned int parentSpriteId);
    
    // Several ways to call the render sprite function - remaining values should be set before calling rendersprite
    bool         gfxRenderSprite(unsigned int spriteId);
    bool         gfxRenderSprite(unsigned int spriteId, unsigned int x, unsigned int y);
    bool         gfxRenderSprite(unsigned int spriteId, unsigned int x, unsigned int y, float scaleFactor, float rotateDegrees);

    // Character rendering functions
    bool         gfxRenderString(unsigned int spriteId,  std::string input, unsigned int x, unsigned int y);
    
    // Sprite manipulation functions
    unsigned int gfxSetXY(unsigned int spriteId, unsigned int X, unsigned int Y, bool addXY);
    unsigned int gfxSetTextureAlpha(unsigned int spriteId, float textureAlpha);
    unsigned int gfxSetColor(unsigned int spriteId, unsigned int red, unsigned int green, unsigned int blue, unsigned int alpha);
    unsigned int gfxSetScaleFactor(unsigned int spriteId, float scaleFactor, bool addFactor);
    unsigned int gfxSetRotateDegrees(unsigned int spriteId, float rotateDegrees, bool addDegrees);
    unsigned int gfxSetUpdateBoundingBox(unsigned int spriteId, bool updateBoundingBox);
    
    // Sprite Query Functions
    unsigned int gfxGetBaseHeight(unsigned int spriteId);
    unsigned int gfxGetBaseWidth(unsigned int spriteId);
    unsigned int gfxGetXY(unsigned int spriteId, unsigned int* X, unsigned int* Y);
    unsigned int gfxGetTextureAlpha(unsigned int spriteId);
    unsigned int gfxGetSystemSpriteId();
    unsigned int gfxGetColor(unsigned int spriteId, unsigned int* red, unsigned int* green, unsigned int* blue, unsigned int* alpha);
    float        gfxGetScaleFactor(unsigned int spriteId);
    float        gfxGetRotateDegrees(unsigned int spriteId);
    stBoundingBox gfxGetBoundingBox(unsigned int spriteId);
    bool         gfxIsLoaded(unsigned int spriteId);

    // Rendering functions
    void         gfxSwap();
    void         gfxClear(float red, float blue, float green, float alpha, bool doFlip);

    /*  Functions to add
    Create a font sprite type
    Write text from font sprite at location x,y (from a string / string array?)
    Create system font(s)
    */

private:
    unsigned int gfxSysLoadSprite(stSpriteInfo spriteInfo, bool bSystem);

    // User sprites start at 100. System sprites will use lower numbers
    unsigned int m_nextSystemSpriteId;
    unsigned int m_nextUserSpriteId;
    unsigned int m_systemSpriteId;
    // Color state used for sprite rendering.  These colors are used for the vertex of the sprite quad.
    std::map<unsigned int, stSpriteInfo> m_spriteList;
    std::map<unsigned int, stSpriteInstance> m_instanceList;
    std::map<unsigned int, json> m_textMapJSON;
    std::map<unsigned int, json> m_spriteMapJSON;
};

#endif // PBGfx_h

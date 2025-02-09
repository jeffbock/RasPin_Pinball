
// PBGfx contains all the fundamental graphics management operations and sits on top of PGOGLES
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#include "PBGfx.h"

// Constructor
PBGfx::PBGfx() {
    // Initialization code if needed
}

// Destructor
PBGfx::~PBGfx() {
    // Cleanup code if needed
}

// Private function to create a sprite
unsigned int PBGfx::gfxSysCreateSprite(stSpriteInfo spriteInfo, bool bSystem) {
    
    GLuint texture=0;
    oglTexType textureType;
    unsigned int width=0, height=0;

    // Convert the texture type to OGL type
    switch (spriteInfo.textureType) {
        case GFX_BMP:
            textureType = OGL_BMP;
            break;
        case GFX_PNG:
            textureType = OGL_PNG;
            break;
        default:
            return (NOSPRITE);
    }

    // If the texture file name is not empty, load the texture
    if (!spriteInfo.textureFileName.empty()) {
        texture = oglLoadTexture(spriteInfo.textureFileName.c_str(), textureType, &width, &height);
        if (texture == 0) return (NOSPRITE);
        else {
            spriteInfo.glTextureId = texture;
            spriteInfo.baseWidth = width;
            spriteInfo.baseHeight = height;
        }
    }
    else return (NOSPRITE);

    unsigned int spriteId = bSystem ? nextSystemSpriteId++ : nextUserSpriteId++;
    spriteMap[spriteId] = spriteInfo;
    return spriteId;
}

// Overloaded function to create a sprite with individual parameters
unsigned int PBGfx::gfxCreateSprite(const std::string& spriteName, const std::string& textureFileName, 
                                    gfxTexType textureType, gfxTexCenter textureCenter, float textureAlpha, 
                                    bool keepResident, float scaleFactor, int rotateDegrees, bool updateBoundingBox) {
    stSpriteInfo spriteInfo;
    spriteInfo.spriteName = spriteName;
    spriteInfo.textureFileName = textureFileName;
    spriteInfo.textureType = textureType;
    spriteInfo.textureCenter = textureCenter;
    spriteInfo.textureAlpha = textureAlpha;
    spriteInfo.keepResident = keepResident;
    spriteInfo.scaleFactor = scaleFactor;
    spriteInfo.rotateDegrees = rotateDegrees;
    spriteInfo.updateBoundingBox = updateBoundingBox;
    spriteInfo.baseWidth = 0; // Default value
    spriteInfo.baseHeight = 0; // Default value
    spriteInfo.glTextureId = 0; // Default value
    spriteInfo.boundingBox = {0, 0, 0, 0}; // Default value

    return gfxCreateSprite(spriteInfo);
}

// Public function to create a sprite
unsigned int PBGfx::gfxCreateSprite(stSpriteInfo spriteInfo) {
    return gfxSysCreateSprite(spriteInfo, false);
}

bool PBGfx::gfxRenderSprite(unsigned int spriteId, unsigned int x, unsigned int y){
    
    auto it = spriteMap.find(spriteId);
    if (it != spriteMap.end()) {

        // Using the center shifts the input
        bool useCenter = it->second.textureCenter == GFX_CENTER ? true : false;

        // Convert the integer x and y to float ranging from -1 to 1 based on the ratio of screen size in pixels
        float x2, y2;
        float x1 = (float)x / (float)oglGetScreenWidth() * 2.0f - 1.0f;
        float y1 = 1.0f - (float)y / (float)oglGetScreenHeight() * 2.0f;
        x2 = x1 + (float)it->second.baseWidth / (float)oglGetScreenWidth() * 2.0f;
        y2 = y1 - (float)it->second.baseHeight / (float)oglGetScreenHeight() * 2.0f;
        
        // If using center, then need to move everything up and left by the right amount
        if (useCenter){
        
            float shiftleft = (x2-x1)/2;
            float shiftup = (y2-y1)/2;

            x1 -= shiftleft;
            x2 -= shiftleft;
            y1 -= shiftup;
            y2 -= shiftup;
        }

        // Use alpha if the texture is a BMP (eg: use the the supplied alpha value, otherwise it is assumed PNG already has alpha)
        bool useAlpha = it->second.textureType == GFX_BMP ? true : false;

        // Render the sprite quad
        oglRenderQuad(&x1, &y1, &x2, &y2, it->second.scaleFactor, it->second.rotateDegrees, useCenter,it->second.updateBoundingBox, it->second.glTextureId, useAlpha, it->second.textureAlpha);

        // Update the bounding box if needed.  Convert the float X1,Y1,X2,Y2 values to screen space corridates and save them in the bounding box struct
        if (it->second.updateBoundingBox) {

            float fwidth = (float)oglGetScreenWidth();
            float fheight = (float)oglGetScreenHeight();

            it->second.boundingBox.x1 = (unsigned int)(((x1 + 1.0f) / 2.0f) * fwidth);
            it->second.boundingBox.y1 = (unsigned int)(((y1 + 1.0f) / 2.0f) * fheight);
            it->second.boundingBox.x2 = (unsigned int)(((x2 + 1.0f) / 2.0f) * fwidth);
            it->second.boundingBox.y2 = (unsigned int)(((y2 + 1.0f) / 2.0f) * fheight);
        }   
    }
    else return (false);

    return (true);
}

void PBGfx::gfxSwap() {
    oglSwap();
}

void PBGfx::gfxClear(float red, float blue, float green, float alpha, bool doFlip){
    oglClear(red, blue, green, alpha, doFlip);
}

// Set function for scaleFactor
unsigned int PBGfx::gfxSetScaleFactor(unsigned int spriteId, float scaleFactor, bool addFactor) {
    auto it = spriteMap.find(spriteId);
    if (it != spriteMap.end()) {
        if (!addFactor) it->second.scaleFactor = scaleFactor;
        else {
         it->second.scaleFactor += scaleFactor;
         if (it->second.scaleFactor < 0.1f) it->second.scaleFactor = 0.1f;
        }
    }
    else return (NOSPRITE);
    return (spriteId);
}

// Set function for rotateDegrees
unsigned int PBGfx::gfxSetRotateDegrees(unsigned int spriteId, int rotateDegrees, bool addDegrees) {
    auto it = spriteMap.find(spriteId);
    if (it != spriteMap.end()) {
        if (!addDegrees) it->second.rotateDegrees = rotateDegrees;
        else {
            it->second.rotateDegrees += rotateDegrees;
            it->second.rotateDegrees %= 360;
        }
    }
    else return (NOSPRITE);
    return (spriteId);
}

void PBGfx::gfxSetSpriteColor(unsigned int red, unsigned int green, unsigned int blue, unsigned int alpha){
    
    // Save these values, although they are not used anywhere
    m_Red = red;
    m_Green = green;
    m_Blue = blue;
    m_Alpha = alpha;

    // Convert to float, which is what OGL understands
    float fRed = (float)red / 255.0f;
    float fGreen = (float)green / 255.0f;
    float fBlue = (float)blue / 255.0f;
    float fAlpha = (float)alpha / 255.0f;

    oglSetQuadColor(fRed, fGreen, fBlue, fAlpha);
}

// Query function for baseHeight
unsigned int PBGfx::gfxGetBaseHeight(unsigned int spriteId) {
    auto it = spriteMap.find(spriteId);
    if (it != spriteMap.end()) {
        return it->second.baseHeight;
    }
    return 0; // Default value if spriteId not found
}

// Query function for baseWidth
unsigned int PBGfx::gfxGetBaseWidth(unsigned int spriteId) {
    auto it = spriteMap.find(spriteId);
    if (it != spriteMap.end()) {
        return it->second.baseWidth;
    }
    return 0; // Default value if spriteId not found
}

// Query function for boundingBox
stBoundingBox PBGfx::gfxGetBoundingBox(unsigned int spriteId) {
    auto it = spriteMap.find(spriteId);
    if (it != spriteMap.end()) {
        if (!it->second.updateBoundingBox) {
            return {0, 0, 0, 0}; // Return all zeros if updateBoundingBox is not set
        }
        return it->second.boundingBox;
    }
    return {0, 0, 0, 0}; // Default value if spriteId not found
}
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
    
    // TODO:  call the functions to create the intialize all the sprite information
    
    unsigned int spriteId = bSystem ? nextSystemSpriteId++ : nextUserSpriteId++;
    spriteMap[spriteId] = spriteInfo;
    return spriteId;
}

// Public function to create a sprite
unsigned int PBGfx::gfxCreateSprite(stSpriteInfo spriteInfo) {
    return gfxSysCreateSprite(spriteInfo, false);
}

// Set function for scaleFactor
unsigned int PBGfx::gfxSetScaleFactor(unsigned int spriteId, float scaleFactor, bool addFactor) {
    auto it = spriteMap.find(spriteId);
    if (it != spriteMap.end()) {
        if (!addFactor) it->second.scaleFactor = scaleFactor;
        else it->second.scaleFactor += scaleFactor;
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
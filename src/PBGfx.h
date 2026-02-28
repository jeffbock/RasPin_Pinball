// Gfx front-end sprite / font libary - generic interface, currently using OpenGL ES 3.1 backend
// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#ifndef PBGfx_h
#define PBGfx_h

// Prevent Windows min/max macros from interfering with std::min/std::max
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

//#include "include_ogl/egl.h"
//#include "include_ogl/gl31.h"
// #include <egl.h>
#include <gl31.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <random>
#include "3rdparty/json.hpp"
#include "PB3D.h"
 
#define NOSPRITE 0
#define SYSTEMFONTSPRITE "src/resources/fonts/Ubuntu-Regular_24_256.png"

using json = nlohmann::json;

// Define an enum for different texture file sources
enum gfxTexType {
    GFX_BMP = 0,
    GFX_PNG = 1, 
    GFX_NONE = 2,
    GFX_VIDEO = 3,     // Video texture (updated dynamically)
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

// Enum for text justification
enum gfxTextJustify {
    GFX_TEXTLEFT = 0,
    GFX_TEXTCENTER = 1, 
    GFX_TEXTRIGHT = 2, 
    GFX_TJEND
};

struct stBoundingBox {
    int x1;  // Upper Left
    int y1;  // Upper Left
    int x2;  // Lower Right
    int y2;  // Lower Right
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
    int x;
    int y;
    unsigned int width;
    unsigned int height;
    float u1,v1,u2,v2;
    float textureAlpha;
    float vertRed, vertGreen, vertBlue, vertAlpha;
    float scaleFactor;
    float rotateDegrees;
    bool updateBoundingBox;

    // These two will never be set by calling app
    // unsigned int glTextureId;
    stBoundingBox boundingBox;
};

struct stTextMapData {
    unsigned int width;
    unsigned int height;
    float U1, V1, U2, V2;
};

struct stSpriteMapData {
    unsigned int width;
    unsigned int height;
    float U1, V1, U2, V2;
};

#define ANIMATE_NOMASK 0x0
#define ANIMATE_X_MASK 0x1
#define ANIMATE_Y_MASK 0x2
#define ANIMATE_U_MASK 0x4
#define ANIMATE_V_MASK 0x8
#define ANIMATE_TEXALPHA_MASK 0x10
#define ANIMATE_COLOR_MASK 0x20
#define ANIMATE_SCALE_MASK 0x40
#define ANIMATE_ROTATE_MASK 0x80
#define ANIMATE_ALL_MASK 0xFF

// Animation enums are defined in PB3D.h (included above)

struct stAnimateData {
    unsigned int animateSpriteId;
    unsigned int startSpriteId;
    unsigned int endSpriteId;
    unsigned int startTick;
    unsigned int typeMask;
    float animateTimeSec;
    float accelPixelPerSecX;
    float accelPixelPerSecY;
    float accelDegPerSec;
    float randomPercent;
    bool isActive;
    bool rotateClockwise;
    gfxLoopType loop;
    gfxAnimType animType;
    
    // Initial velocity values (set when animation is created)
    float initialVelocityX;
    float initialVelocityY;
    float initialVelocityDeg;
    
    // Internal state for acceleration animations (updated during animation)
    float currentVelocityX;
    float currentVelocityY;
    float currentVelocityDeg;
};

// Define a class for the OGL ES code
class PBGfx : public PB3D {

public:
    PBGfx();
    ~PBGfx();

    // Initialization function
    bool gfxInit ();

    // Sprite creation
    unsigned int gfxLoadSprite(const std::string& spriteName, const std::string& textureFileName, gfxTexType textureType,
                               gfxSpriteMap mapType, gfxTexCenter textureCenter, bool keepResident, bool useTexture);
    unsigned int gfxLoadSprite(stSpriteInfo spriteInfo);
    bool         gfxUnloadTexture(unsigned int spriteId);
    bool         gfxUnloadAllTextures();
    bool         gfxReloadTexture(unsigned int spriteId);
    bool         gfxTextureLoaded(unsigned int spriteId);
    
    unsigned int gfxInstanceSprite (unsigned int parentSpriteId, int x, int y, unsigned int textureAlpha, 
                                    unsigned int vertRed, unsigned int vertGreen, unsigned int vertBlue, unsigned int vertAlpha, float scaleFactor, float rotateDegrees);
    unsigned int gfxInstanceSprite (unsigned int parentSpriteId, stSpriteInstance instance);
    unsigned int gfxInstanceSprite (unsigned int parentSpriteId);
    bool         gfxIsSprite (unsigned int spriteId);
    bool         gfxIsFontSprite (unsigned int spriteId);
    
    // Several ways to call the render sprite function - remaining values should be set before calling rendersprite
    bool         gfxRenderSprite(unsigned int spriteId);
    bool         gfxRenderSprite(unsigned int spriteId, int x, int y);
    bool         gfxRenderSprite(unsigned int spriteId, int x, int y, float scaleFactor, float rotateDegrees);

    // Character rendering functions
    bool         gfxRenderString(unsigned int spriteId, std::string input, unsigned int spacingPixels, gfxTextJustify justify);
    bool         gfxRenderString(unsigned int spriteId, std::string input, int x, int y, int spacingPixels, gfxTextJustify justify);
    bool         gfxRenderShadowString(unsigned int spriteId,  std::string input, int x, int y, unsigned int spacingPixels, gfxTextJustify justify,
                                       unsigned int red, unsigned int green, unsigned int blue, unsigned int alpha, unsigned int shadowOffset);
    int          gfxStringWidth(unsigned int spriteId, std::string input, unsigned int spacingPixels);
    
    // Sprite manipulation functions
    unsigned int gfxSetXY(unsigned int spriteId, int X, int Y, bool addXY);
    unsigned int gfxSetTextureAlpha(unsigned int spriteId, float textureAlpha);
    unsigned int gfxSetUV(unsigned int spriteId, float u1, float v1, float u2, float v2);
    unsigned int gfxSetWH(unsigned int spriteId, unsigned int width, unsigned int height);
    unsigned int gfxSetColor(unsigned int spriteId, unsigned int red, unsigned int green, unsigned int blue, unsigned int alpha);
    unsigned int gfxSetScaleFactor(unsigned int spriteId, float scaleFactor, bool addFactor);
    unsigned int gfxSetRotateDegrees(unsigned int spriteId, float rotateDegrees, bool addDegrees);
    unsigned int gfxSetUpdateBoundingBox(unsigned int spriteId, bool updateBoundingBox);
    
    // Sprite Query Functions
    unsigned int gfxGetBaseHeight(unsigned int spriteId);
    unsigned int gfxGetBaseWidth(unsigned int spriteId);
    unsigned int gfxGetXY(unsigned int spriteId, int* X, int* Y);
    unsigned int gfxGetTextureAlpha(unsigned int spriteId);
    unsigned int gfxGetSystemFontSpriteId();
    unsigned int gfxGetColor(unsigned int spriteId, unsigned int* red, unsigned int* green, unsigned int* blue, unsigned int* alpha);
    float        gfxGetScaleFactor(unsigned int spriteId);
    float        gfxGetRotateDegrees(unsigned int spriteId);
    stBoundingBox gfxGetBoundingBox(unsigned int spriteId);
    bool         gfxIsLoaded(unsigned int spriteId);
    unsigned int gfxGetTextHeight(unsigned int spriteId);

    // Automatic Sprite Animation Functions
    bool          gfxCreateAnimation(stAnimateData animateData, bool replaceExisting);
    bool          gfxAnimateSprite(unsigned int animateSpriteId, unsigned int currentTick);
    bool          gfxAnimateActive(unsigned int animateSpriteId);
    bool          gfxAnimateClear(unsigned int animateSpriteId);
    bool          gfxAnimateRestart(unsigned int animateSpriteId);
    bool          gfxAnimateRestart(unsigned int animateSpriteId, unsigned long startTick);
    void          gfxLoadAnimateData(stAnimateData *animateData, unsigned int animateSpriteId, unsigned int startSpriteId, unsigned int endSpriteId, 
                                     unsigned int typeMask, float animateTimeSec, bool isActive, gfxLoopType loop, gfxAnimType animType, 
                                     unsigned int startTick, float accelPixelPerSecX, float accelPixelPerSecY, float accelDegPerSec, 
                                     float randomPercent, bool rotateClockwise, float initialVelocityX, float initialVelocityY, float initialVelocityDeg);
    void          gfxLoadAnimateDataShort(stAnimateData *animateData, unsigned int animateSpriteId, unsigned int startSpriteId, unsigned int endSpriteId, 
                                          unsigned int typeMask, float animateTimeSec, bool isActive, gfxLoopType loop, gfxAnimType animType);

    // Rendering functions
    void         gfxSwap();
    void         gfxSwap(bool flush);
    void         gfxClear(float red, float blue, float green, float alpha, bool doFlip);
    void         gfxSetScissor(bool enable, stBoundingBox rect);

    // System Clock Function
    unsigned long GetTickCountGfx();
    
    // Video texture functions
    bool         gfxUpdateVideoTexture(unsigned int spriteId, const uint8_t* frameData, unsigned int width, unsigned int height);
    
private:
    unsigned int gfxSysLoadSprite(stSpriteInfo spriteInfo, bool bSystem);
    
    // Helper functions for animation types
    void gfxAnimateNormal(stAnimateData& animateData, unsigned int currentTick, float timeSinceStart, float percentComplete);
    void gfxAnimateAcceleration(stAnimateData& animateData, unsigned int currentTick, float timeSinceStart);
    void gfxAnimateJump(stAnimateData& animateData, unsigned int currentTick, float timeSinceStart);
    void gfxAnimateJumpRandom(stAnimateData& animateData, unsigned int currentTick, float timeSinceStart);
    void gfxSetFinalAnimationValues(const stAnimateData& animateData);
    float gfxGetRandomFloat(float min, float max);

    // User sprites start at 100. System sprites will use lower numbers
    unsigned int m_nextSystemSpriteId;
    unsigned int m_nextUserSpriteId;
    unsigned int m_systemFontSpriteId;

    // Color state used for sprite rendering.  These colors are used for the vertex of the sprite quad.
    std::map<unsigned int, stSpriteInfo> m_spriteList;
    std::map<unsigned int, stSpriteInstance> m_instanceList;

    // Text and sprite map data ([spriteId][char or spriteOrderNumber][stTextMapData or stSpriteMapData])
    std::map<unsigned int, std::map<std::string, stTextMapData>> m_textMapList;
    std::map<unsigned int, std::map<unsigned int, stSpriteMapData>> m_spriteMapList;

    // Animation list
    std::map<unsigned int, stAnimateData> m_animateList; 

};

#endif // PBGfx_h

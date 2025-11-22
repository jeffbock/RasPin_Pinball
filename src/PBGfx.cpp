// PBGfx contains all the fundamental graphics management operations and sits on top of PBOGLES
// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "PBGfx.h"

// Constructor
PBGfx::PBGfx() {
    
    m_nextSystemSpriteId = 1;
    m_nextUserSpriteId = 100;
    m_systemFontSpriteId = NOSPRITE;
}

// Destructor
PBGfx::~PBGfx() {
    // Cleanup code if needed
}

// Must use std::chrono so tick count can be used in a cross-platform way
unsigned long PBGfx::GetTickCountGfx() {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return static_cast<unsigned long>(duration.count());
}

// Any Gfx specific initialization code
bool PBGfx::gfxInit(){

    // Create the system font sprite
    m_systemFontSpriteId = gfxSysLoadSprite({"System Font", SYSTEMFONTSPRITE, GFX_PNG, GFX_TEXTMAP, GFX_UPPERLEFT, true, true}, true);

    if (m_systemFontSpriteId == NOSPRITE) return (false);
    else return (true);
}

bool PBGfx::gfxUnloadTexture(unsigned int spriteId) {
    
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {
        if (m_spriteList[it->second.parentSpriteId].keepResident) return (false);
        else {
            oglUnloadTexture (m_spriteList[it->second.parentSpriteId].glTextureId);
            m_spriteList[it->second.parentSpriteId].glTextureId = 0;
            m_spriteList[it->second.parentSpriteId].isLoaded = false;
            return (true);
        }
    }



    else return (false);
}

bool PBGfx::gfxUnloadAllTextures() {

    // Loop through m_SpriteList and unload all the textures for sprites that should not be kept resident
    for (auto it = m_spriteList.begin(); it != m_spriteList.end(); ++it) {
        if (!it->second.keepResident) {
            oglUnloadTexture(it->second.glTextureId);
            it->second.glTextureId = 0;
            it->second.isLoaded = false;
        }
    }

    return (true);
}

bool PBGfx::gfxTextureLoaded(unsigned int spriteId)
{
    if (spriteId == NOSPRITE) return (false);
    
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) return (m_spriteList[it->second.parentSpriteId].isLoaded);
    else return (false);
}

// Reload a texture if it's been freed
bool PBGfx::gfxReloadTexture(unsigned int spriteId) {
        
        unsigned int tempX, tempY;

        auto it = m_instanceList.find(spriteId);
        if (it != m_instanceList.end()) {
            if (m_spriteList[it->second.parentSpriteId].isLoaded) return (true);
            if (it->second.parentSpriteId != spriteId) return (false);
            else {
                oglTexType textureType;
                switch (m_spriteList[it->second.parentSpriteId].textureType) {
                    case GFX_BMP: textureType = OGL_BMP; break;
                    case GFX_PNG: textureType = OGL_PNG; break;
                    case GFX_NONE: textureType = OGL_NONE; break;
                    default: return (false);
                }
                m_spriteList[it->second.parentSpriteId].glTextureId = oglLoadTexture(m_spriteList[it->second.parentSpriteId].textureFileName.c_str(), textureType, &tempX, &tempY);
                if (m_spriteList[it->second.parentSpriteId].glTextureId != 0) 
                {
                    m_spriteList[it->second.parentSpriteId].isLoaded = true;
                    return (true);
                }
            }
        }

        return (false);
}

// Private function to create a sprite
unsigned int PBGfx::gfxSysLoadSprite(stSpriteInfo spriteInfo, bool bSystem) {
    
    // Check if a sprite with this name already exists
    for (auto it = m_spriteList.begin(); it != m_spriteList.end(); ++it) {
        if (it->second.spriteName == spriteInfo.spriteName) {
            // Sprite already exists - reload its texture and return the existing ID
            gfxReloadTexture(it->first);
            return it->first;
        }
    }
    
    
    GLuint texture=0;
    oglTexType textureType;
    unsigned int width=0, height=0;

    // Convert the texture and map type to OGL type - this is a bit awkward, but keeps in the interface
    // consistent with the GFX types
    switch (spriteInfo.textureType) {
        case GFX_BMP:
            textureType = OGL_BMP;
            // Text sprites must be PNG because they require alpha
            if (spriteInfo.mapType == GFX_TEXTMAP) return (NOSPRITE); 
            break;
        case GFX_PNG: textureType = OGL_PNG; break;
        case GFX_NONE: textureType = OGL_NONE; break;
        case GFX_VIDEO: textureType = OGL_VIDEO; break;
        default: return (NOSPRITE);
    }

    // If the texture file name is not empty, load the texture
    if ((!spriteInfo.textureFileName.empty()) && (spriteInfo.useTexture)) {
        texture = oglLoadTexture(spriteInfo.textureFileName.c_str(), textureType, &width, &height);
        if (texture == 0) 
        {
            // Code won't fail on a texture load, but it will update the sprite to make it no texture
            // This is more of a debug feature since if you specified a texture, it should load.
            // But we don't want to crash the app - quads will be rendered without a texture
            spriteInfo.useTexture = false;
            spriteInfo.glTextureId = 0;
            spriteInfo.baseWidth = 64;
            spriteInfo.baseHeight = 64;
            spriteInfo.isLoaded = false;
        }
        else {
            spriteInfo.glTextureId = texture;
            spriteInfo.baseWidth = width;
            spriteInfo.baseHeight = height;
        }
    }
    else {
        spriteInfo.glTextureId = texture;
        spriteInfo.baseWidth = width;
        spriteInfo.baseHeight = height;
    }

    // Assign the next sprite ID based on whether it is a system sprite or not
    unsigned int spriteId = 0;
    if (bSystem) {
        spriteId = m_nextSystemSpriteId;
        m_nextSystemSpriteId++;
    }
    else {
        spriteId = m_nextUserSpriteId;
        m_nextUserSpriteId++;
    }

    // If the sprite is a text sprite (and the texture load worked), load the JSON UV map from the text file into an internal map
    if (((spriteInfo.mapType == GFX_TEXTMAP) || (spriteInfo.mapType == GFX_SPRITEMAP)) && spriteInfo.useTexture) {
        // Create a file name that has the same name as filename as the texutre (minus the extension), and change extension to .json
        std::string jsonFileName = spriteInfo.textureFileName;
        jsonFileName = jsonFileName.substr(0, jsonFileName.find_last_of(".")) + ".json";

        json uvJson;

        // Use Json.hpp functions to load the JSON file and put it in a map structure with the key being the character
        std::ifstream jsonFile(jsonFileName);
        jsonFile >> uvJson;

        unsigned int test = 0;

        if (spriteInfo.mapType == GFX_TEXTMAP){
            // Go through uvJson, and place the values in the m_textMapList map
            for (auto it = uvJson.begin(); it != uvJson.end(); ++it) {
                std::string charString = it.key();

                stTextMapData textMapData;

                textMapData.width = uvJson[charString]["width"];
                textMapData.height = uvJson[charString]["height"];
                textMapData.U1 = uvJson[charString]["u1"];
                textMapData.V1 = uvJson[charString]["v1"];
                textMapData.U2 = uvJson[charString]["u2"];
                textMapData.V2 = uvJson[charString]["v2"];
                
                m_textMapList[spriteId][charString] = textMapData;
            }

            // Set the width of space to be the width of the letter "j"
            m_textMapList[spriteId][" "].width = m_textMapList[spriteId]["j"].width;
        }
        else {
            // Go through uvJson, and place the values in the m_spriteMapList map
            // This path is not currently implemented, but would be used for animated sprites
            // Need to change the stuctures, key to integer and come up with a way to generate the JSON file
            /*
            for (auto it = uvJson.begin(); it != uvJson.end(); ++it) {
                auto spriteOrder = it.key();

                stTextMapData textMapData;

                textMapData.width = uvJson[spriteOrder]["width"];
                textMapData.height = uvJson[spriteOrder]["height"];
                textMapData.U1 = uvJson[spriteOrder]["u1"];
                textMapData.V1 = uvJson[spriteOrder]["v1"];
                textMapData.U2 = uvJson[spriteOrder]["u2"];
                textMapData.V2 = uvJson[spriteOrder]["v2"];
                
                m_spriteMapList[spriteId][] = textMapData;
            }
            */
        } 

        // Release the json file
        jsonFile.close();
    }
    
    // Only set isLoaded if it's a texture sprite and the texture was loaded successfully
    if (spriteInfo.useTexture) spriteInfo.isLoaded = true;

    // Create the Sprite Info struct and add it to the map
    m_spriteList[spriteId] = spriteInfo;

    // Create the first corresponding sprite instance automatically
    stSpriteInstance instance;    
    
    instance.parentSpriteId = spriteId;
    instance.x = 0;
    instance.y = 0;
    instance.width = width;
    instance.height = height;
    instance.u1 = 0.0f;
    instance.v1 = 1.0f;
    instance.u2 = 1.0f;
    instance.v2 = 0.0f;
    instance.textureAlpha = 1.0f;
    instance.vertRed = 1.0f; instance.vertGreen = 1.0f; instance.vertBlue = 1.0f; instance.vertAlpha = 1.0f;
    instance.scaleFactor = 1.0f;
    instance.rotateDegrees = 0.0f;
    instance.updateBoundingBox = false;
    instance.boundingBox = {0, 0, 0, 0};

    m_instanceList[spriteId] = instance;

    return spriteId;
}

// Create an instance sprite - can be created from any instance sprite.  
unsigned int PBGfx::gfxInstanceSprite (unsigned int parentSpriteId, int x, int y, unsigned int textureAlpha, 
                                       unsigned int vertRed, unsigned int vertGreen, unsigned int vertBlue, unsigned int vertAlpha, float scaleFactor, float rotateDegrees){

    stSpriteInstance instance;    

    instance.x = x;
    instance.y = y;
    instance.textureAlpha = (float) textureAlpha / 255.0f;
    instance.vertRed = (float) vertRed / 255.0f; instance.vertGreen = (float)vertGreen / 255.0f; instance.vertBlue = (float)vertBlue / 255.0f; instance.vertAlpha = (float)vertAlpha / 255.0f;
    instance.scaleFactor = scaleFactor;
    instance.rotateDegrees = rotateDegrees;
    instance.updateBoundingBox = false;
    instance.boundingBox = {0, 0, 0, 0};

    return (gfxInstanceSprite (parentSpriteId, instance));
   
};

// Create an instance sprite - can be created from any instance sprite, but partent sprite will always be the original sprite
unsigned int PBGfx::gfxInstanceSprite (unsigned int parentSpriteId){
    
    auto it = m_instanceList.find(parentSpriteId);
    if (it != m_instanceList.end()){
        return gfxInstanceSprite(parentSpriteId, it->second);
    }
    else return (NOSPRITE);
}

// Create an instance sprite - can be created from any instance sprite, but partent sprite will always be the original sprite
unsigned int PBGfx::gfxInstanceSprite (unsigned int parentSpriteId,  stSpriteInstance instance){
    
    auto it = m_instanceList.find(parentSpriteId);
    if (it != m_instanceList.end()){

        // Cannot make instance sprites from font sprites or sprites with a map (eg: animated sprites)
        // if (m_spriteList[it->second.parentSpriteId].mapType != GFX_NOMAP) return (NOSPRITE);

        unsigned int spriteId = m_nextUserSpriteId;
        m_nextUserSpriteId++;

        // This will always point to a sprite ID that has a backing stSpriteInfo struct, never an instance only sprite.
        // First creation of a sprite will have the same ID for both the sprite and the instance
        instance.parentSpriteId = it->second.parentSpriteId;
        instance.updateBoundingBox = it->second.updateBoundingBox;
        instance.width = it->second.width;
        instance.height = it->second.height;

        m_instanceList[spriteId] = instance;

        return spriteId;
    }
    else return (NOSPRITE);
}

// Query function to see if a sprite exists
bool PBGfx::gfxIsSprite (unsigned int spriteId){
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) return (true);
    else return (false);
}

// Query function to see if a sprite exists
bool PBGfx::gfxIsFontSprite (unsigned int spriteId){
    auto it = m_instanceList.find(spriteId);
    if (it == m_instanceList.end()) return (false);
    
    // Check to make sure it's a font
    if (m_spriteList[it->second.parentSpriteId].mapType == GFX_TEXTMAP) return (true);
    else return (false);
}

// Will load all texture and key info for a base sprite and create the 1st instance of the sprite
unsigned int PBGfx::gfxLoadSprite(const std::string& spriteName, const std::string& textureFileName, gfxTexType textureType,
                                  gfxSpriteMap mapType, gfxTexCenter textureCenter, bool keepResident, bool useTexture) {

    stSpriteInfo spriteInfo;

    spriteInfo.spriteName = spriteName;
    spriteInfo.textureFileName = textureFileName;
    spriteInfo.textureType = textureType;
    spriteInfo.mapType = mapType;
    if ((mapType == GFX_TEXTMAP) || (mapType == GFX_SPRITEMAP)) spriteInfo.textureCenter = GFX_UPPERLEFT;
    else spriteInfo.textureCenter = textureCenter;
    spriteInfo.keepResident = keepResident;
    spriteInfo.useTexture = useTexture;

    spriteInfo.baseWidth = 0; // Default value
    spriteInfo.baseHeight = 0; // Default value
    spriteInfo.glTextureId = 0; // Default value
    spriteInfo.isLoaded = false; // Default value
    
    return gfxLoadSprite(spriteInfo);
}

// Public function to create a sprite
unsigned int PBGfx::gfxLoadSprite(stSpriteInfo spriteInfo) {
    return gfxSysLoadSprite(spriteInfo, false);
}

// First two render functions are conveinence functions that will automatically set sprite intance values rather than calling discrete set functions
bool PBGfx::gfxRenderSprite(unsigned int spriteId, int x, int y){
            
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {

        m_instanceList[spriteId].x = x;
        m_instanceList[spriteId].y = y;

        return gfxRenderSprite(spriteId);
    }

    return (false);
}

bool PBGfx::gfxRenderSprite(unsigned int spriteId, int x, int y, float scaleFactor, float rotateDegrees){

auto it = m_instanceList.find(spriteId);
if (it != m_instanceList.end()) {

    m_instanceList[spriteId].x = x;
    m_instanceList[spriteId].y = y;
    m_instanceList[spriteId].scaleFactor = scaleFactor;
    m_instanceList[spriteId].rotateDegrees = rotateDegrees;
    
    return gfxRenderSprite(spriteId);
}

return (false);
}

// Primary render function for sprites - this will need to be adjusted for difference sprite map types, but for now it's just basic no-map
// NOTE:  A few palaces in the code, we swap X1/X2 or U1/U2 values to shift from UV / 3D space to screen space
//        This may may also may not be right - it may be that we're rendering back facing triangles but it works w/ current render

bool PBGfx::gfxRenderSprite(unsigned int spriteId){
    
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {

        // Using the center shifts the input
        bool useCenter = m_spriteList[it->second.parentSpriteId].textureCenter == GFX_CENTER ? true : false;

        // Convert the integer x and y to float ranging from -1 to 1 based on the ratio of screen size in pixels
        float x2, y2;
        float x1 = (float)m_instanceList[spriteId].x / (float)oglGetScreenWidth() * 2.0f - 1.0f;
        float y1 = 1.0f - (float)m_instanceList[spriteId].y / (float)oglGetScreenHeight() * 2.0f;
        x2 = x1 + (float)m_instanceList[spriteId].width / (float)oglGetScreenWidth() * 2.0f;
        y2 = y1 - (float)m_instanceList[spriteId].height / (float)oglGetScreenHeight() * 2.0f;
        
        // If using center, then need to move everything up and left by the right amount
        if (useCenter){
        
            float shiftleft = (x2-x1)/2;
            float shiftup = (y2-y1)/2;

            x1 -= shiftleft;
            x2 -= shiftleft;
            y1 -= shiftup;
            y2 -= shiftup;
        }

        // Use alpha if the texture is a BMP or VIDEO (eg: use the the supplied alpha value, otherwise it is assumed PNG already has alpha)
        bool useTexAlpha = (m_spriteList[it->second.parentSpriteId].textureType == GFX_BMP || 
                           m_spriteList[it->second.parentSpriteId].textureType == GFX_VIDEO) ? true : false;

        // Change the textureID to no texture if the sprite is not using a texture
        unsigned int tempTextureId = (m_spriteList[it->second.parentSpriteId].glTextureId);
        // if (!m_spriteList[it->second.parentSpriteId].useTexture) tempTextureId = 0;

        // Check if a texture is being used, if it is, make sure the texture is loaded
        // If the load fails, simply don't use a texture, which at least allows the render to continue, but the sprite will not be textured
        if (m_spriteList[it->second.parentSpriteId].useTexture) {
            if (!m_spriteList[it->second.parentSpriteId].isLoaded) {
                if (!gfxReloadTexture(it->second.parentSpriteId)) return (tempTextureId == 0);
            }
        }

        // Render the sprite quad
        oglRenderQuad(&x1, &y1, &x2, &y2, it->second.u1, it->second.v1, it->second.u2, it->second.v2, useCenter, useTexAlpha, it->second.textureAlpha, tempTextureId, it->second.vertRed, it->second.vertGreen, it->second.vertBlue, it->second.vertAlpha, it->second.scaleFactor, it->second.rotateDegrees, it->second.updateBoundingBox);
            
        // Update the bounding box if needed.  Convert the float X1,Y1,X2,Y2 values to screen space corridates and save them in the bounding box struct
        if (it->second.updateBoundingBox) {

            float fwidth = (float)oglGetScreenWidth();
            float fheight = (float)oglGetScreenHeight();

            it->second.boundingBox.x2 = (int)(((x1 + 1.0f) / 2.0f) * fwidth);
            it->second.boundingBox.y1 = (int)(fheight - (((y1 + 1.0f) / 2.0f) * fheight));
            it->second.boundingBox.x1 = (int)(((x2 + 1.0f) / 2.0f) * fwidth);
            it->second.boundingBox.y2 = (int)(fheight - (((y2 + 1.0f) / 2.0f) * fheight));
        }   
    }
    else return (false);

    return (true);
}

// This version just uses the X and Y values from the sprite instance

bool PBGfx::gfxRenderString(unsigned int spriteId, std::string input, unsigned int spacingPixels, gfxTextJustify justify) {

    auto it2 = m_instanceList.find(spriteId);
    if (it2 == m_instanceList.end()) return (false);
    if (m_spriteList[it2->second.parentSpriteId].mapType != GFX_TEXTMAP) return (false);

    unsigned int x = m_instanceList[spriteId].x;
    unsigned int y = m_instanceList[spriteId].y;

    return gfxRenderString(spriteId, input, x, y, spacingPixels, justify);
}

// Full Version of the function that renders a string of text

bool PBGfx::gfxRenderString(unsigned int spriteId, std::string input, int x, int y, int spacingPixels, gfxTextJustify justify) {

     // Find the sprite ID in the m_instanceList, if it isn't a textmap then return false
     auto it2 = m_instanceList.find(spriteId);
     if (it2 == m_instanceList.end()) return (false);
     if (m_spriteList[it2->second.parentSpriteId].mapType != GFX_TEXTMAP) return (false);

    unsigned int width=0, height=0;
    float u1=0.0f, v1=0.0f, u2 = 1.0f, v2 = 1.0f;

    // For each character in the string, set up the rendering info and call the render sprite function
    for (unsigned int i = 0; i < input.length(); i++) {
        unsigned int charId = (unsigned int)input[i];

        // if the character isn't the the standard ASCII range, then skip it
        if (charId < 32 || charId > 126) continue;

        // Convert the character to a string
        std::string charString = std::string(1, input[i]);

        // Check if the character exists in m_textMapList
        if (m_textMapList[spriteId].find(charString) != m_textMapList[spriteId].end()) {

            // Get the the required info from the font JSON
            width = m_textMapList[spriteId][charString].width;
            height = m_textMapList[spriteId][charString].height;
            u1 = m_textMapList[spriteId][charString].U1;
            v1 = m_textMapList[spriteId][charString].V1;
            u2 = m_textMapList[spriteId][charString].U2;
            v2 = m_textMapList[spriteId][charString].V2;
        }
        else continue;       

        // Set the relevant values in the sprite instance
        m_instanceList[spriteId].width = width;
        m_instanceList[spriteId].height = height;
        m_instanceList[spriteId].u1 = u1;
        m_instanceList[spriteId].v1 = v2;
        m_instanceList[spriteId].u2 = u2;
        m_instanceList[spriteId].v2 = v1;
        m_instanceList[spriteId].y = y;
        m_instanceList[spriteId].x = x;

        int stringWidth = gfxStringWidth(spriteId, input, spacingPixels);
        int oldX = m_instanceList[spriteId].x;

        // Adjust the x coordinate based on the justification
        if (justify == GFX_TEXTCENTER) m_instanceList[spriteId].x -= stringWidth / 2;
        else if (justify == GFX_TEXTRIGHT) m_instanceList[spriteId].x -= stringWidth;

        // Render the sprite instance using gfxRenderSprite and the parameters from the sprite instance
        // Skip space since there's nothing to render
        if (charString != " ") gfxRenderSprite(spriteId);
        
        x = oldX;
        // Move the x coordinate to the right for the next character
        if (m_instanceList[spriteId].scaleFactor != 1.0f) x += (int)((float)(width + spacingPixels) * m_instanceList[spriteId].scaleFactor);
        else x += (width + spacingPixels);
    }

    return (true);
}

// Render a string, except also render a shadow behind the string

bool  PBGfx::gfxRenderShadowString(unsigned int spriteId,  std::string input, int x, int y, unsigned int spacingPixels, gfxTextJustify justify,
                                   unsigned int red, unsigned int green, unsigned int blue, unsigned int alpha, unsigned int shadowOffset) {

    // Find the sprite ID in the m_instanceList, if it isn't a textmap then return false
    auto it2 = m_instanceList.find(spriteId);
    if (it2 == m_instanceList.end()) return (false);

    // Save the colors, they will need to be set back later
    float origRed = m_instanceList[spriteId].vertRed;
    float origGreen = m_instanceList[spriteId].vertGreen;
    float origBlue = m_instanceList[spriteId].vertBlue;
    float origAlpha = m_instanceList[spriteId].vertAlpha;
    bool success = false;

    // Set the shadow color
    gfxSetColor(spriteId, red, green, blue, alpha);

    // Render the shadow string
    success = gfxRenderString(spriteId, input, x + shadowOffset, y + shadowOffset, spacingPixels, justify);

    // Restore the old X/Y and color values
    m_instanceList[spriteId].vertRed = origRed;
    m_instanceList[spriteId].vertGreen = origGreen;
    m_instanceList[spriteId].vertBlue = origBlue;
    m_instanceList[spriteId].vertAlpha = origAlpha;

    if (!success) return (false);

    // Render the original string
    success = gfxRenderString(spriteId, input, x, y, spacingPixels, justify);

    return (success);
}

int  PBGfx::gfxStringWidth(unsigned int spriteId, std::string input, unsigned int spacingPixels){
     
    // Find the sprite ID in the m_instanceList, if it isn't a textmap then return false
     auto it2 = m_instanceList.find(spriteId);
     if (it2 == m_instanceList.end()) return (NOSPRITE);
     if (m_spriteList[it2->second.parentSpriteId].mapType != GFX_TEXTMAP) return (NOSPRITE);

     int width = 0;
     // Go through each character in the string and add the width of the character plus the spacing to the width
    for (unsigned int i = 0; i < input.length(); i++) {
        
        // Convert the character to a string
        std::string charString = std::string(1, input[i]);

        // Check if the character exists in m_textMapList
        if (m_textMapList[spriteId].find(charString) != m_textMapList[spriteId].end()) {
            width += m_textMapList[spriteId][charString].width + spacingPixels;
        }
    }
    // Subtract the last spacing value
    width -= spacingPixels;

    // If the scale factor is not 1.0, then scale the width
    if (m_instanceList[spriteId].scaleFactor != 1.0f) width = (int)((float)width * m_instanceList[spriteId].scaleFactor);

    return (width);
}
                                

void PBGfx::gfxSwap() {
    oglSwap(false);
}

void PBGfx::gfxSwap(bool flush) {
    oglSwap(flush);
}

void PBGfx::gfxClear(float red, float blue, float green, float alpha, bool doFlip){
    oglClear(red, blue, green, alpha, doFlip);
}

void PBGfx::gfxSetScissor(bool enable, stBoundingBox rect){
    oglSetScissor(enable, rect.x1, rect.y1, rect.x2, rect.y2);
}

// Set function for scaleFactor
unsigned int PBGfx::gfxSetScaleFactor(unsigned int spriteId, float scaleFactor, bool addFactor) {
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {
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
unsigned int PBGfx::gfxSetRotateDegrees(unsigned int spriteId, float rotateDegrees, bool addDegrees) {
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {
        if (!addDegrees) it->second.rotateDegrees = rotateDegrees;
        else {
            it->second.rotateDegrees += rotateDegrees;
            it->second.rotateDegrees = fmod(it->second.rotateDegrees, 360.0f);
        }
    }
    else return (NOSPRITE);
    return (spriteId);
}

// Set function for setting sprite vertex color
unsigned int PBGfx::gfxSetColor(unsigned int spriteId, unsigned int red, unsigned int green, unsigned int blue, unsigned int alpha){
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {
        it->second.vertRed = (float)red / 255.0f;
        it->second.vertGreen = (float)green / 255.0f;
        it->second.vertBlue = (float)blue / 255.0f;
        it->second.vertAlpha = (float)alpha / 255.0f;

        return (spriteId);
    }
    else return (NOSPRITE);
}

// Set function for setting XY coordinates of the sprite (can also be handled automatically with the render functions)
unsigned int PBGfx::gfxSetXY(unsigned int spriteId, int X, int Y, bool addXY){
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {
        if (!addXY) {
            it->second.x = X;
            it->second.y = Y;
        }
        else {
            it->second.x += X;
            it->second.y += Y;
        }
    }
    else return (NOSPRITE);
    return (spriteId);
}

unsigned int PBGfx:: gfxSetUV(unsigned int spriteId, float u1, float v1, float u2, float v2){
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {
        it->second.u1 = u1;
        it->second.v1 = v1;
        it->second.u2 = u2;
        it->second.v2 = v2;
        return (spriteId);
    }
    else return (NOSPRITE);
}

unsigned int PBGfx:: gfxSetWH(unsigned int spriteId, unsigned int width, unsigned int height){
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {
        it->second.width = width;
        it->second.height = height;
        return (spriteId);
    }
    else return (NOSPRITE);
}

// Set fucntion for updatng the texture alpha value (this will allow for texture fade in / out)
unsigned int PBGfx:: gfxSetTextureAlpha(unsigned int spriteId, float textureAlpha){
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {
        it->second.textureAlpha = textureAlpha;
        return (spriteId);
    }
    else return (NOSPRITE);
}

// Set function for updating the bounding box or not
unsigned int PBGfx:: gfxSetUpdateBoundingBox(unsigned int spriteId, bool updateBoundingBox){
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {
        it->second.updateBoundingBox = updateBoundingBox;
        return (spriteId);
    }
    else return (NOSPRITE);
}


// Query function for baseHeight
unsigned int PBGfx::gfxGetBaseHeight(unsigned int spriteId) {
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {
        return m_spriteList[spriteId].baseHeight;
    }
    return 0; // Default value if spriteId not found
}

// Query function for baseWidth
unsigned int PBGfx::gfxGetBaseWidth(unsigned int spriteId) {
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {
        return m_spriteList[spriteId].baseWidth;
    }
    return 0; // Default value if spriteId not found
}

// Query function for text height
unsigned int PBGfx::gfxGetTextHeight(unsigned int spriteId){
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {
        // If the sprite isn't a text sprite, then return 0
        if (m_spriteList[it->second.parentSpriteId].mapType != GFX_TEXTMAP) return NOSPRITE;
        // Doesn't really matter what we pick because all characters are the same height
        return m_textMapList[spriteId]["A"].height;
    }
    return NOSPRITE; // Default value if spriteId not found
}

// Query function for XY
unsigned int PBGfx::gfxGetXY(unsigned int spriteId, int* X, int* Y){
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {
        *X = it->second.x;
        *Y = it->second.y;
        return (spriteId);
    }
    else return (NOSPRITE);
}

// Query fucntion for texture alpha, convert from float   
unsigned int PBGfx::gfxGetTextureAlpha(unsigned int spriteId){
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {
        return (unsigned int)(it->second.textureAlpha * 255.0f);
    }
    return 0; // Default value if spriteId not found
}

// Query function for color
unsigned int PBGfx::gfxGetColor(unsigned int spriteId, unsigned int* red, unsigned int* green, unsigned int* blue, unsigned int* alpha){
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {
        *red = (unsigned int)(it->second.vertRed * 255.0f);
        *green = (unsigned int)(it->second.vertGreen * 255.0f);
        *blue = (unsigned int)(it->second.vertBlue * 255.0f);
        *alpha = (unsigned int)(it->second.vertAlpha * 255.0f);
        return (spriteId);
    }
    else return (NOSPRITE);
}

// Query function for scale factor
float PBGfx::gfxGetScaleFactor(unsigned int spriteId){
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {
        return it->second.scaleFactor;
    }
    return 0.0f; // Default value if spriteId not found
}

// Query function for rotate degrees
float PBGfx::gfxGetRotateDegrees(unsigned int spriteId){
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {
        return it->second.rotateDegrees;
    }
    return 0.0f; // Default value if spriteId not found
}

// Query function for boundingBox
stBoundingBox PBGfx::gfxGetBoundingBox(unsigned int spriteId) {
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {
        if (!it->second.updateBoundingBox) {
            return {0, 0, 0, 0}; // Return all zeros if updateBoundingBox is not set
        }
        return it->second.boundingBox;
    }
    return {0, 0, 0, 0}; // Default value if spriteId not found
}

// Query function to see if the texture is current loaded
bool PBGfx::gfxIsLoaded(unsigned int spriteId){
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {
        return m_spriteList[spriteId].isLoaded;
    }
    return false; // Default value if spriteId not found
}

// PBGfx loads a sprite by default, this allows the caller to get the ID to use for text rendering
unsigned int PBGfx::gfxGetSystemFontSpriteId(){
    return m_systemFontSpriteId;
}

// Functions to animation sprites

// Load the animate data for a sprite
void PBGfx::gfxLoadAnimateData(stAnimateData *animateData, unsigned int animateSpriteId, unsigned int startSpriteId, unsigned int endSpriteId, 
    unsigned int typeMask, float animateTimeSec, bool isActive, gfxLoopType loop, gfxAnimType animType,
    unsigned int startTick, float accelPixelPerSecX, float accelPixelPerSecY, float accelDegPerSec, 
    float randomPercent, bool rotateClockwise, float initialVelocityX, float initialVelocityY, float initialVelocityDeg){

    // Set the values in the animateData struct
    animateData->animateSpriteId = animateSpriteId;
    animateData->startSpriteId = startSpriteId;
    animateData->endSpriteId = endSpriteId;
    animateData->typeMask = typeMask;   
    animateData->animateTimeSec = animateTimeSec;
    animateData->isActive = isActive;
    animateData->loop = loop;
    animateData->animType = animType;
    animateData->startTick = (startTick == 0) ? GetTickCountGfx() : startTick;
    animateData->accelPixelPerSecX = accelPixelPerSecX;
    animateData->accelPixelPerSecY = accelPixelPerSecY;
    animateData->accelDegPerSec = accelDegPerSec;
    animateData->randomPercent = randomPercent;
    animateData->rotateClockwise = rotateClockwise;
    animateData->initialVelocityX = initialVelocityX;
    animateData->initialVelocityY = initialVelocityY;
    animateData->initialVelocityDeg = initialVelocityDeg;
    animateData->currentVelocityX = initialVelocityX;
    animateData->currentVelocityY = initialVelocityY;
    animateData->currentVelocityDeg = initialVelocityDeg;
}

// Simplified version for common use cases where acceleration, random, and velocity parameters are not needed
void PBGfx::gfxLoadAnimateDataShort(stAnimateData *animateData, unsigned int animateSpriteId, unsigned int startSpriteId, unsigned int endSpriteId, 
    unsigned int typeMask, float animateTimeSec, bool isActive, gfxLoopType loop, gfxAnimType animType){
    
    // Call the full version with zeros for all optional parameters
    gfxLoadAnimateData(animateData, animateSpriteId, startSpriteId, endSpriteId, 
                       typeMask, animateTimeSec, isActive, loop, animType,
                       0, 0.0f, 0.0f, 0.0f, 0.0f, true, 0.0f, 0.0f, 0.0f);
}

// Creation of the animation will add it to the active animation list
bool PBGfx::gfxCreateAnimation(stAnimateData animateData, bool replaceExisting){

    // Check that all instance sprites point the the same parent and have the same map type
    auto it = m_instanceList.find(animateData.startSpriteId);
    if (it == m_instanceList.end()) return (false);
    // if (m_spriteList[it->second.parentSpriteId].mapType != GFX_NOMAP) return (false);
    unsigned int parentSpriteId = it->second.parentSpriteId;

    it = m_instanceList.find(animateData.endSpriteId);
    if (it == m_instanceList.end()) return (false);
    // if (m_spriteList[it->second.parentSpriteId].mapType != GFX_NOMAP) return (false);
    if (it->second.parentSpriteId != parentSpriteId) return (false);

    it = m_instanceList.find(animateData.animateSpriteId);
    if (it == m_instanceList.end()) return (false);
    // if (m_spriteList[it->second.parentSpriteId].mapType != GFX_NOMAP) return (false);
    if (it->second.parentSpriteId != parentSpriteId) return (false);

    // Check to see if there's an active animation for this sprite and then remove it if needed
    if (!replaceExisting) {
        auto it = m_animateList.find(animateData.animateSpriteId);
        if (it != m_animateList.end()) return (false);
    }
    else {
        auto it = m_animateList.find(animateData.animateSpriteId);
        if (it != m_animateList.end()) m_animateList.erase(it);
    }

    // Initialize the animate sprite to match the start sprite's values
    // (but not for JUMP/JUMPRANDOM - they set their own values)
    if (animateData.animType != GFX_ANIM_JUMP && animateData.animType != GFX_ANIM_JUMPRANDOM) {
        m_instanceList[animateData.animateSpriteId] = m_instanceList[animateData.startSpriteId];
    }

    // Add the animation to the animationList map
    m_animateList[animateData.animateSpriteId] = animateData;

    return (true);
}

// This function will animate a sprite based on the current tick, values for the render sprite will be updated in the animateSpriteId instance
bool PBGfx::gfxAnimateSprite(unsigned int animateSpriteId, unsigned int currentTick){

    // Loop through all the animateList, and update the sprite instance values
    // If animateSpriteId is 0 then update all animations otherwise skip everything but the one with the animateSpriteId

    static unsigned long lastDebugTick = 0;
    bool foundAnimation = false;

    for (auto it = m_animateList.begin(); it != m_animateList.end(); ++it) {
        if ((animateSpriteId == NOSPRITE) || (it->first == animateSpriteId)) {
            foundAnimation = true;

            // Safety check on startTick, it should not be in the future, if it is, then reset it to currentTick
            if (it->second.startTick > currentTick) it->second.startTick = currentTick;

            // Calculate the time since the animation started
            float timeSinceStart = (float)(currentTick -  it->second.startTick) / 1000.0f;
            
            // If the animation is active, then update the sprite instance values
            if ((it->second.isActive) && (timeSinceStart >= 0.0f)) {
                // Calculate the percentage of the animation that has been completed
                float percentComplete;

                // Handle case of zero or negative animateTimeSec (error case, but could happen in acceleration cases)
                if (it->second.animateTimeSec <= 0.0f) percentComplete = 1.0f;
                else percentComplete  = timeSinceStart / it->second.animateTimeSec;

                // Dispatch to appropriate animation handler based on type
                // JUMP and JUMPRANDOM need to run even when percentComplete >= 1.0
                // ACCL runs continuously and checks its own completion
                switch (it->second.animType) {
                case GFX_ANIM_NORMAL:
                    if (percentComplete < 1.0f) {
                        gfxAnimateNormal(it->second, currentTick, timeSinceStart, percentComplete);
                    }
                    break;
                case GFX_ANIM_ACCL:
                    // Acceleration animation runs continuously until it reaches target
                    gfxAnimateAcceleration(it->second, currentTick, timeSinceStart);
                    break;
                case GFX_ANIM_JUMP:
                    gfxAnimateJump(it->second, currentTick, timeSinceStart);
                    break;
                case GFX_ANIM_JUMPRANDOM:
                    gfxAnimateJumpRandom(it->second, currentTick, timeSinceStart);
                    break;
                default:
                    if (percentComplete < 1.0f) {
                        gfxAnimateNormal(it->second, currentTick, timeSinceStart, percentComplete);
                    }
                    break;
                }

                // Handle animation completion and looping AFTER animation functions run
                // For ACCL animations, check if they deactivated themselves (reached target)
                bool animationComplete = false;
                if (it->second.animType == GFX_ANIM_ACCL) {
                    animationComplete = !it->second.isActive;
                } else {
                    if (percentComplete >= 1.0f) animationComplete = true;
                }
                
                if (animationComplete) {
                    unsigned long temp;
                    switch (it->second.loop) {
                    case GFX_RESTART:
                        // For JUMP/JUMPRANDOM, RESTART behaves like REVERSE - swap positions and jump back
                        // This makes the sprite stay at each position for the full duration
                        if (it->second.animType == GFX_ANIM_JUMP || it->second.animType == GFX_ANIM_JUMPRANDOM) {
                            // Swap start and end sprites to reverse direction
                            temp = it->second.startSpriteId;
                            it->second.startSpriteId = it->second.endSpriteId;
                            it->second.endSpriteId = temp;
                            it->second.startTick = currentTick;
                            it->second.isActive = true;
                            // Sprite stays at current position (which becomes the new "start")
                            // It will jump to the new "end" (old start) when time elapses
                        } else {
                            // For other animation types, restart means go back to original start
                            it->second.startTick = currentTick;
                            it->second.isActive = true;
                            m_instanceList[it->second.animateSpriteId] = m_instanceList[it->second.startSpriteId];
                            // Reset velocity to initial values for acceleration animations
                            if (it->second.animType == GFX_ANIM_ACCL) {
                                it->second.currentVelocityX = it->second.initialVelocityX;
                                it->second.currentVelocityY = it->second.initialVelocityY;
                                it->second.currentVelocityDeg = it->second.initialVelocityDeg;
                            }
                        }
                        break;
                    case GFX_REVERSE:
                        // Note: REVERSE looping not supported on ACCL, because completion position and rotation depends on velocity and acceleration
                        if (it->second.animType != GFX_ANIM_ACCL) {
                            // Swap start and end sprites to reverse direction
                            temp = it->second.startSpriteId;
                            it->second.startSpriteId = it->second.endSpriteId;
                            it->second.endSpriteId = temp;
                            it->second.startTick = currentTick;
                            it->second.isActive = true;

                            // Reset sprite to new start position
                            // For NORMAL animations, copy all properties from start sprite
                            // For JUMP/JUMPRANDOM animations, they will set their own values on the next jump
                            // but we still need to position them at the start for consistency
                            if (it->second.animType == GFX_ANIM_NORMAL || 
                                it->second.animType == GFX_ANIM_JUMP || 
                                it->second.animType == GFX_ANIM_JUMPRANDOM) {
                                m_instanceList[it->second.animateSpriteId] = m_instanceList[it->second.startSpriteId];
                            }
                        }

                        break;
                    case GFX_NOLOOP:
                        it->second.isActive = false;
                        // Configure final values based on type mask.  ACCL does not set final values since final position depends on velocity/accel
                        if (it->second.animType != GFX_ANIM_ACCL) gfxSetFinalAnimationValues(it->second);
                        break;
                    default: break;
                    }
                }
            }   
        }
    }

    return (true);
}

// Query to see if any animation is active.  If the animateSpriteId is 0, then it will return true if any animation is active
bool PBGfx::gfxAnimateActive(unsigned int animateSpriteId){

    // Case to match a particular spriteId
    if (animateSpriteId != NOSPRITE)
    {
        auto it = m_animateList.find(animateSpriteId);
        if (it == m_animateList.end()) return (false);

        return (it->second.isActive);
    }
    // Case of any spriteId is active
    else {
        for (auto it = m_animateList.begin(); it != m_animateList.end(); ++it) {
            if (it->second.isActive) return (true);
        }
    }
    return (false);
}

// Clears an animation from the active animation list.  If animateSpriteId is 0, then all animations are cleared
bool PBGfx::gfxAnimateClear(unsigned int animateSpriteId){

    // If animateSpriteId is 0, then clear all animations
    if (animateSpriteId == NOSPRITE) {
        m_animateList.clear();
        return (true);
    }
    // Clear a specific animation
    else{
        auto it = m_animateList.find(animateSpriteId);
        if (it != m_animateList.end()) {
            m_animateList.erase(it);
            return (true);
        }
    }

return (false);

}

// Restarts an animation from the beginning
// Allows Animate Restart to use a passed in tick count for the start tick
bool PBGfx::gfxAnimateRestart(unsigned int animateSpriteId, unsigned long startTick){

    auto it = m_animateList.find(animateSpriteId);
    if (it != m_animateList.end()) {
        it->second.startTick = startTick;
        it->second.isActive = true;
        
        // Reset the animate sprite to the start sprite's values for all animation types
        // JUMP/JUMPRANDOM animations need to be at start so they can jump to end when time elapses
        m_instanceList[it->second.animateSpriteId] = m_instanceList[it->second.startSpriteId];
        
        // Reset velocity to initial values for acceleration animations
        if (it->second.animType == GFX_ANIM_ACCL) {
            it->second.currentVelocityX = it->second.initialVelocityX;
            it->second.currentVelocityY = it->second.initialVelocityY;
            it->second.currentVelocityDeg = it->second.initialVelocityDeg;
        }
        
        return (true);
    }

    return (false);
}

// Restarts an animation from the beginning, automatically using the current tick count
// This can cause issues if a given rendering function is using a saved value for the current tick, but then also trying to animate during the same frame after restarting the animation.
bool PBGfx::gfxAnimateRestart(unsigned int animateSpriteId){
    return gfxAnimateRestart(animateSpriteId, GetTickCountGfx());
}

// Helper function to set final animation values when animation completes with GFX_NOLOOP
void PBGfx::gfxSetFinalAnimationValues(const stAnimateData& animateData) {
    // Depending on the mask, set the final values so the sprite ends up in the right place
    if (animateData.typeMask & ANIMATE_X_MASK) 
        m_instanceList[animateData.animateSpriteId].x = m_instanceList[animateData.endSpriteId].x;
    if (animateData.typeMask & ANIMATE_Y_MASK) 
        m_instanceList[animateData.animateSpriteId].y = m_instanceList[animateData.endSpriteId].y;
    if (animateData.typeMask & ANIMATE_SCALE_MASK) 
        m_instanceList[animateData.animateSpriteId].scaleFactor = m_instanceList[animateData.endSpriteId].scaleFactor;
    
    // For rotation with ACCL animations: only set final rotation if start != end
    // If start == end with velocity/accel (free rotation), keep current rotation
    if (animateData.typeMask & ANIMATE_ROTATE_MASK) {
        float startDeg = m_instanceList[animateData.startSpriteId].rotateDegrees;
        float endDeg = m_instanceList[animateData.endSpriteId].rotateDegrees;
        bool hasFreeRotation = (animateData.animType == GFX_ANIM_ACCL) && (startDeg == endDeg) && 
                               (animateData.accelDegPerSec != 0.0f || animateData.initialVelocityDeg != 0.0f);
        if (!hasFreeRotation) {
            m_instanceList[animateData.animateSpriteId].rotateDegrees = m_instanceList[animateData.endSpriteId].rotateDegrees;
        }
    }
    
    if (animateData.typeMask & ANIMATE_TEXALPHA_MASK) 
        m_instanceList[animateData.animateSpriteId].textureAlpha = m_instanceList[animateData.endSpriteId].textureAlpha;
    if (animateData.typeMask & ANIMATE_COLOR_MASK) {
        m_instanceList[animateData.animateSpriteId].vertRed = m_instanceList[animateData.endSpriteId].vertRed;
        m_instanceList[animateData.animateSpriteId].vertGreen = m_instanceList[animateData.endSpriteId].vertGreen;
        m_instanceList[animateData.animateSpriteId].vertBlue = m_instanceList[animateData.endSpriteId].vertBlue;
        m_instanceList[animateData.animateSpriteId].vertAlpha = m_instanceList[animateData.endSpriteId].vertAlpha;
    }
    if (animateData.typeMask & ANIMATE_U_MASK) 
        m_instanceList[animateData.animateSpriteId].u1 = m_instanceList[animateData.endSpriteId].u1;
    if (animateData.typeMask & ANIMATE_V_MASK) 
        m_instanceList[animateData.animateSpriteId].v1 = m_instanceList[animateData.endSpriteId].v1;
}

// Helper function to generate random float between min and max
float PBGfx::gfxGetRandomFloat(float min, float max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

// GFX_ANIM_NORMAL: Linear interpolation without acceleration
void PBGfx::gfxAnimateNormal(stAnimateData& animateData, unsigned int currentTick, float timeSinceStart, float percentComplete) {
    // Calculate the new sprite instance values based on the percent complete, using the masks to only calculate what's needed
    if (animateData.typeMask & ANIMATE_X_MASK) 
        m_instanceList[animateData.animateSpriteId].x = m_instanceList[animateData.startSpriteId].x + 
            (m_instanceList[animateData.endSpriteId].x - m_instanceList[animateData.startSpriteId].x) * percentComplete;
    
    if (animateData.typeMask & ANIMATE_Y_MASK) 
        m_instanceList[animateData.animateSpriteId].y = m_instanceList[animateData.startSpriteId].y + 
            (m_instanceList[animateData.endSpriteId].y - m_instanceList[animateData.startSpriteId].y) * percentComplete;
    
    if (animateData.typeMask & ANIMATE_SCALE_MASK) 
        m_instanceList[animateData.animateSpriteId].scaleFactor = m_instanceList[animateData.startSpriteId].scaleFactor + 
            (m_instanceList[animateData.endSpriteId].scaleFactor - m_instanceList[animateData.startSpriteId].scaleFactor) * percentComplete;
    
    if (animateData.typeMask & ANIMATE_TEXALPHA_MASK) 
        m_instanceList[animateData.animateSpriteId].textureAlpha = m_instanceList[animateData.startSpriteId].textureAlpha + 
            (m_instanceList[animateData.endSpriteId].textureAlpha - m_instanceList[animateData.startSpriteId].textureAlpha) * percentComplete;
    
    if (animateData.typeMask & ANIMATE_COLOR_MASK) {
        m_instanceList[animateData.animateSpriteId].vertRed = m_instanceList[animateData.startSpriteId].vertRed + 
            (m_instanceList[animateData.endSpriteId].vertRed - m_instanceList[animateData.startSpriteId].vertRed) * percentComplete;
        m_instanceList[animateData.animateSpriteId].vertGreen = m_instanceList[animateData.startSpriteId].vertGreen + 
            (m_instanceList[animateData.endSpriteId].vertGreen - m_instanceList[animateData.startSpriteId].vertGreen) * percentComplete;
        m_instanceList[animateData.animateSpriteId].vertBlue = m_instanceList[animateData.startSpriteId].vertBlue + 
            (m_instanceList[animateData.endSpriteId].vertBlue - m_instanceList[animateData.startSpriteId].vertBlue) * percentComplete;
        m_instanceList[animateData.animateSpriteId].vertAlpha = m_instanceList[animateData.startSpriteId].vertAlpha + 
            (m_instanceList[animateData.endSpriteId].vertAlpha - m_instanceList[animateData.startSpriteId].vertAlpha) * percentComplete;
    }
    
    if (animateData.typeMask & ANIMATE_U_MASK) 
        m_instanceList[animateData.animateSpriteId].u1 = m_instanceList[animateData.startSpriteId].u1 + 
            (m_instanceList[animateData.endSpriteId].u1 - m_instanceList[animateData.startSpriteId].u1) * percentComplete;
    
    if (animateData.typeMask & ANIMATE_V_MASK) 
        m_instanceList[animateData.animateSpriteId].v1 = m_instanceList[animateData.startSpriteId].v1 + 
            (m_instanceList[animateData.endSpriteId].v1 - m_instanceList[animateData.startSpriteId].v1) * percentComplete;

    // Rotate is a special case. Depending on rotateClockwise bool, we should rotate clockwise or counter clockwise and make sure to roll over at 360 degrees
    if (animateData.typeMask & ANIMATE_ROTATE_MASK) {
        if (animateData.rotateClockwise) {
            m_instanceList[animateData.animateSpriteId].rotateDegrees = m_instanceList[animateData.startSpriteId].rotateDegrees - 
                (m_instanceList[animateData.endSpriteId].rotateDegrees - m_instanceList[animateData.startSpriteId].rotateDegrees) * percentComplete;
            if (m_instanceList[animateData.animateSpriteId].rotateDegrees > 360.0f) 
                m_instanceList[animateData.animateSpriteId].rotateDegrees -= 360.0f;
        }
        else {
            m_instanceList[animateData.animateSpriteId].rotateDegrees = m_instanceList[animateData.startSpriteId].rotateDegrees + 
                (m_instanceList[animateData.endSpriteId].rotateDegrees - m_instanceList[animateData.startSpriteId].rotateDegrees) * percentComplete;
            if (m_instanceList[animateData.animateSpriteId].rotateDegrees < 0.0f) 
                m_instanceList[animateData.animateSpriteId].rotateDegrees += 360.0f;
        }
    }
}

// GFX_ANIM_ACCL: Animation with acceleration for X, Y, and Rotation
// In this mode, acceleration and initial velocity determine when destination is reached
// Duration is ignored - each axis stops when it reaches its target
void PBGfx::gfxAnimateAcceleration(stAnimateData& animateData, unsigned int currentTick, float timeSinceStart) {
    bool xComplete = true;
    bool yComplete = true;
    bool rotComplete = true;
    
    // For X position with acceleration
    if (animateData.typeMask & ANIMATE_X_MASK) {
        int startX = m_instanceList[animateData.startSpriteId].x;
        int endX = m_instanceList[animateData.endSpriteId].x;
        int currentX = m_instanceList[animateData.animateSpriteId].x;
        
        // Check if we've reached the target
        bool movingRight = (endX > startX);
        bool reachedTarget = movingRight ? (currentX >= endX) : (currentX <= endX);
        
        if (!reachedTarget) {
            xComplete = false;
            
            // Calculate position: x = x0 + v0*t + 0.5*a*t^2
            float newX = (float)startX + 
                         animateData.initialVelocityX * timeSinceStart + 
                         0.5f * animateData.accelPixelPerSecX * timeSinceStart * timeSinceStart;
            
            // Clamp to end position if we've passed it
            if (movingRight && newX >= endX) {
                newX = (float)endX;
            } else if (!movingRight && newX <= endX) {
                newX = (float)endX;
            }
            
            m_instanceList[animateData.animateSpriteId].x = (int)newX;
            
            // Update current velocity: v = v0 + a*t
            animateData.currentVelocityX = animateData.initialVelocityX + animateData.accelPixelPerSecX * timeSinceStart;
        }
    }
    
    // For Y position with acceleration
    if (animateData.typeMask & ANIMATE_Y_MASK) {
        int startY = m_instanceList[animateData.startSpriteId].y;
        int endY = m_instanceList[animateData.endSpriteId].y;
        int currentY = m_instanceList[animateData.animateSpriteId].y;
        
        // Check if we've reached the target
        bool movingDown = (endY > startY);
        bool reachedTarget = movingDown ? (currentY >= endY) : (currentY <= endY);
        
        if (!reachedTarget) {
            yComplete = false;
            
            // Calculate position: y = y0 + v0*t + 0.5*a*t^2
            float newY = (float)startY + 
                         animateData.initialVelocityY * timeSinceStart + 
                         0.5f * animateData.accelPixelPerSecY * timeSinceStart * timeSinceStart;
            
            // Clamp to end position if we've passed it
            if (movingDown && newY >= endY) {
                newY = (float)endY;
            } else if (!movingDown && newY <= endY) {
                newY = (float)endY;
            }
            
            m_instanceList[animateData.animateSpriteId].y = (int)newY;
            
            // Update current velocity: v = v0 + a*t
            animateData.currentVelocityY = animateData.initialVelocityY + animateData.accelPixelPerSecY * timeSinceStart;
        }
    }
    
    // For rotation with acceleration
    if (animateData.typeMask & ANIMATE_ROTATE_MASK) {
        float startDeg = m_instanceList[animateData.startSpriteId].rotateDegrees;
        float endDeg = m_instanceList[animateData.endSpriteId].rotateDegrees;
        float currentDeg = m_instanceList[animateData.animateSpriteId].rotateDegrees;
        
        // Check if we've reached the target
        bool rotatingCW = (endDeg > startDeg);
        bool reachedTarget = rotatingCW ? (currentDeg >= endDeg) : (currentDeg <= endDeg);
        
        // Special case: If start == end AND there's acceleration/velocity, allow free spinning
        // This is for explicit physics-based rotation (not random jumps that happen to match)
        bool hasFreeRotation = (startDeg == endDeg) && 
                               (animateData.accelDegPerSec != 0.0f || animateData.initialVelocityDeg != 0.0f);
        
        // Only check completion if we have a target (not free rotation)
        if (!hasFreeRotation && !reachedTarget) {
            rotComplete = false;
        }
        
        // Always update rotation (even if target reached, to avoid glitches on final frame)
        if (!reachedTarget || hasFreeRotation) {
            // Calculate rotation: deg = deg0 + v0*t + 0.5*a*t^2
            float newDeg = startDeg + 
                           animateData.initialVelocityDeg * timeSinceStart + 
                           0.5f * animateData.accelDegPerSec * timeSinceStart * timeSinceStart;
            
            // Clamp to end rotation if we've passed it (only when we have a target)
            if (!hasFreeRotation) {
                if (rotatingCW && newDeg >= endDeg) {
                    newDeg = endDeg;
                } else if (!rotatingCW && newDeg <= endDeg) {
                    newDeg = endDeg;
                }
            }
            
            // Update current velocity: v = v0 + a*t
            animateData.currentVelocityDeg = animateData.initialVelocityDeg + animateData.accelDegPerSec * timeSinceStart;
            
            // Handle wrapping - use fmod to avoid infinite loops
            newDeg = fmod(newDeg, 360.0f);
            if (newDeg < 0.0f) newDeg += 360.0f;
            
            m_instanceList[animateData.animateSpriteId].rotateDegrees = newDeg;
        }
    }
    
    // Mark animation as complete if all axes have reached their targets
    if (xComplete && yComplete && rotComplete) {
        animateData.isActive = false;
    }
}

// GFX_ANIM_JUMP: Jump from start to end instance when time expires
void PBGfx::gfxAnimateJump(stAnimateData& animateData, unsigned int currentTick, float timeSinceStart) {
    // Check if it's time to jump
    float percentComplete = timeSinceStart / animateData.animateTimeSec;
    
    if (percentComplete >= 1.0f) {
        // Time to jump - set to end instance values for all animated properties
        if (animateData.typeMask & ANIMATE_X_MASK) {
            m_instanceList[animateData.animateSpriteId].x = m_instanceList[animateData.endSpriteId].x;
        }
        
        if (animateData.typeMask & ANIMATE_Y_MASK) {
            m_instanceList[animateData.animateSpriteId].y = m_instanceList[animateData.endSpriteId].y;
        }
        
        if (animateData.typeMask & ANIMATE_SCALE_MASK) {
            m_instanceList[animateData.animateSpriteId].scaleFactor = m_instanceList[animateData.endSpriteId].scaleFactor;
        }
        
        if (animateData.typeMask & ANIMATE_ROTATE_MASK) {
            m_instanceList[animateData.animateSpriteId].rotateDegrees = m_instanceList[animateData.endSpriteId].rotateDegrees;
        }
        
        if (animateData.typeMask & ANIMATE_TEXALPHA_MASK) {
            m_instanceList[animateData.animateSpriteId].textureAlpha = m_instanceList[animateData.endSpriteId].textureAlpha;
        }
        
        if (animateData.typeMask & ANIMATE_COLOR_MASK) {
            m_instanceList[animateData.animateSpriteId].vertRed = m_instanceList[animateData.endSpriteId].vertRed;
            m_instanceList[animateData.animateSpriteId].vertGreen = m_instanceList[animateData.endSpriteId].vertGreen;
            m_instanceList[animateData.animateSpriteId].vertBlue = m_instanceList[animateData.endSpriteId].vertBlue;
            m_instanceList[animateData.animateSpriteId].vertAlpha = m_instanceList[animateData.endSpriteId].vertAlpha;
        }
        
        if (animateData.typeMask & ANIMATE_U_MASK) {
            m_instanceList[animateData.animateSpriteId].u1 = m_instanceList[animateData.endSpriteId].u1;
        }
        
        if (animateData.typeMask & ANIMATE_V_MASK) {
            m_instanceList[animateData.animateSpriteId].v1 = m_instanceList[animateData.endSpriteId].v1;
        }
        
        // Note: Do NOT reset startTick here - let the loop handling code in gfxAnimateSprite handle it
        // Otherwise it interferes with loop completion detection
    }
}

// GFX_ANIM_JUMPRANDOM: Randomly decide to jump based on randomPercent
void PBGfx::gfxAnimateJumpRandom(stAnimateData& animateData, unsigned int currentTick, float timeSinceStart) {
    // Check if it's time to potentially jump
    float percentComplete = timeSinceStart / animateData.animateTimeSec;
    
    if (percentComplete >= 1.0f) {
        // Time to check if we should jump - compare randomPercent to a random value
        float randomCheck = gfxGetRandomFloat(0.0f, 1.0f);
        
        if (randomCheck <= animateData.randomPercent) {
            // Jump! Pick random values between start and end for all animated properties
            if (animateData.typeMask & ANIMATE_SCALE_MASK) {
                float startScale = m_instanceList[animateData.startSpriteId].scaleFactor;
                float endScale = m_instanceList[animateData.endSpriteId].scaleFactor;
                float newScale = startScale + gfxGetRandomFloat(0.0f, 1.0f) * (endScale - startScale);
                m_instanceList[animateData.animateSpriteId].scaleFactor = newScale;
            }
            
            if (animateData.typeMask & ANIMATE_X_MASK) {
                int startX = m_instanceList[animateData.startSpriteId].x;
                int endX = m_instanceList[animateData.endSpriteId].x;
                int minX = std::min(startX, endX);
                int maxX = std::max(startX, endX);
                m_instanceList[animateData.animateSpriteId].x = minX + (int)(gfxGetRandomFloat(0.0f, 1.0f) * (maxX - minX));
            }
            
            if (animateData.typeMask & ANIMATE_Y_MASK) {
                int startY = m_instanceList[animateData.startSpriteId].y;
                int endY = m_instanceList[animateData.endSpriteId].y;
                int minY = std::min(startY, endY);
                int maxY = std::max(startY, endY);
                m_instanceList[animateData.animateSpriteId].y = minY + (int)(gfxGetRandomFloat(0.0f, 1.0f) * (maxY - minY));
            }
            
            if (animateData.typeMask & ANIMATE_SCALE_MASK) {
                float startScale = m_instanceList[animateData.startSpriteId].scaleFactor;
                float endScale = m_instanceList[animateData.endSpriteId].scaleFactor;
                m_instanceList[animateData.animateSpriteId].scaleFactor = startScale + gfxGetRandomFloat(0.0f, 1.0f) * (endScale - startScale);
            }
            
            if (animateData.typeMask & ANIMATE_ROTATE_MASK) {
                float startDeg = m_instanceList[animateData.startSpriteId].rotateDegrees;
                float endDeg = m_instanceList[animateData.endSpriteId].rotateDegrees;
                m_instanceList[animateData.animateSpriteId].rotateDegrees = startDeg + gfxGetRandomFloat(0.0f, 1.0f) * (endDeg - startDeg);
            }
            
            if (animateData.typeMask & ANIMATE_TEXALPHA_MASK) {
                float startAlpha = m_instanceList[animateData.startSpriteId].textureAlpha;
                float endAlpha = m_instanceList[animateData.endSpriteId].textureAlpha;
                m_instanceList[animateData.animateSpriteId].textureAlpha = startAlpha + gfxGetRandomFloat(0.0f, 1.0f) * (endAlpha - startAlpha);
            }
            
            if (animateData.typeMask & ANIMATE_COLOR_MASK) {
                float startRed = m_instanceList[animateData.startSpriteId].vertRed;
                float endRed = m_instanceList[animateData.endSpriteId].vertRed;
                m_instanceList[animateData.animateSpriteId].vertRed = startRed + gfxGetRandomFloat(0.0f, 1.0f) * (endRed - startRed);
                
                float startGreen = m_instanceList[animateData.startSpriteId].vertGreen;
                float endGreen = m_instanceList[animateData.endSpriteId].vertGreen;
                m_instanceList[animateData.animateSpriteId].vertGreen = startGreen + gfxGetRandomFloat(0.0f, 1.0f) * (endGreen - startGreen);
                
                float startBlue = m_instanceList[animateData.startSpriteId].vertBlue;
                float endBlue = m_instanceList[animateData.endSpriteId].vertBlue;
                m_instanceList[animateData.animateSpriteId].vertBlue = startBlue + gfxGetRandomFloat(0.0f, 1.0f) * (endBlue - startBlue);
                
                float startAlpha = m_instanceList[animateData.startSpriteId].vertAlpha;
                float endAlpha = m_instanceList[animateData.endSpriteId].vertAlpha;
                m_instanceList[animateData.animateSpriteId].vertAlpha = startAlpha + gfxGetRandomFloat(0.0f, 1.0f) * (endAlpha - startAlpha);
            }
            
            if (animateData.typeMask & ANIMATE_U_MASK) {
                float startU = m_instanceList[animateData.startSpriteId].u1;
                float endU = m_instanceList[animateData.endSpriteId].u1;
                m_instanceList[animateData.animateSpriteId].u1 = startU + gfxGetRandomFloat(0.0f, 1.0f) * (endU - startU);
            }
            
            if (animateData.typeMask & ANIMATE_V_MASK) {
                float startV = m_instanceList[animateData.startSpriteId].v1;
                float endV = m_instanceList[animateData.endSpriteId].v1;
                m_instanceList[animateData.animateSpriteId].v1 = startV + gfxGetRandomFloat(0.0f, 1.0f) * (endV - startV);
            }
        }
        
        // Reset timer for next check (handled by loop logic in main function)
        animateData.startTick = currentTick;
    }
}

// Update a video texture with new frame data
bool PBGfx::gfxUpdateVideoTexture(unsigned int spriteId, const uint8_t* frameData, unsigned int width, unsigned int height) {
    
    auto it = m_instanceList.find(spriteId);
    if (it == m_instanceList.end()) {
        return false;
    }
    
    // Check if this is a video texture type
    if (m_spriteList[it->second.parentSpriteId].textureType != GFX_VIDEO) {
        return false;
    }
    
    // Check if texture is loaded
    if (!m_spriteList[it->second.parentSpriteId].isLoaded) {
        return false;
    }
    
    // Verify dimensions match
    if (width != m_spriteList[it->second.parentSpriteId].baseWidth || 
        height != m_spriteList[it->second.parentSpriteId].baseHeight) {
        return false;
    }
    
    // Update the OpenGL texture with new frame data
    return oglUpdateTexture(m_spriteList[it->second.parentSpriteId].glTextureId, frameData, width, height);
}

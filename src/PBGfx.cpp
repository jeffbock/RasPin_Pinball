
// PBGfx contains all the fundamental graphics management operations and sits on top of PGOGLES
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#include "PBGfx.h"

// Constructor
PBGfx::PBGfx() {
    
    m_nextSystemSpriteId = 1;
    m_nextUserSpriteId = 100;
    m_systemSpriteId = NOSPRITE;
}

// Destructor
PBGfx::~PBGfx() {
    // Cleanup code if needed
}

// Any Gfx specific initialization code
bool PBGfx::gfxInit(){

    // Create the system font sprite
    m_systemSpriteId = gfxSysLoadSprite({"System Font", SYSTEMFONTSPRITE, GFX_PNG, GFX_TEXTMAP, GFX_UPPERLEFT, true, true}, true);

    if (m_systemSpriteId == NOSPRITE) return (false);
    else return (true);
}


// Private function to create a sprite
unsigned int PBGfx::gfxSysLoadSprite(stSpriteInfo spriteInfo, bool bSystem) {
    
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
        default: return (NOSPRITE);
    }

    // If the texture file name is not empty, load the texture
    if ((!spriteInfo.textureFileName.empty()) && (spriteInfo.useTexture)) {
        texture = oglLoadTexture(spriteInfo.textureFileName.c_str(), textureType, &width, &height);
        if (texture == 0) return (NOSPRITE);
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

    // If the sprite is a text sprite, load the JSON UV map 
    
    if ((spriteInfo.mapType == GFX_TEXTMAP) || (spriteInfo.mapType == GFX_SPRITEMAP)) {
        // Create a file name that has the same name as filename as the texutre (minus the extension), and change extension to .json
        std::string jsonFileName = spriteInfo.textureFileName;
        jsonFileName = jsonFileName.substr(0, jsonFileName.find_last_of(".")) + ".json";

        json uvJson;

        // Use Json.hpp functions to load the JSON file and put it in a map structure with the key being the character
        std::ifstream jsonFile(jsonFileName);
        jsonFile >> uvJson;

        unsigned int test = 0;

        if (spriteInfo.mapType == GFX_TEXTMAP) m_textMapJSON[spriteId] = uvJson;
        else m_spriteMapJSON[spriteId] = uvJson;

        // test = m_textMapJSON[spriteId]["A"]["width"];

    }
    
    spriteInfo.isLoaded = true;

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
    instance.v1 = 0.0f;
    instance.u2 = 1.0f;
    instance.v2 = 1.0f;
    instance.textureAlpha = 1.0f;
    instance.vertRed = 1.0f; instance.vertGreen = 1.0f; instance.vertBlue = 1.0f; instance.vertAlpha = 1.0f;
    instance.scaleFactor = 1.0f;
    instance.rotateDegrees = 0.0f;
    instance.glTextureId = texture;
    instance.updateBoundingBox = false;
    instance.boundingBox = {0, 0, 0, 0};

    m_instanceList[spriteId] = instance;

    return spriteId;
}

// Create an instance sprite - can be created from any instance sprite.  
unsigned int PBGfx::gfxInstanceSprite (unsigned int parentSpriteId, unsigned int x, unsigned int y, unsigned int textureAlpha, 
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

        // Cannont make instance sprites from font sprites or sprites with a map (eg: animated sprites)
        if (m_spriteList[it->second.parentSpriteId].mapType != GFX_NOMAP) return (NOSPRITE);

        unsigned int spriteId = m_nextUserSpriteId;
        m_nextUserSpriteId++;

        // This will always point to a sprite ID that has a backing stSpriteInfo struct, never an instance only sprite.
        // First creation of a sprite will have the same ID for both the sprite and the instance
        instance.parentSpriteId = it->second.parentSpriteId;
        instance.glTextureId = it->second.glTextureId;
        instance.updateBoundingBox = it->second.updateBoundingBox;
        instance.width = it->second.width;
        instance.height = it->second.height;

        m_instanceList[spriteId] = instance;

        return spriteId;
    }
    else return (NOSPRITE);
}

// Will load all texture and key info for a base sprite and create the 1st instance of the sprite
unsigned int PBGfx::gfxLoadSprite(const std::string& spriteName, const std::string& textureFileName, gfxTexType textureType,
                                  gfxSpriteMap mapType, gfxTexCenter textureCenter, bool keepResident, bool useTexture) {

    stSpriteInfo spriteInfo;

    spriteInfo.spriteName = spriteName;
    spriteInfo.textureFileName = textureFileName;
    spriteInfo.textureType = textureType;
    spriteInfo.mapType = mapType;
    spriteInfo.textureCenter = textureCenter;
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
bool PBGfx::gfxRenderSprite(unsigned int spriteId, unsigned int x, unsigned int y){
            
    auto it = m_instanceList.find(spriteId);
    if (it != m_instanceList.end()) {

        m_instanceList[spriteId].x = x;
        m_instanceList[spriteId].y = y;

        return gfxRenderSprite(spriteId);
    }

    return (false);
}

bool PBGfx::gfxRenderSprite(unsigned int spriteId, unsigned int x, unsigned int y, float scaleFactor, float rotateDegrees){

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

        // Use alpha if the texture is a BMP (eg: use the the supplied alpha value, otherwise it is assumed PNG already has alpha)
        bool useTexAlpha = m_spriteList[it->second.parentSpriteId].textureType == GFX_BMP ? true : false;

        // Change the textureID if 
        unsigned int tempTextureId = it->second.glTextureId;
        if (!m_spriteList[it->second.parentSpriteId].useTexture) tempTextureId = 0;

        // Render the sprite quad
        oglRenderQuad(&x1, &y1, &x2, &y2, it->second.u1, it->second.v1, it->second.u2, it->second.v2, useCenter, useTexAlpha, it->second.textureAlpha, tempTextureId, it->second.vertRed, it->second.vertGreen, it->second.vertBlue, it->second.vertAlpha, it->second.scaleFactor, it->second.rotateDegrees, it->second.updateBoundingBox);
            
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

bool PBGfx::gfxRenderString(unsigned int spriteId, std::string input, unsigned int x, unsigned int y){

    unsigned int width=0, height=0;
    float U1=0.0f, V1=0.0f, U2 = 1.0f, V2 = 1.0f;

    // For each character in the string, set up the rendering info and call the render sprite function
    for (unsigned int i = 0; i < input.length(); i++) {
        unsigned int charId = (unsigned int)input[i];

        // if the character isn't the the standard ASCII range, then skip it
        if (charId < 32 || charId > 126) continue;

        // Convert the character to a string
        std::string charString = std::string(1, input[i]);

        // Check if the character exists in the JSON object
        if (m_textMapJSON[spriteId].find(charString) == m_textMapJSON[spriteId].end()) {
            std::cerr << "Error: character " << charString << " not found in JSON" << std::endl;
            continue;
        }

        // Get the width of character "A" from the font JSON
        width = m_textMapJSON[spriteId]["A"]["width"];

        // test = m_textMapJSON[spriteId]["A"]["width"];

        // Get the the required info from the font JSON
        width = m_textMapJSON[spriteId]["T"]["width"];
        width = m_textMapJSON[spriteId][charString]["width"];
        height = m_textMapJSON[spriteId][charString]["height"];
        U1 = m_textMapJSON[spriteId][charString]["U1"];
        V1 = m_textMapJSON[spriteId][charString]["V1"];
        U2 = m_textMapJSON[spriteId][charString]["U2"];
        V2 = m_textMapJSON[spriteId][charString]["V2"];

        // Find the sprite ID in the m_instanceList, if it isn't a textmap then return false
        auto it2 = m_instanceList.find(spriteId);
        if (it2 == m_instanceList.end()) return (false);
        if (m_spriteList[it2->second.parentSpriteId].mapType != GFX_TEXTMAP) return (false);

        // Set the relevant values in the sprite instance
        m_instanceList[spriteId].width = width;
        m_instanceList[spriteId].height = height;
        m_instanceList[spriteId].x = x;
        m_instanceList[spriteId].y = y;

        // Render the sprite instance using gfxRenderSprite and the parameters from the sprite instance
        gfxRenderSprite(spriteId);
        
        // Move the x coordinate to the right for the next character
        x += width;
    }

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
unsigned int PBGfx::gfxSetXY(unsigned int spriteId, unsigned int X, unsigned int Y, bool addXY){
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

// Query function for XY
unsigned int PBGfx::gfxGetXY(unsigned int spriteId, unsigned int* X, unsigned int* Y){
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
unsigned int PBGfx::gfxGetSystemSpriteId(){
    return m_systemSpriteId;
}
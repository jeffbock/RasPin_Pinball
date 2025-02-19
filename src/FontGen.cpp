// FontGen.cpp - utility for generating a font texture and UV map from a TrueType font file
// The PNG and JSON file can be loaded by PBGfx to load a font and render it on-screen
// Usage: FontGen <font_file.ttf> <font_size> [<buffer_size>]
// Example: FontGen Arial.ttf 24 512
//  <font_file.ttf> - the TrueType font file to use
//  <font_size> - the size of the font in pixels
//  [<buffer_size>] - the size of the texture buffer (default is 256)

// FontGen.cpp - Font generator for support in the PBGfx library
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <cstring>
#include "stb_truetype.h"
#include "stb_image_write.h"
#include "json.hpp"

using json = nlohmann::json;

struct UVRect {
    float u1, v1, u2, v2;
    int width, height;
};

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <font_file.ttf> <font_size> [<buffer_size>]" << std::endl;
        return 1;
    }

    const char* fontFile = argv[1];
    int fontSize = std::stoi(argv[2]);
    int textureSize = (argc == 4) ? std::stoi(argv[3]) : 256;

    // Load the font
    std::ifstream fontStream(fontFile, std::ios::binary);
    if (!fontStream) {
        std::cerr << "Error: Unable to open font file " << fontFile << std::endl;
        return 1;
    }

    std::vector<unsigned char> fontData((std::istreambuf_iterator<char>(fontStream)), std::istreambuf_iterator<char>());
    fontStream.close();

    // Initialize stb_truetype
    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, fontData.data(), stbtt_GetFontOffsetForIndex(fontData.data(), 0))) {
        std::cerr << "Error: Unable to initialize font" << std::endl;
        return 1;
    }

    // Calculate font metrics
    int ascent, descent, lineGap;
    float scale = stbtt_ScaleForPixelHeight(&font, fontSize);
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
    ascent = static_cast<int>(ascent * scale);
    descent = static_cast<int>(descent * scale);

    // Create a memory buffer with 8/8/8/8 format
    std::vector<unsigned char> buffer(textureSize * textureSize * 4, 0);

    std::map<char, UVRect> uvMap;
    int x = 0, y = ascent, maxCharHeight = ascent - descent;

    for (char c = 32; c < 127; ++c) {
        int width, height, xOffset, yOffset;
        unsigned char* bitmap = stbtt_GetCodepointBitmap(&font, 0, scale, c, &width, &height, &xOffset, &yOffset);

        if (x + width + 2 > textureSize) {
            x = 0; 
            y += maxCharHeight + 2;
        }

        if (y + maxCharHeight + 2 > textureSize) {
            std::cerr << "Error: Not enough space in the texture buffer" << std::endl;
            stbtt_FreeBitmap(bitmap, nullptr);
            return 1;
        }

        int baseline = y + ascent;

        for (int j = 0; j < height; ++j) {
            for (int i = 0; i < width; ++i) {
                int bufferIndex = ((baseline + yOffset + j) * textureSize + (x + i)) * 4;
                unsigned char alpha = bitmap[j * width + i];
                buffer[bufferIndex] = 255; // R
                buffer[bufferIndex + 1] = 255; // G
                buffer[bufferIndex + 2] = 255; // B
                buffer[bufferIndex + 3] = alpha; // A
            }
        }

        // Add the character to the UV map - the (-2) for the u1 value is a bit of a hack to get the full descent - it may not be right for all fonts...
        uvMap[c] = { x / (float)textureSize, (y - descent - 2) / (float)textureSize, (x + width) / (float)textureSize, (y + maxCharHeight) / (float)textureSize, width, maxCharHeight };

        x += width + 2;

        stbtt_FreeBitmap(bitmap, nullptr);
    }

    // Save the buffer as a PNG file
    std::string baseFileName = std::string(fontFile).substr(0, std::string(fontFile).find_last_of('.'));
    std::string outputFileName = baseFileName + "_" + std::to_string(fontSize) + "_" + std::to_string(textureSize) + ".png";
    if (!stbi_write_png(outputFileName.c_str(), textureSize, textureSize, 4, buffer.data(), textureSize * 4)) {
        std::cerr << "Error: Unable to save PNG file " << outputFileName << std::endl;
        return 1;
    }

    std::cout << "Font texture saved as " << outputFileName << std::endl;

    // Save the UV map as a JSON file
    json uvJson;
    for (auto it = uvMap.begin(); it != uvMap.end(); ++it) {
        uvJson[std::string(1, it->first)] = {
            {"u1", it->second.u1},
            {"v1", it->second.v1},
            {"u2", it->second.u2},
            {"v2", it->second.v2},
            {"width", it->second.width},
            {"height", it->second.height}
        };
    }

    std::string jsonFileName = baseFileName + "_" + std::to_string(fontSize) + "_" + std::to_string(textureSize) + ".json";
    std::ofstream jsonFile(jsonFileName);
    if (!jsonFile) {
        std::cerr << "Error: Unable to save JSON file " << jsonFileName << std::endl;
        return 1;
    }

    jsonFile << uvJson.dump(4);
    jsonFile.close();

    std::cout << "Character map saved as " << jsonFileName << std::endl;

    return 0;
}
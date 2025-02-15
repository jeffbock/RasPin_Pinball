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
};

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <font_file.ttf> <font_size (in pixels)> [<PNG pixel X/Y size>]" << std::endl;
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

    // Create a memory buffer with 8/8/8/8 format
    std::vector<unsigned char> buffer(textureSize * textureSize * 4, 0);

    std::map<char, UVRect> uvMap;
    int x = 0, y = 0, maxHeight = 0;

    float scale = stbtt_ScaleForPixelHeight(&font, fontSize);

    for (char c = 32; c < 127; ++c) {
        int width, height, xOffset, yOffset;
        unsigned char* bitmap = stbtt_GetCodepointBitmap(&font, 0, scale, c, &width, &height, &xOffset, &yOffset);

        if (x + width > textureSize) {
            x = 0;
            y += maxHeight;
            maxHeight = 0;
        }

        if (y + height > textureSize) {
            std::cerr << "Error: Not enough space in the texture buffer" << std::endl;
            stbtt_FreeBitmap(bitmap, nullptr);
            return 1;
        }

        for (int j = 0; j < height; ++j) {
            for (int i = 0; i < width; ++i) {
                int bufferIndex = ((y + j) * textureSize + (x + i)) * 4;
                unsigned char alpha = bitmap[j * width + i];
                buffer[bufferIndex] = 255; // R
                buffer[bufferIndex + 1] = 255; // G
                buffer[bufferIndex + 2] = 255; // B
                buffer[bufferIndex + 3] = alpha; // A
            }
        }

        uvMap[c] = { x / (float)textureSize, y / (float)textureSize, (x + width) / (float)textureSize, (y + height) / (float)textureSize };

        x += width;
        if (height > maxHeight) maxHeight = height;

        stbtt_FreeBitmap(bitmap, nullptr);
    }

    // Save the buffer as a PNG file
    std::string baseFileName = std::string(fontFile).substr(0, std::string(fontFile).find_last_of('.'));
    std::string outputFileName = baseFileName + "_" + std::to_string(fontSize) + ".png";
    if (!stbi_write_png(outputFileName.c_str(), textureSize, textureSize, 4, buffer.data(), textureSize * 4)) {
        std::cerr << "Error: Unable to save PNG file " << outputFileName << std::endl;
        return 1;
    }

    std::cout << "Font texture saved as " << outputFileName << std::endl;

    // Save the UV map as a JSON file
    json uvJson;
    for (auto it = uvMap.begin(); it != uvMap.end(); ++it) {
        uvJson[std::string(1, it->first)] = { it->second.u1, it->second.v1, it->second.u2, it->second.v2 };
    }

    std::string jsonFileName = baseFileName + "_UVMap_" + std::to_string(fontSize) + ".json";
    std::ofstream jsonFile(jsonFileName);
    if (!jsonFile) {
        std::cerr << "Error: Unable to save JSON file " << jsonFileName << std::endl;
        return 1;
    }

    jsonFile << uvJson.dump(4);
    jsonFile.close();

    std::cout << "UV map saved as " << jsonFileName << std::endl;

    return 0;
}
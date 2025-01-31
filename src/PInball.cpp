// Main entry point into the pinball .exe
#include "PInball.h"

int main(int argc, char const *argv[])
{
    // create a new instance of the pinball class
    //PInball pinball;

    // run the pinball game
    //pinball.run();

    // return 0 to indicate success
    return 0;
}

// Sample code for using the schrift library
/*

// Load the font
struct SFT sft;
sft = (struct SFT){
    .xScale = 16,
    .yScale = 16,
    .flags = SFT_DOWNWARD_Y,
    .font = schrift_load("path/to/font.ttf")
};

// Render the Glyph
struct SFT_Glyph glyph;
schrift_lookup(&sft, 'A', &glyph);

// Get the bitmap of the glyph
struct SFT_Image image;
schrift_render(&sft, &glyph, &image);

*/
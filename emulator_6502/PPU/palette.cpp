//
//  palette.cpp
//  emulator_6502
//
//  Created by Kyle Chiem on 1/11/25.
//

#include "palette.hpp"

Palette::Palette(const char* filename)
{
    loadPaletteFile(filename);
}

void Palette::loadPaletteFile(const char* filename)
{
    FILE* file = fopen(filename, "rb");
    if (!file)
    {
        perror("Error opening file");
        return;
    }
    
    struct RGBField palette[64];
    auto read = fread(palette, sizeof(RGBField), 64, file);
    
    if (read != 64) // .pal always reads 64 lines (64 colors)
    {
        perror("Invalid palette file!");
        fclose(file);
        return;
    }
    
    m_COLOR_PALETTE = std::to_array(palette);
    fclose(file);
}

const std::array<RGBField, 64> Palette::getPalette() const
{
    return m_COLOR_PALETTE;
}

//
//  palette.hpp
//  emulator_6502
//
//  Created by Kyle Chiem on 1/11/25.
//

#pragma once

#include <stdint.h>
#include <array>

struct RGBField
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class Palette
{
    std::array<RGBField, 64> m_COLOR_PALETTE;
    
public:
    
    Palette(const char* filename);
    
    /**
     *  Loads a color palette file into the color palette variable.
     *
     *  @param filename File which to read the palette from. Must be a .pal file.
     */
    void loadPaletteFile(const char* filename);
    
    /// Get the color palette
    const std::array<RGBField, 64> getPalette() const;
};

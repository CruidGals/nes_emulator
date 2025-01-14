//
//  loader.hpp
//  emulator_6502
//
//  Created by Kyle Chiem on 1/13/25.
//

#pragma once

#include <iostream>
#include <fstream>
#include <string>

#include "rom_params.hpp"

class Loader
{
    
    // Store the open file in variable to easy access
    std::ifstream file;
    
    struct Header m_header;
    struct RomData m_romData;
    
public:
    
    // Constructor
    Loader(const char* filename);
    
    int getPrgRomSize() const;
    int getChrRomSize() const;
    
    bool successfulLoad;
    
private:
    
    // General func for the get__RomSize:
    int getRomSize(uint16_t romSize, uint8_t sizeMultiplier, bool exponent) const;
    
    // This has to be private to disallow access from other files.
    void loadHeader();
    void loadRomData();
    
    // Ease of life functions
    uint8_t readByte();
};

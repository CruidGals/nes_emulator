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
    
    std::unique_ptr<Header> m_header;
    std::unique_ptr<RomData> m_romData;
    
    bool m_romLoaded;
    
public:
    
    // Constructor
    Loader();
    
    int getPrgRomSize() const;
    int getChrRomSize() const;
    
    const bool isLoaded() const;
    
    void loadRom(const char* filename);
    void clearRom();
    
private:
    
    // General func for the get__RomSize:
    int getRomSize(uint16_t romSize, uint8_t sizeMultiplier, bool exponent) const;
    
    // This has to be private to disallow access from other files.
    void loadHeader();
    void loadRomData();
    
    // Ease of life functions
    uint8_t readByte();
};

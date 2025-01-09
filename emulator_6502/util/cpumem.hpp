//
//  cpumem.hpp
//  emulator_6502
//
//  Created by Kyle Chiem on 1/9/25.
//

#pragma once

#include "abstract/memory.h"
#include "../PPU/PPU.hpp"

class CPUMemory : public Memory
{
    PPU* ppu;
    
public:
    
    CPUMemory(PPU* ppu);
    
    uint16_t mirroredAddress(uint16_t address) const override;
    
    uint8_t read(uint16_t address) const override;
    void write(uint16_t address, uint8_t value) const override;
    
};

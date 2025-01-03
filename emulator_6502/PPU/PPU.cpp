//
//  PPU.cpp
//  emulator_6502
//
//  Created by Kyle Chiem on 11/19/24.
//

#include "PPU.hpp"

#include <stdio.h>
#include <stdint.h>
#include <iostream>

PPU::PPU()
{
    memory = static_cast<uint8_t*>(calloc(0x4000, sizeof(uint8_t))); // Allocate 16 KB
}
PPU::~PPU()
{
    free(memory);
}

//Power up function
void PPU::powerResetState(bool isReset)
{
    regs.PPUCTRL.val = 0;
    regs.PPUMASK.val = 0;
    regs.PPUSTATUS.val = (isReset) ? regs.PPUSTATUS.val | 0x80 : 0xa0;
    regs.PPUSCROLL.val = 0;
    regs.PPUDATA = 0;
    
    // Registers that are only changed by Power on events
    if (!isReset)
    {
        regs.OAMADDR = 0;
        regs.PPUADDR.val = 0;
    }
}

// Overload operator[] for read-only access (const)
const uint8_t& PPU::operator[](uint16_t address) const 
{
    if (address >= 0x4000) 
    {
        throw std::out_of_range("Attempted to access beyond PPU memory bounds");
    }
    return memory[address];
}

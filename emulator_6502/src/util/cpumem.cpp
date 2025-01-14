//
//  cpumem.cpp
//  emulator_6502
//
//  Created by Kyle Chiem on 1/9/25.
//

#include "cpumem.hpp"

CPUMemory::CPUMemory(PPU* ppu) : Memory(0xFFFF), ppu(ppu) {}

uint16_t CPUMemory::mirroredAddress(uint16_t address) const
{
    if (address < 0x2000)
        return address % 0x0800; // CPU Ram is mirrored every 2KB
    else if (address < 0x4000)
        return (address % 0x2008); // PPU Memory mirrored every byte, starting at 0x2000
    
    return address;
}

uint8_t CPUMemory::read(uint16_t address) const
{
    if (address >= 0x2000 && address < 0x4000)
        return ppu->read(mirroredAddress(address)); // Specific read functions attached to the PPU
    
    // TODO: Implement specific read side effects for APU
    
    return Memory::read(address); // Access own memory if not a specific address
}

void CPUMemory::write(uint16_t address, uint8_t value) const
{
    if (address >= 0x2000 && address < 0x4000)
        ppu->write(mirroredAddress(address), value); // Specific write functions attached to the PPU
    else
        Memory::write(address, value); // Access own memory if not a specific address
    
    // TODO: Implement specific write side effects for APU
}

//
//  memory.cpp
//  emulator_6502
//
//  Created by Kyle Chiem on 1/7/25.
//

#include "memory.hpp"

/*
 
 Base memory class. Used to implement CPUMemory and PPUMemory
 
 */

Memory::Memory(size_t size): m_data(std::make_unique<uint8_t[]>(size)) {}

uint8_t& Memory::operator[](size_t index)
{
    if (index >= 0xFFFF) {
        throw std::out_of_range("Memory access out of range");
    }
    return m_data[mirroredAddress(index)];
}

// Const access operator
const uint8_t& Memory::operator[](size_t index) const
{
    if (index >= 0xFFFF) {
        throw std::out_of_range("Memory access out of range");
    }
    return m_data[mirroredAddress(index)];
}

uint16_t Memory::mirroredAddress(uint16_t address) const
{
    return address; // Base Class Implementation
}

uint8_t* Memory::getBaseAddress() { return m_data.get(); }


/*
 
 CPU-Specific Memory class
 
 */

CPUMemory::CPUMemory() : Memory(0xFFFF) {}

uint16_t CPUMemory::mirroredAddress(uint16_t address) const
{
    if (address < 0x2000)
        return address % 0x0800; // CPU Ram is mirrored every 2KB
    else if (address < 0x4000)
        return (address % 8) + 0x2000; // PPU Memory mirrored every byte, starting at 0x2000
    
    return address;
}

/*
 
 PPU-Specific Memory class
 
 */

PPUMemory::PPUMemory(NametableMirroring type) : Memory(0xFFFF), mirroringType(type) {}

uint16_t PPUMemory::mirroredAddress(uint16_t address) const
{
    
    if (address >= 0x2000 && address < 0x3000) // Handles nametable mirroring
    {
        switch (mirroringType) {
            case NametableMirroring::SINGLE:
            {
                return (address % 0x0400) + 0x2000;
            }
            case NametableMirroring::HORIZONTAL:
            {
                // Address 0x2000-0x23FF and 0x2400 and 0x27FF mirrored, 0x2800-0x2BFF and 0x2C00-2FFF are mirrored
                return (address < 0x2800) ? (address % 0x0400) + 0x2000 : (address % 0x0400) + 0x2800;
            }
            case NametableMirroring::VERTICAL:
            {
                // If address lies in the mirrored region
                if (address >= 0x2800)
                {
                    // Address 0x2000-0x23FF and 0x2800 and 0x2BFF mirrored, 0x2400-0x27FF and 0x2C00-2FFF are mirrored
                    return (address < 0x2C00) ? (address % 0x0400) + 0x2000 : (address % 0x0400) + 0x2400;
                }
                
                // If address doesn't lie in the mirrored region
                return address;
            }
                
            // Catches 4-screen mirroring type (NametableMirroring::NONE)
            default:
                break;
        }
    }
    else if (address >= 0x3000 && address < 0x3F00)
    {
        // $3000-$3EFF is mirrored from $2000-$2EFF
        return address - 0x1000;
    }
    else if (address >= 0x3F00 && address < 0x4000)
    {
        // $3F00-$3F1F is mirrored every 0x20
        return (address % 0x20) + 0x3F00;
    }
    
    return address;
}

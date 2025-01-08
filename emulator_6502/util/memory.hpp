//
//  memory.hpp
//  emulator_6502
//
//  Created by Kyle Chiem on 1/7/25.
//

#pragma once

#include <stdint.h>
#include <string>

class Memory
{
    std::unique_ptr<uint8_t[]> data;
    
public:
    Memory(size_t size);
    
    // Access operator
    uint8_t& operator[](size_t index);

    // Const access operator
    const uint8_t& operator[](size_t index) const;
    
    /*
     Memory mirroring mimicker:
     It mimics the act of memory mirroring by returning the mirrored value from the base position than actual position
     
     For example:
        CPU RAM memory is mirrored every 2KB (0x0000 - 0x07FF, 0x0800 - 0x0FFF , ... , 0x17FF - 0x1FFF)
        A read to address 0x1FFF will yield the value of 0x07FF
        A read to address 0x0800 will yield the value of 0x0000, etc.
     
     Same for PPU Nametable memory
     */
    virtual uint16_t mirroredAddress(uint16_t address) const;
    
    /*
     Helper function that grabs the memory address at data[0]
     */
    uint8_t* getBaseAddress();
};

/*
 
 CPU-Specific Memory class
 
 */
class CPUMemory : public Memory
{
    
public:
    
    CPUMemory();
    
    uint16_t mirroredAddress(uint16_t address) const override;
};

/*
 
 PPU-Specific Memory class
 
 */

enum class NametableMirroring { NONE, SINGLE, HORIZONTAL, VERTICAL };

class PPUMemory : public Memory
{

public:
    PPUMemory(NametableMirroring type);

    uint16_t mirroredAddress(uint16_t address) const override;
    
    NametableMirroring mirroringType;
};


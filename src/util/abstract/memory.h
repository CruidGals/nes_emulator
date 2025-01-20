//
//  memory.h
//  emulator_6502
//
//  Created by Kyle Chiem on 1/9/25.
//

#pragma once

#include <iostream>
#include <stdint.h>
#include <string>

class Memory
{
    std::unique_ptr<uint8_t[]> m_data;
    
public:
    Memory(uint16_t size) : m_data(std::make_unique<uint8_t[]>(size)) {}

    // Access operator
    uint8_t& operator[](uint16_t address)
    {
        return m_data[mirroredAddress(address)];
    }
    
    /*
     Helper function that grabs the memory address at data[0]
     */
    uint8_t* getBaseAddress()
    {
        return m_data.get();
    }
    
    uint8_t* getAbsoluteAddress(uint16_t address)
    {
        return getBaseAddress() + mirroredAddress(address);
    }
    
    /*
     Memory mirroring mimicker:
     It mimics the act of memory mirroring by returning the mirrored value from the base position than actual position
     
     For example:
        CPU RAM memory is mirrored every 2KB (0x0000 - 0x07FF, 0x0800 - 0x0FFF , ... , 0x17FF - 0x1FFF)
        A read to address 0x1FFF will yield the value of 0x07FF
        A read to address 0x0800 will yield the value of 0x0000, etc.
     
     Same for PPU Nametable memory
     */
    virtual uint16_t mirroredAddress(uint16_t address) const = 0;
    
    /**
     *  Reads the memory data via the inherting class' own read function. If no read function is provided, use this one.
     *
     *  @param addr Address to be read
     *  @return Returns the read data from the address
     */
    virtual uint8_t read(uint16_t addr) const
    {
        return m_data[mirroredAddress(addr)];
    }
    
    /**
     *  Writes to the memory data via the inherting class' own write function. If no write function is provided, use this one.
     *
     *  @param addr Address to be written to
     *  @param data Data to be written onto the address
     */
    virtual void write(uint16_t addr, uint8_t data) const
    {
        m_data[mirroredAddress(addr)] = data;
    }
};


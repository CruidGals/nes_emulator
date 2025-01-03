//
//  PPU.hpp
//  emulator_6502
//
//  Created by Kyle Chiem on 11/19/24.
//

#ifndef PPU_hpp
#define PPU_hpp

#include <stdio.h>
#include <stdint.h>

/* 
 Memory mapped registers to the CPU
 
 For each register, keep Bitfields to easily track the bits in register
*/
struct PPURegisters
{
    union PPUCTRL // $2000
    {
        struct 
        {
            unsigned N : 2; // Base Nametable addr (0: $2000, 1: $2400, 2: $2800, 3: $2c00)
            unsigned I : 1; // Direction of VRAM Incr. after reading PPUData (0: go across [add 1], 1: go down [add 32])
            unsigned S : 1; // Sprite pattern table address (0: $0000, 1: $1000)
            unsigned B : 1; // Background pattern table address (0: $0000; 1: $1000)
            unsigned H : 1; // Sprite size (0: 8x8 pixels; 1: 8x16 pixels)
            unsigned P : 1; // PPU master/slave select
            unsigned V : 1; // Vblank NMI enable (0: off, 1: on)
        };
        uint8_t val; // Full val of union
    };
    union PPUMASK    // $2001
    {
        struct
        {
            unsigned g : 1; // Greyscale (0: normal color, 1: greyscale)
            unsigned m : 1; // 1: Show background in leftmost 8 pixels of screen, 0: Hide
            unsigned M : 1; // 1: Show sprites in leftmost 8 pixels of screen, 0: Hide
            unsigned b : 1; // 1: Enable background rendering
            unsigned s : 1; // 1: Enable sprite rendering
            unsigned R : 1; // Emphasize red
            unsigned G : 1; // Emphasize green
            unsigned B : 1; // Emphasize blue
        };
        uint8_t val; // Full val of union
    };
    union PPUSTATUS  // $2002
    {
        struct
        {
            unsigned openBus : 5;
            unsigned O : 1;
            unsigned S : 1;
            unsigned V : 1;
        };
        uint8_t val; // Full val of union
    };
    uint8_t OAMADDR;    // $2003
    uint8_t OAMDATA;    // $2004
    union PPUSCROLL // $2005
    {
        struct
        {
            unsigned lo : 8;
            unsigned hi : 8;
        };
        uint8_t val;
    };
    union PPUADDR // $2006
    {
        struct
        {
            unsigned lo : 8;
            unsigned hi : 8; // Only should go up to 6 bits
        };
        uint8_t val;
    };
    uint8_t PPUDATA;    // $2007
    uint8_t OAMDMA;     // $4014
};

class PPU
{
    //Internal registers
    struct PPURegisters regs;
    uint8_t *memory;
    
public:
    uint8_t cpuDataBus;
    
    //Constructors Destructors
    PPU();
    ~PPU();
    
    /*
     Power/Reset function for PPU
     
     Power up state and Reset state differ marginally, so put in same function
     */
    void powerReset(bool isReset);
    
    // Overload operator[] for memory access (read only)
    const uint8_t& operator[](uint16_t address) const;
    
    //PPU Memory write functions
    void read(uint16_t addr);
    void write(uint16_t addr);
};

#endif /* PPU_hpp */

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
#include <array>

// LIB includes
#include "../util/ppumem.hpp"
#include "palette.hpp"

namespace Registers
{
/*
 Memory mapped registers to the CPU
 
 For each register, keep Bitfields to easily track the bits in register
 */
struct CPUMapped
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
    } PPUCTRL;
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
    } PPUMASK;
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
    } PPUSTATUS;
    uint8_t OAMADDR;    // $2003
    uint8_t OAMDATA;    // $2004
    uint8_t PPUSCROLL; // $2005
    union PPUADDR // $2006
    {
        struct
        {
            unsigned lo : 8;
            unsigned hi : 8; // Only should go up to 6 bits
        };
        uint8_t val;
    } PPUADDR;
    uint8_t PPUDATA;    // $2007
    uint8_t OAMDMA;     // $4014
};

struct Internal
{
    union // Current (v) and Temp (t) VRAM address separated bitdields
    {
        struct
        {
            unsigned coarseX : 5;   // Coarse X scroll
            unsigned coarseY : 5;   // Coarse Y scroll
            unsigned nametable : 2; // Nametable select
            unsigned fineY : 3;     // Fine y scroll
        };
        uint16_t val;
    } v, t;
    
    uint8_t x : 3; // Fine X scroll
    uint8_t w : 1; // First or second write toggle
};

} // namespace Registers

class PPU
{
    // Internal color palette
    Palette palette;
    
    //Internal registers
    struct Registers::CPUMapped m_regs; // Completely remove this later
    struct Registers::Internal m_intRegs;
    
    Memory& memory;
    
public:
    // Used to talk between CPU and PPU
    uint8_t cpuDataBus;
    
    //Constructors Destructors
    PPU(Memory& mem);
    
    /*
     Power/Reset function for PPU
     
     Power up state and Reset state differ marginally, so put in same function
     */
    void powerResetState(bool isReset);
    
    // Overload operator[] for memory access (read only)
    const uint8_t& operator[](uint16_t address) const;
    
    //PPU Memory read/write functions
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t result);
    
    // Write helper functions
    void writePPUScroll(uint8_t result);
    void writePPUAddr(uint8_t result);
    void writePPUData(uint8_t result);
    
    // Other helper functions
    
    /**
     *  When the fine Y value of internal register v is incremented, it has the following implications on coarse Y and nametable Select:
     *  - If fine Y is greater than 7 and incremented, it's value will overflow into the coarse Y variable.
     *  - If coarse Y is equal to 29, the vertical nametable (bit 11 of internal register v) will toggle. Coarse Y is set to 0.
     *  - If coarse Y is equal to 31, the vertical nametable will not toggle. Coarse Y is set to 0.
     *  - Otherwise, coarse Y is set incremented.
     */
    void fineYIncrement();
    
    void debug() const;
};

#endif /* PPU_hpp */

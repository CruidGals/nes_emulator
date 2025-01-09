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

PPU::PPU(Memory& mem) : memory(mem) {}

//Power up function
void PPU::powerResetState(bool isReset)
{
    regs.PPUCTRL.val = 0;
    regs.PPUMASK.val = 0;
    regs.PPUSTATUS.val = (isReset) ? regs.PPUSTATUS.val | 0x80 : 0xa0;
    regs.PPUSCROLL = 0;
    regs.PPUDATA = 0;
    
    // Registers that are only changed by Power on events
    if (!isReset)
    {
        regs.OAMADDR = 0;
        regs.PPUADDR.val = 0;
    }
}

/* --------------- READ WRITE FUNCTIONS ---------------*/
uint8_t PPU::read(uint16_t addr)
{
    uint8_t readValue;
    
    switch (addr) {
        case 0x2002: // PPUSTATUS
            readValue = regs.PPUSTATUS.val;
            regs.PPUSTATUS.V = 0;   // Clear V-Blank flag
            intRegs.w = 0;          // Side effect
            break;
        case 0x2004: // OAMDATA
            readValue = regs.OAMDATA;
            break;
        case 0x2007: // PPUDATA
            readValue = regs.PPUDATA;
            break;
        
        default:
            readValue = cpuDataBus;
            break;
    }
    
    return readValue;
}

void PPU::write(uint16_t addr, uint8_t result)
{
    switch (addr) {
        case 0x2000: // PPUCTRL
            regs.PPUSTATUS.val = result;
            intRegs.w = 0; // Side effect
            break;
        case 0x2001: // PPUMASK
            cpuDataBus = regs.OAMDATA;
            //TODO -- After power/reset, writes are ignored until first pre-render scanline
            break;
        case 0x2003: // OAMADDR
            regs.OAMADDR = result;
            break;
        case 0x2004: // OAMDATA
            regs.OAMDATA = result;
            regs.OAMADDR++;         // OAMADDR Increments after every write
            break;
        case 0x2005: {// PPUSCROLL
            regs.PPUSCROLL = result;
            // TODO -- do something with the data depending on what write register it is
            intRegs.w = ~intRegs.w; // Toggle the w bit
        }   break;
        case 0x2006: { // PPUADDR
            if (intRegs.w)
                regs.PPUADDR.lo = result;   // Write to low byte (w = 1)
            else
                regs.PPUADDR.hi = result;   // Write to high byte (w = 0)
            
            intRegs.w = ~intRegs.w; //Toggle w bit
            
            // TODO -- do some work on the temporary VRAM address thing
        }
            break;
        case 0x2007: {// PPUDATA
            regs.PPUDATA = result;
            
            if (regs.PPUCTRL.I)
                regs.PPUADDR.val += 0x20;
            else
                regs.PPUADDR.val += 1;
            
        }   break;
        case 0x4014: // OAMDMA
            // TODO -- Implement later
            break;
    
        default:
            break;
    }
}

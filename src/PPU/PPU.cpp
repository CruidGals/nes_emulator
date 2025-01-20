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

PPU::PPU(Memory& mem) : memory(mem), palette("../../res/Composite_wiki.pal") {}

//Power up function
void PPU::powerResetState(bool isReset)
{
    m_regs.PPUCTRL.val = 0;
    m_regs.PPUMASK.val = 0;
    m_regs.PPUSTATUS.val = (isReset) ? m_regs.PPUSTATUS.val | 0x80 : 0xa0;
    m_regs.PPUSCROLL = 0;
    m_regs.PPUDATA = 0;
    
    // Registers that are only changed by Power on events
    if (!isReset)
    {
        m_regs.OAMADDR = 0;
        m_regs.PPUADDR.val = 0;
    }
}

/* --------------- READ WRITE FUNCTIONS ---------------*/
uint8_t PPU::read(uint16_t addr)
{
    uint8_t readValue;
    
    switch (addr) {
        case 0x2002: // PPUSTATUS
            readValue = m_regs.PPUSTATUS.val;
            m_regs.PPUSTATUS.V = 0;   // Clear V-Blank flag AFTER being read
            m_intRegs.w = 0;          // Side effect
            break;
        case 0x2004: // OAMDATA
            return m_regs.OAMDATA;
            break;
        case 0x2007: // PPUDATA
            return memory[m_intRegs.v.val];
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
            m_regs.PPUCTRL.val = result;
            m_intRegs.t.nametable = (result & 0x03); // t: ...GH.. ........ <- d: ......GH
            break;
        case 0x2001: // PPUMASK
            m_regs.PPUMASK.val = result;
            break;
        case 0x2003: // OAMADDR
            m_regs.OAMADDR = result;
            break;
        case 0x2004: // OAMDATA
            m_regs.OAMDATA = result;
            m_regs.OAMADDR++;         // OAMADDR Increments after every write
            break;
        case 0x2005: // PPUSCROLL
            writePPUScroll(result);
            break;
        case 0x2006: // PPUADDR
            writePPUAddr(result);
            break;
        case 0x2007: // PPUDATA
            writePPUData(result);
            break;
        case 0x4014: // OAMDMA
            // TODO -- Implement later
            break;
    
        default:
            break;
    }
}

/* --------- WRITE HELPER FUNCTIONS --------- */

void PPU::writePPUScroll(uint8_t result)
{
    m_regs.PPUSCROLL = result;
    
    if (!m_intRegs.w) // m_intRegs.w == 0
    {
        m_intRegs.t.coarseX = (result >> 3);
        m_intRegs.x = (result & 0x08);
    }
    else              // m_intRegs.w == 1
    {
        m_intRegs.t.coarseY = (result >> 3);
        m_intRegs.t.fineY = (result & 0x08);
    }
    
    // Toggle after every write
    m_intRegs.w = ~m_intRegs.w;
}

void PPU::writePPUAddr(uint8_t result)
{
    if (!m_intRegs.w)   // m_intRegs.w == 0
    {
        m_regs.PPUADDR.hi = result;            // d: ABCDEFGH
        uint16_t truncatedResult = result & 0x3F; // t: .CDEFGH ........ <- d: ..CDEFGH
        m_intRegs.t.val = truncatedResult << 8 | (m_intRegs.t.val & 0xFF); // Move into higher byte
    }
    else                // m_intRegs.w == 1
    {
        m_regs.PPUADDR.lo = result;
        m_intRegs.t.val = (m_intRegs.t.val & 0xFF00) | result; // t: ....... ABCDEFGH <- d: ABCDEFGH
        m_intRegs.v.val = m_intRegs.t.val;  // Transfer to the v register (b/c t is temp register)
    }
    
    // Toggle after every write
    m_intRegs.w = ~m_intRegs.w;
}

void PPU::writePPUData(uint8_t result)
{
    m_regs.PPUDATA = result;
    memory[m_intRegs.v.val] = result;
    
    if (m_regs.PPUCTRL.I)
    {
        m_regs.PPUADDR.val += 0x20;
        m_intRegs.v.val += 0x20;
    }
    else
    {
        m_regs.PPUADDR.val++;
        m_intRegs.v.val++;
    }
    
}

/* ---------- Other Helper Functions ---------- */

void PPU::fineYIncrement()
{
    // Increment normally if fineY != 7 (does not overflow)
    if (m_intRegs.v.fineY < 7)
    {
        m_intRegs.v.fineY++;
        return;
    }
    
    // Case that fineY does overflow, goes into coarse Y
    m_intRegs.v.fineY = 0;
    
    if (m_intRegs.v.coarseY == 29)
    {
        m_intRegs.v.coarseY = 0;    // Simulate overflow
        m_intRegs.v.nametable ^= 1; // Toggle vertical nametable
    }
    else if (m_intRegs.v.coarseY == 31)
    {
        m_intRegs.v.coarseY = 0;    // Simulate overflow, but don't toggle vertical nametable
    }
    else
    {
        m_intRegs.v.coarseY++;      // Act as normal overflow
    }
}

void PPU::debug() const
{
    std::cout << std::hex << static_cast<int>(m_intRegs.v.val) << std::dec << "\n";
}

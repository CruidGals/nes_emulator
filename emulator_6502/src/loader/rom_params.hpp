//
//  rom_params.hpp
//  emulator_6502
//
//  Created by Kyle Chiem on 1/13/25.
//

#pragma once

#include <stdint.h>
#include <vector>
#include <array>

struct Header 
{
    // This union determines what format the .nes file is
    union
    {
        struct
        {
            unsigned iNES : 1;
            unsigned nes2_0 : 1;
        };
        uint8_t val;
    } format;
    
    // Defines the size of prg and chr Rom
    uint16_t prgRomSize : 12;
    uint16_t chrRomSize : 12;
    
    // This union contains mapper + submapper number
    uint16_t mapperNumber : 12;
    uint8_t submapperNumber : 4;
    
    // This union combines flags 6 and 7, and ignores the mapper numbers at the end of them
    union
    {
        struct
        {
            // Ones from flag 6
            unsigned M : 1; // Hard-wired nametable layout. 0: Vertical or mapper controlled, 1: Horizontal
            unsigned B : 1; // Battery / non volatile memory. 0 : Not present, 1 : present
            unsigned T : 1; // 512 Byte Trainer. 0 : Not present, 1 : present
            unsigned F : 1; // Alternative nametables. 0 : No, 1 : Yes
            
            // Ones from flag 7
            unsigned consoleType : 2; // 0: NES/Famicom, 1: Nintendo vs System,
                                      // 2: Nintendo Playchoice 10, 3: Extended Console Type
            unsigned nes2_0Id : 2;    // NES 2.0 Identifier. If it is 10 in binary, then it is 2.0 format
        };
        uint8_t val;
    } flags;
    
    // If Console Type is Nintendo vs System, this byte is used
    union
    {
        struct
        {
            unsigned ppuType : 4;
            unsigned hardwareType : 4;
        };
        uint8_t val;
    } systemType;
    
    // If Console Type is Extended Console Type, this byte is used
    uint8_t extConsoleType : 4;
    
    // Other misc bytes
    uint8_t timingMode : 2; //0: RP2C02 ("NTSC NES"), 1: RP2C07 ("Licensed PAL NES"), 2: Multiple-region, 3: UA6538 ("Dendy")
    uint8_t miscRoms : 2;
    uint8_t defaultExpansionDevice : 6;
};

struct RomData
{
    std::array<uint8_t, 512> trainer; // Optional
    std::vector<uint8_t> prgRom;
    std::vector<uint8_t> chrRom;
};

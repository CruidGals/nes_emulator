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

typedef struct NESPPU
{
    uint8_t *chr_memory; //8KiB Memory
    uint8_t *work_mem; //2KiB Memory
    
} NESPPU;

#endif /* PPU_hpp */

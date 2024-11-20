//
//  6502emu.hpp
//  emulator_6502
//
//  Created by Kyle Chiem on 10/16/24.
//

#ifndef _502emu_hpp
#define _502emu_hpp

#include <stdint.h>

struct ProcessorStatus
{
    uint8_t c:1; //0 - carry bit        1 = carry
    uint8_t z:1; //1 - zero bit         1 = zero
    uint8_t i:1; //2 - Irq disable      1 = disable
    uint8_t d:1; //3 - Decimal mode     1 = True
    uint8_t b:1; //4 - Break command    1 = BRK
    //5th code skipped (Expansion code)
    uint8_t v:1; //6 - Overflow         1 = True
    uint8_t n:1; //7 - Negative         1 = NEG
};

typedef struct State6502
{
    uint8_t a;  //Accumulator
    uint8_t x;  //X index register
    uint8_t y;  //Y index register
    uint8_t s;  //Stack pointer
    
    uint16_t pc; //Program counter
    uint8_t *memory; //Memory accessed by program counter
    struct ProcessorStatus ps; //Processor status
    
} State6502;

int emulate(State6502 *state);
void disassemble(State6502 *state);

#endif /* _502emu_hpp */

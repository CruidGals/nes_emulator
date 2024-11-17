//
//  main.cpp
//  emulator_6502
//
//  Created by Kyle Chiem on 10/16/24.
//

#include <iostream>
#include <stdint.h>
#include "6502emu.hpp"

State6502* init6502()
{
    State6502* state = static_cast<State6502*>(calloc(1, sizeof(State6502)));
    state->memory = static_cast<uint8_t*>(malloc(0x10000));
    return state;
}

int main(int argc, const char * argv[]) 
{
    uint8_t program[] = {
        0xA9, 0x01, 0xA2, 0xFF, 0xA0, 0x80, 0x8D, 0x00, 0x02, 0x8E, 0x01, 0x02,
        0x8C, 0x02, 0x02, 0x18, 0x69, 0x02, 0x38, 0xE9, 0x01, 0x29, 0x0F, 0x09,
        0xF0, 0x49, 0xFF, 0xC9, 0x03, 0xF0, 0x02, 0xE8, 0xCA, 0x48, 0x68, 0x60
    };
    
    State6502* state = init6502();
    
    int counter { 0 };
    
    for (auto const opcode : program)
    {
        state->memory[counter] = opcode;
        counter++;
    }

    for (int i=0; i < counter; i++)
    {
        emulate(state);
    }
    
}

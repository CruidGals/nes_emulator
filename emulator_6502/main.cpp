//
//  main.cpp
//  emulator_6502
//
//  Created by Kyle Chiem on 10/16/24.
//

#include <iostream>
#include <fstream>
#include <stdint.h>
#include "CPU/6502emu.hpp"
#include "PPU/PPU.hpp"

//Loads hex file into 6502 memory
void readFile(State6502 *const state, const std::string_view filename, uint32_t offset)
{
    //Start reading file through stream in binary mode (start at end of list to get size)
    std::ifstream file(std::string(filename), std::ios::binary | std::ios::ate);
    if(!file)
    {
        throw std::runtime_error("Could not open file: " + std::string(filename));
    }
    
    //Get size (can do because used std::ios::ate before)
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    //Set position at start of list and read file into memory
    file.read(reinterpret_cast<char*>(&state->memory[offset]), size);
    if(!file)
    {
        throw std::runtime_error("Could not read entire file");
    }
}

State6502* init6502()
{
    State6502* state = static_cast<State6502*>(calloc(1, sizeof(State6502)));
    state->memory = static_cast<uint8_t*>(calloc(0x10000, sizeof(uint8_t)));
    state->ps.i = 1;
    state->s = 0xff;
    return state;
}

void free6502(State6502* state)
{
    if (state)
    {
        free(state->memory);
        free(state);
    }
}

int main(int argc, const char * argv[]) 
{
    /*
    State6502* state = init6502();
    
    uint8_t program[] = {
        0x38, 0x90, 0x02, 0xA2, 0x04, 0xA9, 0x01, 0xC8
    };
    
    int counter { 0 };
    
    for (auto const opcode : program)
    {
        state->memory[counter] = opcode;
        counter++;
    }
    
    std::string filepath = "./roms/Donkey Kong (Japan).nes";
    readFile(state, filepath, 0x8000);
    state->pc = 0;
    int cycles = 0;
    
    for (int i = 0; i < 4; i++)
    {
        cycles += emulate(state);
    }
    
    std::cout << cycles << std::endl;
    
    free6502(state);
     */
    
    return 0;
}

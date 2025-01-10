//
//  main.cpp
//  emulator_6502
//
//  Created by Kyle Chiem on 10/16/24.
//

#include <iostream>
#include <fstream>
#include <stdint.h>

// Lib includes
#include "CPU/6502emu.hpp"
#include "PPU/PPU.hpp"
#include "screen/gui.hpp"

#include "util/cpumem.hpp"
#include "util/ppumem.hpp"

// SFML includes
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

//Loads hex file into 6502 memory
void readFile(cpu6502 *const cpu, const std::string_view filename, uint32_t offset)
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
    file.read(reinterpret_cast<char*>(cpu->memory.getAbsoluteAddress(offset)), size);
    if(!file)
    {
        throw std::runtime_error("Could not read entire file");
    }
}

int main(int argc, const char * argv[]) 
{
    /*
    PPUMemory* ppuMem = new PPUMemory(NametableMirroring::NONE);
    PPU* ppu = new PPU(*ppuMem);
    CPUMemory* cpuMem = new CPUMemory(ppu);
    cpu6502* cpu = new cpu6502(*cpuMem);
    
    uint8_t program[] = {
        0xA9, 0x00,        // LDA #$00
        0x8D, 0x00, 0x02,  // STA $0200
        0xEE, 0x00, 0x02,  // INC $0200
        0xAD, 0x00, 0x02,  // LDA $0200
        0x4C, 0x05, 0x02   // JMP LOOP (to $0203)
    };
    
    uint16_t counter { 0x200 };
    
    for (auto const opcode : program)
    {
        cpu->memory[counter] = opcode;
        counter++;
    }
    
    //std::string filepath = "./roms/Donkey Kong (Japan).nes";
    //readFile(cpu, filepath, 0x8000);
    cpu->pc.val = 0x200;
    int cycles = 0;
    
    for (int i = 0; i < 100; i++)
    {
        cycles += cpu->emulate();
    }
    
    std::cout << cycles << std::endl;
    
    delete cpu;
    delete cpuMem;
    delete ppu;
    delete ppuMem;
    */
    
    GUI game;
    
    
    struct Pixel pixel = {255, 255, 255};
    
    for (int i = 100; i < 110; ++i)
    {
        game.drawPixel(100, i, pixel);
    }

    while (game.running())
    {
        game.update();
        game.render();
    }
    
    
    return 0;
}

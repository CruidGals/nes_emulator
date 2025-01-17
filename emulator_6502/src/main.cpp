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
#include "loader/loader.hpp"

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

void drawMario(GUI* gui)
{
    struct Pixel background = {0, 0, 0};
    struct Pixel clothes = {177, 52, 37};
    struct Pixel hair = {106, 107, 4};
    struct Pixel skin = {227, 157, 37};
    
    int y = 0;
    
    // Line 1
    for (int i = 0; i < 12; i++)
    {
        gui->drawPixel(i, y, background);
    }
    
    y++;
    
    // Line 2
    for (int i = 0; i < 12; i++)
    {
        if (i > 2 && i < 8)
            gui->drawPixel(i, y, clothes);
        else
            gui->drawPixel(i, y, background);
    }
    
    // Line 3
    
    y++;
    for (int i = 0; i < 12; i++)
    {
        if (i > 1 && i < 11)
            gui->drawPixel(i, y, clothes);
        else
            gui->drawPixel(i, y, background);
    }
    
    // Line 4
    
    y++;
    int i = 0;
    
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, background);
    
    // Line 5
    
    y++;
    i = 0;
    
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, background);
    
    // Line 6
    
    y++;
    i = 0;
    
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, skin);
    
    // Line 7
    
    y++;
    i = 0;
    
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, background);
    
    // Line 8
    
    y++;
    for (int j = 0; j < 12; j++)
    {
        if (j > 2 && j < 10)
            gui->drawPixel(j, y, skin);
        else
            gui->drawPixel(j, y, background);
    }
    
    // Line 9
    y++;
    i = 0;
    
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, clothes);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, background);
    
    // Line 10
    y++;
    i = 0;
    
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, clothes);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, clothes);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, background);
    
    // Line 11
    y++;
    i = 0;
    
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, clothes);
    gui->drawPixel(i++, y, clothes);
    gui->drawPixel(i++, y, clothes);
    gui->drawPixel(i++, y, clothes);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    
    // Line 12
    y++;
    i = 0;
    
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, clothes);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, clothes);
    gui->drawPixel(i++, y, clothes);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, clothes);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, skin);
    gui->drawPixel(i++, y, skin);
    
    // Line 13
    
    y++;
    for (int j = 0; j < 12; j++)
    {
        if (j > 2 && j < 9)
            gui->drawPixel(j, y, clothes);
        else
            gui->drawPixel(j, y, skin);
    }
    
    // Line 14
    
    y++;
    for (int j = 0; j < 12; j++)
    {
        if (j > 1 && j < 10)
            gui->drawPixel(j, y, clothes);
        else
            gui->drawPixel(j, y, skin);
    }
    
    // Line 15
    y++;
    i = 0;
    
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, clothes);
    gui->drawPixel(i++, y, clothes);
    gui->drawPixel(i++, y, clothes);
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, clothes);
    gui->drawPixel(i++, y, clothes);
    gui->drawPixel(i++, y, clothes);
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, background);
    
    // Line 16
    
    y++;
    i = 0;
    
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, background);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, hair);
    gui->drawPixel(i++, y, background);
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
    
    /*
    GUI game;
    drawMario(&game);

    while (game.running())
    {
        game.update();
        game.render();
    }
    */
    
    Loader loader("/Users/kylechiem/Documents/VSCode Projects/c++/xcodejawn/emulator_6502/emulator_6502/roms/Donkey Kong (Japan).nes");
    
    
    
    return 0;
}
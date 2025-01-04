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
    file.read(reinterpret_cast<char*>(&cpu->memory[offset]), size);
    if(!file)
    {
        throw std::runtime_error("Could not read entire file");
    }
}

int main(int argc, const char * argv[]) 
{
    
    cpu6502* cpu = new cpu6502();
    
    uint8_t program[] = {
        0x38, 0x90, 0x02, 0xA2, 0x04, 0xA9, 0x01, 0xC8
    };
    
    int counter { 0 };
    
    for (auto const opcode : program)
    {
        cpu->memory[counter] = opcode;
        counter++;
    }
    
    //std::string filepath = "./roms/Donkey Kong (Japan).nes";
    //readFile(cpu, filepath, 0x8000);
    cpu->pc.val = 0;
    int cycles = 0;
    
    for (int i = 0; i < 4; i++)
    {
        cycles += cpu->emulate();
    }
    
    std::cout << cycles << std::endl;
    
    delete cpu;
     
    /*
    sf::RenderWindow window(sf::VideoMode({800, 600}), "SFML works!");
    sf::CircleShape shape(50);
    shape.setFillColor(sf::Color::Green);

    while (window.isOpen()) 
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        // Clear
        window.clear();
        
        // Draw
        window.draw(shape);
        
        // Update
        window.display();
    }
     */
    
    return 0;
}

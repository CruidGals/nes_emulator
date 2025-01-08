//
//  gui.hpp
//  emulator_6502
//
//  Created by Kyle Chiem on 1/3/25.
//

#pragma once

// STD Library includes
#include <stdio.h>
#include <chrono>
#include <thread>

// SFML includes
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

class GUI {
    // Important variables for screen
    sf::RenderWindow window;
    sf::CircleShape circle;
    
    // Clock for handling CPU, PPU timing
    
public:
    // Constructors
    GUI();
    
    const bool running() const;
    
    void update();
    void render();
};

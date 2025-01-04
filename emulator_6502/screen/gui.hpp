//
//  gui.hpp
//  emulator_6502
//
//  Created by Kyle Chiem on 1/3/25.
//

#pragma once

#include <stdio.h>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

class GUI {
    // Important variables for screen
    sf::RenderWindow* window;
    sf::Event event;
    
public:
    void update();
    void render();
};

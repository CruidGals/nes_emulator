//
//  gui.hpp
//  emulator_6502
//
//  Created by Kyle Chiem on 1/3/25.
//

#pragma once

// STD Library includes
#include <stdio.h>
#include <array>

// SFML includes
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

/*
 Max dimensions for a pixel is 256x240
 Min dimensions are 0x0
 */
struct Pixel
{
    uint8_t r, g, b;
    uint8_t a = 0xFF;
};

class GUI {
    // Important variables for screen
    sf::RenderWindow m_window;
    
    std::array<Pixel, 256*240> m_pixelRepr; // Array representation of screen
    sf::Texture m_renderedNametable;  // Off-screen buffer that is later drawn onto sf::Sprite
    sf::Sprite m_screen;
    
public:
    // Constructors & Destructors
    GUI();
    
    // Checks if the game is running or not
    const bool running() const;
    
    // Game loop functions
    void update();
    void render();
    
    // Updates the m_renderedNametable function with the new data from m_pixelRepr
    void updateNametable();
    
    // Draws and gets pixels from m_pixelRepr
    void drawPixel(int x, int y, struct Pixel color);
    Pixel getPixel(int x, int y) const;
    
    // Other helper functions
    const bool inBounds(int x, int y) const;
};

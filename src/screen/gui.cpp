//
//  gui.cpp
//  emulator_6502
//
//  Created by Kyle Chiem on 1/3/25.
//

#include "gui.hpp"

#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

GUI::GUI() : m_window(sf::VideoMode({256,240}), "NES Emulator"), m_screen(m_renderedNametable)
{
    if (!m_renderedNametable.resize({256,240}))
    {
        perror("Error loading texture");
    }
    
    // Required to make it show onto the screen (for some reason)
    // The {256,240} part makes it so it shows the entire rect, b/c it initalizes it with {0,0} to start
    m_screen.setTextureRect({{0,0}, {256,240}});
    
    m_screen.setPosition({0,0});
    
    // Initialize pixel data with all black
    for (int i = 0; i < 256 * 240; ++i) {
        m_pixelRepr[i] = Pixel{0, 0, 0};
    }
    
    updateNametable();
}

const bool GUI::running() const
{
    return m_window.isOpen();
}

void GUI::update()
{
    while (const std::optional event = m_window.pollEvent())
    {
        if (event->is<sf::Event::Closed>() ||
           (event->is<sf::Event::KeyPressed>() &&
            event->getIf<sf::Event::KeyPressed>()->code == sf::Keyboard::Key::Escape))
            m_window.close();
    }
}

void GUI::render()
{
    // Clear
    m_window.clear();
    
    // Update the texture of nametable
    updateNametable();
    
    // Draw
    m_window.draw(m_screen);
    
    // Update
    m_window.display();
}

void GUI::updateNametable()
{
    // This works as m_pixelRepr stores a simple struct (Pixel) that can easily be
    // written into the r,g,b,a uint8_t format that SFML wants
    m_renderedNametable.update(reinterpret_cast<const uint8_t*>(m_pixelRepr.data()),
                              {256,240}, // Size of screen (256*240)
                              {0,0});    // Position of screen
}

/*
 *  Because m_pixelRepr is a 1D array, must use (256 * y) + x arithmetic:
 *  Every pixel down the screen (y increment), 256 pixels across the x-axis are skipped
 *  To get position in array, do y * 256 to see how many lines are skipped
 *  Then, add on the remaining x pixels to get offset.
 */
void GUI::drawPixel(int x, int y, struct Pixel color)
{
    if (!inBounds(x, y))
    {
        std::cerr << "Tried accessing memory that is out of bounds.\n";
    }
    
    m_pixelRepr[(256 * y) + x] = color;
}

Pixel GUI::getPixel(int x, int y) const
{
    if (!inBounds(x, y))
    {
        std::cerr << "Tried accessing memory that is out of bounds.\n";
    }
    
    return m_pixelRepr[(256 * y) + x];
}

const bool GUI::inBounds(int x, int y) const
{
    return ((x >= 0) && (x < 256)) && ((y >= 0) && (y < 240));
}

//
//  gui.cpp
//  emulator_6502
//
//  Created by Kyle Chiem on 1/3/25.
//

#include "gui.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

GUI::GUI() : window(sf::VideoMode({256,240}), "NES Emulator"), circle(50.0f)
{
    circle.setPosition({100,100});
    circle.setFillColor(sf::Color::Green);
}
const bool GUI::running() const
{
    return window.isOpen();
}

void GUI::update()
{
    while (const std::optional event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>() ||
           (event->is<sf::Event::KeyPressed>() &&
            event->getIf<sf::Event::KeyPressed>()->code == sf::Keyboard::Key::Escape))
            window.close();
    }
}

void GUI::render()
{
    // Clear
    window.clear();
    
    // Draw
    window.draw(circle);
    
    // Update
    window.display();
}

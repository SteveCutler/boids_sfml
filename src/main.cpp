#include <iostream>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "BoidSystem.hpp"

int main()
{
    unsigned int x_max = 512;
    unsigned int y_max = 256;
    // create the window
    sf::RenderWindow window(sf::VideoMode({x_max, y_max}), "Boids");

    
    // create the boid system
    BoidSystem boids(100, x_max, y_max);

    // create a clock to track the elapsed time
    sf::Clock clock;

    // run the main loop
    while (window.isOpen())
    {
        // handle events
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        // make the particle system emitter follow the mouse
       
       // particles.setEmitter(window.mapPixelToCoords(mouse));

        // update it
        sf::Time elapsed = clock.restart();
        boids.update(elapsed);

        // draw it
        window.clear();
        window.draw(boids);
        window.display();
    }
}
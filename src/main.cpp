#include <iostream>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "BoidSystem.hpp"

int main()
{
    //SET BOID COUNT
    unsigned int boidCount = 400;

    //SET WIDTH AND HEIGHT
    unsigned int x_max = 512;
    unsigned int y_max = 256;

    //CELL VARIABLES
    std::size_t cell_size = 64;
    std::size_t grid_width = ceil(static_cast<float>(x_max)/64);
    std::size_t grid_height = ceil(static_cast<float>(y_max)/64);

    // create the window
    sf::RenderWindow window(sf::VideoMode({x_max, y_max}), "Boids");

    //Performance debug text
    sf::Font font;
    bool loaded = font.openFromFile("/Users/stevecutler/Library/Fonts/digital-7 (italic).ttf");

    sf::Text fpsText(font);
    fpsText.setCharacterSize(12);
    fpsText.setFillColor(sf::Color::White);
    fpsText.setPosition({5.f, 5.f});
  
    sf::Text msText(font);
    msText.setCharacterSize(12);
    msText.setFillColor(sf::Color::White);
    msText.setPosition({5.f, 15.f});
  
    sf::Text boidText(font);
    boidText.setCharacterSize(12);
    boidText.setFillColor(sf::Color::White);
    boidText.setPosition({5.f, 35.f});
    
    // create the boid system
    BoidSystem boids(boidCount, x_max, y_max, cell_size, grid_width, grid_height);

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

        // update it
        sf::Time elapsed = clock.restart();
        boids.update(elapsed);

        //calculate performance values
        float dt = elapsed.asSeconds();
        float fps = 1.f / dt;
        float ms = dt * 1000.f;

        //overlay strings

        boidText.setString(
            "\nBoids: " + std::to_string(boidCount)
        );

        fpsText.setString(
            "FPS: " + std::to_string(static_cast<int>(fps))
        );

        msText.setString(
            "\nFrame ms: " + std::to_string(ms)
        );


        // DRAW BLOCK

        //clear window
        window.clear();

        //draw boids
        window.draw(boids);

        //draw text
        window.draw(boidText);
        window.draw(fpsText);
        window.draw(msText);

        //display
        window.display();
    }
}
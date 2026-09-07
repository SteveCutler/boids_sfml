#include "BoidSystem.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>

int main()
{
    constexpr std::size_t boid_count = 400;
    constexpr unsigned int width = 512;
    constexpr unsigned int height = 256;
    constexpr std::size_t cell_size = 64;

    sf::Font font;
    if (!font.openFromFile(BOIDS_FONT_PATH))
    {
        std::cerr << "Failed to load bundled font: " << BOIDS_FONT_PATH << '\n';
        return 1;
    }

    sf::RenderWindow window(sf::VideoMode({width, height}), "Boids");
    window.setVerticalSyncEnabled(true);
    sf::Text overlay(font);
    overlay.setCharacterSize(12);
    overlay.setFillColor(sf::Color::White);
    overlay.setPosition({5.f, 5.f});
    BoidSystem boids(boid_count, width, height, cell_size);

    // SFML time uses integer microseconds: approximately 60 simulation steps/s.
    const sf::Time step = sf::microseconds(16667);
    const sf::Time max_frame_time = sf::milliseconds(250);
    sf::Time accumulator;
    sf::Clock clock;

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }
        if (!window.isOpen())
            break;

        const sf::Time elapsed = clock.restart();
        // Drop time beyond 250 ms after a stall instead of unlimited catch-up.
        accumulator += std::min(elapsed, max_frame_time);
        while (accumulator >= step)
        {
            boids.update(step);
            accumulator -= step;
        }

        const float dt = elapsed.asSeconds();
        const float fps = dt > 0.f ? 1.f / dt : 0.f;
        std::ostringstream text;
        text << "Boids: " << boid_count << '\n'
             << std::fixed << std::setprecision(1) << "FPS: " << fps << '\n'
             << "Frame ms: " << dt * 1000.f;
        overlay.setString(text.str());

        window.clear();
        window.draw(boids);
        window.draw(overlay);
        window.display();
    }
}

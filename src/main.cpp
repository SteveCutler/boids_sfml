#include <iostream>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

int main()
{
    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8;

    //Render window with added anti-alias
    sf::RenderWindow window(sf::VideoMode({800, 600}), "My window", sf::Style::Default, sf::State::Windowed, settings);

    // fix vertical tearing
    window.setVerticalSyncEnabled(true); // call it once after creating the window

    //set framerate manually
    //window.setFramerateLimit(60); // call it once after creating the window


    // run the program as long as the window is open
    while (window.isOpen())
    {
        // check all the window's events that were triggered since the last iteration of the loop
        while (const std::optional event = window.pollEvent())
        {


            // "close requested" event: we close the window
            if (event->is<sf::Event::Closed>())
                window.close();
        }

         // clear the window with black color
        window.clear(sf::Color::Black);

        // draw everything here...


        // create an empty shape
        sf::ConvexShape convex;

        // color
        convex.setFillColor(sf::Color(100, 250, 50));

        // resize it to 3 points
        convex.setPointCount(3);
        

        // define the points
        convex.setPoint(0, {400.f, 195.f});
        convex.setPoint(1, {395.f, 205.f});
        convex.setPoint(2, {405.f, 205.f});


        // window.draw(...);
        window.draw(convex);

        // end the current frame
        window.display();
    }
}
#pragma once

#include <SFML/Graphics.hpp>
#include <random>
#include <vector>
#include <cstdint>
#include <cmath>

class BoidSystem : public sf::Drawable, public sf::Transformable
{

public:
    BoidSystem(unsigned int count, int x, int y) : m_boids(count), m_vertices(sf::PrimitiveType::Points, count)
    {
        SpawnBoids(count, x , y);
    }

    // void setEmitter(sf::Vector2f position)
    // {
    //     m_emitter = position;
    // }

    void update(sf::Time elapsed)
    {
        for (std::size_t i = 0; i < m_boids.size(); ++i)
        {
            // update the particle lifetime
            Boid& p = m_boids[i];


            // update the position of the corresponding vertex
            m_vertices[i].position.x += p.velocity.x * elapsed.asSeconds();
            m_vertices[i].position.y += p.velocity.y * elapsed.asSeconds();

            // add color
            m_vertices[i].color = p.color;

            // add boundary check
            // check to red on collision
          
            //m_vertices[i].color.a = static_cast<std::uint8_t>(ratio * 255);

            //add size and velocity drag updates
        }
    }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        // apply the transform
        states.transform *= getTransform();

        // our particles don't use a texture
        states.texture = nullptr;

        // draw the vertex array
        target.draw(m_vertices, states);
    }

    void create_tris(){
    // write function for rendering boids are tris
    }

    void forces(){
        //write function to update pos, vel, etc
        //write helper functions for this
    }

    struct Boid
    {
        int id;
        sf::Vector2f velocity;
        sf::Vector2f position;
        sf::Color color;
    };

    void SpawnBoids(std::size_t count, unsigned int x, unsigned int y)
    {

        // create random number generator
        static std::random_device rd;
        static std::mt19937       rng(rd());

        //set possible color range
        std::uniform_int_distribution<int> colorDist(50, 250);

        for (std::size_t i = 0; i < count; ++i){

            Boid b;
            b.id = i;

            // give a random birth position to the boid
            b.position = sf::Vector2f(std::uniform_real_distribution(0.f, static_cast<float>(x))(rng), std::uniform_real_distribution(0.f, static_cast<float>(y))(rng));

            //give random birth vel and angle to boid
            const double angle       = (std::uniform_real_distribution(0.f, 360.f)(rng));
            const float     speed       = std::uniform_real_distribution(5.f, 10.f)(rng);

            // convert angle and speed into vel
            double vx = std::cos(angle)*speed;
            double vy = std::sin(angle)*speed;
            b.velocity = sf::Vector2f(vx, vy);

            //add random color
            // unsigned int red = colorDist(rng);
            // unsigned int green = colorDist(rng);
            // unsigned int blue = colorDist(rng);
            // b.color = sf::Color(red,green,blue);
            //add random greyscale color
            int color = colorDist(rng);

            std::cout << "Boid " << i
            << " color: "
            << color << '\n';

            //unsigned int color = 255;
            b.color = sf::Color(color,color,color);
            
            // add point to boids and vertex array
            m_boids[i] = b;
            m_vertices[i].position = b.position;
        }
    }

    std::vector<Boid>     m_boids;
    sf::VertexArray       m_vertices;
};
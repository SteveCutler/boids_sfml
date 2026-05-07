#pragma once

#include <SFML/Graphics.hpp>
#include <random>
#include <vector>
#include <cstdint>
#include <cmath>

class BoidSystem : public sf::Drawable, public sf::Transformable
{

public:
    BoidSystem(unsigned int count, int x, int y) : m_boids(count), m_vertices(sf::PrimitiveType::Points, count), m_width(x), m_height(y)
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


            //collision check
            collision_detect(p, elapsed);
        
            //add color
            if (p.hit>0.1){
                double new_red = std::lerp(static_cast<float>(p.color.r), 255.f, p.hit);
                double new_green = std::lerp(static_cast<float>(p.color.g), 0.f, p.hit);
                double new_blue = std::lerp(static_cast<float>(p.color.b), 0.f, p.hit);

                std::cout << "new red: " << new_red << " new blue: " << new_blue << "new green: " << new_green << "\n";

                m_vertices[i].color = sf::Color(static_cast<unsigned int>(new_red),static_cast<unsigned int>(new_green),static_cast<unsigned int>(new_blue));
                p.hit -=.0001;
                
                std::cout << "Hit = " << p.hit << "\n";
                std::cout << "id = " << p.id << "\n";
                std::cout << "Color = "
                    << static_cast<int>(m_vertices[i].color.r) << " "
                    << static_cast<int>(m_vertices[i].color.g) << " "
                    << static_cast<int>(m_vertices[i].color.b) << "\n";
            }else{
                // add color
                m_vertices[i].color = p.color;
            }
            
            


            // update the position of the corresponding vertex
            m_boids[i].position.x += p.velocity.x * elapsed.asSeconds();
            m_boids[i].position.y += p.velocity.y * elapsed.asSeconds();



            //copy data over to vertex array
            m_vertices[i].position = m_boids[i].position;
            //m_vertices[i].color = m_boids[i].color;

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

    // void collision_check(Boid b){
        
    // }



    void create_tris(){
    // write function for rendering boids are tris
    }

        struct Boid
    {
        int id;
        sf::Vector2f velocity;
        sf::Vector2f position;
        sf::Color color;
        double hit;
    };
    

    void collision_detect(Boid& b, sf::Time elapsed){
        double x_delta = b.position.x + (b.velocity.x * elapsed.asSeconds());
        double y_delta = b.position.y + (b.velocity.y * elapsed.asSeconds());

        if ( x_delta > m_width or x_delta < 0){
            b.velocity.x *= -1;
            m_boids[b.id].position.x += b.velocity.x * elapsed.asSeconds();
            b.hit = 1.f;
        }
        if ( y_delta > m_height or y_delta < 0){
            b.velocity.y *= -1;
            m_boids[b.id].position.y += b.velocity.y * elapsed.asSeconds();
            b.hit = 1.f;
        }

    }

    
    void forces(){
        //write function to update pos, vel, etc
        //write helper functions for this
    }

 
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
            b.hit =0.f;

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

            // std::cout << "Boid " << i
            // << " color: "
            // << color << '\n';

            //unsigned int color = 255;
            b.color = sf::Color(color,color,color);
            
            // add point to boids and vertex array
            m_boids[i] = b;
            m_vertices[i].position = b.position;
            m_vertices[i].color = b.color;
        }
    }

    unsigned int m_width;
    unsigned int m_height;
    std::vector<Boid>     m_boids;
    sf::VertexArray       m_vertices;
};
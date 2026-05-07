#pragma once

#include <SFML/Graphics.hpp>
#include <random>
#include <vector>
#include <cstdint>
#include <cmath>

class BoidSystem : public sf::Drawable, public sf::Transformable
{

public:
    BoidSystem(unsigned int count, int x, int y) : m_boids(count*3), m_vertices(sf::PrimitiveType::Triangles, count*3), m_width(x), m_height(y)
    {
        SpawnBoids(count, x , y);
    }

    void update(sf::Time elapsed)
    {
        for (std::size_t i = 0; i < m_boids.size(); ++i)
        {
            // update the particle lifetime
            Boid& p = m_boids[i];


            //collision check
            collision_detect(p, elapsed);
        
            //add color
            //collision_tint(p);
            
            // update the position of the corresponding vertex
            m_boids[i].position.x += p.velocity.x * elapsed.asSeconds();
            m_boids[i].position.y += p.velocity.y * elapsed.asSeconds();

            //copy data over to vertex array
            create_tris(p);
            //m_vertices[i].position = m_boids[i].position;

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




        struct Boid
    {
        int id;
        sf::Vector2f velocity;
        sf::Vector2f position;
        sf::Color color;
        double hit;
    };
    
        void create_tris(Boid& b){
        sf::Vector2f point1;
        sf::Vector2f point2;
        sf::Vector2f point3;
        sf::Vector2f vel_cross;
        sf::Color color;
        int point_id = (b.id)*3;


        color = collision_tint(b);
        
        vel_cross.x = b.velocity.y * -1;
        vel_cross.y = b.velocity.x;
        
        
        //write helper function for triangle
        point1.x = b.position.x + (b.velocity.x * size);
        point1.y = b.position.y + (b.velocity.y * size);

        point2.x = b.position.x + vel_cross.x * (size*0.5);
        point2.y = b.position.y + vel_cross.y * (size*0.5);

        point3.x = b.position.x - vel_cross.x * (size*0.5);
        point3.y = b.position.y - vel_cross.y * (size*0.5);

        //create 3 points of triangle
        m_vertices[point_id].position = point1;
        m_vertices[point_id+1].position = point2;
        m_vertices[point_id+2].position = point3;

        //add colors to points
        m_vertices[point_id].color = color;
        m_vertices[point_id+1].color = color;
        m_vertices[point_id+2].color = color;



    }


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

    sf::Color collision_tint(Boid& b){
        //add color
        int point_id = (b.id)*3;
            if (b.hit>0.1){
                double new_red = std::lerp(static_cast<float>(b.color.r), 255.f, b.hit);
                double new_green = std::lerp(static_cast<float>(b.color.g), 0.f, b.hit);
                double new_blue = std::lerp(static_cast<float>(b.color.b), 0.f, b.hit);

               // std::cout << "new red: " << new_red << " new blue: " << new_blue << "new green: " << new_green << "\n";

                b.hit -=.0001;
                return sf::Color(static_cast<unsigned int>(new_red),static_cast<unsigned int>(new_green),static_cast<unsigned int>(new_blue));
                
            }else{
                // return normal color
                return b.color;
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
    double size = 1;
    std::vector<Boid>     m_boids;
    sf::VertexArray       m_vertices;
};
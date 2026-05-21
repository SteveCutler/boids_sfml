#pragma once

#include <SFML/Graphics.hpp>
#include <random>
#include <vector>
#include <cstdint>
#include <cmath>

class BoidSystem : public sf::Drawable, public sf::Transformable
{

public:
    BoidSystem(unsigned int count, int x, int y, std::size_t cell_size, std::size_t grid_width, std::size_t grid_height):
    m_boids(count), 
    m_vertices(sf::PrimitiveType::Triangles, count*3), 
    m_width(x), 
    m_height(y),
    m_grid_width(grid_width),
    m_grid_height(grid_height), 
    m_cell_num(grid_width * grid_height), 
    m_grid_cells(m_cell_num),
    m_cell_size(cell_size)
    {
        SpawnBoids(count, x , y);
    }

    void update(sf::Time elapsed)
    {
            //Cell Grid Helpers
            clear_grid();
            build_grid();
            std::vector<size_t>n_boids;
        
        
        for (std::size_t i = 0; i < m_boids.size(); ++i)
        {
            sf::Vector2f force_vel = sf::Vector2f(0.f,0.f);

            Boid& b = m_boids[i];

            //collision check
            collision_detect(b, elapsed);

            //calculate cell
            size_t cell = calc_cell(b);
            //find neighbours
            n_boids = retrieve_neighbours(cell);

                    
            //apply boid forces
            force_vel = seperation(b, elapsed, n_boids);  
            force_vel += alignment(b, elapsed, n_boids) * align_master;
            force_vel += attract(b, elapsed, n_boids) * attract_master;

            //master fade control on forces
            force_vel.x = std::lerp(0.f,force_vel.x,force_strength);
            force_vel.y = std::lerp(0.f,force_vel.y,force_strength);


            //adding new force vector to velocity
            b.velocity.x += force_vel.x*b.scale * master_force;
            b.velocity.y += force_vel.y*b.scale * master_force;

            // update the position of the corresponding vertex
            m_boids[i].position.x += b.velocity.x * elapsed.asSeconds();
            m_boids[i].position.y += b.velocity.y * elapsed.asSeconds();

            b.velocity.x = std::clamp(b.velocity.x, -vel_clamp,vel_clamp);
            b.velocity.y = std::clamp(b.velocity.y, -vel_clamp,vel_clamp);

            //render boids as triangles
            
        }
        create_tris();
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
        double scale;
    };
    
        void create_tris(){
        sf::Vector2f point1;
        sf::Vector2f point2;
        sf::Vector2f point3;
        sf::Vector2f vel_cross;
        sf::Color color;
        
        for (Boid& b :m_boids){
            sf::Vector2f norm_vel = normalize(b.velocity);

            int point_id = (b.id)*3;
    
            color = collision_tint(b);
            
            vel_cross.x = norm_vel.y * -1;
            vel_cross.y = norm_vel.x;
            
            
            //write helper function for triangle
            point1.x = b.position.x + (norm_vel.x * size * b.scale);
            point1.y = b.position.y + (norm_vel.y * size)* b.scale;
    
            point2.x = b.position.x + vel_cross.x * (size*0.5*b.scale);
            point2.y = b.position.y + vel_cross.y * (size*0.5*b.scale);
    
            point3.x = b.position.x - vel_cross.x * (size*0.5*b.scale);
            point3.y = b.position.y - vel_cross.y * (size*0.5*b.scale);
    
            //create 3 points of triangle
            m_vertices[point_id].position = point1;
            m_vertices[point_id+1].position = point2;
            m_vertices[point_id+2].position = point3;
    
            //add colors to points
            m_vertices[point_id].color = color;
            m_vertices[point_id+1].color = color;
            m_vertices[point_id+2].color = color;
        }

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

                b.hit -=.005;
                return sf::Color(static_cast<unsigned int>(new_red),static_cast<unsigned int>(new_green),static_cast<unsigned int>(new_blue));
                
            }else{
                // return normal color
                return b.color;
            }
    }

    // HELPERS

    
    std::size_t calc_cell(Boid& b){
        // calc x value by dividing the x pos by width of cell
        std::size_t cell_x = (b.position.x/m_cell_size);
        // calc y value by dividing the y pos by height of cell
        std::size_t cell_y = (b.position.y/m_cell_size);

        // multiply the height by the y index and then add the x index
        std::size_t cell_num = cell_y*m_grid_width+cell_x;

        return cell_num;
    }

    void clear_grid(){
        //clear all grid cells
        for(auto& cell : m_grid_cells){
            cell.clear();
        }
    }
    
    void build_grid(){
        //loop through all boids and assign cell numbers
        for (Boid& boid : m_boids){
            size_t cell = calc_cell(boid);
            m_grid_cells[cell].push_back(boid.id);
        }     
    }

    std::pair<size_t, size_t> retrieve_coords(size_t cell_num){
        //calc x index by moduloing by grid width
        size_t x = cell_num % m_grid_width;

        //calc y index by integer dividing by grid width
        size_t y = cell_num / m_grid_width;

        //return as std::pair
        return std::pair(x,y);
    }

    std::vector<size_t> retrieve_neighbours(size_t cell_num){
        //retrieve x and y cell coordinates
        std::pair<size_t, size_t> coords = retrieve_coords(cell_num);
        //create empty list
        std::vector<size_t> n_boids;

        for (int dx = -1; dx <= 1; dx++){
            for (int dy = -1; dy <= 1; dy++){

                int x = static_cast<int>(coords.first) + dx;
                int y = static_cast<int>(coords.second) + dy;

                if ( x>=0 && x < m_grid_width &&
                    y>=0 && y < m_grid_height)
                    {

                        size_t cell = y*m_grid_width+x;

                        if(!m_grid_cells[cell].empty()){
                        //add member ids to neighbour list
                        for(auto& num : m_grid_cells[cell])
                            n_boids.push_back(num);
                        }

                    }
            }
        }
        return n_boids;
    }

    std::vector<size_t> find_b_neighbours(Boid& b){
        std::vector<size_t> n_boids;

        //cell search logic
        std::size_t cell = calc_cell(b);
        std::vector<size_t> n_list = retrieve_neighbours(cell);

        return n_list;
    }

    /*
    for each boid:
    retrieve neighbours
    then run the checking tests for distance etc
    */


    float distance(sf::Vector2f pos1, sf::Vector2f pos2){
        float dist = sqrt(::pow((pos2.x - pos1.x),2) + std::pow((pos2.y - pos1.y),2));
        return dist;
    }


    float calc_length(sf::Vector2f vector){
        return sqrt(std::pow(vector.x,2)+std::pow(vector.y,2));
    }

    void move_boid (Boid& b, sf::Time elapsed){
        b.position.x += b.velocity.x*elapsed.asSeconds();
        b.position.y += b.velocity.y*elapsed.asSeconds();
    }


    sf::Vector2f normalize (sf::Vector2f vector){
        double length = calc_length(vector);
        return sf::Vector2f((vector.x/length),vector.y/length);

    }

    sf::Vector2f calc_vector(sf::Vector2f b1, sf::Vector2f b2){
        return sf::Vector2f((b2.x - b1.x),(b2.y-b1.y));
    }
    
    // FORCES



    // SEPERATE
    sf::Vector2f seperation(Boid& b, sf::Time elapsed, std::vector<size_t> n_list){


        //create empty sf vector
        sf::Vector2f seperation_force = sf::Vector2f(0.f,0.f);

        //check if any neighbours found in surrounding cells
        if (n_list.size()>1){
            for (size_t& n_boid : n_list){
                
                //distance check
                float dist = distance(b.position,m_boids[n_boid].position);
                if(dist < seperation_dist and n_boid != b.id){
    
                   
                   //determine fade effect based on distance
                    float fade = std::clamp( seperation_dist-(dist) ,0.001f,seperation_dist)/seperation_dist;
                    
                    //determine collision vector
                    seperation_force += normalize(calc_vector(m_boids[n_boid].position, b.position))*fade;
                }
            }

        }
       // seperation_force =  normalize(seperation_force);
        return seperation_force;
    }

    // ALIGN
    sf::Vector2f alignment(Boid& b,  sf::Time elapsed, std::vector<size_t> n_list){
      
        
        sf::Vector2f align_force = sf::Vector2f(0.f,0.f);
        float counter = 0.f;
        float fade = 1.f;
        //check if any neighbours found in surrounding cells
        if (n_list.size()>1){
            for (size_t& n_boid : n_list){
                

                float dist = distance(b.position,m_boids[n_boid].position);
                

                if(dist<align_dist and n_boid != b.id){
                    
                    //determine fade effect based on distance
                    fade = 1-(std::clamp((align_dist-dist) ,0.001f,align_dist)/(align_dist));       

                    //calculate new velocity
                    float new_x = std::lerp(b.velocity.x, m_boids[n_boid].velocity.x, fade);
                    float new_y = std::lerp(b.velocity.y, m_boids[n_boid].velocity.y, fade);

                    //create new vel vector
                    align_force += normalize(sf::Vector2f(new_x, new_y))*fade;
                    counter+=1;
                    }
                }
        }
        if(counter>0.f){
            return (align_force/counter)*fade;
        }   
        else{
            return align_force;
        }
    }

    // ATTRACT

    sf::Vector2f attract(Boid& b, sf::Time elapsed, std::vector<size_t> n_list){

        
       // std::vector<Boid> boids;
        float sumX = 0.f;
        float sumY = 0.f;
        std::size_t count = 0;

        sf::Vector2f attractor;

        //check if any neighbours found in surrounding cells
        if (n_list.size()>1){
            for (size_t& n_boid : n_list){
                double dist = distance(b.position,m_boids[n_boid].position);
                if(dist<attract_dist and n_boid != b.id){

                    //loop through neighbouring boids and if close enough add their position to the sums
                    sumX += m_boids[n_boid].position.x;
                    sumY += m_boids[n_boid].position.y;
                    count ++;
                }   
            }   
        }

        //check if any close neighbouring boids were found
        if (count > 0){

            float av_x = 0.f;
            float av_y = 0.f;

            //average out positions by dividing by the number of boids
            av_x = sumX/count;
            av_y = sumY/count;

            //mean position of surrounding boids
            attractor = sf::Vector2f(av_x,av_y);

            //calculate scalar distance to attractor
            float dist = distance(b.position,attractor);

            //determine fade effect based on distance, clamp above 0
            float fade = std::clamp( attract_dist-(dist) ,0.001f,attract_dist)/attract_dist;       

            sf::Vector2f attract_vector = calc_vector(b.position, attractor) * fade;

            return attract_vector;
        }
        else{

            return sf::Vector2f(0.f,0.f);
        }
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
            b.scale = (std::uniform_real_distribution(0.6f, 1.1f)(rng));

            // give a random birth position to the boid
            b.position = sf::Vector2f(std::uniform_real_distribution(0.f, static_cast<float>(x))(rng), std::uniform_real_distribution(0.f, static_cast<float>(y))(rng));

            //give random birth vel and angle to boid
            const double angle       = (std::uniform_real_distribution(0.f, 360.f)(rng));
            float radians = angle * (M_PI/ 180.f);
            const float  speed       = std::uniform_real_distribution(1.f, 10.f)(rng);
            

            // convert angle and speed into vel
            double vx = std::cos(radians)*speed;
            double vy = std::sin(radians)*speed;
            b.velocity = sf::Vector2f(vx, vy);

            //add random color option

            // unsigned int red = colorDist(rng);
            // unsigned int green = colorDist(rng);
            // unsigned int blue = colorDist(rng);
            // b.color = sf::Color(red,green,blue);


            //add random greyscale color
            int color = colorDist(rng);

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
    float force_strength = .5;
    float size = 5;
    //Force control
    float seperation_dist = 15;

    float align_dist = 50;
    float align_master = .4;

    float attract_dist = 55;
    float attract_master = .3;

    float vel_clamp = 120.0;

    float master_force = 1.3;
    std::vector<Boid>     m_boids;
    sf::VertexArray       m_vertices;
    std::size_t m_cell_num;
    std::vector<std::vector<std::size_t>> m_grid_cells;
    std::size_t m_grid_width;
    std::size_t m_grid_height;
    std::size_t m_cell_size;
};
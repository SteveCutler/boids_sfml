cmake_minimum_required(VERSION 3.16)
project(boids_v1)

set(CMAKE_CXX_STANDARD 17)

find_package(SFML COMPONENTS Graphics Window System REQUIRED)

add_executable(boids_v1 src/main.cpp)

target_link_libraries(boids_v1
    PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
)
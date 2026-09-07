cmake_minimum_required(VERSION 3.16)
project(Boids LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)
include(CTest)

function(boids_target target)
    target_link_libraries(${target} PRIVATE SFML::Graphics SFML::Window SFML::System)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_executable(Boids src/main.cpp)
boids_target(Boids)
# Build-tree executables can start from any working directory. Reconfigure if
# the source checkout moves; standalone application packaging is not provided.
target_compile_definitions(Boids PRIVATE
    "BOIDS_FONT_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/RobotoMono.ttf\"")

if(BUILD_TESTING)
    add_executable(boids_tests tests/boids_tests.cpp)
    boids_target(boids_tests)
    foreach(case grid_reference radius_coverage dense_neighbours vector_safety
                 finite_state boundaries time_scaling)
        add_test(NAME ${case} COMMAND boids_tests ${case})
    endforeach()
endif()

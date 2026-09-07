#include "BoidSystem.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

struct BoidSystemTestAccess
{
    static void set(BoidSystem& system, std::size_t i, sf::Vector2f position,
                    sf::Vector2f velocity = {})
    {
        system.m_boid_pos_x[i] = position.x;
        system.m_boid_pos_y[i] = position.y;
        system.m_boid_vel_x[i] = velocity.x;
        system.m_boid_vel_y[i] = velocity.y;
        system.m_scale[i] = 1.f;
        system.m_hit[i] = 0.f;
    }

    static sf::Vector2f position(const BoidSystem& system, std::size_t i)
    {
        return {system.m_boid_pos_x[i], system.m_boid_pos_y[i]};
    }

    static sf::Vector2f velocity(const BoidSystem& system, std::size_t i)
    {
        return {system.m_boid_vel_x[i], system.m_boid_vel_y[i]};
    }

    static void rebuild(BoidSystem& system)
    {
        system.clear_grid();
        system.build_grid();
    }

    static std::vector<std::size_t> neighbours(BoidSystem& system, std::size_t i)
    {
        system.retrieve_neighbours(i);
        return system.m_neighbour_buffer;
    }

    static std::vector<std::size_t> insertion_counts(const BoidSystem& system)
    {
        std::vector<std::size_t> counts(system.m_boid_pos_x.size());
        for (const auto& cell : system.m_grid_cells)
            for (const auto i : cell)
                ++counts.at(i);
        return counts;
    }

    static sf::Vector2f normalize(sf::Vector2f vector)
    {
        return BoidSystem::normalize(vector);
    }

    static bool finite(const BoidSystem& system)
    {
        for (const auto* values : {&system.m_boid_pos_x, &system.m_boid_pos_y,
                                  &system.m_boid_vel_x, &system.m_boid_vel_y,
                                  &system.m_force_x, &system.m_force_y, &system.m_hit})
            for (float value : *values)
                if (!std::isfinite(value))
                    return false;
        for (std::size_t i = 0; i < system.m_vertices.getVertexCount(); ++i)
        {
            const auto p = system.m_vertices[i].position;
            if (!std::isfinite(p.x) || !std::isfinite(p.y))
                return false;
        }
        return true;
    }

    static void disable_forces(BoidSystem& system) { system.master_force = 0.f; }
    static void set_tint(BoidSystem& system, float value) { system.m_hit[0] = value; }
    static float tint(const BoidSystem& system) { return system.m_hit[0]; }
    static void geometry(BoidSystem& system) { system.create_tris(); }
};

using Access = BoidSystemTestAccess;

void require(bool condition, const std::string& message)
{
    // Unlike assert(), checks remain active in Release builds.
    if (!condition)
        throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance = 1e-5f)
{
    return std::abs(a - b) <= tolerance;
}

void require_inside(const BoidSystem& system, std::size_t count, float width, float height)
{
    for (std::size_t i = 0; i < count; ++i)
    {
        const auto p = Access::position(system, i);
        require(p.x >= 0.f && p.x <= width && p.y >= 0.f && p.y <= height,
                "Boid center outside world");
    }
}

void grid_reference()
{
    // Include borders, partial edge cells, and a seeded integer-coordinate scene.
    std::vector<sf::Vector2f> positions{{0.f, 0.f}, {513.f, 257.f}, {513.f, 0.f},
                                        {0.f, 257.f}, {64.f, 64.f}, {63.f, 100.f},
                                        {128.f, 100.f}, {168.f, 100.f}};
    std::mt19937 rng(1234);
    std::uniform_int_distribution<int> x_dist(0, 513);
    std::uniform_int_distribution<int> y_dist(0, 257);
    for (std::size_t i = 0; i < 64; ++i)
        positions.emplace_back(static_cast<float>(x_dist(rng)), static_cast<float>(y_dist(rng)));

    for (const std::size_t cell_size : {32u, 64u, 128u})
    {
        BoidSystem system(positions.size(), 513, 257, cell_size, 1);
        for (int pass = 0; pass < 2; ++pass)
        {
            // Move all boids on the second rebuild to expose stale/duplicate cells.
            std::vector<sf::Vector2f> scene = positions;
            if (pass == 1)
                for (auto& p : scene)
                    p = {513.f - p.x, 257.f - p.y};
            for (std::size_t i = 0; i < scene.size(); ++i)
                Access::set(system, i, scene[i]);
            Access::rebuild(system);
            for (const auto count : Access::insertion_counts(system))
                require(count == 1, "Each boid must appear in exactly one grid cell");

            for (std::size_t i = 0; i < scene.size(); ++i)
            {
                std::vector<std::size_t> expected;
                for (std::size_t j = 0; j < scene.size(); ++j)
                {
                    if (j == i)
                        continue;
                    const double dx = static_cast<double>(scene[j].x) - scene[i].x;
                    const double dy = static_cast<double>(scene[j].y) - scene[i].y;
                    if (std::hypot(dx, dy) < 105.0)
                        expected.push_back(j);
                }
                auto actual = Access::neighbours(system, i);
                std::sort(actual.begin(), actual.end());
                require(actual == expected, "Grid disagrees with brute-force reference");
            }
        }
    }
}

void radius_coverage()
{
    BoidSystem system(5, 512, 256, 64, 2);
    Access::set(system, 0, {63.f, 100.f});   // Cell x=0.
    Access::set(system, 1, {128.f, 100.f});  // Cell x=2; only 65 units away.
    Access::set(system, 2, {128.f, 165.f});  // Also within radius, diagonally.
    Access::set(system, 3, {168.f, 100.f});  // Exactly 105: strict cutoff excludes it.
    Access::set(system, 4, {169.f, 100.f});  // Outside radius.
    Access::rebuild(system);
    auto neighbours = Access::neighbours(system, 0);
    std::sort(neighbours.begin(), neighbours.end());
    require(neighbours == std::vector<std::size_t>{1, 2}, "Missing two-cell neighbour or incorrect cutoff");
    const auto reverse = Access::neighbours(system, 1);
    require(std::find(reverse.begin(), reverse.end(), 0) != reverse.end(), "Neighbour search must be symmetric");
}

void dense_neighbours()
{
    constexpr std::size_t count = 80;
    BoidSystem system(count, 512, 256, 64, 3);
    for (std::size_t i = 0; i < count; ++i)
        Access::set(system, i, {100.f + static_cast<float>(i) * 0.01f, 100.f});
    Access::rebuild(system);
    for (std::size_t i = 0; i < count; ++i)
    {
        auto neighbours = Access::neighbours(system, i);
        std::sort(neighbours.begin(), neighbours.end());
        require(neighbours.size() == count - 1, "Dense cell must not truncate neighbours");
        require(std::find(neighbours.begin(), neighbours.end(), i) == neighbours.end(), "Self was included");
        require(std::adjacent_find(neighbours.begin(), neighbours.end()) == neighbours.end(), "Duplicate neighbour");
    }
}

void vector_safety()
{
    require(Access::normalize({}) == sf::Vector2f{}, "Zero normalization must return zero");
    require(Access::normalize({1e-9f, -1e-9f}) == sf::Vector2f{}, "Tiny normalization must return zero");
    const auto unit = Access::normalize({3.f, 4.f});
    require(near(unit.x, 0.6f) && near(unit.y, 0.8f), "Ordinary normalization changed");

    BoidSystem coincident(2, 512, 256, 64, 4);
    Access::set(coincident, 0, {200.f, 100.f});
    Access::set(coincident, 1, {200.f, 100.f});
    for (int step = 0; step < 120; ++step)
    {
        coincident.update(sf::microseconds(16667));
        require(Access::finite(coincident), "Coincident, stationary boids produced nonfinite state");
    }
    require(Access::position(coincident, 0) == sf::Vector2f{200.f, 100.f},
            "Coincident zero vectors must not invent a direction");

    // Halfway inside the 55-unit alignment radius, opposite velocities cancel
    // exactly in lerp. This exercises the normalization call inside alignment.
    BoidSystem cancelling(2, 512, 256, 64, 5);
    Access::set(cancelling, 0, {100.f, 100.f}, {1.f, 0.f});
    Access::set(cancelling, 1, {127.5f, 100.f}, {-1.f, 0.f});
    cancelling.update(sf::microseconds(16667));
    require(Access::finite(cancelling), "Cancelling alignment velocities produced nonfinite state");
}

void finite_state()
{
    BoidSystem system(100, 512, 256, 64, 2026);
    for (int step = 0; step < 600; ++step)
    {
        system.update(sf::microseconds(16667));
        require(Access::finite(system), "Seeded simulation produced nonfinite state");
        require_inside(system, 100, 512.f, 256.f);
    }
    BoidSystem empty(0, 512, 256, 64, 1);
    empty.update(sf::seconds(1.f));
    require(Access::finite(empty), "Empty simulation failed");
}

void boundaries()
{
    BoidSystem system(4, 512, 256, 64, 6);
    Access::disable_forces(system);
    Access::set(system, 0, {511.f, 255.f}, {150.f, 150.f});
    Access::set(system, 1, {1.f, 1.f}, {-150.f, -150.f});
    Access::set(system, 2, {511.f, 1.f}, {150.f, -150.f});
    Access::set(system, 3, {1.f, 255.f}, {-150.f, 150.f});
    system.update(sf::seconds(10.f)); // Crosses multiple world widths/heights.
    require_inside(system, 4, 512.f, 256.f);
    require(Access::finite(system), "Large boundary step produced nonfinite state");
    for (std::size_t i = 0; i < 4; ++i)
    {
        const auto p = Access::position(system, i);
        const auto v = Access::velocity(system, i);
        require((p.x == 0.f && v.x > 0.f) || (p.x == 512.f && v.x < 0.f), "X velocity must point inward");
        require((p.y == 0.f && v.y > 0.f) || (p.y == 256.f && v.y < 0.f), "Y velocity must point inward");
    }
    system.update(sf::milliseconds(10));
    for (std::size_t i = 0; i < 4; ++i)
    {
        const auto p = Access::position(system, i);
        require(p.x > 0.f && p.x < 512.f && p.y > 0.f && p.y < 256.f, "Boid remained stuck on boundary");
    }
}

void time_scaling()
{
    BoidSystem short_step(2, 512, 256, 64, 7);
    BoidSystem long_step(2, 512, 256, 64, 7);
    for (auto* system : {&short_step, &long_step})
    {
        Access::set(*system, 0, {100.f, 100.f});
        Access::set(*system, 1, {120.f, 100.f});
    }
    short_step.update(sf::milliseconds(10));
    long_step.update(sf::milliseconds(20));
    // Both evaluate the same initial scene. Acceleration must scale with dt.
    const auto short_velocity = Access::velocity(short_step, 0);
    const auto long_velocity = Access::velocity(long_step, 0);
    require(short_velocity.x > 0.f, "Single neighbour must contribute cohesion");
    require(near(long_velocity.x, 2.f * short_velocity.x), "Force increment must scale with elapsed time");
    // At distance 20, cohesion is 20*(20/105)*0.8, then the original master
    // weights (0.5*0.5), converted from increments at 60 Hz to acceleration.
    const float expected = 20.f * (20.f / 105.f) * 0.8f * 0.25f * 60.f * 0.01f;
    require(near(short_velocity.x, expected), "60 Hz calibration changed the original force weights");

    BoidSystem one_step(1, 512, 256, 64, 8);
    BoidSystem split_steps(1, 512, 256, 64, 8);
    for (auto* system : {&one_step, &split_steps})
    {
        Access::set(*system, 0, {100.f, 100.f}, {10.f, 5.f});
        Access::set_tint(*system, 1.f);
    }
    one_step.update(sf::milliseconds(100));
    for (int i = 0; i < 10; ++i)
        split_steps.update(sf::milliseconds(10));
    require(near(Access::tint(one_step), 0.94f), "Tint must decay per second");
    require(near(Access::tint(one_step), Access::tint(split_steps)), "Tint depends on update count");
    const auto p = Access::position(split_steps, 0);
    require(near(p.x, 101.f, 1e-4f) && near(p.y, 100.5f, 1e-4f), "Free motion depends on update count");

    const auto before = Access::position(one_step, 0);
    const float tint_before = Access::tint(one_step);
    Access::geometry(one_step);
    Access::geometry(one_step);
    one_step.update(sf::Time::Zero);
    require(Access::position(one_step, 0) == before && Access::tint(one_step) == tint_before,
            "Geometry generation or zero elapsed time mutated simulation state");
}

int main(int argc, char** argv)
{
    const std::array<std::pair<std::string_view, void (*)()>, 7> tests{{
        {"grid_reference", grid_reference}, {"radius_coverage", radius_coverage},
        {"dense_neighbours", dense_neighbours}, {"vector_safety", vector_safety},
        {"finite_state", finite_state}, {"boundaries", boundaries}, {"time_scaling", time_scaling}
    }};
    try
    {
        if (argc != 2)
            throw std::runtime_error("Expected one test case name");
        for (const auto& [name, test] : tests)
        {
            if (name == argv[1])
            {
                test();
                std::cout << "PASS: " << name << '\n';
                return 0;
            }
        }
        throw std::runtime_error("Unknown test case");
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}

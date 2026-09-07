#pragma once

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <random>
#include <stdexcept>
#include <vector>

// Drawing scaffold adapted from SFML's vertex-array / particle-system example.
// The flocking, SoA state and spatial grid are project additions. See THIRD_PARTY.md.
class BoidSystem : public sf::Drawable, public sf::Transformable
{
public:
    BoidSystem(std::size_t count, unsigned int width, unsigned int height,
               std::size_t cell_size, std::uint32_t seed = std::random_device{}()) :
        m_width(checked_extent(width)),
        m_height(checked_extent(height)),
        m_cell_size(checked_cell_size(cell_size)),
        m_grid_width(1 + (m_width - 1) / m_cell_size),
        m_grid_height(1 + (m_height - 1) / m_cell_size),
        m_grid_cells(m_grid_width * m_grid_height),
        m_boid_pos_x(count),
        m_boid_pos_y(count),
        m_boid_vel_x(count),
        m_boid_vel_y(count),
        m_color(count),
        m_hit(count),
        m_scale(count),
        m_force_x(count),
        m_force_y(count),
        m_vertices(sf::PrimitiveType::Triangles, count * 3)
    {
        m_neighbour_buffer.reserve(count);
        SpawnBoids(seed);
        create_tris();
    }

    void update(sf::Time elapsed)
    {
        const float dt = elapsed.asSeconds();
        if (dt <= 0.f)
            return;

        clear_grid();
        build_grid();
        std::fill(m_force_x.begin(), m_force_x.end(), 0.f);
        std::fill(m_force_y.begin(), m_force_y.end(), 0.f);

        // Read the same position/velocity snapshot for every boid's forces.
        for (std::size_t i = 0; i < m_boid_pos_x.size(); ++i)
        {
            retrieve_neighbours(i);
            separation(i);
            alignment(i);
            attract(i);
        }

        apply_forces(dt);
        integrate(dt);
        resolve_boundaries();
        create_tris();
    }

private:
    // Narrow access for deterministic regression tests without a runtime debug API.
    friend struct BoidSystemTestAccess;

    static unsigned int checked_extent(unsigned int extent)
    {
        if (extent < 7)
            throw std::invalid_argument("World dimensions must be at least 7 units");
        return extent;
    }

    static std::size_t checked_cell_size(std::size_t cell_size)
    {
        if (cell_size == 0)
            throw std::invalid_argument("Cell size must be positive");
        return cell_size;
    }

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        states.transform *= getTransform();
        states.texture = nullptr;
        target.draw(m_vertices, states);
    }

    void create_tris()
    {
        for (std::size_t i = 0; i < m_boid_pos_x.size(); ++i)
        {
            sf::Vector2f heading = normalize({m_boid_vel_x[i], m_boid_vel_y[i]});
            // A stationary boid still has visible, finite geometry.
            if (heading == sf::Vector2f{})
                heading = {1.f, 0.f};
            const sf::Vector2f perpendicular{-heading.y, heading.x};
            const sf::Vector2f position{m_boid_pos_x[i], m_boid_pos_y[i]};
            const float length = size * m_scale[i];
            const std::size_t vertex = i * 3;
            m_vertices[vertex].position = position + heading * length;
            m_vertices[vertex + 1].position = position + perpendicular * (0.5f * length);
            m_vertices[vertex + 2].position = position - perpendicular * (0.5f * length);
            const sf::Color color = collision_tint(i);
            for (std::size_t offset = 0; offset < 3; ++offset)
                m_vertices[vertex + offset].color = color;
        }
    }

    sf::Color collision_tint(std::size_t i) const
    {
        if (m_hit[i] <= 0.1f)
            return m_color[i];
        return {
            static_cast<std::uint8_t>(std::lerp(static_cast<float>(m_color[i].r), 255.f, m_hit[i])),
            static_cast<std::uint8_t>(std::lerp(static_cast<float>(m_color[i].g), 0.f, m_hit[i])),
            static_cast<std::uint8_t>(std::lerp(static_cast<float>(m_color[i].b), 0.f, m_hit[i]))
        };
    }

    std::size_t calc_cell(std::size_t i) const
    {
        const float cell_size = static_cast<float>(m_cell_size);
        const auto x = static_cast<std::size_t>(std::clamp(
            m_boid_pos_x[i] / cell_size, 0.f, static_cast<float>(m_grid_width - 1)));
        const auto y = static_cast<std::size_t>(std::clamp(
            m_boid_pos_y[i] / cell_size, 0.f, static_cast<float>(m_grid_height - 1)));
        return y * m_grid_width + x;
    }

    void clear_grid()
    {
        for (auto& cell : m_grid_cells)
            cell.clear(); // Retain capacity for later frames.
    }

    void build_grid()
    {
        for (std::size_t i = 0; i < m_boid_pos_x.size(); ++i)
            m_grid_cells[calc_cell(i)].push_back(i);
    }

    void retrieve_neighbours(std::size_t i)
    {
        m_neighbour_buffer.clear();
        const float radius = std::max({separation_dist, align_dist, attract_dist});
        const auto extent = static_cast<std::size_t>(
            std::ceil(radius / static_cast<float>(m_cell_size)));
        const std::size_t cell = calc_cell(i);
        const std::size_t x = cell % m_grid_width;
        const std::size_t y = cell / m_grid_width;
        const std::size_t min_x = x > extent ? x - extent : 0;
        const std::size_t min_y = y > extent ? y - extent : 0;
        const std::size_t max_x = x + std::min(extent, m_grid_width - 1 - x);
        const std::size_t max_y = y + std::min(extent, m_grid_height - 1 - y);

        // No arbitrary cap: include all other boids strictly inside the largest
        // rule radius. Individual rules apply their own smaller distance tests.
        for (std::size_t cx = min_x; cx <= max_x; ++cx)
        {
            for (std::size_t cy = min_y; cy <= max_y; ++cy)
            {
                for (const std::size_t neighbour : m_grid_cells[cy * m_grid_width + cx])
                {
                    if (neighbour == i)
                        continue;
                    const float dx = m_boid_pos_x[neighbour] - m_boid_pos_x[i];
                    const float dy = m_boid_pos_y[neighbour] - m_boid_pos_y[i];
                    if (dx * dx + dy * dy < radius * radius)
                        m_neighbour_buffer.push_back(neighbour);
                }
            }
        }
    }

    static float distance(sf::Vector2f a, sf::Vector2f b)
    {
        const sf::Vector2f delta = b - a;
        return std::sqrt(delta.x * delta.x + delta.y * delta.y);
    }

    static sf::Vector2f normalize(sf::Vector2f vector)
    {
        const float length = std::hypot(vector.x, vector.y);
        constexpr float minimum_length = 1e-6f;
        if (!std::isfinite(length) || length <= minimum_length)
            return {};
        return vector / length;
    }

    void apply_forces(float dt)
    {
        // Preserve the old per-frame force increment at a 60 Hz reference rate.
        // Multiplying by dt makes these weights accelerations in units/second².
        const float force_scale = reference_rate * dt * force_strength * master_force;
        for (std::size_t i = 0; i < m_boid_pos_x.size(); ++i)
        {
            m_boid_vel_x[i] += m_force_x[i] * m_scale[i] * force_scale;
            m_boid_vel_y[i] += m_force_y[i] * m_scale[i] * force_scale;
            m_boid_vel_x[i] = std::clamp(m_boid_vel_x[i], -vel_clamp, vel_clamp);
            m_boid_vel_y[i] = std::clamp(m_boid_vel_y[i], -vel_clamp, vel_clamp);
        }
    }

    void integrate(float dt)
    {
        for (std::size_t i = 0; i < m_boid_pos_x.size(); ++i)
        {
            m_boid_pos_x[i] += m_boid_vel_x[i] * dt;
            m_boid_pos_y[i] += m_boid_vel_y[i] * dt;
            m_hit[i] = std::max(0.f, m_hit[i] - tint_decay_per_second * dt);
        }
    }

    void resolve_boundaries()
    {
        for (std::size_t i = 0; i < m_boid_pos_x.size(); ++i)
        {
            // Clamp the boid's center and point velocity inward. This deliberately
            // discards overshoot rather than simulating multiple wall bounces.
            const auto resolve_axis = [&](float& position, float& velocity, float limit)
            {
                if (position < 0.f)
                {
                    position = 0.f;
                    velocity = std::abs(velocity);
                    m_hit[i] = 1.f;
                }
                else if (position > limit)
                {
                    position = limit;
                    velocity = -std::abs(velocity);
                    m_hit[i] = 1.f;
                }
            };
            resolve_axis(m_boid_pos_x[i], m_boid_vel_x[i], static_cast<float>(m_width));
            resolve_axis(m_boid_pos_y[i], m_boid_vel_y[i], static_cast<float>(m_height));
        }
    }

    void separation(std::size_t i)
    {
        const sf::Vector2f position{m_boid_pos_x[i], m_boid_pos_y[i]};
        sf::Vector2f force{};
        for (const std::size_t neighbour : m_neighbour_buffer)
        {
            const sf::Vector2f other{m_boid_pos_x[neighbour], m_boid_pos_y[neighbour]};
            const float dist = distance(position, other);
            if (dist < separation_dist)
            {
                const float fade = std::clamp(separation_dist - dist, 0.001f, separation_dist)
                                   / separation_dist;
                force += normalize(position - other) * fade;
            }
        }
        m_force_x[i] += force.x;
        m_force_y[i] += force.y;
    }

    void alignment(std::size_t i)
    {
        const sf::Vector2f position{m_boid_pos_x[i], m_boid_pos_y[i]};
        sf::Vector2f force{};
        std::size_t count = 0;
        for (const std::size_t neighbour : m_neighbour_buffer)
        {
            const sf::Vector2f other{m_boid_pos_x[neighbour], m_boid_pos_y[neighbour]};
            const float dist = distance(position, other);
            if (dist < align_dist)
            {
                const float fade = std::clamp(align_dist - dist, 0.001f, align_dist) / align_dist;
                const sf::Vector2f velocity{
                    std::lerp(m_boid_vel_x[i], m_boid_vel_x[neighbour], fade),
                    std::lerp(m_boid_vel_y[i], m_boid_vel_y[neighbour], fade)
                };
                force += normalize(velocity) * fade;
                ++count;
            }
        }
        if (count > 0)
        {
            m_force_x[i] += force.x / static_cast<float>(count) * align_master;
            m_force_y[i] += force.y / static_cast<float>(count) * align_master;
        }
    }

    void attract(std::size_t i)
    {
        const sf::Vector2f position{m_boid_pos_x[i], m_boid_pos_y[i]};
        sf::Vector2f sum{};
        std::size_t count = 0;
        for (const std::size_t neighbour : m_neighbour_buffer)
        {
            const sf::Vector2f other{m_boid_pos_x[neighbour], m_boid_pos_y[neighbour]};
            if (distance(position, other) < attract_dist)
            {
                sum += other;
                ++count;
            }
        }
        if (count > 0)
        {
            const sf::Vector2f attractor = sum / static_cast<float>(count);
            const float dist = distance(position, attractor);
            const float fade = 1.f - std::clamp(attract_dist - dist, 0.001f, attract_dist)
                                    / attract_dist;
            const sf::Vector2f force = (attractor - position) * fade;
            m_force_x[i] += force.x * attract_master;
            m_force_y[i] += force.y * attract_master;
        }
    }

    void SpawnBoids(std::uint32_t seed)
    {
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> color_dist(50, 250);
        for (std::size_t i = 0; i < m_boid_pos_x.size(); ++i)
        {
            m_scale[i] = std::uniform_real_distribution<float>(0.6f, 1.1f)(rng);
            m_boid_pos_x[i] = std::uniform_real_distribution<float>(3.f, static_cast<float>(m_width) - 3.f)(rng);
            m_boid_pos_y[i] = std::uniform_real_distribution<float>(3.f, static_cast<float>(m_height) - 3.f)(rng);
            const float angle = std::uniform_real_distribution<float>(0.f, 2.f * std::numbers::pi_v<float>)(rng);
            const float speed = std::uniform_real_distribution<float>(1.f, 10.f)(rng);
            m_boid_vel_x[i] = std::cos(angle) * speed;
            m_boid_vel_y[i] = std::sin(angle) * speed;
            const auto color = static_cast<std::uint8_t>(color_dist(rng));
            m_color[i] = sf::Color(color, color, color);
        }
    }

    unsigned int m_width;
    unsigned int m_height;

    // Original artistic weights, calibrated to the old update at 60 Hz.
    static constexpr float reference_rate = 60.f;
    static constexpr float tint_decay_per_second = 0.01f * reference_rate;
    float force_strength = 0.5f;
    float size = 5.f;
    float separation_dist = 10.f;
    float align_dist = 55.f;
    float attract_dist = 105.f;
    float vel_clamp = 150.f; // Per component, not a speed-magnitude limit.
    float align_master = 1.2f;
    float attract_master = 0.8f;
    float master_force = 0.5f;

    std::size_t m_cell_size;
    std::size_t m_grid_width;
    std::size_t m_grid_height;
    std::vector<std::vector<std::size_t>> m_grid_cells;

    // A boid's index identifies it in every SoA vector.
    std::vector<float> m_boid_pos_x;
    std::vector<float> m_boid_pos_y;
    std::vector<float> m_boid_vel_x;
    std::vector<float> m_boid_vel_y;
    std::vector<sf::Color> m_color;
    std::vector<float> m_hit;
    std::vector<float> m_scale;
    std::vector<std::size_t> m_neighbour_buffer;
    std::vector<float> m_force_x;
    std::vector<float> m_force_y;
    sf::VertexArray m_vertices;
};

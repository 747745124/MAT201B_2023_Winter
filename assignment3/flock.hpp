#pragma once
#include <iostream>
#include <unordered_map>
#include <vector>
#include <iterator>
#include <string>
#include <algorithm>
#include <functional>
#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/io/al_File.hpp"
#include "al/math/al_Random.hpp"

using namespace al;

namespace Common
{
    auto max_speed = 0.3f;
    auto max_force = 1.f;
};

Vec3f limit(float max_force, Vec3f vec)
{
    if (vec.mag() > max_force)
        return vec.normalize(max_force);
    return vec;
};

// return a point that is outside a circle
Vec3f r_limiter_circle(float radius = 1)
{
    while (true)
    {
        float x = rnd::uniformS();
        float y = rnd::uniformS();
        if ((x * x + y * y) > abs(1 * radius))
        {
            return Vec3f(x, y, 0.f);
        }
    }
};

Vec3f r() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }

class Boid
{
public:
    // just for easy access
    Vec3f position;
    Vec3f velocity;
    Boid(std::function<Vec3f(void)> &f, float speed_factor = 0.001)
    {
        this->position = f();
        this->velocity = 0.01f * f();
    };

    Boid(std::function<Vec3f(float)> &f, float radius, float speed_factor = 0.001)
    {
        this->position = f(radius);
        this->velocity = Vec3f(0.f);
    };

    auto get_position() const { return position; }
    auto get_velocity() const { return velocity; }
    void set_position(Vec3f position) { this->position = std::move(position); }
    void set_velocity(Vec3f velocity) { this->velocity = std::move(velocity); }
    void add_position(Vec3f position) { this->position += position; }
    void add_velocity(Vec3f velocity) { this->velocity += velocity; }
};

class Flock
{
protected:
    std::vector<std::unique_ptr<Boid>> boids;

public:
    Flock(int boid_num, std::function<Vec3f(void)> f)
    {
        for (int i = 0; i < boid_num; i++)
        {
            boids.push_back(std::make_unique<Boid>(f));
        }
    };

    Flock(int boid_num, std::function<Vec3f(float)> f, float radius)
    {
        for (int i = 0; i < boid_num; i++)
        {
            boids.push_back(std::make_unique<Boid>(f, radius));
        }
    };

    void update_border_velocity()
    {
        for (auto &boid : boids)
        {
            auto look_ahead = boid->get_position() + 0.25f * boid->get_velocity();
            // auto look_ahead = boid->get_position();
            if (look_ahead.x > 1.f || look_ahead.x < -1.f)
            {
                boid->position.x = (boid->position.x > 0) ? 1 : -1;
                boid->velocity.x *= -1.f;
            }
            if (look_ahead.y > 1.f || look_ahead.y < -1.f)
            {
                boid->position.y = (boid->position.y > 0) ? 1 : -1;
                boid->velocity.y *= -1.f;
            }
            if (look_ahead.z > 1.f || look_ahead.z < -1.f)
            {
                boid->position.z = (boid->position.z > 0) ? 1 : -1;
                boid->velocity.z *= -1.f;
            }
        }
        return;
    }

    // separate distance
    void
    update_separate_velocity(float s_distance)
    {
        for (int i = 0; i < boids.size(); i++)
        {
            // how many boids are close
            int count = 0;
            Vec3f sum_velocity(0.f);
            for (int j = 0; j < boids.size(); j++)
            {
                if (i != j)
                {
                    auto position_diff = boids[i]->position - boids[j]->position;
                    auto distance = position_diff.mag();
                    if ((distance > 0) && (distance < s_distance))
                    {
                        count++;
                        // the closer, the larger weight
                        position_diff = position_diff.normalize();
                        position_diff /= distance;
                        sum_velocity += position_diff;
                    }
                }

                // no separation force
                if (count <= 0)
                    continue;

                sum_velocity /= (float)count;
                auto steer = sum_velocity.normalize() * Common::max_speed - boids[i]->get_velocity();
                boids[i]->add_velocity(limit(Common::max_force, steer));
            }
        }
    };

    void update_cohesion_velocity(float p_distance)
    {
        for (int i = 0; i < boids.size(); i++)
        {
            // how many boids are close
            int count = 0;
            Vec3f sum_velocity(0.f);
            for (int j = 0; j < boids.size(); j++)
            {
                if (i != j)
                {
                    auto position_diff = boids[i]->position - boids[j]->position;
                    auto distance = position_diff.mag();
                    if ((distance > 0) && (distance < p_distance))
                    {

                        count++;
                        sum_velocity += boids[j]->position;
                    }
                }

                // no separation force
                if (count <= 0)
                    continue;

                // use average other location as velocity
                sum_velocity /= (float)count;
                auto steer = sum_velocity.normalize() * Common::max_speed - boids[i]->get_velocity();
                boids[i]->add_velocity(limit(Common::max_force, steer));
            }
        }
    }

    /// perceive distance
    void update_align_velocity(float p_distance)
    {
        for (int i = 0; i < boids.size(); i++)
        {
            // how many boids are close
            int count = 0;
            Vec3f sum_velocity(0.f);
            for (int j = 0; j < boids.size(); j++)
            {
                if (i != j)
                {
                    auto distance = (boids[i]->get_position() - boids[j]->get_position()).mag();
                    if ((distance > 0) && (distance < p_distance))
                    {
                        count++;
                        sum_velocity += boids[j]->get_velocity();
                    }
                }

                // no align force
                if (count <= 0)
                    continue;

                sum_velocity /= (float)count;
                auto steer = sum_velocity.normalize() * Common::max_speed - boids[i]->get_velocity();
                boids[i]->add_velocity(limit(Common::max_force, steer));
            }
        }
    }

    void update_position(float delta_t)
    {
        for (auto &boid : boids)
        {
            boid->position += (delta_t * boid->get_velocity());
        }
    }

    std::vector<Vec3f> get_vertices() const
    {
        std::vector<Vec3f> v;
        for (const auto &boid : boids)
        {
            v.push_back(boid->get_position());
        }
        return v;
    };
};
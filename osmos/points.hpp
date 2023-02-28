#pragma once
#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/graphics/al_Graphics.hpp"

struct Point
{
    float mass;
    al::Vec2f size;
    al::Color color;
    al::Vec3f position;
    al::Vec3f acceleration;
    al::Vec3f velocity;
    float rotation_angle;

    Point(float mass, al::Vec2f size, al::Color color, al::Vec3f position, al::Vec3f acceleration, al::Vec3f velocity)
    {
        this->mass = mass;
        this->size = size;
        this->color = color;
        this->position = position;
        this->acceleration = acceleration;
        this->velocity = velocity;
        rotation_angle = rnd::normal();
    };
};
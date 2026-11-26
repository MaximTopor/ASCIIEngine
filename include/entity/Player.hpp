#pragma once

#include "world/World.hpp"

class Player
{
private:
    float x;
    float y;

    float movementSpeed = 180.0f;

public:

    Player(float x, float y);

    bool move(
        float dx,
        float dy,
        float deltaTime,
        const World& world
    );

    float getX() const;
    float getY() const;

    float getMovementSpeed() const;

    void setMovementSpeed(float speed);

    void setPosition(float x, float y);
};
#pragma once

#include "world/World.hpp"

class Player
{
private:
    int x;
    int y;

public:
    Player(int x, int y);

    bool move(
        int dx,
        int dy,
        const World& world
    );

    int getX() const;
    int getY() const;
};
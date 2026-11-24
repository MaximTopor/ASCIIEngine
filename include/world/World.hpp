#pragma once

#include "world/Tile.hpp"

#include <vector>

class World
{
private:
    int width;
    int height;

    std::vector<Tile> tiles;

    void generate();

public:
    World(int width, int height);

    bool isInside(int x, int y) const;

    bool isWalkable(int x, int y) const;

    const Tile& getTile(int x, int y) const;

    int getWidth() const;
    int getHeight() const;
};
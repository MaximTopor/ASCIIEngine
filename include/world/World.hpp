#pragma once

#include "world/Tile.hpp"

#include <vector>
#include <utility>

class World
{
private:
    int width;
    int height;
    int seed;

    std::vector<Tile> tiles;

    void generate();

public:
    World(
        int width,
        int height,
        int seed = 42
    );

    bool isInside(int x, int y) const;
    bool isWalkable(int x, int y) const;

    const Tile& getTile(int x, int y) const;

    std::pair<int, int> findSpawn() const;

    int getWidth() const;
    int getHeight() const;
};
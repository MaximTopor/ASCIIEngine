#pragma once

#include "world/Tile.hpp"

#include <FastNoiseLite.h>

class WorldGenerator
{
private:
    FastNoiseLite elevationNoise;
    FastNoiseLite moistureNoise;
    FastNoiseLite vegetationNoise;

public:
    explicit WorldGenerator(int seed = 42);

    Tile generateTile(int x, int y) const;
};
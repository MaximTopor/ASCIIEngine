#pragma once

#include "renderer/VisualGlyph.hpp"
#include "world/Tile.hpp"

#include <vector>
#include <cstdint>

class VegetationGenerator
{
private:

    static std::uint32_t hash(
        std::uint32_t value
    );

    static std::uint32_t tileSeed(
        int x,
        int y
    );

    static float random(
        std::uint32_t& state
    );

    static float densityNoise(
        int x,
        int y
    );

public:

    static void generate(
        const Tile& tile,
        int tileX,
        int tileY,
        float cellWidth,
        float cellHeight,
        std::vector<VisualGlyph>& output
    );
};
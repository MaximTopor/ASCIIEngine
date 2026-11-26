#pragma once

enum class TileType
{
    Ground,
    Grass,
    Tree,
    Water,
    Wall,

    DeepWater,
    Sand,
    Rock,
    Mountain
};

struct Tile
{
    TileType type = TileType::Ground;

    bool isWalkable() const
    {
        return type != TileType::DeepWater;
    }

    float getLightOpacity() const
    {
        return 0.0f;
    }

    bool blocksLight() const
    {
        return getLightOpacity() >= 1.0f;
    }
};

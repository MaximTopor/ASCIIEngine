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
        switch (type)
        {
            case TileType::Ground:
            case TileType::Grass:
            case TileType::Sand:
                return true;

            default:
                return false;
        }
    }

    float getLightOpacity() const
    {
        switch (type)
        {
            case TileType::Tree:
                return 0.3f;

            case TileType::Rock:
            case TileType::Mountain:
            case TileType::Wall:
                return 1.0f;

            default:
                return 0.0f;
        }
    }

    bool blocksLight() const
    {
        return getLightOpacity() >= 1.0f;
    }
};
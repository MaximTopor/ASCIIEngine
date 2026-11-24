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
};
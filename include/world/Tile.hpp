#pragma once

enum class TileType
{
    Ground,
    Grass,
    Tree,
    Water,
    Wall
};

struct Tile
{
    TileType type = TileType::Ground;

    bool isWalkable() const
    {
        return type == TileType::Ground
            || type == TileType::Grass;
    }
};
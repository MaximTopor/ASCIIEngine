#include "world/World.hpp"

#include <random>
#include <stdexcept>
#include <cstddef>

World::World(int width, int height)
    : width(width),
      height(height)
{
    if (width < 3 || height < 3)
    {
        throw std::invalid_argument(
            "Invalid world dimensions"
        );
    }

    tiles.resize(
        static_cast<std::size_t>(width) * height
    );

    generate();
}

void World::generate()
{
    std::mt19937 generator(42);

    std::uniform_int_distribution<int>
        distribution(0, 99);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            Tile& tile = tiles[
                static_cast<std::size_t>(y) * width + x
            ];

            // World boundaries
            if (x == 0 || y == 0 ||
                x == width - 1 ||
                y == height - 1)
            {
                tile.type = TileType::Wall;
                continue;
            }

            // River
            if (x == width * 2 / 3 ||
                x == width * 2 / 3 + 1)
            {
                tile.type = TileType::Water;
                continue;
            }

            const int value = distribution(generator);

            if (value < 10)
            {
                tile.type = TileType::Tree;
            }
            else if (value < 40)
            {
                tile.type = TileType::Grass;
            }
            else
            {
                tile.type = TileType::Ground;
            }
        }
    }

    // Clear the player's starting area
    for (int y = 1; y <= 3 && y < height - 1; ++y)
    {
        for (int x = 1; x <= 3 && x < width - 1; ++x)
        {
            tiles[
                static_cast<std::size_t>(y) * width + x
            ].type = TileType::Ground;
        }
    }
}

bool World::isInside(int x, int y) const
{
    return x >= 0 && x < width
        && y >= 0 && y < height;
}

bool World::isWalkable(int x, int y) const
{
    if (!isInside(x, y))
    {
        return false;
    }

    return getTile(x, y).isWalkable();
}

const Tile& World::getTile(int x, int y) const
{
    if (!isInside(x, y))
    {
        throw std::out_of_range(
            "Tile coordinates out of range"
        );
    }

    return tiles[
        static_cast<std::size_t>(y) * width + x
    ];
}

int World::getWidth() const
{
    return width;
}

int World::getHeight() const
{
    return height;
}
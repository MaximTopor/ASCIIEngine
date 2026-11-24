#include "world/World.hpp"
#include "world/WorldGenerator.hpp"

#include <stdexcept>
#include <cstddef>
#include <cstdlib>

World::World(
    int width,
    int height,
    int seed
)
    : width(width),
      height(height),
      seed(seed)
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
    WorldGenerator generator(seed);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const std::size_t index =
                static_cast<std::size_t>(y)
                * width + x;

            tiles[index] =
                generator.generateTile(x, y);
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

std::pair<int, int> World::findSpawn() const
{
    const int centerX = width / 2;
    const int centerY = height / 2;

    int bestX = -1;
    int bestY = -1;
    long long bestDistance = -1;

    for (int y = 1; y < height - 1; ++y)
    {
        for (int x = 1; x < width - 1; ++x)
        {
            if (!isWalkable(x, y))
            {
                continue;
            }

            // Require free adjacent tiles

            if (!isWalkable(x + 1, y) ||
                !isWalkable(x - 1, y) ||
                !isWalkable(x, y + 1) ||
                !isWalkable(x, y - 1))
            {
                continue;
            }

            const long long dx = x - centerX;
            const long long dy = y - centerY;

            const long long distance =
                dx * dx + dy * dy;

            if (bestDistance == -1 ||
                distance < bestDistance)
            {
                bestDistance = distance;
                bestX = x;
                bestY = y;
            }
        }
    }

    if (bestX == -1)
    {
        throw std::runtime_error(
            "No suitable spawn position found"
        );
    }

    return {bestX, bestY};
}

int World::getWidth() const
{
    return width;
}

int World::getHeight() const
{
    return height;
}
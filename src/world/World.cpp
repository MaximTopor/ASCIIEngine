#include "world/World.hpp"
#include "world/WorldGenerator.hpp"

#include <stdexcept>
#include <cstddef>
#include <algorithm>
#include <cmath>

namespace
{
    std::uint32_t hash(std::uint32_t value)
    {
        value = (value ^ (value >> 16)) * 0x7feb352du;
        value = (value ^ (value >> 15)) * 0x846ca68bu;
        return value ^ (value >> 16);
    }

    float fraction(std::uint32_t value)
    {
        return static_cast<float>(value >> 8) / 16777216.0f;
    }
}

World::World(
    int width,
    int height,
    int seed,
    WorldScale scale,
    bool benchmark
)
    : width(width),
      height(height),
      seed(seed),
      scale(scale)
{
    if (width < 3 || height < 3)
    {
        throw std::invalid_argument(
            "Invalid world dimensions"
        );
    }

    const float maxDiameter = scale.maximumRockDiameter * scale.playerHeight;
    const auto positive = [](float value) { return std::isfinite(value) && value > 0; };
    if (!positive(scale.cellWidth) || !positive(scale.cellHeight)
        || !positive(scale.playerHeight) || !positive(scale.minimumRockDiameter)
        || !positive(maxDiameter) || scale.maximumRockDiameter < scale.minimumRockDiameter
        || !positive(width * scale.cellWidth) || !positive(height * scale.cellHeight)
        || !positive(3.3f * maxDiameter))
    {
        throw std::invalid_argument("Invalid world scale");
    }

    tiles.resize(
        static_cast<std::size_t>(width) * height
    );

    rockFootprint.resize(tiles.size());
    treeTrunks.resize(tiles.size());
    if (benchmark)
    {
        generateBenchmark();
    }
    else
    {
        generate();
        generateRocks();
        generateTrees();
    }
}

void World::generateBenchmark()
{
    const float centerX = (width / 2 + 0.5f) * scale.cellWidth;
    const float centerY = (height / 2 + 0.5f) * scale.cellHeight;
    const auto seedValue = static_cast<std::uint32_t>(seed);
    FastNoiseLite detail(seed);
    detail.SetFrequency(0.021f);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const float px = (x + 0.5f) * scale.cellWidth - centerX;
            const float py = (y + 0.5f) * scale.cellHeight - centerY;
            const float stream = std::abs(px - 225.0f - 34.0f * std::sin(py * 0.011f));
            const float path = std::abs(py - 35.0f * std::sin(px * 0.009f));
            const float grain = detail.GetNoise(px, py);
            TileType type = TileType::Grass;
            if (stream < 11.0f) type = TileType::DeepWater;
            else if (stream < 26.0f) type = TileType::Water;
            else if (stream < 37.0f + grain * 5.0f) type = TileType::Sand;
            else if (path < 25.0f + grain * 8.0f || grain < -0.60f) type = TileType::Ground;
            tiles[static_cast<std::size_t>(y) * width + x] = Tile{type};
        }
    }
    placeTree(centerX - 175.0f, centerY + 238.0f, 720.0f, hash(seedValue + 1u), TreeSpecies::Oak);
    placeTree(centerX + 108.0f, centerY - 113.0f, 330.0f, hash(seedValue + 2u), TreeSpecies::Oak);
    placeTree(centerX - 218.0f, centerY - 183.0f, 440.0f, hash(seedValue + 3u), TreeSpecies::Pine);
    placeTree(centerX + 335.0f, centerY + 96.0f, 380.0f, hash(seedValue + 4u), TreeSpecies::Birch);
    placeTree(centerX + 124.0f, centerY + 196.0f, 245.0f, hash(seedValue + 5u), TreeSpecies::Birch);
    placeRock(centerX - 58.0f, centerY - 87.0f, hash(seedValue + 6u), 63.0f);
    placeRock(centerX + 62.0f, centerY + 52.0f, hash(seedValue + 7u), 23.0f);
    for (std::uint32_t i = 0; i < 150; ++i)
    {
        const auto stone = hash(seedValue + 100u + i);
        const float x = (fraction(stone) - 0.5f) * 800.0f;
        const float y = (fraction(hash(stone + 1u)) - 0.5f) * 640.0f;
        placeRock(centerX + x, centerY + y, stone, 3.0f + 7.0f * fraction(hash(stone + 2u)), false);
    }
    placeRock(centerX - 14.0f, centerY + 14.0f, hash(seedValue + 250u), 8.0f, false);
    placeRock(centerX + 19.0f, centerY + 23.0f, hash(seedValue + 251u), 5.0f, false);
}

void World::placeTree(float x, float y, float treeHeight, std::uint32_t treeSeed, TreeSpecies species)
{
    const int cx = static_cast<int>(x / scale.cellWidth);
    const int cy = static_cast<int>(y / scale.cellHeight);
    if (!isWalkable(cx, cy))
    {
        return;
    }
    trees.push_back({x, y, treeHeight, treeSeed, species});
    const float radius = treeHeight * treeBaseRadiusRatio(species);
    for (int row = std::max(0, static_cast<int>((y - radius) / scale.cellHeight));
         row <= std::min(height - 1, static_cast<int>((y + radius) / scale.cellHeight)); ++row)
    {
        for (int col = std::max(0, static_cast<int>((x - radius) / scale.cellWidth));
             col <= std::min(width - 1, static_cast<int>((x + radius) / scale.cellWidth)); ++col)
        {
            const float dx = (col + 0.5f) * scale.cellWidth - x;
            const float dy = (row + 0.5f) * scale.cellHeight - y;
            if ((cx == col && cy == row) || dx * dx + dy * dy <= radius * radius)
            {
                treeTrunks[static_cast<std::size_t>(row) * width + col] = true;
            }
        }
    }
}

void World::generateTrees()
{
    const double spacing = std::max(5.4 * scale.playerHeight,
        2.0 * std::max(scale.cellWidth, scale.cellHeight));
    const auto [spawnX, spawnY] = findSpawnCell();
    for (int row = 0; row * spacing < height * scale.cellHeight; ++row)
    {
        for (int col = 0; col * spacing < width * scale.cellWidth; ++col)
        {
            const auto treeSeed = hash(static_cast<std::uint32_t>(seed) ^ 0x1b873593u
                ^ (static_cast<std::uint32_t>(col) * 1597334677u)
                ^ (static_cast<std::uint32_t>(row) * 3812015801u));
            const float x = static_cast<float>((col + 0.22 + 0.56 * fraction(hash(treeSeed + 1u))) * spacing);
            const float y = static_cast<float>((row + 0.22 + 0.56 * fraction(hash(treeSeed + 2u))) * spacing);
            if (x >= width * scale.cellWidth || y >= height * scale.cellHeight)
            {
                continue;
            }
            const int cx = std::min(width - 1, static_cast<int>(x / scale.cellWidth));
            const int cy = std::min(height - 1, static_cast<int>(y / scale.cellHeight));
            const bool nearSpawn = spawnX >= 0 && std::abs(cx - spawnX) <= 1 && std::abs(cy - spawnY) <= 1;
            if (nearSpawn || getTile(cx, cy).type != TileType::Tree || !isWalkable(cx, cy))
            {
                continue;
            }
            const auto kind = treeSeed % 10u;
            const auto species = kind < 5 ? TreeSpecies::Oak : (kind < 8 ? TreeSpecies::Birch : TreeSpecies::Pine);
            placeTree(x, y, scale.playerHeight * (3.5f + 2.7f * fraction(treeSeed)), treeSeed, species);
        }
    }
}

bool World::placeRock(float x, float y, std::uint32_t objectSeed, float diameter, bool solid)
{
    const Rock rock{
        rocks.size(), x, y, diameter * 0.5f,
        diameter * (0.30f + 0.12f * fraction(hash(objectSeed))),
        diameter * (0.33f + 0.14f * fraction(hash(objectSeed + 1u))),
        fraction(hash(objectSeed + 2u)) * 6.2831853f, objectSeed
    };
    const float c = std::cos(rock.angle);
    const float s = std::sin(rock.angle);
    const float extentX = std::hypot(c * rock.radiusX, s * rock.radiusY);
    const float extentY = std::hypot(s * rock.radiusX, c * rock.radiusY);
    if (x - extentX < 0 || y - extentY < 0
        || x + extentX >= width * scale.cellWidth
        || y + extentY >= height * scale.cellHeight)
    {
        return false;
    }
    if (!solid)
    {
        if (!isWalkable(static_cast<int>(x / scale.cellWidth), static_cast<int>(y / scale.cellHeight)))
        {
            return false;
        }
        rocks.push_back(rock);
        return true;
    }

    std::vector<std::size_t> cells;
    const int firstX = static_cast<int>((x - extentX) / scale.cellWidth);
    const int lastX = std::min(width - 1, static_cast<int>((x + extentX) / scale.cellWidth));
    const int firstY = static_cast<int>((y - extentY) / scale.cellHeight);
    const int lastY = std::min(height - 1, static_cast<int>((y + extentY) / scale.cellHeight));
    for (int cy = firstY; cy <= lastY; ++cy)
    {
        for (int cx = firstX; cx <= lastX; ++cx)
        {
            const float dx = (cx + 0.5f) * scale.cellWidth - x;
            const float dy = (cy + 0.5f) * scale.cellHeight - y;
            if (rock.surface((dx * c + dy * s) / rock.radiusX,
                             (-dx * s + dy * c) / rock.radiusY) <= 0)
            {
                continue;
            }
            if (!isWalkable(cx, cy))
            {
                return false;
            }
            cells.push_back(static_cast<std::size_t>(cy) * width + cx);
        }
    }
    if (cells.empty())
    {
        return false;
    }
    rocks.push_back(rock);
    for (const auto index : cells)
    {
        rockFootprint[index] = true;
    }
    return true;
}

void World::generateRocks()
{
    const float maxDiameter = scale.maximumRockDiameter * scale.playerHeight;
    const float worldWidth = width * scale.cellWidth;
    const float worldHeight = height * scale.cellHeight;
    const auto worldSeed = static_cast<std::uint32_t>(seed);

    // Place a large example near the opening view wherever the terrain permits.
    const auto [spawnX, spawnY] = findSpawnCell();
    if (spawnX >= 0)
    {
        const float distance = std::max(maxDiameter * 1.15f,
            3.0f * std::max(scale.cellWidth, scale.cellHeight));
        for (int i = 0; i < 12; ++i)
        {
            const float angle = i * 0.5235988f;
            if (placeRock((spawnX + 0.5f) * scale.cellWidth + distance * std::cos(angle),
                          (spawnY + 0.5f) * scale.cellHeight + distance * std::sin(angle),
                          hash(worldSeed), maxDiameter))
            {
                break;
            }
        }
    }

    // A jittered coarse placement lattice sized in physical world units.
    const double spacing = std::max(3.3 * maxDiameter,
        2.0 * std::max(scale.cellWidth, scale.cellHeight));
    for (int row = 0; row * spacing < worldHeight; ++row)
    {
        for (int col = 0; col * spacing < worldWidth; ++col)
        {
            const auto objectSeed = hash(worldSeed ^ (static_cast<std::uint32_t>(col) * 374761393u)
                ^ (static_cast<std::uint32_t>(row) * 668265263u));
            const float x = static_cast<float>((col + 0.25 + 0.5 * fraction(hash(objectSeed + 3u))) * spacing);
            const float y = static_cast<float>((row + 0.25 + 0.5 * fraction(hash(objectSeed + 4u))) * spacing);
            if (x >= worldWidth || y >= worldHeight)
            {
                continue;
            }
            const int cx = std::min(width - 1, static_cast<int>(x / scale.cellWidth));
            const int cy = std::min(height - 1, static_cast<int>(y / scale.cellHeight));
            if (spawnX >= 0
                && std::abs(cx - static_cast<double>(spawnX)) * scale.cellWidth < maxDiameter + 2.0 * scale.cellWidth
                && std::abs(cy - static_cast<double>(spawnY)) * scale.cellHeight < maxDiameter + 2.0 * scale.cellHeight)
            {
                continue;
            }
            if (getTile(cx, cy).type == TileType::Rock || fraction(objectSeed) < 0.45f)
            {
                const float diameter = scale.playerHeight * std::lerp(
                    scale.minimumRockDiameter, scale.maximumRockDiameter, fraction(hash(objectSeed + 5u)));
                placeRock(x, y, objectSeed, diameter);
            }
        }
    }
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const auto stone = hash(worldSeed ^ (static_cast<std::uint32_t>(x) * 1376312589u)
                ^ (static_cast<std::uint32_t>(y) * 120196573u));
            if (stone % 103u == 0u)
            {
                placeRock((x + 0.25f + 0.5f * fraction(hash(stone + 1u))) * scale.cellWidth,
                          (y + 0.25f + 0.5f * fraction(hash(stone + 2u))) * scale.cellHeight,
                          stone, 3.0f + 7.0f * fraction(hash(stone + 3u)), false);
            }
        }
    }
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

    return getTile(x, y).isWalkable()
        && !hasRock(x, y)
        && !treeTrunks[static_cast<std::size_t>(y) * width + x];
}

bool World::hasRock(int x, int y) const
{
    return isInside(x, y)
        && rockFootprint[static_cast<std::size_t>(y) * width + x];
}

float World::getLightOpacity(int x, int y) const
{
    const auto& tile = getTile(x, y);
    if (hasRock(x, y))
    {
        return 1.0f;
    }
    return treeTrunks[static_cast<std::size_t>(y) * width + x] ? 0.3f : tile.getLightOpacity();
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
    const auto spawn = findSpawnCell();
    if (spawn.first < 0)
    {
        throw std::runtime_error("No suitable spawn position found");
    }
    return spawn;
}

std::pair<int, int> World::findSpawnCell() const
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
bool World::isWalkableAt(
    float worldX,
    float worldY,
    float radius
) const
{
    // Validate coordinates.

    if (!std::isfinite(worldX) ||
        !std::isfinite(worldY) ||
        !std::isfinite(radius) ||
        radius < 0.0f)
    {
        return false;
    }

    // World dimensions.

    const float worldWidth =
        width * scale.cellWidth;

    const float worldHeight =
        height * scale.cellHeight;

    // World boundaries.

    if (worldX - radius < 0.0f ||
        worldY - radius < 0.0f ||
        worldX + radius >= worldWidth ||
        worldY + radius >= worldHeight)
    {
        return false;
    }

    // Find all potentially intersecting cells.

    const int startX =
        static_cast<int>(
            std::floor(
                (worldX - radius)
                / scale.cellWidth
            )
        );

    const int endX =
        static_cast<int>(
            std::floor(
                (worldX + radius)
                / scale.cellWidth
            )
        );

    const int startY =
        static_cast<int>(
            std::floor(
                (worldY - radius)
                / scale.cellHeight
            )
        );

    const int endY =
        static_cast<int>(
            std::floor(
                (worldY + radius)
                / scale.cellHeight
            )
        );

    const float radiusSquared =
        radius * radius;

    for (int cy = startY; cy <= endY; ++cy)
    {
        for (int cx = startX; cx <= endX; ++cx)
        {
            if (isWalkable(cx, cy))
            {
                continue;
            }

            const float left =
                cx * scale.cellWidth;

            const float right =
                left + scale.cellWidth;

            const float top =
                cy * scale.cellHeight;

            const float bottom =
                top + scale.cellHeight;

            // Find the closest point
            // on the blocked cell.

            const float closestX =
                std::clamp(
                    worldX,
                    left,
                    right
                );

            const float closestY =
                std::clamp(
                    worldY,
                    top,
                    bottom
                );

            const float dx =
                worldX - closestX;

            const float dy =
                worldY - closestY;

            // Circle versus rectangle collision.

            if (dx * dx + dy * dy
                <= radiusSquared)
            {
                return false;
            }
        }
    }

    return true;
}

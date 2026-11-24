#include "world/WorldGenerator.hpp"

WorldGenerator::WorldGenerator(int seed)
{
    // Elevation

    elevationNoise.SetSeed(seed);

    elevationNoise.SetNoiseType(
        FastNoiseLite::NoiseType_OpenSimplex2
    );

    elevationNoise.SetFrequency(0.015f);

    elevationNoise.SetFractalType(
        FastNoiseLite::FractalType_FBm
    );

    elevationNoise.SetFractalOctaves(5);

    elevationNoise.SetFractalLacunarity(2.0f);
    elevationNoise.SetFractalGain(0.5f);

    // Moisture

    moistureNoise.SetSeed(seed + 1);

    moistureNoise.SetNoiseType(
        FastNoiseLite::NoiseType_OpenSimplex2
    );

    moistureNoise.SetFrequency(0.025f);

    moistureNoise.SetFractalType(
        FastNoiseLite::FractalType_FBm
    );

    moistureNoise.SetFractalOctaves(3);

    // Vegetation

    vegetationNoise.SetSeed(seed + 2);

    vegetationNoise.SetNoiseType(
        FastNoiseLite::NoiseType_OpenSimplex2
    );

    vegetationNoise.SetFrequency(0.15f);
}

Tile WorldGenerator::generateTile(int x, int y) const
{
    const float fx = static_cast<float>(x);
    const float fy = static_cast<float>(y);

    const float elevation =
        (elevationNoise.GetNoise(fx, fy) + 1.0f)
        * 0.5f;

    const float moisture =
        (moistureNoise.GetNoise(fx, fy) + 1.0f)
        * 0.5f;

    const float vegetation =
        (vegetationNoise.GetNoise(fx, fy) + 1.0f)
        * 0.5f;

    // Deep water

    if (elevation < 0.30f)
    {
        return Tile{TileType::DeepWater};
    }

    // Shallow water

    if (elevation < 0.39f)
    {
        return Tile{TileType::Water};
    }

    // Beach

    if (elevation < 0.43f)
    {
        return Tile{TileType::Sand};
    }

    // Mountains

    if (elevation > 0.76f)
    {
        return Tile{TileType::Mountain};
    }

    // Rocky areas

    if (elevation > 0.69f)
    {
        return Tile{TileType::Rock};
    }

    // Forest

    if (moisture > 0.52f &&
        vegetation > 0.55f)
    {
        return Tile{TileType::Tree};
    }

    // Grass

    if (moisture > 0.40f)
    {
        return Tile{TileType::Grass};
    }

    return Tile{TileType::Ground};
}
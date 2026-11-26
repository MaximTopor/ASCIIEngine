#include "renderer/VegetationGenerator.hpp"

#include <cmath>
#include <algorithm>
#include <array>

std::uint32_t VegetationGenerator::hash(
    std::uint32_t value
)
{
    value ^= value >> 16;
    value *= 0x7feb352dU;

    value ^= value >> 15;
    value *= 0x846ca68bU;

    value ^= value >> 16;

    return value;
}

std::uint32_t VegetationGenerator::tileSeed(
    int x,
    int y
)
{
    return hash(
        static_cast<std::uint32_t>(x)
            * 374761393U
        ^
        static_cast<std::uint32_t>(y)
            * 668265263U
        ^
        42U
    );
}

float VegetationGenerator::random(
    std::uint32_t& state
)
{
    state = hash(state + 0x9e3779b9U);

    return static_cast<float>(
        state >> 8
    ) / 16777216.0f;
}

float VegetationGenerator::densityNoise(
    int x,
    int y
)
{
    const float a =
        std::sin(x * 0.19f + y * 0.07f);

    const float b =
        std::cos(y * 0.23f - x * 0.11f);

    const float c =
        std::sin(x * 0.51f + y * 0.37f);

    return std::clamp(
        0.5f
        + a * 0.23f
        + b * 0.18f
        + c * 0.09f,
        0.0f,
        1.0f
    );
}

void VegetationGenerator::generate(
    const Tile& tile,
    int tileX,
    int tileY,
    float cellWidth,
    float cellHeight,
    std::vector<VisualGlyph>& output
)
{
    output.clear();

    // Vegetation only grows on suitable terrain.

    if (tile.type != TileType::Grass &&
        tile.type != TileType::Ground &&
        tile.type != TileType::Tree)
    {
        return;
    }

    std::uint32_t state =
        tileSeed(tileX, tileY);

    const float density =
        densityNoise(tileX, tileY);

    const float originX =
        tileX * cellWidth;

    const float originY =
        tileY * cellHeight;

    // Determine vegetation density.

    int grassCount = 0;

    switch (tile.type)
    {
        case TileType::Grass:

            grassCount = static_cast<int>(
                12.0f + density * density * 29.0f
            );

            break;

        case TileType::Tree:

            grassCount = static_cast<int>(
                9.0f + density * density * 24.0f
            );

            break;

        case TileType::Ground:

            grassCount = static_cast<int>(
                density * density * 12.0f
            );

            break;

        default:
            return;
    }

    output.reserve(grassCount + 8);

    // Grass symbols.

    static constexpr std::array<char32_t, 7>
        grassCharacters =
    {
        U'|',
        U'/',
        U'\\',
        U'\'',
        U';',
        U',',
        U'!'
    };

    // Grass colors.

    static constexpr std::array<Color, 6>
        grassColors =
    {{
        {0.15f, 0.26f, 0.17f},
        {0.21f, 0.34f, 0.21f},
        {0.28f, 0.40f, 0.23f},
        {0.36f, 0.48f, 0.29f},
        {0.43f, 0.53f, 0.33f},
        {0.18f, 0.30f, 0.24f}
    }};

    // Generate individual blades of grass.

    for (int i = 0; i < grassCount; ++i)
    {
        VisualGlyph glyph;

        const int characterIndex =
            static_cast<int>(
                random(state)
                * grassCharacters.size()
            );

        glyph.character =
            grassCharacters[characterIndex];

        glyph.x =
            originX
            + random(state) * cellWidth;

        glyph.y =
            originY
            + random(state) * cellHeight;

        // Different blade heights.

        glyph.scale =
            0.055f
            + random(state) * 0.095f;
        glyph.z = glyph.scale * 20.0f;

        // Select grass color.

        const int colorIndex =
            static_cast<int>(
                random(state)
                * grassColors.size()
            );

        glyph.color =
            grassColors[colorIndex];

        // Additional brightness variation.

        const float brightness =
            0.75f + random(state) * 0.5f;

        glyph.color.r *= brightness;
        glyph.color.g *= brightness;
        glyph.color.b *= brightness;

        // Independent base rotation.

        glyph.transform.rotation =
            (random(state) - 0.5f) * 0.95f;

        // Independent opacity.

        glyph.transform.opacity =
            0.55f + random(state) * 0.45f;

        // Rotate around the bottom.

        glyph.transform.pivotX = 0.5f;
        glyph.transform.pivotY = 1.0f;

        // Spatially coherent wind.

        glyph.animation.type =
            AnimationType::Wind;

        glyph.animation.phase =
            glyph.x * 0.025f
            + glyph.y * 0.018f
            + random(state) * 0.5f;

        glyph.animation.amplitude =
            0.04f + random(state) * 0.15f;

        output.push_back(glyph);
    }

    // Small wildflowers.

    if (tile.type == TileType::Grass &&
        density > 0.55f &&
        random(state) < 0.23f)
    {
        const int flowerCount =
            1 + static_cast<int>(
                random(state) * 3.0f
            );

        for (int i = 0; i < flowerCount; ++i)
        {
            const float flowerX =
                originX
                + random(state) * cellWidth;

            const float flowerY =
                originY
                + random(state) * cellHeight;

            // Flower stem.

            VisualGlyph stem;

            stem.character = U'|';

            stem.x = flowerX;
            stem.y = flowerY;
            stem.z = 2.5f;

            stem.scale = 0.13f;

            stem.color = {
                0.32f,
                0.49f,
                0.25f
            };

            stem.animation.type =
                AnimationType::Wind;

            stem.animation.phase =
                flowerX * 0.025f
                + flowerY * 0.018f;

            stem.animation.amplitude = 0.08f;

            output.push_back(stem);

            // Flower head.

            VisualGlyph flower;

            flower.character =
                random(state) < 0.5f
                    ? U'*'
                    : U'o';

            flower.x = flowerX;
            flower.y = flowerY;
            flower.z = 5.0f;

            flower.scale =
                0.075f + random(state) * 0.035f;

            if (random(state) < 0.75f)
            {
                flower.color = {
                    0.84f,
                    0.85f,
                    0.73f
                };
            }
            else
            {
                flower.color = {
                    0.91f,
                    0.76f,
                    0.29f
                };
            }

            flower.transform.opacity = 0.9f;

            flower.animation = stem.animation;

            output.push_back(flower);
        }
    }
}

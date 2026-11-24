#include "renderer/WorldRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace
{
    float randomValue(int x, int y, int salt)
    {
        std::uint32_t n =
            static_cast<std::uint32_t>(x) * 374761393u
            + static_cast<std::uint32_t>(y) * 668265263u
            + static_cast<std::uint32_t>(salt) * 2246822519u;

        n = (n ^ (n >> 13u)) * 1274126177u;

        n ^= n >> 16u;

        return static_cast<float>(n & 0xFFFFu)
            / 65535.0f;
    }
}

WorldRenderer::WorldRenderer(
    GlyphRenderer& glyphRenderer,
    float cellWidth,
    float cellHeight,
    float glyphScale
)
    : glyphRenderer(glyphRenderer),
      cellWidth(cellWidth),
      cellHeight(cellHeight),
      glyphScale(glyphScale)
{
}

void WorldRenderer::drawGlyph(
    const Camera& camera,
    char32_t symbol,
    float worldX,
    float worldY,
    float scale,
    Color color,
    GlyphAnimation animation
)
{
    glyphRenderer.draw(
        symbol,
        camera.worldToScreenX(worldX),
        camera.worldToScreenY(worldY),
        scale * camera.getZoom(),
        color,
        animation
    );
}

void WorldRenderer::renderGround(
    const Camera& camera,
    const Tile& tile,
    int x,
    int y
)
{
    const float worldX = x * cellWidth;
    const float worldY = y * cellHeight;

    const float variation =
        randomValue(x, y, 1);

    char32_t symbol = U'.';

    Color color = {
        0.25f + variation * 0.12f,
        0.35f + variation * 0.10f,
        0.18f
    };

    GlyphAnimation animation;

    switch (tile.type)
    {
        case TileType::DeepWater:
            symbol = U'~';
            color = {0.12f, 0.25f, 0.60f};

            animation = {
                AnimationType::Water,
                x * 0.35f + y * 0.20f,
                0.12f
            };
            break;

        case TileType::Water:
            symbol = U'~';
            color = {0.25f, 0.55f, 0.85f};

            animation = {
                AnimationType::Water,
                x * 0.35f + y * 0.20f,
                0.15f
            };
            break;

        case TileType::Sand:
            symbol = U':';
            color = {
                0.60f + variation * 0.20f,
                0.52f + variation * 0.15f,
                0.32f
            };
            break;

        case TileType::Rock:
        case TileType::Mountain:
            color = {0.30f, 0.32f, 0.30f};
            break;

        default:
            break;
    }

    drawGlyph(
        camera,
        symbol,
        worldX + 4.0f,
        worldY + cellHeight - 2.0f,
        glyphScale,
        color,
        animation
    );
}

void WorldRenderer::renderVegetation(
    const Camera& camera,
    const Tile& tile,
    int x,
    int y
)
{
    if (tile.type != TileType::Grass &&
        tile.type != TileType::Tree)
    {
        return;
    }

    const float worldX = x * cellWidth;
    const float worldY = y * cellHeight;

    constexpr char32_t symbols[] = {
        U'|', U'/', U'\\', U';', U','
    };

    constexpr int grassCount = 4;

    for (int i = 0; i < grassCount; ++i)
    {
        const float r1 = randomValue(x, y, i * 4 + 10);
        const float r2 = randomValue(x, y, i * 4 + 11);
        const float r3 = randomValue(x, y, i * 4 + 12);
        const float r4 = randomValue(x, y, i * 4 + 13);

        const float grassX =
            worldX + r1 * cellWidth;

        const float grassY =
            worldY + cellHeight * 0.4f
            + r2 * cellHeight * 0.6f;

        const float scale =
            0.12f + r3 * 0.14f;

        const Color color{
            0.18f + r3 * 0.20f,
            0.35f + r4 * 0.30f,
            0.10f + r1 * 0.12f
        };

        const char32_t symbol =
            symbols[
                std::min(
                    4,
                    static_cast<int>(r4 * 5)
                )
            ];

        const float phase =
            grassX * 0.025f
            + grassY * 0.015f
            + r1 * 0.5f;

        drawGlyph(
            camera,
            symbol,
            grassX,
            grassY,
            scale,
            color,
            {
                AnimationType::Wind,
                phase,
                0.25f + r2 * 0.20f
            }
        );
    }
}

void WorldRenderer::renderObjects(
    const Camera& camera,
    const Tile& tile,
    int x,
    int y
)
{
    const float worldX = x * cellWidth;
    const float worldY = y * cellHeight;

    const float centerX =
        worldX + cellWidth * 0.5f;

    const float bottomY =
        worldY + cellHeight;

    switch (tile.type)
    {
        case TileType::Tree:
        {
            // Trunk

            drawGlyph(
                camera,
                U'|',
                centerX - 3.0f,
                bottomY,
                0.35f,
                {0.42f, 0.29f, 0.15f}
            );

            // Crown

            for (int row = 0; row < 3; ++row)
            {
                for (int column = 0;
                     column < 3;
                     ++column)
                {
                    const float r =
                        randomValue(
                            x,
                            y,
                            row * 3 + column + 50
                        );

                    drawGlyph(
                        camera,
                        U'*',
                        centerX - 11.0f
                            + column * 7.0f,
                        bottomY - 10.0f
                            - row * 6.0f,
                        0.25f + r * 0.1f,
                        {
                            0.12f + r * 0.10f,
                            0.35f + r * 0.25f,
                            0.12f
                        },
                        {
                            AnimationType::Wind,
                            worldX * 0.02f
                                + worldY * 0.01f
                                + r,
                            0.10f
                        }
                    );
                }
            }

            break;
        }

        case TileType::Rock:
            drawGlyph(
                camera,
                U'o',
                worldX + 4.0f,
                bottomY - 2.0f,
                0.45f,
                {0.55f, 0.57f, 0.53f}
            );
            break;

        case TileType::Mountain:
            drawGlyph(
                camera,
                U'^',
                worldX,
                bottomY - 2.0f,
                0.65f,
                {0.75f, 0.77f, 0.73f}
            );
            break;

        case TileType::Wall:
            drawGlyph(
                camera,
                U'#',
                worldX + 4.0f,
                bottomY - 2.0f,
                glyphScale,
                {0.50f, 0.50f, 0.50f}
            );
            break;

        default:
            break;
    }
}

void WorldRenderer::render(
    const World& world,
    const Camera& camera
)
{
    const float halfWidth =
        camera.getVisibleWidth() * 0.5f;

    const float halfHeight =
        camera.getVisibleHeight() * 0.5f;

    const int startX = std::clamp(
        static_cast<int>(
            std::floor(
                (camera.getX() - halfWidth)
                / cellWidth
            )
        ) - 2,
        0,
        world.getWidth()
    );

    const int endX = std::clamp(
        static_cast<int>(
            std::ceil(
                (camera.getX() + halfWidth)
                / cellWidth
            )
        ) + 2,
        0,
        world.getWidth()
    );

    const int startY = std::clamp(
        static_cast<int>(
            std::floor(
                (camera.getY() - halfHeight)
                / cellHeight
            )
        ) - 2,
        0,
        world.getHeight()
    );

    const int endY = std::clamp(
        static_cast<int>(
            std::ceil(
                (camera.getY() + halfHeight)
                / cellHeight
            )
        ) + 2,
        0,
        world.getHeight()
    );

    // Layer 0: Ground

    for (int y = startY; y < endY; ++y)
    {
        for (int x = startX; x < endX; ++x)
        {
            renderGround(
                camera,
                world.getTile(x, y),
                x, y
            );
        }
    }

    // Layer 1: Vegetation

    for (int y = startY; y < endY; ++y)
    {
        for (int x = startX; x < endX; ++x)
        {
            renderVegetation(
                camera,
                world.getTile(x, y),
                x, y
            );
        }
    }

    // Layer 2: Objects

    for (int y = startY; y < endY; ++y)
    {
        for (int x = startX; x < endX; ++x)
        {
            renderObjects(
                camera,
                world.getTile(x, y),
                x, y
            );
        }
    }
}
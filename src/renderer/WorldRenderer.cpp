#include "renderer/CpuProfiler.hpp"
#include "renderer/WorldRenderer.hpp"
#include "renderer/SurfaceSampler.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string_view>

namespace
{
    float randomValue(int x, int y, int salt)
    {
        auto n = static_cast<std::uint32_t>(x) * 374761393u
            + static_cast<std::uint32_t>(y) * 668265263u
            + static_cast<std::uint32_t>(salt) * 2246822519u;

        n = (n ^ (n >> 13u)) * 1274126177u;

        return static_cast<float>(
            (n ^ (n >> 16u)) & 0xFFFFu
        ) / 65536.0f;
    }

    struct GroundMaterial
    {
        Color color;
        float water;
    };

    GroundMaterial material(TileType type)
    {
        switch (type)
        {
            case TileType::DeepWater:
                return {{0.12f, 0.24f, 0.39f}, 1.0f};

            case TileType::Water:
                return {{0.20f, 0.39f, 0.51f}, 1.0f};

            case TileType::Sand:
                return {{0.47f, 0.39f, 0.25f}, 0.0f};

            case TileType::Tree:
            case TileType::Grass:
                return {{0.28f, 0.32f, 0.18f}, 0.0f};

            case TileType::Rock:
            case TileType::Mountain:
                return {{0.33f, 0.34f, 0.31f}, 0.0f};

            default:
                return {{0.34f, 0.28f, 0.19f}, 0.0f};
        }
    }

    GroundMaterial blend(
        GroundMaterial a,
        GroundMaterial b,
        float t
    )
    {
        return {
            {
                std::lerp(a.color.r, b.color.r, t),
                std::lerp(a.color.g, b.color.g, t),
                std::lerp(a.color.b, b.color.b, t)
            },
            std::lerp(a.water, b.water, t)
        };
    }

    // ========================================
    // PROCEDURAL TREE GENERATION
    // ========================================

    void contactShadow(
        const Camera& camera,
        GlyphRenderer& renderer,
        float x,
        float y,
        float radiusX,
        float radiusY,
        float angle
    )
    {
        const float c = std::cos(angle);
        const float s = std::sin(angle);

        for (int i = 0; i < 14; ++i)
        {
            const float t =
                i * 2.399963f;

            const float r =
                0.50f
                + 0.45f * std::sqrt(i / 14.0f);

            const float dx =
                std::cos(t) * radiusX * r;

            const float dy =
                std::sin(t) * radiusY * r;

            GlyphTransform transform;

            transform.opacity = 0.48f;

            transform.rotation = angle + t;

            transform.pivotY = 0.5f;

            renderer.drawWorld(
                camera,
                U'=',

                x + c * dx - s * dy,
                y + s * dx + c * dy,

                0.0f,

                radiusX / 115.0f,

                {
                    0.035f,
                    0.045f,
                    0.033f
                },

                {},
                transform,

                GlyphAnchor::Center,

                true
            );
        }
    }
}

// ============================================
// CONSTRUCTOR
// ============================================

WorldRenderer::WorldRenderer(
    GlyphRenderer& glyphRenderer,
    const World& world,
    float glyphScale,
    bool benchmark
)
    : glyphRenderer(glyphRenderer),
      world(world),
      cellWidth(world.getScale().cellWidth),
      cellHeight(world.getScale().cellHeight),
      glyphScale(glyphScale),
      benchmark(benchmark),
      treeRenderer(world.getTrees().size())
{
}

// ============================================
// GROUND GEOMETRY GENERATION
// ============================================

void WorldRenderer::renderGround(
    const Camera& camera,
    int x,
    int y
)
{
    const auto type = world.getTile(x, y).type;

    if (
        type != TileType::Water
        && type != TileType::DeepWater
    )
    {
        return;
    }

    const bool deep =
        type == TileType::DeepWater;

    const bool lowDetail =
        camera.getZoom() < 0.65f;

    constexpr float BASE_SPACING = 1.65f;
    constexpr float DETAIL_SPACING = 3.20f;

    constexpr float BASE_COVERAGE = 2.30f;
    constexpr float DETAIL_COVERAGE = 2.10f;

    const Color shallowColor{
        0.16f,
        0.37f,
        0.39f
    };

    const Color deepColor{
        0.075f,
        0.20f,
        0.29f
    };

    const Color baseColor =
        deep ? deepColor : shallowColor;

    // World-space current direction.
    // Positive Y follows the benchmark river.

    float flowAngle =
        std::numbers::pi_v<float> * 0.5f;

    if (benchmark)
    {
        const float centerY =
            (world.getHeight() / 2 + 0.5f)
            * cellHeight;

        const float py =
            (y + 0.5f) * cellHeight - centerY;

        const float slope =
            34.0f * 0.011f
            * std::cos(py * 0.011f);

        flowAngle =
            std::atan2(1.0f, slope);
    }

    const auto generateLayer =
        [&](bool detailLayer)
    {
        const float spacing =
            (detailLayer
                ? DETAIL_SPACING
                : BASE_SPACING)
            * (lowDetail ? 1.65f : 1.0f);

        const float coverage =
            detailLayer
                ? DETAIL_COVERAGE
                : BASE_COVERAGE;

        const int columns =
            std::max(
                1,
                static_cast<int>(
                    std::ceil(cellWidth / spacing)
                )
            );

        const int rows =
            std::max(
                1,
                static_cast<int>(
                    std::ceil(cellHeight / spacing)
                )
            );

        const float stepX =
            cellWidth / columns;

        const float stepY =
            cellHeight / rows;

        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < columns; ++col)
            {
                const int salt =
                    (row * columns + col) * 7
                    + (detailLayer ? 100000 : 0);

                const float grain =
                    randomValue(x, y, salt + 1);

                const float jitterX =
                    (randomValue(x, y, salt + 2)
                    - 0.5f) * 0.35f;

                const float jitterY =
                    (randomValue(x, y, salt + 3)
                    - 0.5f) * 0.35f;

                if (detailLayer && grain < 0.18f)
                {
                    continue;
                }

                const float px =
                    x * cellWidth
                    + (col + 0.5f + jitterX)
                    * stepX;

                const float py =
                    y * cellHeight
                    + (row + 0.5f + jitterY)
                    * stepY;

                const float brightness =
                    detailLayer
                        ? 1.15f + grain * 0.30f
                        : 0.88f + grain * 0.24f;

                const Color color{
                    baseColor.r * brightness,
                    baseColor.g * brightness,
                    baseColor.b * brightness
                };

                char32_t character;

                if (detailLayer)
                {
                    constexpr std::array palette{
                        U'~', U'-', U'=', U'~'
                    };

                    character =
                        palette[(salt / 7)
                                % palette.size()];
                }
                else
                {
                    constexpr std::array palette{
                        U'#', U'%', U'@', U'8'
                    };

                    character =
                        palette[(salt / 7)
                                % palette.size()];
                }

                GlyphTransform transform;

                transform.pivotX = 0.5f;
                transform.pivotY = 0.5f;

                transform.opacity =
                    detailLayer ? 0.68f : 1.0f;

                const GlyphAnimation animation{
                    AnimationType::Water,

                    flowAngle,

                    detailLayer ? 1.0f : 0.0f
                };

                const float scale =
                    std::max(stepX, stepY)
                    * coverage / 36.0f;

                glyphRenderer.drawWorld(
                    camera,

                    character,

                    px,
                    py,
                    0.0f,

                    scale,

                    color,

                    animation,
                    transform,

                    GlyphAnchor::Center,

                    true
                );
            }
        }
    };

    generateLayer(false);
    generateLayer(true);
}

// ============================================
// GPU GROUND CHUNK CACHE
// ============================================

void WorldRenderer::renderGroundChunks(
    const Camera& camera,
    int startX,
    int startY,
    int endX,
    int endY
)
{
    if (
        startX >= endX
        || startY >= endY
    )
    {
        return;
    }

    constexpr int chunkSize =
        GROUND_CHUNK_SIZE;

    const int startChunkX =
        startX / chunkSize;

    const int startChunkY =
        startY / chunkSize;

    const int endChunkX =
        (endX - 1) / chunkSize;

    const int endChunkY =
        (endY - 1) / chunkSize;

    // Separate GPU cache for each LOD.

    const int detailLevel =
        camera.getZoom() < 0.65f
            ? 0
            : 1;

    for (
        int chunkY = startChunkY;
        chunkY <= endChunkY;
        ++chunkY
    )
    {
        for (
            int chunkX = startChunkX;
            chunkX <= endChunkX;
            ++chunkX
        )
        {
            const int firstX =
                chunkX * chunkSize;

            const int firstY =
                chunkY * chunkSize;

            const int lastX =
                std::min(
                    firstX + chunkSize,
                    world.getWidth()
                );

            const int lastY =
                std::min(
                    firstY + chunkSize,
                    world.getHeight()
                );

            // --------------------------------
            // Chunk-level visibility check
            // --------------------------------

            const float centerX =
                (firstX + lastX)
                * 0.5f
                * cellWidth;

            const float centerY =
                (firstY + lastY)
                * 0.5f
                * cellHeight;

            const float padding =
                (
                    chunkSize
                    * (cellWidth + cellHeight)
                    * 0.5f
                    + 32.0f
                )
                * camera.getZoom();

            if (!camera.isVisible(
                    camera.project(
                        centerX,
                        centerY
                    ),
                    padding
                ))
            {
                continue;
            }

            // --------------------------------
            // Cache hit
            // --------------------------------

            if (glyphRenderer.useGroundChunk(
                    chunkX,
                    chunkY,
                    detailLevel
                ))
            {
                continue;
            }

            // --------------------------------
            // Cache miss
            // Generate the complete chunk.
            // --------------------------------

            for (
                int y = firstY;
                y < lastY;
                ++y
            )
            {
                for (
                    int x = firstX;
                    x < lastX;
                    ++x
                )
                {
                    renderGround(
                        camera,
                        x,
                        y
                    );
                }
            }

            // Upload generated geometry
            // to permanent GPU storage.

            glyphRenderer.finishGroundChunk();
        }
    }
}

// ============================================
// MAIN WORLD RENDERING
// ============================================

void WorldRenderer::render(
    const Camera& camera
)
{
    visibleTileCount = 0;

    // ========================================
    // VISIBLE WORLD BOUNDS
    // ========================================

    const auto ground =
        camera.visibleGround();

    const auto elevated =
        camera.visibleGround(
            world.getScale().playerHeight
            * 7.0f
        );

    const int startX =
        std::max(
            0,
            static_cast<int>(
                std::floor(
                    ground.left / cellWidth
                )
            ) - 2
        );

    const int endX =
        std::min(
            world.getWidth(),
            static_cast<int>(
                std::ceil(
                    ground.right / cellWidth
                )
            ) + 2
        );

    const int startY =
        std::max(
            0,
            static_cast<int>(
                std::floor(
                    ground.top / cellHeight
                )
            ) - 2
        );

    const int endY =
        std::min(
            world.getHeight(),
            static_cast<int>(
                std::ceil(
                    ground.bottom / cellHeight
                )
            ) + 2
        );

    // ========================================
    // GROUND AND TERRAIN
    // ========================================

    {
        CpuProfiler::Scope timer(
            CpuProfiler::Stage::Ground
        );

        renderGroundChunks(
            camera,
            startX,
            startY,
            endX,
            endY
        );

        const float padding =
            (
                cellWidth
                + cellHeight
                + 20.0f
            )
            * camera.getZoom();

        for (
            int y = startY;
            y < endY;
            ++y
        )
        {
            for (
                int x = startX;
                x < endX;
                ++x
            )
            {
                const float centerX =
                    (x + 0.5f) * cellWidth;

                const float centerY =
                    (y + 0.5f) * cellHeight;

                if (!camera.isVisible(
                        camera.project(
                            centerX,
                            centerY
                        ),
                        padding
                    ))
                {
                    continue;
                }

                ++visibleTileCount;

            }
        }
    }

    // ========================================
    // VEGETATION GPU CACHE
    // ========================================

    {
        CpuProfiler::Scope timer(
            CpuProfiler::Stage::Vegetation
        );

        vegetationCache.render(
            world,
            camera,
            glyphRenderer,
            cellWidth,
            cellHeight
        );
    }

    // ========================================
    // OBJECT VISIBILITY
    // ========================================

    const auto visible =
        [&elevated](
            float x,
            float y,
            float padding
        )
    {
        return
            x >= elevated.left - padding
            && x <= elevated.right + padding
            && y >= elevated.top - padding
            && y <= elevated.bottom + padding;
    };

    // ========================================
    // ROCKS — GPU CACHE
    // ========================================

    {
        CpuProfiler::Scope timer(
            CpuProfiler::Stage::Rocks
        );

        for (
            const Rock& rock
            : world.getRocks()
        )
        {
            if (!visible(
                    rock.x,
                    rock.y,
                    rock.radiusX * 1.5f
                ))
            {
                continue;
            }

            // --------------------------------
            // Dynamic contact shadow
            // --------------------------------

            if (rock.radiusX >= 5.0f)
            {
                contactShadow(
                    camera,
                    glyphRenderer,

                    rock.x,
                    rock.y,

                    rock.radiusX * 1.1f,
                    rock.radiusY * 1.1f,

                    rock.angle
                );
            }

            // --------------------------------
            // Cached rock geometry
            // --------------------------------

            rockRenderer.render(
                rock,
                camera,
                glyphRenderer
            );
        }
    }

    // ========================================
    // TREES — GPU CACHE
    // ========================================

    {
        CpuProfiler::Scope timer(
            CpuProfiler::Stage::Trees
        );

        const auto& trees =
            world.getTrees();

        for (
            std::size_t i = 0;
            i < trees.size();
            ++i
        )
        {
            const auto& tree = trees[i];

            // --------------------------------
            // Tree visibility
            // --------------------------------

            if (!camera.isVisible(
                    camera.project(
                        tree.x,
                        tree.y,
                        tree.height * 0.52f
                    ),

                    tree.height
                        * 0.78f
                        * camera.getZoom()
                ))
            {
                continue;
            }

            // --------------------------------
            // Dynamic contact shadow
            // --------------------------------

            contactShadow(
                camera,
                glyphRenderer,

                tree.x,
                tree.y,

                tree.height * 0.17f,
                tree.height * 0.12f,

                0.0f
            );

            treeRenderer.render(i, tree, camera, glyphRenderer);
        }
    }
}

void WorldRenderer::renderMaterialSamples(const Camera& camera)
{
    const float x = (world.getWidth()/2+0.5f)*cellWidth;
    const float y = (world.getHeight()/2+0.5f)*cellHeight;
    const auto center = camera.project(x-62.0f,y+70.0f,13.0f);
    if (!camera.isVisible(center,70.0f*camera.getZoom()) || glyphRenderer.useGroundChunk(0,0,-300))
    {
        return;
    }
    for (const auto type : {MaterialPresets::Type::Steel,MaterialPresets::Type::Stone})
    {
        const float offset = type == MaterialPresets::Type::Steel ? -20.0f : 20.0f;
        SurfaceSampler::ellipsoid({x-62.0f+offset,y+70.0f,10.0f},{15.0f,14.0f,11.0f},
            {2.7f,0.17f,421u}, [&](const SurfaceSampler::Sample& sample)
            {
                if (sample.position.z < 0.0f) return;
                constexpr std::u32string_view ink = U"#%8H";
                const auto symbol = ink[static_cast<std::size_t>(sample.variation*ink.size())];
                const auto& p = sample.position;
                const auto& n = sample.normal;
                glyphRenderer.drawWorld(camera,symbol,p.x,p.y,p.z,0.112f,{0.44f,0.45f,0.46f},
                    {},{0,1,0.5f,0.5f},GlyphAnchor::Center,false,true,
                    MaterialPresets::surface(type,n.x,n.y,n.z));
            });
    }
    glyphRenderer.finishGroundChunk();
}

std::size_t WorldRenderer::getVisibleTileCount() const
{
    return visibleTileCount;
}

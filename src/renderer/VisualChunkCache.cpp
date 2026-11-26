#include "renderer/VisualChunkCache.hpp"
#include "renderer/VegetationGenerator.hpp"

#include <algorithm>
#include <cmath>

// ============================================
// CPU CHUNK GENERATION
// ============================================

VisualChunkCache::Chunk&
VisualChunkCache::getChunk(
    const World& world,
    int chunkX,
    int chunkY,
    float cellWidth,
    float cellHeight
)
{
    const auto key =
        std::make_pair(
            chunkX,
            chunkY
        );

    auto [iterator, inserted] =
        chunks.try_emplace(key);

    Chunk& chunk =
        iterator->second;

    chunk.lastUsed =
        frameIndex;

    // Already generated.

    if (!inserted)
    {
        return chunk;
    }

    // Generate every tile in the chunk.

    for (
        int localY = 0;
        localY < CHUNK_SIZE;
        ++localY
    )
    {
        for (
            int localX = 0;
            localX < CHUNK_SIZE;
            ++localX
        )
        {
            const int worldX =
                chunkX * CHUNK_SIZE
                + localX;

            const int worldY =
                chunkY * CHUNK_SIZE
                + localY;

            if (!world.isInside(worldX, worldY))
            {
                continue;
            }

            const int index =
                localY * CHUNK_SIZE
                + localX;

            VegetationGenerator::generate(
                world.getTile(
                    worldX,
                    worldY
                ),

                worldX,
                worldY,

                cellWidth,
                cellHeight,

                chunk.tiles[index]
            );
        }
    }

    return chunk;
}

// ============================================
// CPU CACHE EVICTION
// ============================================

void VisualChunkCache::removeOldChunks()
{
    while (chunks.size() > MAX_CHUNKS)
    {
        auto oldest =
            chunks.end();

        for (
            auto it = chunks.begin();
            it != chunks.end();
            ++it
        )
        {
            if (
                it->second.lastUsed
                == frameIndex
            )
            {
                continue;
            }

            if (
                oldest == chunks.end()
                ||
                it->second.lastUsed
                    < oldest->second.lastUsed
            )
            {
                oldest = it;
            }
        }

        if (oldest == chunks.end())
        {
            break;
        }

        chunks.erase(oldest);
    }
}

// ============================================
// GPU VEGETATION CHUNK RENDERING
// ============================================

void VisualChunkCache::render(
    const World& world,
    const Camera& camera,
    GlyphRenderer& renderer,
    float cellWidth,
    float cellHeight
)
{
    ++frameIndex;

    const float zoom =
        camera.getZoom();

    const float playerScale =
        world.getScale().playerHeight
        / 64.0f;

    // ========================================
    // VISIBLE WORLD BOUNDS
    // ========================================

    const auto bounds =
        camera.visibleGround(
            22.0f * playerScale
        );

    const int startX =
        std::max(
            0,

            static_cast<int>(
                std::floor(
                    bounds.left
                    / cellWidth
                )
            ) - 2
        );

    const int endX =
        std::min(
            world.getWidth() - 1,

            static_cast<int>(
                std::ceil(
                    bounds.right
                    / cellWidth
                )
            ) + 2
        );

    const int startY =
        std::max(
            0,

            static_cast<int>(
                std::floor(
                    bounds.top
                    / cellHeight
                )
            ) - 2
        );

    const int endY =
        std::min(
            world.getHeight() - 1,

            static_cast<int>(
                std::ceil(
                    bounds.bottom
                    / cellHeight
                )
            ) + 2
        );

    if (
        startX > endX
        || startY > endY
    )
    {
        return;
    }

    // ========================================
    // VISIBLE CHUNK BOUNDS
    // ========================================

    const int startChunkX =
        startX / CHUNK_SIZE;

    const int endChunkX =
        endX / CHUNK_SIZE;

    const int startChunkY =
        startY / CHUNK_SIZE;

    const int endChunkY =
        endY / CHUNK_SIZE;

    // ========================================
    // LEVEL OF DETAIL
    // ========================================

    std::size_t detailStep = 1;

    if (zoom < 0.40f)
    {
        detailStep = 8;
    }
    else if (zoom < 0.65f)
    {
        detailStep = 4;
    }
    else if (zoom < 0.90f)
    {
        detailStep = 2;
    }

    // ========================================
    // PROCESS VISIBLE CHUNKS
    // ========================================

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
            // Complete chunk bounds.

            const int firstX =
                chunkX * CHUNK_SIZE;

            const int firstY =
                chunkY * CHUNK_SIZE;

            const int lastX =
                std::min(
                    firstX + CHUNK_SIZE,
                    world.getWidth()
                );

            const int lastY =
                std::min(
                    firstY + CHUNK_SIZE,
                    world.getHeight()
                );

            // =================================
            // CHUNK-LEVEL FRUSTUM CULLING
            // =================================

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
                    CHUNK_SIZE
                    * (cellWidth + cellHeight)
                    * 0.5f

                    + 32.0f * playerScale
                ) * zoom;

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

            // =================================
            // GPU CACHE HIT
            //
            // No individual glyph processing.
            // =================================

            if (
                renderer.useVegetationChunk(
                    chunkX,
                    chunkY,

                    static_cast<int>(
                        detailStep
                    )
                )
            )
            {
                continue;
            }

            // =================================
            // GPU CACHE MISS
            // =================================

            Chunk& chunk =
                getChunk(
                    world,

                    chunkX,
                    chunkY,

                    cellWidth,
                    cellHeight
                );

            // Capture the complete chunk.
            //
            // Do not cull individual glyphs
            // based on the current camera.

            for (
                const auto& glyphs
                : chunk.tiles
            )
            {
                for (
                    std::size_t i = 0;
                    i < glyphs.size();
                    i += detailStep
                )
                {
                    const VisualGlyph& glyph =
                        glyphs[i];

                    auto animation = glyph.animation;
                    if (animation.type == AnimationType::Grass)
                        animation.amplitude = std::min(2.8f, std::sqrt(static_cast<float>(detailStep)));

                    renderer.drawWorld(
                        camera,

                        glyph.character,

                        glyph.x,
                        glyph.y,

                        glyph.z
                            * playerScale,

                        glyph.scale
                            * playerScale,

                        glyph.color,

                        animation,

                        glyph.transform,

                        GlyphAnchor::Center,

                        false,

                        true
                    );
                }
            }

            // Permanent GPU buffer.

            renderer.finishGroundChunk();
        }
    }

    removeOldChunks();
}

// ============================================
// CLEAR CPU CACHE
// ============================================

void VisualChunkCache::clear()
{
    chunks.clear();

    frameIndex = 0;
}
#include "renderer/VisualChunkCache.hpp"
#include "renderer/VegetationGenerator.hpp"

#include <algorithm>
#include <cmath>

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
        std::make_pair(chunkX, chunkY);

    auto [iterator, inserted] =
        chunks.try_emplace(key);

    Chunk& chunk = iterator->second;

    chunk.lastUsed = frameIndex;

    // Already generated.

    if (!inserted)
    {
        return chunk;
    }

    // Generate all tiles inside the chunk.

    for (int localY = 0;
         localY < CHUNK_SIZE;
         ++localY)
    {
        for (int localX = 0;
             localX < CHUNK_SIZE;
             ++localX)
        {
            const int worldX =
                chunkX * CHUNK_SIZE
                + localX;

            const int worldY =
                chunkY * CHUNK_SIZE
                + localY;

            if (!world.isInside(
                    worldX,
                    worldY
                ) || world.hasRock(worldX, worldY))
            {
                continue;
            }

            const int index =
                localY * CHUNK_SIZE
                + localX;

            VegetationGenerator::generate(
                world.getTile(worldX, worldY),
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

void VisualChunkCache::removeOldChunks()
{
    while (chunks.size() > MAX_CHUNKS)
    {
        auto oldest = chunks.end();

        for (auto it = chunks.begin();
             it != chunks.end();
             ++it)
        {
            // Never remove a chunk used
            // during the current frame.

            if (it->second.lastUsed == frameIndex)
            {
                continue;
            }

            if (oldest == chunks.end() ||
                it->second.lastUsed <
                    oldest->second.lastUsed)
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

void VisualChunkCache::render(
    const World& world,
    const Camera& camera,
    GlyphRenderer& renderer,
    float cellWidth,
    float cellHeight
)
{
    ++frameIndex;

    const float zoom = camera.getZoom();
    const float playerScale = world.getScale().playerHeight / WorldScale{}.playerHeight;

    const auto bounds = camera.visibleGround(5.0f * playerScale);

    const int startX = std::max(
        0,
        static_cast<int>(
            std::floor(bounds.left / cellWidth)
        ) - 2
    );

    const int endX = std::min(
        world.getWidth() - 1,
        static_cast<int>(
            std::ceil(bounds.right / cellWidth)
        ) + 2
    );

    const int startY = std::max(
        0,
        static_cast<int>(
            std::floor(bounds.top / cellHeight)
        ) - 2
    );

    const int endY = std::min(
        world.getHeight() - 1,
        static_cast<int>(
            std::ceil(bounds.bottom / cellHeight)
        ) + 2
    );

    if (startX > endX || startY > endY)
    {
        return;
    }

    // Visible chunk bounds.

    const int startChunkX =
        startX / CHUNK_SIZE;

    const int endChunkX =
        endX / CHUNK_SIZE;

    const int startChunkY =
        startY / CHUNK_SIZE;

    const int endChunkY =
        endY / CHUNK_SIZE;

    // Temporary density-based LOD.

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

    for (int chunkY = startChunkY;
         chunkY <= endChunkY;
         ++chunkY)
    {
        for (int chunkX = startChunkX;
             chunkX <= endChunkX;
             ++chunkX)
        {
            Chunk& chunk = getChunk(
                world,
                chunkX,
                chunkY,
                cellWidth,
                cellHeight
            );

            const int firstX = std::max(
                startX,
                chunkX * CHUNK_SIZE
            );

            const int lastX = std::min(
                endX,
                (chunkX + 1) * CHUNK_SIZE - 1
            );

            const int firstY = std::max(
                startY,
                chunkY * CHUNK_SIZE
            );

            const int lastY = std::min(
                endY,
                (chunkY + 1) * CHUNK_SIZE - 1
            );

            for (int y = firstY;
                 y <= lastY;
                 ++y)
            {
                for (int x = firstX;
                     x <= lastX;
                     ++x)
                {
                    const int localX =
                        x - chunkX * CHUNK_SIZE;

                    const int localY =
                        y - chunkY * CHUNK_SIZE;

                    const int index =
                        localY * CHUNK_SIZE
                        + localX;

                    const auto& glyphs =
                        chunk.tiles[index];

                    for (std::size_t i = 0;
                         i < glyphs.size();
                         i += detailStep)
                    {
                        const VisualGlyph& glyph =
                            glyphs[i];

                        renderer.drawWorld(camera, glyph.character, glyph.x, glyph.y,
                                           glyph.z * playerScale, glyph.scale * playerScale,
                                           glyph.color, glyph.animation, glyph.transform);
                    }
                }
            }
        }
    }

    removeOldChunks();
}

void VisualChunkCache::clear()
{
    chunks.clear();

    frameIndex = 0;
}

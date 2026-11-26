#pragma once

#include "renderer/VisualGlyph.hpp"
#include "renderer/Camera.hpp"
#include "world/World.hpp"

#include <array>
#include <map>
#include <vector>
#include <utility>
#include <cstdint>

class VisualChunkCache
{
private:

    static constexpr int CHUNK_SIZE = 16;

    static constexpr std::size_t MAX_CHUNKS = 128;

    struct Chunk
    {
        std::array<
            std::vector<VisualGlyph>,
            CHUNK_SIZE * CHUNK_SIZE
        > tiles;

        std::uint64_t lastUsed = 0;
    };

    std::map<
        std::pair<int, int>,
        Chunk
    > chunks;

    std::uint64_t frameIndex = 0;

    Chunk& getChunk(
        const World& world,
        int chunkX,
        int chunkY,
        float cellWidth,
        float cellHeight
    );

    void removeOldChunks();

public:

    void render(
        const World& world,
        const Camera& camera,
        GlyphRenderer& renderer,
        float cellWidth,
        float cellHeight
    );

    void clear();
};
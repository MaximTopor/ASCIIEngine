#pragma once

#include "renderer/GlyphRenderer.hpp"
#include "renderer/Camera.hpp"
#include "world/World.hpp"
#include <cstddef>
#include "renderer/RockRenderer.hpp"
#include "renderer/VisualChunkCache.hpp"
#include "renderer/TreeRenderer.hpp"

class WorldRenderer
{
private:
    GlyphRenderer& glyphRenderer;
    const World& world;

    float cellWidth;
    float cellHeight;
    float glyphScale;
    bool benchmark;
    std::size_t visibleTileCount = 0;
    RockRenderer rockRenderer;
    VisualChunkCache vegetationCache;
    TreeRenderer treeRenderer;
    void renderGround(const Camera& camera, int x, int y);
    void renderMaterialSamples(const Camera& camera);

    static constexpr int GROUND_CHUNK_SIZE = 16;

    void renderGroundChunks(
        const Camera& camera,
        int startX,
        int startY,
        int endX,
        int endY
    );

public:
    WorldRenderer(
        GlyphRenderer& glyphRenderer,
        const World& world,
        float glyphScale,
        bool benchmark
    );

    void render(const Camera& camera);

    std::size_t getVisibleTileCount() const;
};

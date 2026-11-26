#pragma once

#include "renderer/GlyphRenderer.hpp"
#include "renderer/Camera.hpp"
#include "world/World.hpp"
#include <cstddef>
#include "renderer/RockRenderer.hpp"
#include "renderer/VisualChunkCache.hpp"

class WorldRenderer
{
private:
    GlyphRenderer& glyphRenderer;
    const World& world;

    float cellWidth;
    float cellHeight;
    float glyphScale;
    std::size_t visibleTileCount = 0;
    RockRenderer rockRenderer;
    VisualChunkCache vegetationCache;
    std::vector<std::vector<VisualGlyph>> treeGlyphs;
    void renderGround(const Camera& camera, int x, int y);

public:
    WorldRenderer(
        GlyphRenderer& glyphRenderer,
        const World& world,
        float glyphScale
    );

    void render(const Camera& camera);

    std::size_t getVisibleTileCount() const;
};

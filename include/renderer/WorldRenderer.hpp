#pragma once

#include "renderer/GlyphRenderer.hpp"
#include "renderer/Camera.hpp"
#include "world/World.hpp"

class WorldRenderer
{
private:
    GlyphRenderer& glyphRenderer;

    float cellWidth;
    float cellHeight;
    float glyphScale;

    void drawGlyph(
        const Camera& camera,
        char32_t symbol,
        float worldX,
        float worldY,
        float scale,
        Color color,
        GlyphAnimation animation = {}
    );

    void renderGround(
        const Camera& camera,
        const Tile& tile,
        int x,
        int y
    );

    void renderVegetation(
        const Camera& camera,
        const Tile& tile,
        int x,
        int y
    );

    void renderObjects(
        const Camera& camera,
        const Tile& tile,
        int x,
        int y
    );

public:
    WorldRenderer(
        GlyphRenderer& glyphRenderer,
        float cellWidth,
        float cellHeight,
        float glyphScale
    );

    void render(
        const World& world,
        const Camera& camera
    );
};
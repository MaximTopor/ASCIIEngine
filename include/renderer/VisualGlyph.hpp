#pragma once

#include "renderer/GlyphRenderer.hpp"

struct VisualGlyph
{
    char32_t character = U'.';

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    float scale = 1.0f;

    Color color;
    GlyphMaterial material;

    GlyphAnimation animation;

    GlyphTransform transform;
};

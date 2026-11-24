#pragma once

#include "renderer/Font.hpp"
#include "renderer/Texture.hpp"

#include <unordered_map>
#include <memory>

struct AtlasGlyph
{
    // Glyph metrics
    int width = 0;
    int height = 0;

    int bearingX = 0;
    int bearingY = 0;

    long advance = 0;

    // UV coordinates
    float u0 = 0.0f;
    float v0 = 0.0f;

    float u1 = 0.0f;
    float v1 = 0.0f;
};

class FontAtlas
{
private:
    int width = 0;
    int height = 0;

    std::unique_ptr<Texture> texture;

    std::unordered_map<char32_t, AtlasGlyph> glyphs;

public:
    explicit FontAtlas(const Font& font);

    const AtlasGlyph& getGlyph(char32_t character) const;

    const Texture& getTexture() const;

    int getWidth() const;
    int getHeight() const;
};
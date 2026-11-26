#include "renderer/FontAtlas.hpp"

#include <algorithm>
#include <vector>
#include <stdexcept>
#include <utility>

FontAtlas::FontAtlas(const Font& font)
{
    constexpr char32_t firstCharacter = 32;
    constexpr char32_t lastCharacter = 126;

    constexpr int columns = 16;
    constexpr int mipLevels = 2;
    constexpr int padding = 1 << mipLevels;

    constexpr int glyphCount =
        lastCharacter - firstCharacter + 1;

    constexpr int rows =
        (glyphCount + columns - 1) / columns;

    int maxWidth = 0;
    int maxHeight = 0;

    std::vector<std::pair<char32_t, Glyph>> loadedGlyphs;

    // 1. Load all printable ASCII characters

    for (char32_t character = firstCharacter;
         character <= lastCharacter;
         ++character)
    {
        Glyph glyph = font.loadGlyph(character);

        maxWidth = std::max(maxWidth, glyph.width);
        maxHeight = std::max(maxHeight, glyph.height);

        loadedGlyphs.emplace_back(
            character,
            std::move(glyph)
        );
    }

    // 2. Calculate atlas dimensions

    const auto alignCell = [padding](int ink) { return (ink + 3 * padding - 1) / padding * padding; };
    const int cellWidth = alignCell(maxWidth);
    const int cellHeight = alignCell(maxHeight);

    width = columns * cellWidth;
    height = rows * cellHeight;

    // 3. Allocate atlas pixels

    std::vector<unsigned char> pixels(
        static_cast<std::size_t>(width) * height,
        0
    );

    // 4. Copy glyphs into the atlas

    for (std::size_t i = 0;
         i < loadedGlyphs.size();
         ++i)
    {
        const auto& [character, glyph] = loadedGlyphs[i];

        const int column =
            static_cast<int>(i % columns);

        const int row =
            static_cast<int>(i / columns);

        const int x = column * cellWidth + padding;
        const int y = row * cellHeight + padding;

        for (int gy = 0; gy < glyph.height; ++gy)
        {
            for (int gx = 0; gx < glyph.width; ++gx)
            {
                const std::size_t sourceIndex =
                    static_cast<std::size_t>(gy)
                    * glyph.width + gx;

                const std::size_t destinationIndex =
                    static_cast<std::size_t>(y + gy)
                    * width + x + gx;

                pixels[destinationIndex] =
                    glyph.pixels[sourceIndex];
            }
        }

        // 5. Store glyph metrics and UV coordinates

        AtlasGlyph atlasGlyph;

        atlasGlyph.width = glyph.width;
        atlasGlyph.height = glyph.height;

        atlasGlyph.bearingX = glyph.bearingX;
        atlasGlyph.bearingY = glyph.bearingY;

        atlasGlyph.advance = glyph.advance;

        atlasGlyph.u0 =
            static_cast<float>(x) / width;

        atlasGlyph.v0 =
            static_cast<float>(y) / height;

        atlasGlyph.u1 =
            static_cast<float>(x + glyph.width) / width;

        atlasGlyph.v1 =
            static_cast<float>(y + glyph.height) / height;

        glyphs.emplace(character, atlasGlyph);
    }

    // 6. Upload the complete atlas to OpenGL

    texture = std::make_unique<Texture>(
        width,
        height,
        pixels.data(),
        mipLevels
    );
}

const AtlasGlyph& FontAtlas::getGlyph(
    char32_t character
) const
{
    const auto iterator = glyphs.find(character);

    if (iterator == glyphs.end())
    {
        throw std::runtime_error(
            "Character not found in font atlas"
        );
    }

    return iterator->second;
}

const Texture& FontAtlas::getTexture() const
{
    return *texture;
}

int FontAtlas::getWidth() const
{
    return width;
}

int FontAtlas::getHeight() const
{
    return height;
}

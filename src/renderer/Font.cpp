#include "renderer/Font.hpp"

#include <stdexcept>
#include <algorithm>
#include <cstddef>

Font::Font(
    const std::string& path,
    unsigned int pixelSize
)
{
    if (FT_Init_FreeType(&library))
    {
        throw std::runtime_error(
            "Failed to initialize FreeType"
        );
    }

    if (FT_New_Face(
        library,
        path.c_str(),
        0,
        &face
    ))
    {
        FT_Done_FreeType(library);

        throw std::runtime_error(
            "Failed to load font: " + path
        );
    }

    if (FT_Set_Pixel_Sizes(face, 0, pixelSize))
    {
        FT_Done_Face(face);
        FT_Done_FreeType(library);

        throw std::runtime_error(
            "Failed to set font size"
        );
    }
}

Font::~Font()
{
    FT_Done_Face(face);
    FT_Done_FreeType(library);
}

Glyph Font::loadGlyph(char32_t character) const
{
    if (FT_Get_Char_Index(face, character) == 0)
    {
        throw std::runtime_error(
            "Character not found in font"
        );
    }

    if (FT_Load_Char(
        face,
        character,
        FT_LOAD_RENDER
    ))
    {
        throw std::runtime_error(
            "Failed to load glyph"
        );
    }

    const FT_GlyphSlot slot = face->glyph;
    const FT_Bitmap& bitmap = slot->bitmap;

    Glyph glyph;

    glyph.width = static_cast<int>(bitmap.width);
    glyph.height = static_cast<int>(bitmap.rows);

    glyph.bearingX = slot->bitmap_left;
    glyph.bearingY = slot->bitmap_top;

    glyph.advance = slot->advance.x;

    if (glyph.width == 0 || glyph.height == 0)
    {
        return glyph;
    }

    if (bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
    {
        throw std::runtime_error(
            "Unsupported glyph pixel format"
        );
    }

    glyph.pixels.resize(
        static_cast<std::size_t>(glyph.width)
        * glyph.height
    );

    for (int y = 0; y < glyph.height; ++y)
    {
        const unsigned char* source =
            bitmap.buffer
            + static_cast<std::ptrdiff_t>(y)
            * bitmap.pitch;

        unsigned char* destination =
            glyph.pixels.data()
            + static_cast<std::size_t>(y)
            * glyph.width;

        std::copy_n(
            source,
            glyph.width,
            destination
        );
    }

    return glyph;
}
#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>
#include <vector>

struct Glyph
{
    int width = 0;
    int height = 0;

    int bearingX = 0;
    int bearingY = 0;

    long advance = 0;

    std::vector<unsigned char> pixels;
};

class Font
{
private:
    FT_Library library = nullptr;
    FT_Face face = nullptr;

public:
    Font(
        const std::string& path,
        unsigned int pixelSize
    );

    ~Font();

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    Glyph loadGlyph(char32_t character) const;
};
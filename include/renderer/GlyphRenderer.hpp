#pragma once

#include "renderer/FontAtlas.hpp"
#include "renderer/Shader.hpp"
#include "renderer/Mesh.hpp"

#include <vector>

struct Color
{
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
};

struct GlyphInstance
{
    // Position
    float x;
    float y;

    // Size
    float width;
    float height;

    // Texture coordinates
    float u0;
    float v0;
    float u1;
    float v1;

    // Color
    float r;
    float g;
    float b;
};

class GlyphRenderer
{
private:
    const FontAtlas& atlas;

    Shader shader;
    Mesh quad;

    GLuint instanceVBO = 0;

    std::vector<GlyphInstance> instances;

    void initializeInstanceBuffer();

public:
    explicit GlyphRenderer(const FontAtlas& atlas);

    ~GlyphRenderer();

    GlyphRenderer(const GlyphRenderer&) = delete;
    GlyphRenderer& operator=(const GlyphRenderer&) = delete;

    void setScreenSize(int width, int height);

    void begin();

    void draw(
        char32_t character,
        float x,
        float y,
        float scale = 1.0f,
        Color color = {}
    );

    void flush();
};
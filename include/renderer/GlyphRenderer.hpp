#pragma once

#include "renderer/FontAtlas.hpp"
#include "renderer/Shader.hpp"
#include "renderer/Mesh.hpp"
#include "renderer/Lighting.hpp"
#include <vector>

enum class AnimationType
{
    None = 0,
    Wind = 1,
    Water = 2
};

struct GlyphAnimation
{
    AnimationType type = AnimationType::None;

    float phase = 0.0f;
    float amplitude = 0.0f;
};

struct GlyphInstance
{
    // Position
    float x;
    float y;

    // Size
    float width;
    float height;

    // UV coordinates
    float u0;
    float v0;
    float u1;
    float v1;

    // Color
    float r;
    float g;
    float b;

    // Animation
    float animationType;
    float phase;
    float amplitude;
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

    static constexpr int MAX_LIGHTS = 32;

public:
    explicit GlyphRenderer(const FontAtlas& atlas);

    ~GlyphRenderer();

    GlyphRenderer(const GlyphRenderer&) = delete;
    GlyphRenderer& operator=(const GlyphRenderer&) = delete;

    void setScreenSize(int width, int height);

    void setTime(float time);

    void begin();

    void draw(
        char32_t character,
        float x,
        float y,
        float scale = 1.0f,
        Color color = {},
        GlyphAnimation animation = {}
    );

    void flush();

    void setAmbientLight(Color color);

    void setPointLights(
        const std::vector<PointLight>& lights
    );
};
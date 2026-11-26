#pragma once

#include "renderer/FontAtlas.hpp"
#include "renderer/Shader.hpp"
#include "renderer/Mesh.hpp"
#include "renderer/Lighting.hpp"
#include "renderer/Camera.hpp"
#include <cstddef>
#include <optional>
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

struct GlyphTransform
{
    float rotation = 0.0f;

    float opacity = 1.0f;

    float pivotX = 0.5f;
    float pivotY = 1.0f;
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

    // Animation

    float animationType;
    float phase;
    float amplitude;

    // Transform

    float rotation;
    float opacity;
    float pivotX;
    float pivotY;
    float lightOffsetY;
};

enum class GlyphAnchor
{
    Baseline,
    Center
};

class GlyphRenderer
{
private:

    const FontAtlas& atlas;

    Shader shader;
    Mesh quad;

    GLuint instanceVBO = 0;

    GLuint lightmapTexture = 0;

    std::vector<GlyphInstance> instances;
    std::vector<GlyphInstance> orderedInstances;
    std::vector<float> depths;

    void initializeInstanceBuffer();
    void appendDepthSortedInstances();

public:
    std::size_t getInstanceCount() const;

    explicit GlyphRenderer(
        const FontAtlas& atlas
    );

    ~GlyphRenderer();

    GlyphRenderer(
        const GlyphRenderer&
    ) = delete;

    GlyphRenderer& operator=(
        const GlyphRenderer&
    ) = delete;

    void setScreenSize(
        int width,
        int height
    );

    void setTime(float time);

    void setLightmap(GLuint texture);

    void begin();

    void draw(
        char32_t character,
        float x,
        float y,
        float scale = 1.0f,
        Color color = {},
        GlyphAnimation animation = {},
        GlyphTransform transform = {},
        GlyphAnchor anchor = GlyphAnchor::Baseline,
        float lightOffsetY = 0.0f,
        std::optional<float> depth = {}
    );

    void drawWorld(const Camera& camera, char32_t character, float x, float y, float z,
                   float scale, Color color, GlyphAnimation animation = {},
                   GlyphTransform transform = {}, GlyphAnchor anchor = GlyphAnchor::Center,
                   bool groundLayer = false);

    void flush();
};

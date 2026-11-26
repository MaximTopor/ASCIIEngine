#include "renderer/GlyphRenderer.hpp"

#include <cstddef>
#include <vector>
#include <algorithm>
#include <array>
#include <string>

namespace
{
    const std::vector<Vertex> vertices = {
        {-0.5f,  0.5f, 0.0f, 0.0f, 1.0f},
        { 0.5f,  0.5f, 0.0f, 1.0f, 1.0f},
        { 0.5f, -0.5f, 0.0f, 1.0f, 0.0f},
        {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f}
    };

    const std::vector<GLuint> indices = {
        0, 3, 2,
        0, 2, 1
    };
}

GlyphRenderer::GlyphRenderer(const FontAtlas& atlas)
    : atlas(atlas),
      shader(
          "shaders/glyph.vert",
          "shaders/glyph.frag"
      ),
      quad(vertices, indices)
{
    instances.reserve(10000);

    shader.setInt("glyphTexture", 0);
    shader.setInt("uLightmap", 1);

    initializeInstanceBuffer();
}

GlyphRenderer::~GlyphRenderer()
{
    glDeleteBuffers(1, &instanceVBO);
}

void GlyphRenderer::initializeInstanceBuffer()
{
    quad.bind();

    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    // Instance position
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(GlyphInstance),
        reinterpret_cast<void*>(
            offsetof(GlyphInstance, x)
        )
    );

    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // Instance size
    glVertexAttribPointer(
        3,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(GlyphInstance),
        reinterpret_cast<void*>(
            offsetof(GlyphInstance, width)
        )
    );

    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    // Instance UV coordinates
    glVertexAttribPointer(
        4,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(GlyphInstance),
        reinterpret_cast<void*>(
            offsetof(GlyphInstance, u0)
        )
    );

    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    // Instance color
    glVertexAttribPointer(
        5,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(GlyphInstance),
        reinterpret_cast<void*>(
            offsetof(GlyphInstance, r)
        )
    );

    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    // Instance animation
    glVertexAttribPointer(
        6,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(GlyphInstance),
        reinterpret_cast<void*>(
            offsetof(GlyphInstance, animationType)
        )
    );

    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);

    // Instance transform
    glVertexAttribPointer(
        7,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(GlyphInstance),
        reinterpret_cast<void*>(
            offsetof(GlyphInstance, rotation)
        )
    );

    glEnableVertexAttribArray(7);
    glVertexAttribDivisor(7, 1);

    glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, sizeof(GlyphInstance),
                          reinterpret_cast<void*>(offsetof(GlyphInstance, lightOffsetY)));
    glEnableVertexAttribArray(8);
    glVertexAttribDivisor(8, 1);

    quad.unbind();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GlyphRenderer::setScreenSize(
    int width,
    int height
)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    shader.setVec2(
        "uScreenSize",
        static_cast<float>(width),
        static_cast<float>(height)
    );
}

void GlyphRenderer::begin()
{
    instances.clear();
    orderedInstances.clear();
    depths.clear();
}

void GlyphRenderer::drawWorld(const Camera& camera, char32_t character, float x, float y, float z,
                              float scale, Color color, GlyphAnimation animation,
                              GlyphTransform transform, GlyphAnchor anchor, bool groundLayer)
{
    const auto screen = camera.project(x, y, z);
    const float screenScale = scale * camera.getZoom();
    // The atlas uses a 64 px em. Include margin for bearings, rotation and wind.
    if (!camera.isVisible(screen, 128.0f * screenScale))
    {
        return;
    }
    const auto depth = groundLayer ? std::nullopt : std::optional{camera.depth(x, y, z)};
    draw(character, screen.x, screen.y, screenScale, color, animation,
         transform, anchor, camera.elevationPixels(z), depth);
}

void GlyphRenderer::draw(
    char32_t character,
    float x,
    float y,
    float scale,
    Color color,
    GlyphAnimation animation,
    GlyphTransform transform,
    GlyphAnchor anchor,
    float lightOffsetY,
    std::optional<float> depth
)
{
    const AtlasGlyph& glyph =
        atlas.getGlyph(character);

    if (glyph.width == 0 ||
        glyph.height == 0)
    {
        return;
    }

    if (transform.opacity <= 0.0f)
    {
        return;
    }

    GlyphInstance instance
    {
        // Position

        x + (anchor == GlyphAnchor::Center ? -0.5f * glyph.width : glyph.bearingX) * scale,
        y - (anchor == GlyphAnchor::Center ? 0.5f * glyph.height : glyph.bearingY) * scale,

        // Size

        glyph.width * scale,
        glyph.height * scale,

        // UV

        glyph.u0,
        glyph.v0,
        glyph.u1,
        glyph.v1,

        // Color

        color.r,
        color.g,
        color.b,

        // Animation

        static_cast<float>(animation.type),
        animation.phase,
        animation.amplitude,

        // Transform

        transform.rotation,

        std::clamp(
            transform.opacity,
            0.0f,
            1.0f
        ),

        transform.pivotX,
        transform.pivotY,
        lightOffsetY
    };

    if (depth)
    {
        depths.push_back(*depth);
        orderedInstances.push_back(instance);
    }
    else
    {
        instances.push_back(instance);
    }
}

void GlyphRenderer::appendDepthSortedInstances()
{
    if (depths.empty())
    {
        return;
    }
    // Stable back-to-front buckets retain submission order within a small depth interval.
    constexpr std::size_t bucketCount = 4096;
    std::array<std::size_t, bucketCount> offsets{};
    const auto [minimum, maximum] = std::minmax_element(depths.begin(), depths.end());
    const float range = *maximum - *minimum;
    const float scale = range > 0.0f ? static_cast<float>(bucketCount - 1) / range : 0.0f;
    const auto bucket = [near = *minimum, scale](float depth)
    {
        return std::min(bucketCount - 1, static_cast<std::size_t>((depth - near) * scale));
    };
    for (float depth : depths)
    {
        ++offsets[bucket(depth)];
    }
    std::size_t offset = instances.size();
    for (auto& count : offsets)
    {
        const auto next = offset + count;
        count = offset;
        offset = next;
    }
    instances.resize(offset);
    for (std::size_t i = 0; i < depths.size(); ++i)
    {
        instances[offsets[bucket(depths[i])]++] = orderedInstances[i];
    }
    orderedInstances.clear();
    depths.clear();
}

void GlyphRenderer::flush()
{
    appendDepthSortedInstances();
    if (instances.empty())
    {
        return;
    }

    shader.use();

    // Alpha blending

    glEnable(GL_BLEND);

    glBlendEquation(GL_FUNC_ADD);

    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA
    );

    // Font Atlas

    atlas.getTexture().bind(0);

    // Shared Light Map

    glBindTextureUnit(
        1,
        lightmapTexture
    );

    quad.bind();

    glBindBuffer(
        GL_ARRAY_BUFFER,
        instanceVBO
    );

    glBufferData(
        GL_ARRAY_BUFFER,
        instances.size() * sizeof(GlyphInstance),
        instances.data(),
        GL_STREAM_DRAW
    );

    glDrawElementsInstanced(
        GL_TRIANGLES,
        quad.getIndexCount(),
        GL_UNSIGNED_INT,
        nullptr,
        static_cast<GLsizei>(instances.size())
    );

    quad.unbind();

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDisable(GL_BLEND);

    instances.clear();
}

void GlyphRenderer::setTime(float time)
{
    shader.setFloat("uTime", time);
}
void GlyphRenderer::setLightmap(GLuint texture)
{
    lightmapTexture = texture;
}
std::size_t GlyphRenderer::getInstanceCount() const
{
    return instances.size() + orderedInstances.size();
}

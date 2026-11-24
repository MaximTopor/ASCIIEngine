#include "renderer/GlyphRenderer.hpp"

#include <cstddef>
#include <vector>
#include <algorithm>
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

    shader.setInt("uLightCount", 0);

    setAmbientLight({1.0f, 1.0f, 1.0f});

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
}

void GlyphRenderer::draw(
    char32_t character,
    float x,
    float y,
    float scale,
    Color color,
    GlyphAnimation animation
)
{
    const AtlasGlyph& glyph =
        atlas.getGlyph(character);

    if (glyph.width == 0 || glyph.height == 0)
    {
        return;
    }

    GlyphInstance instance{
        x + glyph.bearingX * scale,
        y - glyph.bearingY * scale,

        glyph.width * scale,
        glyph.height * scale,

        glyph.u0,
        glyph.v0,
        glyph.u1,
        glyph.v1,

        color.r,
        color.g,
        color.b,

        static_cast<float>(animation.type),
        animation.phase,
        animation.amplitude
    };

    instances.push_back(instance);
}

void GlyphRenderer::flush()
{
    if (instances.empty())
    {
        return;
    }

    shader.use();

    atlas.getTexture().bind(0);

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

    instances.clear();
}

void GlyphRenderer::setTime(float time)
{
    shader.setFloat("uTime", time);
}

void GlyphRenderer::setAmbientLight(Color color)
{
    shader.setVec3(
        "uAmbientLight",
        color.r,
        color.g,
        color.b
    );
}

void GlyphRenderer::setPointLights(
    const std::vector<PointLight>& lights
)
{
    const int count = static_cast<int>(
        std::min(
            lights.size(),
            static_cast<std::size_t>(MAX_LIGHTS)
        )
    );

    shader.setInt("uLightCount", count);

    for (int i = 0; i < count; ++i)
    {
        const PointLight& light = lights[i];

        const std::string index =
            "[" + std::to_string(i) + "]";

        shader.setVec4(
            "uLightPositionRadius" + index,

            light.x,
            light.y,
            light.radius,
            light.intensity
        );

        shader.setVec3(
            "uLightColor" + index,

            light.color.r,
            light.color.g,
            light.color.b
        );
    }
}
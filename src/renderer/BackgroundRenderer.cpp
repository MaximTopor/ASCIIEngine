#include "renderer/BackgroundRenderer.hpp"

#include <algorithm>
#include <string>
#include <stdexcept>

BackgroundRenderer::BackgroundRenderer()
    : lightmapShader(
          "shaders/fullscreen.vert",
          "shaders/lightmap.frag"
      ),
      backgroundShader(
          "shaders/fullscreen.vert",
          "shaders/background.frag"
      )
{
    glGenVertexArrays(1, &vao);

    glGenFramebuffers(1, &framebuffer);

    lightmapShader.setInt("uOcclusionMap", 1);
    lightmapShader.setInt("uShadowsEnabled", 1);
    lightmapShader.setInt("uLightCount", 0);

    backgroundShader.setInt("uLightmap", 0);

    backgroundShader.setFloat(
        "uBackgroundStrength",
        1.0f
    );
}

BackgroundRenderer::~BackgroundRenderer()
{
    glDeleteTextures(1, &lightmapTexture);

    glDeleteFramebuffers(1, &framebuffer);

    glDeleteVertexArrays(1, &vao);
}

void BackgroundRenderer::resizeLightmap(
    int width,
    int height
)
{
    const int newWidth = std::max(
        1,
        width / RESOLUTION_DIVISOR
    );

    const int newHeight = std::max(
        1,
        height / RESOLUTION_DIVISOR
    );

    if (newWidth == lightmapWidth &&
        newHeight == lightmapHeight)
    {
        return;
    }

    lightmapWidth = newWidth;
    lightmapHeight = newHeight;

    // Delete old texture

    if (lightmapTexture != 0)
    {
        glDeleteTextures(1, &lightmapTexture);
    }

    // Create new lightmap texture

    glCreateTextures(
        GL_TEXTURE_2D,
        1,
        &lightmapTexture
    );

    glTextureStorage2D(
        lightmapTexture,
        1,
        GL_RGB16F,
        lightmapWidth,
        lightmapHeight
    );

    // Smooth interpolation

    glTextureParameteri(
        lightmapTexture,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR
    );

    glTextureParameteri(
        lightmapTexture,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    glTextureParameteri(
        lightmapTexture,
        GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_EDGE
    );

    glTextureParameteri(
        lightmapTexture,
        GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_EDGE
    );

    // Attach texture to framebuffer

    glNamedFramebufferTexture(
        framebuffer,
        GL_COLOR_ATTACHMENT0,
        lightmapTexture,
        0
    );

    glNamedFramebufferDrawBuffer(
        framebuffer,
        GL_COLOR_ATTACHMENT0
    );

    // Validate framebuffer

    if (glCheckNamedFramebufferStatus(
            framebuffer,
            GL_FRAMEBUFFER
        ) != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error(
            "Lightmap framebuffer is incomplete"
        );
    }
}

void BackgroundRenderer::setLighting(
    Color ambient,
    const std::vector<PointLight>& lights
)
{
    lightmapShader.setVec3(
        "uAmbientLight",
        ambient.r,
        ambient.g,
        ambient.b
    );

    const int count = static_cast<int>(
        std::min(
            lights.size(),
            static_cast<std::size_t>(MAX_LIGHTS)
        )
    );

    lightmapShader.setInt(
        "uLightCount",
        count
    );

    for (int i = 0; i < count; ++i)
    {
        const PointLight& light = lights[i];

        const std::string index =
            "[" + std::to_string(i) + "]";

        lightmapShader.setVec4(
            "uLightPositionRadius" + index,
            light.x,
            light.y,
            light.radius,
            light.intensity
        );

        lightmapShader.setVec3(
            "uLightColor" + index,
            light.color.r,
            light.color.g,
            light.color.b
        );

        lightmapShader.setFloat(
            "uLightSourceRadius" + index,
            light.sourceRadius
        );
    }
}

void BackgroundRenderer::render(
    const Camera& camera,
    const OcclusionMap& occlusionMap,
    int screenWidth,
    int screenHeight,
    float cellWidth,
    float cellHeight
)
{
    if (screenWidth <= 0 ||
        screenHeight <= 0)
    {
        return;
    }

    resizeLightmap(
        screenWidth,
        screenHeight
    );

    // Camera uniforms

    lightmapShader.setVec2(
        "uScreenSize",
        static_cast<float>(screenWidth),
        static_cast<float>(screenHeight)
    );

    lightmapShader.setVec2(
        "uWorldOrigin",
        camera.screenToWorldX(0.0f),
        camera.screenToWorldY(0.0f)
    );

    lightmapShader.setVec2(
        "uCellSize",
        cellWidth,
        cellHeight
    );

    lightmapShader.setFloat(
        "uCameraZoom",
        camera.getZoom()
    );

    // PASS 1: Render lighting into texture

    glBindFramebuffer(
        GL_FRAMEBUFFER,
        framebuffer
    );

    glViewport(
        0,
        0,
        lightmapWidth,
        lightmapHeight
    );

    lightmapShader.use();

    occlusionMap.getTexture().bind(1);

    glBindVertexArray(vao);

    glDrawArrays(
        GL_TRIANGLES,
        0,
        3
    );

    // PASS 2: Render background to screen

    glBindFramebuffer(
        GL_FRAMEBUFFER,
        0
    );

    glViewport(
        0,
        0,
        screenWidth,
        screenHeight
    );

    backgroundShader.use();

    glBindTextureUnit(
        0,
        lightmapTexture
    );

    glDrawArrays(
        GL_TRIANGLES,
        0,
        3
    );

    glBindVertexArray(0);
}
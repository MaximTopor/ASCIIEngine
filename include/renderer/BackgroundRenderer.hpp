#pragma once

#include "renderer/Shader.hpp"
#include "renderer/Lighting.hpp"
#include "renderer/Camera.hpp"
#include "renderer/OcclusionMap.hpp"

#include <glad/gl.h>

class BackgroundRenderer
{
private:
    Shader lightmapShader;
    Shader backgroundShader;

    GLuint vao = 0;
    GLuint framebuffer = 0;
    GLuint lightmapTexture = 0;

    int lightmapWidth = 0;
    int lightmapHeight = 0;

    static constexpr int RESOLUTION_DIVISOR = 4;
    static constexpr int MAX_LIGHTS = 32;

    void resizeLightmap(int width, int height);

public:
    BackgroundRenderer();
    ~BackgroundRenderer();
    GLuint getLightmapTexture() const;
    BackgroundRenderer(
        const BackgroundRenderer&
    ) = delete;

    BackgroundRenderer& operator=(
        const BackgroundRenderer&
    ) = delete;

    void setLighting(
        Color ambient,
        std::span<const PointLight> lights,
        const Camera& camera
    );

    void render(
        const Camera& camera,
        const OcclusionMap& occlusionMap,
        int screenWidth,
        int screenHeight,
        float cellWidth,
        float cellHeight
    );
};

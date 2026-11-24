#pragma once

#include "renderer/Shader.hpp"
#include "renderer/Mesh.hpp"
#include "renderer/Texture.hpp"

class Renderer
{
public:
    void setClearColor(
        float r,
        float g,
        float b,
        float a = 1.0f
    ) const;

    void clear() const;

    void draw(
        const Mesh& mesh,
        const Shader& shader,
        const Texture& texture
    ) const;
};
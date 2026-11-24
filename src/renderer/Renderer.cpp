#include "renderer/Renderer.hpp"

void Renderer::setClearColor(
    float r,
    float g,
    float b,
    float a
) const
{
    glClearColor(r, g, b, a);
}

void Renderer::clear() const
{
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::draw(
    const Mesh& mesh,
    const Shader& shader,
    const Texture& texture
) const
{
    shader.use();
    texture.bind(0);
    mesh.bind();

    glDrawElements(
        GL_TRIANGLES,
        mesh.getIndexCount(),
        GL_UNSIGNED_INT,
        nullptr
    );

    mesh.unbind();
}
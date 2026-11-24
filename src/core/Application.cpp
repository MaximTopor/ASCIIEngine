#include "core/Application.hpp"

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

    constexpr unsigned char pixels[] = {
        0, 0,   0,   255, 255, 0,   0,   0,
        0, 0,   255, 0,   0,   255, 0,   0,
        0, 255, 0,   0,   0,   0,   255, 0,
        0, 255, 0,   0,   0,   0,   255, 0,
        0, 255, 255, 255, 255, 255, 255, 0,
        0, 255, 0,   0,   0,   0,   255, 0,
        0, 255, 0,   0,   0,   0,   255, 0,
        0, 0,   0,   0,   0,   0,   0,   0
    };
}

Application::Application()
    : window(1280, 720, "ASCII Engine"),
      renderer(),
      shader(
          "shaders/vertex.glsl",
          "shaders/fragment.glsl"
      ),
      mesh(vertices, indices),
      texture(8, 8, pixels)
{
    shader.setInt("glyphTexture", 0);

    renderer.setClearColor(
        0.05f, 0.08f, 0.06f
    );
}

void Application::run()
{
    while (!window.shouldClose())
    {
        window.pollEvents();

        processInput();

        update();

        render();

        window.swapBuffers();
    }
}

void Application::processInput()
{
    if (window.isKeyPressed(GLFW_KEY_ESCAPE))
    {
        window.close();
    }
}

void Application::update()
{
}

void Application::render()
{
    renderer.clear();

    renderer.draw(mesh, shader, texture);
}
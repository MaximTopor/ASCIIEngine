#pragma once

#include "core/Window.hpp"

#include "renderer/Shader.hpp"
#include "renderer/Mesh.hpp"
#include "renderer/Texture.hpp"
#include "renderer/Renderer.hpp"

class Application
{
private:
    Window window;
    Renderer renderer;

    Shader shader;
    Mesh mesh;
    Texture texture;

    void processInput();
    void update();
    void render();

public:
    Application();

    void run();
};
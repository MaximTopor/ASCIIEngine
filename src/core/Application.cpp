#include "core/Application.hpp"

#include <cmath>
#include <string>
#include <algorithm>
#include <vector>

Application::Application()
    : window(1280, 720, "ASCII Engine"),
      renderer(),
      font(
          "assets/fonts/ttf/JetBrainsMono-Regular.ttf",
          64
      ),
      fontAtlas(font),
      glyphRenderer(fontAtlas),
      camera(),
      worldRenderer(
          glyphRenderer,
          CELL_WIDTH,
          CELL_HEIGHT,
          GLYPH_SCALE
      ),
      world(500, 500, 42),
      player(0, 0)
{
    const auto [spawnX, spawnY] = world.findSpawn();

    player.setPosition(spawnX, spawnY);

    renderer.setClearColor(
        0.05f, 0.08f, 0.06f
    );

    camera.setZoom(1.0f);
}

void Application::run()
{
    double lastFPSTime = glfwGetTime();
    double lastFrameTime = lastFPSTime;

    int frameCount = 0;

    while (!window.shouldClose())
    {
        const double frameTime = glfwGetTime();

        const float deltaTime = std::clamp(
            static_cast<float>(
                frameTime - lastFrameTime
            ),
            0.0f,
            0.1f
        );

        lastFrameTime = frameTime;

        window.pollEvents();

        processInput();
        update(deltaTime);
        render();

        window.swapBuffers();

        ++frameCount;

        const double currentTime = glfwGetTime();

        const double elapsed =
            currentTime - lastFPSTime;

        if (elapsed >= 1.0)
        {
            const int fps = static_cast<int>(
                frameCount / elapsed
            );

            const int hours =
                static_cast<int>(lighting.getHour());

            const int minutes =
                static_cast<int>(
                    (lighting.getHour() - hours)
                    * 60.0f
                );

            const std::string time =
                std::string(hours < 10 ? "0" : "")
                + std::to_string(hours)
                + ":"
                + (minutes < 10 ? "0" : "")
                + std::to_string(minutes);

            window.setTitle(
                "ASCII Engine | FPS: "
                + std::to_string(fps)
                + " | Zoom: "
                + std::to_string(camera.getZoom())
                + " | Time: "
                + time
                + " | Torch: "
                + (torchEnabled ? "ON" : "OFF")
            );

            frameCount = 0;

            lastFPSTime = currentTime;
        }
    }
}

void Application::processInput()
{
    if (window.isKeyPressed(GLFW_KEY_ESCAPE))
    {
        window.close();
        return;
    }

    // Toggle torch

    const bool lKey =
        window.isKeyPressed(GLFW_KEY_L);

    if (lKey && !previousLKey)
    {
        torchEnabled = !torchEnabled;
    }

    previousLKey = lKey;

    // Pause or resume day/night cycle

    const bool pKey =
        window.isKeyPressed(GLFW_KEY_P);

    if (pKey && !previousPKey)
    {
        lighting.setCycleEnabled(
            !lighting.isCycleEnabled()
        );
    }

    previousPKey = pKey;

    // Time presets

    if (window.isKeyPressed(GLFW_KEY_1))
    {
        lighting.setHour(12.0f);
    }

    if (window.isKeyPressed(GLFW_KEY_2))
    {
        lighting.setHour(18.0f);
    }

    if (window.isKeyPressed(GLFW_KEY_3))
    {
        lighting.setHour(0.0f);
    }

    if (window.isKeyPressed(GLFW_KEY_4))
    {
        lighting.setHour(6.0f);
    }

    const double scroll = window.consumeScroll();

    if (scroll != 0.0)
    {
        camera.changeZoom(
            static_cast<float>(
                std::pow(1.15, scroll)
            )
        );
    }

    if (window.isKeyPressed(GLFW_KEY_R))
    {
        camera.setZoom(1.0f);
    }

    const double currentTime = glfwGetTime();

    if (currentTime - lastMoveTime < MOVE_INTERVAL)
    {
        return;
    }

    int dx = 0;
    int dy = 0;

    if (window.isKeyPressed(GLFW_KEY_W))
    {
        dy = -1;
    }
    else if (window.isKeyPressed(GLFW_KEY_S))
    {
        dy = 1;
    }
    else if (window.isKeyPressed(GLFW_KEY_A))
    {
        dx = -1;
    }
    else if (window.isKeyPressed(GLFW_KEY_D))
    {
        dx = 1;
    }

    if (dx != 0 || dy != 0)
    {
        player.move(dx, dy, world);
        lastMoveTime = currentTime;
    }
}

void Application::update(float deltaTime)
{
    lighting.update(deltaTime);

    camera.setPosition(
        (player.getX() + 0.5f) * CELL_WIDTH,
        (player.getY() + 0.5f) * CELL_HEIGHT
    );
}

void Application::render()
{
    int width, height;

    window.getFramebufferSize(width, height);

    if (width <= 0 || height <= 0)
    {
        return;
    }

    camera.setViewportSize(width, height);

    glyphRenderer.setScreenSize(width, height);

    glyphRenderer.setTime(
        static_cast<float>(glfwGetTime())
    );

    // Lighting

    updateLighting();

    renderer.clear();

    // Rendering

    glyphRenderer.begin();

    worldRenderer.render(world, camera);

    renderPlayer();

    glyphRenderer.flush();
}

void Application::renderPlayer()
{
    const float worldX =
        player.getX() * CELL_WIDTH + 4.0f;

    const float worldY =
        (player.getY() + 1) * CELL_HEIGHT - 2.0f;

    glyphRenderer.draw(
        U'@',
        camera.worldToScreenX(worldX),
        camera.worldToScreenY(worldY),
        GLYPH_SCALE * camera.getZoom(),
        {1.0f, 1.0f, 1.0f}
    );
}

void Application::updateLighting()
{
    const Color ambient =
        lighting.getAmbientColor();

    glyphRenderer.setAmbientLight(ambient);

    // Adjust background brightness

    renderer.setClearColor(
        0.05f * ambient.r,
        0.08f * ambient.g,
        0.06f * ambient.b
    );

    std::vector<PointLight> lights;

    // Player torch

    if (torchEnabled)
    {
        const float worldX =
            (player.getX() + 0.5f) * CELL_WIDTH;

        const float worldY =
            (player.getY() + 0.5f) * CELL_HEIGHT;

        PointLight torch;

        torch.x =
            camera.worldToScreenX(worldX);

        torch.y =
            camera.worldToScreenY(worldY);

        torch.radius =
            220.0f * camera.getZoom();

        const float time =
            static_cast<float>(glfwGetTime());

        // Subtle torch flickering

        const float flicker =
            1.0f
            + std::sin(time * 7.0f) * 0.04f
            + std::sin(time * 13.0f) * 0.025f;

        torch.intensity =
            1.8f * flicker;

        torch.color = {
            1.0f,
            0.72f,
            0.35f
        };

        lights.push_back(torch);
    }

    glyphRenderer.setPointLights(lights);
}
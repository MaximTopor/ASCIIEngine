#include "core/Application.hpp"

#include <algorithm>
#include <cmath>
#include <string>

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
    world(500, 500, 42),
    player(0, 0)
{
    renderer.setClearColor(
        0.05f, 0.08f, 0.06f
    );

    camera.setZoom(1.0f);

    const auto [spawnX, spawnY] = world.findSpawn();

    player.setPosition(spawnX, spawnY);
}

void Application::run()
{
    double lastFPSTime = glfwGetTime();

    int frameCount = 0;

    while (!window.shouldClose())
    {
        window.pollEvents();

        processInput();
        update();
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

            window.setTitle(
                "ASCII Engine | FPS: "
                + std::to_string(fps)
                + " | Zoom: "
                + std::to_string(camera.getZoom())
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

    // Mouse wheel zoom

    const double scroll = window.consumeScroll();

    if (scroll != 0.0)
    {
        camera.changeZoom(
            static_cast<float>(
                std::pow(1.15, scroll)
            )
        );
    }

    // Reset zoom

    if (window.isKeyPressed(GLFW_KEY_R))
    {
        camera.setZoom(1.0f);
    }

    // Player movement

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

void Application::update()
{
    // Camera follows the player

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

    renderer.clear();

    glyphRenderer.setScreenSize(width, height);

    glyphRenderer.begin();

    renderWorld();
    renderPlayer();

    glyphRenderer.flush();
}

void Application::renderWorld()
{
    // Calculate visible world boundaries

    const float halfWidth =
        camera.getVisibleWidth() / 2.0f;

    const float halfHeight =
        camera.getVisibleHeight() / 2.0f;

    const float left =
        camera.getX() - halfWidth;

    const float right =
        camera.getX() + halfWidth;

    const float top =
        camera.getY() - halfHeight;

    const float bottom =
        camera.getY() + halfHeight;

    // Convert world coordinates to tile coordinates

    const int startX = std::clamp(
        static_cast<int>(
            std::floor(left / CELL_WIDTH)
        ) - 2,
        0,
        world.getWidth()
    );

    const int endX = std::clamp(
        static_cast<int>(
            std::ceil(right / CELL_WIDTH)
        ) + 2,
        0,
        world.getWidth()
    );

    const int startY = std::clamp(
        static_cast<int>(
            std::floor(top / CELL_HEIGHT)
        ) - 2,
        0,
        world.getHeight()
    );

    const int endY = std::clamp(
        static_cast<int>(
            std::ceil(bottom / CELL_HEIGHT)
        ) + 2,
        0,
        world.getHeight()
    );

    const float scale =
        GLYPH_SCALE * camera.getZoom();

    // Render only visible tiles

    for (int y = startY; y < endY; ++y)
    {
        for (int x = startX; x < endX; ++x)
        {
            if (x == player.getX() &&
                y == player.getY())
            {
                continue;
            }

            const Tile& tile = world.getTile(x, y);

            char32_t symbol = U'.';

            Color color;

            switch (tile.type)
            {
                case TileType::DeepWater:
                    symbol = U'~';
                    color = {0.12f, 0.25f, 0.65f};
                    break;

                case TileType::Water:
                    symbol = U'~';
                    color = {0.25f, 0.55f, 0.85f};
                    break;

                case TileType::Sand:
                    symbol = U':';
                    color = {0.78f, 0.70f, 0.45f};
                    break;

                case TileType::Ground:
                    symbol = U'.';
                    color = {0.45f, 0.48f, 0.28f};
                    break;

                case TileType::Grass:
                    symbol = U',';
                    color = {0.45f, 0.70f, 0.25f};
                    break;

                case TileType::Tree:
                    symbol = U'T';
                    color = {0.15f, 0.50f, 0.18f};
                    break;

                case TileType::Rock:
                    symbol = U'o';
                    color = {0.55f, 0.55f, 0.55f};
                    break;

                case TileType::Mountain:
                    symbol = U'^';
                    color = {0.75f, 0.75f, 0.75f};
                    break;

                case TileType::Wall:
                    symbol = U'#';
                    color = {0.5f, 0.5f, 0.5f};
                    break;
            }

            // Tile coordinates -> World coordinates

            const float worldX =
                x * CELL_WIDTH + 4.0f;

            const float worldY =
                (y + 1) * CELL_HEIGHT - 2.0f;

            // World coordinates -> Screen coordinates

            const float screenX =
                camera.worldToScreenX(worldX);

            const float screenY =
                camera.worldToScreenY(worldY);

            glyphRenderer.draw(
                symbol,
                screenX,
                screenY,
                scale,
                color
            );
        }
    }
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
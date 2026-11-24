#include "core/Application.hpp"
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
      world(50, 25),
      player(2, 2)
{
    renderer.setClearColor(
        0.05f, 0.08f, 0.06f
    );
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
}

void Application::render()
{
    int width, height;

    window.getFramebufferSize(width, height);

    if (width <= 0 || height <= 0)
    {
        return;
    }

    renderer.clear();

    glyphRenderer.setScreenSize(width, height);

    glyphRenderer.begin();

    renderWorld();
    renderPlayer();

    glyphRenderer.flush();
}

void Application::renderWorld()
{
    for (int y = 0; y < world.getHeight(); ++y)
    {
        for (int x = 0; x < world.getWidth(); ++x)
        {
            // Do not draw terrain under the player
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
                case TileType::Ground:
                    symbol = U'.';
                    color = {0.3f, 0.4f, 0.2f};
                    break;

                case TileType::Grass:
                    symbol = U',';
                    color = {0.3f, 0.7f, 0.25f};
                    break;

                case TileType::Tree:
                    symbol = U'T';
                    color = {0.2f, 0.65f, 0.2f};
                    break;

                case TileType::Water:
                    symbol = U'~';
                    color = {0.2f, 0.5f, 0.9f};
                    break;

                case TileType::Wall:
                    symbol = U'#';
                    color = {0.5f, 0.5f, 0.5f};
                    break;
            }

            glyphRenderer.draw(
                symbol,
                x * CELL_WIDTH + 4.0f,
                (y + 1) * CELL_HEIGHT - 2.0f,
                GLYPH_SCALE,
                color
            );
        }
    }
}

void Application::renderPlayer()
{
    glyphRenderer.draw(
        U'@',
        player.getX() * CELL_WIDTH + 4.0f,
        (player.getY() + 1) * CELL_HEIGHT - 2.0f,
        GLYPH_SCALE,
        {1.0f, 1.0f, 1.0f}
    );
}
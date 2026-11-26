#include "core/Application.hpp"

#include <cmath>
#include <string>
#include <algorithm>
#include <vector>

Application::Application()
    : window(1280, 720, "ASCII Engine"),
      renderer(),
      renderStats(),
      font(
          "assets/fonts/ttf/JetBrainsMono-Regular.ttf",
          64
      ),
      fontAtlas(font),
      glyphRenderer(fontAtlas),
      backgroundRenderer(),
      camera(),
      world(500, 500, 42),
      worldRenderer(
          glyphRenderer,
          world,
          GLYPH_SCALE
      ),
      occlusionMap(world),
      player(0.0f, 0.0f)
{
    const auto [spawnX, spawnY] =
        world.findSpawn();

    const auto& scale =
        world.getScale();

    // Convert spawn cell coordinates
    // into continuous world coordinates.

    player.setPosition(
        (spawnX + 0.5f) * scale.cellWidth,
        (spawnY + 0.5f) * scale.cellHeight
    );

    player.setMovementSpeed(
        scale.playerHeight * 4.0f
    );

    renderer.setClearColor(
        0.05f,
        0.08f,
        0.06f
    );

    camera.setZoom(1.0f);

    camera.setPosition(
        player.getX(),
        player.getY()
    );
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
                + renderStats.getTitleSuffix()
            );

            const double frameTime =
                elapsed * 1000.0 / frameCount;

            renderStats.print(
                fps,
                frameTime
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

    // Toggle torch.

    const bool lKey =
        window.isKeyPressed(GLFW_KEY_L);

    if (lKey && !previousLKey)
    {
        torchEnabled = !torchEnabled;
    }

    previousLKey = lKey;

    // Pause or resume day/night cycle.

    const bool pKey =
        window.isKeyPressed(GLFW_KEY_P);

    if (pKey && !previousPKey)
    {
        lighting.setCycleEnabled(
            !lighting.isCycleEnabled()
        );
    }

    previousPKey = pKey;

    // Time presets.

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

    // Camera zoom.

    const double scroll =
        window.consumeScroll();

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
}

void Application::update(float deltaTime)
{
    lighting.update(deltaTime);

    // ====================================
    // Camera rotation
    // ====================================

    const float orbit =
        (
            window.isKeyPressed(GLFW_KEY_RIGHT)
            - window.isKeyPressed(GLFW_KEY_LEFT)
        ) * deltaTime;

    const float tilt =
        (
            window.isKeyPressed(GLFW_KEY_UP)
            - window.isKeyPressed(GLFW_KEY_DOWN)
        ) * deltaTime;

    if (orbit != 0.0f || tilt != 0.0f)
    {
        camera.orbit(orbit, tilt);
    }

    // ====================================
    // Screen-relative movement input
    // ====================================

    float inputX = 0.0f;
    float inputY = 0.0f;

    if (window.isKeyPressed(GLFW_KEY_W))
    {
        inputY -= 1.0f;
    }

    if (window.isKeyPressed(GLFW_KEY_S))
    {
        inputY += 1.0f;
    }

    if (window.isKeyPressed(GLFW_KEY_A))
    {
        inputX -= 1.0f;
    }

    if (window.isKeyPressed(GLFW_KEY_D))
    {
        inputX += 1.0f;
    }

    // ====================================
    // Convert screen direction into
    // world-space direction
    // ====================================

    if (inputX != 0.0f || inputY != 0.0f)
    {
        const auto screenPosition =
            camera.project(
                player.getX(),
                player.getY(),
                0.0f
            );

        const auto targetPosition =
            camera.unproject(
                screenPosition.x + inputX,
                screenPosition.y + inputY,
                0.0f
            );

        const float worldDX =
            targetPosition.x - player.getX();

        const float worldDY =
            targetPosition.y - player.getY();

        player.move(
            worldDX,
            worldDY,
            deltaTime,
            world
        );
    }

    // ====================================
    // Camera follow
    // ====================================

    camera.setPosition(
        player.getX(),
        player.getY()
    );
}

void Application::render()
{
    int width, height;

    window.getFramebufferSize(
        width,
        height
    );

    if (width <= 0 || height <= 0)
    {
        return;
    }

    // Begin performance measurement

    renderStats.beginFrame();

    camera.setViewportSize(width, height);

    glyphRenderer.setScreenSize(
        width,
        height
    );

    glyphRenderer.setTime(
        static_cast<float>(glfwGetTime())
    );

    // Update lighting

    updateLighting();

    // Clear framebuffer

    renderer.clear();

    // Generate Light Map and render background

    backgroundRenderer.render(
        camera,
        occlusionMap,
        width,
        height,
        world.getScale().cellWidth,
        world.getScale().cellHeight
    );

    // Light Map fullscreen triangle

    renderStats.recordDrawCall(1);

    // Background fullscreen triangle

    renderStats.recordDrawCall(1);

    // Share Light Map

    glyphRenderer.setLightmap(
        backgroundRenderer.getLightmapTexture()
    );

    // Render world

    glyphRenderer.begin();

    worldRenderer.render(camera);

    renderPlayer();

    // Collect rendering statistics

    renderStats.setVisibleTiles(
        worldRenderer.getVisibleTileCount()
    );

    renderStats.recordGlyphs(
        glyphRenderer.getInstanceCount(),
        sizeof(GlyphInstance)
    );

    // Submit glyph instances to GPU

    glyphRenderer.flush();

    // Finish performance measurement

    renderStats.endFrame();
}

void Application::renderPlayer()
{
    const auto& scale =
        world.getScale();

    const float playerGlyphScale =
        GLYPH_SCALE
        * (
            scale.playerHeight
            / WorldScale{}.playerHeight
        );

    glyphRenderer.drawWorld(
        camera,

        U'@',

        player.getX(),
        player.getY(),

        scale.playerHeight * 0.5f,

        playerGlyphScale,

        {0.85f, 0.88f, 0.78f}
    );
}

void Application::updateLighting()
{
    const Color ambient =
        lighting.getAmbientColor();

    renderer.setClearColor(
        0.05f * ambient.r,
        0.08f * ambient.g,
        0.06f * ambient.b
    );

    std::vector<PointLight> lights;

    if (torchEnabled)
    {
        PointLight torch;

        const auto ground =
            camera.project(
                player.getX(),
                player.getY(),
                0.0f
            );

        torch.x = ground.x;
        torch.y = ground.y;

        torch.radius =
            220.0f * camera.getZoom();

        torch.sourceRadius =
            40.0f * camera.getZoom();

        const float time =
            static_cast<float>(
                glfwGetTime()
            );

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

    backgroundRenderer.setLighting(
        ambient,
        lights
    );
}

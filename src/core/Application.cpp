#include "renderer/CpuProfiler.hpp"
#include "core/Application.hpp"

#include <cmath>
#include <string>
#include <algorithm>
#include <iostream>
#include <array>

namespace
{
    constexpr std::array lightingViews{"Final","Albedo","Normals","Ambient","Directional diffuse",
        "Point light","Specular","Torch visibility","Roughness","Reflectivity","Legacy light map"};
    constexpr float torchRange = 220.0f;
    constexpr float torchSourceRadius = 40.0f;
    constexpr float torchIntensity = 1.8f;
    constexpr float torchHeight = 0.74f;
}

Application::Application(bool benchmark)
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
      world(benchmark ? 80 : 500, benchmark ? 80 : 500, 42,
            benchmark ? WorldScale{16, 16, 64, 0.19f, 1.10f}
                      : WorldScale{24, 28, 64, 0.30f, 1.10f}, benchmark),
      worldRenderer(
          glyphRenderer,
          world,
          GLYPH_SCALE,
          benchmark
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
        112.0f
    );

    renderer.setClearColor(
        0.05f,
        0.08f,
        0.06f
    );

    camera.setZoom(GAMEPLAY_ZOOM);
    camera.setAngles(GAMEPLAY_AZIMUTH, GAMEPLAY_ELEVATION);
    if (benchmark)
    {
        lighting.setHour(10.5f);
    }

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

        CpuProfiler::beginFrame();
        {
            CpuProfiler::Scope timer(CpuProfiler::Stage::Events);
            window.pollEvents();
        }
        {
            CpuProfiler::Scope timer(CpuProfiler::Stage::Input);
            processInput();
        }
        update(deltaTime);
        {
            CpuProfiler::Scope timer(CpuProfiler::Stage::Render);
            render();
        }
        {
            CpuProfiler::Scope timer(CpuProfiler::Stage::Swap);
            window.swapBuffers();
        }

        ++frameCount;

        const double currentTime = glfwGetTime();
        CpuProfiler::endFrame((currentTime - frameTime) * 1000.0);

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
                + (debugCamera ? " | Orbit" : "")
                + (lightingDebug == 0 ? std::string{} : std::string(" | Lighting: ") + lightingViews[lightingDebug])
                + renderStats.getTitleSuffix()
            );

            const double frameTime =
                elapsed * 1000.0 / frameCount;

            renderStats.print(
                fps,
                frameTime
            );
            glyphRenderer.printCacheStats(std::cout);

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
        camera.setZoom(GAMEPLAY_ZOOM);
    }
    const bool debugKey = window.isKeyPressed(GLFW_KEY_F3);
    if (debugKey && !previousDebugKey)
    {
        debugCamera = !debugCamera;
        if (!debugCamera)
        {
            camera.setAngles(GAMEPLAY_AZIMUTH, GAMEPLAY_ELEVATION);
        }
    }
    previousDebugKey = debugKey;
    const bool lightingKey = window.isKeyPressed(GLFW_KEY_F4);
    if (lightingKey && !previousLightingKey)
    {
        lightingDebug = (lightingDebug + 1) % static_cast<int>(lightingViews.size());
    }
    if (window.isKeyPressed(GLFW_KEY_F5))
    {
        lightingDebug = 0;
    }
    previousLightingKey = lightingKey;
}

void Application::update(float deltaTime)
{
    {
        CpuProfiler::Scope timer(CpuProfiler::Stage::WorldUpdate);
        lighting.update(deltaTime);
    }
    {
        CpuProfiler::Scope timer(CpuProfiler::Stage::CameraUpdate);
        const float orbit = (window.isKeyPressed(GLFW_KEY_RIGHT)
                           - window.isKeyPressed(GLFW_KEY_LEFT)) * deltaTime;
        const float tilt = (window.isKeyPressed(GLFW_KEY_UP)
                          - window.isKeyPressed(GLFW_KEY_DOWN)) * deltaTime;
        if (debugCamera && (orbit != 0.0f || tilt != 0.0f))
        {
            camera.orbit(orbit, tilt);
        }
    }
    {
        CpuProfiler::Scope timer(CpuProfiler::Stage::PlayerUpdate);
        const float inputX = window.isKeyPressed(GLFW_KEY_D) - window.isKeyPressed(GLFW_KEY_A);
        const float inputY = window.isKeyPressed(GLFW_KEY_S) - window.isKeyPressed(GLFW_KEY_W);
        const float oldX = player.getX();
        const float oldY = player.getY();
        if (inputX != 0.0f || inputY != 0.0f)
        {
            const auto screen = camera.project(oldX, oldY, 0.0f);
            const auto target = camera.unproject(screen.x + inputX, screen.y + inputY, 0.0f);
            player.move(target.x - oldX, target.y - oldY, deltaTime, world);
        }
        knight.update(player.getX() - oldX, player.getY() - oldY, deltaTime, world.getScale().playerHeight);
    }
    {
        CpuProfiler::Scope timer(CpuProfiler::Stage::CameraUpdate);
        const float follow = 1.0f - std::exp(-11.0f * deltaTime);
        camera.setPosition(std::lerp(camera.getX(), player.getX(), follow),
                           std::lerp(camera.getY(), player.getY(), follow));
    }
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
    glyphRenderer.setCamera(camera);

    glyphRenderer.setScreenSize(
        width,
        height
    );

    glyphRenderer.setTime(
        static_cast<float>(glfwGetTime())
    );

    // World-space wind: direction (x,y), strength in [0,1].
    // A future WeatherSystem can supply these three values.
    glyphRenderer.setGrassEnvironment(player.getX(), player.getY(),
        world.getScale().playerHeight, static_cast<float>(glfwGetTime()),
        1.0f, 0.35f, 0.30f);

    const auto& scale = world.getScale();

    const int waterTileX =
        static_cast<int>(
            std::floor(
                player.getX() / scale.cellWidth
            )
        );

    const int waterTileY =
        static_cast<int>(
            std::floor(
                player.getY() / scale.cellHeight
            )
        );

    const bool inWater =
        world.isInside(waterTileX, waterTileY)
        &&
        world.getTile(
            waterTileX,
            waterTileY
        ).type == TileType::Water;

    glyphRenderer.setWaterEnvironment(
        player.getX(),
        player.getY(),
        static_cast<float>(glfwGetTime()),
        inWater
    );

    // Update lighting

    updateLighting();

    // Clear framebuffer

    renderer.clear();

    // Generate Light Map and render background

    {
    CpuProfiler::Scope timer(CpuProfiler::Stage::LightMap);
    backgroundRenderer.render(
        camera,
        occlusionMap,
        width,
        height,
        world.getScale().cellWidth,
        world.getScale().cellHeight
    );

    }

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

    {
        CpuProfiler::Scope timer(CpuProfiler::Stage::Knight);
        renderPlayer();
    }

    // Collect rendering statistics

    renderStats.setVisibleTiles(
        worldRenderer.getVisibleTileCount()
    );

    // Dynamic geometry statistics.

    renderStats.recordGlyphs(
        glyphRenderer.getInstanceCount(),
        sizeof(GlyphInstance),
        glyphRenderer.getGroundInstanceCount()
    );

    // Static GPU geometry statistics.

    renderStats.recordCachedGround(
        glyphRenderer.getCachedGroundCount(),
        glyphRenderer.getCachedDrawCalls(),
        glyphRenderer.getCachedUploadBytes()
    );

    // Submit all geometry.

    glyphRenderer.flush();

    // Finish performance measurement

    renderStats.endFrame();
}

void Application::renderPlayer()
{
    knight.render(camera, glyphRenderer, player.getX(), player.getY(),
                  world.getScale().playerHeight, lighting.getHour());
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

    std::array<PointLight,1> lights;

    if (torchEnabled)
    {
        auto& torch = lights.front();
        torch.x = player.getX();
        torch.y = player.getY();
        torch.z = world.getScale().playerHeight * torchHeight;
        torch.radius = torchRange;
        torch.sourceRadius = torchSourceRadius;

        const float time =
            static_cast<float>(
                glfwGetTime()
            );

        const float flicker =
            1.0f
            + std::sin(time * 7.0f) * 0.04f
            + std::sin(time * 13.0f) * 0.025f;

        torch.intensity =
            torchIntensity * flicker;

        torch.color = {
            1.0f,
            0.72f,
            0.35f
        };

    }

    const auto activeLights = std::span<const PointLight>(lights).first(torchEnabled ? 1 : 0);
    backgroundRenderer.setLighting(ambient,activeLights,camera);
    glyphRenderer.setSurfaceLighting(lighting.getSurfaceLighting(), activeLights, lightingDebug);
}

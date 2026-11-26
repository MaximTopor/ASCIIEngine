#pragma once

#include "core/Window.hpp"

#include "renderer/BackgroundRenderer.hpp"
#include "renderer/Renderer.hpp"
#include "renderer/Font.hpp"
#include "renderer/FontAtlas.hpp"
#include "renderer/GlyphRenderer.hpp"
#include "renderer/Camera.hpp"
#include "renderer/WorldRenderer.hpp"
#include "renderer/Lighting.hpp"
#include "renderer/OcclusionMap.hpp"
#include "renderer/RenderStats.hpp"

#include "world/World.hpp"
#include "entity/Player.hpp"

class Application
{
private:

    static constexpr float GLYPH_SCALE = 0.5f;

    Lighting lighting;

    bool torchEnabled = true;

    bool previousLKey = false;
    bool previousPKey = false;

    Window window;
    Renderer renderer;
    RenderStats renderStats;

    Font font;
    FontAtlas fontAtlas;

    GlyphRenderer glyphRenderer;
    BackgroundRenderer backgroundRenderer;

    Camera camera;

    World world;
    WorldRenderer worldRenderer;
    OcclusionMap occlusionMap;

    Player player;

    void processInput();

    void update(float deltaTime);

    void render();

    void renderPlayer();

    void updateLighting();

public:

    Application();

    void run();
};
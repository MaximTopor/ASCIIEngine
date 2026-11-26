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
#include "renderer/KnightRenderer.hpp"

#include "world/World.hpp"
#include "entity/Player.hpp"

class Application
{
private:

    static constexpr float GLYPH_SCALE = 0.5f;
    static constexpr float GAMEPLAY_ZOOM = 1.45f;
    static constexpr float GAMEPLAY_AZIMUTH = 0.37f;
    static constexpr float GAMEPLAY_ELEVATION = 0.73f;

    Lighting lighting;

    bool torchEnabled = true;

    bool previousLKey = false;
    bool previousPKey = false;
    bool previousDebugKey = false;
    bool debugCamera = false;
    bool previousLightingKey = false;
    int lightingDebug = 0;

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
    KnightRenderer knight;

    void processInput();

    void update(float deltaTime);

    void render();

    void renderPlayer();

    void updateLighting();

public:

    explicit Application(bool benchmark = true);

    void run();
};

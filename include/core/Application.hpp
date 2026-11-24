#pragma once

#include "core/Window.hpp"

#include "renderer/Renderer.hpp"
#include "renderer/Font.hpp"
#include "renderer/FontAtlas.hpp"
#include "renderer/GlyphRenderer.hpp"

#include "world/World.hpp"
#include "entity/Player.hpp"

class Application
{
private:
    Window window;
    Renderer renderer;

    Font font;
    FontAtlas fontAtlas;
    GlyphRenderer glyphRenderer;

    World world;
    Player player;

    double lastMoveTime = 0.0;

    static constexpr float CELL_WIDTH = 24.0f;
    static constexpr float CELL_HEIGHT = 28.0f;
    static constexpr float GLYPH_SCALE = 0.5f;

    static constexpr double MOVE_INTERVAL = 0.12;

    void processInput();
    void update();
    void render();

    void renderWorld();
    void renderPlayer();

public:
    Application();

    void run();
};
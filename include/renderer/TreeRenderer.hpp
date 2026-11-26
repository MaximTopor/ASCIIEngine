#pragma once

#include "renderer/VisualGlyph.hpp"
#include "world/World.hpp"

#include <vector>

class TreeRenderer
{
    std::vector<int> levels;

public:
    explicit TreeRenderer(std::size_t treeCount) : levels(treeCount, -1) {}
    void render(std::size_t id, const Tree& tree, const Camera& camera, GlyphRenderer& renderer);
    static std::vector<VisualGlyph> generate(const Tree& tree, int level);
};

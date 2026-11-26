#pragma once

#include "renderer/VisualGlyph.hpp"
#include "renderer/Camera.hpp"
#include "world/Rock.hpp"

#include <list>
#include <map>
#include <utility>
#include <vector>

class RockRenderer
{
    using Key = std::pair<std::size_t, int>;

    struct CachedSurface
    {
        std::vector<VisualGlyph> glyphs;
        std::list<Key>::iterator recency;
    };

    static constexpr std::size_t MAX_CACHED_SURFACES = 256;
    std::list<Key> recentSurfaces;
    std::map<Key, CachedSurface> cache;

    static std::vector<VisualGlyph> generate(const Rock& rock, float spacing);

public:
    void render(const Rock& rock, const Camera& camera, GlyphRenderer& renderer);
};

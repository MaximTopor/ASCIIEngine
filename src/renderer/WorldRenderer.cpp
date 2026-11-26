#include "renderer/WorldRenderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace
{
    float randomValue(int x, int y, int salt)
    {
        auto n = static_cast<std::uint32_t>(x) * 374761393u
            + static_cast<std::uint32_t>(y) * 668265263u
            + static_cast<std::uint32_t>(salt) * 2246822519u;
        n = (n ^ (n >> 13u)) * 1274126177u;
        return static_cast<float>((n ^ (n >> 16u)) & 0xFFFFu) / 65536.0f;
    }

    struct GroundMaterial
    {
        Color color;
        float water;
    };

    GroundMaterial material(TileType type)
    {
        switch (type)
        {
            case TileType::DeepWater: return {{0.12f, 0.24f, 0.39f}, 1.0f};
            case TileType::Water: return {{0.20f, 0.39f, 0.51f}, 1.0f};
            case TileType::Sand: return {{0.47f, 0.39f, 0.25f}, 0.0f};
            case TileType::Tree:
            case TileType::Grass: return {{0.28f, 0.32f, 0.18f}, 0.0f};
            case TileType::Rock:
            case TileType::Mountain: return {{0.33f, 0.34f, 0.31f}, 0.0f};
            default: return {{0.34f, 0.28f, 0.19f}, 0.0f};
        }
    }

    GroundMaterial blend(GroundMaterial a, GroundMaterial b, float t)
    {
        return {{std::lerp(a.color.r, b.color.r, t), std::lerp(a.color.g, b.color.g, t),
                 std::lerp(a.color.b, b.color.b, t)}, std::lerp(a.water, b.water, t)};
    }

    std::vector<VisualGlyph> growTree(const Tree& tree)
    {
        std::vector<VisualGlyph> glyphs;
        std::uint32_t state = tree.seed;
        const auto random = [&state]()
        {
            state = state * 1664525u + 1013904223u;
            return static_cast<float>(state >> 8u) / 16777216.0f;
        };
        const float h = tree.height;
        const float leanX = (random() - 0.5f) * h * 0.13f;
        const float leanY = (random() - 0.5f) * h * 0.13f;
        const float orientation = random() * 2.0f * std::numbers::pi_v<float>;
        const auto bark = [&](float x, float y, float z, float em, float angle)
        {
            VisualGlyph glyph;
            glyph.character = random() < 0.35f ? U'/' : U'|';
            glyph.x = tree.x + x;
            glyph.y = tree.y + y;
            glyph.z = z;
            glyph.scale = em * h / 64.0f;
            const float grain = 0.78f + random() * 0.36f;
            glyph.color = {0.46f * grain, 0.33f * grain, 0.22f * grain};
            glyph.transform.rotation = angle;
            glyph.transform.pivotY = 0.5f;
            glyphs.push_back(glyph);
        };

        for (int i = 0; i < 13; ++i)
        {
            const float t = i / 12.0f;
            bark(leanX * t, leanY * t, t * h * 0.81f, 0.13f - 0.055f * t, 0.0f);
            if (i < 6)
            {
                bark(leanX * t + h * 0.019f, leanY * t, t * h * 0.81f, 0.09f, 0.10f);
                bark(leanX * t - h * 0.019f, leanY * t, t * h * 0.81f, 0.09f, -0.12f);
            }
        }

        for (int root = 0; root < 5; ++root)
        {
            const float angle = orientation + root * 1.256637f;
            const float length = h * (0.09f + 0.05f * random());
            for (int i = 1; i <= 4; ++i)
            {
                const float t = i * 0.25f;
                bark(std::cos(angle) * length * t, std::sin(angle) * length * t,
                     h * 0.055f * (1.0f - t), 0.08f * (1.0f - 0.5f * t), angle);
            }
        }

        constexpr std::array leaves{U'*', U'+', U';', U'x', U'&', U'o'};
        for (int branch = 0; branch < 11; ++branch)
        {
            const float tier = branch / 10.0f;
            const float angle = orientation + branch * 2.399963f;
            const float reach = h * (0.29f - tier * 0.24f) * (0.75f + 0.25f * random());
            const float dx = std::cos(angle) * reach;
            const float dy = std::sin(angle) * reach;
            const float branchZ = h * (0.43f + tier * 0.44f);
            const float baseX = leanX * (0.5f + tier * 0.5f);
            const float baseY = leanY * (0.5f + tier * 0.5f);
            for (int i = 1; i <= 5; ++i)
            {
                const float t = i * 0.2f;
                bark(baseX + t * dx, baseY + t * dy, branchZ + h * 0.1f * t,
                     0.085f - 0.03f * t, angle);
            }
            for (int i = 0; i < 18; ++i)
            {
                const float a = random() * 2.0f * std::numbers::pi_v<float>;
                const float r = std::sqrt(random()) * h * (0.16f - 0.055f * tier);
                VisualGlyph leaf;
                leaf.x = tree.x + baseX + dx + std::cos(a) * r;
                leaf.y = tree.y + baseY + dy + std::sin(a) * r;
                leaf.z = branchZ + h * (0.035f + 0.16f * random());
                leaf.character = leaves[static_cast<std::size_t>(random() * leaves.size())];
                leaf.scale = h * (0.078f + 0.05f * random()) / 64.0f;
                const float light = 0.57f + 0.32f * tier + 0.24f * random();
                leaf.color = {0.30f * light, 0.51f * light, 0.27f * light};
                leaf.transform.rotation = (random() - 0.5f) * 1.4f;
                leaf.transform.pivotY = 0.5f;
                leaf.transform.opacity = 0.82f + 0.18f * random();
                leaf.animation = {AnimationType::Wind, angle + orientation, 0.06f + 0.13f * tier};
                glyphs.push_back(leaf);
            }
        }
        return glyphs;
    }

    void contactShadow(const Camera& camera, GlyphRenderer& renderer, float x, float y,
                       float radiusX, float radiusY, float angle)
    {
        const float c = std::cos(angle);
        const float s = std::sin(angle);
        for (int i = 0; i < 14; ++i)
        {
            const float t = i * 2.399963f;
            const float r = 0.50f + 0.45f * std::sqrt(i / 14.0f);
            const float dx = std::cos(t) * radiusX * r;
            const float dy = std::sin(t) * radiusY * r;
            GlyphTransform transform;
            transform.opacity = 0.48f;
            transform.rotation = angle + t;
            transform.pivotY = 0.5f;
            renderer.drawWorld(camera, U'=', x + c * dx - s * dy, y + s * dx + c * dy, 0.0f,
                               radiusX / 115.0f, {0.035f, 0.045f, 0.033f},
                               {}, transform, GlyphAnchor::Center, true);
        }
    }
}

WorldRenderer::WorldRenderer(GlyphRenderer& glyphRenderer, const World& world, float glyphScale)
    : glyphRenderer(glyphRenderer), world(world),
      cellWidth(world.getScale().cellWidth), cellHeight(world.getScale().cellHeight),
      glyphScale(glyphScale), treeGlyphs(world.getTrees().size())
{
}

void WorldRenderer::renderGround(const Camera& camera, int x, int y)
{
    const int nx = std::min(x + 1, world.getWidth() - 1);
    const int ny = std::min(y + 1, world.getHeight() - 1);
    const auto a = material(world.getTile(x, y).type);
    const auto b = material(world.getTile(nx, y).type);
    const auto c = material(world.getTile(x, ny).type);
    const auto d = material(world.getTile(nx, ny).type);
    const int samples = camera.getZoom() < 0.65f ? 1 : 3;
    constexpr std::array ground{U'.', U',', U'\'', U'`', U':'};
    for (int i = 0; i < samples; ++i)
    {
        const float u = randomValue(x, y, 4 * i + 1);
        const float v = randomValue(x, y, 4 * i + 2);
        const float grain = randomValue(x, y, 4 * i + 3);
        const float px = (x + u) * cellWidth;
        const float py = (y + v) * cellHeight;
        const auto surface = blend(blend(a, b, u), blend(c, d, u), v);
        const bool water = surface.water > 0.55f;
        const float brightness = 0.64f + 0.46f * grain;
        const Color color{surface.color.r * brightness, surface.color.g * brightness,
                          surface.color.b * brightness};
        GlyphTransform transform;
        transform.rotation = water ? 0.0f : (grain - 0.5f) * 1.0f;
        transform.pivotY = 0.5f;
        const GlyphAnimation wave = water
            ? GlyphAnimation{AnimationType::Water, px * 0.012f + py * 0.009f, 0.14f}
            : GlyphAnimation{};
        glyphRenderer.drawWorld(camera, water ? U'~' : ground[static_cast<std::size_t>(grain * ground.size())],
                                px, py, 0.0f, water ? 0.16f + grain * 0.08f : 0.11f + grain * 0.13f,
                                color, wave, transform, GlyphAnchor::Center, true);
    }
}

void WorldRenderer::render(const Camera& camera)
{
    visibleTileCount = 0;
    const auto ground = camera.visibleGround();
    const auto elevated = camera.visibleGround(world.getScale().playerHeight * 7.0f);
    const int startX = std::max(0, static_cast<int>(std::floor(ground.left / cellWidth)) - 2);
    const int endX = std::min(world.getWidth(), static_cast<int>(std::ceil(ground.right / cellWidth)) + 2);
    const int startY = std::max(0, static_cast<int>(std::floor(ground.top / cellHeight)) - 2);
    const int endY = std::min(world.getHeight(), static_cast<int>(std::ceil(ground.bottom / cellHeight)) + 2);
    for (int y = startY; y < endY; ++y)
    {
        for (int x = startX; x < endX; ++x)
        {
            ++visibleTileCount;
            renderGround(camera, x, y);
            const auto type = world.getTile(x, y).type;
            if (type == TileType::Mountain || type == TileType::Wall)
            {
                glyphRenderer.drawWorld(camera, type == TileType::Mountain ? U'^' : U'#',
                    (x + 0.5f) * cellWidth, (y + 0.5f) * cellHeight, 10.0f,
                    glyphScale, {0.59f, 0.59f, 0.55f});
            }
        }
    }

    vegetationCache.render(world, camera, glyphRenderer, cellWidth, cellHeight);

    const auto visible = [&elevated](float x, float y, float padding)
    {
        return x >= elevated.left - padding && x <= elevated.right + padding
            && y >= elevated.top - padding && y <= elevated.bottom + padding;
    };

    for (const Rock& rock : world.getRocks())
    {
        if (visible(rock.x, rock.y, rock.radiusX * 1.5f))
        {
            contactShadow(camera, glyphRenderer, rock.x, rock.y, rock.radiusX * 1.1f,
                          rock.radiusY * 1.1f, rock.angle);
            rockRenderer.render(rock, camera, glyphRenderer);
        }
    }
    const auto& trees = world.getTrees();
    for (std::size_t i = 0; i < trees.size(); ++i)
    {
        const auto& tree = trees[i];
        if (!visible(tree.x, tree.y, tree.height * 0.60f))
        {
            continue;
        }
        contactShadow(camera, glyphRenderer, tree.x, tree.y, tree.height * 0.17f, tree.height * 0.12f, 0.0f);
        auto& glyphs = treeGlyphs[i];
        if (glyphs.empty())
        {
            glyphs = growTree(tree);
        }
        for (const auto& glyph : glyphs)
        {
            glyphRenderer.drawWorld(camera, glyph.character, glyph.x, glyph.y, glyph.z, glyph.scale,
                                    glyph.color, glyph.animation, glyph.transform);
        }
    }
}

std::size_t WorldRenderer::getVisibleTileCount() const
{
    return visibleTileCount;
}

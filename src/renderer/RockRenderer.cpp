#include "renderer/RockRenderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace
{
    float random(std::uint32_t& state)
    {
        state = state * 1664525u + 1013904223u;
        return static_cast<float>(state >> 8) / 16777216.0f;
    }
}

std::vector<VisualGlyph> RockRenderer::generate(const Rock& rock, float spacing)
{
    constexpr float tau = 2.0f * std::numbers::pi_v<float>;
    constexpr float epsilon = 0.02f;
    constexpr std::array stone{U'/', U'\\', U'=', U'o', U'x', U';', U'+', U'~'};
    const float c = std::cos(rock.angle);
    const float s = std::sin(rock.angle);
    const int rings = static_cast<int>(std::clamp(std::ceil(rock.radiusX / spacing), 3.0f, 32.0f));
    const float glyphScale = (rock.radiusX / rings) / 36.0f;

    auto state = rock.seed;
    const float phase = random(state) * tau;
    const float tone = 0.84f + 0.25f * random(state);
    const auto height = [&rock, phase](float u, float v)
    {
        const float slab = std::min(1.0f - 0.26f * std::abs(u + 0.45f * v),
                                    0.97f - 0.20f * std::abs(v - 0.6f * u));
        const float ridges = 0.05f * std::abs(std::sin(7.0f * u + 4.0f * v + phase));
        return std::sqrt(std::max(0.0f, rock.surface(u, v))) * (slab - ridges);
    };

    std::vector<VisualGlyph> output;
    output.reserve(4 * rings * rings);
    for (int ring = 0; ring <= rings; ++ring)
    {
        const float radius = static_cast<float>(ring) / rings;
        const int samples = ring == 0 ? 1 : 6 * ring;
        for (int i = 0; i < samples; ++i)
        {
            const float angle = phase + tau * (i + 0.5f * (ring % 2)
                + (random(state) - 0.5f) * 0.5f) / samples;
            const float dx = std::cos(angle);
            const float dy = std::sin(angle);

            // Find the same outer contour at every detail level.
            float inner = 0.0f;
            float outer = 1.0f;
            for (int step = 0; step < 8; ++step)
            {
                const float middle = (inner + outer) * 0.5f;
                if (rock.surface(dx * middle, dy * middle) > 0)
                {
                    inner = middle;
                }
                else
                {
                    outer = middle;
                }
            }
            const float sampleRadius = inner * (0.97f * radius
                + (random(state) - 0.5f) * 0.10f / rings);
            const float u = dx * sampleRadius;
            const float v = dy * sampleRadius;
            const float sx = u * rock.radiusX;
            const float sy = v * rock.radiusY;
            const float slopeX = (height(u - epsilon, v) - height(u + epsilon, v))
                * rock.height / (2.0f * epsilon * rock.radiusX);
            const float slopeY = (height(u, v - epsilon) - height(u, v + epsilon))
                * rock.height / (2.0f * epsilon * rock.radiusY);
            const float nx = c * slopeX - s * slopeY;
            const float ny = s * slopeX + c * slopeY;
            const float diffuse = std::max(0.0f, (-0.48f * nx - 0.57f * ny + 0.667f)
                / std::sqrt(1.0f + nx * nx + ny * ny));

            const float strata = std::sin(18.0f * v + 7.0f * u
                + 1.0f * std::sin(8.0f * u + phase) + phase);
            const bool crack = strata > 0.95f;
            const float brightness = tone * (0.36f + 0.72f * diffuse)
                * (0.84f + 0.27f * random(state)) * (crack ? 0.49f : 1.0f);
            const float moss = std::clamp(
                (std::sin(6.0f * u - 3.0f * v + phase)
                 + std::cos(9.0f * v + phase) - 0.85f) * 0.6f, 0.0f, 1.0f)
                * (1.0f - diffuse);

            VisualGlyph glyph;
            const auto index = static_cast<std::size_t>(random(state) * stone.size());
            glyph.character = crack ? U':' : stone[index];
            glyph.x = rock.x + sx * c - sy * s;
            glyph.y = rock.y + sx * s + sy * c;
            glyph.z = rock.height * height(u, v);
            glyph.scale = glyphScale * (0.88f + 0.24f * random(state));
            glyph.color = {
                std::lerp(0.69f, 0.33f, moss) * brightness,
                std::lerp(0.72f, 0.57f, moss) * brightness,
                std::lerp(0.73f, 0.29f, moss) * brightness
            };
            glyph.transform.rotation = -0.20f + (random(state) - 0.5f) * 0.20f;
            glyph.transform.pivotY = 0.5f;
            output.push_back(glyph);
        }
    }
    return output;
}

void RockRenderer::render(const Rock& rock, const Camera& camera, GlyphRenderer& renderer)
{
    const float zoom = camera.getZoom();
    const float projectedDiameter = 2.0f * rock.radiusX * zoom;
    const int level = projectedDiameter < 48.0f ? 2 : (projectedDiameter < 96.0f ? 1 : 0);
    constexpr std::array spacing{6.0f, 9.0f, 15.0f};
    const Key key{rock.id, level};
    auto entry = cache.find(key);
    if (entry == cache.end())
    {
        auto glyphs = generate(rock, spacing[level]);
        if (cache.size() == MAX_CACHED_SURFACES)
        {
            cache.erase(recentSurfaces.back());
            recentSurfaces.pop_back();
        }
        recentSurfaces.push_front(key);
        entry = cache.emplace(key, CachedSurface{std::move(glyphs), recentSurfaces.begin()}).first;
    }
    else
    {
        recentSurfaces.splice(recentSurfaces.begin(), recentSurfaces, entry->second.recency);
    }

    for (const auto& glyph : entry->second.glyphs)
    {
        renderer.drawWorld(camera, glyph.character, glyph.x, glyph.y, glyph.z,
                           glyph.scale, glyph.color, glyph.animation, glyph.transform);
    }
}

#pragma once

#include <cstdint>
#include <functional>
#include <span>

namespace SurfaceSampler
{
    struct Vec
    {
        float x{}, y{}, z{};
        Vec operator+(Vec b) const { return {x+b.x, y+b.y, z+b.z}; }
        Vec operator-(Vec b) const { return {x-b.x, y-b.y, z-b.z}; }
        Vec operator*(float s) const { return {x*s, y*s, z*s}; }
    };

    float dot(Vec a, Vec b);
    Vec cross(Vec a, Vec b);
    float length(Vec v);
    Vec unit(Vec v);

    struct Settings
    {
        float spacing = 4.0f;
        float jitter = 0.20f; // Fraction of the local spacing.
        std::uint32_t seed = 0;
    };

    struct Sample
    {
        Vec position, normal, tangent;
        float spacing, u, v, variation;
    };

    struct TubePoint { Vec position; float radius; };
    using Sink = std::function<void(const Sample&)>;
    using Mapping = std::function<Vec(float u, float v)>;
    using RadiusProfile = std::function<float(float turns, float along)>;

    void cylinder(Vec a, Vec b, float radius, Settings settings, const Sink& emit);
    void frustum(Vec a, Vec b, float startRadius, float endRadius, Settings settings, const Sink& emit);
    void tube(std::span<const TubePoint> points, Settings settings, const Sink& emit,
              const RadiusProfile& profile = {});
    void sphere(Vec center, float radius, Settings settings, const Sink& emit);
    void ellipsoid(Vec center, Vec radii, Settings settings, const Sink& emit,
                   const RadiusProfile& profile = {});
    void capsule(Vec a, Vec b, float radius, Settings settings, const Sink& emit);
    void plane(Vec origin, Vec uEdge, Vec vEdge, Settings settings, const Sink& emit);
    // The dimensions describe the approximate physical lengths of the parameter axes.
    void parametric(const Mapping& mapping, float uLength, float vLength,
                    Settings settings, const Sink& emit);
}

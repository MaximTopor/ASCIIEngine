#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

struct Rock
{
    std::size_t id;
    float x;
    float y;
    float radiusX;
    float radiusY;
    float height;
    float angle;
    std::uint32_t seed;

    // Shared by collision and rendering. The contour stays inside the unit ellipse.
    float surface(float u, float v) const
    {
        const float phase = static_cast<float>(seed & 1023u) * 0.03125f;
        const float boundary = 0.9f
            + 0.06f * std::sin(5.0f * u + 4.0f * v + phase)
            + 0.04f * std::cos(7.0f * v - 3.0f * u + phase);
        return boundary - u * u - v * v;
    }
};

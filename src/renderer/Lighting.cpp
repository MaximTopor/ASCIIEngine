#include "renderer/Lighting.hpp"

#include <cmath>
#include <algorithm>

namespace
{
    constexpr float PI = 3.14159265359f;

    float smoothstep(float edge0, float edge1, float x)
    {
        const float t = std::clamp(
            (x - edge0) / (edge1 - edge0),
            0.0f,
            1.0f
        );

        return t * t * (3.0f - 2.0f * t);
    }

    float lerp(float a, float b, float t)
    {
        return a + (b - a) * t;
    }
}

void Lighting::update(float deltaTime)
{
    if (!cycleEnabled)
    {
        return;
    }

    hour += deltaTime * 24.0f / DAY_DURATION;

    setHour(hour);
}

void Lighting::setHour(float value)
{
    hour = std::fmod(value, 24.0f);

    if (hour < 0.0f)
    {
        hour += 24.0f;
    }
}

float Lighting::getHour() const
{
    return hour;
}

void Lighting::setCycleEnabled(bool enabled)
{
    cycleEnabled = enabled;
}

bool Lighting::isCycleEnabled() const
{
    return cycleEnabled;
}

Color Lighting::getAmbientColor() const
{
    const float sunHeight = std::sin(
        (hour - 6.0f) / 24.0f * 2.0f * PI
    );

    const float daylight = smoothstep(
        -0.15f,
        0.35f,
        sunHeight
    );

    const float twilight =
        4.0f * daylight * (1.0f - daylight);

    const Color night{
        0.10f,
        0.16f,
        0.32f
    };

    const Color day{
        1.0f,
        1.0f,
        0.95f
    };

    return {
        lerp(night.r, day.r, daylight)
            + twilight * 0.18f,

        lerp(night.g, day.g, daylight)
            + twilight * 0.04f,

        lerp(night.b, day.b, daylight)
            - twilight * 0.08f
    };
}
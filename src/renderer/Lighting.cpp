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

SurfaceLighting Lighting::getSurfaceLighting() const
{
    constexpr float orbitAzimuth = -0.65f;
    constexpr float solarPeak = 0.94f;
    constexpr float lunarPeak = 0.73f;
    const float phase = (hour - 6.0f) * PI / 12.0f;
    const float solarHeight = std::sin(phase);
    const float sunStrength = smoothstep(-0.035f,0.42f,solarHeight) * 0.91f;
    const float moonStrength = smoothstep(0.03f,0.65f,-solarHeight) * 0.27f;
    const float warmth = smoothstep(0.04f,0.70f,solarHeight);
    const auto direction = [=](float angle, float peak, float azimuth, Color radiance)
    {
        const float x = -std::cos(angle);
        const float y = 0.34f * std::sin(angle);
        const float z = peak * std::sin(angle);
        const float inv = 1.0f / std::sqrt(x*x+y*y+z*z);
        const float c = std::cos(azimuth), s = std::sin(azimuth);
        return DirectionalLight{(c*x-s*y)*inv, (s*x+c*y)*inv, z*inv, radiance};
    };
    const float day = smoothstep(-0.17f,0.38f,solarHeight);
    return {
        {lerp(0.105f,0.64f,day),lerp(0.14f,0.66f,day),lerp(0.22f,0.65f,day)},
        {lerp(0.065f,0.38f,day),lerp(0.075f,0.35f,day),lerp(0.105f,0.31f,day)},
        direction(phase,solarPeak,orbitAzimuth,
                  {sunStrength,sunStrength*lerp(0.58f,0.94f,warmth),sunStrength*lerp(0.34f,0.83f,warmth)}),
        direction(phase+PI,lunarPeak,orbitAzimuth+1.22f,
                  {moonStrength*0.54f,moonStrength*0.68f,moonStrength})
    };
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

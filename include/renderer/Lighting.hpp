#pragma once

struct Color
{
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
};

struct PointLight
{
    float x = 0.0f;
    float y = 0.0f;
    float sourceRadius = 12.0f;
    float radius = 180.0f;
    float intensity = 1.5f;

    Color color = {1.0f, 0.75f, 0.4f};
};

class Lighting
{
private:
    float hour = 22.0f;

    bool cycleEnabled = true;

    static constexpr float DAY_DURATION = 180.0f;

public:
    void update(float deltaTime);

    void setHour(float value);

    float getHour() const;

    void setCycleEnabled(bool enabled);

    bool isCycleEnabled() const;

    Color getAmbientColor() const;
};
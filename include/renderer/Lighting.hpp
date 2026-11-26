#pragma once

#include <span>

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
    float z = 0.0f;
    float sourceRadius = 12.0f;
    float radius = 180.0f;
    float intensity = 1.5f;

    Color color = {1.0f, 0.75f, 0.4f};
};

struct DirectionalLight
{
    float x, y, z;
    Color radiance;
};

struct SurfaceLighting
{
    Color sky, ground;
    DirectionalLight sun, moon;
};

enum class LightingMode { Legacy, Surface, Foliage };

struct GlyphMaterial
{
    float nx = 0, ny = 0, nz = 1;
    float roughness = 0.90f;
    float reflectivity = 0.04f;
    float ambientOcclusion = 1.0f;
    LightingMode mode = LightingMode::Legacy;
};

namespace MaterialPresets
{
    enum class Type { Bark, Branch, Root, Leaf, Stone, Moss, Grass, Soil, Sand, Wood, Steel, Iron, Leather, Cloth };
    constexpr GlyphMaterial surface(Type type, float nx = 0, float ny = 0, float nz = 1)
    {
        constexpr float roughness[]{0.90f,0.88f,0.94f,0.65f,0.90f,0.98f,0.85f,0.98f,0.95f,0.83f,0.25f,0.45f,0.75f,0.95f};
        constexpr float reflection[]{0.03f,0.04f,0.02f,0.06f,0.04f,0.01f,0.03f,0.01f,0.02f,0.04f,0.80f,0.55f,0.05f,0.02f};
        const auto index = static_cast<unsigned>(type);
        const auto mode = type == Type::Leaf || type == Type::Grass ? LightingMode::Foliage : LightingMode::Surface;
        return {nx, ny, nz, roughness[index], reflection[index], 1.0f, mode};
    }
}

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
    SurfaceLighting getSurfaceLighting() const;
};

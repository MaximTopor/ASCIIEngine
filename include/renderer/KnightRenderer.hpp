#pragma once

#include "renderer/GlyphRenderer.hpp"

#include <vector>

class KnightRenderer
{
    struct Vec
    {
        float x = 0, y = 0, z = 0;

        Vec operator+(Vec b) const { return {x + b.x, y + b.y, z + b.z}; }
        Vec operator*(float s) const { return {x * s, y * s, z * s}; }
    };

    enum class Material { Steel, Recess, Leather, Cloth, Edge, Wood, Count };
    enum PartId
    {
        Body, Hips, Chest, Neck, Head, ShoulderL, UpperArmL, ForearmL, HandL,
        ShoulderR, UpperArmR, ForearmR, HandR, ThighL, ShinL, FootL,
        ThighR, ShinR, FootR, Shield, Sword, PartCount
    };

    struct Sample
    {
        Vec position;
        Vec normal;
        Material material;
        unsigned pattern;
        float size;
    };

    struct Part
    {
        int parent = -1;
        Vec offset;
        std::vector<Sample> surface;
    };

    std::vector<Part> parts;
    float heading = 0.0f;
    float stride = 0.0f;
    float walkWeight = 0.0f;
    float idleTime = 0.0f;

    void ellipsoid(PartId part, Vec center, Vec radius, Material material, float tilt = 0.0f);
    static Vec pitch(Vec v, float angle);
    static Vec roll(Vec v, float angle);

public:
    KnightRenderer();
    void update(float dx, float dy, float dt, float height);
    void render(const Camera& camera, GlyphRenderer& renderer, float x, float y,
                float height, float hour) const;
};

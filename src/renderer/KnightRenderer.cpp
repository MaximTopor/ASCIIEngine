#include "renderer/KnightRenderer.hpp"
#include "renderer/SurfaceSampler.hpp"
#include "renderer/SurfaceMaterial.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <cstdint>

namespace
{
    constexpr float PI = std::numbers::pi_v<float>;
    constexpr float TAU = 2.0f * PI;

    // ========================================
    // KNIGHT GEOMETRY SETTINGS
    // ========================================

    constexpr float SAMPLE_SPACING = 0.018f;

    constexpr float STEEL_COVERAGE = 2.90f;
    constexpr float CLOTH_COVERAGE = 2.60f;
    constexpr float DETAIL_COVERAGE = 2.35f;

    constexpr float SURFACE_JITTER = 0.045f;

    // ========================================
    // MATERIAL CONFIGURATION
    // ========================================

    SurfaceMaterial makeMaterial(
        Color base,
        Color secondary,
        std::u32string_view palette,
        MaterialPresets::Type type,
        float coverage,
        float rotation,
        float variation
    )
    {
        const auto preset =
            MaterialPresets::surface(type);

        SurfaceMaterial material{};

        material.base = base;
        material.secondary = secondary;

        material.shadow = {
            base.r * 0.35f,
            base.g * 0.35f,
            base.b * 0.35f
        };

        material.highlight = {
            std::min(1.0f, base.r * 1.45f),
            std::min(1.0f, base.g * 1.45f),
            std::min(1.0f, base.b * 1.45f)
        };

        material.palette = palette;

        material.density = 1.0f;
        material.coverage = coverage;
        material.rotation = rotation;
        material.variation = variation;
        material.detail = 0.75f;

        material.roughness = preset.roughness;
        material.reflectivity = preset.reflectivity;

        return material;
    }
}

// ============================================
// LOCAL SPACE TRANSFORMATIONS
// ============================================

KnightRenderer::Vec KnightRenderer::pitch(
    Vec v,
    float angle
)
{
    const float c = std::cos(angle);
    const float s = std::sin(angle);

    return {
        v.x,
        c * v.y - s * v.z,
        s * v.y + c * v.z
    };
}

KnightRenderer::Vec KnightRenderer::roll(
    Vec v,
    float angle
)
{
    const float c = std::cos(angle);
    const float s = std::sin(angle);

    return {
        c * v.x + s * v.z,
        v.y,
        -s * v.x + c * v.z
    };
}

// ============================================
// UNIVERSAL ELLIPSOID GENERATION
// ============================================

void KnightRenderer::ellipsoid(
    PartId part,
    Vec center,
    Vec radius,
    Material material,
    float tilt
)
{
    auto& surface = parts[part].surface;

    const float maximumRadius =
        std::max({
            radius.x,
            radius.y,
            radius.z
        });

    const float spacing =
        std::max(
            0.004f,
            std::min(
                SAMPLE_SPACING,
                maximumRadius * 0.45f
            )
        );

    SurfaceSampler::Settings settings;

    settings.spacing = spacing;
    settings.jitter = SURFACE_JITTER;

    settings.seed =
        static_cast<std::uint32_t>(
            part * 73856093u
            + surface.size() * 19349663u
            + 83492791u
        );

    SurfaceSampler::ellipsoid(
        {0.0f, 0.0f, 0.0f},

        {
            radius.x,
            radius.y,
            radius.z
        },

        settings,

        [&](const SurfaceSampler::Sample& sample)
        {
            const Vec position = {
                sample.position.x,
                sample.position.y,
                sample.position.z
            };

            const Vec normal = {
                sample.normal.x,
                sample.normal.y,
                sample.normal.z
            };

            surface.push_back({
                center + roll(position, tilt),

                roll(normal, tilt),

                material,

                static_cast<unsigned>(
                    sample.variation * 65535.0f
                ),

                spacing
            });
        }
    );
}

// ============================================
// KNIGHT GEOMETRY
// ============================================

KnightRenderer::KnightRenderer()
    : parts(PartCount)
{
    using SVec = SurfaceSampler::Vec;
    using SSample = SurfaceSampler::Sample;

    // ========================================
    // JOINT HIERARCHY
    // ========================================

    const auto joint =
        [this](
            PartId id,
            PartId parent,
            Vec offset
        )
    {
        parts[id].parent = parent;
        parts[id].offset = offset;
    };

    // ========================================
    // PARAMETRIC SURFACE HELPER
    // ========================================

    const auto customSurface =
        [this](
            PartId part,
            const SurfaceSampler::Mapping& mapping,
            float uLength,
            float vLength,
            float spacing,
            auto materialSelector,
            bool invertNormal = false
        )
    {
        auto& output = parts[part].surface;

        SurfaceSampler::Settings settings;

        settings.spacing = spacing;
        settings.jitter = SURFACE_JITTER;

        settings.seed =
            static_cast<std::uint32_t>(
                part * 73856093u
                + output.size() * 19349663u
            );

        SurfaceSampler::parametric(
            mapping,

            uLength,
            vLength,

            settings,

            [&](const SSample& sample)
            {
                const float direction =
                    invertNormal ? -1.0f : 1.0f;

                output.push_back({
                    {
                        sample.position.x,
                        sample.position.y,
                        sample.position.z
                    },

                    {
                        sample.normal.x * direction,
                        sample.normal.y * direction,
                        sample.normal.z * direction
                    },

                    materialSelector(sample),

                    static_cast<unsigned>(
                        sample.variation * 65535.0f
                    ),

                    spacing
                });
            }
        );
    };

    // ========================================
    // MAIN SKELETON
    // ========================================

    joint(Hips, Body, {0, 0, 0.49f});

    joint(Chest, Hips, {0, 0, 0.105f});

    joint(Neck, Chest, {0, 0, 0.215f});

    joint(Head, Neck, {0, 0, 0.085f});

    // ========================================
    // PELVIS AND UNDERARMOR
    // ========================================

    ellipsoid(
        Hips,
        {},
        {0.113f, 0.074f, 0.072f},
        Material::Cloth
    );

    // Leather belt.

    ellipsoid(
        Hips,

        {0, 0, 0.068f},

        {0.116f, 0.079f, 0.018f},

        Material::Leather
    );

    // Metal belt buckle.

    ellipsoid(
        Hips,

        {0, 0.080f, 0.068f},

        {0.024f, 0.012f, 0.017f},

        Material::Edge
    );

    // ========================================
    // SEGMENTED WAIST ARMOR
    // ========================================

    for (int i = 0; i < 3; ++i)
    {
        const float t =
            static_cast<float>(i);

        ellipsoid(
            Chest,

            {
                0,
                0,
                -0.005f + t * 0.026f
            },

            {
                0.115f + t * 0.006f,
                0.078f + t * 0.005f,
                0.023f
            },

            Material::Steel
        );

        // Lower edge of each armor plate.

        ellipsoid(
            Chest,

            {
                0,
                0,
                -0.016f + t * 0.026f
            },

            {
                0.117f + t * 0.006f,
                0.080f + t * 0.005f,
                0.008f
            },

            Material::Recess
        );
    }

    // ========================================
    // BREASTPLATE
    // ========================================

    customSurface(
        Chest,

        [](float u, float v) -> SVec
        {
            const float angle = u * TAU;

            const float c = std::cos(angle);
            const float s = std::sin(angle);

            // Wider chest, narrower waist.

            const float width =
                0.107f
                + 0.037f
                * std::sin(v * PI * 0.82f);

            const float depth =
                0.076f
                + 0.024f
                * std::sin(v * PI);

            // Raised central breastplate ridge.

            const float front =
                std::max(0.0f, s);

            const float ridge =
                0.018f
                * std::pow(front, 12.0f)
                * std::sin(v * PI);

            return {
                width * c,

                depth * s + ridge,

                0.045f + v * 0.157f
            };
        },

        0.76f,
        0.18f,

        SAMPLE_SPACING,

        [](const SSample& sample)
        {
            if (
                sample.v < 0.055f
                || sample.v > 0.95f
            )
            {
                return Material::Edge;
            }

            return Material::Steel;
        }
    );

    // Upper closure below the neck.

    ellipsoid(
        Chest,

        {0, 0, 0.192f},

        {0.105f, 0.070f, 0.025f},

        Material::Steel
    );

    // ========================================
    // GORGET AND NECK
    // ========================================

    ellipsoid(
        Neck,
        {},
        {0.057f, 0.054f, 0.033f},
        Material::Recess
    );

    ellipsoid(
        Neck,

        {0, 0, 0.013f},

        {0.061f, 0.058f, 0.018f},

        Material::Steel
    );

    // ========================================
    // HELMET
    // ========================================

    customSurface(
        Head,

        [](float u, float v) -> SVec
        {
            const float angle =
                u * TAU;

            const float latitude =
                (v - 0.5f) * PI;

            const float radial =
                std::cos(latitude);

            const float z =
                std::sin(latitude);

            const float cx =
                std::cos(angle);

            const float sy =
                std::sin(angle);

            const float front =
                std::max(0.0f, sy);

            // Slightly angular helmet profile.

            const float side =
                0.073f
                * std::pow(radial, 0.85f);

            // Raised visor and central ridge.

            const float visor =
                0.018f
                * std::pow(front, 8.0f)
                * radial;

            return {
                side * cx,

                0.075f * radial * sy
                    + visor,

                0.016f + z * 0.079f
            };
        },

        0.48f,
        0.25f,

        0.010f,

        [](const SSample& sample)
        {
            const auto& p =
                sample.position;

            // Horizontal dark visor opening.

            if (
                p.y > 0.061f
                && std::abs(p.x) < 0.052f
                && p.z > 0.019f
                && p.z < 0.034f
            )
            {
                return Material::Recess;
            }

            // Lower edge of the helmet.

            if (p.z < -0.046f)
            {
                return Material::Edge;
            }

            return Material::Steel;
        }
    );

    // Lower visor plate.

    ellipsoid(
        Head,

        {0, 0.048f, -0.024f},

        {0.054f, 0.037f, 0.029f},

        Material::Steel
    );

    // ========================================
    // ARM AND LEG GEOMETRY
    // ========================================

    for (const bool right : {false, true})
    {
        const float side =
            right ? 1.0f : -1.0f;

        const auto shoulder =
            right ? ShoulderR : ShoulderL;

        const auto arm =
            right ? UpperArmR : UpperArmL;

        const auto fore =
            right ? ForearmR : ForearmL;

        const auto hand =
            right ? HandR : HandL;

        const auto thigh =
            right ? ThighR : ThighL;

        const auto shin =
            right ? ShinR : ShinL;

        const auto foot =
            right ? FootR : FootL;

        // ====================================
        // ARM JOINTS
        // ====================================

        joint(
            shoulder,
            Chest,
            {side * 0.137f, 0, 0.165f}
        );

        joint(
            arm,
            shoulder,
            {side * 0.022f, 0, -0.032f}
        );

        joint(
            fore,
            arm,
            {side * 0.017f, 0, -0.118f}
        );

        joint(
            hand,
            fore,
            {side * 0.005f, 0.013f, -0.108f}
        );

        // ====================================
        // SHOULDER ARMOR
        // ====================================

        ellipsoid(
            shoulder,
            {},
            {0.067f, 0.065f, 0.047f},
            Material::Steel,
            side * -0.24f
        );

        // Overlapping shoulder plates.

        for (int i = 0; i < 2; ++i)
        {
            ellipsoid(
                shoulder,

                {
                    side * (0.019f + i * 0.006f),
                    0,
                    -0.025f - i * 0.017f
                },

                {
                    0.053f - i * 0.007f,
                    0.055f - i * 0.005f,
                    0.023f
                },

                Material::Steel,

                side * -0.15f
            );
        }

        // ====================================
        // UPPER ARM
        // ====================================

        ellipsoid(
            arm,
            {},
            {0.035f, 0.035f, 0.039f},
            Material::Cloth
        );

        ellipsoid(
            arm,

            {side * 0.006f, 0, -0.060f},

            {0.037f, 0.037f, 0.069f},

            Material::Steel
        );

        // ====================================
        // ELBOW JOINT
        // ====================================

        ellipsoid(
            fore,
            {},
            {0.035f, 0.036f, 0.030f},
            Material::Recess
        );

        ellipsoid(
            fore,

            {side * 0.009f, -0.008f, 0},

            {0.038f, 0.037f, 0.028f},

            Material::Edge
        );

        // ====================================
        // FOREARM ARMOR
        // ====================================

        customSurface(
            fore,

            [](float u, float v) -> SVec
            {
                const float a = u * TAU;

                const float radius =
                    std::lerp(
                        0.034f,
                        0.024f,
                        v
                    );

                return {
                    std::cos(a) * radius,

                    std::sin(a)
                        * radius
                        * 1.08f,

                    -0.012f - v * 0.098f
                };
            },

            0.19f,
            0.10f,

            0.013f,

            [](const SSample& sample)
            {
                if (
                    sample.v < 0.08f
                    || sample.v > 0.92f
                )
                {
                    return Material::Edge;
                }

                return Material::Steel;
            },

            true
        );

        // ====================================
        // GAUNTLETS
        // ====================================

        ellipsoid(
            hand,
            {},
            {0.027f, 0.029f, 0.037f},
            Material::Leather
        );

        ellipsoid(
            hand,

            {0, 0.014f, -0.006f},

            {0.029f, 0.022f, 0.026f},

            Material::Steel
        );

        // Finger armor segments.

        for (int i = 0; i < 3; ++i)
        {
            ellipsoid(
                hand,

                {
                    0,
                    0.018f,
                    -0.014f - i * 0.009f
                },

                {
                    0.026f,
                    0.019f,
                    0.008f
                },

                Material::Edge
            );
        }

        // ====================================
        // LEG JOINTS
        // ====================================

        joint(
            thigh,
            Hips,
            {side * 0.066f, 0, -0.026f}
        );

        joint(
            shin,
            thigh,
            {0, 0, -0.211f}
        );

        joint(
            foot,
            shin,
            {0, 0, -0.196f}
        );

        // ====================================
        // THIGH ARMOR
        // ====================================

        ellipsoid(
            thigh,
            {},
            {0.049f, 0.052f, 0.044f},
            Material::Cloth
        );

        ellipsoid(
            thigh,

            {0, 0.007f, -0.100f},

            {0.048f, 0.051f, 0.112f},

            Material::Steel
        );

        // ====================================
        // KNEE JOINT
        // ====================================

        ellipsoid(
            shin,
            {},
            {0.035f, 0.039f, 0.038f},
            Material::Recess
        );

        // Knee armor.

        ellipsoid(
            shin,

            {0, 0.028f, -0.002f},

            {0.043f, 0.029f, 0.042f},

            Material::Steel
        );

        ellipsoid(
            shin,

            {0, 0.052f, -0.006f},

            {0.028f, 0.012f, 0.028f},

            Material::Edge
        );

        // ====================================
        // GREAVES
        // ====================================

        customSurface(
            shin,

            [](float u, float v) -> SVec
            {
                const float a =
                    u * TAU;

                const float ca =
                    std::cos(a);

                const float sa =
                    std::sin(a);

                const float radius =
                    std::lerp(
                        0.041f,
                        0.026f,
                        v
                    );

                const float calf =
                    0.006f * std::sin(v * PI);

                const float ridge =
                    0.009f
                    * std::pow(
                        std::max(0.0f, sa),
                        10.0f
                    );

                return {
                    ca * radius,

                    sa * (radius + calf)
                        + ridge,

                    -0.032f - v * 0.164f
                };
            },

            0.23f,
            0.17f,

            0.014f,

            [](const SSample& sample)
            {
                if (sample.v > 0.94f)
                {
                    return Material::Edge;
                }

                return Material::Steel;
            },

            true
        );

        // ====================================
        // ARMORED FOOT
        // ====================================

        ellipsoid(
            foot,

            {0, 0.037f, -0.021f},

            {0.039f, 0.068f, 0.035f},

            Material::Leather
        );

        // Overlapping metal foot plates.

        for (int i = 0; i < 4; ++i)
        {
            const float t =
                static_cast<float>(i) / 3.0f;

            ellipsoid(
                foot,

                {
                    0,
                    0.014f + t * 0.073f,
                    -0.013f - t * 0.014f
                },

                {
                    0.039f - t * 0.007f,
                    0.025f,
                    0.024f - t * 0.007f
                },

                Material::Steel
            );
        }
    }

    // ========================================
    // HEATER SHIELD
    // ========================================

    joint(
        Shield,
        HandL,

        {-0.034f, 0.060f, 0.060f}
    );

    const auto shieldWidth =
        [](float v)
    {
        // Narrow bottom, wide shoulders.

        return 0.008f
            + 0.113f
            * std::pow(v, 0.65f);
    };

    for (const bool front : {false, true})
    {
        customSurface(
            Shield,

            [front, shieldWidth](
                float u,
                float v
            ) -> SVec
            {
                const float horizontal =
                    2.0f * u - 1.0f;

                const float width =
                    shieldWidth(v);

                const float curve =
                    0.023f
                    * (
                        1.0f
                        - horizontal * horizontal
                    );

                return {
                    horizontal * width,

                    curve
                        + (front ? 0.014f : -0.014f),

                    -0.165f + v * 0.33f
                };
            },

            0.25f,
            0.33f,

            0.012f,

            [](const SSample& sample)
            {
                // Metal edge around the shield.

                if (
                    sample.u < 0.07f
                    || sample.u > 0.93f
                    || sample.v < 0.055f
                    || sample.v > 0.95f
                )
                {
                    return Material::Edge;
                }

                // Metallic cross on the front.

                if (
                    std::abs(sample.u - 0.5f)
                        < 0.045f
                    ||
                    std::abs(sample.v - 0.68f)
                        < 0.035f
                )
                {
                    return Material::Steel;
                }

                return Material::Wood;
            },

            front
        );
    }

    // Central shield reinforcement.

    ellipsoid(
        Shield,

        {0, 0.048f, 0.055f},

        {0.030f, 0.017f, 0.030f},

        Material::Edge
    );

    // ========================================
    // SWORD
    // ========================================

    joint(
        Sword,
        HandR,

        {0.012f, 0.014f, 0}
    );

    constexpr float swordTilt =
        -0.47f;

    // Leather grip.

    ellipsoid(
        Sword,
        {},
        {0.015f, 0.018f, 0.043f},
        Material::Leather,
        swordTilt
    );

    // Pommel.

    ellipsoid(
        Sword,

        roll(
            {0, 0, 0.052f},
            swordTilt
        ),

        {0.022f, 0.021f, 0.021f},

        Material::Edge
    );

    // Crossguard.

    ellipsoid(
        Sword,

        roll(
            {0, 0, -0.048f},
            swordTilt
        ),

        {0.067f, 0.018f, 0.012f},

        Material::Edge,

        swordTilt
    );

    // ========================================
    // FOUR-FACED SWORD BLADE
    // ========================================

    constexpr std::array<SVec, 4> bladeSection{{
        { 1,  0, 0},
        { 0,  1, 0},
        {-1,  0, 0},
        { 0, -1, 0}
    }};

    for (int face = 0; face < 4; ++face)
    {
        const SVec a =
            bladeSection[face];

        const SVec b =
            bladeSection[(face + 1) % 4];

        customSurface(
            Sword,

            [a, b, swordTilt](
                float u,
                float v
            ) -> SVec
            {
                // Blade becomes narrow near its tip.

                const float taper =
                    v < 0.78f
                    ? std::lerp(
                        1.0f,
                        0.75f,
                        v / 0.78f
                    )
                    : 0.75f
                        * (1.0f - v)
                        / 0.22f;

                const SVec section =
                    a * (1.0f - u) + b * u;

                const Vec p = {
                    section.x * 0.023f * taper,

                    section.y * 0.007f * taper,

                    -0.058f - v * 0.335f
                };

                const Vec rotated =
                    roll(p, swordTilt);

                return {
                    rotated.x,
                    rotated.y,
                    rotated.z
                };
            },

            0.025f,
            0.335f,

            0.008f,

            [](const SSample& sample)
            {
                if (
                    sample.u < 0.14f
                    || sample.u > 0.86f
                )
                {
                    return Material::Edge;
                }

                return Material::Steel;
            },

            true
        );
    }
}

// ============================================
// ANIMATION UPDATE
// ============================================

void KnightRenderer::update(
    float dx,
    float dy,
    float dt,
    float height
)
{
    if (dt <= 0.0f || height <= 0.0f)
    {
        return;
    }

    const float distance =
        std::hypot(dx, dy);

    const bool moving =
        distance > 0.0001f;

    idleTime =
        std::fmod(
            idleTime + dt,
            12.0f
        );

    walkWeight =
        std::lerp(
            walkWeight,

            moving ? 1.0f : 0.0f,

            1.0f - std::exp(-12.0f * dt)
        );

    if (moving)
    {
        const float target =
            std::atan2(-dx, dy);

        heading +=
            std::remainder(
                target - heading,
                TAU
            )
            * (
                1.0f
                - std::exp(-16.0f * dt)
            );

        stride =
            std::fmod(
                stride
                    + distance * TAU
                    / (height * 0.97f),

                TAU
            );
    }
}

// ============================================
// KNIGHT RENDERING
// ============================================

void KnightRenderer::render(
    const Camera& camera,
    GlyphRenderer& renderer,
    float x,
    float y,
    float height,
    float hour
) const
{
    if (height <= 0.0f)
    {
        return;
    }

    // Lighting is now provided by the GPU.
    // Keep this argument for API compatibility.

    (void)hour;

    // ========================================
    // ANIMATION POSE
    // ========================================

    const float swing =
        std::sin(stride) * walkWeight;

    const float breathing =
        std::sin(
            idleTime * PI / 2.0f
        ) * 0.005f;

    std::array<float, PartCount> angles{};

    std::array<Vec, PartCount> origins{};

    origins[Body].z =
        -0.014f * std::abs(swing);

    angles[Chest] =
        -0.035f * walkWeight
        + breathing;

    angles[Head] =
        -angles[Chest] * 0.55f;

    angles[ShoulderL] =
        -0.13f - swing * 0.13f;

    angles[ShoulderR] =
        -0.04f + swing * 0.25f;

    angles[ForearmL] = -0.55f;
    angles[ForearmR] = -0.18f;

    angles[ThighL] = swing * 0.46f;
    angles[ThighR] = -swing * 0.46f;

    angles[ShinL] =
        -std::max(
            0.0f,
            swing
        ) * 0.48f;

    angles[ShinR] =
        -std::max(
            0.0f,
            -swing
        ) * 0.48f;

    angles[FootL] =
        -(
            angles[ThighL]
            + angles[ShinL]
        ) * 0.65f;

    angles[FootR] =
        -(
            angles[ThighR]
            + angles[ShinR]
        ) * 0.65f;

    // ========================================
    // PHYSICAL MATERIALS
    // ========================================

    using Type =
        MaterialPresets::Type;

    static const std::array<
        SurfaceMaterial,
        static_cast<unsigned>(Material::Count)
    > materials{{
        // Steel armor.

        makeMaterial(
            {0.53f, 0.57f, 0.61f},

            {0.64f, 0.67f, 0.70f},

            U"@@##%%88HH",

            Type::Steel,

            STEEL_COVERAGE,

            0.10f,
            0.13f
        ),

        // Dark recesses and joints.

        makeMaterial(
            {0.095f, 0.110f, 0.125f},

            {0.15f, 0.16f, 0.17f},

            U"##%%@@88",

            Type::Iron,

            CLOTH_COVERAGE,

            0.10f,
            0.12f
        ),

        // Leather.

        makeMaterial(
            {0.22f, 0.115f, 0.060f},

            {0.31f, 0.18f, 0.09f},

            U"##88%%@@",

            Type::Leather,

            CLOTH_COVERAGE,

            0.25f,
            0.22f
        ),

        // Dark cloth.

        makeMaterial(
            {0.16f, 0.19f, 0.17f},

            {0.24f, 0.26f, 0.22f},

            U"##%%xx88",

            Type::Cloth,

            CLOTH_COVERAGE,

            0.30f,
            0.18f
        ),

        // Polished metal edges.

        makeMaterial(
            {0.67f, 0.70f, 0.73f},

            {0.80f, 0.81f, 0.82f},

            U"##HH88@@",

            Type::Steel,

            DETAIL_COVERAGE,

            0.05f,
            0.08f
        ),

        // Shield wood.

        makeMaterial(
            {0.23f, 0.12f, 0.065f},

            {0.36f, 0.21f, 0.11f},

            U"##88HH%%",

            Type::Wood,

            STEEL_COVERAGE,

            0.20f,
            0.25f
        )
    }};

    // ========================================
    // WORLD SPACE ORIENTATION
    // ========================================

    const float c =
        std::cos(heading);

    const float s =
        std::sin(heading);

    const auto yaw =
        [c, s](Vec v)
    {
        return Vec{
            c * v.x - s * v.y,

            s * v.x + c * v.y,

            v.z
        };
    };

    const Vec viewDirection = {
        camera.getSinAzimuth()
            * camera.getLift(),

        camera.getCosAzimuth()
            * camera.getLift(),

        camera.getRise()
    };

    const auto dot =
        [](Vec a, Vec b)
    {
        return
            a.x * b.x
            + a.y * b.y
            + a.z * b.z;
    };

    // ========================================
    // CHARACTER LEVEL OF DETAIL
    // ========================================

    const float projectedHeight =
        height * camera.getZoom();

    const int sampleStep =
        projectedHeight < 42.0f
        ? 2
        : 1;

    const float lodCoverage =
        sampleStep == 2
        ? 1.40f
        : 1.0f;

    // ========================================
    // HIERARCHICAL GEOMETRY TRANSFORMATION
    // ========================================

    for (
        std::size_t i = 0;
        i < parts.size();
        ++i
    )
    {
        const Part& part =
            parts[i];

        if (part.parent >= 0)
        {
            origins[i] =
                origins[part.parent]
                + pitch(
                    part.offset,
                    angles[part.parent]
                );

            angles[i] +=
                angles[part.parent];
        }

        for (
            std::size_t j = 0;
            j < part.surface.size();
            j += sampleStep
        )
        {
            const auto& sample =
                part.surface[j];

            // Rotate the physical surface normal
            // using the same joint transformation.

            const Vec n =
                yaw(
                    pitch(
                        sample.normal,
                        angles[i]
                    )
                );

            // Conservative backface culling.

            if (
                dot(n, viewDirection)
                < -0.25f
            )
            {
                continue;
            }

            // Local position -> world position.

            const Vec p =
                yaw(
                    origins[i]
                    + pitch(
                        sample.position,
                        angles[i]
                    )
                );

            const auto& material =
                materials[
                    static_cast<unsigned>(
                        sample.material
                    )
                ];

            // Convert the cached local sample
            // to the existing SurfaceMaterial API.

            SurfaceSampler::Sample gpuSample{};

            gpuSample.position = {
                x + p.x * height,

                y + p.y * height,

                std::max(
                    0.0f,
                    p.z * height
                )
            };

            gpuSample.normal = {
                n.x,
                n.y,
                n.z
            };

            gpuSample.spacing =
                sample.size
                * height
                * lodCoverage;

            gpuSample.variation =
                static_cast<float>(
                    sample.pattern % 65536u
                ) / 65535.0f;

            const float pattern =
                0.5f
                + 0.25f
                * std::sin(
                    sample.position.z * 95.0f
                    + sample.position.x * 38.0f
                );

            // Generate intrinsic material data.
            // No CPU directional lighting.

            VisualGlyph glyph =
                material.glyph(
                    gpuSample,

                    pattern,

                    1.0f
                );

            glyph.transform.opacity = 1.0f;
            glyph.transform.pivotX = 0.5f;
            glyph.transform.pivotY = 0.5f;

            // Submit dynamic geometry and
            // world-space lighting properties.

            renderer.drawWorld(
                camera,

                glyph.character,

                glyph.x,
                glyph.y,
                glyph.z,

                glyph.scale,

                glyph.color,

                glyph.animation,

                glyph.transform,

                GlyphAnchor::Center,

                false,

                true,

                glyph.material
            );
        }
    }

    // ========================================
    // GROUND CONTACT SHADOW
    // ========================================

    GlyphTransform shadow;

    shadow.opacity = 0.45f;

    shadow.pivotX = 0.5f;
    shadow.pivotY = 0.5f;

    for (int i = 0; i < 12; ++i)
    {
        const float angle =
            i * 2.399963f;

        const float radius =
            std::sqrt(
                (i + 0.5f) / 12.0f
            );

        renderer.drawWorld(
            camera,

            U'=',

            x + std::cos(angle)
                * radius * height * 0.15f,

            y + std::sin(angle)
                * radius * height * 0.10f,

            0.0f,

            height / 310.0f,

            {0.025f, 0.029f, 0.025f},

            {},

            shadow,

            GlyphAnchor::Center,

            true
        );
    }
}
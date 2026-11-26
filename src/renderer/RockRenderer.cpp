#include "renderer/RockRenderer.hpp"
#include "renderer/SurfaceSampler.hpp"
#include "renderer/SurfaceMaterial.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace
{
    using namespace SurfaceSampler;

    constexpr float PI =
        std::numbers::pi_v<float>;

    constexpr float TAU = 2.0f * PI;

    // ========================================
    // ROCK GENERATION SETTINGS
    // ========================================

    constexpr float BASE_SURFACE_DENSITY   = 1.75f;
    constexpr float BASE_SURFACE_COVERAGE  = 2.35f;

    constexpr float DETAIL_SURFACE_DENSITY  = 1.05f;
    constexpr float DETAIL_SURFACE_COVERAGE = 1.20f;

    constexpr float MOSS_AMOUNT       = 0.85f;
    constexpr float CRACK_STRENGTH    = 0.055f;
    constexpr float SURFACE_ROUGHNESS = 0.06f;

    // ========================================
    // RANDOM GENERATION
    // ========================================

    float random(std::uint32_t& state)
    {
        state =
            state * 1664525u + 1013904223u;

        return static_cast<float>(
            state >> 8u
        ) / 16777216.0f;
    }

    // ========================================
    // PROCEDURAL SURFACE DETAILS
    // ========================================

    float smoothstep(
        float edge0,
        float edge1,
        float value
    )
    {
        const float t = std::clamp(
            (value - edge0)
                / (edge1 - edge0),
            0.0f,
            1.0f
        );

        return t * t * (3.0f - 2.0f * t);
    }

    float surfaceNoise(
        float u,
        float v,
        float phase
    )
    {
        return
            0.50f * std::sin(
                u * 7.0f
                + v * 4.0f
                + phase
            )
            +
            0.30f * std::cos(
                u * 13.0f
                - v * 9.0f
                + phase * 1.3f
            )
            +
            0.20f * std::sin(
                u * 23.0f
                + v * 19.0f
                - phase
            );
    }

    float crackPattern(
        float u,
        float v,
        float phase
    )
    {
        const float distortion =
            std::sin(
                u * 8.0f
                + phase
            ) * 1.1f;

        const float strata =
            std::sin(
                v * 12.0f
                + u * 6.0f
                + distortion
                + phase
            );

        return smoothstep(
            0.86f,
            0.99f,
            strata
        );
    }

    // ========================================
    // ROCK MATERIAL
    // ========================================

    SurfaceMaterial baseRockMaterial(float tone)
    {
        SurfaceMaterial material{};

        material.base = {
            0.52f * tone,
            0.54f * tone,
            0.55f * tone
        };

        material.secondary = {
            0.66f * tone,
            0.66f * tone,
            0.63f * tone
        };

        material.shadow = {
            0.15f * tone,
            0.16f * tone,
            0.17f * tone
        };

        material.highlight = {
            0.78f,
            0.80f,
            0.82f
        };

        // ЩІЛЬНА палітра для базового заповнення
        material.palette = U"@@##%%88OO";

        material.density   = BASE_SURFACE_DENSITY;
        material.coverage  = BASE_SURFACE_COVERAGE;
        material.rotation  = 0.18f;
        material.variation = 0.08f;
        material.detail    = 0.15f;
        material.roughness = 0.94f;
        material.reflectivity = 0.04f;

        return material;
    }

    SurfaceMaterial detailRockMaterial(float tone)
    {
        SurfaceMaterial material{};

        material.base = {
            0.56f * tone,
            0.57f * tone,
            0.58f * tone
        };

        material.secondary = {
            0.70f * tone,
            0.69f * tone,
            0.66f * tone
        };

        material.shadow = {
            0.13f * tone,
            0.14f * tone,
            0.15f * tone
        };

        material.highlight = {
            0.82f,
            0.83f,
            0.84f
        };

        // Менш щільна палітра для деталей
        material.palette = U"#%8Oox;:.";

        material.density   = DETAIL_SURFACE_DENSITY;
        material.coverage  = DETAIL_SURFACE_COVERAGE;
        material.rotation  = 0.80f;
        material.variation = 0.30f;
        material.detail    = 1.00f;
        material.roughness = 0.96f;
        material.reflectivity = 0.03f;

        return material;
    }

    // ========================================
    // MOSS MATERIAL
    // ========================================

    SurfaceMaterial mossMaterial()
    {
        SurfaceMaterial material{};

        material.base = {
            0.23f,
            0.37f,
            0.14f
        };

        material.secondary = {
            0.36f,
            0.48f,
            0.19f
        };

        material.shadow = {
            0.07f,
            0.13f,
            0.045f
        };

        material.highlight = {
            0.48f,
            0.57f,
            0.27f
        };

        material.palette =
            U"#%8*x+o";

        material.density = 1.30f;

        material.coverage = 1.90f;

        material.rotation = 1.20f;

        material.variation = 0.30f;

        material.detail = 0.90f;

        material.roughness = 0.98f;

        material.reflectivity = 0.0f;

        return material;
    }

    // ========================================
    // ROCK BOUNDARY
    // ========================================

    float boundaryRadius(
        const Rock& rock,
        float dx,
        float dy
    )
    {
        float inner = 0.0f;
        float outer = 1.0f;

        // Find the existing collision contour.

        for (int i = 0; i < 14; ++i)
        {
            const float middle =
                (inner + outer) * 0.5f;

            if (
                rock.surface(
                    dx * middle,
                    dy * middle
                ) > 0.0f
            )
            {
                inner = middle;
            }
            else
            {
                outer = middle;
            }
        }

        return inner;
    }

    // ========================================
    // ROCK HEIGHT FUNCTION
    // ========================================

    float rockHeight(
        const Rock& rock,
        float u,
        float v,
        float phase
    )
    {
        const float surface =
            rock.surface(u, v);

        if (surface <= 0.0f)
        {
            return 0.0f;
        }

        // Basic volumetric shape.

        const float base =
            std::sqrt(surface);

        // Large asymmetric facets.

        const float facetA =
            1.0f
            - 0.22f
            * std::abs(
                u + 0.45f * v
            );

        const float facetB =
            0.98f
            - 0.18f
            * std::abs(
                v - 0.60f * u
            );

        const float facet =
            std::min(facetA, facetB);

        // Small coherent surface deformation.

        const float roughness =
            surfaceNoise(u, v, phase)
            * SURFACE_ROUGHNESS;

        // Geological cracks.

        const float crack =
            crackPattern(u, v, phase)
            * CRACK_STRENGTH;

        const float deformation =
            std::max(
                0.25f,
                facet + roughness - crack
            );

        return
            rock.height
            * base
            * deformation;
    }

    // ========================================
    // SURFACE NORMAL CALCULATION
    // ========================================

    Vec calculateNormal(
        const Rock& rock,
        float u,
        float v,
        float phase
    )
    {
        constexpr float epsilon = 0.003f;

        const float left =
            rockHeight(
                rock,
                u - epsilon,
                v,
                phase
            );

        const float right =
            rockHeight(
                rock,
                u + epsilon,
                v,
                phase
            );

        const float top =
            rockHeight(
                rock,
                u,
                v - epsilon,
                phase
            );

        const float bottom =
            rockHeight(
                rock,
                u,
                v + epsilon,
                phase
            );

        // Local height derivatives.

        const float derivativeX =
            (right - left)
            / (
                2.0f
                * epsilon
                * rock.radiusX
            );

        const float derivativeY =
            (bottom - top)
            / (
                2.0f
                * epsilon
                * rock.radiusY
            );

        const Vec localNormal =
            unit({
                -derivativeX,
                -derivativeY,
                1.0f
            });

        // Rotate the normal into world space.

        const float c =
            std::cos(rock.angle);

        const float s =
            std::sin(rock.angle);

        return unit({
            c * localNormal.x
                - s * localNormal.y,

            s * localNormal.x
                + c * localNormal.y,

            localNormal.z
        });
    }

    // ========================================
    // MOSS DISTRIBUTION
    // ========================================

    float mossPattern(
        float u,
        float v,
        float height,
        const Vec& normal,
        float phase
    )
    {
        // Spatially coherent moss patches.

        const float pattern =
            std::sin(
                u * 5.0f
                - v * 3.0f
                + phase
            )
            +
            0.65f * std::cos(
                v * 8.0f
                + phase * 0.7f
            )
            +
            0.30f * std::sin(
                u * 16.0f
                + v * 11.0f
            );

        const float patch =
            smoothstep(
                0.25f,
                1.10f,
                pattern
            );

        // Prefer lower and less exposed areas.

        const float moisture =
            1.0f - height * 0.55f;

        // Growth favors sheltered slopes. This is an intrinsic material mask.
        const float exposure = std::clamp(normal.z,0.0f,1.0f);

        const float shade =
            0.55f
            + 0.45f * (1.0f - exposure);

        return std::clamp(
            patch
                * moisture
                * shade
                * MOSS_AMOUNT,
            0.0f,
            1.0f
        );
    }
}

// ============================================
// ROCK GEOMETRY GENERATION
// ============================================

std::vector<VisualGlyph>
RockRenderer::generate(
    const Rock& rock,
    float spacing
)
{
    using namespace SurfaceSampler;

    std::vector<VisualGlyph> output;

    if (
        rock.radiusX <= 0.0f
        || rock.radiusY <= 0.0f
        || rock.height <= 0.0f
        || spacing <= 0.0f
    )
    {
        return output;
    }

    auto state = rock.seed;

    const float phase =
        random(state) * TAU;

    const float tone =
        0.85f + random(state) * 0.30f;

    const float c =
        std::cos(rock.angle);

    const float s =
        std::sin(rock.angle);

    const SurfaceMaterial baseStone =
        baseRockMaterial(tone);

    const SurfaceMaterial detailStone =
        detailRockMaterial(tone);

    const SurfaceMaterial moss =
        mossMaterial();

    const float minimumDimension =
        std::min({
            rock.radiusX,
            rock.radiusY,
            rock.height
        });

    // Базовий шар - щільний
    const float baseSpacing =
        std::max(
            0.16f,
            std::min(
                baseStone.spacing(spacing * 0.72f),
                minimumDimension * 0.35f
            )
        );

    // Декоративний шар - рідший
    const float detailSpacing =
        std::max(
            0.24f,
            std::min(
                detailStone.spacing(spacing * 1.05f),
                minimumDimension * 0.60f
            )
        );

    auto emitLayer =
        [&](const SurfaceMaterial& material,
            float sampleSpacing,
            float jitter,
            bool detailLayer)
    {
        Settings settings;
        settings.spacing = sampleSpacing;
        settings.jitter  = jitter;
        settings.seed    = detailLayer
            ? (rock.seed ^ 0xC8013EA4u)
            : (rock.seed ^ 0xA341316Cu);

        SurfaceSampler::ellipsoid(
            {0.0f, 0.0f, 0.0f},
            {
                rock.radiusX,
                rock.radiusY,
                rock.height
            },
            settings,
            [&](const Sample& source)
            {
                if (source.position.z < 0.0f)
                {
                    return;
                }

                const float nx =
                    source.position.x / rock.radiusX;

                const float ny =
                    source.position.y / rock.radiusY;

                const float radial =
                    std::hypot(nx, ny);

                float u = 0.0f;
                float v = 0.0f;

                if (radial > 0.00001f)
                {
                    const float dx = nx / radial;
                    const float dy = ny / radial;

                    const float boundary =
                        boundaryRadius(
                            rock,
                            dx,
                            dy
                        );

                    const float radius =
                        boundary
                        * std::min(
                            radial,
                            0.9995f
                        );

                    u = dx * radius;
                    v = dy * radius;
                }

                const float localX =
                    u * rock.radiusX;

                const float localY =
                    v * rock.radiusY;

                const float z =
                    rockHeight(
                        rock,
                        u,
                        v,
                        phase
                    );

                Sample sample = source;

                sample.position = {
                    rock.x
                        + c * localX
                        - s * localY,

                    rock.y
                        + s * localX
                        + c * localY,

                    z + (detailLayer ? 0.10f : 0.0f)
                };

                sample.normal =
                    calculateNormal(
                        rock,
                        u,
                        v,
                        phase
                    );

                sample.spacing =
                    sampleSpacing;

                const float noise =
                    surfaceNoise(
                        u,
                        v,
                        phase
                    );

                const float materialPattern =
                    std::clamp(
                        0.5f + noise * 0.5f,
                        0.0f,
                        1.0f
                    );

                const float crack =
                    crackPattern(
                        u,
                        v,
                        phase
                    );

                const float normalizedHeight =
                    std::clamp(
                        z / rock.height,
                        0.0f,
                        1.0f
                    );

                const float contactExposure =
                    0.62f
                    + 0.38f * smoothstep(
                        0.0f,
                        0.35f,
                        normalizedHeight
                    );

                VisualGlyph glyph =
                    material.glyph(
                        sample,
                        materialPattern,
                        contactExposure
                    );

                if (!detailLayer)
                {
                    // Базовий шар максимально щільний
                    glyph.scale *=
                        1.12f
                        + sample.variation * 0.10f;

                    glyph.character =
                        material.palette[
                            static_cast<std::size_t>(
                                sample.variation
                                * (
                                    material.palette.size() - 1
                                )
                            )
                        ];

                    const float crackDarkness =
                        1.0f - crack * 0.28f;

                    glyph.color.r *= crackDarkness;
                    glyph.color.g *= crackDarkness;
                    glyph.color.b *= crackDarkness;
                }
                else
                {
                    // Деталі більш вибіркові
                    if (
                        crack < 0.18f
                        && sample.variation < 0.25f
                    )
                    {
                        return;
                    }

                    glyph.scale *=
                        0.92f
                        + sample.variation * 0.14f;

                    const float crackDarkness =
                        1.0f - crack * 0.52f;

                    glyph.color.r *= crackDarkness;
                    glyph.color.g *= crackDarkness;
                    glyph.color.b *= crackDarkness;

                    if (
                        crack > 0.80f
                        && sample.variation < 0.30f
                    )
                    {
                        glyph.character = U'%';
                    }

                    const float mossAmount =
                        mossPattern(
                            u,
                            v,
                            normalizedHeight,
                            sample.normal,
                            phase
                        );

                    if (mossAmount > 0.12f)
                    {
                        const VisualGlyph mossGlyph =
                            moss.glyph(
                                sample,
                                materialPattern,
                                contactExposure
                            );

                        glyph.color.r =
                            std::lerp(
                                glyph.color.r,
                                mossGlyph.color.r,
                                mossAmount
                            );

                        glyph.color.g =
                            std::lerp(
                                glyph.color.g,
                                mossGlyph.color.g,
                                mossAmount
                            );

                        glyph.color.b =
                            std::lerp(
                                glyph.color.b,
                                mossGlyph.color.b,
                                mossAmount
                            );
                        glyph.material.roughness = std::lerp(glyph.material.roughness,
                            mossGlyph.material.roughness,mossAmount);
                        glyph.material.reflectivity = std::lerp(glyph.material.reflectivity,
                            mossGlyph.material.reflectivity,mossAmount);

                        if (
                            mossAmount > 0.58f
                            && sample.variation < mossAmount
                        )
                        {
                            glyph.character =
                                mossGlyph.character;
                        }
                    }
                }

                glyph.transform.opacity = 1.0f;
                glyph.transform.pivotX = 0.5f;
                glyph.transform.pivotY = 0.5f;

                output.push_back(glyph);
            }
        );
    };

    // 1. Щільний базовий шар
    emitLayer(
        baseStone,
        baseSpacing,
        0.035f,
        false
    );

    // 2. Менш щільний детальний шар
    emitLayer(
        detailStone,
        detailSpacing,
        0.12f,
        true
    );

    if (output.empty())
    {
        Sample sample{};

        sample.position = {
            rock.x,
            rock.y,
            rockHeight(
                rock,
                0.0f,
                0.0f,
                phase
            )
        };

        sample.normal =
            {0.0f, 0.0f, 1.0f};

        sample.tangent =
            {1.0f, 0.0f, 0.0f};

        sample.spacing =
            std::min(
                rock.radiusX,
                rock.radiusY
            );

        sample.variation = 0.5f;

        auto fallback =
            baseStone.glyph(
                sample,
                0.5f,
                1.0f
            );

        fallback.scale *= 1.15f;

        output.push_back(fallback);
    }

    return output;
}

// ============================================
// ROCK RENDERING AND GPU CACHE
// ============================================

void RockRenderer::render(
    const Rock& rock,
    const Camera& camera,
    GlyphRenderer& renderer
)
{
    // ========================================
    // LEVEL OF DETAIL
    // ========================================

    const float zoom =
        camera.getZoom();

    const float projectedDiameter =
        2.0f
        * std::max(
            rock.radiusX,
            rock.radiusY
        )
        * zoom;

    const int level =
        projectedDiameter < 48.0f
            ? 2
            : (
                projectedDiameter < 96.0f
                    ? 1
                    : 0
            );

    constexpr std::array spacing{
        1.35f, // High
        2.4f,  // Medium
        4.2f   // Low
    };

    // ========================================
    // GPU CACHE HIT
    // ========================================

    if (renderer.useRockCache(
            rock.id,
            level
        ))
    {
        return;
    }

    // ========================================
    // CPU GEOMETRY CACHE
    // ========================================

    const Key key{
        rock.id,
        level
    };

    auto entry =
        cache.find(key);

    if (entry == cache.end())
    {
        auto glyphs =
            generate(
                rock,
                spacing[level]
            );

        if (
            cache.size()
            >= MAX_CACHED_SURFACES
        )
        {
            cache.erase(
                recentSurfaces.back()
            );

            recentSurfaces.pop_back();
        }

        recentSurfaces.push_front(key);

        entry = cache.emplace(
            key,

            CachedSurface{
                std::move(glyphs),
                recentSurfaces.begin()
            }
        ).first;
    }
    else
    {
        recentSurfaces.splice(
            recentSurfaces.begin(),

            recentSurfaces,

            entry->second.recency
        );
    }

    // ========================================
    // CAPTURE STATIC GEOMETRY
    // ========================================

    for (
        const auto& glyph
        : entry->second.glyphs
    )
    {
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

    // ========================================
    // PERMANENT GPU STORAGE
    // ========================================

    renderer.finishGroundChunk();
}

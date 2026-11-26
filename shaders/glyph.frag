#version 450 core

in vec2 TexCoord;

in vec3 GlyphColor;

in vec2 ScreenPosition;

in float GlyphOpacity;

flat in float Solid;
flat in uint SurfaceMode;
flat in vec3 SurfaceColor;
flat in vec3 ShadowedPoint;

out vec4 FragColor;

uniform sampler2D glyphTexture;

uniform sampler2D uLightmap;

uniform vec2 uScreenSize;

uniform int uCoveragePass;
uniform int uLightingDebug;

// ============================================
// COVERAGE PASSES
//
// 0 - Ground and shadows
// 1 - Opaque object cores
// 2 - Transparent object surfaces
// 3 - Static vegetation alpha cutout
// ============================================

void main()
{
    // ========================================
    // GLYPH COVERAGE
    // ========================================

    float coverage = texture(
        glyphTexture,

        TexCoord
    ).r;

    float alpha =
    coverage * GlyphOpacity;

    bool core =
    Solid > 0.5
    && alpha >= 0.94;

    // ========================================
    // STATIC VEGETATION
    // ========================================

    if (uCoveragePass == 3)
    {
        // Remove transparent regions
        // of the glyph quad.

        if (
        coverage < 0.35
        || alpha < 0.18
        )
        {
            discard;
        }

        // Surviving fragments are treated
        // as opaque vegetation surfaces.

        alpha = 1.0;
    }

    // ========================================
    // EXISTING GLYPH PASSES
    // ========================================

    else
    {
        if (
        alpha <= 0.003
        ||
        (
        uCoveragePass == 1
        && !core
        )
        ||
        (
        uCoveragePass == 2
        && core
        )
        )
        {
            discard;
        }
    }

    // ========================================
    // LIGHT MAP COORDINATES
    // ========================================

    vec2 lightUV = vec2(
    ScreenPosition.x
    / uScreenSize.x,

    1.0
    - ScreenPosition.y
    / uScreenSize.y
    );

    // ========================================
    // SHARED LIGHTING
    // ========================================

    vec4 lightmap = texture(uLightmap,lightUV);
    float visibility = all(greaterThanEqual(lightUV,vec2(0.0))) &&
        all(lessThanEqual(lightUV,vec2(1.0))) ? lightmap.a : 1.0;
    vec3 finalColor;
    if (uLightingDebug == 7)
        finalColor = vec3(visibility);
    else if (uLightingDebug == 1)
        finalColor = GlyphColor;
    else if (SurfaceMode == 0u || uLightingDebug == 10)
        finalColor = GlyphColor * lightmap.rgb;
    else
        finalColor = SurfaceColor + ShadowedPoint * visibility;

    // ========================================
    // FINAL OUTPUT
    // ========================================

    FragColor = vec4(
    clamp(
        finalColor,
        0.0,
        1.0
    ),

    alpha
    );
}

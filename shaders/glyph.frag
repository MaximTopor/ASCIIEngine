#version 450 core

in vec2 TexCoord;
in vec3 GlyphColor;
in vec2 ScreenPosition;
in float GlyphOpacity;

out vec4 FragColor;

uniform sampler2D glyphTexture;
uniform sampler2D uLightmap;

uniform vec2 uScreenSize;

void main()
{
    // Glyph coverage

    float coverage = texture(
        glyphTexture,
        TexCoord
    ).r;

    // Final opacity

    float alpha = coverage * GlyphOpacity;

    if (alpha <= 0.003)
    {
        discard;
    }

    // Light Map coordinates

    vec2 lightUV = vec2(
    ScreenPosition.x / uScreenSize.x,
    1.0 - ScreenPosition.y / uScreenSize.y
    );

    // Shared lighting

    vec3 lighting = texture(
        uLightmap,
        lightUV
    ).rgb;

    vec3 finalColor =
    GlyphColor * lighting;

    // Output

    FragColor = vec4(
    clamp(finalColor, 0.0, 1.0),
    alpha
    );
}
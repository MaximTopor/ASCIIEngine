#version 450 core

in vec2 TexCoord;
in vec3 GlyphColor;
in vec2 ScreenPosition;

out vec4 FragColor;

uniform sampler2D glyphTexture;

// Global ambient lighting
uniform vec3 uAmbientLight;

// Point lights
const int MAX_LIGHTS = 32;

uniform int uLightCount;

uniform vec4 uLightPositionRadius[MAX_LIGHTS];
uniform vec3 uLightColor[MAX_LIGHTS];

void main()
{
    // Glyph texture

    float alpha = texture(
        glyphTexture,
        TexCoord
    ).r;

    if (alpha < 0.5)
    {
        discard;
    }

    // Global lighting

    vec3 lighting = uAmbientLight;

    // Local point lights

    for (int i = 0; i < uLightCount; ++i)
    {
        vec2 lightPosition =
        uLightPositionRadius[i].xy;

        float radius =
        uLightPositionRadius[i].z;

        float intensity =
        uLightPositionRadius[i].w;

        float d = distance(
            ScreenPosition,
            lightPosition
        );

        if (radius <= 0.0 || d >= radius)
        {
            continue;
        }

        // Quadratic attenuation

        float attenuation =
        1.0 - d / radius;

        attenuation *= attenuation;

        vec3 contribution = uLightColor[i] * intensity * attenuation;

        lighting += contribution;
    }

    // Final illuminated glyph

    vec3 finalColor =
    GlyphColor * lighting;

    FragColor = vec4(
    clamp(finalColor, 0.0, 1.0),
    1.0
    );
}
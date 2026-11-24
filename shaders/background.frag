#version 450 core

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D uLightmap;

uniform float uBackgroundStrength;

void main()
{
    vec3 lighting = texture(
        uLightmap,
        TexCoord
    ).rgb;

    vec3 baseColor = vec3(
    0.05,
    0.08,
    0.06
    );

    vec3 finalColor =
    baseColor
    * lighting
    * uBackgroundStrength;

    FragColor = vec4(
    clamp(finalColor, 0.0, 1.0),
    1.0
    );
}
#version 450 core

// Per-vertex attributes
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

// Per-instance attributes
layout (location = 2) in vec2 iPosition;
layout (location = 3) in vec2 iSize;
layout (location = 4) in vec4 iUVRect;
layout (location = 5) in vec3 iColor;

out vec2 TexCoord;
out vec3 GlyphColor;

uniform vec2 uScreenSize;

void main()
{
    vec2 localPosition = vec2(
    aPos.x + 0.5,
    0.5 - aPos.y
    );

    vec2 pixelPosition =
    iPosition + localPosition * iSize;

    vec2 ndc = vec2(
    pixelPosition.x / uScreenSize.x * 2.0 - 1.0,
    1.0 - pixelPosition.y / uScreenSize.y * 2.0
    );

    gl_Position = vec4(ndc, 0.0, 1.0);

    TexCoord = mix(
        iUVRect.xy,
        iUVRect.zw,
        vec2(
        aTexCoord.x,
        1.0 - aTexCoord.y
        )
    );

    GlyphColor = iColor;
}
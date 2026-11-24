#version 450 core

// Per-vertex attributes
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

// Per-instance attributes
layout (location = 2) in vec2 iPosition;
layout (location = 3) in vec2 iSize;
layout (location = 4) in vec4 iUVRect;
layout (location = 5) in vec3 iColor;
layout (location = 6) in vec3 iAnimation;

out vec2 TexCoord;
out vec3 GlyphColor;
out vec2 ScreenPosition;

uniform vec2 uScreenSize;
uniform float uTime;

void main()
{
    vec2 localPosition = vec2(
    aPos.x + 0.5,
    0.5 - aPos.y
    );

    vec2 localPixel = localPosition * iSize;

    vec2 pixelPosition = iPosition + localPixel;

    float animationType = iAnimation.x;
    float phase = iAnimation.y;
    float amplitude = iAnimation.z;

    vec3 color = iColor;

    // Wind animation
    if (animationType == 1.0)
    {
        float angle =
        sin(uTime * 2.0 + phase)
        * amplitude;

        float s = sin(angle);
        float c = cos(angle);

        vec2 pivot = vec2(
        iSize.x * 0.5,
        iSize.y
        );

        vec2 relative = localPixel - pivot;

        mat2 rotation = mat2(
        c, s,
        -s, c
        );

        relative = rotation * relative;

        pixelPosition =
        iPosition + pivot + relative;
    }

    // Water animation
    else if (animationType == 2.0)
    {
        pixelPosition.x +=
        sin(uTime * 1.5 + phase)
        * amplitude
        * iSize.x;

        float brightness =
        0.85 + 0.15
        * sin(uTime * 2.0 + phase);

        color *= brightness;
    }

    // Screen coordinates -> NDC

    vec2 ndc = vec2(
    pixelPosition.x / uScreenSize.x
    * 2.0 - 1.0,

    1.0 - pixelPosition.y / uScreenSize.y
    * 2.0
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

    GlyphColor = color;
    ScreenPosition = pixelPosition;
}
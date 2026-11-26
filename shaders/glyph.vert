#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

layout(location = 2) in vec2 iPosition;
layout(location = 3) in vec2 iSize;
layout(location = 4) in vec4 iUVRect;
layout(location = 5) in vec3 iColor;
layout(location = 6) in vec3 iAnimation;
layout(location = 7) in vec4 iTransform;
layout(location = 8) in float iLightOffsetY;

out vec2 TexCoord;
out vec3 GlyphColor;
out vec2 ScreenPosition;
out float GlyphOpacity;

uniform vec2 uScreenSize;
uniform float uTime;

void main()
{
    // Local coordinates

    vec2 localPosition = vec2(
    aPos.x + 0.5,
    0.5 - aPos.y
    );

    vec2 localPixel =
    localPosition * iSize;

    // Animation parameters

    float animationType = iAnimation.x;
    float phase = iAnimation.y;
    float amplitude = iAnimation.z;

    // Transform parameters

    float rotation = iTransform.x;
    float opacity = iTransform.y;

    vec2 pivot =
    iTransform.zw * iSize;

    vec2 translation = vec2(0.0);

    vec3 color = iColor;

    // Wind animation

    if (animationType == 1.0)
    {
        rotation +=
        sin(uTime * 2.0 + phase)
        * amplitude;
    }

    // Water animation

    else if (animationType == 2.0)
    {
        translation.x +=
        sin(uTime * 1.5 + phase)
        * amplitude
        * iSize.x;

        float brightness =
        0.85
        + 0.15 * sin(
            uTime * 2.0 + phase
        );

        color *= brightness;
    }

    // Rotation

    float s = sin(rotation);
    float c = cos(rotation);

    mat2 rotationMatrix = mat2(
    c, s,
    -s, c
    );

    vec2 relativePosition =
    localPixel - pivot;

    relativePosition =
    rotationMatrix * relativePosition;

    // Final screen position

    vec2 pixelPosition =
    iPosition
    + pivot
    + relativePosition
    + translation;

    // Screen -> NDC

    vec2 ndc = vec2(
    pixelPosition.x / uScreenSize.x
    * 2.0 - 1.0,

    1.0 - pixelPosition.y / uScreenSize.y
    * 2.0
    );

    gl_Position = vec4(
    ndc,
    0.0,
    1.0
    );

    // Texture coordinates

    TexCoord = mix(
        iUVRect.xy,
        iUVRect.zw,
        vec2(
        aTexCoord.x,
        1.0 - aTexCoord.y
        )
    );

    GlyphColor = color;

    GlyphOpacity = opacity;

    ScreenPosition = pixelPosition + vec2(0.0, iLightOffsetY);
}

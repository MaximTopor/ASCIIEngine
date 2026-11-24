#version 450 core

in vec2 TexCoord;
in vec3 GlyphColor;

out vec4 FragColor;

uniform sampler2D glyphTexture;

void main()
{
    float alpha = texture(
        glyphTexture,
        TexCoord
    ).r;

    if (alpha < 0.5)
    {
        discard;
    }

    FragColor = vec4(
    GlyphColor,
    1.0
    );
}
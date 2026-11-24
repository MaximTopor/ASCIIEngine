#version 450 core

in vec2 TexCoord;

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
    0.3, 0.7, 0.4, 1.0
    );
}
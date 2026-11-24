#version 450 core

out vec2 TexCoord;
out vec2 ScreenPosition;

uniform vec2 uScreenSize;

void main()
{
    vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 3.0, -1.0),
    vec2(-1.0,  3.0)
    );

    vec2 position = positions[gl_VertexID];

    gl_Position = vec4(
    position,
    0.0,
    1.0
    );

    TexCoord = position * 0.5 + 0.5;

    ScreenPosition = vec2(
    TexCoord.x * uScreenSize.x,
    (1.0 - TexCoord.y) * uScreenSize.y
    );
}
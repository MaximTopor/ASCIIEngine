#version 450 core


in vec2 ScreenPosition;

out vec4 FragColor;

// Textures


uniform sampler2D uOcclusionMap;

// Lighting

uniform vec3 uAmbientLight;

const int MAX_LIGHTS = 32;

uniform int uLightCount;

uniform vec4 uLightPositionRadius[MAX_LIGHTS];
uniform vec3 uLightColor[MAX_LIGHTS];

uniform float uLightSourceRadius[MAX_LIGHTS];

// Shadows

uniform int uShadowsEnabled;

// Camera

uniform vec2 uWorldOrigin;
uniform vec2 uCellSize;
uniform vec2 uWorldPerScreenX;
uniform vec2 uWorldPerScreenY;


// Screen -> Tile coordinates

vec2 screenToTile(vec2 screenPosition)
{
    vec2 worldPosition =
    uWorldOrigin
    + screenPosition.x * uWorldPerScreenX
    + screenPosition.y * uWorldPerScreenY;

    return worldPosition / uCellSize;
}


// Read material opacity

float getOpacity(ivec2 cell)
{
    ivec2 mapSize =
    textureSize(uOcclusionMap, 0);

    if (cell.x < 0 ||
    cell.y < 0 ||
    cell.x >= mapSize.x ||
    cell.y >= mapSize.y)
    {
        return 0.0;
    }

    return texelFetch(
        uOcclusionMap,
        cell,
        0
    ).r;
}


// DDA ray transmission

float traceRay(
    vec2 lightScreenPosition,
    vec2 fragmentScreenPosition
)
{
    vec2 start =
    screenToTile(lightScreenPosition);

    vec2 end =
    screenToTile(fragmentScreenPosition);

    vec2 direction = end - start;

    ivec2 cell = ivec2(floor(start));

    ivec2 target = ivec2(floor(end));

    if (all(equal(cell, target)))
    {
        return 1.0;
    }

    ivec2 stepDirection =
    ivec2(sign(direction));

    const float INF = 1e30;

    vec2 tDelta = vec2(INF);
    vec2 tMax = vec2(INF);

    // X axis

    if (direction.x != 0.0)
    {
        tDelta.x =
        abs(1.0 / direction.x);

        float boundary =
        float(cell.x)
        + (direction.x > 0.0 ? 1.0 : 0.0);

        tMax.x =
        (boundary - start.x)
        / direction.x;
    }

    // Y axis

    if (direction.y != 0.0)
    {
        tDelta.y =
        abs(1.0 / direction.y);

        float boundary =
        float(cell.y)
        + (direction.y > 0.0 ? 1.0 : 0.0);

        tMax.y =
        (boundary - start.y)
        / direction.y;
    }

    ivec2 difference =
    abs(target - cell);

    int maxSteps =
    difference.x + difference.y + 1;

    float transmission = 1.0;

    // Grid traversal

    for (int i = 0; i < maxSteps; ++i)
    {
        if (tMax.x < tMax.y)
        {
            cell.x += stepDirection.x;
            tMax.x += tDelta.x;
        }
        else if (tMax.y < tMax.x)
        {
            cell.y += stepDirection.y;
            tMax.y += tDelta.y;
        }
        else
        {
            cell += stepDirection;
            tMax += tDelta;
        }

        // Do not block light with target cell

        if (all(equal(cell, target)))
        {
            return transmission;
        }

        float opacity = getOpacity(cell);

        transmission *= 1.0 - opacity;

        // Early termination

        if (transmission < 0.005)
        {
            return 0.0;
        }
    }

    return transmission;
}


// Soft shadows

float calculateVisibility(
    vec2 lightPosition,
    vec2 fragmentPosition,
    float sourceRadius
)
{
    if (uShadowsEnabled == 0)
    {
        return 1.0;
    }

    vec2 direction =
    fragmentPosition - lightPosition;

    float d = length(direction);

    if (d < 0.001)
    {
        return 1.0;
    }

    // Hard shadow fallback

    if (sourceRadius <= 0.0)
    {
        return traceRay(
            lightPosition,
            fragmentPosition
        );
    }

    // Direction perpendicular to the ray

    vec2 tangent = vec2(
    -direction.y,
    direction.x
    ) / d;

    float visibility = 0.0;

    const int SAMPLE_COUNT = 9;

    for (int i = 0; i < SAMPLE_COUNT; ++i)
    {
        float offset =
        (float(i) / float(SAMPLE_COUNT - 1))
        * 2.0 - 1.0;

        vec2 samplePosition =
        lightPosition
        + tangent * offset * sourceRadius;

        visibility += traceRay(
            samplePosition,
            fragmentPosition
        );
    }

    return visibility / float(SAMPLE_COUNT);
}


void main()
{
    vec3 lighting = uAmbientLight;
    float primaryVisibility = 1.0;

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

        float attenuation =
        1.0 - d / radius;

        attenuation *= attenuation;

        float visibility =
        calculateVisibility(
            lightPosition,
            ScreenPosition,
            uLightSourceRadius[i]
        );
        if (i == 0)
        {
            primaryVisibility = visibility;
        }

        lighting +=
        uLightColor[i]
        * intensity
        * attenuation
        * visibility;
    }

    FragColor = vec4(lighting, primaryVisibility);
}

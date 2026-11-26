#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

layout(location = 2) in vec2 iPosition;
layout(location = 3) in vec2 iSize;
layout(location = 4) in vec4 iUVRect;
layout(location = 5) in vec3 iColor;
layout(location = 6) in vec3 iAnimation;
layout(location = 7) in vec4 iTransform;
layout(location = 8) in vec3 iLightingAndDepth;
layout(location = 9) in vec4 iWorldAnchor;
layout(location = 10) in uvec2 iSurface;

out vec2 TexCoord;
out vec3 GlyphColor;
out vec2 ScreenPosition;
out float GlyphOpacity;

flat out float Solid;
flat out uint SurfaceMode;
flat out vec3 SurfaceColor;
flat out vec3 ShadowedPoint;

// Screen parameters

uniform vec2 uScreenSize;

uniform float uTime;

uniform int uCoveragePass;

// Camera parameters

uniform vec2 uCameraPosition;

// x = cos(azimuth)
// y = sin(azimuth)
// z = sin(elevation)
// w = cos(elevation)

uniform vec4 uCameraBasis;

uniform float uCameraZoom;
uniform vec3 uGrassWind; // normalized world XY direction, strength
uniform vec3 uGrassPlayer; // world XY, interaction radius
uniform vec4 uGrassTrail[6]; // world XY, radius, recovery weight

uniform vec4 uWaterTrail[6];

// Water configuration.

const float WATER_FLOW_SPEED = 18.0;

const float WATER_WAVE_HEIGHT = 0.65;

const float WATER_RIPPLE_SPEED = 22.0;

const float WATER_RIPPLE_HEIGHT = 1.20;

vec2 grassPush(vec2 root, vec2 center, float radius, float weight, float phase)
{
    if (radius <= 0.0 || weight <= 0.0) return vec2(0.0);
    vec2 d = root-center;
    float dist2=dot(d,d);
    if(dist2 >= radius*radius) return vec2(0.0);
    float dist=sqrt(dist2);
    vec2 direction=dist>0.001 ? d/dist : vec2(cos(phase),sin(phase));
    return direction*(1.0-smoothstep(0.0,radius,dist))*weight;
}

uniform vec3 uSurfaceSky;
uniform vec3 uSurfaceGround;
uniform vec3 uSunDirection;
uniform vec3 uMoonDirection;
uniform vec3 uSunRadiance;
uniform vec3 uMoonRadiance;
uniform vec4 uSurfacePoint[4];
uniform vec3 uSurfaceRadiance[4];
uniform int uSurfacePointCount;
uniform int uLightingDebug;

vec3 incident(vec3 n, vec3 v, vec3 l, vec3 radiance, float power,
              float reflectivity, bool foliage, out vec3 specular)
{
    float nl = dot(n,l);
    float diffuse = foliage ? max(0.0, (nl + 0.32) / 1.32) + 0.18 * max(0.0,-nl)
                            : max(0.0,nl);
    vec3 halfway = l + v;
    float nh = max(0.0,dot(n,halfway) * inversesqrt(max(dot(halfway,halfway),1e-5)));
    specular = radiance * (reflectivity * max(0.0,nl) * pow(nh,power));
    return diffuse * radiance;
}

void shadeSurface(vec3 albedo)
{
    SurfaceMode = iSurface.x >> 30u;
    SurfaceColor = albedo;
    ShadowedPoint = vec3(0.0);
    if (SurfaceMode == 0u || uLightingDebug == 1 || uLightingDebug == 10)
        return;

    vec3 encodedNormal = vec3(uvec3(iSurface.x, iSurface.x >> 10u, iSurface.x >> 20u) & 1023u);
    vec3 n = normalize(encodedNormal * (2.0 / 1023.0) - 1.0);
    vec4 material = unpackUnorm4x8(iSurface.y);
    if (uLightingDebug == 2 || uLightingDebug == 8 || uLightingDebug == 9)
    {
        SurfaceColor = uLightingDebug == 2 ? n * 0.5 + 0.5
            : vec3(uLightingDebug == 8 ? material.x : material.y);
        return;
    }

    vec3 view = vec3(uCameraBasis.y * uCameraBasis.w, uCameraBasis.x * uCameraBasis.w, uCameraBasis.z);
    float sky = n.z * 0.5 + 0.5;
    bool foliage = SurfaceMode == 2u;
    if (foliage) sky = mix(sky,0.65,0.42);
    vec3 ambient = albedo * mix(uSurfaceGround,uSurfaceSky,sky) * material.z;
    if (uLightingDebug == 3)
    {
        SurfaceColor = ambient;
        return;
    }

    float smoothness = 1.0 - material.x;
    float power = mix(5.0,112.0,smoothness*smoothness);
    float reflection = material.y * mix(0.18,0.97,smoothness);
    vec3 directionalSpecular = vec3(0.0);
    vec3 sunSpec, moonSpec;
    vec3 sun = incident(n,view,uSunDirection,uSunRadiance,power,reflection,foliage,sunSpec);
    vec3 moon = incident(n,view,uMoonDirection,uMoonRadiance,power,reflection,foliage,moonSpec);
    vec3 directionalDiffuse = albedo * (sun + moon);
    directionalSpecular = sunSpec + moonSpec;
    if (uLightingDebug == 4)
    {
        SurfaceColor = directionalDiffuse;
        return;
    }
    SurfaceColor = uLightingDebug == 5 ? vec3(0.0) : (uLightingDebug == 6
        ? directionalSpecular : ambient + directionalDiffuse + directionalSpecular);
    for (int i = 0; i < uSurfacePointCount; ++i)
    {
        vec3 delta = uSurfacePoint[i].xyz - vec3(iPosition,iWorldAnchor.z);
        float d2 = dot(delta,delta);
        float radius = uSurfacePoint[i].w;
        if (radius <= 0.0 || d2 >= radius*radius) continue;
        float distance = sqrt(d2);
        float fade = 1.0 - distance/radius;
        vec3 specular;
        vec3 diffuse = incident(n,view,delta * inversesqrt(max(d2,1e-5)),
                               uSurfaceRadiance[i] * (fade*fade),power,reflection,foliage,specular);
        vec3 contribution = uLightingDebug == 6 ? specular : diffuse * albedo + specular;
        if (i == 0) ShadowedPoint = contribution;
        else SurfaceColor += contribution;
    }
}

void main()
{
    // Skip non-solid instances
    // during the opaque coverage pass.

    if (
    uCoveragePass == 1 &&
    iLightingAndDepth.z < 0.5
    )
    {
        gl_Position =
        vec4(2.0, 2.0, 2.0, 1.0);

        return;
    }

    vec2 position = iPosition;

    vec2 size = iSize;

    float lightOffsetY =
    iLightingAndDepth.x;

    // ====================================
    // GPU WORLD-SPACE PROJECTION
    // ====================================

    if (iWorldAnchor.w > 0.5)
    {
        vec2 d =
        iPosition - uCameraPosition;

        float cameraCos =
        uCameraBasis.x;

        float cameraSin =
        uCameraBasis.y;

        float rise =
        uCameraBasis.z;

        float lift =
        uCameraBasis.w;

        float worldZ =
        iWorldAnchor.z;

        // Oblique orthographic projection.

        vec2 projected = vec2(
        cameraCos * d.x
        - cameraSin * d.y,

        rise * (
        cameraSin * d.x
        + cameraCos * d.y
        )
        - lift * worldZ
        );

        position =
        uScreenSize * 0.5
        + projected * uCameraZoom;

        // Apply glyph anchor offset.

        position +=
        iWorldAnchor.xy
        * uCameraZoom;

        // Apply camera zoom.

        size *= uCameraZoom;

        // Preserve existing ground-plane
        // Light Map sampling.

        lightOffsetY =
        lift * worldZ
        * uCameraZoom;
    }

    // ====================================
    // LOCAL GLYPH COORDINATES
    // ====================================

    vec2 localPosition = vec2(
    aPos.x + 0.5,
    0.5 - aPos.y
    );

    vec2 localPixel =
    localPosition * size;

    // ====================================
    // ANIMATION
    // ====================================

    float animationType =
    iAnimation.x;

    float phase =
    iAnimation.y;

    float amplitude =
    iAnimation.z;

    // Transform parameters.

    float rotation =
    iTransform.x;

    float opacity =
    iTransform.y;

    vec2 pivot =
    iTransform.zw * size;

    vec2 translation =
    vec2(0.0);

    vec3 color =
    iColor;

    // Wind animation.

    if (animationType == 1.0)
    {
        rotation +=
        sin(uTime * 2.0 + phase)
        * amplitude;
    }


    // ====================================
    // GLYPH ROTATION
    // ====================================

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

    // ====================================
    // FINAL SCREEN POSITION
    // ====================================

    vec2 pixelPosition =
    position
    + pivot
    + relativePosition
    + translation;


    // ============================================
    // WORLD-SPACE WATER SURFACE
    // ============================================

    bool water =
    animationType == 2.0
    && iWorldAnchor.w > 0.5;

    if (water)
    {
        vec2 root = iPosition;

        bool detailLayer =
        amplitude > 0.5;

        // World-space direction of the current.

        vec2 flowDirection = vec2(
        cos(phase),
        sin(phase)
        );

        vec2 crossDirection = vec2(
        -flowDirection.y,
        flowDirection.x
        );

        // Lay the glyph on the horizontal
        // water surface instead of using
        // a vertical camera-facing billboard.

        vec2 local =
        localPosition - vec2(0.5);

        float glyphWidth =
        iSize.x;

        float glyphLength =
        detailLayer
        ? glyphWidth * 0.65
        : glyphWidth;

        vec2 surfacePosition =
        root
        + crossDirection * local.x * glyphWidth
        + flowDirection * local.y * glyphLength;

        // ========================================
        // DIRECTIONAL FLOW
        // ========================================

        float along =
        dot(root, flowDirection);

        float across =
        dot(root, crossDirection);

        float movingPosition =
        along - uTime * WATER_FLOW_SPEED;

        float waveA = sin(
            movingPosition * 0.24
            + across * 0.095
        );

        float waveB = sin(
            movingPosition * 0.11
            - across * 0.17
            + 1.8
        );

        float waveC = sin(
            movingPosition * 0.43
            + across * 0.21
        );

        float wave =
        waveA * 0.55
        + waveB * 0.30
        + waveC * 0.15;

        float surfaceHeight =
        WATER_WAVE_HEIGHT * wave;

        // ========================================
        // PLAYER INTERACTION AND RIPPLE WAVES
        // ========================================

        float rippleHeight = 0.0;

        float rippleFoam = 0.0;

        for (int i = 0; i < 6; ++i)
        {
            vec4 footprint =
            uWaterTrail[i];

            float age = footprint.z;

            float strength = footprint.w;

            if (strength <= 0.0)
            {
                continue;
            }

            // Ripples are slowly carried downstream.

            vec2 rippleCenter =
            footprint.xy
            + flowDirection * age * 6.0;

            float distanceToStep =
            length(root - rippleCenter);

            float rippleRadius =
            age * WATER_RIPPLE_SPEED;

            float edgeDistance =
            distanceToStep - rippleRadius;

            float envelope =
            exp(
                -edgeDistance
                * edgeDistance
                * 0.10
            );

            float oscillation =
            cos(edgeDistance * 1.10);

            rippleHeight +=
            oscillation
            * envelope
            * strength
            * WATER_RIPPLE_HEIGHT;

            rippleFoam +=
            envelope
            * strength
            * max(0.0, oscillation);
        }

        surfaceHeight += rippleHeight;

        surfaceHeight =
        clamp(surfaceHeight, -2.0, 2.0);

        rippleFoam =
        clamp(rippleFoam, 0.0, 1.0);

        // ========================================
        // WATER COLOR
        // ========================================

        color *=
        0.92 + wave * 0.12;

        if (detailLayer)
        {
            float crest =
            smoothstep(
                0.15,
                0.85,
                wave
            );

            float highlight =
            crest * 0.45
            + rippleFoam * 0.75;

            color = mix(
                color,
                vec3(0.67, 0.83, 0.85),
                clamp(highlight, 0.0, 0.85)
            );

            opacity *=
            clamp(
                0.30
                + crest * 0.55
                + rippleFoam * 0.65,
                0.0,
                1.0
            );
        }
        else
        {
            color = mix(
                color,
                vec3(0.42, 0.62, 0.65),
                rippleFoam * 0.22
            );
        }

        // ========================================
        // WORLD-SPACE PROJECTION
        // ========================================

        vec2 delta =
        surfacePosition - uCameraPosition;

        pixelPosition =
        uScreenSize * 0.5
        + uCameraZoom * vec2(
        uCameraBasis.x * delta.x
        - uCameraBasis.y * delta.y,

        uCameraBasis.z * (
        uCameraBasis.y * delta.x
        + uCameraBasis.x * delta.y
        )
        - uCameraBasis.w * surfaceHeight
        );

        lightOffsetY =
        uCameraBasis.w
        * surfaceHeight
        * uCameraZoom;
    }


    // Grass is rooted in the world, not rotated as a screen-space sprite.
    bool grass = animationType == 3.0 && iWorldAnchor.w > 0.5;
    vec3 grassPoint = vec3(iPosition,iWorldAnchor.z);
    if (grass)
    {
        float t = 1.0-localPosition.y;
        float h = iSize.y;
        vec2 root = iPosition;
        float windStrength = uGrassWind.z;
        float wave = 0.68+0.32*sin(dot(root,uGrassWind.xy)*0.075-uTime*2.1);
        vec2 bend = uGrassWind.xy*windStrength*wave*0.85;
        bend += vec2(sin(phase+uTime*1.3),cos(phase+uTime*1.1))*0.09;
        vec2 push = grassPush(root,uGrassPlayer.xy,uGrassPlayer.z,1.0,phase);
        for(int i=0;i<6;++i)
        {
            vec2 trail = grassPush(root,uGrassTrail[i].xy,uGrassTrail[i].z,uGrassTrail[i].w,phase);
            if(dot(trail,trail)>dot(push,push)) push=trail;
        }
        bend += push*1.35;
        float amount=length(bend);
        if(amount>1.45) bend*=1.45/amount;
        // Retain the world-space root and lower the tip when flattened.
        float compression=clamp(length(bend)*0.43,0.0,0.68);
        grassPoint.xy += bend*h*t*t;
        grassPoint.z += h*t*(1.0-compression*t);
        vec2 delta=grassPoint.xy-uCameraPosition;
        float sideways=(localPosition.x-0.5)*iSize.x*amplitude*1.15;
        pixelPosition=uScreenSize*0.5+uCameraZoom*vec2(
            uCameraBasis.x*delta.x-uCameraBasis.y*delta.y+sideways,
            uCameraBasis.z*(uCameraBasis.y*delta.x+uCameraBasis.x*delta.y)-uCameraBasis.w*grassPoint.z);
        lightOffsetY=uCameraBasis.w*grassPoint.z*uCameraZoom;
        color *= mix(0.62,1.13,t);
    }

    // Screen -> NDC.

    vec2 ndc = vec2(
    pixelPosition.x
    / uScreenSize.x
    * 2.0 - 1.0,

    1.0
    - pixelPosition.y
    / uScreenSize.y
    * 2.0
    );

    // Preserve existing depth.

    // ========================================
    // GPU WORLD-SPACE DEPTH
    // ========================================

    float clipDepth = iLightingAndDepth.y;

    if (
    iWorldAnchor.w > 0.5
    && uCoveragePass != 0
    )
    {
        vec2 delta =
        (grass ? grassPoint.xy : iPosition) - uCameraPosition;

        float depth =
        uCameraBasis.w * (
        uCameraBasis.y * delta.x
        + uCameraBasis.x * delta.y
        )
        + uCameraBasis.z * (grass ? grassPoint.z : iWorldAnchor.z);

        // Larger world-space depth means
        // closer to the camera.
        //
        // Shared depth range for all objects.

        const float DEPTH_RANGE = 32768.0;

        clipDepth = clamp(
            -depth / DEPTH_RANGE,
            -0.999,
            0.999
        );
    }

    gl_Position = vec4(
    ndc,

    clipDepth,

    1.0
    );

    // ====================================
    // FRAGMENT SHADER OUTPUT
    // ====================================

    TexCoord = mix(
        iUVRect.xy,
        iUVRect.zw,
        vec2(
        aTexCoord.x,
        1.0 - aTexCoord.y
        )
    );

    GlyphColor = color;
    shadeSurface(color);

    GlyphOpacity = opacity;

    ScreenPosition =
    pixelPosition
    + vec2(0.0, lightOffsetY);

    Solid = iLightingAndDepth.z;
}

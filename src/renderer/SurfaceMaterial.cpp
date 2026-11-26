#include "renderer/SurfaceMaterial.hpp"

#include <algorithm>
#include <cmath>

VisualGlyph SurfaceMaterial::glyph(const SurfaceSampler::Sample& sample, float pattern, float exposure) const
{
    const float t = std::clamp(pattern * detail,0.0f,1.0f);
    const float grain = 1.0f + (sample.variation-0.5f)*variation;
    const auto channel = [=](float a,float b,float low,float high)
    {
        const float tint = std::lerp(a,b,t);
        // The dark and bright palette colors describe pigment variation, not light.
        return std::clamp((tint + (high-low)*(sample.variation-0.5f)*0.12f)*grain,0.0f,1.0f);
    };
    VisualGlyph result;
    const auto ink = palette.empty() ? std::u32string_view{U"#"} : palette;
    const auto index = std::min(ink.size()-1,
        static_cast<std::size_t>(std::clamp(sample.variation,0.0f,1.0f) * ink.size()));
    result.character = ink[index];
    result.x = sample.position.x;
    result.y = sample.position.y;
    result.z = sample.position.z;
    result.color = {channel(base.r,secondary.r,shadow.r,highlight.r),
                    channel(base.g,secondary.g,shadow.g,highlight.g),
                    channel(base.b,secondary.b,shadow.b,highlight.b)};
    result.material = {sample.normal.x,sample.normal.y,sample.normal.z,roughness,reflectivity,
                       exposure,LightingMode::Surface};
    // Use nominal ink width rather than the atlas em; narrow detail palettes can tune coverage separately.
    result.scale = sample.spacing * coverage / 36.0f;
    result.transform.rotation = (sample.variation-0.5f)*rotation;
    result.transform.pivotY = 0.5f;
    return result;
}

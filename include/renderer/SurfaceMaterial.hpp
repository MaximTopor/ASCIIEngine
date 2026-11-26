#pragma once

#include "renderer/SurfaceSampler.hpp"
#include "renderer/VisualGlyph.hpp"

#include <string_view>

struct SurfaceMaterial
{
    Color base, secondary, shadow, highlight;
    std::u32string_view palette;
    float density, coverage, rotation, variation, detail;
    float roughness = 1.0f;
    float reflectivity = 0.0f;

    float spacing(float target) const { return target / density; }
    VisualGlyph glyph(const SurfaceSampler::Sample& sample, float pattern, float exposure) const;
};

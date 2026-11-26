#include "renderer/TreeRenderer.hpp"
#include "renderer/SurfaceMaterial.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace
{
    using namespace SurfaceSampler;
    constexpr float tau = 2.0f*std::numbers::pi_v<float>;
    constexpr std::array spacing{4.7f, 7.6f, 12.0f};
    constexpr float highPixels = 520.0f;
    constexpr float mediumPixels = 225.0f;
    constexpr float hysteresis = 0.12f;

    constexpr SurfaceMaterial bark{{0.37f,0.26f,0.17f},{0.23f,0.17f,0.12f},
        {0.13f,0.09f,0.06f},{0.34f,0.24f,0.14f},U"#%8H##",1.0f,1.62f,0.25f,0.12f,0.80f,0.90f,0.03f};
    constexpr SurfaceMaterial birchBark{{0.64f,0.60f,0.49f},{0.24f,0.21f,0.17f},
        {0.22f,0.20f,0.16f},{0.49f,0.47f,0.43f},U"#8H%=#",0.95f,1.60f,0.20f,0.10f,1.0f,0.85f,0.04f};
    constexpr SurfaceMaterial freshWood{{0.49f,0.36f,0.21f},{0.29f,0.20f,0.12f},
        {0.18f,0.12f,0.075f},{0.41f,0.29f,0.17f},U"#8%H",0.9f,1.50f,0.27f,0.13f,0.80f,0.83f,0.04f};
    constexpr SurfaceMaterial rootWood{{0.27f,0.20f,0.13f},{0.19f,0.16f,0.10f},
        {0.10f,0.08f,0.05f},{0.27f,0.19f,0.11f},U"#%H8",1.0f,1.55f,0.28f,0.15f,0.85f,0.94f,0.02f};
    constexpr SurfaceMaterial branchWood{{0.33f,0.24f,0.15f},{0.23f,0.17f,0.11f},
        {0.12f,0.08f,0.05f},{0.32f,0.22f,0.12f},U"#%8H",0.95f,1.46f,0.26f,0.12f,0.78f,0.88f,0.04f};
    constexpr SurfaceMaterial oakLeaves{{0.28f,0.43f,0.15f},{0.38f,0.46f,0.17f},
        {0.09f,0.19f,0.07f},{0.28f,0.37f,0.12f},U"*%&xo+",1.0f,1.32f,1.8f,0.28f,0.85f};
    constexpr SurfaceMaterial birchLeaves{{0.36f,0.49f,0.16f},{0.49f,0.55f,0.21f},
        {0.12f,0.23f,0.09f},{0.31f,0.42f,0.13f},U"*+%xo",0.95f,1.26f,2.2f,0.32f,0.8f};
    constexpr SurfaceMaterial pineLeaves{{0.17f,0.33f,0.15f},{0.26f,0.37f,0.17f},
        {0.07f,0.16f,0.09f},{0.19f,0.30f,0.15f},U"*!/\\x%",0.9f,1.40f,1.2f,0.22f,0.8f};
    constexpr SurfaceMaterial moss{{0.23f,0.34f,0.12f},{0.31f,0.39f,0.16f},
        {0.09f,0.16f,0.06f},{0.23f,0.30f,0.10f},U"%#o:",1.0f,1.47f,0.6f,0.19f,0.8f,0.98f,0.01f};

    struct Species
    {
        float lean, spread, crownStart, clusterWidth, clusterHeight;
        int branches, roots;
        const SurfaceMaterial& trunk;
        const SurfaceMaterial& leaves;
    };

    Species species(TreeSpecies type)
    {
        switch (type)
        {
            case TreeSpecies::Birch: return {0.073f,0.22f,0.42f,0.054f,0.059f,9,4,birchBark,birchLeaves};
            case TreeSpecies::Pine: return {0.020f,0.29f,0.27f,0.063f,0.027f,15,5,bark,pineLeaves};
            case TreeSpecies::Oak: return {0.075f,0.33f,0.29f,0.077f,0.061f,11,7,bark,oakLeaves};
        }
        return {0.075f,0.33f,0.29f,0.077f,0.061f,11,7,bark,oakLeaves};
    }

    float random(std::uint32_t& state)
    {
        state = state*1664525u+1013904223u;
        return static_cast<float>(state >> 8)/16777216.0f;
    }

    Vec radial(float angle) { return {std::cos(angle),std::sin(angle),0}; }
}

std::vector<VisualGlyph> TreeRenderer::generate(const Tree& tree, int level)
{
    level = std::clamp(level,0,2);
    const auto shape = species(tree.species);
    const float h = tree.height;
    const float radius = h*treeBaseRadiusRatio(tree.species);
    const Vec origin{tree.x,tree.y,0};
    const Vec up{0,0,1};
    auto seed = tree.seed;
    const float rotation = random(seed)*tau;
    const Vec lean = radial(rotation)*(h*shape.lean);
    const Vec sway = radial(rotation+1.4f)*(h*shape.lean*0.33f);
    const auto trunk = [&](float t)
    {
        return origin + up*(t*h) + lean*(t*t) + sway*std::sin(t*3.14159265f);
    };
    std::vector<VisualGlyph> result;

    const auto woody = [&](std::span<const TubePoint> path, const SurfaceMaterial& material,
                           std::uint32_t sampleSeed, bool structuralTrunk)
    {
        const float phase = static_cast<float>(sampleSeed % 314u)*0.02f;
        const auto ridges = [=](float u,float v)
        {
            return 1.0f + 0.074f*std::sin(tau*(3*u+0.24f*v)+phase)
                + 0.040f*std::cos(tau*(9*u-0.12f*v)+phase)
                + 0.027f*std::sin(tau*(15*u+0.32f*v)+phase);
        };
        tube(path,{material.spacing(spacing[level]),0.20f,sampleSeed},
            [&](const Sample& sample)
            {
                if (sample.position.z < 0.7f) return;
                const float furrow = std::sin(tau*(11*sample.u+0.29f*sample.v)+phase);
                const float grain = 0.5f + 0.39f*furrow + 0.11f*std::cos(35*sample.v+phase);
                const float patch = std::sin(sample.position.x*0.04f+phase)
                    * std::cos(sample.position.y*0.047f+sample.position.z*0.035f);
                const bool mossy = patch > 0.33f && sample.position.z < h*0.12f;
                const float exposure = 0.72f + 0.28f*std::clamp(sample.position.z/(h*0.24f),0.0f,1.0f);
                const float pattern = structuralTrunk && tree.species == TreeSpecies::Birch
                    ? std::clamp(grain * (0.52f+0.52f*std::cos(82*sample.v+phase)),0.0f,1.0f)
                    : grain;
                result.push_back((mossy ? moss : material).glyph(sample,pattern,exposure));
                if (structuralTrunk && level == 0 && furrow < -0.84f && sample.variation > 0.62f)
                {
                    auto groove = bark.glyph(sample,0.96f,exposure*0.73f);
                    groove.character = U'|';
                    groove.x += sample.normal.x*0.29f;
                    groove.y += sample.normal.y*0.29f;
                    groove.z += sample.normal.z*0.29f;
                    groove.scale *= 0.73f;
                    result.push_back(groove);
                }
            },ridges);
    };

    const std::array stem{
        TubePoint{trunk(0.0f),radius},TubePoint{trunk(0.085f),radius*0.73f},
        TubePoint{trunk(0.33f),radius*0.56f},TubePoint{trunk(0.60f),radius*0.38f},
        TubePoint{trunk(0.82f),radius*0.22f},TubePoint{trunk(0.98f),radius*0.035f}
    };
    woody(stem,shape.trunk,tree.seed,true);

    for (int i=0;i<shape.roots;++i)
    {
        const float angle = rotation + i*tau/shape.roots + (random(seed)-0.5f)*0.48f;
        const float run = radius*(1.8f+2.05f*random(seed));
        const Vec direction = radial(angle);
        const std::array root{
            TubePoint{origin+direction*(radius*0.41f)+up*(radius*0.56f),radius*0.43f},
            TubePoint{origin+direction*(run*0.42f)+up*(radius*0.19f),radius*0.27f},
            TubePoint{origin+radial(angle+0.19f)*(run*0.73f),radius*0.13f},
            TubePoint{origin+radial(angle+0.32f)*run-up*(radius*0.075f),radius*0.025f}};
        woody(root,rootWood,seed,false);
    }

    const auto foliage = [&](Vec center, Vec radii, std::uint32_t clusterSeed, float phase)
    {
        const float leafSpacing = shape.leaves.spacing(spacing[level]*1.32f);
        const auto patchAt = [phase](float u, float v)
        {
            return std::sin(tau*u*3.0f+phase)*std::cos(v*21.0f+phase);
        };
        for (const float shell : {1.0f,0.66f})
        {
            ellipsoid(center,radii*shell,{leafSpacing,0.46f,clusterSeed+(shell<1.0f ? 31u : 0u)},
                [&](const Sample& sample)
                {
                    const float patch = patchAt(sample.u,sample.v);
                    if (patch < -0.55f && sample.variation < 0.74f) return;
                    const Sample& leaf = sample;
                    const float exposure = (0.65f+0.35f*shell)
                        * (0.76f+0.24f*std::clamp(leaf.position.z/h,0.0f,1.0f));
                    auto glyph = shape.leaves.glyph(leaf,0.5f+0.43f*patch,exposure);
                    glyph.material.mode = LightingMode::Foliage;
                    glyph.material.roughness = 0.65f;
                    glyph.material.reflectivity = 0.06f;
                    glyph.animation = {AnimationType::Wind,
                        leaf.position.x*0.014f+leaf.position.y*0.012f,0.11f+0.14f*shell};
                    result.push_back(glyph);
                },[&](float u,float v) { return 1.0f+0.18f*patchAt(u,v); });
        }
    };

    for (int i=0;i<shape.branches;++i)
    {
        const float tier = static_cast<float>(i)/(shape.branches-1);
        const float t = std::lerp(shape.crownStart,0.86f,tier) + (random(seed)-0.5f)*0.055f;
        const float angle = rotation + i*2.399963f + (random(seed)-0.5f)*0.67f;
        const float reach = h*shape.spread*(0.35f+0.65f*(1-tier))*(0.79f+0.27f*random(seed));
        const float rise = h*(tree.species == TreeSpecies::Pine ? 0.02f : 0.105f)*(1.0f-0.37f*tier);
        const float joint = radius*(0.32f-0.20f*tier)*(0.8f+0.35f*random(seed));
        const Vec start = trunk(t);
        const Vec direction = radial(angle);
        const Vec bend = start + direction*(reach*0.53f)+up*(rise*0.52f);
        const Vec end = start + radial(angle+0.09f)*reach+up*rise;
        const std::array primary{TubePoint{start,joint},TubePoint{bend,joint*0.63f},
            TubePoint{end,joint*0.13f}};
        woody(primary,tree.species == TreeSpecies::Birch ? shape.trunk : branchWood,seed,false);

        for (int fork=0;fork<3;++fork)
        {
            const float directionOffset = (fork-1)*0.88f+(random(seed)-0.5f)*0.24f;
            const float length = reach*(0.23f+0.20f*random(seed));
            const Vec base = bend + (end-bend)*(0.15f+fork*0.28f);
            const Vec arm = radial(angle+directionOffset)*length;
            const Vec tip = base+arm+up*(h*(0.04f+0.04f*random(seed)));
            const auto forkSeed = seed;
            const float fullness = 0.74f+0.52f*random(seed);
            const std::array secondary{TubePoint{base,joint*0.36f},
                TubePoint{base+arm*0.49f+(tip-base-arm)*0.48f,joint*0.20f},
                TubePoint{tip,joint*0.055f}};
            woody(secondary,branchWood,forkSeed,false);
            const Vec twigTip = tip+radial(angle+directionOffset+0.69f)*(length*0.38f)+up*(h*0.027f);
            if (level < 2)
            {
                const std::array twig{TubePoint{tip-(tip-base)*0.2f,joint*0.12f},TubePoint{twigTip,0.22f}};
                woody(twig,freshWood,forkSeed+17u,false);
            }
            const Vec radii{h*shape.clusterWidth*fullness,h*shape.clusterWidth*(0.72f+0.20f*fullness),
                            h*shape.clusterHeight*fullness};
            foliage(tip,radii,forkSeed,angle);
            foliage(twigTip,radii*0.52f,forkSeed+53u,angle+0.8f);
        }
    }
    foliage(trunk(0.93f),{h*shape.clusterWidth,h*shape.clusterWidth*0.82f,h*shape.clusterHeight},
            tree.seed+199u,rotation);
    return result;
}

void TreeRenderer::render(std::size_t id, const Tree& tree, const Camera& camera, GlyphRenderer& renderer)
{
    // An overhead view still resolves the crown's horizontal extent.
    const float pixels = tree.height*std::max(camera.getLift(),0.62f)*camera.getZoom();
    int& level = levels[id];
    if (level < 0) level = pixels >= highPixels ? 0 : (pixels >= mediumPixels ? 1 : 2);
    if (level == 0 && pixels < highPixels*(1-hysteresis)) level = 1;
    if (level == 1 && pixels > highPixels*(1+hysteresis)) level = 0;
    if (level == 1 && pixels < mediumPixels*(1-hysteresis)) level = 2;
    if (level == 2 && pixels > mediumPixels*(1+hysteresis)) level = 1;
    renderer.recordTreeLevel(level);
    if (renderer.useTreeCache(id,level)) return;

    for (const auto& glyph : generate(tree,level))
        renderer.drawWorld(camera,glyph.character,glyph.x,glyph.y,glyph.z,glyph.scale,
                           glyph.color,glyph.animation,glyph.transform,GlyphAnchor::Center,false,true,glyph.material);
    renderer.finishGroundChunk();
}

#include "renderer/VegetationGenerator.hpp"

#include <cmath>
#include <algorithm>
#include <array>

std::uint32_t VegetationGenerator::hash(
    std::uint32_t value
)
{
    value ^= value >> 16;
    value *= 0x7feb352dU;

    value ^= value >> 15;
    value *= 0x846ca68bU;

    value ^= value >> 16;

    return value;
}

std::uint32_t VegetationGenerator::tileSeed(
    int x,
    int y
)
{
    return hash(
        static_cast<std::uint32_t>(x)
            * 374761393U
        ^
        static_cast<std::uint32_t>(y)
            * 668265263U
        ^
        42U
    );
}

float VegetationGenerator::random(
    std::uint32_t& state
)
{
    state = hash(state + 0x9e3779b9U);

    return static_cast<float>(
        state >> 8
    ) / 16777216.0f;
}

float VegetationGenerator::densityNoise(
    int x,
    int y
)
{
    const float a =
        std::sin(x * 0.19f + y * 0.07f);

    const float b =
        std::cos(y * 0.23f - x * 0.11f);

    const float c =
        std::sin(x * 0.51f + y * 0.37f);

    return std::clamp(
        0.5f
        + a * 0.23f
        + b * 0.18f
        + c * 0.09f,
        0.0f,
        1.0f
    );
}

void VegetationGenerator::generate(const Tile& tile, int tileX, int tileY,
    float cellWidth, float cellHeight, std::vector<VisualGlyph>& output)
{
    output.clear();
    if (cellWidth <= 0 || cellHeight <= 0) return;
    auto state = tileSeed(tileX,tileY);
    const float noise = densityNoise(tileX,tileY);
    // Blades per square world unit. Forest and meadow remain densely covered.
    if (tile.type == TileType::Water ||
    tile.type == TileType::DeepWater)
    {
        return;
    }

    const float density = 0.54f + 0.16f * noise;
    const int count = std::clamp(static_cast<int>(std::round(cellWidth*cellHeight*density)),0,1024);
    if (count == 0) return;
    const int cols = std::max(1,static_cast<int>(std::ceil(std::sqrt(count*cellWidth/cellHeight))));
    const int rows = (count+cols-1)/cols;
    std::vector<int> cells(cols*rows);
    for (int i=0;i<static_cast<int>(cells.size());++i) cells[i]=i;
    for (int i=static_cast<int>(cells.size())-1;i>0;--i)
        std::swap(cells[i],cells[static_cast<int>(random(state)*(i+1))]);
    constexpr std::array<char32_t,8> palette{U'/',U'\\',U'|',U'v',U'V',U'Y',U'/',U'\\'};
    constexpr std::array<Color,6> colors{{
        {0.19f,0.32f,0.12f},{0.25f,0.40f,0.15f},{0.30f,0.45f,0.18f},
        {0.35f,0.47f,0.20f},{0.20f,0.35f,0.17f},{0.40f,0.50f,0.23f}}};
    output.reserve(count);
    for(int i=0;i<count;++i) {
        const int cell=cells[i];
        const float u=(cell%cols+0.5f+(random(state)-0.5f)*0.72f)/cols;
        const float v=(cell/cols+0.5f+(random(state)-0.5f)*0.72f)/rows;
        VisualGlyph g;
        g.x=(tileX+u)*cellWidth; g.y=(tileY+v)*cellHeight; g.z=0.0f;
        const float patch=0.5f+0.5f*std::sin(g.x*0.075f+std::cos(g.y*0.085f)*1.8f);
        float h=3.5f+4.0f*random(state)+3.0f*patch;
        if(random(state)<0.08f) h+=2.5f+3.0f*random(state);
        g.scale=h/36.0f;
        g.character=palette[static_cast<std::size_t>(random(state)*palette.size())];
        g.color=colors[static_cast<std::size_t>(random(state)*colors.size())];
        const float brightness=0.85f+0.26f*random(state);
        g.color.r*=brightness; g.color.g*=brightness; g.color.b*=brightness;
        g.transform.rotation=0.0f;
        g.transform.opacity=1.0f;
        g.transform.pivotX=0.5f;g.transform.pivotY=1.0f;
        g.animation={AnimationType::Grass,random(state)*6.2831853f,1.0f};
        output.push_back(g);
    }
}

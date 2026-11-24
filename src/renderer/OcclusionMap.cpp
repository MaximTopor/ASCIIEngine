#include "renderer/OcclusionMap.hpp"

#include <vector>
#include <cstddef>

OcclusionMap::OcclusionMap(const World& world)
{
    rebuild(world);
}

void OcclusionMap::rebuild(const World& world)
{
    const int width = world.getWidth();
    const int height = world.getHeight();

    std::vector<unsigned char> pixels(
        static_cast<std::size_t>(width) * height,
        0
    );

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const Tile& tile = world.getTile(x, y);

            const std::size_t index =
                static_cast<std::size_t>(y)
                * width + x;

            pixels[index] = static_cast<unsigned char>(
                tile.getLightOpacity() * 255.0f + 0.5f
            );
        }
    }

    texture = std::make_unique<Texture>(
        width,
        height,
        pixels.data()
    );
}

const Texture& OcclusionMap::getTexture() const
{
    return *texture;
}
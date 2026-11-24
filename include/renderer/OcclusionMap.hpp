#pragma once

#include "renderer/Texture.hpp"
#include "world/World.hpp"

#include <memory>

class OcclusionMap
{
private:
    std::unique_ptr<Texture> texture;

public:
    explicit OcclusionMap(const World& world);

    void rebuild(const World& world);

    const Texture& getTexture() const;
};
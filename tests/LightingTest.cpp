#include "renderer/Lighting.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
    void require(bool condition, const char* failure)
    {
        if (!condition) throw std::runtime_error(failure);
    }

    float length(DirectionalLight light)
    {
        return std::sqrt(light.x*light.x+light.y*light.y+light.z*light.z);
    }
}

int main()
{
    try
    {
        Lighting lighting;
        lighting.setHour(12.0f);
        const auto noon = lighting.getSurfaceLighting();
        lighting.setHour(0.0f);
        const auto midnight = lighting.getSurfaceLighting();
        require(noon.sun.z > 0.5f && noon.sun.radiance.r > 0.5f, "noon sunlight missing");
        require(midnight.sun.radiance.r == 0.0f, "sun remains active at midnight");
        require(midnight.moon.z > 0.5f && midnight.moon.radiance.b > 0.0f, "midnight moonlight missing");
        require(midnight.moon.radiance.b > midnight.moon.radiance.r, "moonlight should be cool");
        require(noon.sky.r > midnight.sky.r && midnight.ground.r > 0 && noon.sky.r > noon.ground.r,
            "incorrect hemispherical ambient transition");

        lighting.setHour(7.0f);
        const auto morning = lighting.getSurfaceLighting();
        lighting.setHour(17.0f);
        const auto evening = lighting.getSurfaceLighting();
        require(morning.sun.x * evening.sun.x < 0.0f, "sun does not move across the scene");
        require(morning.sun.radiance.r > morning.sun.radiance.b, "morning light should be warm");
        for (int minutes = 0; minutes < 24*60; minutes += 7)
        {
            lighting.setHour(minutes / 60.0f);
            const auto frame = lighting.getSurfaceLighting();
            require(std::abs(length(frame.sun) - 1.0f) < 0.0001f, "sun direction is not normalized");
            require(std::abs(length(frame.moon) - 1.0f) < 0.0001f, "moon direction is not normalized");
        }

        using MaterialPresets::Type;
        constexpr auto steel = MaterialPresets::surface(Type::Steel);
        constexpr auto stone = MaterialPresets::surface(Type::Stone);
        static_assert(steel.roughness < stone.roughness && steel.reflectivity > stone.reflectivity);
        static_assert(MaterialPresets::surface(Type::Leaf).mode == LightingMode::Foliage);
        static_assert(GlyphMaterial{}.mode == LightingMode::Legacy);
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

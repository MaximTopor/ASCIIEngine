#pragma once

struct WorldScale
{
    // At zoom 1, one world unit maps to one framebuffer pixel.
    float cellWidth = 24.0f;
    float cellHeight = 28.0f;
    float playerHeight = 28.0f;
    float minimumRockDiameter = 2.8f; // Measured in player heights.
    float maximumRockDiameter = 4.0f;
};

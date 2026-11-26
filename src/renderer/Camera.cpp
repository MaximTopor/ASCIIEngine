#include "renderer/Camera.hpp"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <numbers>

Camera::Camera()
{
    setAngles(azimuth, elevation);
}

void Camera::setPosition(float x, float y)
{
    this->x = x;
    this->y = y;
}

void Camera::move(float dx, float dy)
{
    x += dx;
    y += dy;
}

void Camera::setZoom(float value)
{
    zoom = std::clamp(
        value,
        MIN_ZOOM,
        MAX_ZOOM
    );
}

void Camera::changeZoom(float factor)
{
    setZoom(zoom * factor);
}

void Camera::setViewportSize(int width, int height)
{
    if (width > 0 && height > 0)
    {
        viewportWidth = width;
        viewportHeight = height;
    }
}

void Camera::setAngles(float azimuthRadians, float elevationRadians)
{
    azimuth = std::remainder(azimuthRadians, 2.0f * std::numbers::pi_v<float>);
    elevation = std::clamp(elevationRadians, 0.32f, std::numbers::pi_v<float> * 0.5f);
    c = std::cos(azimuth);
    s = std::sin(azimuth);
    rise = std::sin(elevation);
    lift = std::cos(elevation);
}

void Camera::orbit(float azimuthDelta, float elevationDelta)
{
    setAngles(azimuth + azimuthDelta, elevation + elevationDelta);
}

PlanePoint Camera::project(float worldX, float worldY, float z) const
{
    const float dx = worldX - x;
    const float dy = worldY - y;
    return {viewportWidth * 0.5f + zoom * (c * dx - s * dy),
            viewportHeight * 0.5f + zoom * (rise * (s * dx + c * dy) - lift * z)};
}

PlanePoint Camera::unproject(float screenX, float screenY, float z) const
{
    const float u = (screenX - viewportWidth * 0.5f) / zoom;
    const float v = ((screenY - viewportHeight * 0.5f) / zoom + lift * z) / rise;
    return {x + c * u + s * v, y - s * u + c * v};
}

bool Camera::isVisible(PlanePoint screen, float padding) const
{
    return screen.x >= -padding && screen.x <= viewportWidth + padding
        && screen.y >= -padding && screen.y <= viewportHeight + padding;
}

GroundBounds Camera::visibleGround(float maxElevation) const
{
    GroundBounds bounds{x, x, y, y};
    for (float z : {0.0f, maxElevation})
    {
        for (float sx : {0.0f, static_cast<float>(viewportWidth)})
        {
            for (float sy : {0.0f, static_cast<float>(viewportHeight)})
            {
                const auto point = unproject(sx, sy, z);
                bounds.left = std::min(bounds.left, point.x);
                bounds.right = std::max(bounds.right, point.x);
                bounds.top = std::min(bounds.top, point.y);
                bounds.bottom = std::max(bounds.bottom, point.y);
            }
        }
    }
    return bounds;
}

float Camera::depth(float worldX, float worldY, float z) const
{
    return lift * (s * (worldX - x) + c * (worldY - y)) + rise * z;
}

float Camera::getX() const
{
    return x;
}

float Camera::getY() const
{
    return y;
}

float Camera::getZoom() const
{
    return zoom;
}

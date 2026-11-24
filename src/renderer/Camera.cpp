#include "renderer/Camera.hpp"

#include <algorithm>

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

float Camera::worldToScreenX(float worldX) const
{
    return (worldX - x) * zoom
        + viewportWidth / 2.0f;
}

float Camera::worldToScreenY(float worldY) const
{
    return (worldY - y) * zoom
        + viewportHeight / 2.0f;
}

float Camera::screenToWorldX(float screenX) const
{
    return (screenX - viewportWidth / 2.0f)
        / zoom + x;
}

float Camera::screenToWorldY(float screenY) const
{
    return (screenY - viewportHeight / 2.0f)
        / zoom + y;
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

float Camera::getVisibleWidth() const
{
    return viewportWidth / zoom;
}

float Camera::getVisibleHeight() const
{
    return viewportHeight / zoom;
}
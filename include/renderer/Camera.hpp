#pragma once

class Camera
{
private:
    float x = 0.0f;
    float y = 0.0f;

    float zoom = 1.0f;

    int viewportWidth = 1280;
    int viewportHeight = 720;

    static constexpr float MIN_ZOOM = 0.25f;
    static constexpr float MAX_ZOOM = 4.0f;

public:
    void setPosition(float x, float y);

    void move(float dx, float dy);

    void setZoom(float value);
    void changeZoom(float factor);

    void setViewportSize(int width, int height);

    float worldToScreenX(float worldX) const;
    float worldToScreenY(float worldY) const;

    float screenToWorldX(float screenX) const;
    float screenToWorldY(float screenY) const;

    float getX() const;
    float getY() const;
    float getZoom() const;

    float getVisibleWidth() const;
    float getVisibleHeight() const;
};
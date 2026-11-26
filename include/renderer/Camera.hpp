#pragma once

struct PlanePoint
{
    float x;
    float y;
};

struct GroundBounds
{
    float left;
    float right;
    float top;
    float bottom;
};

class Camera
{
private:
    float x = 0.0f;
    float y = 0.0f;

    float zoom = 1.0f;
    float azimuth = 0.26f;
    float elevation = 0.66f;
    float c;
    float s;
    float rise;
    float lift;

    int viewportWidth = 1280;
    int viewportHeight = 720;

    static constexpr float MIN_ZOOM = 0.25f;
    static constexpr float MAX_ZOOM = 4.0f;

public:
    Camera();
    void setPosition(float x, float y);

    void move(float dx, float dy);

    void setZoom(float value);
    void changeZoom(float factor);

    void setViewportSize(int width, int height);

    void setAngles(float azimuthRadians, float elevationRadians);
    void orbit(float azimuthDelta, float elevationDelta);
    PlanePoint project(float worldX, float worldY, float z = 0.0f) const;
    bool isVisible(PlanePoint screen, float padding) const;
    PlanePoint unproject(float screenX, float screenY, float z = 0.0f) const;
    GroundBounds visibleGround(float maxElevation = 0.0f) const;
    float depth(float worldX, float worldY, float z = 0.0f) const;
    float elevationPixels(float z) const { return lift * z * zoom; }

    float getX() const;
    float getY() const;
    float getZoom() const;

    float getCosAzimuth() const
    {
        return c;
    }

    float getSinAzimuth() const
    {
        return s;
    }

    float getRise() const
    {
        return rise;
    }

    float getLift() const
    {
        return lift;
    }

};

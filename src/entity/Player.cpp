#include "entity/Player.hpp"

#include <cmath>
#include <algorithm>

Player::Player(float x, float y)
    : x(x),
      y(y)
{
}

bool Player::move(
    float dx,
    float dy,
    float deltaTime,
    const World& world
)
{
    if (deltaTime <= 0.0f)
    {
        return false;
    }

    const float length =
        std::hypot(dx, dy);

    if (length < 0.0001f)
    {
        return false;
    }

    // Normalize movement direction.

    dx /= length;
    dy /= length;

    // Frame-rate-independent movement.

    const auto& waterScale = world.getScale();

    const int tileX = static_cast<int>(
        std::floor(x / waterScale.cellWidth)
    );

    const int tileY = static_cast<int>(
        std::floor(y / waterScale.cellHeight)
    );

    float speedMultiplier = 1.0f;

    if (world.isInside(tileX, tileY))
    {
        if (world.getTile(tileX, tileY).type
            == TileType::Water)
        {
            speedMultiplier = 0.55f;
        }
    }

    const float distance =
        movementSpeed
        * speedMultiplier
        * deltaTime;

    const float moveX = dx * distance;
    const float moveY = dy * distance;

    const auto& scale = world.getScale();

    // Player collision radius.

    const float radius = std::min(
        scale.playerHeight * 0.15f,
        std::min(
            scale.cellWidth,
            scale.cellHeight
        ) * 0.45f
    );

    // Split movement into small steps
    // to prevent tunneling.

    const float maxStep =
        std::min(
            scale.cellWidth,
            scale.cellHeight
        ) * 0.25f;

    const int steps = std::max(
        1,
        static_cast<int>(
            std::ceil(distance / maxStep)
        )
    );

    const float stepX = moveX / steps;
    const float stepY = moveY / steps;

    bool moved = false;

    for (int i = 0; i < steps; ++i)
    {
        const float newX = x + stepX;
        const float newY = y + stepY;

        // Try full movement first.

        if (world.isWalkableAt(
                newX,
                newY,
                radius
            ))
        {
            x = newX;
            y = newY;

            moved = true;

            continue;
        }

        // Slide along obstacles on the X axis.

        if (stepX != 0.0f &&
            world.isWalkableAt(
                newX,
                y,
                radius
            ))
        {
            x = newX;

            moved = true;
        }

        // Slide along obstacles on the Y axis.

        if (stepY != 0.0f &&
            world.isWalkableAt(
                x,
                newY,
                radius
            ))
        {
            y = newY;

            moved = true;
        }
    }

    return moved;
}

float Player::getX() const
{
    return x;
}

float Player::getY() const
{
    return y;
}

float Player::getMovementSpeed() const
{
    return movementSpeed;
}

void Player::setMovementSpeed(float speed)
{
    movementSpeed = std::max(0.0f, speed);
}

void Player::setPosition(float x, float y)
{
    this->x = x;
    this->y = y;
}
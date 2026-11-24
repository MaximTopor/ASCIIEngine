#include "entity/Player.hpp"

Player::Player(int x, int y)
    : x(x),
      y(y)
{
}

bool Player::move(
    int dx,
    int dy,
    const World& world
)
{
    const int newX = x + dx;
    const int newY = y + dy;

    if (!world.isWalkable(newX, newY))
    {
        return false;
    }

    x = newX;
    y = newY;

    return true;
}

int Player::getX() const
{
    return x;
}

int Player::getY() const
{
    return y;
}
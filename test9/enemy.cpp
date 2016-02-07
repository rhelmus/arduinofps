#include "enemy.h"
#include "game.h"

void Enemy::think()
{
    const Real dx = game.getPlayer().getPos().x - getPos().x;
    const Real dy = game.getPlayer().getPos().y - getPos().y;
    const Real dist = sqrt(dx*dx + dy*dy);

    if (dist > 3.0)
    {
        setAngle(atan2(dy, dx));
        move(0.01);
        game.getWorld().markDirty();
    }
}

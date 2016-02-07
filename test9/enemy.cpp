#include "world.h"

void Enemy::think()
{
    const Real dx = world.getPlayer().getPos().x - getPos().x;
    const Real dy = world.getPlayer().getPos().y - getPos().y;
    const Real dist = sqrt(dx*dx + dy*dy);

    if (dist > 3.0)
    {
        setAngle(atan2(dy, dx));
        world.move(this, 0.01);
        world.markDirty();
    }
}

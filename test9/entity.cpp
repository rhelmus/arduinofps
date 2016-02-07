#include "entity.h"
#include "game.h"

void MovableEntity::move(Real speed)
{
    const Real dx = cos(angle) * speed;
    const Real dy = sin(angle) * speed;
    const Vec2D pos = getPos() + Vec2D(dx, dy);
    setPos(game.getWorld().checkCollision(getPos(), pos, 0.35)); // UNDONE: magic nr
}

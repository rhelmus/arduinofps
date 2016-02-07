#ifndef ENTITY
#define ENTITY

#include "defs.h"
#include "utils.h"

class Entity
{
public:
    enum Flags
    {
        FLAG_NONE = 1<<0,
        FLAG_ENABLED = 1<<1,
        FLAG_ROTATIONAL_SPRITE = 1<<2,
        FLAG_VSIBILE = 1<<3, // set automatically when visible
    };

private:
    Vec2D pos = Vec2D(0.0, 0.0);
    int flags = FLAG_NONE;
    Sprite sprite = SPRITE_NONE;

public:
    void setPos(const Vec2D &v) { pos = v; }
    void setPos(Real x, Real y) { pos.x = x; pos.y = y; }
    Vec2D getPos(void) const { return pos; }

    void setFlags(int f) { flags = f; }
    void setFlag(Flags f) { flags |= f; }
    void unsetFlag(Flags f) { flags &= ~f; }
    int getFlags(void) const { return flags; }

    void setSprite(Sprite s) { sprite = s; }
    Sprite getSprite(void) const { return sprite; }
};

class MovableEntity : public Entity
{
    Real angle = 0.0;

public:
    void setAngle(Real a) { angle = a; }
    Real getAngle(void) const { return angle; }
    void move(Real speed);
};


#endif // ENTITY


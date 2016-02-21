#ifndef WORLD_H
#define WORLD_H

#include <math.h>
#include <stdint.h>

#include "defs.h"
#include "entity.h"
#include "gfx.h"
#include "raycast.h"
#include "render.h"
#include "utils.h"

class World
{
    RayCast rayCaster;
    Render renderer;
    Entity *entities[MAX_STATIC_ENTITIES + MAX_ENEMIES];
    uint8_t entityCount = 0;
    bool dirty = true;

    bool isBlocking(int x, int y) const;

public:
    World(void) { }

    void setup(void);
    void update(void);
    void markDirty(void) { dirty = true; }
    Vec2D checkCollision(const Vec2D &from, const Vec2D &to, Real radius) const;
    void addEntity(Entity *e);
};

#endif // WORLD_H

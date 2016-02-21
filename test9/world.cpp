#include "game.h"
#include "utils.h"
#include "world.h"

namespace {

const uint8_t worldMap[WORLD_SIZE][WORLD_SIZE] = {
    { 1, 4, 1, 1, 6, 1, 1, 1 },
    { 2, 0, 0, 0, 0, 0, 0, 7 },
    { 1, 0, 0, 0, 0, 0, 0, 1 },
    { 6, 0, 0, 0, 0, 0, 0, 3 },
    { 1, 0, 0, 0, 5, 0, 0, 1 },
    { 2, 0, 0, 0, 0, 0, 0, 1 },
    { 3, 0, 0, 0, 0, 0, 0, 5 },
    { 2, 2, 2, 2, 7, 4, 4, 4 },
};

/*const int worldMap[WORLD_SIZE][WORLD_SIZE] = {
    { 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 1, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1 },
};*/

/*const int worldMap[WORLD_SIZE][WORLD_SIZE] = {
    { 1, 4, 1, 1, 6, 1, 1, 1 },
    { 2, 0, 0, 5, 0, 0, 0, 7 },
    { 1, 1, 0, 1, 0, 1, 1, 1 },
    { 6, 0, 0, 0, 0, 0, 0, 3 },
    { 1, 8, 8, 0, 8, 0, 8, 1 },
    { 2, 2, 0, 0, 8, 8, 7, 1 },
    { 3, 0, 0, 0, 0, 0, 0, 5 },
    { 2, 2, 2, 2, 7, 4, 4, 4 },
};*/



}

bool World::isBlocking(int x, int y) const
{
    // UNDONE: check for blocking sprites
    return (x < 0 || x >= WORLD_SIZE || y < 0 || y >= WORLD_SIZE || worldMap[y][x] != 0);
}

void World::setup()
{
    renderer.setup(rayCaster.getRayCastInfo(), rayCaster.getEntityCastInfo(), entities);
}

void World::update()
{
    if (dirty)
    {
        const uint32_t start = getMillis();

        rayCaster.rayCast(worldMap);
        rayCaster.castEntities(entities, entityCount);
        renderer.render(rayCaster.getRayFrameNumber());
        dirty = false;

        debugf("update time: %u\n", static_cast<uint32_t>(getMillis()) - start);
    }
}

// Based on https://dev.opera.com/articles/3d-games-with-canvas-and-raycasting-part-2/
Vec2D World::checkCollision(const Vec2D &from, const Vec2D &to, Real radius) const
{
    const int maptox = static_cast<int>(to.x);
    const int maptoy = static_cast<int>(to.y);

    if (isBlocking(maptox, maptoy))
        return from;

    Vec2D pos = to;

    const bool blocktop = isBlocking(maptox, maptoy-1);
    const bool blockbottom = isBlocking(maptox, maptoy+1);
    const bool blockleft = isBlocking(maptox-1, maptoy);
    const bool blockright = isBlocking(maptox+1, maptoy);

    if (blocktop && pos.y - maptoy < radius)
        pos.y = maptoy + radius;
    if (blockbottom && maptoy+1 - pos.y < radius)
        pos.y = maptoy + 1 - radius;
    if (blockleft && pos.x - maptox < radius)
        pos.x = maptox + radius;
    if (blockright && maptox+1 - pos.x < radius)
        pos.x = maptox + 1 - radius;

    // is tile to the top-left a wall
    if (isBlocking(maptox-1, maptoy-1) && !(blocktop && blockleft))
    {
        const Real dx = pos.x - maptox;
        const Real dy = pos.y - maptoy;
        if (dx*dx+dy*dy < radius*radius)
        {
            if (dx*dx > dy*dy)
                pos.x = maptox + radius;
            else
                pos.y = maptoy + radius;
        }
    }

    // is tile to the top-right a wall
    if (isBlocking(maptox+1, maptoy-1) && !(blocktop && blockright))
    {
        const Real dx = pos.x - (maptox+1);
        const Real dy = pos.y - maptoy;
        if (dx*dx+dy*dy < radius*radius)
        {
            if (dx*dx > dy*dy)
                pos.x = maptox + 1 - radius;
            else
                pos.y = maptoy + radius;
        }
    }

    // is tile to the bottom-left a wall
    if (isBlocking(maptox-1, maptoy+1) && !(blockbottom && blockleft))
    {
        const Real dx = pos.x - maptox;
        const Real dy = pos.y - (maptoy+1);
        if (dx*dx+dy*dy < radius*radius)
        {
            if (dx*dx > dy*dy)
                pos.x = maptox + radius;
            else
                pos.y = maptoy + 1 - radius;
        }
    }

    // is tile to the bottom-right a wall
    if (isBlocking(maptox+1, maptoy+1) && !(blockbottom && blockright))
    {
        const Real dx = pos.x - (maptox+1);
        const Real dy = pos.y - (maptoy+1);
        if (dx*dx+dy*dy < radius*radius)
        {
            if (dx*dx > dy*dy)
                pos.x = maptox + 1 - radius;
            else
                pos.y = maptoy + 1 - radius;
        }
    }

    return pos;
}

void World::addEntity(Entity *e)
{
    entities[entityCount] = e;
    ++entityCount;
    renderer.setEntityCount(entityCount);
}

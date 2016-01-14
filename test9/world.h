#ifndef RAYCAST_H
#define RAYCAST_H

#include <math.h>
#include <stdint.h>

enum
{
    SCREEN_WIDTH = 200,
    SCREEN_HEIGHT = 100,
    WORLD_SIZE = 8,
    WORLD_MAX_LENGTH = static_cast<int>(sqrt(WORLD_SIZE*WORLD_SIZE + WORLD_SIZE*WORLD_SIZE) + 0.5),
    TEXTURE_SIZE = 64,
    TEXTURE_COUNT = 8,
    MAX_STATIC_ENTITIES = 128,
    MAX_ENEMIES = 128
};

typedef float Real;

struct Vec2D
{
    Real x; Real y;

    Vec2D(Real _x, Real _y) : x(_x), y(_y) { }
    Vec2D(void) = default;

    Vec2D operator+(const Vec2D &v) const { return Vec2D(x + v.x, y + v.y); }
    Vec2D operator-(const Vec2D &v) const { return Vec2D(x - v.x, y - v.y); }
    Vec2D operator*(const Vec2D &v) const { return Vec2D(x * v.x, y * v.y); }
};

class Object
{
    Vec2D pos;

public:
    Object(void) : pos(Vec2D(0.0, 0.0)) { }

    void setPos(const Vec2D &v) { pos = v; }
    void setPos(Real x, Real y) { pos.x = x; pos.y = y; }
    Vec2D getPos(void) const { return pos; }
};

class Entity : public Object
{
public:
    enum Flags
    {
        FLAG_NONE = 1<<0,
        FLAG_ENABLED = 1<<1,
    };

private:
    Flags flags = FLAG_NONE;

public:
    void setFlags(Flags t) { flags = t; }
    Flags getFlags(void) const { return flags; }
};

class Player : public Object
{
    Real angle = 0.0;

public:
    Player(void) { }

    void setAngle(Real a) { angle = a; }
    Real getAngle(void) const { return angle; }
};

class Enemy : public Entity
{
};

class World
{
    struct RayInfo
    {
        uint8_t mapX, mapY;
    };

    Player player;
    Entity staticEntityStore[MAX_STATIC_ENTITIES];
    Enemy enemyStore[MAX_ENEMIES];
    Entity *entities[MAX_STATIC_ENTITIES + MAX_ENEMIES];
    uint8_t staticEntityCount = 0;
    uint8_t enemyCount = 0;
    uint8_t entityCount = 0;

    int lastWallTexture = -1;
    bool visibleCells[WORLD_SIZE][WORLD_SIZE]; // filled during every ray cast session
    bool dirty = true;

    void drawStripe(int x, float dist, int texture, int col);
    void preRender(void);
    void rayCast(void);
    void drawEntities(void);
    void render(void);
    void handleInput(void);

    void addStaticEntity(const Vec2D &pos, Entity::Flags flags)
    {
        staticEntityStore[staticEntityCount].setPos(pos);
        staticEntityStore[staticEntityCount].setFlags(flags);

        entities[entityCount] = &staticEntityStore[staticEntityCount];

        ++staticEntityCount;
        ++entityCount;
    }

public:
    World(void);

    void setup(void);
    void update(void);
};

#endif // RAYCAST_H

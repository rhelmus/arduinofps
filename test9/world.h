#ifndef RAYCAST_H
#define RAYCAST_H

#include <math.h>
#include <stdint.h>

#include <virtmem.h>
#include <alloc/spiram_alloc.h>

#include "gfx.h"

using namespace virtmem;

enum
{
    SCREEN_WIDTH = 200,
    SCREEN_HEIGHT = 100,
    WORLD_SIZE = 8,
    WORLD_MAX_LENGTH = static_cast<int>(sqrt(WORLD_SIZE*WORLD_SIZE + WORLD_SIZE*WORLD_SIZE) + 0.5),
    TEXTURE_SIZE = 64,
    TEXTURE_COUNT = 8,
    MAX_STATIC_ENTITIES = 128,
    MAX_ENEMIES = 128,
    SPRITE_BLOCK = 8192UL,
    MAX_LOADED_SPRITES = 28,
    SPRITE_NOT_LOADED = -1,
    SPRITE_SHOULD_LOAD = -2,
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
    Sprite sprite = SPRITE_NONE;

public:
    void setFlags(Flags t) { flags = t; }
    Flags getFlags(void) const { return flags; }
    void setSprite(Sprite s) { sprite = s; }
    Sprite getSprite(void) const { return sprite; }
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
        Real hitDist;
        Sprite texture;
        uint8_t texColumn;
        bool dark;
    };

    struct SpriteGfxInfo
    {
        int8_t gfxIndex = SPRITE_NOT_LOADED; // index in loadedSprites
        uint32_t lastRayFrameNumber = 0;
    };

    typedef VPtr<uint8_t, SPIRAMVAlloc> CachedSprite;

    SPIRAMVAlloc valloc;

    Player player;
    Entity staticEntityStore[MAX_STATIC_ENTITIES];
    Enemy enemyStore[MAX_ENEMIES];
    Entity *entities[MAX_STATIC_ENTITIES + MAX_ENEMIES];
    uint8_t staticEntityCount = 0;
    uint8_t enemyCount = 0;
    uint8_t entityCount = 0;

    CachedSprite cachedSprites[SPRITE_END];
    Sprite currentLoadedSprite = SPRITE_NONE;
    int currentLoadedSpriteW = -1, currentLoadedSpriteH = -1;
    uint32_t rayFrameNumber = 0;
    SpriteGfxInfo spriteGfxInfo[SPRITE_END];
    Sprite loadedSprites[MAX_LOADED_SPRITES];
    bool dirty = true;

    // filled during every ray cast session
    bool visibleCells[WORLD_SIZE][WORLD_SIZE];
    RayInfo rayCastInfo[SCREEN_WIDTH];
    Sprite spritesToLoad[MAX_LOADED_SPRITES];
    uint8_t spritesToLoadCount = 0;

    void initSprite(Sprite s, int w=-1, int h=-1);
    void drawStripe(int x, float dist, Sprite texture, int col);
    void rayCast(void);
    void loadSprites(void);
    void preRender(void);
    void drawWorld(void);
    void drawEntities(void);
    void render(void);
    void handleInput(void);

    void addStaticEntity(const Vec2D &pos, Entity::Flags flags, Sprite sprite);

public:
    World(void);

    void setup(void);
    void update(void);
};

#endif // RAYCAST_H

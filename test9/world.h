#ifndef RAYCAST_H
#define RAYCAST_H

#include <math.h>
#include <stdint.h>

#include <virtmem.h>
#include <alloc/spiram_alloc.h>

#include "defs.h"
#include "gfx.h"

using namespace virtmem;

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
    Real angle = 0.0;

public:
    Object(void) : pos(Vec2D(0.0, 0.0)) { }

    void setPos(const Vec2D &v) { pos = v; }
    void setPos(Real x, Real y) { pos.x = x; pos.y = y; }
    Vec2D getPos(void) const { return pos; }
    void setAngle(Real a) { angle = a; }
    Real getAngle(void) const { return angle; }
};

class Entity : public Object
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
    int flags = FLAG_NONE;
    Sprite sprite = SPRITE_NONE;

public:
    void setFlags(int f) { flags = f; }
    void setFlag(Flags f) { flags |= f; }
    void unsetFlag(Flags f) { flags &= ~f; }
    int getFlags(void) const { return flags; }
    void setSprite(Sprite s) { sprite = s; }
    Sprite getSprite(void) const { return sprite; }
};

class Player : public Object
{
public:
    Player(void) { }
};

class Enemy : public Entity
{
public:
    void think(void);
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

    struct EntityDrawInfo
    {
        int16_t spriteX, spriteY, spriteW;
        Real dist, scaleF;
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
    Sprite loadedGDSprites[MAX_LOADED_SPRITES];
    uint8_t freeGDSpriteIndex = 0;
    bool dirty = true;

    // filled during every ray cast session
    bool visibleCells[WORLD_SIZE][WORLD_SIZE];
    RayInfo rayCastInfo[SCREEN_WIDTH];
    EntityDrawInfo entityDrawInfo[MAX_STATIC_ENTITIES + MAX_ENEMIES];
    Sprite spritesToLoad[MAX_LOADED_SPRITES];
    uint8_t spritesToLoadCount = 0;

    void initSprite(Sprite s, int w=-1, int h=-1);
    void drawStripe(int x, float dist, Sprite texture, int col);
    void rayCast(void);
    void castEntities(void);
    void loadSprites(void);
    void preRender(void);
    void drawWorld(void);
    void drawEntities(void);
    void render(void);
    void handleInput(void);
    bool isBlocking(int x, int y) const;
    Vec2D checkCollision(const Vec2D &from, const Vec2D &to, Real radius) const;
    void addStaticEntity(const Vec2D &pos, Entity::Flags flags, Sprite sprite);
    void addEnemy(const Vec2D &pos, Sprite sprite);

public:
    World(void);

    void setup(void);
    void update(void);

    const Player &getPlayer(void) const { return player; }
    void move(Object *obj, Real speed);
    void markDirty(void) { dirty = true; }
};

extern World world;

#endif // RAYCAST_H

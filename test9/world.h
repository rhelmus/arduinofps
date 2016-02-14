#ifndef WORLD_H
#define WORLD_H

#include <math.h>
#include <stdint.h>

#include <virtmem.h>
#include <alloc/spiram_alloc.h>

#include "defs.h"
#include "entity.h"
#include "gfx.h"
#include "raycast.h"
#include "render.h"
#include "utils.h"

using namespace virtmem;

class World
{
#if 0
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

    Entity *entities[MAX_STATIC_ENTITIES + MAX_ENEMIES];
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
    bool isBlocking(int x, int y) const;
#endif

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

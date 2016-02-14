#ifndef RENDER_H
#define RENDER_H

#include <virtmem.h>
#include <alloc/spiram_alloc.h>
#include <SdFat.h>

#include "defs.h"
#include "gfx.h"
#include "raycast.h"

using namespace virtmem;

class Entity;

class Render
{
    struct SpriteGfxInfo
    {
        int8_t gfxIndex = SPRITE_NOT_LOADED; // index in loadedSprites
        uint32_t lastRayFrameNumber = 0;
    };

    typedef VPtr<uint8_t, SPIRAMVAlloc> CachedSprite;

    SdFat sd;
    SPIRAMVAlloc valloc;

    CachedSprite cachedSprites[SPRITE_END];
    Sprite currentLoadedSprite = SPRITE_NONE;
    int currentLoadedSpriteW = -1, currentLoadedSpriteH = -1;
    SpriteGfxInfo spriteGfxInfo[SPRITE_END];
    Sprite loadedGDSprites[MAX_LOADED_SPRITES];
    uint8_t freeGDSpriteIndex = 0;
    Sprite spritesToLoad[MAX_LOADED_SPRITES];
    uint8_t spritesToLoadCount = 0;
    uint32_t renderStartTime;

    // bound data from RayCast/World
    const RayCast::RayCastInfo *rayCastInfo;
    const RayCast::EntityCastInfo *entityCastInfo;
    Entity **entities;
    int entityCount = 0;

    void initSpriteDraw(Sprite s, int w=-1, int h=-1);
    void drawStripe(int x, float dist, Sprite texture, int col);

    void markSpriteShouldLoad(uint32_t rayframe, Sprite s);
    void collectSpritesToLoad(uint32_t rayframe);
    void loadSprites(uint32_t rayframe);
    void beginRender(void);
    void renderWalls(void);
    void renderEntities(void);
    void endRender(void);

public:
    Render(void);

    void setup(const RayCast::RayCastInfo *rinfo, const RayCast::EntityCastInfo *einfo,
               Entity **e);
    void setEntityCount(int ecount) { entityCount = ecount; }
    void render(uint32_t rayframe);
};

#endif // RENDER_H

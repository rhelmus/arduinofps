#include <EEPROM.h>
#include <SPI.h>
#include <SPIFIFO.h>
#include <GD2.h>
#include <serialram.h>

#include "entity.h"
#include "game.h"
#include "render.h"
#include "utils.h"

namespace {

inline int stencilFromDist(float dist)
{
    return min(static_cast<int>(dist * 255.0 / WORLD_MAX_LENGTH + 0.5), 255);
}


}

Render::Render()
{
    memset(loadedGDSprites, SPRITE_NONE, sizeof(loadedGDSprites));
}

void Render::markSpriteShouldLoad(uint32_t rayframe, Sprite s)
{
    const int is = static_cast<int>(s);
    spriteGfxInfo[is].lastRayFrameNumber = rayframe; // mark it visible during this frame
    if (spriteGfxInfo[is].gfxIndex == SPRITE_NOT_LOADED) // not yet loaded into GD2 memory?
    {
        spriteGfxInfo[is].gfxIndex = SPRITE_SHOULD_LOAD; // make sure to only add it once
        spritesToLoad[spritesToLoadCount] = s;
        ++spritesToLoadCount;
    }
}

void Render::collectSpritesToLoad(uint32_t rayframe)
{
    // check which wall textures need to be loaded
    for (int x=0; x<SCREEN_WIDTH; ++x)
        markSpriteShouldLoad(rayframe, rayCastInfo[x].texture);

    // check which entity sprites need to be loaded
    for (int e=0; e<entityCount; ++e)
    {
        if (entities[e]->getFlags() & Entity::FLAG_VSIBILE)
        {
            int s = entities[e]->getSprite();
            if (entities[e]->getFlags() & Entity::FLAG_ROTATIONAL_SPRITE)
                s += (Sprite)getEntityDrawRotation(game.getPlayer().getAngle(),
                                                   ((MovableEntity *)entities[e])->getAngle(),
                                                   entityCastInfo[e].spriteX);

            markSpriteShouldLoad(rayframe, static_cast<Sprite>(s));
        }
    }
}

void Render::loadSprites(uint32_t rayframe)
{
    // load new sprites into GD2 and replace old ones if possible
    if (spritesToLoadCount)
    {
        if (spritesToLoadCount > MAX_LOADED_SPRITES)
        {
            Serial.printf("WARNING!! Too many sprites need be loaded (%d, max: %d)\n",
                          spritesToLoadCount, MAX_LOADED_SPRITES);
            spritesToLoadCount = MAX_LOADED_SPRITES;
        }

        int replaceind = 0;
        for (int s=0; s<spritesToLoadCount; ++s)
        {
            int gdind;

            if (freeGDSpriteIndex != MAX_LOADED_SPRITES)
            {
                gdind = freeGDSpriteIndex;
                ++freeGDSpriteIndex;
            }
            else
            {
                // find a free slot: only sprites that need be available have a current frame number
                while (loadedGDSprites[replaceind] != SPRITE_NONE &&
                       spriteGfxInfo[loadedGDSprites[replaceind]].lastRayFrameNumber == rayframe)
                    ++replaceind;
                gdind = replaceind;
                ++replaceind; // no need to check for this (and previous) index again
            }

            if (loadedGDSprites[gdind] != SPRITE_NONE)
                spriteGfxInfo[loadedGDSprites[gdind]].gfxIndex = -1; // mark as not loaded

            const int spr = static_cast<int>(spritesToLoad[s]);
            spriteGfxInfo[spr].gfxIndex = gdind;
            spriteGfxInfo[spr].lastRayFrameNumber = rayframe;
            loadedGDSprites[gdind] = spritesToLoad[s];

            // load actual sprite
            Serial.printf("LOAD: %s @ %d\n", sprites[spritesToLoad[s]].file, gdind);

            const uint32_t begintime = millis();
            GD.cmd_inflate(spr * SPRITE_BLOCK);
            GD.__end();

            uint32_t cachesize = sprites[spr].comprsize;
            CachedSprite cp = cachedSprites[spr];
            while (cachesize)
            {
                VPtrLock<CachedSprite> lock = makeVirtPtrLock(cp, cachesize, true);
                const uint32_t lockedsize = lock.getLockSize();

                GD.resume();
                GD.copyram(*lock, lockedsize);
                GD.__end();

                cachesize -= lockedsize;
                cp += lockedsize;
            }

            GD.resume();
            Serial.printf("load time: %d\n", millis() - begintime);
        }

        spritesToLoadCount = 0;
    }
}

void Render::initSpriteDraw(Sprite s, int w, int h)
{
    if (currentLoadedSprite != s)
    {
        GD.BitmapSource(s * SPRITE_BLOCK);
        GD.BitmapLayout(sprites[s].fmt, sprites[s].width * 2, sprites[s].height);
        currentLoadedSprite = s;
    }

    if (w == -1)
        w = sprites[s].width;
    if (h == -1)
        h = sprites[s].height;

    if (currentLoadedSpriteW != w || currentLoadedSpriteH != h)
    {
        GD.BitmapSize(NEAREST, BORDER, BORDER, w, h);
        currentLoadedSpriteW = w; currentLoadedSpriteH = h;
    }
}

void Render::drawStripe(int x, float dist, Sprite texture, int col)
{
    const int h = static_cast<int>(SCREEN_HEIGHT / dist);
    const int y = (SCREEN_HEIGHT / 2) - (h / 2);
    const Real sh = (Real)h / TEXTURE_SIZE;

    initSpriteDraw(texture, 2, SCREEN_HEIGHT*2);

    GD.cmd_loadidentity();
    GD.cmd_translate(F16(-col*2), F16(y*2));
    GD.cmd_scale(F16(2.0), F16(sh*2));
    GD.cmd_setmatrix();

    GD.StencilFunc(ALWAYS, stencilFromDist(dist), 255);

    GD.Vertex2f(x*2 * 16, 0 * 16);
//    Serial.printf("x/y/h/tex/col: %d/%d/%d/%d/%d\n", x, y, h, texture, col);
}

void Render::beginRender()
{
    renderStartTime = millis();

    GD.ClearStencil(255);
    GD.Clear();

    // ceiling
    GD.SaveContext();
    GD.ScissorXY(0, 0);
    GD.ScissorSize(SCREEN_WIDTH*2, SCREEN_HEIGHT*2 / 2);
    GD.ClearColorRGB(RGB(56, 56, 56));
    GD.Clear();

    // floor
    GD.ScissorXY(0, SCREEN_HEIGHT*2 / 2 - 1);
    GD.ClearColorRGB(RGB(112, 112, 112));
    GD.Clear();
    GD.RestoreContext();

    GD.Begin(BITMAPS);
    GD.AlphaFunc(GREATER, 0); // for stencil: otherwise transparency doesn't work with overlapping gfx
    GD.StencilOp(REPLACE, REPLACE);

    // needs to be reset every render frame to ensure up to date settings
    currentLoadedSprite = SPRITE_NONE;
    currentLoadedSpriteW = currentLoadedSpriteH = -1;
}

void Render::renderWalls(void)
{
    GD.SaveContext();

    bool wasdark = false;
    for (int ray=0; ray<SCREEN_WIDTH; ++ray)
    {
        if (wasdark != rayCastInfo[ray].dark)
        {
            if (rayCastInfo[ray].dark)
                GD.ColorRGB(127, 127, 127);
            else
                GD.ColorRGB(255, 255, 255);

            wasdark = rayCastInfo[ray].dark;
        }

        drawStripe(ray, rayCastInfo[ray].hitDist, rayCastInfo[ray].texture,
                   rayCastInfo[ray].texColumn);
    }

    GD.RestoreContext();
}

void Render::renderEntities(void)
{
    GD.StencilOp(KEEP, REPLACE); // only draw stuff closer than walls

    for (uint8_t e=0; e<entityCount; ++e)
    {
        if (!(entities[e]->getFlags() & Entity::FLAG_VSIBILE))
            continue;

        int s = entities[e]->getSprite();
        if (entities[e]->getFlags() & Entity::FLAG_ROTATIONAL_SPRITE)
            s += getEntityDrawRotation(game.getPlayer().getAngle(),
                                       ((MovableEntity *)entities[e])->getAngle(),
                                       entityCastInfo[e].spriteX);

        initSpriteDraw(static_cast<Sprite>(s), entityCastInfo[e].spriteW*2, (SCREEN_HEIGHT - entityCastInfo[e].spriteY) * 2);

        GD.cmd_loadidentity();
        GD.cmd_scale(F16(entityCastInfo[e].scaleF*2), F16(entityCastInfo[e].scaleF*2));
        GD.cmd_setmatrix();

        GD.StencilFunc(GREATER, stencilFromDist(entityCastInfo[e].dist), 255);

        GD.Vertex2f(entityCastInfo[e].spriteX*2 * 16, entityCastInfo[e].spriteY*2 * 16);
    }
}

void Render::endRender()
{
    GD.finish();

    Serial.print("draw buffer: ");
    Serial.println(GD.rd16(REG_CMD_DL));

    GD.swap();

    Serial.print("render time: "); Serial.println(millis() - renderStartTime);
}

void Render::setup(const RayCast::RayCastInfo *rinfo, const RayCast::EntityCastInfo *einfo,
                   Entity **e)
{
    rayCastInfo = rinfo;
    entityCastInfo = einfo;
    entities = e;

    GD.__end();
//    SPI.endTransaction();

    Serial.println("Init SD");
    if (!sd.begin(9, SPI_FULL_SPEED))
        sd.initErrorHalt();

    valloc.setSettings(true, 10, SerialRam::SPEED_FULL);
    valloc.setPoolSize(128 * 1024);
    valloc.start();

    const uint32_t begintime2 = millis();

    for (int i=0; i<SPRITE_END; ++i)
    {
        cachedSprites[i] = valloc.alloc<uint8_t>(sprites[i].comprsize);
        loadGDFileInVMem(sprites[i].file, cachedSprites[i]);
    }

    Serial.printf("load time: %d\n", millis() - begintime2);

//    SPI.endTransaction();
    SPIFIFO.begin(6, SPI_CLOCK_24MHz);
//    SPI.beginTransaction(SPISettings(/*3000000*/20000000, MSBFIRST, SPI_MODE0));
    GD.resume();
}

void Render::render(uint32_t rayframe)
{
    collectSpritesToLoad(rayframe);
    loadSprites(rayframe);
    beginRender();
    renderWalls();
    renderEntities();
    endRender();
}

#include <EEPROM.h>
#include <SPI.h>
#include <SPIFIFO.h>
#include <GD2.h>

#include <virtmem.h>
#include <alloc/spiram_alloc.h>
#include <serialram.h>

#include "utils.h"
#include "world.h"

namespace {

const int worldMap[WORLD_SIZE][WORLD_SIZE] = {
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
    { 1, 4, 1, 1, 6, 1, 1, 1 },
    { 2, 0, 0, 5, 0, 0, 0, 7 },
    { 1, 1, 0, 1, 0, 1, 1, 1 },
    { 6, 0, 0, 0, 0, 0, 0, 3 },
    { 1, 8, 8, 0, 8, 0, 8, 1 },
    { 2, 2, 0, 0, 8, 8, 7, 1 },
    { 3, 0, 0, 0, 0, 0, 0, 5 },
    { 2, 2, 2, 2, 7, 4, 4, 4 },
};*/


inline int stencilFromDist(float dist)
{
    return min(static_cast<int>(dist * 255.0 / WORLD_MAX_LENGTH + 0.5), 255);
}

World world;

}


World::World()
{
    memset(loadedSprites, SPRITE_NONE, sizeof(loadedSprites));

    player.setAngle(0.5);
    player.setPos(1.5, 1.5);

    addStaticEntity(Vec2D(2.5, 3.5), Entity::FLAG_ENABLED, SPRITE_SOLDIER0);
    addStaticEntity(Vec2D(3.5, 3.5), Entity::FLAG_ENABLED, SPRITE_SOLDIER0);
}

void World::initSprite(Sprite s, int w, int h)
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

void World::drawStripe(int x, float dist, Sprite texture, int col)
{
    const int h = static_cast<int>(SCREEN_HEIGHT / dist);
    const int y = (SCREEN_HEIGHT / 2) - (h / 2);
    const Real sh = (Real)h / TEXTURE_SIZE;

    initSprite(texture, 2, SCREEN_HEIGHT*2);

    GD.cmd_loadidentity();
    GD.cmd_translate(F16(-col*2), F16(y*2));
    GD.cmd_scale(F16(2.0), F16(sh*2));
    GD.cmd_setmatrix();
    
    GD.StencilFunc(ALWAYS, stencilFromDist(dist), 255);
    
    GD.Vertex2f(x*2 * 16, 0 * 16);
//    Serial.printf("x/y/h/tex/col: %d/%d/%d/%d/%d\n", x, y, h, texture, col);
}

void World::rayCast()
{
    ++rayFrameNumber;

    // cast all rays here
    Real sina = sin(player.getAngle());
    Real cosa = cos(player.getAngle());
    Real u = cosa - sina;
    Real v = sina + cosa;
    Real du = 2 * sina / SCREEN_WIDTH;
    Real dv = -2 * cosa / SCREEN_WIDTH;
    
    memset(visibleCells, false, sizeof(visibleCells));

    for (int ray = 0; ray < SCREEN_WIDTH; ++ray, u += du, v += dv)
    {
        // every time this ray advances 'u' units in x direction,
        // it also advanced 'v' units in y direction
        Real uu = (u < 0) ? -u : u;
        Real vv = (v < 0) ? -v : v;
        Real duu = 1 / uu;
        Real dvv = 1 / vv;
        int stepx = (u < 0) ? -1 : 1;
        int stepy = (v < 0) ? -1 : 1;
        
        // the cell in the map that we need to check
        Real px = player.getPos().x;
        Real py = player.getPos().y;
        int mapx = static_cast<int>(px);
        int mapy = static_cast<int>(py);
        
        // the position and texture for the hit
        int texture = 0;
        Real hitdist = 0.1;
        Real texofs = 0;
        bool dark = false;

        // first hit at constant x and constant y lines
        Real distx = (u > 0) ? (mapx + 1 - px) * duu : (px - mapx) * duu;
        Real disty = (v > 0) ? (mapy + 1 - py) * dvv : (py - mapy) * dvv;
        
        // loop until we hit something
        while (texture <= 0)
        {
            if (distx > disty)
            {
                // shorter distance to a hit in constant y line
                hitdist = disty;
                disty += dvv;
                mapy += stepy;
                texture = worldMap[mapy][mapx];
                if (texture > 0) {
                    dark = true;
                    if (stepy > 0)
                    {
                        Real ofs = px + u * (mapy - py) / v;
                        texofs = ofs - floor(ofs);
                    }
                    else
                    {
                        Real ofs = px + u * (mapy + 1 - py) / v;
                        texofs = ofs - floor(ofs);
                    }
                }
            }
            else
            {
                // shorter distance to a hit in constant x line
                hitdist = distx;
                distx += duu;
                mapx += stepx;
                texture = worldMap[mapy][mapx];
                if (texture > 0)
                {
                    if (stepx > 0)
                    {
                        Real ofs = py + v * (mapx - px) / u;
                        texofs = ofs - floor(ofs);
                    }
                    else
                    {
                        Real ofs = py + v * (mapx + 1 - px) / u;
                        texofs = ceil(ofs) - ofs;
                    }
                }
            }
            
            visibleCells[mapy][mapx] = (texture == 0);
        }
        
        int col = static_cast<int>((1.0 - texofs) * TEXTURE_SIZE);
        col = constrain(col, 0, TEXTURE_SIZE - 1);
        texture = (texture - 1) % TEXTURE_COUNT;

        rayCastInfo[ray].dark = dark;
        rayCastInfo[ray].hitDist = hitdist;
        rayCastInfo[ray].texColumn = col;
        rayCastInfo[ray].texture = static_cast<Sprite>(texture);

        spriteGfxInfo[texture].lastRayFrameNumber = rayFrameNumber; // mark it visible during this frame
        if (spriteGfxInfo[texture].gfxIndex == SPRITE_NOT_LOADED) // not yet loaded into GD2 memory?
        {
            spriteGfxInfo[texture].gfxIndex = SPRITE_SHOULD_LOAD; // make sure to only add it once
            spritesToLoad[spritesToLoadCount] = rayCastInfo[ray].texture;
            ++spritesToLoadCount;
        }
    }
}

void World::loadSprites()
{
    // check for entities (world already has been checked during raycasting)
    for (uint8_t e=0; e<entityCount; ++e)
    {
        const int mapx = static_cast<int>(entities[e]->getPos().x);
        const int mapy = static_cast<int>(entities[e]->getPos().y);

        // make sure cell is visible
        if (!visibleCells[mapy][mapx])
            continue;

        const Sprite s = entities[e]->getSprite();

        spriteGfxInfo[s].lastRayFrameNumber = rayFrameNumber; // mark it visible during this frame
        if (spriteGfxInfo[s].gfxIndex == SPRITE_NOT_LOADED) // not yet loaded into GD2 memory?
        {
            spriteGfxInfo[s].gfxIndex = SPRITE_SHOULD_LOAD; // make sure to only add it once
            spritesToLoad[spritesToLoadCount] = s;
            ++spritesToLoadCount;
        }
    }

    // load new sprites into GD2 and replace old ones if possible
    if (spritesToLoadCount)
    {
        if (spritesToLoadCount > MAX_LOADED_SPRITES)
        {
            Serial.printf("WARNING!! Too many sprites need be loaded (%d, max: %d)\n",
                          spritesToLoadCount, MAX_LOADED_SPRITES);
            spritesToLoadCount = MAX_LOADED_SPRITES;
        }

        int lastGfxIndex = 0;
        for (int s=0; s<spritesToLoadCount; ++s)
        {
            // find a free slot: only sprites that need be available have a current frame number
            while (loadedSprites[lastGfxIndex] != SPRITE_NONE &&
                   spriteGfxInfo[loadedSprites[lastGfxIndex]].lastRayFrameNumber == rayFrameNumber)
                ++lastGfxIndex;

            if (loadedSprites[lastGfxIndex] != SPRITE_NONE)
                spriteGfxInfo[loadedSprites[lastGfxIndex]].gfxIndex = -1; // mark as not loaded

            const int spr = static_cast<int>(spritesToLoad[s]);
            spriteGfxInfo[spr].gfxIndex = lastGfxIndex;
            spriteGfxInfo[spr].lastRayFrameNumber = rayFrameNumber;
            loadedSprites[lastGfxIndex] = spritesToLoad[s];

            // load actual sprite
            Serial.printf("LOAD: %s @ %d\n", sprites[spritesToLoad[s]].file, lastGfxIndex);
#if 0
            const uint32_t begintime = millis();
            GD.cmd_inflate(spr * SPRITE_BLOCK);
//            GD.safeload(sprites[spr].file);
            if (!loadGDFile(sprites[spr].file))
                GD.alert("Failed to load resource!");
            Serial.printf("load time: %d\n", millis() - begintime);
#else
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

#endif
            ++lastGfxIndex; // no need to check for this (and previous) index again
        }

        spritesToLoadCount = 0;
    }
}

void World::preRender()
{
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

void World::drawWorld()
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

void World::drawEntities()
{
    GD.StencilOp(KEEP, REPLACE); // only draw stuff closer than walls

    for (uint8_t e=0; e<entityCount; ++e)
    {
        const int mapx = static_cast<int>(entities[e]->getPos().x);
        const int mapy = static_cast<int>(entities[e]->getPos().y);

        // make sure cell is visible
        if (!visibleCells[mapy][mapx])
            continue;

        const Real dx = entities[e]->getPos().x - player.getPos().x;
        const Real dy = entities[e]->getPos().y - player.getPos().y;
        const Real dist = sqrt(dx*dx + dy*dy); // sqrt needed?
        const Real sangle = atan2(dy, dx) - player.getAngle();
        const Real size = SCREEN_HEIGHT / (cos(sangle) * dist);
        const Real scalef = size / TEXTURE_SIZE;

        const int sx = tan(sangle) * SCREEN_HEIGHT;
        int sw = scalef * sprites[entities[e]->getSprite()].width;
        const int sh = scalef * sprites[entities[e]->getSprite()].height;
        const int sxl = SCREEN_WIDTH/2 - sx - (sw/2);
        const int sxt = (SCREEN_HEIGHT - sh) / 2;

        // is the sprite at least partially visible?
        if (sxl < SCREEN_WIDTH && (sxl + sw) >= 0)
        {
            if ((sxl + sw) > SCREEN_WIDTH)
                sw = SCREEN_WIDTH - sxl;

            initSprite(entities[e]->getSprite(), sw*2, (SCREEN_HEIGHT - sxt) * 2);

            GD.cmd_loadidentity();
            GD.cmd_scale(F16(scalef*2), F16(scalef*2));
            GD.cmd_setmatrix();

            GD.StencilFunc(GREATER, stencilFromDist(dist), 255);

            GD.Vertex2f(sxl*2 * 16, sxt*2 * 16);
        }
    }
}

void World::render()
{
    const uint32_t begintime = millis();

    rayCast();
    loadSprites();
    preRender();
    drawWorld();
    drawEntities();

    GD.finish();
    Serial.print("draw buffer: ");
    Serial.println(GD.rd16(REG_CMD_DL));

    GD.swap();

    Serial.print("render time: "); Serial.println(millis() - begintime);
}

void World::handleInput()
{
    if (Serial.available())
    {
        dirty = true;

        const int ch = Serial.read();
        if (ch == 'a')
            player.setAngle(player.getAngle() + (0.025 * M_PI));
        else if (ch == 'd')
            player.setAngle(player.getAngle() - (0.025 * M_PI));
        else if (ch == 'w' || ch == 's')
        {
            const Real step = (ch == 's') ? -0.25 : 0.25;
            Real dx = cos(player.getAngle()) * step;
            Real dy = sin(player.getAngle()) * step;
            Vec2D pos = player.getPos() + Vec2D(dx, dy);
            int xi = static_cast<int>(pos.x);
            int yi = static_cast<int>(pos.y);
            if (worldMap[yi][xi] == 0)
                player.setPos(pos);
        }
        else
            dirty = false;
    }
}

void World::addStaticEntity(const Vec2D &pos, Entity::Flags flags, Sprite sprite)
{
    staticEntityStore[staticEntityCount].setPos(pos);
    staticEntityStore[staticEntityCount].setFlags(flags);
    staticEntityStore[staticEntityCount].setSprite(sprite);

    entities[entityCount] = &staticEntityStore[staticEntityCount];

    ++staticEntityCount;
    ++entityCount;
}

void World::setup()
{
#if 0
    const uint32_t begintime = millis();

    for (int i=0; i<SPRITE_END; ++i)
    {
        GD.cmd_inflate(i * SPRITE_BLOCK);
        GD.safeload(sprites[i].file);
    }

    Serial.printf("load time: %d\n", millis() - begintime);
#endif

    GD.__end();

    valloc.setSettings(true, 10, SerialRam::SPEED_FULL);
    valloc.setPoolSize(128 * 1024);
    valloc.start();

    const uint32_t begintime2 = millis();

    for (int i=0; i<SPRITE_END; ++i)
    {
        cachedSprites[i] = valloc.alloc<uint8_t>(sprites[i].comprsize);
        loadGDFileInVMem(sprites[i].file, cachedSprites[i]);
    }

    Serial.printf("load time2: %d\n", millis() - begintime2);

    SPIFIFO.begin(6, SPI_CLOCK_24MHz);
    GD.resume();
}

void World::update()
{
    handleInput();

    if (dirty)
    {
        render();
        dirty = false;
    }
}


void setup()
{
    pinMode(6, OUTPUT); digitalWrite(6, HIGH);
    pinMode(7, OUTPUT); digitalWrite(7, HIGH);
    pinMode(8, OUTPUT); digitalWrite(8, HIGH);
    pinMode(9, OUTPUT); digitalWrite(9, HIGH);
    pinMode(10, OUTPUT); digitalWrite(10, HIGH);

    GD.begin();
    Serial.begin(115200);
    world.setup();
}

void loop()
{
    world.update();
    delay(5);
}

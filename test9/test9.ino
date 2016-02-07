#include <EEPROM.h>
#include <SPI.h>
#include <SPIFIFO.h>
#include <GD2.h>
#include <SdFat.h>
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


inline int stencilFromDist(float dist)
{
    return min(static_cast<int>(dist * 255.0 / WORLD_MAX_LENGTH + 0.5), 255);
}

SdFat sd;

}

World world;

World::World()
{
    memset(loadedGDSprites, SPRITE_NONE, sizeof(loadedGDSprites));

    player.setAngle(0.5);
    player.setPos(1.5, 1.5);

//    addStaticEntity(Vec2D(2.5, 3.5), Entity::FLAG_ENABLED, SPRITE_SOLDIER00);
    addEnemy(Vec2D(2.5, 3.5), SPRITE_SOLDIER00);
    addStaticEntity(Vec2D(3.5, 3.5), Entity::FLAG_ENABLED, SPRITE_SOLDIER00);
//    staticEntityStore[0].setFlag(Entity::FLAG_ROTATIONAL_SPRITE);
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
    const Real sina = sin(player.getAngle());
    const Real cosa = cos(player.getAngle());
    Real u = cosa - sina;
    Real v = sina + cosa;
    const Real du = 2 * sina / SCREEN_WIDTH;
    const Real dv = -2 * cosa / SCREEN_WIDTH;
    
    memset(visibleCells, false, sizeof(visibleCells));

    for (int ray = 0; ray < SCREEN_WIDTH; ++ray, u += du, v += dv)
    {
        // every time this ray advances 'u' units in x direction,
        // it also advanced 'v' units in y direction
        const Real uu = (u < 0) ? -u : u;
        const Real vv = (v < 0) ? -v : v;
        const Real duu = 1 / uu;
        const Real dvv = 1 / vv;
        const int stepx = (u < 0) ? -1 : 1;
        const int stepy = (v < 0) ? -1 : 1;
        
        // the cell in the map that we need to check
        const Real px = player.getPos().x;
        const Real py = player.getPos().y;
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

void World::castEntities()
{
    for (uint8_t e=0; e<entityCount; ++e)
    {
        entities[e]->unsetFlag(Entity::FLAG_VSIBILE); // until known otherwise

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

            entities[e]->setFlag(Entity::FLAG_VSIBILE);

            int s = entities[e]->getSprite();
            if (entities[e]->getFlags() & Entity::FLAG_ROTATIONAL_SPRITE)
                s += (Sprite)getEntityDrawRotation(player.getAngle(), entities[e]->getAngle(), sxl);

            spriteGfxInfo[s].lastRayFrameNumber = rayFrameNumber; // mark it visible during this frame
            if (spriteGfxInfo[s].gfxIndex == SPRITE_NOT_LOADED) // not yet loaded into GD2 memory?
            {
                spriteGfxInfo[s].gfxIndex = SPRITE_SHOULD_LOAD; // make sure to only add it once
                spritesToLoad[spritesToLoadCount] = static_cast<Sprite>(s);
                ++spritesToLoadCount;
            }
        }

        entityDrawInfo[e].spriteX = sxl;
        entityDrawInfo[e].spriteY = sxt;
        entityDrawInfo[e].spriteW = sw;
        entityDrawInfo[e].dist = dist;
        entityDrawInfo[e].scaleF = scalef;
    }
}

void World::loadSprites()
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
                       spriteGfxInfo[loadedGDSprites[replaceind]].lastRayFrameNumber == rayFrameNumber)
                    ++replaceind;
                gdind = replaceind;
                ++replaceind; // no need to check for this (and previous) index again
            }

            if (loadedGDSprites[gdind] != SPRITE_NONE)
                spriteGfxInfo[loadedGDSprites[gdind]].gfxIndex = -1; // mark as not loaded

            const int spr = static_cast<int>(spritesToLoad[s]);
            spriteGfxInfo[spr].gfxIndex = gdind;
            spriteGfxInfo[spr].lastRayFrameNumber = rayFrameNumber;
            loadedGDSprites[gdind] = spritesToLoad[s];

            // load actual sprite
            Serial.printf("LOAD: %s @ %d\n", sprites[spritesToLoad[s]].file, gdind);
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
        if (!(entities[e]->getFlags() & Entity::FLAG_VSIBILE))
            continue;

        int s = entities[e]->getSprite();
        if (entities[e]->getFlags() & Entity::FLAG_ROTATIONAL_SPRITE)
            s += getEntityDrawRotation(player.getAngle(), entities[e]->getAngle(),
                                       entityDrawInfo[e].spriteX);

        initSprite(static_cast<Sprite>(s), entityDrawInfo[e].spriteW*2, (SCREEN_HEIGHT - entityDrawInfo[e].spriteY) * 2);

        GD.cmd_loadidentity();
        GD.cmd_scale(F16(entityDrawInfo[e].scaleF*2), F16(entityDrawInfo[e].scaleF*2));
        GD.cmd_setmatrix();

        GD.StencilFunc(GREATER, stencilFromDist(entityDrawInfo[e].dist), 255);

        GD.Vertex2f(entityDrawInfo[e].spriteX*2 * 16, entityDrawInfo[e].spriteY*2 * 16);
    }
}

void World::render()
{
    const uint32_t begintime = millis();

    rayCast();
    castEntities();
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
            move(&player, (ch == 's') ? -0.25 : 0.25);
        else
            dirty = false;
    }
}

bool World::isBlocking(int x, int y) const
{
    // UNDONE: check for blocking sprites
    return (x < 0 || x >= WORLD_SIZE || y < 0 || y >= WORLD_SIZE || worldMap[y][x] != 0);
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

void World::addStaticEntity(const Vec2D &pos, Entity::Flags flags, Sprite sprite)
{
    staticEntityStore[staticEntityCount].setPos(pos);
    staticEntityStore[staticEntityCount].setFlags(flags);
    staticEntityStore[staticEntityCount].setSprite(sprite);

    entities[entityCount] = &staticEntityStore[staticEntityCount];

    ++staticEntityCount;
    ++entityCount;
}

void World::addEnemy(const Vec2D &pos, Sprite sprite)
{
    enemyStore[enemyCount].setPos(pos);
    enemyStore[enemyCount].setFlags(Entity::FLAG_ENABLED | Entity::FLAG_ROTATIONAL_SPRITE);
    enemyStore[enemyCount].setSprite(sprite);

    entities[entityCount] = &enemyStore[enemyCount];

    ++enemyCount;
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

    Serial.printf("load time2: %d\n", millis() - begintime2);

//    SPI.endTransaction();
    SPIFIFO.begin(6, SPI_CLOCK_24MHz);
//    SPI.beginTransaction(SPISettings(/*3000000*/20000000, MSBFIRST, SPI_MODE0));
    GD.resume();
}

void World::update()
{
    handleInput();

    for (int i=0; i<enemyCount; ++i)
        enemyStore[i].think();

    if (dirty)
    {
        render();
        dirty = false;
    }
}

void World::move(Object *obj, Real speed)
{
    const Real dx = cos(obj->getAngle()) * speed;
    const Real dy = sin(obj->getAngle()) * speed;
    const Vec2D pos = obj->getPos() + Vec2D(dx, dy);
    obj->setPos(checkCollision(obj->getPos(), pos, 0.35)); // UNDONE: magic nr
}


void setup()
{
    pinMode(6, OUTPUT); digitalWrite(6, HIGH);
    pinMode(7, OUTPUT); digitalWrite(7, HIGH);
    pinMode(8, OUTPUT); digitalWrite(8, HIGH);
    pinMode(9, OUTPUT); digitalWrite(9, HIGH);
    pinMode(10, OUTPUT); digitalWrite(10, HIGH);

    delay(2000);

    Serial.begin(115200);

    Serial.println("Init GD");
    GD.begin(~GD_STORAGE); // use SD fat lib instead of GD2 SD library

    Serial.println("Init world");
    world.setup();
}

void loop()
{
    world.update();
    delay(5);
}

#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

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
    player.setAngle(0.5);
    player.setPos(1.5, 1.5);

    addStaticEntity(Vec2D(2.5, 3.5), Entity::FLAG_ENABLED);
    addStaticEntity(Vec2D(3.5, 3.5), Entity::FLAG_ENABLED);
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

#if 0
    if (currentLoadedSprite != texture)
    {
        // need to set it for every handle (texture), cache to save some GD gfx list memory
        GD.BitmapHandle(texture);
        GD.BitmapSize(NEAREST, BORDER, BORDER, /*WALL0_WIDTH*2*/2, SCREEN_HEIGHT*2);
        currentLoadedSprite = texture;
    }
#endif

    initSprite(texture, 2, SCREEN_HEIGHT*2);

    const int y = (SCREEN_HEIGHT / 2) - (h / 2);
    
    const Real sh = (Real)h / TEXTURE_SIZE;
    
    GD.cmd_loadidentity();
    GD.cmd_translate(F16(-col*2), F16(y*2)); // need to translate: normally images drawn at y<0 are simply discarded completely
    GD.cmd_scale(F16(2.0), F16(sh*2));
    GD.cmd_setmatrix();
    
    GD.StencilFunc(ALWAYS, stencilFromDist(dist), 255);
    
    GD.Vertex2f(x*2 * 16, 0 * 16);
//    Serial.printf("x/y/h/tex/col: %d/%d/%d/%d/%d\n", x, y, h, texture, col);
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

void World::rayCast()
{
    // cast all rays here
    Real sina = sin(player.getAngle());
    Real cosa = cos(player.getAngle());
    Real u = cosa - sina;
    Real v = sina + cosa;
    Real du = 2 * sina / SCREEN_WIDTH;
    Real dv = -2 * cosa / SCREEN_WIDTH;
    
    GD.SaveContext();
    
    memset(visibleCells, false, sizeof(visibleCells));

    bool wasdark = false;

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

        if (wasdark != dark)
        {
            if (dark)
                GD.ColorRGB(127, 127, 127);
            else
                GD.ColorRGB(255, 255, 255);

            wasdark = dark;
        }

        drawStripe(ray, hitdist, static_cast<Sprite>(texture), col);
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
        int sw = scalef * SOLDIER0_IMG_WIDTH;
        const int sh = scalef * SOLDIER0_IMG_HEIGHT;
        const int sxl = SCREEN_WIDTH/2 - sx - (sw/2);
        const int sxt = (SCREEN_HEIGHT - sh) / 2;

        // is the sprite at least partially visible?
        if (sxl < SCREEN_WIDTH && (sxl + sw) >= 0)
        {
            if ((sxl + sw) > SCREEN_WIDTH)
                sw = SCREEN_WIDTH - sxl;

            initSprite(SPRITE_SOLDIER0, sw*2, (SCREEN_HEIGHT - sxt) * 2);

            GD.cmd_loadidentity();
            GD.cmd_scale(F16(scalef*2), F16(scalef*2));
            GD.cmd_setmatrix();

            GD.StencilFunc(GREATER, stencilFromDist(dist), 255);

            /*GD.BitmapHandle(SOLDIER_HANDLE);
            GD.BitmapSize(NEAREST, BORDER, BORDER, sw*2, (SCREEN_HEIGHT - sxt) * 2);*/

            GD.Vertex2f(sxl*2 * 16, sxt*2 * 16);
        }
    }
}

void World::render()
{
    const uint32_t begintime = millis();

    preRender();
    rayCast();
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

void World::addStaticEntity(const Vec2D &pos, Entity::Flags flags)
{
    staticEntityStore[staticEntityCount].setPos(pos);
    staticEntityStore[staticEntityCount].setFlags(flags);

    entities[entityCount] = &staticEntityStore[staticEntityCount];

    ++staticEntityCount;
    ++entityCount;
}

void World::setup()
{
    const uint32_t begintime = millis();

#if 0
    for (int i=0; i<8; ++i)
    {
        GD.BitmapHandle(i);
        GD.BitmapSource(i * WALL0_ASSETS_END);
//        GD.BitmapSize(NEAREST, BORDER, BORDER, WALL0_IMG_WIDTH, WALL0_IMG_HEIGHT);
        GD.BitmapLayout(RGB565, WALL0_IMG_WIDTH * 2, WALL0_IMG_HEIGHT);
        GD.cmd_inflate(i * WALL0_ASSETS_END);
        char fname[20];
        sprintf(fname, "wall%d.gd2", i);
        GD.safeload(fname);
    }

    GD.BitmapHandle(8);
    GD.BitmapSource(8 * WALL0_ASSETS_END);
//        GD.BitmapSize(NEAREST, BORDER, BORDER, WALL0_IMG_WIDTH, WALL0_IMG_HEIGHT);
    GD.BitmapLayout(ARGB1555, SOLDIER0_IMG_WIDTH * 2, SOLDIER0_IMG_HEIGHT);
    GD.cmd_inflate(8 * WALL0_ASSETS_END);
    GD.safeload("soldier0.gd2");
#endif

    for (int i=0; i<SPRITE_END; ++i)
    {
//        GD.BitmapHandle(i);
        GD.BitmapSource(i * WALL0_ASSETS_END);
        GD.BitmapLayout(sprites[i].fmt, sprites[i].width * 2, sprites[i].height);
        GD.cmd_inflate(i * SPRITE_BLOCK);
        GD.safeload(sprites[i].file);
    }

    Serial.printf("load time: %d\n", millis() - begintime);
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
    GD.begin();
    Serial.begin(115200);
    world.setup();
}

void loop()
{
    world.update();
    delay(5);
}

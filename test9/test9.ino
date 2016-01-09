#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#include "assets.h"
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

void World::drawStripe(int x, float dist, int texture, int col)
{
    const int h = static_cast<int>(SCREEN_HEIGHT / dist);
    
    if (lastWallTexture != texture)
    {
        // need to set it for every handle (texture), cache to save some GD gfx list memory
        GD.BitmapHandle(texture);
        GD.BitmapSize(NEAREST, BORDER, BORDER, WALL0_WIDTH*2, SCREEN_HEIGHT*2);
        lastWallTexture = texture;
    }
    
    const int y = (SCREEN_HEIGHT / 2) - (h / 2);
    
    const Real sh = (Real)h / TEXTURE_SIZE;
    
    GD.cmd_loadidentity();
    GD.cmd_translate(F16(0), F16(y*2)); // need to translate: normally images drawn at y<0 are simply discarded completely
    GD.cmd_scale(F16(2.0), F16(sh*2));
    GD.cmd_setmatrix();
    
    GD.StencilFunc(ALWAYS, stencilFromDist(dist), 255);
    
    GD.Vertex2ii(x*2, /*y*/0, texture, col);
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

        drawStripe(ray, hitdist, texture, col);
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
        int sw = scalef * SOLDIER_WIDTH;
        const int sh = scalef * SOLDIER_HEIGHT;
        const int sxl = SCREEN_WIDTH/2 - sx - (sw/2);
        const int sxt = (SCREEN_HEIGHT - sh) / 2;

        // is the sprite at least partially visible?
        if (sxl < SCREEN_WIDTH && (sxl + sw) >= 0)
        {
            lastWallTexture = -1; // reset because we're messing with settings

            if ((sxl + sw) > SCREEN_WIDTH)
                sw = SCREEN_WIDTH - sxl;

            GD.cmd_loadidentity();
            GD.cmd_scale(F16(scalef*2), F16(scalef*2));
            GD.cmd_setmatrix();

            GD.StencilFunc(GREATER, stencilFromDist(dist), 255);

            GD.BitmapHandle(SOLDIER_HANDLE);
            GD.BitmapSize(NEAREST, BORDER, BORDER, sw*2, (SCREEN_HEIGHT - sxt) * 2);

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
    LOAD_ASSETS();
    Serial.begin(115200);
}

void loop()
{
    world.update();
    delay(5);
}

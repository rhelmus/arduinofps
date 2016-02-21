#include "entity.h"
#include "game.h"
#include "raycast.h"

#include <string.h>

void RayCast::rayCast(const uint8_t world[][8])
{
    memset(visibleCells, false, sizeof(visibleCells));
    ++rayFrameNumber;

    const Real sina = sin(game.getPlayer().getAngle());
    const Real cosa = cos(game.getPlayer().getAngle());
    const Real du = 2 * sina / SCREEN_WIDTH;
    const Real dv = -2 * cosa / SCREEN_WIDTH;

    Real u = cosa - sina;
    Real v = sina + cosa;

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
        const Real px = game.getPlayer().getPos().x;
        const Real py = game.getPlayer().getPos().y;
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
                texture = world[mapy][mapx];
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
                texture = world[mapy][mapx];
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

        if (col < 0)
            col = 0;
        else if (col > TEXTURE_SIZE-1)
            col = TEXTURE_SIZE - 1;
        texture = (texture - 1) % TEXTURE_COUNT;

        rayCastInfo[ray].dark = dark;
        rayCastInfo[ray].hitDist = hitdist;
        rayCastInfo[ray].texColumn = col;
        rayCastInfo[ray].texture = static_cast<Sprite>(texture);
    }
}

void RayCast::castEntities(Entity **entities, int ecount)
{
    for (int e=0; e<ecount; ++e)
    {
        entities[e]->unsetFlag(Entity::FLAG_VSIBILE); // until known otherwise

        const int mapx = static_cast<int>(entities[e]->getPos().x);
        const int mapy = static_cast<int>(entities[e]->getPos().y);

        // make sure cell is visible
        if (!visibleCells[mapy][mapx])
            continue;

        const Real dx = entities[e]->getPos().x - game.getPlayer().getPos().x;
        const Real dy = entities[e]->getPos().y - game.getPlayer().getPos().y;
        const Real dist = sqrt(dx*dx + dy*dy); // sqrt needed?
        const Real sangle = atan2(dy, dx) - game.getPlayer().getAngle();
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
                s += (Sprite)getEntityDrawRotation(game.getPlayer().getAngle(),
                                                   ((MovableEntity *)entities[e])->getAngle(), sxl);

            entityCastInfo[e].spriteX = sxl;
            entityCastInfo[e].spriteY = sxt;
            entityCastInfo[e].spriteW = sw;
            entityCastInfo[e].dist = dist;
            entityCastInfo[e].scaleF = scalef;
        }
    }
}

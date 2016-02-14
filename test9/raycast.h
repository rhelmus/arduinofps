#ifndef RAYCAST_H
#define RAYCAST_H

#include <stdint.h>

#include "defs.h"

class Entity;

class RayCast
{
public:
    struct RayCastInfo
    {
        Real hitDist;
        Sprite texture;
        uint8_t texColumn;
        bool dark;
    };

    struct EntityCastInfo
    {
        int16_t spriteX, spriteY, spriteW;
        Real dist, scaleF;
    };

private:
    uint32_t rayFrameNumber = 0;
    bool visibleCells[WORLD_SIZE][WORLD_SIZE];
    RayCastInfo rayCastInfo[SCREEN_WIDTH];
    EntityCastInfo entityCastInfo[MAX_ENTITIES];

public:
    RayCast(void) { }

    void rayCast(const uint8_t world[][8]);
    void castEntities(Entity **entities, int ecount);
    uint32_t getRayFrameNumber(void) const { return rayFrameNumber; }
    bool visibleCell(int x, int y) const { return visibleCells[y][x]; }
    const RayCastInfo *getRayCastInfo(void) const { return rayCastInfo; }
    const EntityCastInfo *getEntityCastInfo(void) const { return entityCastInfo; }
};

#endif // RAYCAST_H

#ifndef RENDERQT_H
#define RENDERQT_H

#ifndef ARDUINO

#include "defs.h"
#include "gfx.h"
#include "raycast.h"

class Render
{
    int entityCount = 0;

public:
    Render(void);

    void setup(const RayCast::RayCastInfo *rinfo, const RayCast::EntityCastInfo *einfo,
               Entity **e) { }
    void setEntityCount(int ecount) { entityCount = ecount; }
    void render(uint32_t rayframe) { }
};

#endif

#endif // RENDERQT_H

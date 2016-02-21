#ifndef RENDERQT_H
#define RENDERQT_H

#ifndef ARDUINO

#include "defs.h"
#include "gfx.h"
#include "raycast.h"

#include <QWidget>

class RenderWidget : public QWidget
{
protected:
    virtual void paintEvent(QPaintEvent *event);

public:
    RenderWidget(QWidget *p) : QWidget(p) { }
};

class Render
{
    // bound data from RayCast/World
    const RayCast::RayCastInfo *rayCastInfo;
    const RayCast::EntityCastInfo *entityCastInfo;
    Entity **entities;
    int entityCount = 0;

    QWidget *widget;

public:
    Render(void);

    void setup(const RayCast::RayCastInfo *rinfo, const RayCast::EntityCastInfo *einfo,
               Entity **e);
    void setEntityCount(int ecount) { entityCount = ecount; }
    void render(uint32_t rayframe);
};

#endif

#endif // RENDERQT_H

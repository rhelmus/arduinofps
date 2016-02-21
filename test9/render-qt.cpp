#ifndef ARDUINO

#include "render-qt.h"

void RenderWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
}


Render::Render()
{
}

void Render::setup(const RayCast::RayCastInfo *rinfo, const RayCast::EntityCastInfo *einfo,
                   Entity **e)
{
    rayCastInfo = rinfo;
    entityCastInfo = einfo;
    entities = e;

    widget = new QWidget;
    widget->resize(480, 272);
    widget->show();
}

void Render::render(uint32_t rayframe)
{

}

#endif

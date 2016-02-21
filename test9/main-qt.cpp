#ifndef ARDUINO

#include <QApplication>
#include <QBasicTimer>

#include "game.h"

namespace {

class GameUpdater : public QObject
{
    QBasicTimer timer;

protected:
    virtual void timerEvent(QTimerEvent *event)
    {
        if (event->timerId() == timer.timerId())
            game.update();
        else
            QObject::timerEvent(event);
    }

public:
    GameUpdater(QObject *p) : QObject(p) { timer.start(5, this); }
};

}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    game.setup();
    GameUpdater gu(&a);

    return a.exec();
}

#endif

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = test9
TEMPLATE = app


SOURCES += enemy.cpp \
    entity.cpp \
    game.cpp \
    player.cpp \
    raycast.cpp \
    render.cpp \
    utils.cpp \
    world.cpp \
    render-qt.cpp \
    main-qt.cpp
        

HEADERS  += defs.h \
    enemy.h \
    entity.h \
    game.h \
    gfx.h \
    player.h \
    raycast.h \
    render.h \
    utils.h \
    world.h \
    render-qt.h

OTHER_FILES += \
    prep_assets.py

QMAKE_CXXFLAGS +=  -std=gnu++11

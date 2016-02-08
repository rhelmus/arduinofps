#ifndef DEFS_H
#define DEFS_H

#include "math.h"

enum
{
    SCREEN_WIDTH = 200,
    SCREEN_HEIGHT = 100,
    WORLD_SIZE = 8,
    WORLD_MAX_LENGTH = static_cast<int>(sqrt(WORLD_SIZE*WORLD_SIZE + WORLD_SIZE*WORLD_SIZE) + 0.5),
    TEXTURE_SIZE = 64,
    TEXTURE_COUNT = 8,
    MAX_STATIC_ENTITIES = 128,
    MAX_ENEMIES = 128,
    SPRITE_BLOCK = 8192UL,
    MAX_LOADED_SPRITES = 28,
    SPRITE_NOT_LOADED = -1,
    SPRITE_SHOULD_LOAD = -2,
};

typedef float Real;

#endif // DEFS_H

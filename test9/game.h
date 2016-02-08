#ifndef GAME_H
#define GAME_H

#include "defs.h"
#include "enemy.h"
#include "player.h"
#include "world.h"

class Game
{
    World world;
    Player player;
    Entity staticEntityStore[MAX_STATIC_ENTITIES];
    Enemy enemyStore[MAX_ENEMIES];
    uint8_t staticEntityCount = 0;
    uint8_t enemyCount = 0;

    void handleInput(void);
    void addStaticEntity(const Vec2D &pos, Entity::Flags flags, Sprite sprite);
    void addEnemy(const Vec2D &pos, Sprite sprite);

public:
    Game(void);

    void setup(void);
    void update(void);

    World &getWorld(void) { return world; }
    Player &getPlayer(void) { return player; }
};

extern Game game;

#endif // GAME_H

#include "Arduino.h"
#include "game.h"

Game game;

Game::Game()
{
    player.setAngle(0.5);
    player.setPos(1.5, 1.5);

    addEnemy(Vec2D(2.5, 3.5), SPRITE_SOLDIER00);
    addStaticEntity(Vec2D(3.5, 3.5), Entity::FLAG_ENABLED, SPRITE_SOLDIER00);
}

void Game::handleInput()
{
    if (Serial.available())
    {
        bool dirty = true;

        const int ch = Serial.read();
        if (ch == 'a')
            player.setAngle(player.getAngle() + (0.025 * M_PI));
        else if (ch == 'd')
            player.setAngle(player.getAngle() - (0.025 * M_PI));
        else if (ch == 'w' || ch == 's')
            player.move((ch == 's') ? -0.25 : 0.25);
        else
            dirty = false;

        if (dirty)
            world.markDirty();
    }
}

void Game::addStaticEntity(const Vec2D &pos, Entity::Flags flags, Sprite sprite)
{
    staticEntityStore[staticEntityCount].setPos(pos);
    staticEntityStore[staticEntityCount].setFlags(flags);
    staticEntityStore[staticEntityCount].setSprite(sprite);
    world.addEntity(&staticEntityStore[staticEntityCount]);
    ++staticEntityCount;
}

void Game::addEnemy(const Vec2D &pos, Sprite sprite)
{
    enemyStore[enemyCount].setPos(pos);
    enemyStore[enemyCount].setFlags(Entity::FLAG_ENABLED | Entity::FLAG_ROTATIONAL_SPRITE);
    enemyStore[enemyCount].setSprite(sprite);
    world.addEntity(&enemyStore[enemyCount]);
    ++enemyCount;
}

void Game::setup()
{
    Serial.println("Init GD");
    GD.begin(~GD_STORAGE); // use SD fat lib instead of GD2 SD library

    Serial.println("Init world");
    world.setup();
}

void Game::update()
{
    handleInput();

    for (int i=0; i<enemyCount; ++i)
        enemyStore[i].think();

    world.update();
}

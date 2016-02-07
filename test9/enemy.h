#ifndef ENEMY
#define ENEMY

#include "entity.h"

class Enemy : public MovableEntity
{
public:
    void think(void);
};


#endif // ENEMY

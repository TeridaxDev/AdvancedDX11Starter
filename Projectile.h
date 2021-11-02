#pragma once
#include "GameEntity.h"
class Projectile :
    public GameEntity
{
private:


public:

    float gravity = 0;

    float velocityX;
    float velocityY;
    float velocityZ;

    float lifespan;
    float age;

    Projectile(Mesh* mesh, Material* material, float lifespan);

    void SetVelocity(float x, float y, float z, float g);
    void Update(float dt);

    bool dead = false;

};


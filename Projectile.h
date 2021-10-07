#pragma once
#include "GameEntity.h"
class Projectile :
    public GameEntity
{
private:

    float velocityX;
    float velocityY;
    float velocityZ;

    float gravity = 0;

    float lifespan;
    float age;

public:

    Projectile(Mesh* mesh, Material* material, float lifespan);

    void SetVelocity(float x, float y, float z, float g);
    void Update(float dt);

    bool dead = false;

};


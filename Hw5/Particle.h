#pragma once
#include "cvec.h"
#include "RigidBody.h"

class Particle
{
public:
   Particle();
   Particle(RigidBody* data, int lifeSpan, int x, int y, int z, float xSpeed, float ySpeed,
      float zSpeed, float gravity);
   ~Particle();

   // Functions
   void updateParticle(int ms);
   static Particle* createRandomParticle();
   static bool testAlive(Particle* p);

   // Variables
   int life, lifeSpan;
   int x, y, z; 
   int colorPhase, colorLife;
   float colorAlpha;
   float xPos, yPos, zPos, xSpeed, ySpeed, zSpeed, gravity;
   Cvec3 color;
   RigidBody* data;
   bool isAlive;
};
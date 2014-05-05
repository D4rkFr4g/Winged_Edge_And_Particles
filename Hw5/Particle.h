#pragma once
#include "cvec.h"
#include "RigidBody.h"

class Particle
{
public:
   // Constructors
   Particle();
   Particle(RigidBody* data, int lifeSpan, float x, float y, float z, float xSpeed, float ySpeed,
      float zSpeed, float gravity);
   ~Particle();

   // Functions
   void updateParticle(int ms);
   static Particle* createRandomParticle();
   static bool testAlive(Particle* p);

   // Variables
   int life, lifeSpan;
   int colorPhase, colorLife;
   float x, y, z;
   float colorAlpha;
   float xSpeed, ySpeed, zSpeed, gravity;
   Cvec3 color;
   RigidBody* data;
   bool isAlive;
};
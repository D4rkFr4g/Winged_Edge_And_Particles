#include "Particle.h"


Particle::Particle()
{
   life = 0;
   lifeSpan = 0;
   x = 0;
   y = 0;
   z = 0;
   xPos = x;
   yPos = y;
   zPos = z;
   xSpeed = 0;
   ySpeed = 0;
   zSpeed = 0;
   gravity = 0;
   color = Cvec3(1, 1, 1);
   colorPhase = 2;
   colorAlpha = 1;
   colorLife = 0;
   isAlive = true;
}
/*-----------------------------------------------*/
Particle::Particle(RigidBody* data, int lifeSpan, int x, int y, int z, float xSpeed, float ySpeed, 
   float zSpeed, float gravity)
{
   this->data = data;
   this->life = 0;
   this->lifeSpan = lifeSpan;
   this->x = x;
   this->y = y;
   this->z = z;
   this->xPos = x;
   this->yPos = y;
   this->zPos = z;
   this->xSpeed = xSpeed;
   this->ySpeed = ySpeed;
   this->zSpeed = zSpeed;
   this->gravity = gravity;
   this->color = Cvec3(1, 1, 1);
   this->colorPhase = 2;
   this->colorAlpha = 1;
   this->colorLife = 0;
   this->isAlive = true;
}
/*-----------------------------------------------*/
Particle::~Particle()
{
}
/*-----------------------------------------------*/
void Particle::updateParticle(int ms)
{
   /*	PURPOSE:		Updates position, color, and life of particle
      RECEIVES:	ms - Time in ms since last frame
      RETURNS:
      REMARKS:
   */

   this;
   // Update Life timers
   life += ms;
   if (life >= lifeSpan)
      isAlive = false;

   colorLife += ms;
   if (colorLife >= (lifeSpan / 3.0))
   {
      colorLife %= (int)(lifeSpan / 3.0);
      colorPhase--;
      if (colorPhase < 0)
         colorPhase = 0;
   }

   // Update position
   ySpeed -= gravity;

   xPos += xSpeed;
   yPos += ySpeed;
   zPos += zSpeed;
   x = floor(xPos);
   y = floor(yPos);
   z = floor(zPos);

   //data->rtf.setTranslation(Cvec3(x, y, z));

   // Update color
   colorAlpha = 1 - (colorLife / (lifeSpan / 3.0));
   color[colorPhase] = colorAlpha;
   data->color = color;
}
/*-----------------------------------------------*/
bool Particle::testAlive(Particle* p)
{
   /*	PURPOSE:		Tests whether particle is alive
      RECEIVES:	p - particle to be tested
      RETURNS:    true if particle is alive otherwise false
      REMARKS:    Created mostly to be used with std vector remove_if
   */

   return p->isAlive;
}
/*-----------------------------------------------*/
Particle* Particle::createRandomParticle()
{
   Particle* p = new Particle();

   // Random Life
   p->lifeSpan = rand() % 2000 + 2000; // Between 2-4 seconds
   
   // Random Speed
   p->xSpeed = (rand() % 10) / 10.0; // Between 0-1;
   p->ySpeed = (rand() % 10) / 10.0;
   p->zSpeed = (rand() % 10) / 10.0;
   
   // Random direction
   if (rand() % 1 == 1)
      p->xSpeed *= -1;
   if (rand() % 1 == 1)
      p->ySpeed *= -1;
   if (rand() % 1 == 1)
      p->zSpeed *= -1;

   // Random gravity variation
   float low = 0;
   float high = 2.5;
   float diff = high - low;
   float random = (float) rand() / RAND_MAX;
   float r = random * diff;
   p->gravity = 0;// 9.8 - r;

   return p;
}
/*-----------------------------------------------*/
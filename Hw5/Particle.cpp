#include "Particle.h"


Particle::Particle()
{
   life = 0;
   lifeSpan = 0;
   x = 0;
   y = 0;
   z = 0;
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
Particle::Particle(RigidBody* data, int lifeSpan, float x, float y, float z, float xSpeed, float ySpeed,
   float zSpeed, float gravity)
{
   this->data = data;
   this->life = 0;
   this->lifeSpan = lifeSpan;
   this->x = x;
   this->y = y;
   this->z = z;
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

   x += xSpeed;
   y += ySpeed;
   z += zSpeed;

   data->rtf.setTranslation(Cvec3(x, y, z));

   // Update color
   colorAlpha = (float)( 1 - (colorLife / (lifeSpan / 3.0)));
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
   /*	PURPOSE:		Creates a particle with a random speed, direction, and life
      RECEIVES:
      RETURNS:    pointer to Random particle
      REMARKS:    starts at origin and shoots out from there
      */

   Particle* p = new Particle();

   // Random Life
   p->lifeSpan = rand() % 3001 + 2000; // Between 2-5 seconds

   // Random Speed
   float low = (float) 0.01;
   float high = (float) 0.05;
   float diff = high - low;
   float random = (float)rand() / RAND_MAX;
   float r = random * diff;
   p->xSpeed = low + r;
   random = (float)rand() / RAND_MAX;
   r = (float) (random * (diff + 0.05));
   p->ySpeed = low + r;
   random = (float)rand() / RAND_MAX;
   r = random * diff;
   p->zSpeed = low + r;

   // Random direction
   if ((rand() % 2) == 1)
      p->xSpeed *= -1;
   if ((rand() % 2) == 1)
      p->ySpeed *= -1;
   if ((rand() % 2) == 1)
      p->zSpeed *= -1;

   // Random gravity variation
   low = (float) 0.0005;
   high = (float) 0.001;
   diff = high - low;
   random = (float)rand() / RAND_MAX;
   r = random * diff;
   p->gravity = low + r;

   return p;
}
/*-----------------------------------------------*/
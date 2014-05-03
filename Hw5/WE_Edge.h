#pragma once
#include "RigidBody.h"

class WE_Vertex;
class WE_Face;

class WE_Edge
{
public:
   // Constructors
   WE_Edge();
   ~WE_Edge();

   // Variables
   WE_Vertex *vert1, *vert2;
   WE_Face *aFace, *bFace;
   WE_Edge *aPrev, *aNext, *bPrev, *bNext;
   RigidBody data;
};


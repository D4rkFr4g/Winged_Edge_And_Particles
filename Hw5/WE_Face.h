#pragma once
#include <vector>
#include "WE_Edge.h"
#include "RigidBody.h"

class WE_Face
{
public:
   // Constructors
   WE_Face();
   ~WE_Face();

   // Variables
   std::vector<WE_Edge*> edges;
   RigidBody data;
};


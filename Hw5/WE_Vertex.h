#pragma once
#include <vector>
#include "WE_Edge.h"
#include "cvec.h"
#include "RigidBody.h"



class WE_Vertex
{
public:
   // Constructors
   WE_Vertex(void);
   ~WE_Vertex();

   // Variables
   std::vector<WE_Edge*> edges;
   RigidBody data;
};


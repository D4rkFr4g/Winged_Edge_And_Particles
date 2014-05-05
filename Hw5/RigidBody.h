#ifndef _RIGID_BODY_H_
#define _RIGID_BODY_H_

#pragma once
#include <string>
#include "matrix4.h"
#include "MySdlApplication.h"

class MySdlApplication;

class RigidBody
{
public:
   // Constructors
   RigidBody();
   RigidBody(RigTForm rtf_, Matrix4 scale_, RigidBody **children_, MySdlApplication::Geometry *geom_, Cvec3 color_, int material_);
   ~RigidBody();

   // Functions
   void drawRigidBody(RigTForm invEyeRbt);
   void draw(RigTForm respectFrame_, Matrix4 respectScale_);
   void draw(Matrix4 respectFrame_);

   // Variables
   RigTForm rtf;
   Matrix4 scale;
   RigidBody **children;
   Cvec3 color;
   Cvec3 originalColor;
   MySdlApplication::Geometry *geom;
   GLenum mode;
   string name;
   int numOfChildren;
   int material;
   bool isVisible;
   bool isChildVisible;

   // Enum
   enum { DIFFUSE, SOLID, TEXTURE, NORMAL, ANISOTROPY, CUBE, SHINY };
};
#endif
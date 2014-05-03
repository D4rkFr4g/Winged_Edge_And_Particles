#include "RigidBody.h"


/*-----------------------------------------------*/
RigidBody::RigidBody()
{
   /*	PURPOSE:		Constructor for RigidBody object
   RECEIVES:
   RETURNS:		RigidBody object
   REMARKS:
   */

   rtf = RigTForm();
   scale = Matrix4();
   children = NULL;
   numOfChildren = 0;
   color = Cvec3(.5, .5, .5);
   geom = NULL;
   isVisible = true;
   isChildVisible = true;
   material = SOLID;
   mode = GL_TRIANGLES;
}
/*-----------------------------------------------*/
RigidBody::~RigidBody()
{
   /*	PURPOSE:		Destructor for RigidBody object
   RECEIVES:
   RETURNS:
   REMARKS:
   */

   for (int i = 0; i < numOfChildren; i++)
   {
      delete children[i];
   }
   delete children;
   delete geom;
}
/*-----------------------------------------------*/
RigidBody::RigidBody(RigTForm rtf_, Matrix4 scale_, RigidBody **children_, MySdlApplication::Geometry *geom_, Cvec3 color_, int material_)
{
   /*	PURPOSE:		Constructor for RigidBody object
   RECEIVES:	rtf_ - RigTForm for the RigidBody
   scale_ - Matrix representing RigidBody scale
   children_ - Pointer to an array of Child RigidBody objects
   geom_ - Geometry type of object
   color_ - Color to draw object
   material_ - Type of shader material to draw object with
   RETURNS:		RigidBody object
   REMARKS:
   */

   rtf = rtf_;
   scale = scale_;
   children = children_;
   numOfChildren = 0;
   geom = geom_;
   color = color_;
   isVisible = true;
   material = material_;
   mode = GL_TRIANGLES;
}
/*-----------------------------------------------*/
void RigidBody::drawRigidBody(RigTForm invEyeRbt)
{
   /*	PURPOSE:		Draw the RigidBody object
   RECEIVES:	invEyeRbt -  Inverse Eye Frame to use
   RETURNS:
   REMARKS:		Recursive starter function
   */

   RigTForm respectFrame = invEyeRbt;
   draw(respectFrame, Matrix4());
}
/*-----------------------------------------------*/
void RigidBody::draw(RigTForm respectFrame_, Matrix4 respectScale_)
{
   /*	PURPOSE:		Draws the RigidBody with respect to parent object
   RECEIVES:	respectFrame_ - Parent Object frame
   respectScale_ - Parent Object scale
   RETURNS:
   REMARKS:		 Recursive function
   */

   const MySdlApplication::ShaderState& curSS = MySdlApplication::setupShader(material);

   safe_glUniform3f(curSS.h_uColor, (GLfloat)color[0], (GLfloat)color[1], (GLfloat)color[2]);

   // Draw Parent
   RigTForm respectFrame = respectFrame_ * rtf;
   Matrix4 respectScale = respectScale_ * scale;
   Matrix4 MVM = RigTForm::makeTRmatrix(respectFrame, respectScale);

   if (isVisible)
   {
      if (geom != NULL)
         geom->draw(curSS, MVM, mode);
   }

   //Draw Children
   if (isChildVisible)
   {
      for (int i = 0; i < numOfChildren; i++)
      {
         children[i]->draw(respectFrame, respectScale);
      }
   }

}
/*-----------------------------------------------*/
void RigidBody::draw(Matrix4 respectFrame_)
{
   /*	PURPOSE:		Draws the RigidBody with respect to parent Frame
   RECEIVES:	respectFrame_ - Parent Object frame
   RETURNS:
   REMARKS:		Recursive Function
   */

   const MySdlApplication::ShaderState& curSS = MySdlApplication::setupShader(material);
   safe_glUniform3f(curSS.h_uColor, (GLfloat)color[0], (GLfloat)color[1], (GLfloat)color[2]);

   //Draw parent
   Matrix4 respectFrame = respectFrame_ * RigTForm::makeTRmatrix(rtf, scale);
   Matrix4 MVM = respectFrame;

   if (isVisible)
   {
      if (geom != NULL)
         geom->draw(curSS, MVM, mode);
   }

   //Draw Children
   for (int i = 0; i < numOfChildren; i++)
   {
      children[i]->draw(respectFrame);
   }
}
/*-----------------------------------------------*/
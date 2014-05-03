/******************************************************
* Copyright (c):   2014, All Rights Reserved.
* Project:         CS 116B Homework #5
* File:            MySdlAppliation.cpp
* Purpose:         To experiment with winged-edge representation. To experiment
with a simple particle system.
* Start date:      5/2/14
* Programmer:      Zane Melcho
*
******************************************************
*/

#include "MySdlApplication.h"
#include "RigidBody.h"

using namespace std;
using namespace tr1;

// Forward Declarations
struct RigidBody;
struct ShaderState;
static const ShaderState& setupShader(int material);
static Matrix4 makeProjectionMatrix();

// Enum
enum { DIFFUSE, SOLID, TEXTURE, NORMAL, ANISOTROPY, CUBE, SHINY };

// Constants
const bool MySdlApplication::G_GL2_COMPATIBLE = false;
static const float G_CAM_ROTATION = 1;
static const float G_FRUST_MIN_FOV = 60.0;  //A minimal of 60 degree field of view
static const unsigned char* KB_STATE = NULL;
static const int G_NUM_OF_OBJECTS = 2; //Number of objects to be drawn
static const int G_NUM_SHADERS = 7;
static const char * const G_SHADER_FILES[G_NUM_SHADERS][2] =
{
   { "./Shaders/basic-gl3.vshader", "./Shaders/diffuse-gl3.fshader" },
   { "./Shaders/basic-gl3.vshader", "./Shaders/solid-gl3.fshader" },
   { "./Shaders/basic-gl3.vshader", "./Shaders/texture-gl3.fshader" },
   { "./Shaders/basic-gl3.vshader", "./Shaders/normal-gl3.fshader" },
   { "./Shaders/basic-gl3.vshader", "./Shaders/anisotropy-gl3.fshader" },
   { "./Shaders/basic-gl3.vshader", "./Shaders/cube-gl3.fshader" },
   { "./Shaders/basic-gl3.vshader", "./Shaders/shiny-gl3.fshader" }
};
static const char * const G_SHADER_FILES_GL2[G_NUM_SHADERS][2] =
{
   { "./Shaders/basic-gl2.vshader", "./Shaders/diffuse-gl2.fshader" },
   { "./Shaders/basic-gl2.vshader", "./Shaders/solid-gl2.fshader" },
   { "./Shaders/basic-gl2.vshader", "./Shaders/texture-gl2.fshader" },
   { "./Shaders/basic-gl2.vshader", "./Shaders/normal-gl2.fshader" },
   { "./Shaders/basic-gl2.vshader", "./Shaders/anisotropy-gl2.fshader" },
   { "./Shaders/basic-gl2.vshader", "./Shaders/cube-gl2.fshader" },
   { "./Shaders/basic-gl2.vshader", "./Shaders/shiny-gl2.fshader" }
};

static const float G_FRUST_NEAR = -0.1f;    // near plane
static const float G_FRUST_FAR = -100.0f;    // far plane
static const float G_GROUND_Y = 0.0f;      // y coordinate of the ground
static const float G_GROUND_SIZE = 50.0f;   // half the ground length

// Global variables
int g_windowWidth = 640;
int g_windowHeight = 480;
unsigned char kbPrevState[SDL_NUM_SCANCODES] = { 0 };

static bool g_mouseClickDown = false;    // is the mouse button pressed
static bool g_mouseLClickButton, g_mouseRClickButton, g_mouseMClickButton;
static int g_mouseClickX, g_mouseClickY; // coordinates for mouse click event
static int g_activeShader = 0;
SDL_TimerID g_animationTimer;
static bool g_isAnimating = false;
static WE_Vertex* g_EcodTM_Vertex;

static float g_frustFovY = G_FRUST_MIN_FOV; // FOV in y direction

static shared_ptr<GlTexture> g_tex0;

/*-----------------------------------------------*/

// --------- Scene

static Cvec3 g_light1(2.0, 3.0, 14.0);
//static Cvec3 g_light2(-2000.0, -3000.0, -5000.0);
// define light positions in world space
static RigTForm g_skyRbt = RigTForm(Cvec3(0.0, 0.0, 0.0)); // Initialized here but set in initCamera()
static RigTForm g_eyeRbt = g_skyRbt;
static Cvec3f g_objectColors[1] = { Cvec3f(1, 0, 0) };

///////////////// END OF G L O B A L S ///////////////////////

/*-----------------------------------------------*/
static vector<shared_ptr<MySdlApplication::ShaderState> > g_shaderStates;
// our global shader states
// Vertex buffer and index buffer associated with the ground and cube geometry
static shared_ptr<MySdlApplication::Geometry> g_ground, g_cube, g_sphere, g_triangle;
static RigidBody g_rigidBodies[G_NUM_OF_OBJECTS]; // Array that holds each Rigid Body Object
/*-----------------------------------------------*/
static MySdlApplication::Geometry* initCube()
{
   /*	PURPOSE:		Sets up index and vertex buffers and calls Geometrymaker for a cube
      RECEIVES:
      RETURNS:		Geometry - returns Geometry object
      REMARKS:
      */

   int ibLen, vbLen;
   getCubeVbIbLen(vbLen, ibLen);

   // Temporary storage for cube geometry
   vector<GenericVertex> vtx(vbLen);
   vector<unsigned short> idx(ibLen);

   makeCube(1, vtx.begin(), idx.begin());
   return new MySdlApplication::Geometry(&vtx[0], &idx[0], vbLen, ibLen);
}
/*-----------------------------------------------*/
static MySdlApplication::Geometry* initSpheres()
{
   /*	PURPOSE:		Sets up index and vertex buffers and calls geometrymaker for a sphere
      RECEIVES:
      RETURNS:		Geometry - returns Geometry object
      REMARKS:
      */

   int slices = 20;
   int stacks = 20;
   float radius = 1;
   int ibLen, vbLen;
   getSphereVbIbLen(slices, stacks, vbLen, ibLen);

   // Temporary storage for Sphere geometry
   vector<GenericVertex> vtx(vbLen);
   vector<unsigned short> idx(ibLen);

   makeSphere(radius, slices, stacks, vtx.begin(), idx.begin());
   return new MySdlApplication::Geometry(&vtx[0], &idx[0], vbLen, ibLen);
}
/*-----------------------------------------------*/
static MySdlApplication::Geometry* initPoint()
{
   /*	PURPOSE:		Sets up index and vertex buffers and calls geometrymaker for a point
      RECEIVES:   
      RETURNS:		Geometry - returns Geometry object representing a point
      REMARKS:
   */
   
   int ibLen = 1;
   int vbLen = 1;

   // Temporary storage for plane geometry
   vector<GenericVertex> vtx(vbLen);
   vector<unsigned short> idx(ibLen);

   makePoint(vtx.begin(), idx.begin());
   return new MySdlApplication::Geometry(&vtx[0], &idx[0], vbLen, ibLen);
}
/*-----------------------------------------------*/
static MySdlApplication::Geometry* initLine()
{
   /*	PURPOSE:		Sets up index and vertex buffers and calls geometrymaker for a line
   RECEIVES:
   RETURNS:		Geometry - returns Geometry object representing a line
   REMARKS:
   */

   int ibLen = 2;
   int vbLen = 2;

   // Temporary storage for plane geometry
   vector<GenericVertex> vtx(vbLen);
   vector<unsigned short> idx(ibLen);

   makeLine(vtx.begin(), idx.begin());
   return new MySdlApplication::Geometry(&vtx[0], &idx[0], vbLen, ibLen);
}
/*-----------------------------------------------*/
static MySdlApplication::Geometry* initCylinders()
{
   /*	PURPOSE:		Sets up index and vertex buffers and calls geometrymaker for a cylinder
      RECEIVES:
      RETURNS:		Geometry - returns Geometry object
      REMARKS:
      */

   float radius = 1;
   float height = 1;
   int slices = 20;
   int ibLen, vbLen;
   getCylinderVbIbLen(slices, vbLen, ibLen);

   // Temporary storage for Cylinder geometry
   vector<GenericVertex> vtx(vbLen);
   vector<unsigned short> idx(ibLen);

   makeCylinder(slices, radius, height, vtx.begin(), idx.begin());
   return new MySdlApplication::Geometry(&vtx[0], &idx[0], vbLen, ibLen);
}
/*-----------------------------------------------*/
static MySdlApplication::Geometry* initPlane()
{
   /*	PURPOSE:		Sets up index and vertex buffers and calls geometrymaker for a plane
      RECEIVES:
      RETURNS:		Geometry - returns Geometry object
      REMARKS:
      */

   int ibLen, vbLen;
   getPlaneVbIbLen(vbLen, ibLen);

   // Temporary storage for plane geometry
   vector<GenericVertex> vtx(vbLen);
   vector<unsigned short> idx(ibLen);

   makePlane(1, vtx.begin(), idx.begin());
   return new MySdlApplication::Geometry(&vtx[0], &idx[0], vbLen, ibLen);
}
/*-----------------------------------------------*/
static RigidBody* buildCube()
{
   /*	PURPOSE:		Builds a cube object
      RECEIVES:
      RETURNS:		RigidBody - Returns RigidBody object containing a cube
      REMARKS:		Cube is inside an invisible container object
      */

   float width = 1;
   float height = 1;
   float thick = 1;

   RigTForm rigTemp = RigTForm(Cvec3(0, 0, 0));
   Matrix4 scaleTemp = Matrix4();

   // Make container
   RigidBody *container = new RigidBody(RigTForm(), Matrix4(), NULL, initCube(), Cvec3(0.5, 0.5, 0.5), DIFFUSE);
   container->isVisible = false;
   container->name = "container";

   // Make Cube
   rigTemp = RigTForm(Cvec3(0, 0, 0));
   scaleTemp = Matrix4::makeScale(Cvec3(width, height, thick));

   RigidBody *cube = new RigidBody(rigTemp, scaleTemp, NULL, initCube(), Cvec3(1, 0, 0), TEXTURE);
   cube->name = "cube";

   //Setup Children
   container->numOfChildren = 1;
   container->children = new RigidBody*[container->numOfChildren];
   container->children[0] = cube;

   return container;

}
/*-----------------------------------------------*/
static RigidBody* buildEpilepticCubeOfDoomTM()
{
   /*	PURPOSE:		Builds a Lander object
      RECEIVES:
      RETURNS:		RigidBody - Returns RigidBody object containing a Lander
      REMARKS:		Lander is inside an invisible container object
      */

   Cvec3 grey = Cvec3(.4, .4, .4);
   Cvec3 red = Cvec3(1, 0, 0);
   Cvec3 blue = Cvec3(0, 0, 1);
   Cvec3 black = Cvec3(0, 0, 0);

   float width = 1;
   float height = 1;
   float thick = 1;

   RigTForm rigTemp = RigTForm(Cvec3(0, 0, 0));
   Matrix4 scaleTemp = Matrix4();

   // Make container
   RigidBody *container = new RigidBody(RigTForm(), Matrix4(), NULL, initCube(), Cvec3(0.5, 0.5, 0.5), DIFFUSE);
   container->isVisible = false;
   container->name = "container";

   // Make Vertices (8)
   const int numVertices = 8;
   RigidBody **vertices = new RigidBody*[numVertices];

   float h = 1.0;
   Cvec3 points[numVertices] = { Cvec3(-h, h, h), Cvec3(h, h, h), Cvec3(h,h,-h), Cvec3(-h,h,-h),
      Cvec3(-h, -h, h), Cvec3(h, -h, h), Cvec3(h, -h, -h), Cvec3(-h, -h, -h) };

   scaleTemp = Matrix4::makeScale(Cvec3(2, 2, 2));
   for (int i = 0; i < numVertices; i++)
   {
      rigTemp = RigTForm(points[i]);
    
      vertices[i] = new RigidBody(rigTemp, scaleTemp, NULL, initPoint(), red, SOLID);
      vertices[i]->mode = GL_POINTS;
   }

   // Make Edges (12)
   const int numEdges = 12;
   RigidBody **edges = new RigidBody*[numEdges];

   Cvec3 edgeTranslations[numEdges] = { Cvec3(-h, h, 0), Cvec3(0, h, h), Cvec3(h,h,0), Cvec3(0,h,-h),
      Cvec3(-h, 0, h), Cvec3(h, 0, h), Cvec3(-h, 0, -h), Cvec3(h, 0, -h),
      Cvec3(-h, -h, 0), Cvec3(0, -h, h), Cvec3(h, -h, 0), Cvec3(0, -h, -h) };
   Cvec3 edgeRotations[numEdges] = {Cvec3(0,90,0), Cvec3(0,0,0), Cvec3(0,-90,0), Cvec3(0,180,0),
      Cvec3(0, 0, 90), Cvec3(0, 0, 90), Cvec3(0, 180, 90), Cvec3(0,180,90),
      Cvec3(0, 90, 0), Cvec3(0, 0, 0), Cvec3(0, -90, 0), Cvec3(0, 180, 0) };

   for (int i = 0; i < numEdges; i++)
   {
      rigTemp = RigTForm(edgeTranslations[i]);
      rigTemp.setRotation(rigTemp.getRotation() * Quat().makeRotation(edgeRotations[i]));
      edges[i] = new RigidBody(rigTemp, scaleTemp, NULL, initLine(), black, SOLID);
      edges[i]->mode = GL_LINES;
   }

   // Make Faces (6)
   const int numFaces = 6;
   RigidBody **faces = new RigidBody*[numFaces];

   Cvec3 faceTranslations[numFaces] = {Cvec3(0,h,0), Cvec3(0,0,h), Cvec3(h,0,0), Cvec3(0,0,-h),
      Cvec3(-h,0,0), Cvec3(0,-h,0)};
   Cvec3 faceRotations[numFaces] = {Cvec3(0,0,0), Cvec3(90,0,0), Cvec3(0,0,-90), Cvec3(-90,0,0),
      Cvec3(0,0,90), Cvec3(180,0,0)};

   for (int i = 0; i < numFaces; i++)
   {
      rigTemp = RigTForm(faceTranslations[i]);
      rigTemp.setRotation(rigTemp.getRotation() * Quat().makeRotation(faceRotations[i]));
      faces[i] = new RigidBody(rigTemp, scaleTemp, NULL, initPlane(), blue, SOLID);
      faces[i]->mode = GL_TRIANGLES;
   }

   //Setup Children
   container->numOfChildren = numVertices + numEdges + numFaces;
   container->children = new RigidBody*[container->numOfChildren];
   
   int i = 0;
   for (int j = 0; j < numVertices; j++)
      container->children[i + j] = vertices[j];

   i += numVertices;
   for (int j = 0; j < numEdges; j++)
      container->children[i + j] = edges[j];

   i += numEdges;
   for (int j = 0; j < numFaces; j++)
      container->children[i + j] = faces[j];

   return container;
}
/*-----------------------------------------------*/
static void initEpilepticCubeOfDoomTM()
{
   /*	PURPOSE:		Creates and adds an Epileptic Cube of DoomTM to the array of RigidBody objects
      RECEIVES:
      RETURNS:
      REMARKS:
      */

   RigidBody *EcodTM;
   EcodTM = buildEpilepticCubeOfDoomTM();
   EcodTM->rtf.setTranslation(Cvec3(0, 0, 0));
   g_rigidBodies[1] = *EcodTM;
}
/*-----------------------------------------------*/
static void initGround()
{
   /*	PURPOSE:		Buils the Generic Vertices for the ground
   RECEIVES:
   RETURNS:
   REMARKS:
   */

   // A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
   GenericVertex vtx[4] =
   {
      GenericVertex(-G_GROUND_SIZE, G_GROUND_Y, -G_GROUND_SIZE, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0),
      GenericVertex(-G_GROUND_SIZE, G_GROUND_Y, G_GROUND_SIZE, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0),
      GenericVertex(G_GROUND_SIZE, G_GROUND_Y, G_GROUND_SIZE, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0),
      GenericVertex(G_GROUND_SIZE, G_GROUND_Y, -G_GROUND_SIZE, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0),
   };
   unsigned short idx[] = { 0, 1, 2, 0, 2, 3 };
   g_ground.reset(new MySdlApplication::Geometry(&vtx[0], &idx[0], 4, 6));
}
/*-----------------------------------------------*/
Uint32 animateLander(Uint32 interval, void *param)
{

   static float stopwatch = 0;
   float msecsPerFrame = interval;
   static int animationPart = 0;
   static int animationParts = 2;
   static bool isAnimating = true;
   static float totalSecs = 5.0;
   static float totalTime = (totalSecs / animationParts) * 1000;
   static float elapsedTime = 0;
   static bool isFirstEntry = true;
   static RigTForm start;
   static RigTForm end;

   // Used to reset variables every time animation is run
   if (isFirstEntry)
   {
      start = g_rigidBodies[1].rtf;
      end = RigTForm(Cvec3(0, 5, 0)) * start;
      elapsedTime = totalTime;

      isFirstEntry = false;
   }

   //Handles which part of animation is currently running
   if (elapsedTime >= totalTime)
   {
      if (animationPart == 0)
      {
         start = g_rigidBodies[1].rtf;
         end = RigTForm(Cvec3(0, 3, 0)) * start;
      }
      else if (animationPart == 1)
      {
         start = g_rigidBodies[1].rtf;
         end = RigTForm(Cvec3(0, -3, 0)) * start;
      }
      else
      {
         isAnimating = false;
      }

      elapsedTime = 0;
      animationPart++;
   }

   if (isAnimating)
   {
      float alpha = elapsedTime / totalTime;

      //Handle Translation Interpolation
      Cvec3 startVec = start.getTranslation();
      Cvec3 temp = end.getTranslation() - startVec;
      g_rigidBodies[1].rtf.setTranslation(startVec + (temp * alpha));

      elapsedTime += msecsPerFrame;

      //Time total animation
      stopwatch += msecsPerFrame;
   }
   else
   {
      // Reset variables
      isAnimating = true;
      //cout << "Stopwatch Camera = " << (stopwatch) / 1000 << "\n"; // Display final time not counting first and last frame
      stopwatch = 0;
      animationPart = 0;
      isFirstEntry = true;

      SDL_RemoveTimer(g_animationTimer);
      g_isAnimating = false;
   }

   return interval;
}
/*-----------------------------------------------*/
static void sendProjectionMatrix(const MySdlApplication::ShaderState& curSS,
   const Matrix4& projMatrix)
{
   /*	PURPOSE:		Sends projection matrix to OpenGL for shader use
      RECEIVES:	curSS - The current ShaderState to be used for drawing
      projMatrix - The projection matrix to be used
      RETURNS:
      REMARKS:
      */

   // takes a projection matrix and send to the the shaders
   GLfloat glmatrix[16];
   projMatrix.writeToColumnMajorMatrix(glmatrix); // send projection matrix
   safe_glUniformMatrix4fv(curSS.h_uProjMatrix, glmatrix);
}
/*-----------------------------------------------*/
static void sendModelViewNormalMatrix(const MySdlApplication::ShaderState& curSS,
   const Matrix4& MVM, const Matrix4& NMVM)
{
   /*	PURPOSE:		Sends the regular and normal Model View Matrix to OpenGL for shader use
      RECEIVES:	curSS -	The current shader state to be used to draw
      MVM -		The model view matrix to be used
      NMVM -	The normal model view matrix to be used
      RETURNS:
      REMARKS:
      */

   // takes MVM and its normal matrix to the shaders
   GLfloat glmatrix[16];
   MVM.writeToColumnMajorMatrix(glmatrix); // send MVM
   safe_glUniformMatrix4fv(curSS.h_uModelViewMatrix, glmatrix);

   NMVM.writeToColumnMajorMatrix(glmatrix); // send NMVM
   safe_glUniformMatrix4fv(curSS.h_uNormalMatrix, glmatrix);
}
/*-----------------------------------------------*/
static void updateFrustFovY()
{
   /*	PURPOSE:		Updates the Frustum field of view
      RECEIVES:
      RETURNS:
      REMARKS:
      */

   if (g_windowWidth >= g_windowHeight)
      g_frustFovY = G_FRUST_MIN_FOV;
   else
   {
      const float RAD_PER_DEG = 0.5f * (float)CS175_PI / 180.0f;
      g_frustFovY = (float)atan2((float)sin(G_FRUST_MIN_FOV * RAD_PER_DEG) * (float)g_windowHeight
         / (float)g_windowWidth,
         (float)cos(G_FRUST_MIN_FOV * RAD_PER_DEG)) / RAD_PER_DEG;
   }
}
/*-----------------------------------------------*/
static float angleBetween(Cvec3 vectorOne, Cvec3 vectorTwo)
{
   /*	PURPOSE:		Finds the angle between two vectors
      RECEIVES:	vectorOne - First vector to use
      vectorTwo - Second vector to use
      RETURNS:		angle between the vectors in degrees
      REMARKS:
      */

   float temp = dot(vectorOne, vectorTwo);
   float vOneNorm = norm(vectorOne);
   float vTwoNorm = norm(vectorTwo);
   temp = (temp / (vOneNorm * vTwoNorm));
   temp = acos(temp) * 180;
   temp /= M_PI;

   //cout << "angle = " << temp << "\n";
   //Matrix4::print(Matrix4::makeXRotation(temp));

   return temp;
}
/*-----------------------------------------------*/
static float lookAt(Cvec3 eyePosition, Cvec3 upPosition)
{
   /*	PURPOSE:		Finds the rotation in degrees the camera needs to move towards origin
      RECEIVES:	eyePosition - Vector describing camera position
      upPosition - Vector describing up or y-basis
      RETURNS:		degrees camera needs to rotate towards origin
      REMARKS:
      */

   return -(90 - angleBetween(eyePosition, upPosition));
}
/*-----------------------------------------------*/
static void lookAtOrigin()
{
   /*	PURPOSE:		Rotates camera to look at the origin
      RECEIVES:
      RETURNS:
      REMARKS:
      */

   // Set angle to look at the origin
   Cvec3 eye = g_eyeRbt.getTranslation();
   Cvec3 up = Cvec3(0, 1, 0);
   g_eyeRbt.setRotation(Quat().makeXRotation(lookAt(eye, up)));
}
/*-----------------------------------------------*/
static void initCamera()
{
   /*	PURPOSE:		Initializes the camera position
      RECEIVES:
      RETURNS:
      REMARKS:
      */

   Cvec3 eye = Cvec3(0.0, 2.0, 5.0);
   g_skyRbt.setTranslation(eye);
   g_eyeRbt = g_skyRbt;
   lookAtOrigin();
}
/*-----------------------------------------------*/
static void rotateAboutOrigin(RigTForm* rbt, float degrees)
{
   /*	PURPOSE:		Rotates RigiBody around origin
      RECEIVES:	rbt - Rigid Body transform representing object
      degrees - angle of rotation in degrees
      RETURNS:
      REMARKS:
      */

   Cvec4 toOrigin = (Cvec4)rbt->getTranslation();
   Quat origin = Quat(0, toOrigin[0], toOrigin[1], toOrigin[2]);
   Quat rot = Quat().makeYRotation(degrees);
   Quat temp = rot * origin * inv(rot);

   // Local Rotation
   Quat objQuat = rbt->getRotation();
   Quat rotation = rot * objQuat;
   rbt->setRotation(rotation);

   // Rotation about origin
   rbt->setTranslation(Cvec3(temp[1], temp[2], temp[3]));
}
/*-----------------------------------------------*/
static Matrix4 makeProjectionMatrix()
{
   /*	PURPOSE:		Builds a projection matrix for the scene
      RECEIVES:
      RETURNS:		Matrix4 - Matrix corresponding to the Projection
      REMARKS:
      */

   return Matrix4::makeProjection(g_frustFovY,
      g_windowWidth / static_cast <double> (g_windowHeight),
      G_FRUST_NEAR, G_FRUST_FAR);
}
/*-----------------------------------------------*/
static void initGLState()
{
   /*	PURPOSE:		Initializes OpenGL
      RECEIVES:
      RETURNS:
      REMARKS:
      */

   glClearColor((GLclampf)(128. / 255.), (GLclampf)(200. / 255.), (GLclampf)(255. / 255.), (GLclampf) 0.);
   glClearDepth(0.);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glPixelStorei(GL_PACK_ALIGNMENT, 1);
   glCullFace(GL_BACK);
   glEnable(GL_CULL_FACE);
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_GREATER);
   glReadBuffer(GL_BACK);
   if (!MySdlApplication::G_GL2_COMPATIBLE)
      glEnable(GL_FRAMEBUFFER_SRGB);
}
/*-----------------------------------------------*/
static void initShaders()
{
   /* PURPOSE:		Initializes Shaders to be used
      RECEIVES:
      RETURNS:
      REMARKS:
      */

   g_shaderStates.resize(G_NUM_SHADERS);
   for (int i = 0; i < G_NUM_SHADERS; ++i)
   {
      if (MySdlApplication::G_GL2_COMPATIBLE)
      {
         g_shaderStates[i].reset(new MySdlApplication::ShaderState(G_SHADER_FILES_GL2[i][0],
            G_SHADER_FILES_GL2[i][1]));
      }
      else
      {
         g_shaderStates[i].reset(new MySdlApplication::ShaderState(G_SHADER_FILES[i][0],
            G_SHADER_FILES[i][1]));
      }
   }
}
/*-----------------------------------------------*/
static void initGeometry()
{
   /*	PURPOSE:		Initializes all Geometry objects to be drawn
      RECEIVES:
      RETURNS:
      REMARKS:
      */

   //initGround();
   initEpilepticCubeOfDoomTM();
}
/*-----------------------------------------------*/
static void loadTexture(GLuint type, GLuint texHandle, const char *filename, int* width, int* height)
{
   /*	PURPOSE:		Load a texture to use in SDL
      RECEIVES:	type - Type of texture to load
      texHandle - OpenGL handle to texture
      filename - texture file to load
      RETURNS:
      REMARKS:
      */

   int texWidth, texHeight;
   vector<PackedPixel> pixData;

   //    ppmRead(filename, texWidth, texHeight, pixData);
   SDL_Surface* newSurface = IMG_Load(filename); // read in image
   SDL_Surface* returnSurface;
   if (newSurface == NULL)
   {
      cout << ":_( Surface null" << endl;
      return;
   }
   returnSurface = SDL_ConvertSurfaceFormat(newSurface,
      SDL_PIXELFORMAT_RGB24, 0); // need to convert to RGB from RGBA
   texWidth = returnSurface->w;
   texHeight = returnSurface->h;

   if (width)
      *width = texWidth;
   if (height)
      *height = texHeight;

   glActiveTexture(type);
   glBindTexture(GL_TEXTURE_2D, texHandle);
   //    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth,
   //                 texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, &pixData[0]);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth,
      texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, returnSurface->pixels);
   SDL_FreeSurface(newSurface);
   checkGlErrors();
}
/*-----------------------------------------------*/
static void loadCubeTexture(GLuint type, GLuint texHandle,
   const char *ppmFilename1, const char *ppmFilename2,
   const char *ppmFilename3, const char *ppmFilename4,
   const char *ppmFilename5, const char *ppmFilename6)
{
   /*	PURPOSE:		Loads textures necessary for a cube map
      RECEIVES:	type - Type of texture to load
      texHandle - OpenGL handle to texture
      ppmFilename1 - First face texture to load
      ppmFilename2 - Second face texture to load
      ppmFilename3 - Third face texture to load
      ppmFilename4 - Fourth face texture to load
      ppmFilename5 - Fifth face texture to load
      ppmFilename6 - Sixth face texture to load
      RETURNS:
      REMARKS:
      */

   int texWidth, texHeight;
   vector<PackedPixel> pixData1, pixData2, pixData3,
      pixData4, pixData5, pixData6;

   ppmRead(ppmFilename1, texWidth, texHeight, pixData1);
   ppmRead(ppmFilename2, texWidth, texHeight, pixData2);
   ppmRead(ppmFilename3, texWidth, texHeight, pixData3);
   ppmRead(ppmFilename4, texWidth, texHeight, pixData4);
   ppmRead(ppmFilename5, texWidth, texHeight, pixData5);
   ppmRead(ppmFilename6, texWidth, texHeight, pixData6);
   glActiveTexture(type);
   glBindTexture(GL_TEXTURE_CUBE_MAP, texHandle);

   glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0,
      GL_RGB, texWidth, texHeight, 0,
      GL_RGB, GL_UNSIGNED_BYTE, &pixData1[0]);
   glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0,
      GL_RGB, texWidth, texHeight, 0,
      GL_RGB, GL_UNSIGNED_BYTE, &pixData2[0]);
   glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0,
      GL_RGB, texWidth, texHeight, 0,
      GL_RGB, GL_UNSIGNED_BYTE, &pixData3[0]);
   glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0,
      GL_RGB, texWidth, texHeight, 0,
      GL_RGB, GL_UNSIGNED_BYTE, &pixData4[0]);
   glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0,
      GL_RGB, texWidth, texHeight, 0,
      GL_RGB, GL_UNSIGNED_BYTE, &pixData5[0]);
   glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0,
      GL_RGB, texWidth, texHeight, 0,
      GL_RGB, GL_UNSIGNED_BYTE, &pixData6[0]);
   checkGlErrors();
}
/*-----------------------------------------------*/
static void loadSphereNormalTexture(GLuint type, GLuint texHandle)
{
   /*	PURPOSE:		Creates textures to be used for a normal map
      RECEIVES:	type - The type of texture
      texHandle - OpenGL handl to texture
      RETURNS:
      REMARKS:
      */

   int width = 512, height = 512;
   vector<PackedPixel> pixels;
   float x = 0;
   float y = 0;
   float z = 0;
   float squareRootThree = sqrt((float)3);
   float invRootThree = 1 / squareRootThree;

   pixels.resize(width * height);
   for (int row = height - 1; row >= 0; row--)
   {
      for (int l = 0; l < width; l++)
      {
         PackedPixel &p = pixels[row * width + l];
         x = invRootThree * ((float)(row - width / 2) / (width / 2));
         y = invRootThree * ((float)(l - height / 2) / (height / 2));
         z = sqrt(1 - x*x - y*y);
         p.r = (unsigned char)(255 * (x + 1) / 2);
         p.g = (unsigned char)(255 * (y + 1) / 2);
         p.b = (unsigned char)(255 * (z + 1) / 2);
      }
   }

   glActiveTexture(type);
   glBindTexture(GL_TEXTURE_2D, texHandle);
   glTexImage2D(GL_TEXTURE_2D, 0, MySdlApplication::G_GL2_COMPATIBLE ? GL_RGB : GL_SRGB, width,
      height, 0, GL_RGB, GL_UNSIGNED_BYTE, &pixels[0]);
   checkGlErrors();
}
/*-----------------------------------------------*/
static void initTextures()
{
   /*	PURPOSE:		Initializes all textures to use with OpenGL
      RECEIVES:
      RETURNS:
      REMARKS:
      */

   g_tex0.reset(new GlTexture());

   loadTexture(GL_TEXTURE0, *g_tex0, "./Images/cool_texture.png", NULL, NULL);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, *g_tex0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

   /*g_tex1.reset(new GlTexture());

   loadSphereNormalTexture(GL_TEXTURE1, *g_tex1);

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, *g_tex1);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);*/

   ////Crater
   //g_tex2.reset(new GlTexture());

   //loadTexture(GL_TEXTURE2, *g_tex2, "./Images/crater2.png", g_sampleWidth, g_sampleHeight);

   //glActiveTexture(GL_TEXTURE2);
   //glBindTexture(GL_TEXTURE_2D, *g_tex2);
   //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}
/*-----------------------------------------------*/
static void drawStuff()
{
   /*	PURPOSE:		Draws all objects
      RECEIVES:
      RETURNS:
      REMARKS:
      */

   // short hand for current shader state
   const MySdlApplication::ShaderState& curSS = *g_shaderStates[g_activeShader];

   // build & send proj. matrix to vshader
   const Matrix4 projmat = makeProjectionMatrix();
   sendProjectionMatrix(curSS, projmat);

   // use the skyRbt as the eyeRbt
   //const Matrix4 eyeRbt = g_skyRbt;
   const RigTForm invEyeRbt = inv(g_eyeRbt);

   const Cvec3 eyeLight1 = Cvec3(invEyeRbt * Cvec4(g_light1, 1));
   //const Cvec3 eyeLight2 = Cvec3(invEyeRbt * Cvec4(g_light2, 1)); // g_light2 position in eye coordinates
   // G_LIGHT1 position in eye coordinates

   safe_glUniform3f(curSS.h_uLight, (GLfloat)eyeLight1[0], (GLfloat)eyeLight1[1], (GLfloat)eyeLight1[2]);
   //safe_glUniform3f(curSS.h_uLight2, (GLfloat) eyeLight2[0], (GLfloat) eyeLight2[1], (GLfloat) eyeLight2[2]);

   safe_glUniform1i(curSS.h_uTexUnit0, 0);
   //safe_glUniform1i(curSS.h_uTexUnit1, 1);
   //safe_glUniform1i(curSS.h_uTexUnit2, 2);

   // Draw Ground
   const RigTForm groundRbt = RigTForm();  // identity
   Matrix4 MVM = RigTForm::makeTRmatrix(invEyeRbt * groundRbt, Matrix4());
   Matrix4 NMVM = normalMatrix(MVM);
   sendModelViewNormalMatrix(curSS, MVM, NMVM);
   safe_glUniform3f(curSS.h_uColor, (GLfloat) 0.1, (GLfloat) 0.95, (GLfloat) 0.1); // set color
   if (g_ground != NULL)
      g_ground->draw(curSS, GL_TRIANGLES);

   // Draw all Rigid body objects
   for (int i = 0; i < G_NUM_OF_OBJECTS; i++)
      g_rigidBodies[i].drawRigidBody(invEyeRbt);
}
/*-----------------------------------------------*/
const MySdlApplication::ShaderState& MySdlApplication::setupShader(int material)
{
   /*	PURPOSE:		Sets up Shader based on material
      RECEIVES:	material - enum value of shader to be used
      RETURNS:		curSS - ShaderState to be used to draw object
      REMARKS:
      */

   // Current Shader State
   glUseProgram(g_shaderStates[material]->program);
   const MySdlApplication::ShaderState& curSS = *g_shaderStates[material];

   safe_glUniform1i(curSS.h_uTexUnit0, 0);
   //safe_glUniform1i(curSS.h_uTexUnit1, 1);
   //safe_glUniform1i(curSS.h_uTexUnit2, 2);

   // Build & send proj. matrix to vshader
   const Matrix4 projmat = makeProjectionMatrix();
   sendProjectionMatrix(curSS, projmat);

   // Use the g_eyeRbt as the eyeRbt;
   const RigTForm eyeRbt = g_eyeRbt;
   const RigTForm invEyeRbt = inv(eyeRbt);

   const Cvec3 eyeLight1 = Cvec3(invEyeRbt * Cvec4(g_light1, 1)); // g_light1 position in eye coordinates
   safe_glUniform3f(curSS.h_uLight, (GLfloat)eyeLight1[0], (GLfloat)eyeLight1[1], (GLfloat)eyeLight1[2]);

   //const Cvec3 eyeLight2 = Cvec3(invEyeRbt * Cvec4(g_light2, 1)); // g_light2 position in eye coordinates
   //safe_glUniform3f(curSS.h_uLight2, (GLfloat) eyeLight2[0], (GLfloat) eyeLight2[1], (GLfloat) eyeLight2[2]);

   const RigTForm identityRbt = RigTForm();
   Matrix4 MVM = RigTForm::makeTRmatrix(invEyeRbt * identityRbt, Matrix4());
   Matrix4 NMVM = normalMatrix(MVM);

   sendModelViewNormalMatrix(curSS, MVM, NMVM);

   return curSS;
}
/*-----------------------------------------------*/
static void reshape(const int w, const int h)
{
   /*	PURPOSE:		Resizes everything based on new window dimensions
      RECEIVES:	w - Width of window
      h - Height of window
      RETURNS:
      REMARKS:
      */

   g_windowWidth = w;
   g_windowHeight = h;
   glViewport(0, 0, w, h);
   cerr << "Size of window is now " << w << "x" << h << endl; // TODO remove this line
   updateFrustFovY();
}
/*-----------------------------------------------*/
void MySdlApplication::keyboard()
{
   /*	PURPOSE:		Handles all keyboard inputs
      RECEIVES:
      RETURNS:
      REMARKS:		Uses KB_STATE and kbPrevState to evaluate which keys are pressed or held down

      */

   if (KB_STATE[SDL_SCANCODE_L])
   {
      rotateAboutOrigin(&g_eyeRbt, -G_CAM_ROTATION);
   }
   else if (KB_STATE[SDL_SCANCODE_R])
   {
      rotateAboutOrigin(&g_eyeRbt, G_CAM_ROTATION);
   }
   else if (KB_STATE[SDL_SCANCODE_T] && !kbPrevState[SDL_SCANCODE_T])
   {
      g_activeShader++;
      if (g_activeShader >= G_NUM_SHADERS)
         g_activeShader = 0;

      g_rigidBodies[1].children[0]->material++;
      if (g_rigidBodies[1].children[0]->material >= G_NUM_SHADERS)
         g_rigidBodies[1].children[0]->material = 0;
   }
   else if (KB_STATE[SDL_SCANCODE_SPACE] && !kbPrevState[SDL_SCANCODE_SPACE])
   {

   }
   else if (KB_STATE[SDL_SCANCODE_ESCAPE])
   {
      // End program
      running = false;
   }
   else if (KB_STATE[SDL_SCANCODE_C] && !kbPrevState[SDL_SCANCODE_C])
   {
      // TODO Remove this
      Cvec3 cam = g_eyeRbt.getTranslation();
      cout << "camera = <" << cam[0] << ", " << cam[1] << ", " << cam[2] << ">" << endl;
   }
}
/*-----------------------------------------------*/
void MySdlApplication::mouse(SDL_MouseButtonEvent button)
{
   /*	PURPOSE:		Handles MouseButton events to set mouse related variables
      RECEIVES:	button - MouseButtonEvent object that has current state of Mouse
      RETURNS:
      REMARKS:
      */

   g_mouseClickX = button.x;
   g_mouseClickY = g_windowHeight - button.y - 1;

   g_mouseLClickButton |= (button.button == SDL_BUTTON_LEFT &&
      button.state == SDL_PRESSED);
   g_mouseRClickButton |= (button.button == SDL_BUTTON_RIGHT &&
      button.state == SDL_PRESSED);
   g_mouseMClickButton |= (button.button == SDL_BUTTON_MIDDLE &&
      button.state == SDL_PRESSED);

   g_mouseLClickButton &= !(button.button == SDL_BUTTON_LEFT &&
      button.state == SDL_RELEASED);
   g_mouseRClickButton &= !(button.button == SDL_BUTTON_RIGHT &&
      button.state == SDL_RELEASED);
   g_mouseMClickButton &= !(button.button == SDL_BUTTON_MIDDLE &&
      button.state == SDL_RELEASED);

   g_mouseClickDown = g_mouseLClickButton || g_mouseRClickButton ||
      g_mouseMClickButton;
}
/*-----------------------------------------------*/
void MySdlApplication::motion(const int x, const int y)
{
   /*	PURPOSE:		Handles all Mouse related input
      RECEIVES:	x - x position of the mouse in screen Coordinates
      y - y position of the mouse in screen Coordinates
      RETURNS:
      REMARKS:
      */

   const double dx = x - g_mouseClickX;
   const double dy = g_windowHeight - y - 1 - g_mouseClickY;

   RigTForm m;
   if (g_mouseLClickButton && !g_mouseRClickButton)
   {
      // left button down?
      //m = g_eyeRbt * RigTForm(Quat().makeXRotation(-dy)) * RigTForm(Quat().makeYRotation(dx)) * inv(g_eyeRbt);
      m = g_rigidBodies[0].rtf * RigTForm(Quat().makeXRotation(-dy)) * RigTForm(Quat().makeYRotation(dx)) * inv(g_rigidBodies[0].rtf);
   }
   else if (g_mouseRClickButton && !g_mouseLClickButton)
   {
      // right button down?
      m = g_eyeRbt * RigTForm(Cvec3(dx, dy, 0) * 0.01) * inv(g_eyeRbt);
   }
   else if (g_mouseMClickButton || (g_mouseLClickButton && g_mouseRClickButton))
   {
      // middle or (left and right) button down?
      //m = RigTForm(Cvec3(0, 0, -dy) * 0.01);
      m = g_eyeRbt * RigTForm(Cvec3(0, 0, dy) * 0.01) * inv(g_eyeRbt);
   }

   if (g_mouseClickDown)
      //g_rigidBodies[0].rtf = m * g_rigidBodies[0].rtf;
      g_eyeRbt = m * g_eyeRbt;

   g_mouseClickX = x;
   g_mouseClickY = g_windowHeight - y - 1;

}
/*-----------------------------------------------*/
void MySdlApplication::onLoop()
{
   /*	PURPOSE:		Handles function calls that need to run once per SDL loop
      RECEIVES:
      RETURNS:
      REMARKS:
      */

   // Logic goes here
   keyboard();
}
/*-----------------------------------------------*/
void MySdlApplication::onRender()
{
   /*	PURPOSE:		Handles all graphics related calls once per SDL loop
      RECEIVES:
      RETURNS:
      REMARKS:
      */

   // All draw calls go here
   glUseProgram(g_shaderStates[g_activeShader]->program);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   // clear framebuffer color&depth
   drawStuff();

   SDL_GL_SwapWindow(display);
   checkGlErrors();
}
/*-----------------------------------------------*/
int MySdlApplication::onExecute()
{
   /*	PURPOSE:		Main function loop of MySdlApplication
      RECEIVES:
      RETURNS:
      REMARKS:
      */

   srand((unsigned int)time(NULL));

   if (onInit() == false)
      return -1;

   SDL_Event Event;
   while (running)
   {
      memcpy(kbPrevState, KB_STATE, sizeof(kbPrevState));

      while (SDL_PollEvent(&Event))
      {
         onEvent(&Event);
      }

      onLoop();
      onRender();
   }

   onCleanup();

   return 0;
}
/*-----------------------------------------------*/
bool MySdlApplication::onInit()
{
   /*	PURPOSE:		Initializes SDL
      RECEIVES:
      RETURNS:
      REMARKS:
      */

   if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
   {
      return false;
   }
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);

   /* Turn on double buffering with a 24bit Z buffer.
   * You may need to change this to 16 or 32 for your system */
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

   if ((display = SDL_CreateWindow("My SDL Application",
      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, g_windowWidth, g_windowHeight,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE)) == NULL)
   {
      return false;
   }

   /* Create our opengl context and attach it to our window */
   SDL_GLContext maincontext = SDL_GL_CreateContext(display);
   /* This makes our buffer swap syncronized with the
   monitor's vertical refresh */
   SDL_GL_SetSwapInterval(1);

   GLenum glewError = glewInit();
   if (glewError != GLEW_OK)
   {
      SDL_Quit();
      return 1;
   }
   if (!GLEW_VERSION_1_5)
   {
      SDL_Quit();
      return 1;
   }

   cout << (G_GL2_COMPATIBLE ?
      "Will use OpenGL 2.x / GLSL 1.0" : "Will use OpenGL 3.x / GLSL 1.3")
      << endl;
   if ((!G_GL2_COMPATIBLE) && !GLEW_VERSION_3_0)
      throw runtime_error(
      "Error: does not support OpenGL Shading Language v1.3");
   else if (G_GL2_COMPATIBLE && !GLEW_VERSION_2_0)
      throw runtime_error(
      "Error: does not support OpenGL Shading Language v1.0");

   initGLState();
   initShaders();
   initGeometry();
   initTextures();
   initCamera();

   KB_STATE = SDL_GetKeyboardState(NULL);

   return true;
}
/*-----------------------------------------------*/
void MySdlApplication::onEvent(SDL_Event* event)
{
   /*	PURPOSE:		Handles SDL events
      RECEIVES:	event - SDL Event to be handled
      RETURNS:
      REMARKS:
      */

   Uint32 type = event->type;

   if (type == SDL_QUIT)
      running = false;
   else if (type == SDL_MOUSEBUTTONDOWN)
      mouse(event->button);
   else if (type == SDL_MOUSEBUTTONUP)
      mouse(event->button);
   else if (type == SDL_MOUSEMOTION)
      motion(event->motion.x, event->motion.y);
   else if (type == SDL_WINDOWEVENT)
      if (event->window.event == SDL_WINDOWEVENT_RESIZED)
         reshape(event->window.data1, event->window.data2);
}
/*-----------------------------------------------*/
void MySdlApplication::onCleanup()
{
   /*	PURPOSE:		Everything to be done before program closes
      RECEIVES:
      RETURNS:
      REMARKS:
      */

   SDL_Quit();
}
/*-----------------------------------------------*/
MySdlApplication::MySdlApplication()
{
   /*	PURPOSE:		Constructor for MySdlApplication
      RECEIVES:
      RETURNS:
      REMARKS:
      */

   running = true;
}
/*-----------------------------------------------*/
int main(int argc, const char* argv[])
{
   /*	PURPOSE:		Main function of MySdlApplication
      RECEIVES:	argc - number of arguments passed
      argv - array of arguments
      RETURNS:		int - Whether or not program ran sucessfully
      REMARKS:
      */

   MySdlApplication application;
   return application.onExecute();
}


// TODO (REMOVE before Submission)
//Coding Guidelines template 

/*-----------------------------------------------*/


/*	PURPOSE:		What does this function do? (must be present)
   RECEIVES:	List every argument name and explain each argument.
   (omit if the function has no arguments)
   RETURNS:		Explain the value returned by the function.
   (omit if the function returns no value)
   REMARKS:		Explain any special preconditions or postconditions.
   See example below. (omit if function is unremarkable)
   */
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
#include "WE_Edge.h"
#include "WE_Face.h"
#include "WE_Vertex.h"
#include "Particle.h"

using namespace std;
using namespace tr1;

// Forward Declarations
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
static const int G_NUM_OF_OBJECTS = 1; //Number of objects to be drawn
static const int G_NUM_SHADERS = 2;
static const char * const G_SHADER_FILES[G_NUM_SHADERS][2] =
{
   { "./Shaders/basic-gl3.vshader", "./Shaders/diffuse-gl3.fshader" },
   { "./Shaders/basic-gl3.vshader", "./Shaders/solid-gl3.fshader" }
};
static const char * const G_SHADER_FILES_GL2[G_NUM_SHADERS][2] =
{
   { "./Shaders/basic-gl2.vshader", "./Shaders/diffuse-gl2.fshader" },
   { "./Shaders/basic-gl2.vshader", "./Shaders/solid-gl2.fshader" }
};

static const float G_FRUST_NEAR = -0.1f;    // near plane
static const float G_FRUST_FAR = -100.0f;    // far plane
static const float G_GROUND_Y = 0.0f;      // y coordinate of the ground
static const float G_GROUND_SIZE = 50.0f;   // half the ground length
static const int G_MAX_PARTICLES = 1000;

// Global variables
int g_windowWidth = 640;
int g_windowHeight = 480;
unsigned char kbPrevState[SDL_NUM_SCANCODES] = { 0 };

static bool g_mouseClickDown = false;    // is the mouse button pressed
static bool g_mouseLClickButton, g_mouseRClickButton, g_mouseMClickButton;
static int g_mouseClickX, g_mouseClickY; // coordinates for mouse click event
static int g_activeShader = 0;
SDL_TimerID g_animationTimer;
SDL_TimerID g_animationReset;
static bool g_isAnimating = false;
static bool g_isParticulating = false;
static WE_Vertex* g_EcodTM_Vertex;
static WE_Vertex* g_EcodTM_FirstVertex;
static WE_Edge g_weEdges[24];
static WE_Face g_weFaces[16];
static WE_Vertex g_weVertices[10];
static std::vector<Particle*> g_particles;

static float g_frustFovY = G_FRUST_MIN_FOV; // FOV in y direction
static shared_ptr<GlTexture> g_tex0;

// --------- Scene

static Cvec3 g_light1(2.0, 3.0, 14.0);
//static Cvec3 g_light2(-2000.0, -3000.0, -5000.0);
// define light positions in world space
static RigTForm g_skyRbt = RigTForm(Cvec3(0.0, 0.0, 0.0)); // Initialized here but set in initCamera()
static RigTForm g_eyeRbt = g_skyRbt;
static Cvec3f g_objectColors[1] = { Cvec3f(1, 0, 0) };



/*-----------------------------------------------*/
static vector<shared_ptr<MySdlApplication::ShaderState> > g_shaderStates;
// our global shader states
// Vertex buffer and index buffer associated with the ground and cube geometry
static shared_ptr<MySdlApplication::Geometry> g_ground, g_cube, g_sphere, g_triangle;
static RigidBody g_rigidBodies[G_NUM_OF_OBJECTS]; // Array that holds each Rigid Body Object
///////////////// END OF G L O B A L S ///////////////////////

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
static MySdlApplication::Geometry* initRightTriangle()
{
   /*	PURPOSE:		Sets up index and vertex buffers and calls geometrymaker for a right triangle
   RECEIVES:
   RETURNS:		Geometry - returns Geometry object
   REMARKS:
   */

   int ibLen = 3;
   int vbLen = 3;

   // Temporary storage for plane geometry
   vector<GenericVertex> vtx(vbLen);
   vector<unsigned short> idx(ibLen);

   makeRightTriangle(vtx.begin(), idx.begin());
   return new MySdlApplication::Geometry(&vtx[0], &idx[0], vbLen, ibLen);
}
/*-----------------------------------------------*/
static MySdlApplication::Geometry* initTriangle()
{
   /*	PURPOSE:		Sets up index and vertex buffers and calls geometrymaker for a right triangle
   RECEIVES:
   RETURNS:		Geometry - returns Geometry object
   REMARKS:
   */

   int ibLen = 3;
   int vbLen = 3;

   // Temporary storage for plane geometry
   vector<GenericVertex> vtx(vbLen);
   vector<unsigned short> idx(ibLen);

   makeTriangle(vtx.begin(), idx.begin());
   return new MySdlApplication::Geometry(&vtx[0], &idx[0], vbLen, ibLen);
}
/*-----------------------------------------------*/
static void buildEdges()
{
   /*	PURPOSE:		Builds the edges of the WingedEdge Cube
      RECEIVES:
      RETURNS:
      REMARKS:
      */

   // Build all edges
   WE_Edge* edge = &g_weEdges[0];
   edge->vert1 = &g_weVertices[0];
   edge->vert2 = &g_weVertices[1];
   edge->aFace = &g_weFaces[1];
   edge->bFace = &g_weFaces[12];
   edge->aPrev = &g_weEdges[9];
   edge->bPrev = &g_weEdges[23];
   edge->aNext = &g_weEdges[10];
   edge->bNext = &g_weEdges[20];

   edge = &g_weEdges[1];
   edge->vert1 = &g_weVertices[1];
   edge->vert2 = &g_weVertices[2];
   edge->aFace = &g_weFaces[3];
   edge->bFace = &g_weFaces[13];
   edge->aPrev = &g_weEdges[11];
   edge->bPrev = &g_weEdges[20];
   edge->aNext = &g_weEdges[12];
   edge->bNext = &g_weEdges[21];

   edge = &g_weEdges[2];
   edge->vert1 = &g_weVertices[2];
   edge->vert2 = &g_weVertices[3];
   edge->aFace = &g_weFaces[5];
   edge->bFace = &g_weFaces[14];
   edge->aPrev = &g_weEdges[13];
   edge->bPrev = &g_weEdges[21];
   edge->aNext = &g_weEdges[14];
   edge->bNext = &g_weEdges[22];

   edge = &g_weEdges[3];
   edge->vert1 = &g_weVertices[3];
   edge->vert2 = &g_weVertices[0];
   edge->aFace = &g_weFaces[7];
   edge->bFace = &g_weFaces[15];
   edge->aPrev = &g_weEdges[15];
   edge->bPrev = &g_weEdges[22];
   edge->aNext = &g_weEdges[8];
   edge->bNext = &g_weEdges[23];

   edge = &g_weEdges[4];
   edge->vert1 = &g_weVertices[4];
   edge->vert2 = &g_weVertices[5];
   edge->aFace = &g_weFaces[0];
   edge->bFace = &g_weFaces[8];
   edge->aPrev = &g_weEdges[19];
   edge->bPrev = &g_weEdges[8];
   edge->aNext = &g_weEdges[16];
   edge->bNext = &g_weEdges[9];

   edge = &g_weEdges[5];
   edge->vert1 = &g_weVertices[5];
   edge->vert2 = &g_weVertices[6];
   edge->aFace = &g_weFaces[2];
   edge->bFace = &g_weFaces[9];
   edge->aPrev = &g_weEdges[16];
   edge->bPrev = &g_weEdges[10];
   edge->aNext = &g_weEdges[17];
   edge->bNext = &g_weEdges[11];

   edge = &g_weEdges[6];
   edge->vert1 = &g_weVertices[6];
   edge->vert2 = &g_weVertices[7];
   edge->aFace = &g_weFaces[4];
   edge->bFace = &g_weFaces[10];
   edge->aPrev = &g_weEdges[17];
   edge->bPrev = &g_weEdges[12];
   edge->aNext = &g_weEdges[18];
   edge->bNext = &g_weEdges[13];

   edge = &g_weEdges[7];
   edge->vert1 = &g_weVertices[7];
   edge->vert2 = &g_weVertices[4];
   edge->aFace = &g_weFaces[6];
   edge->bFace = &g_weFaces[11];
   edge->aPrev = &g_weEdges[18];
   edge->bPrev = &g_weEdges[14];
   edge->aNext = &g_weEdges[19];
   edge->bNext = &g_weEdges[15];

   edge = &g_weEdges[8];
   edge->vert1 = &g_weVertices[0];
   edge->vert2 = &g_weVertices[4];
   edge->aFace = &g_weFaces[7];
   edge->bFace = &g_weFaces[0];
   edge->aPrev = &g_weEdges[3];
   edge->bPrev = &g_weEdges[9];
   edge->aNext = &g_weEdges[7];
   edge->bNext = &g_weEdges[4];

   edge = &g_weEdges[9];
   edge->vert1 = &g_weVertices[0];
   edge->vert2 = &g_weVertices[5];
   edge->aFace = &g_weFaces[0];
   edge->bFace = &g_weFaces[1];
   edge->aPrev = &g_weEdges[8];
   edge->bPrev = &g_weEdges[0];
   edge->aNext = &g_weEdges[4];
   edge->bNext = &g_weEdges[10];

   edge = &g_weEdges[10];
   edge->vert1 = &g_weVertices[1];
   edge->vert2 = &g_weVertices[5];
   edge->aFace = &g_weFaces[1];
   edge->bFace = &g_weFaces[2];
   edge->aPrev = &g_weEdges[0];
   edge->bPrev = &g_weEdges[11];
   edge->aNext = &g_weEdges[9];
   edge->bNext = &g_weEdges[5];

   edge = &g_weEdges[11];
   edge->vert1 = &g_weVertices[1];
   edge->vert2 = &g_weVertices[6];
   edge->aFace = &g_weFaces[2];
   edge->bFace = &g_weFaces[3];
   edge->aPrev = &g_weEdges[10];
   edge->bPrev = &g_weEdges[1];
   edge->aNext = &g_weEdges[5];
   edge->bNext = &g_weEdges[12];

   edge = &g_weEdges[12];
   edge->vert1 = &g_weVertices[2];
   edge->vert2 = &g_weVertices[6];
   edge->aFace = &g_weFaces[3];
   edge->bFace = &g_weFaces[4];
   edge->aPrev = &g_weEdges[1];
   edge->bPrev = &g_weEdges[13];
   edge->aNext = &g_weEdges[11];
   edge->bNext = &g_weEdges[6];

   edge = &g_weEdges[13];
   edge->vert1 = &g_weVertices[2];
   edge->vert2 = &g_weVertices[7];
   edge->aFace = &g_weFaces[4];
   edge->bFace = &g_weFaces[5];
   edge->aPrev = &g_weEdges[12];
   edge->bPrev = &g_weEdges[2];
   edge->aNext = &g_weEdges[6];
   edge->bNext = &g_weEdges[14];

   edge = &g_weEdges[14];
   edge->vert1 = &g_weVertices[3];
   edge->vert2 = &g_weVertices[7];
   edge->aFace = &g_weFaces[5];
   edge->bFace = &g_weFaces[6];
   edge->aPrev = &g_weEdges[2];
   edge->bPrev = &g_weEdges[15];
   edge->aNext = &g_weEdges[13];
   edge->bNext = &g_weEdges[7];

   edge = &g_weEdges[15];
   edge->vert1 = &g_weVertices[3];
   edge->vert2 = &g_weVertices[4];
   edge->aFace = &g_weFaces[6];
   edge->bFace = &g_weFaces[7];
   edge->aPrev = &g_weEdges[14];
   edge->bPrev = &g_weEdges[3];
   edge->aNext = &g_weEdges[7];
   edge->bNext = &g_weEdges[15];

   edge = &g_weEdges[16];
   edge->vert1 = &g_weVertices[8];
   edge->vert2 = &g_weVertices[5];
   edge->aFace = &g_weFaces[8];
   edge->bFace = &g_weFaces[9];
   edge->aPrev = &g_weEdges[4];
   edge->bPrev = &g_weEdges[5];
   edge->aNext = &g_weEdges[19];
   edge->bNext = &g_weEdges[17];

   edge = &g_weEdges[17];
   edge->vert1 = &g_weVertices[6];
   edge->vert2 = &g_weVertices[8];
   edge->aFace = &g_weFaces[9];
   edge->bFace = &g_weFaces[10];
   edge->aPrev = &g_weEdges[5];
   edge->bPrev = &g_weEdges[6];
   edge->aNext = &g_weEdges[16];
   edge->bNext = &g_weEdges[18];

   edge = &g_weEdges[18];
   edge->vert1 = &g_weVertices[8];
   edge->vert2 = &g_weVertices[7];
   edge->aFace = &g_weFaces[10];
   edge->bFace = &g_weFaces[11];
   edge->aPrev = &g_weEdges[6];
   edge->bPrev = &g_weEdges[7];
   edge->aNext = &g_weEdges[17];
   edge->bNext = &g_weEdges[19];

   edge = &g_weEdges[19];
   edge->vert1 = &g_weVertices[4];
   edge->vert2 = &g_weVertices[8];
   edge->aFace = &g_weFaces[11];
   edge->bFace = &g_weFaces[8];
   edge->aPrev = &g_weEdges[7];
   edge->bPrev = &g_weEdges[4];
   edge->aNext = &g_weEdges[18];
   edge->bNext = &g_weEdges[16];

   edge = &g_weEdges[20];
   edge->vert1 = &g_weVertices[9];
   edge->vert2 = &g_weVertices[1];
   edge->aFace = &g_weFaces[12];
   edge->bFace = &g_weFaces[13];
   edge->aPrev = &g_weEdges[0];
   edge->bPrev = &g_weEdges[1];
   edge->aNext = &g_weEdges[23];
   edge->bNext = &g_weEdges[21];

   edge = &g_weEdges[21];
   edge->vert1 = &g_weVertices[2];
   edge->vert2 = &g_weVertices[9];
   edge->aFace = &g_weFaces[13];
   edge->bFace = &g_weFaces[14];
   edge->aPrev = &g_weEdges[1];
   edge->bPrev = &g_weEdges[2];
   edge->aNext = &g_weEdges[20];
   edge->bNext = &g_weEdges[22];

   edge = &g_weEdges[22];
   edge->vert1 = &g_weVertices[9];
   edge->vert2 = &g_weVertices[3];
   edge->aFace = &g_weFaces[14];
   edge->bFace = &g_weFaces[15];
   edge->aPrev = &g_weEdges[2];
   edge->bPrev = &g_weEdges[3];
   edge->aNext = &g_weEdges[21];
   edge->bNext = &g_weEdges[23];

   edge = &g_weEdges[23];
   edge->vert1 = &g_weVertices[0];
   edge->vert2 = &g_weVertices[9];
   edge->aFace = &g_weFaces[15];
   edge->bFace = &g_weFaces[12];
   edge->aPrev = &g_weEdges[3];
   edge->bPrev = &g_weEdges[0];
   edge->aNext = &g_weEdges[22];
   edge->bNext = &g_weEdges[20];
}
/*-----------------------------------------------*/
static void buildWingEdgedCube(RigidBody** vertices, RigidBody** edges, RigidBody** faces)
{
   /*	PURPOSE:		Builds the WingedEdge Cube
      RECEIVES:   vertices - RigidBody array of vertices
      edges - RigidBody array of edges
      faces - RigidBody array of faces
      RETURNS:
      REMARKS:
      */

   // Create all WE objects for cube
   for (int i = 0; i < 24; i++)
   {
      g_weEdges[i] = WE_Edge();
      g_weEdges[i].data = edges[i];
   }
   for (int i = 0; i < 10; i++)
   {
      g_weVertices[i] = WE_Vertex();
      g_weVertices[i].data = vertices[i];
   }
   for (int i = 0; i < 16; i++)
   {
      g_weFaces[i] = WE_Face();
      g_weFaces[i].data = faces[i];
   }

   // Setup pointer for starting vertex
   g_EcodTM_Vertex = &g_weVertices[0];
   g_EcodTM_FirstVertex = &g_weVertices[0];

   // Set edges for all vertices (10)
   int i = 0;
   g_weVertices[i].edges.push_back(&g_weEdges[8]);
   g_weVertices[i].edges.push_back(&g_weEdges[9]);
   g_weVertices[i].edges.push_back(&g_weEdges[0]);
   g_weVertices[i].edges.push_back(&g_weEdges[23]);
   g_weVertices[i].edges.push_back(&g_weEdges[3]);
   i++;
   g_weVertices[i].edges.push_back(&g_weEdges[10]);
   g_weVertices[i].edges.push_back(&g_weEdges[11]);
   g_weVertices[i].edges.push_back(&g_weEdges[1]);
   g_weVertices[i].edges.push_back(&g_weEdges[20]);
   g_weVertices[i].edges.push_back(&g_weEdges[0]);
   i++;
   g_weVertices[i].edges.push_back(&g_weEdges[12]);
   g_weVertices[i].edges.push_back(&g_weEdges[13]);
   g_weVertices[i].edges.push_back(&g_weEdges[2]);
   g_weVertices[i].edges.push_back(&g_weEdges[21]);
   g_weVertices[i].edges.push_back(&g_weEdges[1]);
   i++;
   g_weVertices[i].edges.push_back(&g_weEdges[14]);
   g_weVertices[i].edges.push_back(&g_weEdges[15]);
   g_weVertices[i].edges.push_back(&g_weEdges[3]);
   g_weVertices[i].edges.push_back(&g_weEdges[22]);
   g_weVertices[i].edges.push_back(&g_weEdges[2]);
   i++;
   g_weVertices[i].edges.push_back(&g_weEdges[8]);
   g_weVertices[i].edges.push_back(&g_weEdges[15]);
   g_weVertices[i].edges.push_back(&g_weEdges[7]);
   g_weVertices[i].edges.push_back(&g_weEdges[19]);
   g_weVertices[i].edges.push_back(&g_weEdges[4]);
   i++;
   g_weVertices[i].edges.push_back(&g_weEdges[10]);
   g_weVertices[i].edges.push_back(&g_weEdges[9]);
   g_weVertices[i].edges.push_back(&g_weEdges[4]);
   g_weVertices[i].edges.push_back(&g_weEdges[16]);
   g_weVertices[i].edges.push_back(&g_weEdges[5]);
   i++;
   g_weVertices[i].edges.push_back(&g_weEdges[12]);
   g_weVertices[i].edges.push_back(&g_weEdges[11]);
   g_weVertices[i].edges.push_back(&g_weEdges[5]);
   g_weVertices[i].edges.push_back(&g_weEdges[17]);
   g_weVertices[i].edges.push_back(&g_weEdges[6]);
   i++;
   g_weVertices[i].edges.push_back(&g_weEdges[14]);
   g_weVertices[i].edges.push_back(&g_weEdges[13]);
   g_weVertices[i].edges.push_back(&g_weEdges[6]);
   g_weVertices[i].edges.push_back(&g_weEdges[18]);
   g_weVertices[i].edges.push_back(&g_weEdges[7]);
   i++;
   g_weVertices[i].edges.push_back(&g_weEdges[16]);
   g_weVertices[i].edges.push_back(&g_weEdges[17]);
   g_weVertices[i].edges.push_back(&g_weEdges[18]);
   g_weVertices[i].edges.push_back(&g_weEdges[19]);
   i++;
   g_weVertices[i].edges.push_back(&g_weEdges[20]);
   g_weVertices[i].edges.push_back(&g_weEdges[21]);
   g_weVertices[i].edges.push_back(&g_weEdges[22]);
   g_weVertices[i].edges.push_back(&g_weEdges[23]);

   // Set edges for all faces
   i = 0;
   g_weFaces[i].edges.push_back(&g_weEdges[4]);
   g_weFaces[i].edges.push_back(&g_weEdges[8]);
   g_weFaces[i].edges.push_back(&g_weEdges[9]);
   i++;
   g_weFaces[i].edges.push_back(&g_weEdges[0]);
   g_weFaces[i].edges.push_back(&g_weEdges[10]);
   g_weFaces[i].edges.push_back(&g_weEdges[9]);
   i++;
   g_weFaces[i].edges.push_back(&g_weEdges[5]);
   g_weFaces[i].edges.push_back(&g_weEdges[10]);
   g_weFaces[i].edges.push_back(&g_weEdges[11]);
   i++;
   g_weFaces[i].edges.push_back(&g_weEdges[1]);
   g_weFaces[i].edges.push_back(&g_weEdges[12]);
   g_weFaces[i].edges.push_back(&g_weEdges[11]);
   i++;
   g_weFaces[i].edges.push_back(&g_weEdges[6]);
   g_weFaces[i].edges.push_back(&g_weEdges[12]);
   g_weFaces[i].edges.push_back(&g_weEdges[13]);
   i++;
   g_weFaces[i].edges.push_back(&g_weEdges[2]);
   g_weFaces[i].edges.push_back(&g_weEdges[14]);
   g_weFaces[i].edges.push_back(&g_weEdges[13]);
   i++;
   g_weFaces[i].edges.push_back(&g_weEdges[7]);
   g_weFaces[i].edges.push_back(&g_weEdges[14]);
   g_weFaces[i].edges.push_back(&g_weEdges[15]);
   i++;
   g_weFaces[i].edges.push_back(&g_weEdges[3]);
   g_weFaces[i].edges.push_back(&g_weEdges[8]);
   g_weFaces[i].edges.push_back(&g_weEdges[15]);
   i++;
   g_weFaces[i].edges.push_back(&g_weEdges[4]);
   g_weFaces[i].edges.push_back(&g_weEdges[16]);
   g_weFaces[i].edges.push_back(&g_weEdges[19]);
   i++;
   g_weFaces[i].edges.push_back(&g_weEdges[5]);
   g_weFaces[i].edges.push_back(&g_weEdges[17]);
   g_weFaces[i].edges.push_back(&g_weEdges[16]);
   i++;
   g_weFaces[i].edges.push_back(&g_weEdges[6]);
   g_weFaces[i].edges.push_back(&g_weEdges[18]);
   g_weFaces[i].edges.push_back(&g_weEdges[17]);
   i++;
   g_weFaces[i].edges.push_back(&g_weEdges[7]);
   g_weFaces[i].edges.push_back(&g_weEdges[19]);
   g_weFaces[i].edges.push_back(&g_weEdges[18]);
   i++;
   g_weFaces[i].edges.push_back(&g_weEdges[0]);
   g_weFaces[i].edges.push_back(&g_weEdges[23]);
   g_weFaces[i].edges.push_back(&g_weEdges[20]);
   i++;
   g_weFaces[i].edges.push_back(&g_weEdges[1]);
   g_weFaces[i].edges.push_back(&g_weEdges[20]);
   g_weFaces[i].edges.push_back(&g_weEdges[21]);
   i++;
   g_weFaces[i].edges.push_back(&g_weEdges[2]);
   g_weFaces[i].edges.push_back(&g_weEdges[21]);
   g_weFaces[i].edges.push_back(&g_weEdges[22]);
   i++;
   g_weFaces[i].edges.push_back(&g_weEdges[3]);
   g_weFaces[i].edges.push_back(&g_weEdges[22]);
   g_weFaces[i].edges.push_back(&g_weEdges[23]);

   buildEdges();
}
/*-----------------------------------------------*/
static RigidBody* buildEpilepticCubeOfDoomTM()
{
   /*	PURPOSE:		Builds an Epileptic Cube of Doom TM object
      RECEIVES:
      RETURNS:		RigidBody - Returns RigidBody object containing a Epileptic Cube of Doom TM
      REMARKS:		EcodTM is inside an invisible container object
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

   // Make Vertices (10)
   const int numVertices = 10;
   RigidBody **vertices = new RigidBody*[numVertices];

   float h = 1.0;
   Cvec3 points[numVertices] = { Cvec3(-h, -h, h), Cvec3(h, -h, h), Cvec3(h, -h, -h), Cvec3(-h, -h, -h),
      Cvec3(-h, h, h), Cvec3(h, h, h), Cvec3(h, h, -h), Cvec3(-h, h, -h), Cvec3(0, h, 0), Cvec3(0, -h, 0) };

   scaleTemp = Matrix4::makeScale(Cvec3(2, 2, 2));
   for (int i = 0; i < numVertices; i++)
   {
      rigTemp = RigTForm(points[i]);

      vertices[i] = new RigidBody(rigTemp, scaleTemp, NULL, initPoint(), black, SOLID);
      vertices[i]->mode = GL_POINTS;
   }

   // Make Edges (24)
   const int numEdges = 24;
   RigidBody **edges = new RigidBody*[numEdges];

   Cvec3 edgeTranslations[numEdges] = {
      /*0*/Cvec3(0, -h, h), Cvec3(h, -h, 0), Cvec3(0, -h, -h), Cvec3(-h, -h, 0),
      /*4*/Cvec3(0, h, h), Cvec3(h, h, 0), Cvec3(0, h, -h), Cvec3(-h, h, 0),
      /*8*/Cvec3(-h, 0, h), Cvec3(0, 0, h), Cvec3(h, 0, h), Cvec3(h, 0, 0),
      /*12*/Cvec3(h, 0, -h), Cvec3(0, 0, -h), Cvec3(-h, 0, -h), Cvec3(-h, 0, 0),
      /*16*/Cvec3(h / 2, h, h / 2), Cvec3(h / 2, h, -h / 2), Cvec3(-h / 2, h, -h / 2), Cvec3(-h / 2, h, h / 2),
      /*20*/Cvec3(h / 2, -h, h / 2), Cvec3(h / 2, -h, -h / 2), Cvec3(-h / 2, -h, -h / 2), Cvec3(-h / 2, -h, h / 2) };
   Cvec3 edgeRotations[numEdges] = {
      /*0*/Cvec3(0, 0, 0), Cvec3(0, -90, 0), Cvec3(0, 180, 0), Cvec3(0, 90, 0),
      /*4*/Cvec3(0, 0, 0), Cvec3(0, -90, 0), Cvec3(0, 180, 0), Cvec3(0, 90, 0),
      /*8*/Cvec3(0, 0, 90), Cvec3(0, 0, 45), Cvec3(0, 0, 90), Cvec3(45, -90, 0),
      /*12*/Cvec3(0, 0, 90), Cvec3(0, 180, 45), Cvec3(0, 0, 90), Cvec3(-45, -90, 0),
      /*16*/Cvec3(0, -45, 0), Cvec3(0, 45, 0), Cvec3(0, -45, 0), Cvec3(0, 45, 0),
      /*20*/Cvec3(0, -45, 0), Cvec3(0, 45, 0), Cvec3(0, -45, 0), Cvec3(0, 45, 0) };

   for (int i = 0; i < numEdges; i++)
   {
      // Long Diagonals
      if (i == 9 || i == 11 || i == 13 || i == 15)
         scaleTemp = Matrix4::makeScale(Cvec3(2.8243, 2.8243, 2.8243));
      else if (i >= 16)
         scaleTemp = Matrix4::makeScale(Cvec3(1.4142, 1.4142, 1.4142));
      else
         scaleTemp = Matrix4::makeScale(Cvec3(2, 2, 2));


      rigTemp = RigTForm(edgeTranslations[i]);
      rigTemp.setRotation(rigTemp.getRotation() * Quat().makeRotation(edgeRotations[i]));
      edges[i] = new RigidBody(rigTemp, scaleTemp, NULL, initLine(), black, SOLID);
      edges[i]->mode = GL_LINES;
   }

   // Make Faces (16)
   const int numFaces = 16;
   RigidBody **faces = new RigidBody*[numFaces];
   scaleTemp = Matrix4::makeScale(Cvec3(2, 2, 2));

   Cvec3 faceTranslations[numFaces] = {
      /*0*/Cvec3(0, 0, h), Cvec3(0, 0, h), Cvec3(h, 0, 0), Cvec3(h, 0, 0),
      /*4*/Cvec3(0, 0, -h), Cvec3(0, 0, -h), Cvec3(-h, 0, 0), Cvec3(-h, 0, 0),
      /*8*/Cvec3(0, h, 0), Cvec3(0, h, 0), Cvec3(0, h, 0), Cvec3(0, h, 0),
      /*12*/Cvec3(0, -h, 0), Cvec3(0, -h, 0), Cvec3(0, -h, 0), Cvec3(0, -h, 0) };
   Cvec3 faceRotations[numFaces] = {
      /*0*/Cvec3(0, 0, -90), Cvec3(0, 0, 90), Cvec3(0, 90, -90), Cvec3(0, 90, 90),
      /*4*/Cvec3(0, 180, -90), Cvec3(0, 180, 90), Cvec3(90, -90, 0), Cvec3(-90, -90, 0),
      /*8*/Cvec3(-90, 0, 0), Cvec3(-90, 0, 90), Cvec3(-90, 0, 180), Cvec3(-90, 0, -90),
      /*12*/Cvec3(90, 0, 0), Cvec3(90, 0, 90), Cvec3(90, 0, 180), Cvec3(90, 0, -90) };

   for (int i = 0; i < numFaces; i++)
   {
      rigTemp = RigTForm(faceTranslations[i]);
      rigTemp.setRotation(rigTemp.getRotation() * Quat().makeRotation(faceRotations[i]));
      if (i < 8)
         faces[i] = new RigidBody(rigTemp, scaleTemp, NULL, initRightTriangle(), grey, SOLID);
      else
         faces[i] = new RigidBody(rigTemp, scaleTemp, NULL, initTriangle(), grey, SOLID);
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

   buildWingEdgedCube(vertices, edges, faces);

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
   g_rigidBodies[0] = *EcodTM;
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
Uint32 animationReset(Uint32 interval, void *param)
{
   /*	PURPOSE:		Callback timer function to reset the animation
      RECEIVES:   interval - The time that has passed for the callback
      param - An array of parameters
      RETURNS:    Interval to wait till next callback
      REMARKS:
      */

   g_isParticulating = false;
   g_EcodTM_Vertex = g_EcodTM_FirstVertex;

   return 0;
}
/*-----------------------------------------------*/
Uint32 animateEcodTM(Uint32 interval, void *param)
{
   /*	PURPOSE:		Animates EcodTM to flash crazily
      RECEIVES:   interval - time period that has passed
      RETURNS:    Interval to wait till next callback
      REMARKS:
      */

   static float stopwatch = 0;
   float msecsPerFrame = (float) interval;
   static int animationPart = 0;
   static int animationParts = 4;
   static bool isAnimating = true;
   static float totalSecs = 1.0;
   static float totalTime = (totalSecs / animationParts) * 1000;
   static float elapsedTime = 0;
   static bool isFirstEntry = true;
   static Cvec3 red = Cvec3(0.5, 0, 0);
   static Cvec3 green = Cvec3(0, 0.5, 0);
   static Cvec3 blue = Cvec3(0, 0, 0.5);
   static int colorIndex = 0;
   static vector<Cvec3> colors;
   static vector<RigidBody*> animatedParts;
   static WE_Edge* randoEdge;


   // Used to reset variables every time animation is run
   if (isFirstEntry)
   {
      elapsedTime = totalTime;
      isFirstEntry = false;

      colors.push_back(red);
      colors.push_back(green);
      colors.push_back(blue);
   }

   //Handles which part of animation is currently running
   if (elapsedTime >= totalTime)
   {
      if (animationPart == 0)
      {
         // Set Vertex
         animatedParts.push_back(g_EcodTM_Vertex->data);
      }
      else if (animationPart == 1)
      {
         // Reset colors
         for (int i = 0; i < (int) animatedParts.size(); i++)
            animatedParts[i]->color = animatedParts[i]->originalColor;

         // Set Vertex's Edges
         animatedParts.clear();
         for (int i = 0; i < (int) g_EcodTM_Vertex->edges.size(); i++)
            animatedParts.push_back(g_EcodTM_Vertex->edges[i]->data);
      }
      else if (animationPart == 2)
      {
         // Reset colors
         for (int i = 0; i < (int) animatedParts.size(); i++)
            animatedParts[i]->color = animatedParts[i]->originalColor;

         // Select Random Edge
         int choice = rand() % g_EcodTM_Vertex->edges.size();
         randoEdge = g_EcodTM_Vertex->edges[choice];
         g_EcodTM_Vertex = randoEdge->vert2;
         animatedParts.clear();

         animatedParts.push_back(randoEdge->data);
      }
      else if (animationPart == 3)
      {
         // Reset colors
         for (int i = 0; i < (int) animatedParts.size(); i++)
            animatedParts[i]->color = animatedParts[i]->originalColor;

         // Set Random Edge's Faces
         animatedParts.clear();

         animatedParts.push_back(randoEdge->aFace->data);
         animatedParts.push_back(randoEdge->bFace->data);
      }
      else
      {
         // Reset colors
         for (int i = 0; i < (int) animatedParts.size(); i++)
            animatedParts[i]->color = animatedParts[i]->originalColor;

         animatedParts.clear();
         isAnimating = false;
      }

      elapsedTime = 0;
      animationPart++;
   }

   if (isAnimating)
   {
      //Handle Animation
      for (int i = 0; i < (int) animatedParts.size(); i++)
         animatedParts[i]->color = colors[colorIndex];

      colorIndex++;
      if (colorIndex >= 3)
         colorIndex = 0;

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

   float temp = (float) dot(vectorOne, vectorTwo);
   float vOneNorm = (float) norm(vectorOne);
   float vTwoNorm = (float) norm(vectorTwo);
   temp = (temp / (vOneNorm * vTwoNorm));
   temp = acos(temp) * 180;
   temp /= (float) M_PI;

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

   // Draw all Particles
   for (int i = 0; i < (int) g_particles.size(); i++)
      g_particles[i]->data->drawRigidBody(invEyeRbt);
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

      g_rigidBodies[0].children[0]->material++;
      if (g_rigidBodies[0].children[0]->material >= G_NUM_SHADERS)
         g_rigidBodies[0].children[0]->material = 0;
   }
   else if (KB_STATE[SDL_SCANCODE_B])
   {
      // Setup for particle animation
      SDL_RemoveTimer(g_animationTimer);
      g_isAnimating = false;
      g_isParticulating = true;
      g_rigidBodies[0].isChildVisible = false;

      // Create Particles
      int numParticles = rand() % 20 + 1;
      for (int i = 0; i < numParticles; i++)
      {
         Particle* p = Particle::createRandomParticle();
         p->data = new RigidBody(RigTForm(), Matrix4(), NULL, initPoint(), Cvec3(1.0, 1.0, 1.0), SOLID);
         p->data->mode = GL_POINTS;

         g_particles.push_back(p);
      }
   }
   else if (KB_STATE[SDL_SCANCODE_N])
   {
      g_isParticulating = false;
   }
   else if (KB_STATE[SDL_SCANCODE_ESCAPE])
   {
      // End program
      running = false;
   }
   else if (KB_STATE[SDL_SCANCODE_C] && !kbPrevState[SDL_SCANCODE_C])
   {
      Cvec3 cam = g_eyeRbt.getTranslation();
      cout << "camera = <" << cam[0] << ", " << cam[1] << ", " << cam[2] << ">" << endl;
   }

   if (!KB_STATE[SDL_SCANCODE_B] && kbPrevState[SDL_SCANCODE_B])
   {
      // Call timer for animation reset
      float msToWait = 6.0 * 1000;
      g_animationReset = SDL_AddTimer((Uint32) msToWait, animationReset, (void *) "animationTimer Callback");
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
      //m = g_rigidBodies[0].rtf * RigTForm(Quat().makeXRotation(-dy)) * RigTForm(Quat().makeYRotation(dx)) * inv(g_rigidBodies[0].rtf);

      Quat rot = g_eyeRbt.getRotation() * Quat().makeXRotation(-dy) * Quat().makeYRotation(dx) * inv(g_eyeRbt.getRotation());
      m.setRotation(rot);
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
      //m = g_rigidBodies[0].rtf * RigTForm(Cvec3(0,0,dy) * 0.01) * inv(g_rigidBodies[0].rtf);
      m = g_eyeRbt * RigTForm(Cvec3(0, 0, dy) * 0.01) * inv(g_eyeRbt);
   }

   if (g_mouseClickDown)
      g_rigidBodies[0].rtf = m * g_rigidBodies[0].rtf;
   //g_eyeRbt = m * g_eyeRbt;

   g_mouseClickX = x;
   g_mouseClickY = g_windowHeight - y - 1;
}
/*-----------------------------------------------*/
void MySdlApplication::onLoop(int tick, int* prevPhysicsTick, int ticksPerPhysics)
{
   /*	PURPOSE:		Handles function calls that need to run once per SDL loop
      RECEIVES:   tick - current time in ms that program has been running
      prevPhysicsTick - Time in ms since last physics tick
      ticksPerPhysics - Time in between phsysics ticks in ms
      RETURNS:
      REMARKS:
      */

   // Logic goes here
   keyboard();

   while (tick > *prevPhysicsTick + ticksPerPhysics)
   {
      // Update particles
      bool areDeadParticles = false;
      for (int i = 0; i < (int) g_particles.size(); i++)
      {
         g_particles[i]->updateParticle(ticksPerPhysics);
         if (!areDeadParticles && !g_particles[i]->isAlive)
            areDeadParticles = true;
      }
      // Remove Erase dead particles
      if (areDeadParticles)
      {
         int size = g_particles.size();
         std::vector<Particle*>::iterator itrBegin = g_particles.begin();
         std::vector<Particle*>::iterator itrEnd = g_particles.end();
         g_particles.erase(itrBegin, std::remove_if(itrBegin, itrEnd, Particle::testAlive));
         areDeadParticles = false;
      }

      // Update Timer
      *prevPhysicsTick += ticksPerPhysics;
   }

   // Restart animation if need be
   if (!g_isAnimating && !g_isParticulating)
   {
      float fps = 30.0;
      float msecsPerFrame = (float) (1 / (fps / 1000.0));
      g_animationTimer = SDL_AddTimer((Uint32) msecsPerFrame, animateEcodTM, (void *) "animationTimer Callback");
      g_isAnimating = true;
      g_rigidBodies[0].isChildVisible = true;
   }
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

   int prevTick = SDL_GetTicks();
   int ticksPerPhysics = 1000 / 100;
   int prevPhysicsTick = prevTick;

   SDL_Event Event;
   while (running)
   {
      memcpy(kbPrevState, KB_STATE, sizeof(kbPrevState));

      while (SDL_PollEvent(&Event))
      {
         onEvent(&Event);
      }

      int tick = SDL_GetTicks();

      onLoop(tick, &prevPhysicsTick, ticksPerPhysics);
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

   srand((unsigned)time(NULL));
   KB_STATE = SDL_GetKeyboardState(NULL);
   g_particles.reserve(G_MAX_PARTICLES);

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
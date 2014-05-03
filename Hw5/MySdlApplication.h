#ifndef _MYSDLAPPLICATION_H_
#define _MYSDLAPPLICATION_H_

#define SDL_MAIN_HANDLED
#include <vector>
#include <string>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <stdlib.h>
#include <time.h>
#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include "cvec.h"
#include "matrix4.h"
#include "geometrymaker.h"
#include "ppm.h"
#include "glsupport.h"
#include "rigtform.h"

#if __GNUG__
#	include <tr1/memory>
#endif

// Macro used to obtain relative offset of a field within a struct
#define FIELD_OFFSET(StructType, field) &(((StructType *)0)->field)

class MySdlApplication
{
    private:
        bool running;
        SDL_Window* display;
        void keyboard(const char * key);
        void mouse(SDL_MouseButtonEvent button);
        void motion(const int x, const int y);

    public:
		MySdlApplication();
        int onExecute();
        bool onInit();
		void onEvent(SDL_Event* Event);
        void keyboard();
        void onLoop();
        void onRender();
        void onCleanup();
        
        static const bool G_GL2_COMPATIBLE;// = false; //Texture doesn't render in GL2 mode for some reason

        struct ShaderState
        {
           GlProgram program;

           // Handles to uniform variables
           GLint h_uLight, h_uLight2;
           GLint h_uProjMatrix;
           GLint h_uModelViewMatrix;
           GLint h_uNormalMatrix;
           GLint h_uColor;
           GLint h_uTexUnit0;
           GLint h_uTexUnit1;
           GLint h_uTexUnit2;
           GLint h_uSamples;
           GLint h_uSampledx;
           GLint h_uSampledy;

           // Handles to vertex attributes
           GLint h_aPosition;
           GLint h_aNormal;
           GLint h_aTangent;
           GLint h_aTexCoord0;
           GLint h_aTexCoord1;
           GLint h_aTexCoord2;

           /*-----------------------------------------------*/
           ShaderState(const char* vsfn, const char* fsfn)
           {
              /*	PURPOSE:		Constructor for ShaderState Object
              RECEIVES:	vsfn - Vertex Shader Filename
              fsfn - Fragement Shader Filename
              RETURNS:		ShaderState object
              REMARKS:
              */

              readAndCompileShader(program, vsfn, fsfn);

              const GLuint h = program; // short hand

              // Retrieve handles to uniform variables
              h_uLight = safe_glGetUniformLocation(h, "uLight");
              h_uLight2 = safe_glGetUniformLocation(h, "uLight2");
              h_uProjMatrix = safe_glGetUniformLocation(h, "uProjMatrix");
              h_uModelViewMatrix = safe_glGetUniformLocation(h, "uModelViewMatrix");
              h_uNormalMatrix = safe_glGetUniformLocation(h, "uNormalMatrix");
              h_uColor = safe_glGetUniformLocation(h, "uColor");
              h_uTexUnit0 = safe_glGetUniformLocation(h, "uTexUnit0");
              h_uTexUnit1 = safe_glGetUniformLocation(h, "uTexUnit1");
              h_uTexUnit2 = safe_glGetUniformLocation(h, "uTexUnit2");
              h_uSamples = safe_glGetUniformLocation(h, "uSamples");
              h_uSampledx = safe_glGetUniformLocation(h, "uSampledx");
              h_uSampledy = safe_glGetUniformLocation(h, "uSampledy");

              // Retrieve handles to vertex attributes
              h_aPosition = safe_glGetAttribLocation(h, "aPosition");
              h_aNormal = safe_glGetAttribLocation(h, "aNormal");
              h_aTangent = safe_glGetAttribLocation(h, "aTangent");
              h_aTexCoord0 = safe_glGetAttribLocation(h, "aTexCoord0");
              h_aTexCoord1 = safe_glGetAttribLocation(h, "aTexCoord1");
              h_aTexCoord2 = safe_glGetAttribLocation(h, "aTexCoord2");

              if (!G_GL2_COMPATIBLE)
                 glBindFragDataLocation(h, 0, "fragColor");
              checkGlErrors();
           }
           /*-----------------------------------------------*/
        };

        struct Geometry
        {
           GlBufferObject vbo, texVbo, ibo;
           int vboLen, iboLen;

           /*-----------------------------------------------*/
           Geometry(GenericVertex *vtx, unsigned short *idx, int vboLen, int iboLen)
           {
              /*	PURPOSE:		Constructor for Geometry Object
              RECEIVES:	vtx - vertex buffer array of Generic vertex
              idx - Index buffer array
              vboLen - Length of Vertex buffer array
              iboLen - Length of Index Buffer array
              RETURNS:		Geometry object
              REMARKS:
              */

              this->vboLen = vboLen;
              this->iboLen = iboLen;

              // Now create the VBO and IBO
              glBindBuffer(GL_ARRAY_BUFFER, vbo);
              glBufferData(GL_ARRAY_BUFFER, sizeof(GenericVertex)* vboLen, vtx,
                 GL_STATIC_DRAW);

              glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
              glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short)* iboLen,
                 idx, GL_STATIC_DRAW);

              glBindBuffer(GL_ARRAY_BUFFER, texVbo);
              glBufferData(GL_ARRAY_BUFFER, sizeof(GenericVertex)* vboLen, vtx,
                 GL_STATIC_DRAW);
           }
           /*-----------------------------------------------*/
           void draw(const ShaderState& curSS)
           {
              /*	PURPOSE:		Draws an OpenGL object
              RECEIVES:	curSS - ShaderState to be used when drawing
              RETURNS:
              REMARKS:
              */

              // Enable the attributes used by our shader
              safe_glEnableVertexAttribArray(curSS.h_aPosition);
              safe_glEnableVertexAttribArray(curSS.h_aNormal);
              safe_glEnableVertexAttribArray(curSS.h_aTexCoord0);
              safe_glEnableVertexAttribArray(curSS.h_aTexCoord1);
              safe_glEnableVertexAttribArray(curSS.h_aTexCoord2);

              // bind vbo
              glBindBuffer(GL_ARRAY_BUFFER, vbo);
              safe_glVertexAttribPointer(curSS.h_aPosition, 3, GL_FLOAT, GL_FALSE,
                 sizeof(GenericVertex), FIELD_OFFSET(GenericVertex, pos));
              safe_glVertexAttribPointer(curSS.h_aNormal, 3, GL_FLOAT, GL_FALSE,
                 sizeof(GenericVertex), FIELD_OFFSET(GenericVertex, normal));
              glBindBuffer(GL_ARRAY_BUFFER, texVbo);
              safe_glVertexAttribPointer(curSS.h_aTexCoord0, 2, GL_FLOAT, GL_FALSE,
                 sizeof(GenericVertex), FIELD_OFFSET(GenericVertex, tex));
              safe_glVertexAttribPointer(curSS.h_aTexCoord1, 2, GL_FLOAT, GL_FALSE,
                 sizeof(GenericVertex), FIELD_OFFSET(GenericVertex, tex));
              safe_glVertexAttribPointer(curSS.h_aTexCoord2, 2, GL_FLOAT, GL_FALSE,
                 sizeof(GenericVertex), FIELD_OFFSET(GenericVertex, tex));

              // bind ibo
              glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

              // draw!
              glDrawElements(GL_TRIANGLES, iboLen, GL_UNSIGNED_SHORT, 0);

              // Disable the attributes used by our shader
              safe_glDisableVertexAttribArray(curSS.h_aPosition);
              safe_glDisableVertexAttribArray(curSS.h_aNormal);
              safe_glDisableVertexAttribArray(curSS.h_aTexCoord0);
              safe_glDisableVertexAttribArray(curSS.h_aTexCoord1);
              safe_glDisableVertexAttribArray(curSS.h_aTexCoord2);
           }
           /*-----------------------------------------------*/
           void draw(const ShaderState& curSS, Matrix4 MVM)
           {
              /*	PURPOSE:		Draws an OpenGL object with a specific Model View Matrix
              RECEIVES:	curSS - ShaderState to be used when drawing
              MVM - Model View Matrix to be drawn against
              RETURNS:
              REMARKS:
              */

              Matrix4 NMVM = normalMatrix(MVM);

              GLfloat glmatrix[16];
              MVM.writeToColumnMajorMatrix(glmatrix); // send MVM
              safe_glUniformMatrix4fv(curSS.h_uModelViewMatrix, glmatrix);

              NMVM.writeToColumnMajorMatrix(glmatrix); // send NMVM
              safe_glUniformMatrix4fv(curSS.h_uNormalMatrix, glmatrix);

              draw(curSS);
           }
        };
        
        static const ShaderState& setupShader(int material);
};

#endif
//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_FASTRENDER_H
#define _OGL_FASTRENDER_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

#include "ogl_defs.h"

//------------------------------------------------------------------------------

bool G3DrawFaceSimple (grsFace *faceP, grsBitmap *bmBot, grsBitmap *bmTop, int bBlend, int bTextured, int bDepthOnly);
bool G3DrawFaceArrays (grsFace *faceP, grsBitmap *bmBot, grsBitmap *bmTop, int bBlend, int bTextured, int bDepthOnly);
void G3FlushFaceBuffer (int bForce);
int G3SetupShader (int bColorKey, int bMultiTexture, int bTextured, tRgbaColorf *colorP);

//------------------------------------------------------------------------------

static inline bool FaceIsAdditive (grsFace *faceP)
{
return (faceP->bAdditive == 2) || ((faceP->bAdditive == 1) && !(faceP->bmBot && faceP->bmBot->bmFromPog));
}

//------------------------------------------------------------------------------

static inline bool G3DrawFace (grsFace *faceP, grsBitmap *bmBot, grsBitmap *bmTop, 
										 int bBlend, int bTextured, int bDepthOnly, int bVertexArrays)
{
return bVertexArrays ?
		G3DrawFaceArrays (faceP, bmBot, bmTop, bBlend, bTextured, bDepthOnly) :
		G3DrawFaceSimple (faceP, bmBot, bmTop, bBlend, bTextured, bDepthOnly);
}

//------------------------------------------------------------------------------

#endif

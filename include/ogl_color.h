//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_COLOR_H
#define _OGL_COLOR_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

#include "inferno.h"
#include "gr.h"

//------------------------------------------------------------------------------

#define CPAL2Tr(_p,_c)	((float) ((_p)[(_c)*3])/63.0f)
#define CPAL2Tg(_p,_c)	((float) ((_p)[(_c)*3+1])/63.0f)
#define CPAL2Tb(_p,_c)	((float) ((_p)[(_c)*3+2])/63.0f)
#define PAL2Tr(_p,_c)	((float) ((_p)[(_c)*3])/63.0f)
#define PAL2Tg(_p,_c)	((float) ((_p)[(_c)*3+1])/63.0f)
#define PAL2Tb(_p,_c)	((float) ((_p)[(_c)*3+2])/63.0f)

#define CC2T(c) ((float) c / 255.0f)

//------------------------------------------------------------------------------

extern tFaceColor lightColor;
extern tFaceColor tMapColor;
extern tFaceColor vertColors [8];

extern tRgbaColorf shadowColor [2];
extern tRgbaColorf modelColor [2];

extern float fLightRanges [5];

//------------------------------------------------------------------------------

void OglPalColor (ubyte *palette, int c);
void OglGrsColor (grsColor *pc);
void OglColor4sf (float r, float g, float b, float s);
void SetTMapColor (tUVL *uvlList, int i, grsBitmap *bmP, int bResetColor, tFaceColor *vertColor);
int G3AccumVertColor (int nVertex, fVector3 *pColorSum, tVertColorData *vcdP, int nThread);
void G3VertexColor (fVector3 *pvVertNorm, fVector3 *pVertPos, int nVertex, 
						  tFaceColor *pVertColor, tFaceColor *pBaseColor, 
						  float fScale, int bSetColor, int nThread);
void InitVertColorData (tVertColorData& vcd);

//------------------------------------------------------------------------------

#endif

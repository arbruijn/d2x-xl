//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_COLOR_H
#define _OGL_COLOR_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

#include "descent.h"
#include "gr.h"

//------------------------------------------------------------------------------

#define CPAL2Tr(_p,_c)	((float) ((_p).Raw()[(_c)*3])/63.0f)
#define CPAL2Tg(_p,_c)	((float) ((_p).Raw()[(_c)*3+1])/63.0f)
#define CPAL2Tb(_p,_c)	((float) ((_p).Raw()[(_c)*3+2])/63.0f)

#define CC2T(c) ((float) c / 255.0f)

//------------------------------------------------------------------------------

extern CFaceColor lightColor;
extern CFaceColor tMapColor;
extern CFaceColor vertColors [8];

extern CFloatVector shadowColor [2];
extern CFloatVector modelColor [2];

extern float fLightRanges [5];

//------------------------------------------------------------------------------

void OglPalColor (uint8_t *palette, int32_t c, CFloatVector* pColor = NULL);
CFloatVector GetPalColor (CPalette *palette, int32_t c);
void OglCanvasColor (CCanvasColor* pCanvColor, CFloatVector* pColor = NULL);
CFloatVector GetCanvasColor (CCanvasColor* pColor);
void OglColor4sf (float r, float g, float b, float s);
void SetTMapColor (tUVL *uvlList, int32_t i, CBitmap *pBm, int32_t bResetColor, CFloatVector *pColor = NULL);
int32_t ComputeVertexColor (int32_t nSegment, int32_t nSide, int32_t nVertex, CFloatVector3 *pColorSum, CVertColorData *vcdP, int32_t nThread);
void GetVertexColor (int32_t nSegment, int32_t nSide, int32_t nVertex, 
						  CFloatVector3 *pvVertNorm, CFloatVector3 *pVertPos, 
						  CFaceColor *pVertColor, CFaceColor *pBaseColor, 
						  float fScale, int32_t bSetColor, int32_t nThread);
void InitVertColorData (CVertColorData& vcd);

//------------------------------------------------------------------------------

#endif

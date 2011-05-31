/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"
#include "3d.h"
#include "globvars.h"
#include "fix.h"
#include "light.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_render.h"
#include "transprender.h"

#define RESCALE_ROD	0

grsPoint blobVertices [4];
CRenderPoint rodPoints [4];
CRenderPoint *rodPointList [] = {rodPoints, rodPoints + 1, rodPoints + 2, rodPoints + 3};

tUVL rodUvlList [4] = {
 {0x0200, 0x0200, 0},
 {0xfe00, 0x0200, 0},
 {0xfe00, 0xfe00, 0},
 {0x0200, 0xfe00, 0}};

//------------------------------------------------------------------------------
//compute the corners of a rod.  fills in vertbuf.
int CalcRodCorners (CRenderPoint *btmPoint, fix xBtmWidth, CRenderPoint *topPoint, fix xTopWidth)
{
	CFixVector	vDelta, vTop, vTemp, vRodNorm;
	ubyte			andCodes;
	int			i;

//compute vector from one point to other, do cross product with vector
//from eye to get perpendicular
vDelta = btmPoint->ViewPos () - topPoint->ViewPos ();
//unscale for aspect
#if RESCALE_ROD
vDelta.p.x = FixDiv (vDelta.p.x, transformation.m_info.scale.p.x);
vDelta.p.y = FixDiv (vDelta.p.y, transformation.m_info.scale.p.y);
#endif
//calc Perp vector
//do lots of normalizing to prevent overflowing.  When this code works,
//it should be optimized
CFixVector::Normalize (vDelta);
vTop = topPoint->ViewPos ();
CFixVector::Normalize (vTop);
vRodNorm = CFixVector::Cross (vDelta, vTop);
CFixVector::Normalize (vRodNorm);
//scale for aspect
#if RESCALE_ROD
vRodNorm.p.x = FixMul (vRodNorm.p.x, transformation.m_info.scale.p.x);
vRodNorm.p.y = FixMul (vRodNorm.p.y, transformation.m_info.scale.p.y);
#endif
//now we have the usable edge.  generate four points
//vTop points
vTemp = vRodNorm * xTopWidth;
vTemp.v.coord.z = 0;
rodPoints [0].ViewPos () = topPoint->ViewPos () + vTemp;
rodPoints [1].ViewPos () = topPoint->ViewPos () - vTemp;
vTemp = vRodNorm * xBtmWidth;
vTemp.v.coord.z = 0;
rodPoints [2].ViewPos () = btmPoint->ViewPos () - vTemp;
rodPoints [3].ViewPos () = btmPoint->ViewPos () + vTemp;

//now code the four points
for (i = 0, andCodes = 0xff; i < 4; i++)
	andCodes &= rodPoints [i].Encode ();
if (andCodes)
	return 1;		//1 means off screen
//clear flags for new points (not projected)
for (i = 0; i < 4; i++) {
	rodPoints [i].SetFlags ();
	rodPoints [i].SetIndex (-1);
	}
return 0;
}

//------------------------------------------------------------------------------
//draw a polygon that is always facing you
//returns 1 if off screen, 0 if drew
int G3DrawRodPoly (CRenderPoint *btmPoint, fix xBtmWidth, CRenderPoint *topPoint, fix xTopWidth)
{
if (CalcRodCorners (btmPoint, xBtmWidth, topPoint, xTopWidth))
	return 0;
return G3DrawPoly (4, rodPointList);
}

//------------------------------------------------------------------------------
//draw a bitmap CObject that is always facing you
//returns 1 if off screen, 0 if drew
int G3DrawRodTexPoly (CBitmap *bmP, CRenderPoint *btmPoint, fix xBtmWidth, CRenderPoint *topPoint, fix xTopWidth, fix light, tUVL *uvlList, int bAdditive)
{
if (CalcRodCorners (btmPoint, xBtmWidth, topPoint, xTopWidth))
	return 0;
if (!uvlList)
	uvlList = rodUvlList;
uvlList [0].l =
uvlList [1].l =
uvlList [2].l =
uvlList [3].l = light;
return G3DrawTexPoly (4, rodPointList, uvlList, bmP, NULL, 1, bAdditive, -1);
}

//------------------------------------------------------------------------------
//draw an CObject that is a texture-mapped rod
void DrawObjectRodTexPoly (CObject *objP, tBitmapIndex bmi, int bLit, int iFrame)
{
	CBitmap*			bmP = gameData.pig.tex.bitmaps [0] + bmi.index;
	CRenderPoint	pTop, pBottom;

LoadTexture (bmi.index, 0);
if ((bmP->Type () == BM_TYPE_STD) && bmP->Override ()) {
	bmP->SetupTexture (1, 0);
	bmP = bmP->Override (iFrame);
	}
CFixVector delta = objP->info.position.mOrient.m.dir.u * objP->info.xSize;
CFixVector vTop = objP->info.position.vPos + delta;
CFixVector vBottom = objP->info.position.vPos - delta;
pTop.TransformAndEncode (vTop);
pBottom.TransformAndEncode (vBottom);
fix light = bLit ? ComputeObjectLight (objP, &pTop.ViewPos ()) : I2X (1);
if (!gameStates.render.bPerPixelLighting)
	G3DrawRodTexPoly (bmP, &pBottom, objP->info.xSize, &pTop, objP->info.xSize, light, NULL, objP->info.nType == OBJ_FIREBALL);
else {
	if (CalcRodCorners (&pBottom, objP->info.xSize, &pTop, objP->info.xSize))
		return;

	CFloatVector	vertices [4];
	tTexCoord2f		texCoords [4]; // = {{0,0},{u,0},{u,v},{0,v}};

	for (int i = 0; i < 4; i++) {
		vertices [i].Assign (rodPoints [i].ViewPos ());
		texCoords [i].v.u = X2F (rodUvlList [i].u);
		texCoords [i].v.v = X2F (rodUvlList [i].v);
		}
	if (objP->info.nType == OBJ_FIREBALL)
		transparencyRenderer.AddPoly (NULL, NULL, bmP, vertices, 4, texCoords, NULL, &gameData.objs.color, 1, 1, GL_TRIANGLE_FAN, GL_REPEAT, 1, -1);
	else {
		bmP = bmP->Override (-1);
		bmP->SetupTexture (1, 0);
		bmP->SetTexCoord (texCoords);
		bmP->SetColor (&gameData.objs.color);
		ogl.RenderQuad (bmP, vertices, 3);
		}
	}
}

//------------------------------------------------------------------------------
//eof

/* $Id: render.h,v 1.4 2003/10/10 09:36:35 btb Exp $ */
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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _RENDERLIB_H
#define _RENDERLIB_H

#include "inferno.h"

//------------------------------------------------------------------------------

#define VISITED(_ch)	(gameData.render.mine.bVisited [_ch] == gameData.render.mine.nVisited)
#define VISIT(_ch) (gameData.render.mine.bVisited [_ch] = gameData.render.mine.nVisited)

//------------------------------------------------------------------------------

typedef struct tFaceProps {
	short			segNum, sideNum;
	short			nBaseTex, nOvlTex, nOvlOrient;
	tUVL			uvls [4];
	short			vp [5];
#if LIGHTMAPS
	tUVL			uvl_lMaps [4];
#endif
	vmsVector	vNormal;
	ubyte			nVertices;
	ubyte			widFlags;
	char			nType;
} tFaceProps;

typedef struct tFaceListEntry {
	short			nextFace;
	tFaceProps	props;
} tFaceListEntry;

//------------------------------------------------------------------------------

int LoadExplBlast (void);
void FreeExplBlast (void);
int LoadCorona (void);
int LoadGlare (void);
int LoadHalo (void);
int LoadThruster (void);
int LoadShield (void);
int LoadDeadzone (void);
void LoadExtraImages (void);
void FreeExtraImages (void);

// Given a list of point numbers, rotate any that haven't been rotated
// this frame
g3sCodes RotateVertexList (int nv, short *pointIndex);
void RotateSideNorms (void);
// Given a list of point numbers, project any that haven't been projected
void ProjectVertexList (int nv, short *pointIndex);

void TransformSideCenters (void);
#if USE_SEGRADS
void TransformSideCenters (void);
#endif

int IsTransparentTexture (short nTexture);
int SetVertexColor (int nVertex, tFaceColor *pc);
int SetVertexColors (tFaceProps *propsP);
fix SetVertexLight (int nSegment, int nSide, int nVertex, tFaceColor *pc, fix light);
int SetFaceLight (tFaceProps *propsP);
void AdjustVertexColor (grsBitmap *bmP, tFaceColor *pc, fix xLight);
char IsColoredSegFace (short nSegment, short nSide);
tRgbaColorf *ColoredSegmentColor (int nSegment, int nSide, char nColor);
int IsMonitorFace (short nSegment, short nSide);
float WallAlpha (short nSegment, short nSide, short nWall, ubyte widFlags, int bIsMonitor, tRgbaColorf *pc, int *bCloaking, ubyte *bTextured);
int SetupMonitorFace (short nSegment, short nSide, short nCamera, grsFace *faceP);
grsBitmap *LoadFaceBitmap (short tMapNum, short nFrameNum);
void DrawOutline (int nVertices, g3sPoint **pointList);
int ToggleOutlineMode (void);
int ToggleShowOnlyCurSide (void);
void RotateTexCoord2f (tTexCoord2f *pDest, tTexCoord2f *pSrc, ubyte nOrient);
int FaceIsVisible (short nSegment, short nSide);

#ifdef _DEBUG
void OutlineSegSide (tSegment *seg, int _side, int edge, int vert);
void DrawWindowBox (unsigned int color, short left, short top, short right, short bot);
#endif

//------------------------------------------------------------------------------

extern grsBitmap *bmpCorona;
extern grsBitmap *bmpGlare;
extern grsBitmap *bmpHalo;
extern grsBitmap *bmpThruster [2];
extern grsBitmap *bmpShield;
extern grsBitmap *bmpExplBlast;

extern tRgbaColorf segmentColors [4];

#ifdef _DEBUG
extern short nDbgSeg;
extern short nDbgSide;
extern int nDbgVertex;
extern int nDbgBaseTex;
extern int nDbgOvlTex;
#endif

extern int bOutLineMode, bShowOnlyCurSide;

//------------------------------------------------------------------------------

static inline int IsTransparentFace (tFaceProps *propsP)
{
return IsTransparentTexture (gameData.segs.segments [propsP->segNum].sides [propsP->sideNum].nBaseTex);
}

//------------------------------------------------------------------------------

#endif // _RENDERLIB_H

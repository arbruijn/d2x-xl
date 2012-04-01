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

#include "descent.h"

//------------------------------------------------------------------------------

#define VISITED(_nSegment,_nThread)	(gameData.render.mine.visibility [nThread].bVisited [_nSegment] == gameData.render.mine.visibility [nThread].nVisited)
#define VISIT(_nSegment,_nThread) (gameData.render.mine.visibility [_nThread].bVisited [_nSegment] = gameData.render.mine.visibility [nThread].nVisited)

//------------------------------------------------------------------------------

typedef struct tFaceProps {
	short			segNum, sideNum;
	short			nBaseTex, nOvlTex, nOvlOrient;
	tUVL			uvls [4];
	short			vp [5];
	tUVL			uvl_lMaps [4];
	CFixVector	vNormal;
	ubyte			nVertices;
	ubyte			widFlags;
	char			nType;
} tFaceProps;

typedef struct tFaceListEntry {
	short			nextFace;
	tFaceProps	props;
} tFaceListEntry;

// Given a list of point numbers, rotate any that haven't been rotated
// this frame
tRenderCodes TransformVertexList (int nVerts, ushort* vertexIndexP);
void RotateSideNorms (void);
// Given a list of point numbers, project any that haven't been projected

void TransformSideCenters (void);
#if USE_SEGRADS
void TransformSideCenters (void);
#endif

int IsTransparentTexture (short nTexture);
void AlphaBlend (CFloatVector& dest, CFloatVector& other, float fAlpha);
int SetVertexColor (int nVertex, CFaceColor *pc, int bBlend = 0);
int SetVertexColors (tFaceProps *propsP);
fix SetVertexLight (int nSegment, int nSide, int nVertex, CFaceColor *pc, fix light);
int SetFaceLight (tFaceProps *propsP);
void AdjustVertexColor (CBitmap *bmP, CFaceColor *pc, fix xLight);
char IsColoredSeg (short nSegment);
char IsColoredSegFace (short nSegment, short nSide);
CFloatVector *ColoredSegmentColor (int nSegment, int nSide, char nColor);
int IsMonitorFace (short nSegment, short nSide, int bForce);
float WallAlpha (short nSegment, short nSide, short nWall, ubyte widFlags, int bIsMonitor, ubyte bAdditive,
					  CFloatVector *colorP, int& nColor, ubyte& bTextured, ubyte& bCloaked, ubyte& bTransparent);
int SetupMonitorFace (short nSegment, short nSide, short nCamera, CSegFace *faceP);
CBitmap *LoadFaceBitmap (short nTexture, short nFrameIdx, int bLoadTextures = 1);
void DrawOutline (int nVertices, CRenderPoint **pointList);
int ToggleOutlineMode (void);
int ToggleShowOnlyCurSide (void);
void RotateTexCoord2f (tTexCoord2f& dest, tTexCoord2f& src, ubyte nOrient);
int FaceIsVisible (short nSegment, short nSide);
int SegmentMayBeVisible (short nStartSeg, short nRadius, int nMaxDist, int nThread = 0);
void SetupMineRenderer (void);
void ComputeMineLighting (short nStartSeg, fix xStereoSeparation, int nWindow);

#if DBG
void OutlineSegSide (CSegment *seg, int _side, int edge, int vert);
void DrawWindowBox (uint color, short left, short top, short right, short bot);
#endif

//------------------------------------------------------------------------------

extern CFloatVector segmentColors [4];

#if DBG
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
return IsTransparentTexture (SEGMENTS [propsP->segNum].m_sides [propsP->sideNum].m_nBaseTex);
}

//------------------------------------------------------------------------------

#endif // _RENDERLIB_H

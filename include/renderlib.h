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

#define VISITED(_nSegment,_nThread)	(gameData.render.mine.visibility [_nThread].bVisited [_nSegment] == gameData.render.mine.visibility [_nThread].nVisited)
#define VISIT(_nSegment,_nThread) (gameData.render.mine.visibility [_nThread].bVisited [_nSegment] = gameData.render.mine.visibility [_nThread].nVisited)

//------------------------------------------------------------------------------

typedef struct tFaceProps {
	int16_t			segNum, sideNum;
	int16_t			nBaseTex, nOvlTex, nOvlOrient;
	tUVL			uvls [4];
	int16_t			vp [5];
	tUVL			uvl_lMaps [4];
	CFixVector	vNormal;
	uint8_t			nVertices;
	uint8_t			widFlags;
	char			nType;
} tFaceProps;

typedef struct tFaceListEntry {
	int16_t			nextFace;
	tFaceProps	props;
} tFaceListEntry;

// Given a list of point numbers, rotate any that haven't been rotated
// this frame
tRenderCodes TransformVertexList (int32_t nVerts, uint16_t* vertexIndexP);
void RotateSideNorms (void);
// Given a list of point numbers, project any that haven't been projected

void TransformSideCenters (void);
#if USE_SEGRADS
void TransformSideCenters (void);
#endif

int32_t IsTransparentTexture (int16_t nTexture);
void AlphaBlend (CFloatVector& dest, CFloatVector& other, float fAlpha);
int32_t SetVertexColor (int32_t nVertex, CFaceColor *pc, int32_t bBlend = 0);
int32_t SetVertexColors (tFaceProps *propsP);
fix SetVertexLight (int32_t nSegment, int32_t nSide, int32_t nVertex, CFaceColor *pc, fix light);
int32_t SetFaceLight (tFaceProps *propsP);
void AdjustVertexColor (CBitmap *bmP, CFaceColor *pc, fix xLight);
char IsColoredSeg (int16_t nSegment);
char IsColoredSegFace (int16_t nSegment, int16_t nSide);
CFloatVector *ColoredSegmentColor (int32_t nSegment, int32_t nSide, char nColor);
int32_t IsMonitorFace (int16_t nSegment, int16_t nSide, int32_t bForce);
float WallAlpha (int16_t nSegment, int16_t nSide, int16_t nWall, uint8_t widFlags, int32_t bIsMonitor, uint8_t bAdditive,
					  CFloatVector *colorP, int32_t& nColor, uint8_t& bTextured, uint8_t& bCloaked, uint8_t& bTransparent);
int32_t SetupMonitorFace (int16_t nSegment, int16_t nSide, int16_t nCamera, CSegFace *faceP);
CBitmap *LoadFaceBitmap (int16_t nTexture, int16_t nFrameIdx, int32_t bLoadTextures = 1);
int32_t BitmapFrame (CBitmap* bmP, int16_t nTexture, int16_t nSegment, int32_t nFrame = -1);
void DrawOutline (int32_t nVertices, CRenderPoint **pointList);
int32_t ToggleOutlineMode (void);
int32_t ToggleShowOnlyCurSide (void);
void RotateTexCoord2f (tTexCoord2f& dest, tTexCoord2f& src, uint8_t nOrient);
int32_t FaceIsVisible (int16_t nSegment, int16_t nSide);
int32_t SegmentMayBeVisible (int16_t nStartSeg, int32_t nRadius, int32_t nMaxDist, int32_t nThread = 0);
void SetupMineRenderer (int32_t nWindow);
void ComputeMineLighting (int16_t nStartSeg, fix xStereoSeparation, int32_t nWindow);

#if DBG
void OutlineSegSide (CSegment *seg, int32_t _side, int32_t edge, int32_t vert);
void DrawWindowBox (uint32_t color, int16_t left, int16_t top, int16_t right, int16_t bot);
#endif

int32_t SetRearView (int32_t bOn);
int32_t ToggleRearView (void);
void ResetRearView (void);
void CheckRearView (void);
int32_t SetChaseCam (int32_t bOn);
int32_t ToggleChaseCam (void);
int32_t SetFreeCam (int32_t bOn);
int32_t ToggleFreeCam (void);
void ToggleRadar (void);
void HandleZoom (void);

//------------------------------------------------------------------------------

extern CFloatVector segmentColors [4];

#if DBG
extern int16_t nDbgSeg;
extern int16_t nDbgSide;
extern int32_t nDbgVertex;
extern int32_t nDbgBaseTex;
extern int32_t nDbgOvlTex;
#endif

extern int32_t bOutLineMode, bShowOnlyCurSide;

//------------------------------------------------------------------------------

static inline int32_t IsTransparentFace (tFaceProps *propsP)
{
return IsTransparentTexture (SEGMENT (propsP->segNum)->m_sides [propsP->sideNum].m_nBaseTex);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

class CFrameController {
	private:
		int32_t	m_nFrames;
		int32_t	m_iFrame;
		int32_t	m_nEye;
		int32_t	m_nOffsetSave;

	public:
		CFrameController ();
		void Begin (void);
		bool Continue (void);
		void End (void);
		void Setup (void);

	};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


#endif // _RENDERLIB_H

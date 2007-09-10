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

#ifndef _RENDER_H
#define _RENDER_H

#include "3d.h"

#include "object.h"

#define	APPEND_LAYERED_TEXTURES 0

extern int nClearWindow;    // 1 = Clear whole background window, 2 = clear view portals into rest of world, 0 = no clear

extern float coronaIntensities [];

void GameRenderFrame (void);
void RenderFrame (fix eye_offset, int window_num);  //draws the world into the current canvas

// cycle the flashing light for when mine destroyed
void FlashFrame();

int find_seg_side_face(short x,short y,int *seg,int *tSide,int *face,int *poly);

// these functions change different rendering parameters
// all return the new value of the parameter

// how may levels deep to render
int inc_render_depth(void);
int dec_render_depth(void);
int reset_render_depth(void);

// how many levels deep to render in perspective
int inc_perspective_depth(void);
int dec_perspective_depth(void);
int reset_perspective_depth(void);

// misc toggles
int ToggleOutlineMode(void);
int ToggleShowOnlyCurSide(void);

// When any render function needs to know what's looking at it, it
// should access RenderViewerObject members.
extern fix xRenderZoom;     // the tPlayer's zoom factor

// This is used internally to RenderFrame(), but is included here so AI
// can use it for its own purposes.

extern short nRenderList [MAX_RENDER_SEGS];

#ifdef EDITOR
extern int Render_only_bottom;
#endif


// Set the following to turn on tPlayer head turning
// If the above flag is set, these angles specify the orientation of the head
extern vmsAngVec Player_head_angles;

//
// Routines for conditionally rotating & projecting points
//

// This must be called at the start of the frame if RotateVertexList() will be used
void RenderStartFrame(void);

// Given a list of point numbers, rotate any that haven't been rotated
// this frame
g3s_codes RotateVertexList(int nv, short *pointnumlist);

// Given a list of point numbers, project any that haven't been projected
void ProjectVertexList(int nv, short *pointnumlist);
void RenderMine (short nStartSeg, fix xExeOffset, int nWindow);
void RenderShadowQuad (int bWhite);
int RenderShadowMap (tDynLight *pLight);
void UpdateRenderedData (int window_num, tObject *viewer, int rearViewFlag, int user);

extern tFlightPath externalView;

void ResetFlightPath (tFlightPath *pPath, int nSize, int nFPS);
void SetPathPoint (tFlightPath *pPath, tObject *objP);
tPathPoint *GetPathPoint (tFlightPath *pPath);
int IsTransparentTexture (short nTexture);
int LoadExplBlast (void);
void FreeExplBlast (void);
int LoadCorona (void);
int LoadHalo (void);
int LoadThruster (void);
int LoadShield (void);
int LoadDeadzone (void);
void FreeExtraImages (void);
void CalcSpriteCoords (fVector *vSprite, fVector *vCenter, fVector *vEye, float dx, float dy, fMatrix *r);
void RenderMineSegment (int nn);

extern grsBitmap *bmpCorona;
extern grsBitmap *bmpHalo;
extern grsBitmap *bmpThruster [2];
extern grsBitmap *bmpShield;
extern grsBitmap *bmpExplBlast;

//------------------------------------------------------------------------------

static inline tObject *GuidedMslView (void)
{
	tObject *objP;

return (objP = gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer]) && 
		 (objP->nType == OBJ_WEAPON) && 
		 (objP->id == GUIDEDMSL_ID) && 
		 (objP->nSignature == gameData.objs.guidedMissileSig [gameData.multiplayer.nLocalPlayer]) ?
	objP : NULL;
}

//------------------------------------------------------------------------------

static inline tObject *GuidedInMainView (void)
{
return gameOpts->render.cockpit.bGuidedInMainView ? GuidedMslView () : NULL;
}

//------------------------------------------------------------------------------

#define ITEM_DEPTHBUFFER_SIZE	100000
#define ITEM_BUFFER_SIZE		1000000

typedef enum tRenderItemType {
	riPoly,
	riSprite,
	riSphere,
	riParticle,
	riLightning
} tRenderItemType;

typedef struct tRIPoly {
	grsBitmap			*bmP;
	fVector				vertices [4];
	tUVLf					texCoord [4];
	tRgbaColorf			color [4];
	int					nWrap;
	int					nPrimitive;
	char					nVertices;
	char					nColors;
	char					bDepthMask;
} tRIPoly;

typedef struct tRISprite {
	grsBitmap			*bmP;
	fVector				position;
	tRgbaColorf			color;
	int					nWidth;
	int					nHeight;
	char					nFrame;
	char					bColor;
} tRISprite;

typedef struct tRIParticle {
	tParticle			*particle;
	double				fBrightness;
} tRIParticle;

typedef enum tRISphereType {
	riSphereShield,
	riMonsterball
} tRISphereType;

typedef struct tRISphere {
	tRISphereType		nType;
	tRgbaColorf			color;
	tObject				*objP;
} tRISphere;

typedef struct tRILightning {
	tLightning			*lightning;
	short					nDepth;
} tRILightning;

typedef struct tRenderItem {
	struct tRenderItem	*pNextItem;
	tRenderItemType		nType;
	int						z;
	union {
		tRIPoly				poly;
		tRISprite			sprite;
		tRIParticle			particle;
		tRISphere			sphere;
		tRILightning		lightning;
	} item;
} tRenderItem;

typedef struct tRenderItemBuffer {
	tRenderItem		**pDepthBuffer;
	tRenderItem		*pItemList;
	int				nItems;
	int				nFreeItems;
	int				zMin;
	int				zMax;
	double			zScale;
	int				nWrap;
	char				bClientState;
	char				bTextured;
	char				bClientColor;
	char				bClientTexCoord;
	char				bDepthMask;
	grsBitmap		*bmP;
} tRenderItemBuffer;

int AllocRenderItemBuffer (void);
void FreeRenderItemBuffer (void);
void ResetRenderItemBuffer (void);
void InitRenderItemBuffer (int zMin, int zMax);
int AddRenderItem (tRenderItemType nType, void *itemData, int itemSize, int z);
int RIAddPoly (grsBitmap *bmP, fVector *vertices, char nVertices, tUVLf *texCoord, tRgbaColorf *color, 
					tFaceColor *altColor, char nColors, char bDepthMask, int nPrimitive, int nWrap);
int RIAddSprite (grsBitmap *bmP, vmsVector *position, tRgbaColorf *color, int nWidth, int nHeight, char nFrame);
int RIAddSphere (tRISphereType nType, float red, float green, float blue, float alpha, tObject *objP);
int RIAddParticle (tParticle *particle, double fBrightness);
int RIAddLightnings (tLightning *lightnings, short nLightnings, short nDepth);
void RenderItems (void);

//------------------------------------------------------------------------------

#endif /* _RENDER_H */

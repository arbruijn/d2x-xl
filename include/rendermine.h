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

#include "descent.h"
#include "network.h"
#include "3d.h"
#include "object.h"
#include "renderlib.h"
#include "flightpath.h"

#define	APPEND_LAYERED_TEXTURES 0

extern int nClearWindow;    // 1 = Clear whole background window, 2 = clear view portals into rest of world, 0 = no clear

void GameRenderFrame (void);
void RenderFrame (fix eye_offset, int window_num);  //draws the world into the current canvas
int RenderMissileView (void);

// cycle the flashing light for when mine destroyed
void FlashFrame (void);

int FindSegSideFace (short x,short y,int *seg,int *CSide,int *face,int *poly);

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
extern fix xRenderZoom;     // the CPlayerData's zoom factor

// This is used internally to RenderFrame(), but is included here so AI
// can use it for its own purposes.

extern short nRenderList [MAX_SEGMENTS_D2X];

// Set the following to turn on CPlayerData head turning
// If the above flag is set, these angles specify the orientation of the head
extern CAngleVector Player_head_angles;

//
// Routines for conditionally rotating & projecting points
//

// This must be called at the start of the frame if RotateVertexList() will be used
void RenderStartFrame (void);
void SetRenderView (fix nEyeOffset, short *pnStartSeg, int bOglScale);

void RenderMine (short nStartSeg, fix xExeOffset, int nWindow);
void RenderShadowQuad (int bWhite);
int RenderShadowMap (CDynLight *pLight);
void UpdateRenderedData (int window_num, CObject *viewer, int rearViewFlag, int user);
void RenderObjList (int nListPos, int nWindow);
void RenderMineSegment (int nn);
void RenderEffects (int nWindow);

void InitSegZRef (int i, int j, int nThread);
void QSortSegZRef (short left, short right);

void BuildRenderSegList (short nStartSeg, int nWindow);

void BuildRenderObjLists (int nSegCount);
void BuildRenderSegList (short nStartSeg, int nWindow);
void BuildRenderSegListFast (short nStartSeg, int nWindow);

//------------------------------------------------------------------------------

static inline bool GuidedMslView (CObject ** objPP)
{
	CObject *objP = gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP;

*objPP = objP;
return objP && 
		 (objP->info.nType == OBJ_WEAPON) && 
		 (objP->info.nId == GUIDEDMSL_ID) && 
		 (objP->info.nSignature == gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].nSignature);
}

//------------------------------------------------------------------------------

static inline CObject *GuidedInMainView (void)
{
if (!gameOpts->render.cockpit.bGuidedInMainView)
	return NULL;

CObject *objP;

return GuidedMslView (&objP) ? objP : NULL;
}

//------------------------------------------------------------------------------

static inline int StencilOff (void)
{
if (!SHOW_SHADOWS || (gameStates.render.nShadowPass != 3))
	return 0;
glDisable (GL_STENCIL_TEST);
gameStates.ogl.nStencil--;
return 1;
}

static inline void StencilOn (int bStencil)
{
if (bStencil) {
	glEnable (GL_STENCIL_TEST);
	gameStates.ogl.nStencil++;
	}
}

//------------------------------------------------------------------------------

#endif /* _RENDER_H */

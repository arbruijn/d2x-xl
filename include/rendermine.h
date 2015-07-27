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
#include "ogl_lib.h"
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
extern fix xRenderZoom;     // the player's zoom factor

// This is used internally to RenderFrame(), but is included here so AI
// can use it for its own purposes.

extern short nRenderList [MAX_SEGMENTS_D2X];

// Set the following to turn on player head turning
// If the above flag is set, these angles specify the orientation of the head
extern CAngleVector Player_head_angles;

//
// Routines for conditionally rotating & projecting points
//

void RenderStartFrame (void);
void SetRenderView (fix xStereoSeparation, short *nStartSegP, int bOglScale);

void RenderMine (short nStartSeg, fix xExeOffset, int nWindow);
void RenderShadowQuad (void);
void UpdateRenderedData (int window_num, CObject *viewer, int rearViewFlag, int user);
void RenderObjList (int nListPos, int nWindow);
void RenderMineSegment (int nn);
void RenderEffects (int nWindow);
void RenderSkyBox (int nWindow);

void BuildRenderObjLists (int nSegCount, int nThread = 0);
void BuildRenderSegList (short nStartSeg, int nWindow, bool bIgnoreDoors = false, int nThread = 0);

void ResetFaceList (void);
int AddFaceListItem (CSegFace *faceP, int nThread);

//------------------------------------------------------------------------------

static inline bool GuidedMslView (CObject ** objPP)
{
	CObject *objP = gameData.objs.guidedMissile [N_LOCALPLAYER].objP;

*objPP = objP;
return objP && 
		 (objP->info.nType == OBJ_WEAPON) && 
		 (objP->info.nId == GUIDEDMSL_ID) && 
		 (objP->info.nSignature == gameData.objs.guidedMissile [N_LOCALPLAYER].nSignature);
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

#endif /* _RENDER_H */

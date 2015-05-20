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

extern int32_t nClearWindow;    // 1 = Clear whole background window, 2 = clear view portals into rest of world, 0 = no clear

void GameRenderFrame (void);
void RenderFrame (fix eye_offset, int32_t window_num);  //draws the world into the current canvas
int32_t RenderMissileView (void);

// cycle the flashing light for when mine destroyed
void FlashFrame (void);

int32_t FindSegSideFace (int16_t x,int16_t y,int32_t *seg,int32_t *CSide,int32_t *face,int32_t *poly);

// these functions change different rendering parameters
// all return the new value of the parameter

// how may levels deep to render
int32_t inc_render_depth(void);
int32_t dec_render_depth(void);
int32_t reset_render_depth(void);

// how many levels deep to render in perspective
int32_t inc_perspective_depth(void);
int32_t dec_perspective_depth(void);
int32_t reset_perspective_depth(void);

// misc toggles
int32_t ToggleOutlineMode(void);
int32_t ToggleShowOnlyCurSide(void);

// When any render function needs to know what's looking at it, it
// should access RenderViewerObject members.
extern fix xRenderZoom;     // the player's zoom factor

// This is used internally to RenderFrame(), but is included here so AI
// can use it for its own purposes.

extern int16_t nRenderList [MAX_SEGMENTS_D2X];

// Set the following to turn on player head turning
// If the above flag is set, these angles specify the orientation of the head
extern CAngleVector Player_head_angles;

//
// Routines for conditionally rotating & projecting points
//

void RenderStartFrame (void);
void SetupRenderView (fix xStereoSeparation, int16_t *nStartSegP, int32_t bOglScale);

void RenderMine (int16_t nStartSeg, fix xStereoSeparation, int32_t nWindow);
void RenderMeshOutline (int32_t nScale = -1, float fScale = 1.0f);
void RenderShadowQuad (void);
void UpdateRenderedData (int32_t window_num, CObject *viewer, int32_t rearViewFlag, int32_t user);
void RenderObjList (int32_t nListPos, int32_t nWindow);
void RenderMineSegment (int32_t nn);
void RenderEffects (int32_t nWindow);
void RenderSkyBox (int32_t nWindow);

void BuildRenderObjLists (int32_t nSegCount, int32_t nThread = 0);
void BuildRenderSegList (int16_t nStartSeg, int32_t nWindow, bool bIgnoreDoors = false, int32_t nThread = 0);

void ResetFaceList (void);
int32_t AddFaceListItem (CSegFace *pFace, int32_t nThread);

//------------------------------------------------------------------------------

static inline bool GuidedMslView (CObject ** objPP)
{
return (*objPP = gameData.objData.GetGuidedMissile (N_LOCALPLAYER)) != NULL;
}

//------------------------------------------------------------------------------

static inline CObject *GuidedInMainView (void)
{
if (!gameOpts->render.cockpit.bGuidedInMainView)
	return NULL;

CObject *pObj;

return GuidedMslView (&pObj) ? pObj : NULL;
}

//------------------------------------------------------------------------------

#endif /* _RENDER_H */

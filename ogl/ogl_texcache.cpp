/*
 *
 * Graphics support functions for OpenGL.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef _WIN32
#	include <windows.h>
#	include <stddef.h>
#	include <io.h>
#endif
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef __macosx__
# include <stdlib.h>
# include <SDL/SDL.h>
#else
# include <malloc.h>
# include <SDL.h>
#endif

#include "inferno.h"
#include "error.h"
#include "error.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_texture.h"
#include "ogl_texcache.h"
#include "texmerge.h"
#include "newmenu.h"
#include "menu.h"

//------------------------------------------------------------------------------

void OglCachePolyModelTextures (int nModel)
{
	tPolyModel *po;
if ((po = GetPolyModel (NULL, NULL, nModel, 0)))
	LoadModelTextures (po, NULL);
}

//------------------------------------------------------------------------------

void OglCacheVClipTextures (tVideoClip *vc, int nTransp)
{
	int i;

for (i = 0; i < vc->nFrameCount; i++) {
	PIGGY_PAGE_IN (vc->frames [i].index, 0);
	gameData.pig.tex.bitmaps [0] [vc->frames [i].index].SetupTexture (1, nTransp, 1);
	}
}

//------------------------------------------------------------------------------

#define OglCacheVClipTexturesN(i,t) OglCacheVClipTextures (gameData.eff.vClips [0] + i, t)

void OglCacheWeaponTextures (tWeaponInfo *w)
{
OglCacheVClipTexturesN (w->nFlashVClip, 1);
OglCacheVClipTexturesN (w->robot_hit_vclip, 1);
OglCacheVClipTexturesN (w->wall_hit_vclip, 1);
if (w->renderType == WEAPON_RENDER_VCLIP)
	OglCacheVClipTexturesN (w->nVClipIndex, 3);
else if (w->renderType == WEAPON_RENDER_POLYMODEL)
	OglCachePolyModelTextures (w->nModel);
}

//------------------------------------------------------------------------------

CBitmap *OglLoadFaceBitmap (short nTexture, short nFrameIdx)
{
	CBitmap	*bmP;
	int			nFrames;

PIGGY_PAGE_IN (gameData.pig.tex.bmIndexP [nTexture].index, gameStates.app.bD1Mission);
bmP = gameData.pig.tex.bitmapP + gameData.pig.tex.bmIndexP [nTexture].index;
if (bmP->Override ()) {
	bmP = bmP->Override ();
	if (bmP->WallAnim ()) {
		nFrames = bmP->FrameCount ();
		if (nFrames > 1) {
			bmP->SetupTexture (1, 3, 1);
			if (bmP->Frames ()) {
				if ((nFrameIdx >= 0) || (-nFrames > nFrameIdx))
					bmP->SetCurFrame (bmP->Frames ());
				else
					bmP->SetCurFrame (bmP->Frames () - nFrameIdx - 1);
				bmP->CurFrame ()->SetupTexture (1, 3, 1);
				}
			}
		}
	}
return bmP;
}

//------------------------------------------------------------------------------

void CacheSideTextures (int nSeg)
{
	short				nSide, tMap1, tMap2;
	CBitmap			*bmP, *bm2, *bmm;
	struct tSide	*sideP;

for (nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
	sideP = gameData.segs.segments [nSeg].sides + nSide;
	tMap1 = sideP->nBaseTex;
	if ((tMap1 < 0) || (tMap1 >= gameData.pig.tex.nTextures [gameStates.app.bD1Data]))
		continue;
	bmP = OglLoadFaceBitmap (tMap1, sideP->nFrame);
	if ((tMap2 = sideP->nOvlTex)) {
		bm2 = OglLoadFaceBitmap (tMap1, sideP->nFrame);
		if (!(bm2->Flags () & BM_FLAG_SUPER_TRANSPARENT) ||
			 (gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk))
			bm2->SetupTexture (1, 3, 1);
		else if ((bmm = TexMergeGetCachedBitmap (tMap1, tMap2, sideP->nOvlOrient)))
			bmP = bmm;
		else
			bm2->SetupTexture (1, 3, 1);
		}
	bmP->SetupTexture (1, 3, 1);
	}
}

//------------------------------------------------------------------------------

static int nCacheSeg = 0;
static int nCacheObj = -3;

static int TexCachePoll (int nItems, tMenuItem *m, int *key, int nCurItem)
{
if (nCacheSeg < gameData.segs.nSegments)
	CacheSideTextures (nCacheSeg++);
else if (nCacheObj <= gameData.objs.nLastObject [0]) 
	CacheSideTextures (nCacheObj++);
else {
	*key = -2;
	return nCurItem;
	}
m [0].value++;
m [0].rebuild = 1;
*key = 0;
return nCurItem;
}

//------------------------------------------------------------------------------

int OglCacheTextures (void)
{
	tMenuItem	m [3];
	int i;

memset (m, 0, sizeof (m));
ADD_GAUGE (0, "                    ", 0, gameData.segs.nSegments + gameData.objs.nLastObject [0] + 4); 
m [2].centered = 1;
nCacheSeg = 0;
nCacheObj = -3;
do {
	i = ExecMenu2 (NULL, "Loading textures", 1, m, TexCachePoll, 0, NULL);
	} while (i >= 0);
return 1;
}

//------------------------------------------------------------------------------

void CacheAddonTextures (void)
{
	int	i;

for (i = 0; i < MAX_ADDON_BITMAP_FILES; i++) {
	PageInAddonBitmap (-i - 1);
	BM_ADDON (i)->SetupTexture (1, 0, gameOpts->render.bDepthSort <= 0);
	}
}

//------------------------------------------------------------------------------

int OglCacheLevelTextures (void)
{
	int			i, j, bD1;
	tEffectClip	*ec;
	int			max_efx = 0, ef;
	int			nSegment, nSide;
	short			nBaseTex, nOvlTex;
	CBitmap	*bmBot,*bmTop, *bmm;
	tSegment		*segP;
	tSide			*sideP;
	CObject		*objP;

if (gameStates.render.bBriefing)
	return 0;
PrintLog ("caching level textures\n");
TexMergeClose ();
TexMergeInit (-1);
PrintLog ("   caching effect textures\n");
for (bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++) {
	for (i = 0,ec = gameData.eff.effects [bD1]; i < gameData.eff.nEffects [bD1];i++,ec++) {
		if ((ec->changingWallTexture == -1) && (ec->changingObjectTexture == -1))
			continue;
		if (ec->vc.nFrameCount > max_efx)
			max_efx = ec->vc.nFrameCount;
		}
	for (ef = 0; ef < max_efx; ef++)
		for (i = 0,ec = gameData.eff.effects [bD1]; i < gameData.eff.nEffects [bD1]; i++, ec++) {
			if ((ec->changingWallTexture == -1) && (ec->changingObjectTexture == -1))
				continue;
			ec->time_left = -1;
			}
	}

PrintLog ("   caching geometry textures\n");
for (segP = SEGMENTS, nSegment = 0; nSegment < gameData.segs.nSegments; nSegment++, segP++) {
	for (nSide = 0, sideP = segP->sides; nSide < MAX_SIDES_PER_SEGMENT; nSide++, sideP++) {
		nBaseTex = sideP->nBaseTex;
		if ((nBaseTex < 0) || (nBaseTex >= gameData.pig.tex.nTextures [gameStates.app.bD1Data]))
			continue;
		bmBot = OglLoadFaceBitmap (nBaseTex, sideP->nFrame);
		if ((nOvlTex = sideP->nOvlTex)) {
			bmTop = OglLoadFaceBitmap (nOvlTex, sideP->nFrame);
			if (!(bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT) ||
				 (gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk))
				bmTop->SetupTexture (1, 3, 1);
			else if ((bmm = TexMergeGetCachedBitmap (nBaseTex, nOvlTex, sideP->nOvlOrient)))
				bmBot = bmm;
			else
				bmTop->SetupTexture (1, 3, 1);
			}
		bmBot->SetupTexture (1, 3, 1);
		}
	}
ResetSpecialEffects ();
InitSpecialEffects ();
DoSpecialEffects ();
PrintLog ("   caching object textures\n");
CacheObjectEffects ();
PrintLog ("   caching addon textures\n");
CacheAddonTextures ();
// cache all weapon and powerup textures
PrintLog ("   caching powerup sprites\n");
for (i = 0; i < EXTRA_OBJ_IDS; i++)
	OglCacheWeaponTextures (gameData.weapons.info + i);
for (i = 0; i < MAX_POWERUP_TYPES; i++)
	if (i != 9)
		OglCacheVClipTexturesN (gameData.objs.pwrUp.info [i].nClipIndex, 3);
FORALL_OBJS (objP, i)
	if (objP->info.renderType == RT_POLYOBJ)
		OglCachePolyModelTextures (objP->rType.polyObjInfo.nModel);
// cache the hostage clip frame textures
PrintLog ("   caching hostage sprites\n");
OglCacheVClipTexturesN (33, 2);    
// cache all clip frame textures incl. explosions and effects
PrintLog ("   caching explision sprites\n");
for (i = 0; i < 2; i++)
	for (j = 0; j < MAX_GAUGE_BMS; j++)
		if (gameData.cockpit.gauges [i][j].index != 0xffff)
			PIGGY_PAGE_IN (gameData.cockpit.gauges [i][j].index, 0);
for (i = 0; i < gameData.eff.nClips [0]; i++)
	OglCacheVClipTexturesN (i, 1);
return 0;
}

//------------------------------------------------------------------------------


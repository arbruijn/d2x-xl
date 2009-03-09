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

#include "descent.h"
#include "error.h"
#include "error.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_texture.h"
#include "ogl_texcache.h"
#include "texmerge.h"
#include "menu.h"
#include "menu.h"

static int bLoadTextures = 1;

//------------------------------------------------------------------------------

void OglCachePolyModelTextures (int nModel)
{
	CPolyModel* modelP = GetPolyModel (NULL, NULL, nModel, 0);

if (modelP)
	modelP->LoadTextures (NULL);
}

//------------------------------------------------------------------------------

static void OglCacheVClipTextures (tVideoClip* vc, int nTransp)
{
for (int i = 0; i < vc->nFrameCount; i++) {
	LoadBitmap (vc->frames [i].index, 0);
	gameData.pig.tex.bitmaps [0][vc->frames [i].index].SetupTexture (1, nTransp, bLoadTextures);
	}
}

//------------------------------------------------------------------------------

static void OglCacheVClipTextures (int i, int nTransp)
{
if (i >= 0) 
	OglCacheVClipTextures (&gameData.eff.vClips [0][i], nTransp);
}

//------------------------------------------------------------------------------

static void OglCacheWeaponTextures (CWeaponInfo* wi)
{
OglCacheVClipTextures (wi->nFlashVClip, 1);
OglCacheVClipTextures (wi->nRobotHitVClip, 1);
OglCacheVClipTextures (wi->nWallHitVClip, 1);
if (wi->renderType == WEAPON_RENDER_VCLIP)
	OglCacheVClipTextures (wi->nVClipIndex, 3);
else if (wi->renderType == WEAPON_RENDER_POLYMODEL)
	OglCachePolyModelTextures (wi->nModel);
}

//------------------------------------------------------------------------------

static CBitmap *OglLoadFaceBitmap (short nTexture, short nFrameIdx)
{
	CBitmap*	bmP;
	int		nFrames;

LoadBitmap (gameData.pig.tex.bmIndexP [nTexture].index, gameStates.app.bD1Mission);
bmP = gameData.pig.tex.bitmapP + gameData.pig.tex.bmIndexP [nTexture].index;
if (bmP->Override ()) {
	bmP = bmP->Override ();
	if (bmP->WallAnim ()) {
		nFrames = bmP->FrameCount ();
		if (nFrames > 1) {
			bmP->SetupTexture (1, 3, bLoadTextures);
			if (bmP->Frames ()) {
				if ((nFrameIdx >= 0) || (-nFrames > nFrameIdx))
					bmP->SetCurFrame (bmP->Frames ());
				else
					bmP->SetCurFrame (bmP->Frames () - nFrameIdx - 1);
				bmP->CurFrame ()->SetupTexture (1, 3, bLoadTextures);
				}
			}
		}
	}
return bmP;
}

//------------------------------------------------------------------------------

static void CacheSideTextures (int nSeg)
{
	short			nSide, tMap1, tMap2;
	CBitmap*		bmP, * bm2, * bmm;
	CSide*		sideP;

for (nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
	sideP = SEGMENTS [nSeg].m_sides + nSide;
	tMap1 = sideP->m_nBaseTex;
	if ((tMap1 < 0) || (tMap1 >= gameData.pig.tex.nTextures [gameStates.app.bD1Data]))
		continue;
	bmP = OglLoadFaceBitmap (tMap1, sideP->m_nFrame);
	if ((tMap2 = sideP->m_nOvlTex)) {
		bm2 = OglLoadFaceBitmap (tMap1, sideP->m_nFrame);
		if (!(bm2->Flags () & BM_FLAG_SUPER_TRANSPARENT) || gameOpts->ogl.bGlTexMerge)
			bm2->SetupTexture (1, 3, bLoadTextures);
		else if ((bmm = TexMergeGetCachedBitmap (tMap1, tMap2, sideP->m_nOvlOrient)))
			bmP = bmm;
		else
			bm2->SetupTexture (1, 3, bLoadTextures);
		}
	bmP->SetupTexture (1, 3, bLoadTextures);
	}
}

//------------------------------------------------------------------------------

static int nCacheSeg = 0;
static int nCacheObj = -3;

static int TexCachePoll (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

if (nCacheSeg < gameData.segs.nSegments)
	CacheSideTextures (nCacheSeg++);
else if (nCacheObj <= gameData.objs.nLastObject [0]) 
	CacheSideTextures (nCacheObj++);
else {
	key = -2;
	return nCurItem;
	}
menu [0].m_value++;
menu [0].m_bRebuild = 1;
key = 0;
return nCurItem;
}

//------------------------------------------------------------------------------

int OglCacheTextures (void)
{
	CMenu	m (3);
	int	i;

m.AddGauge ("                    ", -1, gameData.segs.nSegments + gameData.objs.nLastObject [0] + 4); 
nCacheSeg = 0;
nCacheObj = -3;
do {
	i = m.Menu (NULL, "Loading textures", TexCachePoll);
	} while (i >= 0);
return 1;
}

//------------------------------------------------------------------------------

static void CacheAddonTextures (void)
{
	int	i;

for (i = 0; i < MAX_ADDON_BITMAP_FILES; i++) {
	PageInAddonBitmap (-i - 1);
	BM_ADDON (i)->SetupTexture (1, 0, bLoadTextures); //gameOpts->render.bDepthSort <= 0);
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
	CBitmap*		bmBot,* bmTop, * bmm;
	CSegment		*segP;
	CSide			*sideP;
	CObject		*objP;

if (gameStates.render.bBriefing)
	return 0;
PrintLog ("caching level textures\n");
TexMergeClose ();
TexMergeInit (-1);
PrintLog ("   caching effect textures\n");
for (bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++) {
	for (i = 0, ec = gameData.eff.effects [bD1].Buffer (); i < gameData.eff.nEffects [bD1]; i++, ec++) {
		if ((ec->changingWallTexture == -1) && (ec->changingObjectTexture == -1))
			continue;
		if (ec->vc.nFrameCount > max_efx)
			max_efx = ec->vc.nFrameCount;
		}
	for (ef = 0; ef < max_efx; ef++)
		for (i = 0, ec = gameData.eff.effects [bD1].Buffer (); i < gameData.eff.nEffects [bD1]; i++, ec++) {
			if ((ec->changingWallTexture == -1) && (ec->changingObjectTexture == -1))
				continue;
			ec->time_left = -1;
			}
	}

PrintLog ("   caching geometry textures\n");
#if DBG
int bNeedMipMaps = gameStates.ogl.bNeedMipMaps;
//gameStates.ogl.bNeedMipMaps = -1;	// disable loading textures to the OpenGL driver
#endif
for (segP = SEGMENTS.Buffer (), nSegment = 0; nSegment < gameData.segs.nSegments; nSegment++, segP++) {
	for (nSide = 0, sideP = segP->m_sides; nSide < MAX_SIDES_PER_SEGMENT; nSide++, sideP++) {
		nBaseTex = sideP->m_nBaseTex;
		if ((nBaseTex < 0) || (nBaseTex >= gameData.pig.tex.nTextures [gameStates.app.bD1Data]))
			continue;
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
			nDbgSeg = nDbgSeg;
#endif
		bmBot = OglLoadFaceBitmap (nBaseTex, sideP->m_nFrame);
		if ((nOvlTex = sideP->m_nOvlTex)) {
			bmTop = OglLoadFaceBitmap (nOvlTex, sideP->m_nFrame);
			if (!(bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT) || gameOpts->ogl.bGlTexMerge)
				bmTop->SetupTexture (1, 3, bLoadTextures);
			else if ((bmm = TexMergeGetCachedBitmap (nBaseTex, nOvlTex, sideP->m_nOvlOrient)))
				bmBot = bmm;
			else
				bmTop->SetupTexture (1, 3, bLoadTextures);
			}
		bmBot->SetupTexture (1, 3, bLoadTextures);
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
		OglCacheVClipTextures (gameData.objs.pwrUp.info [i].nClipIndex, 3);
FORALL_OBJS (objP, i)
	if (objP->info.renderType == RT_POLYOBJ)
		OglCachePolyModelTextures (objP->rType.polyObjInfo.nModel);
// cache the hostage clip frame textures
PrintLog ("   caching hostage sprites\n");
OglCacheVClipTextures (33, 2);    
// cache all clip frame textures incl. explosions and effects
PrintLog ("   caching explosion sprites\n");
for (i = 0; i < 2; i++)
	for (j = 0; j < MAX_GAUGE_BMS; j++)
		if (gameData.cockpit.gauges [i][j].index != 0xffff)
			LoadBitmap (gameData.cockpit.gauges [i][j].index, 0);
for (i = 0; i < gameData.eff.nClips [0]; i++)
	OglCacheVClipTextures (i, 1);
#if DBG
gameStates.ogl.bNeedMipMaps = bNeedMipMaps;
#endif
return 0;
}

//------------------------------------------------------------------------------


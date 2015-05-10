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
# include <SDL.h>
#endif

#include "descent.h"
#include "error.h"
#include "error.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_texture.h"
#include "ogl_texcache.h"
#include "addon_bitmaps.h"
#include "texmerge.h"
#include "renderlib.h"
#include "menu.h"

static int32_t bLoadTextures = 1;

static CStaticArray< bool, MAX_ANIMATIONS_D2 >	bVClipLoaded;

//------------------------------------------------------------------------------

void OglCachePolyModelTextures (int32_t nModel)
{
	CPolyModel* pModel = GetPolyModel (NULL, NULL, nModel, 0);

if (pModel)
	pModel->LoadTextures (NULL);
}

//------------------------------------------------------------------------------

static CBitmap* OglCacheTexture (int32_t nIndex, int32_t nTranspType)
{
LoadTexture (nIndex, 0, 0);
CBitmap* pBm = &gameData.pig.tex.bitmaps [0][nIndex];
pBm->SetTranspType (nTranspType);
pBm->SetupTexture (1, bLoadTextures);
return pBm;
}

//------------------------------------------------------------------------------

static void OglCacheAnimationTextures (tAnimationInfo* pAnimInfo, int32_t nTranspType)
{
for (int32_t i = 0; i < pAnimInfo->nFrameCount; i++) {
#if DBG
	int32_t h = pAnimInfo->frames [i].index;
	if ((nDbgTexture >= 0) && (h == nDbgTexture))
		BRP;
#endif
	CBitmap* pBm = OglCacheTexture (pAnimInfo->frames [i].index, nTranspType);
	if (!i && pBm->Override ())
		SetupHiresVClip (pAnimInfo, NULL, pBm->Override ());
	}
}

//------------------------------------------------------------------------------

static void OglCacheAnimationTextures (int32_t i, int32_t nTransp)
{
if ((i >= 0) && !bVClipLoaded [i]) {
	bVClipLoaded [i] = true;
	OglCacheAnimationTextures (&gameData.effects.animations [0][i], nTransp);
	}
}

//------------------------------------------------------------------------------

static void OglCacheWeaponTextures (CWeaponInfo* wi)
{
OglCacheAnimationTextures (wi->nFlashAnimation, 1);
OglCacheAnimationTextures (wi->nRobotHitAnimation, 1);
OglCacheAnimationTextures (wi->nWallHitAnimation, 1);
if (wi->renderType == WEAPON_RENDER_BLOB)
	OglCacheTexture (wi->bitmap.index, 1);
else if (wi->renderType == WEAPON_RENDER_VCLIP)
	OglCacheAnimationTextures (wi->nAnimationIndex, 3);
else if (wi->renderType == WEAPON_RENDER_POLYMODEL)
	OglCachePolyModelTextures (wi->nModel);
}

//------------------------------------------------------------------------------

#if 0

static CBitmap *OglLoadFaceBitmap (int16_t nTexture, int16_t nFrameIdx, int32_t bLoadTextures)
{
	CBitmap*	pBm, * pBmo, * pBmf;
	int32_t		nFrames;

LoadTexture (gameData.pig.tex.pBmIndex [nTexture].index, 0, gameStates.app.bD1Mission);
pBm = gameData.pig.tex.pBitmap + gameData.pig.tex.pBmIndex [nTexture].index;
pBm->SetStatic (1);
if (!(pBmo = pBm->Override ()))
	return pBm;
pBmo->SetStatic (1);
if (!pBmo->WallAnim ())
	return pBmo;
if (2 > (nFrames = pBmo->FrameCount ()))
	return pBmo;
pBmo->SetTranspType (3);
pBmo->SetupTexture (1, bLoadTextures);
if (!(pBmf = pBmo->Frames ()))
	return pBmo;
if ((nFrameIdx < 0) && (nFrames >= -nFrameIdx))
	pBmf -= (nFrameIdx + 1);
pBmo->SetCurFrame (pBmf);
pBmf->SetTranspType (3);
pBmf->SetupTexture (1, bLoadTextures);
pBmf->SetStatic (1);
return pBm;
}

#endif

//------------------------------------------------------------------------------

static void CacheSideTextures (int32_t nSegment)
{
	int16_t			nSide, tMap1, tMap2;
	CBitmap*		pBm, * bm2, * bmm;
	CSide*		pSide;
	CSegment*	pSeg = SEGMENT (nSegment);

for (nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++) {
	pSide = pSeg->m_sides + nSide;
	if (!pSide->FaceCount ())
		continue;
	tMap1 = pSide->m_nBaseTex;
	if ((tMap1 < 0) || (tMap1 >= gameData.pig.tex.nTextures [gameStates.app.bD1Data]))
		continue;
	pBm = LoadFaceBitmap (tMap1, pSide->m_nFrame, bLoadTextures);
	if ((tMap2 = pSide->m_nOvlTex)) {
		bm2 = LoadFaceBitmap (tMap1, pSide->m_nFrame, bLoadTextures);
		bm2->SetTranspType (3);
		if (/*!(bm2->Flags () & BM_FLAG_SUPER_TRANSPARENT) ||*/ gameOpts->ogl.bGlTexMerge)
			bm2->SetupTexture (1, bLoadTextures);
		else if ((bmm = TexMergeGetCachedBitmap (tMap1, tMap2, pSide->m_nOvlOrient)))
			pBm = bmm;
		else
			bm2->SetupTexture (1, bLoadTextures);
		}
	pBm->SetTranspType (3);
	pBm->SetupTexture (1, bLoadTextures);
	}
}

//------------------------------------------------------------------------------

static int32_t nCacheSeg = 0;
static int32_t nCacheObj = -3;

static int32_t TexCachePoll (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

if (nCacheSeg < gameData.segData.nSegments)
	CacheSideTextures (nCacheSeg++);
else if (nCacheObj <= gameData.objData.nLastObject [0]) 
	CacheSideTextures (nCacheObj++);
else {
	key = -2;
	return nCurItem;
	}
menu [0].Value ()++;
menu [0].Rebuild ();
key = 0;
return nCurItem;
}

//------------------------------------------------------------------------------

int32_t OglCacheTextures (void)
{
	CMenu	m (3);
	int32_t	i;

m.AddGauge ("progress bar", "                    ", -1, gameData.segData.nSegments + gameData.objData.nLastObject [0] + 4); 
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
for (int32_t i = 0; i < MAX_ADDON_BITMAP_FILES; i++) {
	PageInAddonBitmap (-i - 1);
	BM_ADDON (i)->SetTranspType (0);
	BM_ADDON (i)->SetupTexture (1, 1); 
	}
CAddonBitmap::Prepare ();
}

//------------------------------------------------------------------------------

int32_t OglCacheLevelTextures (void)
{
	int32_t			i, j, bD1;
	tEffectInfo*	pEffectInfo;
	int32_t			max_efx = 0, ef;
	int32_t			nSegment, nSide;
	int16_t			nBaseTex, nOvlTex;
	CBitmap*			bmBot,* bmTop, * bmm;
	CSegment*		pSeg;
	CSide*			pSide;
	CObject*			pObj;
	CStaticArray< bool, MAX_POLYGON_MODELS >	bModelLoaded;

if (gameStates.render.bBriefing)
	return 0;
PrintLog (1, "caching level textures\n");
TexMergeClose ();
TexMergeInit (-1);

PrintLog (1, "caching effect textures\n");
for (bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++) {
	for (i = 0, pEffectInfo = gameData.effects.effects [bD1].Buffer (); i < gameData.effects.nEffects [bD1]; i++, pEffectInfo++) {
		if ((pEffectInfo->changing.nWallTexture == -1) && (pEffectInfo->changing.nObjectTexture == -1))
			continue;
		if (pEffectInfo->animationInfo.nFrameCount > max_efx)
			max_efx = pEffectInfo->animationInfo.nFrameCount;
		}
	for (ef = 0; ef < max_efx; ef++)
		for (i = 0, pEffectInfo = gameData.effects.effects [bD1].Buffer (); i < gameData.effects.nEffects [bD1]; i++, pEffectInfo++) {
			if ((pEffectInfo->changing.nWallTexture == -1) && (pEffectInfo->changing.nObjectTexture == -1))
				continue;
			pEffectInfo->xTimeLeft = -1;
			}
	}
PrintLog (-1);

gameStates.render.EnableCartoonStyle (1, 1, 1);

PrintLog (1, "caching geometry textures\n");
// bLoadTextures = (ogl.m_states.nPreloadTextures > 0);
for (pSeg = SEGMENTS.Buffer (), nSegment = 0; nSegment < gameData.segData.nSegments; nSegment++, pSeg++) {
	for (nSide = 0, pSide = pSeg->m_sides; nSide < SEGMENT_SIDE_COUNT; nSide++, pSide++) {
		if (!pSide->FaceCount ())
			continue;
		nBaseTex = pSide->m_nBaseTex;
		if ((nBaseTex < 0) || (nBaseTex >= gameData.pig.tex.nTextures [gameStates.app.bD1Data]))
			continue;
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
			BRP;
#endif
		bmBot = LoadFaceBitmap (nBaseTex, pSide->m_nFrame, bLoadTextures);
		if ((nOvlTex = pSide->m_nOvlTex)) {
			bmTop = LoadFaceBitmap (nOvlTex, pSide->m_nFrame, bLoadTextures);
			bmTop->SetTranspType (3);
			if (gameOpts->ogl.bGlTexMerge) // || !(bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT))
				bmTop->SetupTexture (1, bLoadTextures);
			else if ((bmm = TexMergeGetCachedBitmap (nBaseTex, nOvlTex, pSide->m_nOvlOrient)))
				bmBot = bmm;
			else
				bmTop->SetupTexture (1, bLoadTextures);
			}
		bmBot->SetTranspType (3);
		bmBot->SetupTexture (1, bLoadTextures);
		}
	}
PrintLog (-1);

PrintLog (1, "caching addon textures\n");
//gameStates.render.DisableCartoonStyle ();
CacheAddonTextures ();
//gameStates.render.EnableCartoonStyle ();
PrintLog (-1);

PrintLog (1, "caching model textures\n");
// bLoadTextures = (ogl.m_states.nPreloadTextures > 1);
bModelLoaded.Clear ();
bVClipLoaded.Clear ();
FORALL_OBJS (pObj) {
	if (pObj->info.renderType != RT_POLYOBJ)
		continue;
	if (bModelLoaded [pObj->ModelId ()])
		continue;
	bModelLoaded [pObj->ModelId ()] = true;
	OglCachePolyModelTextures (pObj->ModelId ());
	}
PrintLog (-1);

gameStates.render.DisableCartoonStyle ();
gameStates.render.EnableCartoonStyle (0, 0, 1);

PrintLog (1, "caching hostage sprites\n");
// bLoadTextures = (ogl.m_states.nPreloadTextures > 3);
OglCacheAnimationTextures (33, 3);    
PrintLog (-1);

PrintLog (1, "caching weapon sprites\n");
// bLoadTextures = (ogl.m_states.nPreloadTextures > 5);
for (i = 0; i < EXTRA_OBJ_IDS; i++)
	OglCacheWeaponTextures (gameData.weapons.info + i);
PrintLog (-1);

PrintLog (1, "caching powerup sprites\n");
// bLoadTextures = (ogl.m_states.nPreloadTextures > 4);
for (i = 0; i < MAX_POWERUP_TYPES; i++)
	if (i != 9)
		OglCacheAnimationTextures (gameData.objData.pwrUp.info [i].nClipIndex, 3);
PrintLog (-1);

PrintLog (1, "caching effect textures\n");
CacheObjectEffects ();
// bLoadTextures = (ogl.m_states.nPreloadTextures > 2);
for (i = 0; i < gameData.effects.nClips [0]; i++)
	OglCacheAnimationTextures (i, 1);
PrintLog (-1);

gameStates.render.DisableCartoonStyle ();

PrintLog (1, "caching cockpit textures\n");
for (i = 0; i < 2; i++)
	for (j = 0; j < MAX_GAUGE_BMS; j++)
		if (gameData.cockpit.gauges [i][j].index != 0xffff)
			LoadTexture (gameData.cockpit.gauges [i][j].index, 0, 0);
PrintLog (-1);

ResetSpecialEffects ();
InitSpecialEffects ();
DoSpecialEffects (true);
PrintLog (-1);
return 0;
}

//------------------------------------------------------------------------------


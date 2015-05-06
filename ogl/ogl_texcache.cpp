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
	CPolyModel* modelP = GetPolyModel (NULL, NULL, nModel, 0);

if (modelP)
	modelP->LoadTextures (NULL);
}

//------------------------------------------------------------------------------

static CBitmap* OglCacheTexture (int32_t nIndex, int32_t nTranspType)
{
LoadTexture (nIndex, 0, 0);
CBitmap* bmP = &gameData.pig.tex.bitmaps [0][nIndex];
bmP->SetTranspType (nTranspType);
bmP->SetupTexture (1, bLoadTextures);
return bmP;
}

//------------------------------------------------------------------------------

static void OglCacheAnimationTextures (tAnimationInfo* animInfoP, int32_t nTranspType)
{
for (int32_t i = 0; i < animInfoP->nFrameCount; i++) {
#if DBG
	int32_t h = animInfoP->frames [i].index;
	if ((nDbgTexture >= 0) && (h == nDbgTexture))
		BRP;
#endif
	CBitmap* bmP = OglCacheTexture (animInfoP->frames [i].index, nTranspType);
	if (!i && bmP->Override ())
		SetupHiresVClip (animInfoP, NULL, bmP->Override ());
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
	CBitmap*	bmP, * bmoP, * bmfP;
	int32_t		nFrames;

LoadTexture (gameData.pig.tex.bmIndexP [nTexture].index, 0, gameStates.app.bD1Mission);
bmP = gameData.pig.tex.bitmapP + gameData.pig.tex.bmIndexP [nTexture].index;
bmP->SetStatic (1);
if (!(bmoP = bmP->Override ()))
	return bmP;
bmoP->SetStatic (1);
if (!bmoP->WallAnim ())
	return bmoP;
if (2 > (nFrames = bmoP->FrameCount ()))
	return bmoP;
bmoP->SetTranspType (3);
bmoP->SetupTexture (1, bLoadTextures);
if (!(bmfP = bmoP->Frames ()))
	return bmoP;
if ((nFrameIdx < 0) && (nFrames >= -nFrameIdx))
	bmfP -= (nFrameIdx + 1);
bmoP->SetCurFrame (bmfP);
bmfP->SetTranspType (3);
bmfP->SetupTexture (1, bLoadTextures);
bmfP->SetStatic (1);
return bmP;
}

#endif

//------------------------------------------------------------------------------

static void CacheSideTextures (int32_t nSegment)
{
	int16_t			nSide, tMap1, tMap2;
	CBitmap*		bmP, * bm2, * bmm;
	CSide*		sideP;
	CSegment*	segP = SEGMENT (nSegment);

for (nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++) {
	sideP = segP->m_sides + nSide;
	if (!sideP->FaceCount ())
		continue;
	tMap1 = sideP->m_nBaseTex;
	if ((tMap1 < 0) || (tMap1 >= gameData.pig.tex.nTextures [gameStates.app.bD1Data]))
		continue;
	bmP = LoadFaceBitmap (tMap1, sideP->m_nFrame, bLoadTextures);
	if ((tMap2 = sideP->m_nOvlTex)) {
		bm2 = LoadFaceBitmap (tMap1, sideP->m_nFrame, bLoadTextures);
		bm2->SetTranspType (3);
		if (/*!(bm2->Flags () & BM_FLAG_SUPER_TRANSPARENT) ||*/ gameOpts->ogl.bGlTexMerge)
			bm2->SetupTexture (1, bLoadTextures);
		else if ((bmm = TexMergeGetCachedBitmap (tMap1, tMap2, sideP->m_nOvlOrient)))
			bmP = bmm;
		else
			bm2->SetupTexture (1, bLoadTextures);
		}
	bmP->SetTranspType (3);
	bmP->SetupTexture (1, bLoadTextures);
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
	tEffectInfo*	effectInfoP;
	int32_t			max_efx = 0, ef;
	int32_t			nSegment, nSide;
	int16_t			nBaseTex, nOvlTex;
	CBitmap*			bmBot,* bmTop, * bmm;
	CSegment*		segP;
	CSide*			sideP;
	CObject*			objP;
	CStaticArray< bool, MAX_POLYGON_MODELS >	bModelLoaded;

if (gameStates.render.bBriefing)
	return 0;
PrintLog (1, "caching level textures\n");
TexMergeClose ();
TexMergeInit (-1);

gameStates.render.EnableCartoonStyle ();

PrintLog (1, "caching effect textures\n");
for (bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++) {
	for (i = 0, effectInfoP = gameData.effects.effects [bD1].Buffer (); i < gameData.effects.nEffects [bD1]; i++, effectInfoP++) {
		if ((effectInfoP->changing.nWallTexture == -1) && (effectInfoP->changing.nObjectTexture == -1))
			continue;
		if (effectInfoP->animationInfo.nFrameCount > max_efx)
			max_efx = effectInfoP->animationInfo.nFrameCount;
		}
	for (ef = 0; ef < max_efx; ef++)
		for (i = 0, effectInfoP = gameData.effects.effects [bD1].Buffer (); i < gameData.effects.nEffects [bD1]; i++, effectInfoP++) {
			if ((effectInfoP->changing.nWallTexture == -1) && (effectInfoP->changing.nObjectTexture == -1))
				continue;
			effectInfoP->xTimeLeft = -1;
			}
	}
PrintLog (-1);
PrintLog (1, "caching geometry textures\n");
// bLoadTextures = (ogl.m_states.nPreloadTextures > 0);
for (segP = SEGMENTS.Buffer (), nSegment = 0; nSegment < gameData.segData.nSegments; nSegment++, segP++) {
	for (nSide = 0, sideP = segP->m_sides; nSide < SEGMENT_SIDE_COUNT; nSide++, sideP++) {
		if (!sideP->FaceCount ())
			continue;
		nBaseTex = sideP->m_nBaseTex;
		if ((nBaseTex < 0) || (nBaseTex >= gameData.pig.tex.nTextures [gameStates.app.bD1Data]))
			continue;
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
			BRP;
#endif
		bmBot = LoadFaceBitmap (nBaseTex, sideP->m_nFrame, bLoadTextures);
		if ((nOvlTex = sideP->m_nOvlTex)) {
			bmTop = LoadFaceBitmap (nOvlTex, sideP->m_nFrame, bLoadTextures);
			bmTop->SetTranspType (3);
			if (gameOpts->ogl.bGlTexMerge) // || !(bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT))
				bmTop->SetupTexture (1, bLoadTextures);
			else if ((bmm = TexMergeGetCachedBitmap (nBaseTex, nOvlTex, sideP->m_nOvlOrient)))
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
gameStates.render.DisableCartoonStyle ();
CacheAddonTextures ();
gameStates.render.EnableCartoonStyle ();
PrintLog (-1);

PrintLog (1, "caching model textures\n");
// bLoadTextures = (ogl.m_states.nPreloadTextures > 1);
bModelLoaded.Clear ();
bVClipLoaded.Clear ();
FORALL_OBJS (objP) {
	if (objP->info.renderType != RT_POLYOBJ)
		continue;
	if (bModelLoaded [objP->ModelId ()])
		continue;
	bModelLoaded [objP->ModelId ()] = true;
	OglCachePolyModelTextures (objP->ModelId ());
	}
PrintLog (-1);


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

PrintLog (1, "caching cockpit textures\n");
for (i = 0; i < 2; i++)
	for (j = 0; j < MAX_GAUGE_BMS; j++)
		if (gameData.cockpit.gauges [i][j].index != 0xffff)
			LoadTexture (gameData.cockpit.gauges [i][j].index, 0, 0);
PrintLog (-1);

gameStates.render.DisableCartoonStyle ();

ResetSpecialEffects ();
InitSpecialEffects ();
DoSpecialEffects (true);
PrintLog (-1);
return 0;
}

//------------------------------------------------------------------------------


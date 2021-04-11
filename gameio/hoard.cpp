#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "descent.h"
#include "u_mem.h"
#include "strutil.h"
#include "network.h"
#include "multi.h"
#include "object.h"
#include "error.h"
#include "powerup.h"
#include "loadgamedata.h"
#include "sounds.h"
#include "kconfig.h"
#include "config.h"
#include "textures.h"
#include "byteswap.h"
#include "sounds.h"
#include "args.h"
#include "cfile.h"
#include "effects.h"
#include "hudmsgs.h"
#include "gamepal.h"
#include "multi.h"

//-----------------------------------------------------------------------------
///
/// CODE TO LOAD HOARD DATA
///
void InitHoardBitmap (CBitmap *pBm, int32_t w, int32_t h, int32_t flags, uint8_t *data)
{
pBm->Init (BM_LINEAR, 0, 0, w, h, 1, NULL);
pBm->SetBuffer (data, true, w * h);
pBm->SetFlags (flags);
pBm->SetAvgColorIndex (0);
}

//-----------------------------------------------------------------------------

#define	CalcHoardItemSizes(_item) \
			 (_item).nFrameSize = (_item).nWidth * (_item).nHeight; \
			 (_item).nSize = (_item).nFrames * (_item).nFrameSize

//-----------------------------------------------------------------------------

int32_t InitMonsterball (int32_t nBitmap)
{
	CBitmap				*pBm, *pAltBm;
	tAnimationInfo		*pAnimInfo;
	tPowerupTypeInfo	*pTypeInfo;
	int32_t				i;

memcpy (&gameData.hoardData.monsterball, &gameData.hoardData.orb, sizeof (tHoardItem));
gameData.hoardData.monsterball.nClip = gameData.effectData.nClips [0]++;
memcpy (&gameData.effectData.animations [0][gameData.hoardData.monsterball.nClip], 
		  &gameData.effectData.animations [0][gameData.hoardData.orb.nClip],
		  sizeof (tAnimationInfo));
gameData.hoardData.monsterball.bm.Init ();

CTGA tga (&gameData.hoardData.monsterball.bm);

if (!tga.Read ("monsterball.tga", gameFolders.game.szTextures [1], -1, 1.0, 0)) {
	pAltBm = CBitmap::Create (0, 0, 0, 1, "Monsterball");
	if (pAltBm && 
		(tga.Read ("mballgold#0.tga", gameFolders.game.szTextures [1], -1, 1.0, 0) ||
		 tga.Read ("mballred#0.tga", gameFolders.game.szTextures [1], -1, 1.0, 0))) {
		pAnimInfo = &gameData.effectData.animations [0][gameData.hoardData.monsterball.nClip];
		for (i = 0; i < gameData.hoardData.orb.nFrames; i++, nBitmap++) {
			Assert (nBitmap < MAX_BITMAP_FILES);
			pAnimInfo->frames [i].index = nBitmap;
			InitHoardBitmap (&gameData.pigData.tex.bitmaps [0][nBitmap], 
									gameData.hoardData.monsterball.nWidth, 
									gameData.hoardData.monsterball.nHeight, 
									BM_FLAG_TRANSPARENT, 
									NULL);
			}
		pAnimInfo->flags |= WCF_ALTFMT;
		pBm = gameData.pigData.tex.bitmaps [0] + pAnimInfo->frames [0].index;
		pBm->SetOverride (pAltBm);
		*pAltBm = gameData.hoardData.monsterball.bm;
		pAltBm->SetType (BM_TYPE_ALT);
		pAltBm->SetFrameCount ();
		}
	}

//Create monsterball powerup
pTypeInfo = gameData.objData.pwrUp.info + POW_MONSTERBALL;
pTypeInfo->nClipIndex = gameData.hoardData.monsterball.nClip;
pTypeInfo->hitSound = -1; //gameData.objData.pwrUp.info [POW_SHIELD_BOOST].hitSound;
pTypeInfo->size = (gameData.objData.pwrUp.info [POW_SHIELD_BOOST].size * extraGameInfo [1].monsterball.nSizeMod) / 2;
pTypeInfo->light = gameData.objData.pwrUp.info [POW_SHIELD_BOOST].light;
return nBitmap;
}

//-----------------------------------------------------------------------------

void SetupHoardBitmapFrames (CFile& cf, CBitmap& bm, tBitmapIndex* frameIndex, CPalette*& paletteP, int32_t flags)
{
	CPalette	palette;
	uint8_t*	bmDataP;
	int32_t	nSize = bm.Width () * bm.Height ();
	int32_t	nFrameSize = bm.Width () * bm.Width ();
	int32_t	nFrames =  nSize / nFrameSize;
	char		szName [10];

paletteManager.Game ();
palette.Read (cf);
paletteP = paletteManager.Add (palette);
bm.Read (cf);
bm.SetName ("hoard");
bmDataP = bm.Buffer ();
for (int32_t i = 0; i < nFrames; i++) {
	CBitmap* pBm = &gameData.pigData.tex.bitmaps [0][frameIndex [i].index];
	InitHoardBitmap (pBm, gameData.hoardData.goal.nWidth, gameData.hoardData.goal.nHeight, flags, bmDataP);
	sprintf (szName, "hoard#%d", i + 1);
	pBm->SetName (szName);
	pBm->SetPalette (&palette, 255, -1);
	bmDataP += nFrameSize;
	}
}

//-----------------------------------------------------------------------------

void InitHoardData (void)
{
	CFile					cf;
	CPalette				palette;
	int32_t				i, j, fPos, nBitmap;
	tAnimationInfo*	pAnimInfo;
	tEffectInfo*		pEffectInfo;
	tPowerupTypeInfo*	pTypeInfo;
	uint8_t*				bmDataP;

#if !DBG
if (!(gameData.appData.GameMode (GM_HOARD | GM_ENTROPY | GM_MONSTERBALL)))
	return;
#endif
if (gameStates.app.bDemoData || gameStates.app.bD1Mission) {
#if !DBG
	Warning ("Hoard data not available with demo data.");
#endif
	return;
	}
if (!cf.Open ("hoard.ham", gameFolders.game.szData [0], "rb", 0)) {
	Warning ("Cannot open hoard data file <hoard.ham>.");
	return;
	}

gameData.hoardData.orb.nFrames = cf.ReadShort ();
gameData.hoardData.orb.nWidth = cf.ReadShort ();
gameData.hoardData.orb.nHeight = cf.ReadShort ();
CalcHoardItemSizes (gameData.hoardData.orb);
fPos = (int32_t) cf.Tell ();
cf.Seek (long (palette.Size () + gameData.hoardData.orb.nSize), SEEK_CUR);
gameData.hoardData.goal.nFrames = cf.ReadShort ();
cf.Seek (fPos, SEEK_SET);

if (!gameData.hoardData.bInitialized) {
	gameData.hoardData.bInitialized = 1;
	gameData.hoardData.goal.nWidth  = 
	gameData.hoardData.goal.nHeight = 64;
	CalcHoardItemSizes (gameData.hoardData.goal);
	nBitmap = gameData.pigData.tex.nBitmaps [0];
	
	//Create orb animation clip
	gameData.hoardData.orb.nClip = gameData.effectData.nClips [0]++;
	Assert (gameData.effectData.nClips [0] <= MAX_ANIMATIONS_D2);
	pAnimInfo = &gameData.effectData.animations [0][gameData.hoardData.orb.nClip];
	pAnimInfo->xTotalTime = I2X (1)/2;
	pAnimInfo->nFrameCount = gameData.hoardData.orb.nFrames;
	pAnimInfo->xFrameTime = pAnimInfo->xTotalTime / pAnimInfo->nFrameCount;
	pAnimInfo->flags = 0;
	pAnimInfo->nSound = -1;
	pAnimInfo->lightValue = I2X (1);
	bmDataP = NEW uint8_t [gameData.hoardData.orb.nSize];
	gameData.hoardData.orb.bm.Init (BM_LINEAR, 0, 0, gameData.hoardData.orb.nWidth, gameData.hoardData.orb.nHeight * gameData.hoardData.orb.nFrames);
	gameData.hoardData.orb.bm.SetBuffer (bmDataP, 0, gameData.hoardData.orb.nSize);
	for (i = 0; i < gameData.hoardData.orb.nFrames; i++, nBitmap++) {
		Assert (nBitmap < MAX_BITMAP_FILES);
		pAnimInfo->frames [i].index = nBitmap;
#if 0
		InitHoardBitmap (&gameData.pigData.tex.bitmaps [0][nBitmap], 
							  gameData.hoardData.orb.nWidth, 
							  gameData.hoardData.orb.nHeight, 
							  BM_FLAG_TRANSPARENT, 
							  bmDataP);
		bmDataP += gameData.hoardData.orb.nFrameSize;
#endif
		}
	
	//Create hoard orb powerup
	pTypeInfo = gameData.objData.pwrUp.info + POW_HOARD_ORB;
	pTypeInfo->nClipIndex = gameData.hoardData.orb.nClip;
	pTypeInfo->hitSound = gameData.objData.pwrUp.info [POW_SHIELD_BOOST].hitSound;
	pTypeInfo->size = gameData.objData.pwrUp.info [POW_SHIELD_BOOST].size;
	pTypeInfo->light = gameData.objData.pwrUp.info [POW_SHIELD_BOOST].light;
	
	//Create orb goal wall effect
	gameData.hoardData.goal.nClip = gameData.effectData.nEffects [0]++;
	Assert (gameData.effectData.nEffects [0] < MAX_EFFECTS);
	if ((pEffectInfo = gameData.effectData.effects [0] + gameData.hoardData.goal.nClip)) {
		*pEffectInfo = gameData.effectData.effects [0][94];        //copy from blue goal
		pEffectInfo->changing.nWallTexture = gameData.pigData.tex.nTextures [0];
		pEffectInfo->animationInfo.nFrameCount = gameData.hoardData.goal.nFrames;
		pEffectInfo->flags &= ~EF_INITIALIZED;
		}
	i = gameData.pigData.tex.nTextures [0];
	if (0 <= (j = MultiFindGoalTexture (TMI_GOAL_BLUE)))
		gameData.pigData.tex.pTexMapInfo [i] = gameData.pigData.tex.pTexMapInfo [j];
	gameData.pigData.tex.pTexMapInfo [i].nEffectClip = gameData.hoardData.goal.nClip;
	gameData.pigData.tex.pTexMapInfo [i].flags = TMI_GOAL_HOARD;
	gameData.hoardData.nTextures = gameData.pigData.tex.nTextures [0]++;
	Assert (gameData.pigData.tex.nTextures [0] < MAX_TEXTURES);
	bmDataP = NEW uint8_t [gameData.hoardData.goal.nSize];
	gameData.hoardData.goal.bm.Init (BM_LINEAR, 0, 0, gameData.hoardData.goal.nWidth, gameData.hoardData.goal.nHeight * gameData.hoardData.goal.nFrames);
	gameData.hoardData.goal.bm.SetBuffer (bmDataP, 0, gameData.hoardData.goal.nSize);
	for (i = 0; i < gameData.hoardData.goal.nFrames; i++, nBitmap++) {
		Assert (nBitmap < MAX_BITMAP_FILES);
		pEffectInfo->animationInfo.frames [i].index = nBitmap;
#if 0
		InitHoardBitmap (&gameData.pigData.tex.bitmaps [0][nBitmap], 
							  gameData.hoardData.goal.nWidth, 
							  gameData.hoardData.goal.nHeight, 
							  0, 
							  bmDataP);
		bmDataP += gameData.hoardData.goal.nFrameSize;
#endif
		}
	nBitmap = InitMonsterball (nBitmap);
	}
else {
	pEffectInfo = gameData.effectData.effects [0] + gameData.hoardData.goal.nClip;
	}

//Load and remap bitmap data for orb
#if 1
SetupHoardBitmapFrames (cf, gameData.hoardData.orb.bm, gameData.effectData.animations [0][gameData.hoardData.orb.nClip].frames, gameData.hoardData.orb.palette, BM_FLAG_TRANSPARENT);
#else
paletteManager.Game ();
palette.Read (cf);
gameData.hoardData.orb.palette = paletteManager.Add (palette);
pAnimInfo = &gameData.effectData.animations [0][gameData.hoardData.orb.nClip];
gameData.hoardData.orb.bm.Read (cf);
bmDataP = gameData.hoardData.orb.bm.Buffer ();
for (i = 0; i < gameData.hoardData.orb.nFrames; i++) {
	CBitmap* pBm = &gameData.pigData.tex.bitmaps [0][pAnimInfo->frames [i].index];
	InitHoardBitmap (pBm, gameData.hoardData.goal.nWidth, gameData.hoardData.goal.nHeight, 0, bmDataP);
	bmDataP += gameData.hoardData.goal.nFrameSize;
	pBm->SetPalette (gameData.hoardData.orb.palette, 255, -1);
	}
#endif
//Load and remap bitmap data for goal texture
cf.ReadShort ();        //skip frame count
#if 1
SetupHoardBitmapFrames (cf, gameData.hoardData.goal.bm, pEffectInfo->animationInfo.frames, gameData.hoardData.goal.palette, 0);
#else
paletteManager.Game ();
palette.Read (cf);
gameData.hoardData.goal.palette = paletteManager.Add (palette);
gameData.hoardData.goal.bm.Read (cf);
bmDataP = gameData.hoardData.goal.bm.Buffer ();
for (i = 0; i < gameData.hoardData.goal.nFrames; i++) {
	CBitmap* pBm = gameData.pigData.tex.bitmaps [0] + pEffectInfo->animationInfo.frames [i].index;
	InitHoardBitmap (pBm, gameData.hoardData.goal.nWidth, gameData.hoardData.goal.nHeight, 0, bmDataP);
	bmDataP += gameData.hoardData.goal.nFrameSize;
	pBm->SetPalette (gameData.hoardData.goal.palette, 255, -1);
	}
#endif
//Load and remap bitmap data for HUD icons

#if 0
for (i = 0; i < 2; i++) {
	gameData.hoardData.icon [i].nFrames = 1;
	gameData.hoardData.icon [i].nHeight = cf.ReadShort ();
	gameData.hoardData.icon [i].nWidth = cf.ReadShort ();
	CalcHoardItemSizes (gameData.hoardData.icon [i]);
	if (!gameData.hoardData.bInitialized) {
		InitHoardBitmap (&gameData.hoardData.icon [i].bm, 
							  gameData.hoardData.icon [i].nHeight, 
							  gameData.hoardData.icon [i].nWidth, 
							  BM_FLAG_TRANSPARENT, 
							  NULL);
		gameData.hoardData.icon [i].bm.CreateBuffer ();
		}
	palette.Read (cf);
	gameData.hoardData.icon [i].palette = paletteManager.Add (palette);
	gameData.hoardData.icon [i].bm.Read (cf, gameData.hoardData.icon [i].nFrameSize);
	gameData.hoardData.icon [i].bm.Remap (gameData.hoardData.icon [i].palette, 255, -1);
	}
#endif

cf.Seek (60677, SEEK_SET);
if (!gameData.hoardData.bInitialized) {
	//Load sounds for orb game
	for (i = 0; i < 4; i++) {
		int32_t len = cf.ReadInt ();        //get 11k len
		if (gameOpts->sound.audioSampleRate == SAMPLE_RATE_22K) {
			cf.Seek (len, SEEK_CUR);     //skip over 11k sample
			len = cf.ReadInt ();    //get 22k len
			}
		j = gameData.pigData.sound.nSoundFiles [0] + i;
		gameData.pigData.sound.sounds [0][j].nLength [0] = len;
		gameData.pigData.sound.sounds [0][j].data [0].Create (len);
		gameData.pigData.sound.sounds [0][j].data [0].Read (cf);
		if (gameOpts->sound.audioSampleRate == SAMPLE_RATE_11K) {
			len = cf.ReadInt ();    //get 22k len
			cf.Seek (len, SEEK_CUR);     //skip over 22k sample
			}
		AltSounds [0][SOUND_YOU_GOT_ORB + i] = 
		Sounds [0][SOUND_YOU_GOT_ORB + i] = j;
		}
	gameData.pigData.sound.nSoundFiles [0] += 4;
	}
cf.Close ();
}

//-----------------------------------------------------------------------------

void ResetHoardData (void)
{
#if 0
gameData.hoardData.orb.bm.SetTexture (NULL);
gameData.hoardData.goal.bm.SetTexture (NULL);
if (gameData.hoardData.monsterball.bm.Override ())
	gameData.hoardData.monsterball.bm.Override ()->SetTexture (NULL);
else
	gameData.hoardData.monsterball.bm.SetTexture (NULL);
for (int32_t i = 0; i < 2; i++)
	gameData.hoardData.icon [i].bm.SetTexture (NULL);
#endif
}

//-----------------------------------------------------------------------------

void FreeHoardData (void)
{
	int32_t		i;
	CBitmap	*pBm;

if (!gameData.hoardData.bInitialized)
	return;
pBm = gameData.hoardData.monsterball.bm.Override () ? gameData.hoardData.monsterball.bm.Override () : &gameData.hoardData.monsterball.bm;
if (pBm->Buffer () == gameData.hoardData.orb.bm.Buffer ())
	pBm->SetBuffer (NULL);
else {
	pBm->DestroyBuffer ();
	if (gameData.hoardData.monsterball.bm.Override ())
		delete pBm;
	}
gameData.hoardData.orb.bm.Destroy ();
gameData.hoardData.goal.bm.Destroy ();
pBm = &gameData.hoardData.monsterball.bm;
memset (&gameData.hoardData.orb.bm, 0, sizeof (CBitmap));
for (i = 0; i < 2; i++)
	gameData.hoardData.icon [i].bm.Destroy ();
for (i = 1; i <= 4; i++) {
	gameData.pigData.sound.sounds [0][gameData.pigData.sound.nSoundFiles [0] - i].data [0].Destroy ();
	}
gameData.effectData.animations [0][gameData.hoardData.monsterball.nClip].nFrameCount = 
gameData.effectData.animations [0][gameData.hoardData.orb.nClip].nFrameCount = 0;
gameData.effectData.nClips [0] = gameData.hoardData.orb.nClip;
gameData.effectData.nEffects [0] = gameData.hoardData.goal.nClip;
gameData.pigData.tex.nTextures [0] = gameData.hoardData.nTextures;
gameData.hoardData.bInitialized = 0;
}

//-----------------------------------------------------------------------------
//eof

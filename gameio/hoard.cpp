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
void InitHoardBitmap (CBitmap *bmP, int w, int h, int flags, ubyte *data)
{
bmP->Init (BM_LINEAR, 0, 0, w, h, 1, NULL);
bmP->SetBuffer (data, true, w * h);
bmP->SetFlags (flags);
bmP->SetAvgColorIndex (0);
}

//-----------------------------------------------------------------------------

#define	CalcHoardItemSizes(_item) \
			 (_item).nFrameSize = (_item).nWidth * (_item).nHeight; \
			 (_item).nSize = (_item).nFrames * (_item).nFrameSize

//-----------------------------------------------------------------------------

int InitMonsterball (int nBitmap)
{
	CBitmap				*bmP, *altBmP;
	tVideoClip			*vcP;
	tPowerupTypeInfo	*ptP;
	int					i;

memcpy (&gameData.hoard.monsterball, &gameData.hoard.orb, sizeof (tHoardItem));
gameData.hoard.monsterball.nClip = gameData.eff.nClips [0]++;
memcpy (&gameData.eff.vClips [0][gameData.hoard.monsterball.nClip], 
		  &gameData.eff.vClips [0][gameData.hoard.orb.nClip],
		  sizeof (tVideoClip));
if (!ReadTGA ("monsterball.tga", gameFolders.szTextureDir [0], &gameData.hoard.monsterball.bm, -1, 1.0, 0, 0)) {
	altBmP = CBitmap::Create (0, 0, 0, 1, "Monsterball");
	if (altBmP && 
		(ReadTGA ("mballgold#0.tga", gameFolders.szTextureDir [0], &gameData.hoard.monsterball.bm, -1, 1.0, 0, 0) ||
		 ReadTGA ("mballred#0.tga", gameFolders.szTextureDir [0], &gameData.hoard.monsterball.bm, -1, 1.0, 0, 0))) {
		vcP = &gameData.eff.vClips [0][gameData.hoard.monsterball.nClip];
		for (i = 0; i < gameData.hoard.orb.nFrames; i++, nBitmap++) {
			Assert (nBitmap < MAX_BITMAP_FILES);
			vcP->frames [i].index = nBitmap;
			InitHoardBitmap (&gameData.pig.tex.bitmaps [0][nBitmap], 
									gameData.hoard.monsterball.nWidth, 
									gameData.hoard.monsterball.nHeight, 
									BM_FLAG_TRANSPARENT, 
									NULL);
			}
		vcP->flags |= WCF_ALTFMT;
		bmP = gameData.pig.tex.bitmaps [0] + vcP->frames [0].index;
		bmP->SetOverride (altBmP);
		*altBmP = gameData.hoard.monsterball.bm;
		altBmP->SetType (BM_TYPE_ALT);
		altBmP->SetFrameCount ();
		}
	}
//Create monsterball powerup
ptP = gameData.objs.pwrUp.info + POW_MONSTERBALL;
ptP->nClipIndex = gameData.hoard.monsterball.nClip;
ptP->hitSound = -1; //gameData.objs.pwrUp.info [POW_SHIELD_BOOST].hitSound;
ptP->size = (gameData.objs.pwrUp.info [POW_SHIELD_BOOST].size * extraGameInfo [1].monsterball.nSizeMod) / 2;
ptP->light = gameData.objs.pwrUp.info [POW_SHIELD_BOOST].light;
return nBitmap;
}

//-----------------------------------------------------------------------------

void InitHoardData (void)
{
	CFile					cf;
	CPalette				palette;
	int					i, j, fPos, nBitmap;
	tVideoClip			*vcP;
	tEffectClip			*ecP;
	tPowerupTypeInfo	*ptP;
	ubyte					*bmDataP;

#if !DBG
if (!(gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY | GM_MONSTERBALL)))
	return;
#endif
if (gameStates.app.bDemoData) {
#if !DBG
	Warning ("Hoard data not available with demo data.");
#endif
	return;
	}
if (!cf.Open ("hoard.ham", gameFolders.szDataDir, "rb", 0)) {
	Warning ("Cannot open hoard data file <hoard.ham>.");
	return;
	}

gameData.hoard.orb.nFrames = cf.ReadShort ();
gameData.hoard.orb.nWidth = cf.ReadShort ();
gameData.hoard.orb.nHeight = cf.ReadShort ();
CalcHoardItemSizes (gameData.hoard.orb);
fPos = cf.Tell ();
cf.Seek (palette.Size () + gameData.hoard.orb.nSize, SEEK_CUR);
gameData.hoard.goal.nFrames = cf.ReadShort ();
cf.Seek (fPos, SEEK_SET);

if (!gameData.hoard.bInitialized) {
	gameData.hoard.goal.nWidth  = 
	gameData.hoard.goal.nHeight = 64;
	CalcHoardItemSizes (gameData.hoard.goal);
	nBitmap = gameData.pig.tex.nBitmaps [0];
	//Create orb tVideoClip
	gameData.hoard.orb.nClip = gameData.eff.nClips [0]++;
	Assert (gameData.eff.nClips [0] <= VCLIP_MAXNUM);
	vcP = &gameData.eff.vClips [0][gameData.hoard.orb.nClip];
	vcP->xTotalTime = I2X (1)/2;
	vcP->nFrameCount = gameData.hoard.orb.nFrames;
	vcP->xFrameTime = vcP->xTotalTime / vcP->nFrameCount;
	vcP->flags = 0;
	vcP->nSound = -1;
	vcP->lightValue = I2X (1);
	bmDataP = new ubyte [gameData.hoard.orb.nSize];
	gameData.hoard.orb.bm.SetBuffer (bmDataP, 0, gameData.hoard.orb.nSize);
	for (i = 0; i < gameData.hoard.orb.nFrames; i++, nBitmap++) {
		Assert (nBitmap < MAX_BITMAP_FILES);
		vcP->frames [i].index = nBitmap;
		InitHoardBitmap (&gameData.pig.tex.bitmaps [0][nBitmap], 
							  gameData.hoard.orb.nWidth, 
							  gameData.hoard.orb.nHeight, 
							  BM_FLAG_TRANSPARENT, 
							  bmDataP);
		bmDataP += gameData.hoard.orb.nFrameSize;
		}
	//Create hoard orb powerup
	ptP = gameData.objs.pwrUp.info + POW_HOARD_ORB;
	ptP->nClipIndex = gameData.hoard.orb.nClip;
	ptP->hitSound = gameData.objs.pwrUp.info [POW_SHIELD_BOOST].hitSound;
	ptP->size = gameData.objs.pwrUp.info [POW_SHIELD_BOOST].size;
	ptP->light = gameData.objs.pwrUp.info [POW_SHIELD_BOOST].light;
	//Create orb goal CWall effect
	gameData.hoard.goal.nClip = gameData.eff.nEffects [0]++;
	Assert (gameData.eff.nEffects [0] < MAX_EFFECTS);
	ecP = gameData.eff.effects [0] + gameData.hoard.goal.nClip;
	*ecP = gameData.eff.effects [0][94];        //copy from blue goal
	ecP->changingWallTexture = gameData.pig.tex.nTextures [0];
	ecP->vc.nFrameCount = gameData.hoard.goal.nFrames;
	ecP->flags &= ~EF_INITIALIZED;

	i = gameData.pig.tex.nTextures [0];
	if (0 <= (j = MultiFindGoalTexture (TMI_GOAL_BLUE)))
		gameData.pig.tex.tMapInfoP [i] = gameData.pig.tex.tMapInfoP [j];
	gameData.pig.tex.tMapInfoP [i].nEffectClip = gameData.hoard.goal.nClip;
	gameData.pig.tex.tMapInfoP [i].flags = TMI_GOAL_HOARD;
	gameData.pig.tex.nTextures [0]++;
	Assert (gameData.pig.tex.nTextures [0] < MAX_TEXTURES);
	bmDataP = new ubyte [gameData.hoard.goal.nSize];
	gameData.hoard.goal.bm.SetBuffer (bmDataP, 0, gameData.hoard.goal.nSize);
	for (i = 0; i < gameData.hoard.goal.nFrames; i++, nBitmap++) {
		Assert (nBitmap < MAX_BITMAP_FILES);
		ecP->vc.frames [i].index = nBitmap;
		InitHoardBitmap (gameData.pig.tex.bitmaps [0] + nBitmap, 
							  gameData.hoard.goal.nWidth, 
							  gameData.hoard.goal.nHeight, 
							  0, 
							  bmDataP);
		bmDataP += gameData.hoard.goal.nFrameSize;
		gameData.hoard.nBitmaps = nBitmap;
		}
	nBitmap = InitMonsterball (nBitmap);
	}
else {
	ecP = gameData.eff.effects [0] + gameData.hoard.goal.nClip;
	}

//Load and remap bitmap data for orb
paletteManager.Game ();
palette.Read (cf);
gameData.hoard.orb.palette = paletteManager.Add (palette);
vcP = &gameData.eff.vClips [0][gameData.hoard.orb.nClip];
bmDataP = gameData.hoard.orb.bm.Buffer ();
for (i = 0; i < gameData.hoard.orb.nFrames; i++) {
	CBitmap* bmP = &gameData.pig.tex.bitmaps [0][vcP->frames [i].index];
	InitHoardBitmap (bmP, gameData.hoard.goal.nWidth, gameData.hoard.goal.nHeight, 0, bmDataP);
	bmDataP += gameData.hoard.goal.nFrameSize;
	bmP->Read (cf, gameData.hoard.orb.nFrameSize);
	bmP->Remap (gameData.hoard.orb.palette, 255, -1);
	}

//Load and remap bitmap data for goal texture
cf.ReadShort ();        //skip frame count
paletteManager.Game ();
palette.Read (cf);
gameData.hoard.goal.palette = paletteManager.Add (palette);
bmDataP = gameData.hoard.goal.bm.Buffer ();
for (i = 0; i < gameData.hoard.goal.nFrames; i++) {
	CBitmap* bmP = gameData.pig.tex.bitmaps [0] + ecP->vc.frames [i].index;
	InitHoardBitmap (bmP, gameData.hoard.goal.nWidth, gameData.hoard.goal.nHeight, 0, bmDataP);
	bmDataP += gameData.hoard.goal.nFrameSize;
	bmP->Read (cf, gameData.hoard.goal.nFrameSize);
	bmP->Remap (gameData.hoard.goal.palette, 255, -1);
	}

//Load and remap bitmap data for HUD icons
for (i = 0; i < 2; i++) {
	gameData.hoard.icon [i].nFrames = 1;
	gameData.hoard.icon [i].nHeight = cf.ReadShort ();
	gameData.hoard.icon [i].nWidth = cf.ReadShort ();
	CalcHoardItemSizes (gameData.hoard.icon [i]);
	if (!gameData.hoard.bInitialized) {
		InitHoardBitmap (&gameData.hoard.icon [i].bm, 
							  gameData.hoard.icon [i].nHeight, 
							  gameData.hoard.icon [i].nWidth, 
							  BM_FLAG_TRANSPARENT, 
							  NULL);
		gameData.hoard.icon [i].bm.CreateBuffer ();
		}
	palette.Read (cf);
	gameData.hoard.icon [i].palette = paletteManager.Add (palette);
	gameData.hoard.icon [i].bm.Read (cf, gameData.hoard.icon [i].nFrameSize);
	gameData.hoard.icon [i].bm.Remap (gameData.hoard.icon [i].palette, 255, -1);
	}

if (!gameData.hoard.bInitialized) {
	//Load sounds for orb game
	for (i = 0; i < 4; i++) {
		int len = cf.ReadInt ();        //get 11k len
		if (gameOpts->sound.digiSampleRate == SAMPLE_RATE_22K) {
			cf.Seek (len, SEEK_CUR);     //skip over 11k sample
			len = cf.ReadInt ();    //get 22k len
			}
		gameData.pig.sound.sounds [0][gameData.pig.sound.nSoundFiles [0] + i].nLength [0] = len;
		gameData.pig.sound.sounds [0][gameData.pig.sound.nSoundFiles [0] + i].data [0].Create (len);
		gameData.pig.sound.sounds [0][gameData.pig.sound.nSoundFiles [0] + i].data [0].Read (cf);
		if (gameOpts->sound.digiSampleRate == SAMPLE_RATE_11K) {
			len = cf.ReadInt ();    //get 22k len
			cf.Seek (len, SEEK_CUR);     //skip over 22k sample
			}
		Sounds [0][SOUND_YOU_GOT_ORB + i] = gameData.pig.sound.nSoundFiles [0] + i;
		AltSounds [0][SOUND_YOU_GOT_ORB + i] = Sounds [0][SOUND_YOU_GOT_ORB + i];
		}
	gameData.pig.sound.nSoundFiles [0] += 4;
	}
cf.Close ();
gameData.hoard.bInitialized = 1;
}

//-----------------------------------------------------------------------------

void ResetHoardData (void)
{
	int	i;

gameData.hoard.orb.bm.SetTexture (NULL);
gameData.hoard.goal.bm.SetTexture (NULL);
if (gameData.hoard.monsterball.bm.Override ())
	gameData.hoard.monsterball.bm.Override ()->SetTexture (NULL);
else
	gameData.hoard.monsterball.bm.SetTexture (NULL);
for (i = 0; i < 2; i++)
	gameData.hoard.icon [i].bm.SetTexture (NULL);
}

//-----------------------------------------------------------------------------

void FreeHoardData (void)
{
	int		i;
	CBitmap	*bmP;

if (!gameData.hoard.bInitialized)
	return;
bmP = gameData.hoard.monsterball.bm.Override () ? gameData.hoard.monsterball.bm.Override () : &gameData.hoard.monsterball.bm;
if (bmP->Buffer () == gameData.hoard.orb.bm.Buffer ())
	bmP->SetBuffer (NULL);
else {
	bmP->DestroyBuffer ();
	if (gameData.hoard.monsterball.bm.Override ())
		delete bmP;
	}
gameData.hoard.orb.bm.Destroy ();
gameData.hoard.goal.bm.Destroy ();
bmP = &gameData.hoard.monsterball.bm;
memset (&gameData.hoard.orb.bm, 0, sizeof (CBitmap));
for (i = 0; i < 2; i++)
	gameData.hoard.icon [i].bm.Destroy ();
for (i = 1; i <= 4; i++) {
	gameData.pig.sound.sounds [0][gameData.pig.sound.nSoundFiles [0] - i].data [0].Destroy ();
	}
gameData.eff.vClips [0][gameData.hoard.monsterball.nClip].nFrameCount = 
gameData.eff.vClips [0][gameData.hoard.orb.nClip].nFrameCount = 0;
gameData.eff.nClips [0] = gameData.hoard.orb.nClip;
gameData.hoard.bInitialized = 0;
}
//-----------------------------------------------------------------------------
//eof

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "inferno.h"
#include "u_mem.h"
#include "strutil.h"
#include "network.h"
#include "multi.h"
#include "object.h"
#include "error.h"
#include "powerup.h"
#include "bm.h"
#include "sounds.h"
#include "kconfig.h"
#include "config.h"
#include "textures.h"
#include "byteswap.h"
#include "sounds.h"
#include "args.h"
#include "cfile.h"
#include "effects.h"
#include "hudmsg.h"
#include "gamepal.h"
#include "multi.h"

//-----------------------------------------------------------------------------
///
/// CODE TO LOAD HOARD DATA
///
void InitHoardBitmap (grsBitmap *bmP, int w, int h, int flags, ubyte *data)
{
bmP->bm_props.x = 
bmP->bm_props.y = 0;
bmP->bm_props.w = 
bmP->bm_props.rowsize = w;
bmP->bm_props.h = h;
bmP->bm_props.nType = BM_LINEAR;
bmP->bm_props.flags = flags;
if (data)
	bmP->bm_texBuf = data;
//bmP->bm_handle = 0;
bmP->bm_avgColor = 0;
}

//-----------------------------------------------------------------------------

#define	CalcHoardItemSizes(_item) \
			 (_item).nFrameSize = (_item).nWidth * (_item).nHeight; \
			 (_item).nSize = (_item).nFrames * (_item).nFrameSize

//-----------------------------------------------------------------------------

int InitMonsterball (int nBitmap)
{
	grsBitmap			*bmP, *altBmP;
	tVideoClip			*vcP;
	powerupType_info	*ptP;
	int					i;

memcpy (&gameData.hoard.monsterball, &gameData.hoard.orb, sizeof (tHoardItem));
gameData.hoard.monsterball.nClip = gameData.eff.nClips [0]++;
memcpy (&gameData.eff.vClips [0][gameData.hoard.monsterball.nClip], 
		  &gameData.eff.vClips [0][gameData.hoard.orb.nClip],
		  sizeof (tVideoClip));
if (!ReadTGA ("monsterball.tga", gameFolders.szTextureDir [0], &gameData.hoard.monsterball.bm, -1, 1.0, 0, 0)) {
	altBmP = (grsBitmap *) D2_ALLOC (sizeof (grsBitmap));
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
		BM_OVERRIDE (bmP) = altBmP;
		*altBmP = gameData.hoard.monsterball.bm;
		altBmP->bmType = BM_TYPE_ALT;
		altBmP->bm_data.alt.bm_frameCount = altBmP->bm_props.h / altBmP->bm_props.w;
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
	ubyte					palette [256*3];
	CFILE					*fp;
	int					i, fPos, nBitmap;
	tVideoClip			*vcP;
	tEffectClip			*ecP;
	powerupType_info	*ptP;
	ubyte					*bmDataP;

#ifdef RELEASE
if (!(gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY | GM_MONSTERBALL)))
	return;
#endif
if (gameStates.app.bDemoData) {
#ifndef _DEBUG
	Warning ("Hoard data not available with demo data.");
#endif
	return;
	}
if (!(fp = CFOpen ("hoard.ham", gameFolders.szDataDir, "rb", 0))) {
	Warning ("Cannot open hoard data file <hoard.ham>.");
	return;
	}

gameData.hoard.orb.nFrames = CFReadShort (fp);
gameData.hoard.orb.nWidth = CFReadShort (fp);
gameData.hoard.orb.nHeight = CFReadShort (fp);
CalcHoardItemSizes (gameData.hoard.orb);
fPos = CFTell (fp);
CFSeek (fp, sizeof (palette) + gameData.hoard.orb.nSize, SEEK_CUR);
gameData.hoard.goal.nFrames = CFReadShort (fp);
CFSeek (fp, fPos, SEEK_SET);

if (!gameData.hoard.bInitialized) {
	gameData.hoard.goal.nWidth  = 
	gameData.hoard.goal.nHeight = 64;
	CalcHoardItemSizes (gameData.hoard.goal);
	nBitmap = gameData.pig.tex.nBitmaps [0];
	//Create orb tVideoClip
	gameData.hoard.orb.nClip = gameData.eff.nClips [0]++;
	Assert (gameData.eff.nClips [0] <= VCLIP_MAXNUM);
	vcP = &gameData.eff.vClips [0][gameData.hoard.orb.nClip];
	vcP->xTotalTime = F1_0/2;
	vcP->nFrameCount = gameData.hoard.orb.nFrames;
	vcP->xFrameTime = vcP->xTotalTime / vcP->nFrameCount;
	vcP->flags = 0;
	vcP->nSound = -1;
	vcP->lightValue = F1_0;
	bmDataP = gameData.hoard.orb.bm.bm_texBuf = (ubyte *) D2_ALLOC (gameData.hoard.orb.nSize);
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
	ptP->hitSound = -1; //gameData.objs.pwrUp.info [POW_SHIELD_BOOST].hitSound;
	ptP->size = gameData.objs.pwrUp.info [POW_SHIELD_BOOST].size;
	ptP->light = gameData.objs.pwrUp.info [POW_SHIELD_BOOST].light;
	//Create orb goal tWall effect
	gameData.hoard.goal.nClip = gameData.eff.nEffects [0]++;
	Assert (gameData.eff.nEffects [0] < MAX_EFFECTS);
	ecP = gameData.eff.effects [0] + gameData.hoard.goal.nClip;
	*ecP = gameData.eff.effects [0][94];        //copy from blue goal
	ecP->changingWallTexture = gameData.pig.tex.nTextures [0];
	ecP->vc.nFrameCount = gameData.hoard.goal.nFrames;
	ecP->flags &= ~EF_INITIALIZED;

	i = gameData.pig.tex.nTextures [0];
	gameData.pig.tex.pTMapInfo [i] = gameData.pig.tex.pTMapInfo [MultiFindGoalTexture (TMI_GOAL_BLUE)];
	gameData.pig.tex.pTMapInfo [i].eclip_num = gameData.hoard.goal.nClip;
	gameData.pig.tex.pTMapInfo [i].flags = TMI_GOAL_HOARD;
	gameData.pig.tex.nTextures [0]++;
	Assert (gameData.pig.tex.nTextures [0] < MAX_TEXTURES);
	bmDataP = gameData.hoard.goal.bm.bm_texBuf = (ubyte *) D2_ALLOC (gameData.hoard.goal.nSize);
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
CFRead (palette, 3, 256, fp);
gameData.hoard.orb.palette = AddPalette (palette);
vcP = &gameData.eff.vClips [0][gameData.hoard.orb.nClip];
for (i = 0; i < gameData.hoard.orb.nFrames; i++) {
	grsBitmap *bmP = &gameData.pig.tex.bitmaps [0][vcP->frames [i].index];
	CFRead (bmP->bm_texBuf, 1, gameData.hoard.orb.nFrameSize, fp);
	GrRemapBitmapGood (bmP, gameData.hoard.orb.palette, 255, -1);
	}

//Load and remap bitmap data for goal texture
CFReadShort (fp);        //skip frame count
CFRead (palette, 3, 256, fp);
gameData.hoard.goal.palette = AddPalette (palette);
for (i = 0; i < gameData.hoard.goal.nFrames; i++) {
	grsBitmap *bmP = gameData.pig.tex.bitmaps [0] + ecP->vc.frames [i].index;
	CFRead (bmP->bm_texBuf, 1, gameData.hoard.goal.nFrameSize, fp);
	GrRemapBitmapGood (bmP, gameData.hoard.goal.palette, 255, -1);
	}

//Load and remap bitmap data for HUD icons
for (i = 0; i < 2; i++) {
	gameData.hoard.icon [i].nFrames = 1;
	gameData.hoard.icon [i].nHeight = CFReadShort (fp);
	gameData.hoard.icon [i].nWidth = CFReadShort (fp);
	CalcHoardItemSizes (gameData.hoard.icon [i]);
	if (!gameData.hoard.bInitialized) {
		gameData.hoard.icon [i].bm.bm_texBuf = (ubyte *) D2_ALLOC (gameData.hoard.icon [i].nSize);
		InitHoardBitmap (&gameData.hoard.icon [i].bm, 
							  gameData.hoard.icon [i].nHeight, 
							  gameData.hoard.icon [i].nWidth, 
							  BM_FLAG_TRANSPARENT, 
							  gameData.hoard.icon [i].bm.bm_texBuf);
		}
	CFRead (palette, 3, 256, fp);
	gameData.hoard.icon [i].palette = AddPalette (palette);
	CFRead (gameData.hoard.icon [i].bm.bm_texBuf, 1, gameData.hoard.icon [i].nFrameSize, fp);
	GrRemapBitmapGood (&gameData.hoard.icon [i].bm, gameData.hoard.icon [i].palette, 255, -1);
	}

if (!gameData.hoard.bInitialized) {
	//Load sounds for orb game
	for (i = 0; i < 4; i++) {
		int len = CFReadInt (fp);        //get 11k len
		if (gameOpts->sound.digiSampleRate == SAMPLE_RATE_22K) {
			CFSeek (fp, len, SEEK_CUR);     //skip over 11k sample
			len = CFReadInt (fp);    //get 22k len
			}
		gameData.pig.sound.sounds [0][gameData.pig.sound.nSoundFiles [0] + i].nLength [0] = len;
		gameData.pig.sound.sounds [0][gameData.pig.sound.nSoundFiles [0] + i].data [0] = D2_ALLOC (len);
		CFRead (gameData.pig.sound.sounds [0][gameData.pig.sound.nSoundFiles [0] + i].data [0], 1, len, fp);
		if (gameOpts->sound.digiSampleRate == SAMPLE_RATE_11K) {
			len = CFReadInt (fp);    //get 22k len
			CFSeek (fp, len, SEEK_CUR);     //skip over 22k sample
			}
		Sounds [0][SOUND_YOU_GOT_ORB + i] = gameData.pig.sound.nSoundFiles [0] + i;
		AltSounds [0][SOUND_YOU_GOT_ORB + i] = Sounds [0][SOUND_YOU_GOT_ORB + i];
		}
	gameData.pig.sound.nSoundFiles [0] += 4;
	}
CFClose (fp);
gameData.hoard.bInitialized = 1;
}

//-----------------------------------------------------------------------------

void ResetHoardData (void)
{
	int	i;

gameData.hoard.orb.bm.glTexture = 0;
gameData.hoard.goal.bm.glTexture = 0;
if (BM_OVERRIDE (&gameData.hoard.monsterball.bm))
	BM_OVERRIDE (&gameData.hoard.monsterball.bm)->glTexture = 0;
else
	gameData.hoard.monsterball.bm.glTexture = 0;
for (i = 0; i < 2; i++)
	gameData.hoard.icon [i].bm.glTexture = 0;
}

//-----------------------------------------------------------------------------

void FreeHoardData (void)
{
	int			i;
	grsBitmap	*bmP;

if (!gameData.hoard.bInitialized)
	return;
D2_FREE (gameData.hoard.orb.bm.bm_texBuf);
gameData.hoard.orb.bm.bm_texBuf = NULL;
D2_FREE (gameData.hoard.goal.bm.bm_texBuf);
gameData.hoard.goal.bm.bm_texBuf = NULL;
bmP = &gameData.hoard.monsterball.bm;
bmP = BM_OVERRIDE (&gameData.hoard.monsterball.bm) ? BM_OVERRIDE (&gameData.hoard.monsterball.bm) : &gameData.hoard.monsterball.bm;
D2_FREE (bmP->bm_texBuf);
if (BM_OVERRIDE (&gameData.hoard.monsterball.bm))
	D2_FREE (bmP);
memset (&gameData.hoard.orb.bm, 0, sizeof (grsBitmap));
for (i = 0; i < 2; i++) {
	D2_FREE (gameData.hoard.icon [i].bm.bm_texBuf);
	gameData.hoard.icon [i].bm.bm_texBuf = NULL;
	}
for (i = 0; i < 4; i++) {
	D2_FREE (gameData.pig.sound.sounds [0][gameData.pig.sound.nSoundFiles [0] + i].data [0]);
	}
}
//-----------------------------------------------------------------------------
//eof

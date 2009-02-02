/* $Id: bm.c,v 1.41 2003/11/03 12:03:43 btb Exp $ */
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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "descent.h"
#include "pstypes.h"
#include "strutil.h"
#include "text.h"
#include "gr.h"
#include "ogl_defs.h"
#include "loadgamedata.h"
#include "u_mem.h"
#include "mono.h"
#include "error.h"
#include "object.h"
#include "vclip.h"
#include "effects.h"
#include "polymodel.h"
#include "wall.h"
#include "textures.h"
#include "game.h"
#include "multi.h"
#include "iff.h"
#include "cfile.h"
#include "powerup.h"
#include "sounds.h"
#include "piggy.h"
#include "aistruct.h"
#include "robot.h"
#include "weapon.h"
#include "cockpit.h"
#include "player.h"
#include "endlevel.h"
#include "reactor.h"
#include "makesig.h"
#include "interp.h"
#include "light.h"
#include "byteswap.h"

ubyte Sounds [2][MAX_SOUNDS];
ubyte AltSounds [2][MAX_SOUNDS];

//right now there's only one CPlayerData ship, but we can have another by
//adding an array and setting the pointer to the active ship.

//---------------- Variables for CObject textures ----------------

#if 0//def FAST_FILE_IO /*disabled for a reason!*/
#define ReadTMapInfoN(ti, n, fp) cf.Read (ti, sizeof (tTexMapInfo), n, fp)
#else
/*
 * reads n tTexMapInfo structs from a CFile
 */
int ReadTMapInfoN (CArray<tTexMapInfo>& ti, int n, CFile& cf)
{
	int i;

for (i = 0;i < n;i++) {
	ti [i].flags = cf.ReadByte ();
	ti [i].pad [0] = cf.ReadByte ();
	ti [i].pad [1] = cf.ReadByte ();
	ti [i].pad [2] = cf.ReadByte ();
	ti [i].lighting = cf.ReadFix ();
	ti [i].damage = cf.ReadFix ();
	ti [i].nEffectClip = cf.ReadShort ();
	ti [i].destroyed = cf.ReadShort ();
	ti [i].slide_u = cf.ReadShort ();
	ti [i].slide_v = cf.ReadShort ();
	}
return i;
}
#endif

//------------------------------------------------------------------------------

int ReadTMapInfoND1 (tTexMapInfo *ti, int n, CFile& cf)
{
	int i;

for (i = 0;i < n;i++) {
	cf.Seek (13, SEEK_CUR);// skip filename
	ti [i].flags = cf.ReadByte ();
	ti [i].lighting = cf.ReadFix ();
	ti [i].damage = cf.ReadFix ();
	ti [i].nEffectClip = cf.ReadInt ();
	}
return i;
}

//-----------------------------------------------------------------
// Read data from piggy.
// This is called when the editor is OUT.
// If editor is in, bm_init_use_table () is called.
int BMInit (void)
{
if (!PiggyInit ())				// This calls BMReadAll
	Error ("Cannot open pig and/or ham file");
/*---*/PrintLog ("   Loading sound data\n");
PiggyReadSounds ();
/*---*/PrintLog ("   Initializing endlevel data\n");
InitEndLevel ();		//this is in bm_init_use_tbl (), so I gues it goes here
return 0;
}

//------------------------------------------------------------------------------

void BMSetAfterburnerSizes (void)
{
	sbyte	nSize = gameData.weapons.info [MERCURYMSL_ID].nAfterburnerSize;

//gameData.weapons.info [VULCAN_ID].nAfterburnerSize = 
//gameData.weapons.info [GAUSS_ID].nAfterburnerSize = nSize / 8;
gameData.weapons.info [CONCUSSION_ID].nAfterburnerSize =
gameData.weapons.info [HOMINGMSL_ID].nAfterburnerSize =
gameData.weapons.info [ROBOT_CONCUSSION_ID].nAfterburnerSize =
gameData.weapons.info [FLASHMSL_ID].nAfterburnerSize =
gameData.weapons.info [GUIDEDMSL_ID].nAfterburnerSize =
gameData.weapons.info [ROBOT_FLASHMSL_ID].nAfterburnerSize =
gameData.weapons.info [ROBOT_MEGA_FLASHMSL_ID].nAfterburnerSize =
gameData.weapons.info [ROBOT_MERCURYMSL_ID].nAfterburnerSize = nSize;
gameData.weapons.info [ROBOT_HOMINGMSL_ID].nAfterburnerSize =
gameData.weapons.info [SMARTMSL_ID].nAfterburnerSize = 2 * nSize;
gameData.weapons.info [MEGAMSL_ID].nAfterburnerSize =
gameData.weapons.info [ROBOT_MEGAMSL_ID].nAfterburnerSize =
gameData.weapons.info [ROBOT_SHAKER_MEGA_ID].nAfterburnerSize =
gameData.weapons.info [EARTHSHAKER_MEGA_ID].nAfterburnerSize = 3 * nSize;
gameData.weapons.info [EARTHSHAKER_ID].nAfterburnerSize =
gameData.weapons.info [ROBOT_EARTHSHAKER_ID].nAfterburnerSize = 4 * nSize;
}

//------------------------------------------------------------------------------

void QSortTextureIndex (short *pti, short left, short right)
{
	short	l = left,
			r = right,
			m = pti [(left + right) / 2],
			h;

do {
	while (pti [l] < m)
		l++;
	while (pti [r] > m)
		r--;
	if (l <= r) {
		if (l < r) {
			h = pti [l];
			pti [l] = pti [r];
			pti [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	QSortTextureIndex (pti, l, right);
if (left < r)
	QSortTextureIndex (pti, left, r);
}

//------------------------------------------------------------------------------

void BuildTextureIndex (int i, int n)
{
	short				*pti = gameData.pig.tex.textureIndex [i].Buffer ();
	tBitmapIndex	*pbi = gameData.pig.tex.bmIndex [i].Buffer ();

gameData.pig.tex.textureIndex [i].Clear (0xff);
for (i = 0; i < n; i++)
	pti [pbi [i].index] = i;
//QSortTextureIndex (pti, 0, n - 1);
}

//------------------------------------------------------------------------------

void BMReadAll (CFile& cf)
{
	int i, t;

gameData.pig.tex.nTextures [0] = cf.ReadInt ();
/*---*/PrintLog ("      Loading %d texture indices\n", gameData.pig.tex.nTextures [0]);
ReadBitmapIndices (gameData.pig.tex.bmIndex [0], gameData.pig.tex.nTextures [0], cf);
BuildTextureIndex (0, gameData.pig.tex.nTextures [0]);
ReadTMapInfoN (gameData.pig.tex.tMapInfo [0], gameData.pig.tex.nTextures [0], cf);

t = cf.ReadInt ();
/*---*/PrintLog ("      Loading %d sound indices\n", t);
cf.Read (Sounds [0], sizeof (ubyte), t);
cf.Read (AltSounds [0], sizeof (ubyte), t);

gameData.eff.nClips [0] = cf.ReadInt ();
/*---*/PrintLog ("      Loading %d animation clips\n", gameData.eff.nClips [0]);
ReadVideoClips (gameData.eff.vClips [0], gameData.eff.nClips [0], cf);

gameData.eff.nEffects [0] = cf.ReadInt ();
/*---*/PrintLog ("      Loading %d animation descriptions\n", gameData.eff.nEffects [0]);
ReadEffectClips (gameData.eff.effects [0], gameData.eff.nEffects [0], cf);
// red glow texture animates way too fast
gameData.eff.effects [0][32].vc.xTotalTime *= 10;
gameData.eff.effects [0][32].vc.xFrameTime *= 10;
gameData.walls.nAnims [0] = cf.ReadInt ();
/*---*/PrintLog ("      Loading %d CWall animations\n", gameData.walls.nAnims [0]);
ReadWallClips (gameData.walls.anims [0], gameData.walls.nAnims [0], cf);

gameData.bots.nTypes [0] = cf.ReadInt ();
/*---*/PrintLog ("      Loading %d robot descriptions\n", gameData.bots.nTypes [0]);
ReadRobotInfos (gameData.bots.info [0], gameData.bots.nTypes [0], cf);
gameData.bots.nDefaultTypes = gameData.bots.nTypes [0];
gameData.bots.defaultInfo = gameData.bots.info [0];

gameData.bots.nJoints = cf.ReadInt ();
/*---*/PrintLog ("      Loading %d robot joint descriptions\n", gameData.bots.nJoints);
ReadJointPositions (gameData.bots.joints, gameData.bots.nJoints, cf);
gameData.bots.nDefaultJoints = gameData.bots.nJoints;
gameData.bots.defaultJoints = gameData.bots.joints;

gameData.weapons.nTypes [0] = cf.ReadInt ();
/*---*/PrintLog ("      Loading %d weapon descriptions\n", gameData.weapons.nTypes [0]);
ReadWeaponInfos (0, gameData.weapons.nTypes [0], cf, gameData.pig.tex.nHamFileVersion);
BMSetAfterburnerSizes ();

gameData.objs.pwrUp.nTypes = cf.ReadInt ();
/*---*/PrintLog ("      Loading %d powerup descriptions\n", gameData.objs.pwrUp.nTypes);
ReadPowerupTypeInfos (gameData.objs.pwrUp.info.Buffer (), gameData.objs.pwrUp.nTypes, cf);

gameData.models.nPolyModels = cf.ReadInt ();
/*---*/PrintLog ("      Loading %d CPolyModel descriptions\n", gameData.models.nPolyModels);
ReadPolyModels (gameData.models.polyModels [0].Buffer (), gameData.models.nPolyModels, cf);
gameData.models.nDefPolyModels = gameData.models.nPolyModels;
memcpy (gameData.models.polyModels [1].Buffer (), gameData.models.polyModels [0].Buffer (), gameData.models.nPolyModels * sizeof (CPolyModel));

/*---*/PrintLog ("      Loading poly model data\n");
for (i = 0; i < gameData.models.nPolyModels; i++) {
	gameData.models.polyModels [0][i].SetBuffer (NULL);
	gameData.models.polyModels [1][i].SetBuffer (NULL);
	gameData.models.polyModels [0][i].ReadData (gameData.models.polyModels [1] + i, cf);
	}

for (i = 0; i < gameData.models.nPolyModels; i++)
	gameData.models.nDyingModels [i] = cf.ReadInt ();
for (i = 0; i < gameData.models.nPolyModels; i++)
	gameData.models.nDeadModels [i] = cf.ReadInt ();

t = cf.ReadInt ();
/*---*/PrintLog ("      Loading %d cockpit gauges\n", t);
ReadBitmapIndices (gameData.cockpit.gauges [1], t, cf);
ReadBitmapIndices (gameData.cockpit.gauges [0], t, cf);

gameData.pig.tex.nObjBitmaps = cf.ReadInt ();
/*---*/PrintLog ("      Loading %d CObject bitmap indices\n", gameData.pig.tex.nObjBitmaps);
ReadBitmapIndices (gameData.pig.tex.objBmIndex, gameData.pig.tex.nObjBitmaps, cf);
for (i = 0; i < gameData.pig.tex.nObjBitmaps; i++)
	gameData.pig.tex.objBmIndexP [i] = cf.ReadShort ();

/*---*/PrintLog ("      Loading CPlayerData ship description\n");
PlayerShipRead (&gameData.pig.ship.only, cf);

gameData.models.nCockpits = cf.ReadInt ();
/*---*/PrintLog ("      Loading %d cockpit bitmaps\n", gameData.models.nCockpits);
ReadBitmapIndices (gameData.pig.tex.cockpitBmIndex, gameData.models.nCockpits, cf);
gameData.pig.tex.nFirstMultiBitmap = cf.ReadInt ();

gameData.reactor.nReactors = cf.ReadInt ();
/*---*/PrintLog ("      Loading %d reactor descriptions\n", gameData.reactor.nReactors);
ReadReactors (cf);

gameData.models.nMarkerModel = cf.ReadInt ();
if (gameData.pig.tex.nHamFileVersion < 3) {
	gameData.endLevel.exit.nModel = cf.ReadInt ();
	gameData.endLevel.exit.nDestroyedModel = cf.ReadInt ();
	}
else
	gameData.endLevel.exit.nModel = 
	gameData.endLevel.exit.nDestroyedModel = gameData.models.nPolyModels;
}

//------------------------------------------------------------------------------

void BMReadWeaponInfoD1N (CFile& cf, int i)
{
	CD1WeaponInfo* wiP = gameData.weapons.infoD1 + i;

wiP->renderType = cf.ReadByte ();
wiP->nModel = cf.ReadByte ();
wiP->nInnerModel = cf.ReadByte ();
wiP->persistent = cf.ReadByte ();
wiP->nFlashVClip = cf.ReadByte ();
wiP->flashSound = cf.ReadShort ();
wiP->nRobotHitVClip = cf.ReadByte ();
wiP->nRobotHitSound = cf.ReadShort ();
wiP->nWallHitVClip = cf.ReadByte ();
wiP->nWallHitSound = cf.ReadShort ();
wiP->fireCount = cf.ReadByte ();
wiP->nAmmoUsage = cf.ReadByte ();
wiP->nVClipIndex = cf.ReadByte ();
wiP->destructible = cf.ReadByte ();
wiP->matter = cf.ReadByte ();
wiP->bounce = cf.ReadByte ();
wiP->homingFlag = cf.ReadByte ();
wiP->dum1 = cf.ReadByte (); 
wiP->dum2 = cf.ReadByte ();
wiP->dum3 = cf.ReadByte ();
wiP->xEnergyUsage = cf.ReadFix ();
wiP->xFireWait = cf.ReadFix ();
wiP->bitmap.index = cf.ReadShort ();
wiP->blob_size = cf.ReadFix ();
wiP->xFlashSize = cf.ReadFix ();
wiP->xImpactSize = cf.ReadFix ();
for (i = 0; i < NDL; i++)
	wiP->strength [i] = cf.ReadFix ();
for (i = 0; i < NDL; i++)
	wiP->speed [i] = cf.ReadFix ();
wiP->mass = cf.ReadFix ();
wiP->drag = cf.ReadFix ();
wiP->thrust = cf.ReadFix ();
wiP->poLenToWidthRatio = cf.ReadFix ();
wiP->light = cf.ReadFix ();
wiP->lifetime = cf.ReadFix ();
wiP->xDamageRadius = cf.ReadFix ();
wiP->picture.index = cf.ReadShort ();

}

//------------------------------------------------------------------------------

void LoadTextureBrightness (const char *pszLevel, int *brightnessP)
{
	CFile		cf;
	char		szFile [FILENAME_LEN];
	int		i, *pb;

if (!brightnessP)
	brightnessP = gameData.pig.tex.brightness.Buffer ();
CFile::ChangeFilenameExtension (szFile, pszLevel, ".lgt");
if (cf.Open (szFile, gameFolders.szDataDir, "rb", 0) &&
	 (cf.Read (brightnessP, sizeof (*brightnessP) * MAX_WALL_TEXTURES, 1) == 1)) {
	for (i = MAX_WALL_TEXTURES, pb = gameData.pig.tex.brightness.Buffer (); i; i--, pb++)
		*pb = INTEL_INT (*pb);
	cf.Close ();
	}
}

//------------------------------------------------------------------------------
//eof

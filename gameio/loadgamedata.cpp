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

#define PRINT_WEAPON_INFO	0

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
gameData.eff.effects [0][32].vClipInfo.xTotalTime *= 10;
gameData.eff.effects [0][32].vClipInfo.xFrameTime *= 10;
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
ReadPolyModels (gameData.models.polyModels [0], gameData.models.nPolyModels, cf);
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
gameData.pig.tex.defaultObjBmIndex = gameData.pig.tex.objBmIndex;
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

#if PRINT_WEAPON_INFO
PrintLog ("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,{",
	wiP->renderType,
	wiP->nModel,
	wiP->nInnerModel,
	wiP->persistent,
	wiP->nFlashVClip,
	wiP->flashSound,
	wiP->nRobotHitVClip,
	wiP->nRobotHitSound,
	wiP->nWallHitVClip,
	wiP->nWallHitSound,
	wiP->fireCount,
	wiP->nAmmoUsage,
	wiP->nVClipIndex,
	wiP->destructible,
	wiP->matter,
	wiP->bounce,
	wiP->homingFlag,
	wiP->dum1, 
	wiP->dum2,
	wiP->dum3,
	wiP->xEnergyUsage,
	wiP->xFireWait,
	wiP->bitmap.index,
	wiP->blob_size,
	wiP->xFlashSize,
	wiP->xImpactSize);
for (i = 0; i < NDL; i++)
	PrintLog ("%s%d", i ? "," : "", wiP->strength [i]);
PrintLog ("},{");
for (i = 0; i < NDL; i++)
	PrintLog ("%s%d", i ? "," : "", wiP->speed [i]);
PrintLog ("},%d,%d,%d,%d,%d,%d,%d,{%d}},\n",
	wiP->mass,
	wiP->drag,
	wiP->thrust,
	wiP->poLenToWidthRatio,
	wiP->light,
	wiP->lifetime,
	wiP->xDamageRadius,
	wiP->picture.index);
#endif
}

//------------------------------------------------------------------------------
// the following values are needed for compiler settings causing struct members 
// not to be byte aligned. If D2X-XL is compiled with such settings, the size of
// the Descent data structures in memory is bigger than on disk. This needs to
// be compensated when reading such data structures from disk, or needing to skip
// them in the disk files.

#define D1_ROBOT_INFO_SIZE			486
#define D1_WEAPON_INFO_SIZE		115
#define JOINTPOS_SIZE				8
#define JOINTLIST_SIZE				4
#define POWERUP_TYPE_INFO_SIZE	16
#define POLYMODEL_SIZE				734
#define PLAYER_SHIP_SIZE			132
#define MODEL_DATA_SIZE_OFFS		4


typedef struct tD1TextureHeader {
	char	name [8];
	ubyte frame; //bits 0-5 anim frame num, bit 6 abm flag
	ubyte xsize; //low 8 bits here, 4 more bits in size2
	ubyte ysize; //low 8 bits here, 4 more bits in size2
	ubyte flag; //see BM_FLAG_XXX
	ubyte ave_color; //palette index of average color
	uint	offset; //relative to end of directory
} __pack__ tD1TextureHeader;

typedef struct tD1SoundHeader {
	char name [8];
	int length; //size in bytes
	int data_length; //actually the same as above
	int offset; //relative to end of directory
} __pack__ tD1SoundHeader;


void BMReadGameDataD1 (CFile& cf)
{
	int				h, i, j, v10DataOffset;
#if 1
	tD1WallClip		w;
	D1_tmap_info	t;
	//D1Robot_info	r;
#endif
	tWallClip		*pw;	
	tTexMapInfo		*pt;
	tRobotInfo		*pr;
	CPolyModel		model;
	ubyte				tmpSounds [D1_MAX_SOUNDS];

v10DataOffset = cf.ReadInt ();
cf.Read (&gameData.pig.tex.nTextures [1], sizeof (int), 1);
j = (gameData.pig.tex.nTextures [1] == 70) ? 70 : D1_MAX_TEXTURES;
/*---*/PrintLog ("         Loading %d texture indices\n", j);
//cf.Read (gameData.pig.tex.bmIndex [1], sizeof (tBitmapIndex), D1_MAX_TEXTURES);
ReadBitmapIndices (gameData.pig.tex.bmIndex [1], D1_MAX_TEXTURES, cf);
BuildTextureIndex (1, D1_MAX_TEXTURES);
/*---*/PrintLog ("         Loading %d texture descriptions\n", j);
for (i = 0, pt = &gameData.pig.tex.tMapInfo [1][0]; i < j; i++, pt++) {
#if DBG
	cf.Read (t.filename, sizeof (t.filename), 1);
#else
	cf.Seek (sizeof (t.filename), SEEK_CUR);
#endif
	pt->flags = (ubyte) cf.ReadByte ();
	pt->lighting = cf.ReadFix ();
	pt->damage = cf.ReadFix ();
	pt->nEffectClip = cf.ReadInt ();
	pt->slide_u = 
	pt->slide_v = 0;
	pt->destroyed = -1;
	}
cf.Read (Sounds [1], sizeof (ubyte), D1_MAX_SOUNDS);
cf.Read (AltSounds [1], sizeof (ubyte), D1_MAX_SOUNDS);
/*---*/PrintLog ("         Initializing %d sounds\n", D1_MAX_SOUNDS);
if (gameOpts->sound.bUseD1Sounds) {
	memcpy (Sounds [1] + D1_MAX_SOUNDS, Sounds [0] + D1_MAX_SOUNDS, MAX_SOUNDS - D1_MAX_SOUNDS);
	memcpy (AltSounds [1] + D1_MAX_SOUNDS, AltSounds [0] + D1_MAX_SOUNDS, MAX_SOUNDS - D1_MAX_SOUNDS);
	}
else {
	memcpy (Sounds [1], Sounds [0], MAX_SOUNDS);
	memcpy (AltSounds [1], AltSounds [0], MAX_SOUNDS);
	}
for (i = 0; i < D1_MAX_SOUNDS; i++) {
	if (Sounds [1][i] == 255)
		Sounds [1][i] = Sounds [0][i];
	if (AltSounds [1][i] == 255)
		AltSounds [1][i] = AltSounds [0][i];
	}
gameData.eff.nClips [1] = cf.ReadInt ();
/*---*/PrintLog ("         Loading %d animation clips\n", gameData.eff.nClips [1]);
ReadVideoClips (gameData.eff.vClips [1], D1_VCLIP_MAXNUM, cf);
gameData.eff.nEffects [1] = cf.ReadInt ();
/*---*/PrintLog ("         Loading %d animation descriptions\n", gameData.eff.nClips [1]);
ReadEffectClips (gameData.eff.effects [1], D1_MAX_EFFECTS, cf);
gameData.walls.nAnims [1] = cf.ReadInt ();
/*---*/PrintLog ("         Loading %d CWall animations\n", gameData.walls.nAnims [1]);
for (i = 0, pw = &gameData.walls.anims [1][0]; i < D1_MAX_WALL_ANIMS; i++, pw++) {
	//cf.Read (&w, sizeof (w), 1);
	pw->xTotalTime = cf.ReadFix ();
	pw->nFrameCount = cf.ReadShort ();
	for (j = 0; j < D1_MAX_CLIP_FRAMES; j++)
		pw->frames [j] = cf.ReadShort ();
	pw->openSound = cf.ReadShort ();
	pw->closeSound = cf.ReadShort ();
	pw->flags = cf.ReadShort ();
	cf.Read (pw->filename, sizeof (w.filename), 1);
	pw->pad = (char) cf.ReadByte ();
	}
cf.Read (&gameData.bots.nTypes [1], sizeof (int), 1);
/*---*/PrintLog ("         Loading %d robot descriptions\n", gameData.bots.nTypes [1]);
gameData.bots.info [1] = gameData.bots.info [0];
if (!gameOpts->sound.bUseD1Sounds)
	return;

for (i = 0, pr = &gameData.bots.info [1][0]; i < D1_MAX_ROBOT_TYPES; i++, pr++) {
	//cf.Read (&r, sizeof (r), 1);
	cf.Seek (
		sizeof (int) * 3 + 
		(sizeof (CFixVector) + sizeof (ubyte)) * MAX_GUNS + 
		sizeof (short) * 5 +
		sizeof (sbyte) * 7 +
		sizeof (fix) * 4 +
		sizeof (fix) * 7 * NDL +
		sizeof (sbyte) * 2 * NDL,
		SEEK_CUR);

	pr->seeSound = (ubyte) cf.ReadByte ();
	pr->attackSound = (ubyte) cf.ReadByte ();
	pr->clawSound = (ubyte) cf.ReadByte ();
	cf.Seek (
		JOINTLIST_SIZE * (MAX_GUNS + 1) * N_ANIM_STATES +
		sizeof (int),
		SEEK_CUR);
	pr->always_0xabcd = 0xabcd;   
	}         

cf.Seek (
	sizeof (int) +
	JOINTPOS_SIZE * D1_MAX_ROBOT_JOINTS +
	sizeof (int) +
	D1_WEAPON_INFO_SIZE * D1_MAX_WEAPON_TYPES +
	sizeof (int) +
	POWERUP_TYPE_INFO_SIZE * MAX_POWERUP_TYPES_D1,
	SEEK_CUR);
i = cf.ReadInt ();
/*---*/PrintLog ("         Acquiring model data size of %d polymodels\n", i);
for (h = 0; i; i--) {
	cf.Seek (MODEL_DATA_SIZE_OFFS, SEEK_CUR);
	model.SetDataSize (cf.ReadInt ());
	h += model.DataSize ();
	cf.Seek (POLYMODEL_SIZE - MODEL_DATA_SIZE_OFFS - sizeof (int), SEEK_CUR);
	}
cf.Seek (
	h +
	sizeof (tBitmapIndex) * D1_MAX_GAUGE_BMS +
	sizeof (int) * 2 * D1_MAX_POLYGON_MODELS +
	sizeof (tBitmapIndex) * D1_MAX_OBJ_BITMAPS +
	sizeof (ushort) * D1_MAX_OBJ_BITMAPS +
	PLAYER_SHIP_SIZE +
	sizeof (int) +
	sizeof (tBitmapIndex) * D1_N_COCKPIT_BITMAPS,
	SEEK_CUR);
/*---*/PrintLog ("         Loading sound data\n", i);
cf.Read (tmpSounds, sizeof (ubyte), D1_MAX_SOUNDS);
//for (i = 0, pr = &gameData.bots.info [1][0]; i < gameData.bots.nTypes [1]; i++, pr++) 
pr = gameData.bots.info [1] + 17;
/*---*/PrintLog ("         Initializing sound data\n", i);
for (i = 0; i < D1_MAX_SOUNDS; i++) {
	if (Sounds [1][i] == tmpSounds [pr->seeSound])
		pr->seeSound = i;
	if (Sounds [1][i] == tmpSounds [pr->attackSound])
		pr->attackSound = i;
	if (Sounds [1][i] == tmpSounds [pr->clawSound])
		pr->clawSound = i;
	}
pr = gameData.bots.info [1] + 23;
for (i = 0; i < D1_MAX_SOUNDS; i++) {
	if (Sounds [1][i] == tmpSounds [pr->seeSound])
		pr->seeSound = i;
	if (Sounds [1][i] == tmpSounds [pr->attackSound])
		pr->attackSound = i;
	if (Sounds [1][i] == tmpSounds [pr->clawSound])
		pr->clawSound = i;
	}
cf.Read (tmpSounds, sizeof (ubyte), D1_MAX_SOUNDS);
//	for (i = 0, pr = &gameData.bots.info [1][0]; i < gameData.bots.nTypes [1]; i++, pr++) {
pr = gameData.bots.info [1] + 17;
for (i = 0; i < D1_MAX_SOUNDS; i++) {
	if (AltSounds [1][i] == tmpSounds [pr->seeSound])
		pr->seeSound = i;
	if (AltSounds [1][i] == tmpSounds [pr->attackSound])
		pr->attackSound = i;
	if (AltSounds [1][i] == tmpSounds [pr->clawSound])
		pr->clawSound = i;
	}
pr = gameData.bots.info [1] + 23;
for (i = 0; i < D1_MAX_SOUNDS; i++) {
	if (AltSounds [1][i] == tmpSounds [pr->seeSound])
		pr->seeSound = i;
	if (AltSounds [1][i] == tmpSounds [pr->attackSound])
		pr->attackSound = i;
	if (AltSounds [1][i] == tmpSounds [pr->clawSound])
		pr->clawSound = i;
	}
#if 0
cf.Seek (v10DataOffset, SEEK_SET);
i = cf.ReadInt ();
j = cf.ReadInt ();
cf.Seek (i * sizeof (tD1TextureHeader), SEEK_CUR);
gameStates.app.bD1Mission = 1;
for (i = 0; i < j; i++) {
	cf.Read (&gameData.pig.sound.sounds [1][i].szName, sizeof (gameData.pig.sound.sounds [1][i].szName), 1);
	cf.Seek (sizeof (tD1SoundHeader) - sizeof (gameData.pig.sound.sounds [1][i].szName), SEEK_CUR);
	}
#endif
}

//------------------------------------------------------------------------------

void BMReadWeaponInfoD1 (CFile& cf)
{
cf.Seek (
	sizeof (int) +
	sizeof (int) +
	sizeof (tBitmapIndex) * D1_MAX_TEXTURES +
	sizeof (D1_tmap_info) * D1_MAX_TEXTURES +
	sizeof (ubyte) * D1_MAX_SOUNDS +
	sizeof (ubyte) * D1_MAX_SOUNDS +
	sizeof (int) +
	sizeof (tVideoClip) * D1_VCLIP_MAXNUM +
	sizeof (int) +
	sizeof (D1_eclip) * D1_MAX_EFFECTS +
	sizeof (int) +
	sizeof (tD1WallClip) * D1_MAX_WALL_ANIMS +
	sizeof (int) +
	sizeof (D1Robot_info) * D1_MAX_ROBOT_TYPES +
	sizeof (int) +
	sizeof (tJointPos) * D1_MAX_ROBOT_JOINTS,
	SEEK_CUR);
gameData.weapons.nTypes [1] = cf.ReadInt ();
#if PRINT_WEAPON_INFO
PrintLog ("\nCD1WeaponInfo defaultWeaponInfosD1 [] = {\n");
#endif
for (int i = 0; i < gameData.weapons.nTypes [1]; i++)
	BMReadWeaponInfoD1N (cf, i);
#if PRINT_WEAPON_INFO
PrintLog ("};\n\n");
#endif
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

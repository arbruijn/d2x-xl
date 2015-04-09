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

//------------------------------------------------------------------------------
//these values are the number of each item in the release of d2
//extra items added after the release get written in an additional hamfile
#define N_D2_ROBOT_TYPES			66
#define N_D2_ROBOT_JOINTS			1145
#define N_D2_POLYGON_MODELS		166
#define N_D2_OBJBITMAPS				422
#define N_D2_OBJBITMAPPTRS			502
#define N_D2_WEAPON_TYPES			62

#define N_VERTIGO_ROBOT_TYPES		12
#define N_VERTIGO_POLYGON_MODELS	13

void _CDECL_ FreeObjExtensionBitmaps (void)
{
	int32_t		i;
	CBitmap*	bmP;

PrintLog (1, "unloading extra bitmaps\n");
if (!gameData.pig.tex.nExtraBitmaps)
	gameData.pig.tex.nExtraBitmaps = gameData.pig.tex.nBitmaps [0];
for (i = gameData.pig.tex.nBitmaps [0], bmP = gameData.pig.tex.bitmaps [0] + i; 
	  i < gameData.pig.tex.nExtraBitmaps; i++, bmP++) {
	gameData.pig.tex.nObjBitmaps--;
	bmP->ReleaseTexture ();
	if (bmP->Buffer ()) {
		bmP->DestroyBuffer ();
		UseBitmapCache (bmP, (int32_t) -bmP->Height () * (int32_t) bmP->RowSize ());
		}
	}
gameData.pig.tex.nExtraBitmaps = gameData.pig.tex.nBitmaps [0];
PrintLog (-1);
}

//------------------------------------------------------------------------------

void FreeModelExtensions (void)
{
	//return;
PrintLog (1, "unloading extra poly models\n");
while (gameData.models.nPolyModels > N_D2_POLYGON_MODELS) {
	gameData.models.polyModels [0][--gameData.models.nPolyModels].Destroy ();
	gameData.models.polyModels [1][gameData.models.nPolyModels].Destroy ();
	}
if (!gameStates.app.bDemoData)
	while (gameData.models.nPolyModels > gameData.endLevel.exit.nModel) {
		gameData.models.polyModels [0][--gameData.models.nPolyModels].Destroy ();
		gameData.models.polyModels [1][gameData.models.nPolyModels].Destroy ();
		}
PrintLog (-1);
}

//------------------------------------------------------------------------------

//nType==1 means 1.1, nType==2 means 1.2 (with weapons)
int32_t LoadRobotExtensions (const char *fname, char *folder, int32_t nType)
{
	CFile cf;
	int32_t t,i,j;
	int32_t bVertigoData;

	//strlwr (fname);
bVertigoData = !strcmp (fname, "d2x.ham");
if (!cf.Open (fname, folder, "rb", 0))
	return 0;

FreeModelExtensions ();
FreeObjExtensionBitmaps ();

if (nType > 1) {
	int32_t sig;

	sig = cf.ReadInt ();
	if (sig != MAKE_SIG ('X','H','A','M'))
		return 0;
	cf.ReadInt ();
}

//read extra weapons

t = cf.ReadInt ();
gameData.weapons.nTypes [0] = N_D2_WEAPON_TYPES+t;
if (gameData.weapons.nTypes [0] >= MAX_WEAPON_TYPES) {
	Warning ("Too many weapons (%d) in <%s>.  Max is %d.", t, fname, MAX_WEAPON_TYPES - N_D2_WEAPON_TYPES);
	return -1;
	}
ReadWeaponInfos (N_D2_WEAPON_TYPES, t, cf, 3, bVertigoData != 0);

//now read robot info

t = cf.ReadInt ();
gameData.botData.nTypes [0] = N_D2_ROBOT_TYPES + t;
if (gameData.botData.nTypes [0] >= MAX_ROBOT_TYPES) {
	Warning ("Too many robots (%d) in <%s>.  Max is %d.", t, fname, MAX_ROBOT_TYPES - N_D2_ROBOT_TYPES);
	return -1;
	}
ReadRobotInfos (gameData.botData.info [0], t, cf, N_D2_ROBOT_TYPES);
if (bVertigoData) {
	gameData.botData.nDefaultTypes = gameData.botData.nTypes [0];
	memcpy (&gameData.botData.defaultInfo [N_D2_ROBOT_TYPES], &gameData.botData.info [0][N_D2_ROBOT_TYPES], sizeof (gameData.botData.info [0][0]) * t);
	}

t = cf.ReadInt ();
gameData.botData.nJoints = N_D2_ROBOT_JOINTS + t;
if (gameData.botData.nJoints >= MAX_ROBOT_JOINTS) {
	Warning ("Too many robot joints (%d) in <%s>.  Max is %d.", t, fname, MAX_ROBOT_JOINTS - N_D2_ROBOT_JOINTS);
	return -1;
	}
ReadJointPositions (gameData.botData.joints, t, cf, N_D2_ROBOT_JOINTS);
if (bVertigoData) {
	gameData.botData.nDefaultJoints = gameData.botData.nJoints;
	memcpy (&gameData.botData.defaultJoints [N_D2_ROBOT_TYPES], &gameData.botData.joints [N_D2_ROBOT_TYPES], sizeof (gameData.botData.joints [0]) * t);
	}

t = cf.ReadInt ();
j = N_D2_POLYGON_MODELS; //gameData.models.nPolyModels;
gameData.models.nPolyModels += t;
if (gameData.models.nPolyModels >= MAX_POLYGON_MODELS) {
	Warning ("Too many polygon models (%d)\nin <%s>.\nMax is %d.",
				t,fname, MAX_POLYGON_MODELS - N_D2_POLYGON_MODELS);
	return -1;
	}
ReadPolyModels (gameData.models.polyModels [0], t, cf, j);
if (bVertigoData) {
	gameData.models.nDefPolyModels = gameData.models.nPolyModels;
	memcpy (gameData.models.polyModels [1] + j, gameData.models.polyModels [0] + j, sizeof (CPolyModel) * t);
	}
for (i = j; i < gameData.models.nPolyModels; i++) {
	gameData.models.polyModels [1][i].ResetBuffer ();
	gameData.models.polyModels [0][i].Destroy ();
	gameData.models.polyModels [0][i].ReadData (bVertigoData ? gameData.models.polyModels [1] + i : NULL, cf);
	}
for (i = j; i < gameData.models.nPolyModels; i++)
	gameData.models.nDyingModels [i] = cf.ReadInt ();
for (i = j; i < gameData.models.nPolyModels; i++)
	gameData.models.nDeadModels [i] = cf.ReadInt ();

t = cf.ReadInt ();
if (N_D2_OBJBITMAPS + t >= MAX_OBJ_BITMAPS) {
	Warning ("Too many object bitmaps (%d) in <%s>.  Max is %d.", t, fname, MAX_OBJ_BITMAPS - N_D2_OBJBITMAPS);
	return -1;
	}
ReadBitmapIndices (gameData.pig.tex.objBmIndex, t, cf, N_D2_OBJBITMAPS);
if (bVertigoData) {
	memcpy (&gameData.pig.tex.defaultObjBmIndex [N_D2_OBJBITMAPS], &gameData.pig.tex.objBmIndex [N_D2_OBJBITMAPS], sizeof (gameData.pig.tex.objBmIndex [0]) * t);
	}

t = cf.ReadInt ();
if (N_D2_OBJBITMAPPTRS + t >= MAX_OBJ_BITMAPS) {
	Warning ("Too many object bitmap pointers (%d) in <%s>.  Max is %d.", t, fname, MAX_OBJ_BITMAPS - N_D2_OBJBITMAPPTRS);
	return -1;
	}
for (i = N_D2_OBJBITMAPPTRS; i < (N_D2_OBJBITMAPPTRS + t); i++)
	gameData.pig.tex.objBmIndexP [i] = cf.ReadShort ();
cf.Close ();
return 1;
}

//------------------------------------------------------------------------------

int32_t LoadRobotReplacements (const char* szLevel, const char* szFolder, int32_t bAddBots, int32_t bAltModels, bool bCustom, bool bUseHog)
{
	CFile			cf;
	CPolyModel*	modelP;
	int32_t			t, i, j;
	int32_t			nBotTypeSave = gameData.botData.nTypes [gameStates.app.bD1Mission], 
					nBotJointSave = gameData.botData.nJoints, 
					nPolyModelSave = gameData.models.nPolyModels;
	tRobotInfo	botInfoSave;
	char			szFile [FILENAME_LEN];

CFile::ChangeFilenameExtension (szFile, szLevel, ".hxm");
if (!cf.Open (szFile, szFolder ? szFolder : gameFolders.game.szData [0], "rb", bUseHog ? 0 : -1))		//no robot replacement file
	return 0;
t = cf.ReadInt ();			//read id "HXM!"
if (t != MAKE_SIG ('!','X','M','H')) {
	Warning (TXT_HXM_ID);
	cf.Close ();
	return 0;
	}
t = cf.ReadInt ();			//read version
if (t < 1) {
	Warning (TXT_HXM_VERSION, t);
	cf.Close ();
	return 0;
	}
t = cf.ReadInt ();			//read number of robots
for (j = 0; j < t; j++) {
	i = cf.ReadInt ();		//read robot number
	if (bAddBots) {
		if (gameData.botData.nTypes [gameStates.app.bD1Mission] >= MAX_ROBOT_TYPES) {
			Warning (TXT_ROBOT_NO, szLevel, i, MAX_ROBOT_TYPES);
			return -1;
			}
		i = gameData.botData.nTypes [gameStates.app.bD1Mission]++;
		}
	else if (i < 0 || i >= gameData.botData.nTypes [gameStates.app.bD1Mission]) {
		Warning (TXT_ROBOT_NO, szLevel, i, gameData.botData.nTypes [gameStates.app.bD1Mission] - 1);
		gameData.botData.nTypes [gameStates.app.bD1Mission] = nBotTypeSave;
		gameData.botData.nJoints = nBotJointSave;
		gameData.models.nPolyModels = nPolyModelSave;
		return -1;
		}
	if (bAltModels)
		cf.Seek (sizeof (tRobotInfo), SEEK_CUR);
	else {
		botInfoSave = gameData.botData.info [gameStates.app.bD1Mission][i];
		ReadRobotInfos (gameData.botData.info [gameStates.app.bD1Mission], 1, cf, i);
		}
	}
t = cf.ReadInt ();			//read number of joints
for (j = 0; j < t; j++) {
	i = cf.ReadInt ();		//read joint number
	if (bAddBots) {
		if (gameData.botData.nJoints < MAX_ROBOT_JOINTS) 
			i = gameData.botData.nJoints++;
		else {
			Warning ("%s: Robots joint (%d) out of range (valid range = 0 - %d).",
						szLevel, i, MAX_ROBOT_JOINTS - 1);
			gameData.botData.nTypes [gameStates.app.bD1Mission] = nBotTypeSave;
			gameData.botData.nJoints = nBotJointSave;
			gameData.models.nPolyModels = nPolyModelSave;
			return -1;
			}
		}
	else if ((i < 0) || (i >= gameData.botData.nJoints)) {
		Warning ("%s: Robots joint (%d) out of range (valid range = 0 - %d).",
					szLevel, i, gameData.botData.nJoints - 1);
		gameData.botData.nTypes [gameStates.app.bD1Mission] = nBotTypeSave;
		gameData.botData.nJoints = nBotJointSave;
		gameData.models.nPolyModels = nPolyModelSave;
		return -1;
		}
	if (bAltModels)
		cf.Seek (4 * sizeof (int16_t), SEEK_CUR);
	else
		ReadJointPositions (gameData.botData.joints, 1, cf, i);
	}
t = cf.ReadInt ();			//read number of polygon models
for (j = 0; j < t; j++) {
	i = cf.ReadInt ();		//read model number
	if (bAddBots) {
		if (gameData.models.nPolyModels < MAX_POLYGON_MODELS) 
			i = gameData.models.nPolyModels++;
		else {
			Warning ("%s: Polygon model (%d) out of range (valid range = 0 - %d).",
						szLevel, i, gameData.models.nPolyModels - 1);
			gameData.botData.nTypes [gameStates.app.bD1Mission] = nBotTypeSave;
			gameData.botData.nJoints = nBotJointSave;
			gameData.models.nPolyModels = nPolyModelSave;
			return -1;
			}
		}
	else if ((i < 0) || (i >= gameData.models.nPolyModels)) {
		if (bAltModels) {
			if (i < MAX_POLYGON_MODELS) 
				gameData.models.nPolyModels = i + 1;
			else {
				Warning ("%s: Polygon model (%d) out of range (valid range = 0 - %d).",
							szLevel, i, gameData.models.nPolyModels - 1);
				gameData.botData.nTypes [gameStates.app.bD1Mission] = nBotTypeSave;
				gameData.botData.nJoints = nBotJointSave;
				gameData.models.nPolyModels = nPolyModelSave;
				return -1;
				}
			}
		else if ((gameData.models.nPolyModels > N_D2_POLYGON_MODELS) || (i >= N_D2_POLYGON_MODELS + N_VERTIGO_POLYGON_MODELS)) {
			Warning ("%s: Polygon model (%d) out of range (valid range = 0 - %d).",
						szLevel, i, gameData.models.nPolyModels - 1);
			gameData.botData.nTypes [gameStates.app.bD1Mission] = nBotTypeSave;
			gameData.botData.nJoints = nBotJointSave;
			gameData.models.nPolyModels = nPolyModelSave;
			return -1;
			}
		}
#if DBG
	if (i == nDbgModel)
		BRP;
#endif
	modelP = &gameData.models.polyModels [bAltModels ? 2 : 0][i];
	modelP->Destroy ();
	if (!modelP->Read (0, 1, cf))
		return -1;
	modelP->ReadData (NULL, cf);
	modelP->SetType (bAltModels ? 1 : -1);
	modelP->SetRad (modelP->Size (), 1);
	modelP->SetCustom (bCustom);
	if (bAltModels) {
#	if DBG
		if (i == nDbgModel)
			BRP;
#endif
		cf.ReadInt ();
		cf.ReadInt ();
		}
	else {
		gameData.models.nDyingModels [i] = cf.ReadInt ();
		gameData.models.nDeadModels [i] = cf.ReadInt ();
		}
	}

t = cf.ReadInt ();			//read number of objbitmaps
for (j = 0; j < t; j++) {
	i = cf.ReadInt ();		//read objbitmap number
	if (!bAddBots && ((i < 0) || (i >= MAX_OBJ_BITMAPS))) {
		Warning ("%s: Object bitmap number (%d) out of range (valid range = 0 - %d).",
					szLevel, i, MAX_OBJ_BITMAPS - 1);
		gameData.botData.nTypes [gameStates.app.bD1Mission] = nBotTypeSave;
		gameData.botData.nJoints = nBotJointSave;
		gameData.models.nPolyModels = nPolyModelSave;
		return -1;
		}
	ReadBitmapIndex (gameData.pig.tex.objBmIndex + i, cf);
	}
t = cf.ReadInt ();			//read number of objbitmapptrs
for (j = 0; j < t; j++) {
	i = cf.ReadInt ();		//read objbitmapptr number
	if ((i < 0) || (i >= MAX_OBJ_BITMAPS)) {
		Warning ("%s: Object bitmap pointer (%d) out of range (valid range = 0 - %d).",
					szLevel, i, MAX_OBJ_BITMAPS - 1);
		gameData.botData.nTypes [gameStates.app.bD1Mission] = nBotTypeSave;
		gameData.botData.nJoints = nBotJointSave;
		gameData.models.nPolyModels = nPolyModelSave;
		return -1;
		}
	gameData.pig.tex.objBmIndexP [i] = cf.ReadShort ();
	}
cf.Close ();
return 1;
}

//------------------------------------------------------------------------------
/*
 * Routines for loading exit models
 *
 * Used by d1 levels (including some add-ons), and by d2 shareware.
 * Could potentially be used by d2 add-on levels, but only if they
 * don't use "extra" robots...
 */

static tBitmapIndex LoadExitModelIFF (const char * filename)
{
	tBitmapIndex	bmi;
	CBitmap*			bmP = gameData.pig.tex.bitmaps [0] + gameData.pig.tex.nExtraBitmaps;
	int32_t				iffError;		//reference parm to avoid warning message
	CIFF				iff;

bmi.index = 0;
iffError = iff.ReadBitmap (filename, bmP, BM_LINEAR);
if (iffError != IFF_NO_ERROR)	 {
#if TRACE
	console.printf (CON_DBG, 
		"Error loading exit model bitmap <%s> - IFF error: %s\n", 
		filename, iff.ErrorMsg (iffError));
#endif		
	return bmi;
	}
if (iff.HasTransparency ())
	bmP->SetPalette (NULL, iff.TransparentColor (), 254);
else
	bmP->SetPalette (NULL, -1, 254);
bmP->AvgColorIndex ();
bmi.index = gameData.pig.tex.nExtraBitmaps;
gameData.pig.tex.bitmapP [gameData.pig.tex.nExtraBitmaps++] = *bmP;
return bmi;
}

//------------------------------------------------------------------------------

static CBitmap *LoadExitModelBitmap (const char *name)
{
	int32_t				i;
	tBitmapIndex*	bip = gameData.pig.tex.objBmIndex + gameData.pig.tex.nObjBitmaps;
	Assert (gameData.pig.tex.nObjBitmaps < MAX_OBJ_BITMAPS);

*bip = LoadExitModelIFF (name);
if (!bip->index) {
	char *name2 = StrDup (name);
	*strrchr (name2, '.') = '\0';
	*bip = ReadExtraBitmapD1Pig (name2);
	delete[] name2;
	}
if (!(i = bip->index))
	return NULL;
//if (gameData.pig.tex.bitmaps [0][i].Width () != 64 || gameData.pig.tex.bitmaps [0][i].Height () != 64)
//	Error ("Bitmap <%s> is not 64x64", name);
gameData.pig.tex.objBmIndexP [gameData.pig.tex.nObjBitmaps] = gameData.pig.tex.nObjBitmaps;
gameData.pig.tex.nObjBitmaps++;
Assert (gameData.pig.tex.nObjBitmaps < MAX_OBJ_BITMAPS);
return gameData.pig.tex.bitmaps [0] + i;
}

//------------------------------------------------------------------------------

void OglCachePolyModelTextures (int32_t nModel);

int32_t LoadExitModels (void)
{
	CFile cf;
	int32_t start_num, i;
	static const char* szExitBm [] = {
		"steel1.bbm", 
		"rbot061.bbm", 
		"rbot062.bbm", 
		"steel1.bbm", 
		"rbot061.bbm", 
		"rbot063.bbm", 
		NULL};

	FreeModelExtensions ();
	FreeObjExtensionBitmaps ();

	start_num = gameData.pig.tex.nObjBitmaps;
	for (i = 0; szExitBm [i]; i++) 
		if (!LoadExitModelBitmap (szExitBm [i])) {
#if TRACE
		console.printf (CON_NORMAL, "Can't load exit models!\n");
#endif
		return 0;
		}

	if (cf.Open ("exit.ham", gameFolders.game.szData [0], "rb", 0)) {
		gameData.endLevel.exit.nModel = gameData.models.nPolyModels++;
		gameData.endLevel.exit.nDestroyedModel = gameData.models.nPolyModels++;
		CPolyModel& exitModel = gameData.models.polyModels [0][gameData.endLevel.exit.nModel];
		CPolyModel& destrModel = gameData.models.polyModels [0][gameData.endLevel.exit.nDestroyedModel];
		if (!exitModel.Read (0, 0, cf))
			return 0;
		if (!destrModel.Read (0, 0, cf))
			return 0;
		exitModel.SetFirstTexture (start_num);
		exitModel.SetBuffer (NULL);
		destrModel.SetFirstTexture (start_num + 3);
		destrModel.SetBuffer (NULL);
		exitModel.ReadData (NULL, cf);
		destrModel.ReadData (NULL, cf);
		cf.Close ();
		}
	else if (cf.Exist ("exit01.pof", gameFolders.game.szData [0], 0) && 
				cf.Exist ("exit01d.pof", gameFolders.game.szData [0], 0)) {
		gameData.endLevel.exit.nModel = LoadPolyModel ("exit01.pof", 3, start_num, NULL);
		gameData.endLevel.exit.nDestroyedModel = LoadPolyModel ("exit01d.pof", 3, start_num + 3, NULL);
		OglCachePolyModelTextures (gameData.endLevel.exit.nModel);
		OglCachePolyModelTextures (gameData.endLevel.exit.nDestroyedModel);
	}
	else if (cf.Exist (D1_PIGFILE,gameFolders.game.szData [0],0)) {
		int32_t offset, offset2;

		cf.Open (D1_PIGFILE, gameFolders.game.szData [0], "rb",0);
		switch (cf.Length ()) { //total hack for loading models
		case D1_PIGSIZE:
			offset = 91848;/* and 92582  */
			offset2 = 383390;/* and 394022 */
			break;
		default:
		case D1_SHARE_BIG_PIGSIZE:
		case D1_SHARE_10_PIGSIZE:
		case D1_SHARE_PIGSIZE:
		case D1_10_BIG_PIGSIZE:
		case D1_10_PIGSIZE:
			Int3 ();/* exit models should be in .pofs */
		case D1_OEM_PIGSIZE:
		case D1_MAC_PIGSIZE:
		case D1_MAC_SHARE_PIGSIZE:
#if TRACE
			console.printf (CON_NORMAL, "Can't load exit models!\n");
#endif
			return 0;
		}
		cf.Seek (offset, SEEK_SET);
		gameData.endLevel.exit.nModel = gameData.models.nPolyModels++;
		gameData.endLevel.exit.nDestroyedModel = gameData.models.nPolyModels++;
		CPolyModel& exitModel = gameData.models.polyModels [0][gameData.endLevel.exit.nModel];
		CPolyModel& destrModel = gameData.models.polyModels [0][gameData.endLevel.exit.nDestroyedModel];
		if (!exitModel.Read (0, 0, cf))
			return 0;
		if (!destrModel.Read (0, 0, cf))
			return 0;
		exitModel.SetFirstTexture (start_num);
		destrModel.SetFirstTexture (start_num+3);
		cf.Seek (offset2, SEEK_SET);
		exitModel.SetBuffer (NULL);
		destrModel.SetBuffer (NULL);
		exitModel.ReadData (NULL, cf);
		destrModel.ReadData (NULL, cf);
		cf.Close ();
	} else {
#if TRACE
		console.printf (CON_NORMAL, "Can't load exit models!\n");
#endif
		return 0;
	}
	atexit (FreeObjExtensionBitmaps);
#if 1
	OglCachePolyModelTextures (gameData.endLevel.exit.nModel);
	OglCachePolyModelTextures (gameData.endLevel.exit.nDestroyedModel);
#endif
	return 1;
}

//------------------------------------------------------------------------------

void RestoreDefaultModels (void)
{
	CPolyModel*	modelP = &gameData.models.polyModels [0][0];
	int32_t			i;

gameData.botData.info [0] = gameData.botData.defaultInfo;
gameData.botData.joints = gameData.botData.defaultJoints;
gameData.pig.tex.objBmIndex = gameData.pig.tex.defaultObjBmIndex;
for (i = 0; i < gameData.models.nDefPolyModels; i++, modelP++) {
#if DBG
	if (i == nDbgModel)
		BRP;
#endif
	if (modelP->Custom ()) {
		modelP->Destroy ();
		*modelP = gameData.models.polyModels [1][i];
		}
	}
for (; i < gameData.models.nPolyModels; i++, modelP++)
	modelP->Destroy ();
gameData.botData.nTypes [0] = gameData.botData.nDefaultTypes;
gameData.botData.nJoints = gameData.botData.nDefaultJoints;
}

//------------------------------------------------------------------------------

#define MODEL_DATA_VERSION 3

typedef struct tModelDataHeader {
	int32_t					nVersion;
} tModelDataHeader;

int32_t LoadModelData (void)
{
	CFile					cf;
	tModelDataHeader	mdh;
	int32_t					bOk;

if (!gameStates.app.bCacheModelData)
	return 0;
if (!cf.Open ("modeldata.d2x", gameFolders.var.szCache, "rb", 0))
	return 0;
bOk = (cf.Read (&mdh, sizeof (mdh), 1) == 1);
if (bOk)
	bOk = (mdh.nVersion == MODEL_DATA_VERSION);
if (bOk)
	bOk = gameData.models.spheres.Read (cf, 1) == 1;
if (!bOk)
	gameData.models.spheres.Clear ();
cf.Close ();
return bOk;
}

//------------------------------------------------------------------------------

int32_t SaveModelData (void)
{
	CFile					cf;
	tModelDataHeader	mdh = {MODEL_DATA_VERSION};
	int32_t					bOk;

if (!gameStates.app.bCacheModelData)
	return 0;
if (!cf.Open ("modeldata.d2x", gameFolders.var.szCache, "wb", 0))
	return 0;
bOk = (cf.Write (&mdh, sizeof (mdh), 1) == 1) &&
		(gameData.models.spheres.Write (cf) == gameData.models.spheres.Length ()) &&
cf.Close ();
return bOk;
}

//------------------------------------------------------------------------------
//eof

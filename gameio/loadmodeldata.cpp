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
#define N_D2_ROBOT_TYPES		66
#define N_D2_ROBOT_JOINTS		1145
#define N_D2_POLYGON_MODELS   166
#define N_D2_OBJBITMAPS			422
#define N_D2_OBJBITMAPPTRS		502
#define N_D2_WEAPON_TYPES		62

void _CDECL_ FreeObjExtensionBitmaps (void)
{
	int		i;
	CBitmap*	bmP;

PrintLog ("unloading extra bitmaps\n");
if (!gameData.pig.tex.nExtraBitmaps)
	gameData.pig.tex.nExtraBitmaps = gameData.pig.tex.nBitmaps [0];
for (i = gameData.pig.tex.nBitmaps [0], bmP = gameData.pig.tex.bitmaps [0] + i; 
	  i < gameData.pig.tex.nExtraBitmaps; i++, bmP++) {
	gameData.pig.tex.nObjBitmaps--;
	bmP->ReleaseTexture ();
	if (bmP->Buffer ()) {
		bmP->DestroyBuffer ();
		UseBitmapCache (bmP, (int) -bmP->Height () * (int) bmP->RowSize ());
		}
	}
gameData.pig.tex.nExtraBitmaps = gameData.pig.tex.nBitmaps [0];
}

//------------------------------------------------------------------------------

void FreeModelExtensions (void)
{
	//return;
PrintLog ("unloading extra poly models\n");
while (gameData.models.nPolyModels > N_D2_POLYGON_MODELS) {
	gameData.models.polyModels [0][--gameData.models.nPolyModels].Destroy ();
	gameData.models.polyModels [1][gameData.models.nPolyModels].Destroy ();
	}
if (!gameStates.app.bDemoData)
	while (gameData.models.nPolyModels > gameData.endLevel.exit.nModel) {
		gameData.models.polyModels [0][--gameData.models.nPolyModels].Destroy ();
		gameData.models.polyModels [1][gameData.models.nPolyModels].Destroy ();
		}
}

//------------------------------------------------------------------------------

//nType==1 means 1.1, nType==2 means 1.2 (with weapons)
int LoadRobotExtensions (const char *fname, char *folder, int nType)
{
	CFile cf;
	int t,i,j;
	int version, bVertigoData;

	//strlwr (fname);
bVertigoData = !strcmp (fname, "d2x.ham");
if (!cf.Open (fname, folder, "rb", 0))
	return 0;

//if (bVertigoData) 
 {
	FreeModelExtensions ();
	FreeObjExtensionBitmaps ();
	}

if (nType > 1) {
	int sig;

	sig = cf.ReadInt ();
	if (sig != MAKE_SIG ('X','H','A','M'))
		return 0;
	version = cf.ReadInt ();
}
else
	version = 0;

//read extra weapons

t = cf.ReadInt ();
gameData.weapons.nTypes [0] = N_D2_WEAPON_TYPES+t;
if (gameData.weapons.nTypes [0] >= MAX_WEAPON_TYPES) {
	Warning ("Too many weapons (%d) in <%s>.  Max is %d.",t,fname,MAX_WEAPON_TYPES-N_D2_WEAPON_TYPES);
	return -1;
	}
ReadWeaponInfos (N_D2_WEAPON_TYPES, t, cf, 3);

//now read robot info

t = cf.ReadInt ();
gameData.bots.nTypes [0] = N_D2_ROBOT_TYPES + t;
if (gameData.bots.nTypes [0] >= MAX_ROBOT_TYPES) {
	Warning ("Too many robots (%d) in <%s>.  Max is %d.",t,fname,MAX_ROBOT_TYPES-N_D2_ROBOT_TYPES);
	return -1;
	}
ReadRobotInfos (gameData.bots.info [0], t, cf, N_D2_ROBOT_TYPES);
if (bVertigoData) {
	gameData.bots.nDefaultTypes = gameData.bots.nTypes [0];
	memcpy (&gameData.bots.defaultInfo [N_D2_ROBOT_TYPES], &gameData.bots.info [0][N_D2_ROBOT_TYPES], sizeof (gameData.bots.info [0][0]) * t);
	}

t = cf.ReadInt ();
gameData.bots.nJoints = N_D2_ROBOT_JOINTS + t;
if (gameData.bots.nJoints >= MAX_ROBOT_JOINTS) {
	Warning ("Too many robot joints (%d) in <%s>.  Max is %d.",t,fname,MAX_ROBOT_JOINTS-N_D2_ROBOT_JOINTS);
	return -1;
	}
ReadJointPositions (gameData.bots.joints, t, cf, N_D2_ROBOT_JOINTS);
if (bVertigoData) {
	gameData.bots.nDefaultJoints = gameData.bots.nJoints;
	memcpy (&gameData.bots.defaultJoints [N_D2_ROBOT_TYPES], &gameData.bots.joints [N_D2_ROBOT_TYPES], sizeof (gameData.bots.joints [0]) * t);
	}

t = cf.ReadInt ();
j = N_D2_POLYGON_MODELS; //gameData.models.nPolyModels;
gameData.models.nPolyModels += t;
if (gameData.models.nPolyModels >= MAX_POLYGON_MODELS) {
	Warning ("Too many polygon models (%d)\nin <%s>.\nMax is %d.",
				t,fname, MAX_POLYGON_MODELS - N_D2_POLYGON_MODELS);
	return -1;
	}
ReadPolyModels (gameData.models.polyModels [0] + j, t, cf);
if (bVertigoData) {
	gameData.models.nDefPolyModels = gameData.models.nPolyModels;
	memcpy (gameData.models.polyModels [1] + j, gameData.models.polyModels [0] + j, sizeof (CPolyModel) * t);
	}
for (i = j; i < gameData.models.nPolyModels; i++) {
	gameData.models.polyModels [1][i].SetBuffer (NULL);
	gameData.models.polyModels [0][i].SetBuffer (NULL);
	gameData.models.polyModels [0][i].ReadData (bVertigoData ? gameData.models.polyModels [1] + i : NULL, cf);
	}
for (i = j; i < gameData.models.nPolyModels; i++)
	gameData.models.nDyingModels [i] = cf.ReadInt ();
for (i = j; i < gameData.models.nPolyModels; i++)
	gameData.models.nDeadModels [i] = cf.ReadInt ();

t = cf.ReadInt ();
if (N_D2_OBJBITMAPS + t >= MAX_OBJ_BITMAPS) {
	Warning ("Too many CObject bitmaps (%d) in <%s>.  Max is %d.", t, fname, MAX_OBJ_BITMAPS - N_D2_OBJBITMAPS);
	return -1;
	}
ReadBitmapIndices (gameData.pig.tex.objBmIndex, t, cf, N_D2_OBJBITMAPS);

t = cf.ReadInt ();
if (N_D2_OBJBITMAPPTRS + t >= MAX_OBJ_BITMAPS) {
	Warning ("Too many CObject bitmap pointers (%d) in <%s>.  Max is %d.",t,fname,MAX_OBJ_BITMAPS-N_D2_OBJBITMAPPTRS);
	return -1;
	}
for (i = N_D2_OBJBITMAPPTRS; i < (N_D2_OBJBITMAPPTRS + t); i++)
	gameData.pig.tex.objBmIndexP [i] = cf.ReadShort ();
cf.Close ();
return 1;
}

//------------------------------------------------------------------------------

int LoadRobotReplacements (const char* szLevel, const char* szFolder, int bAddBots, int bAltModels)
{
	CFile			cf;
	CPolyModel*	modelP;
	int			t, i, j;
	int			nBotTypeSave = gameData.bots.nTypes [0], 
					nBotJointSave = gameData.bots.nJoints, 
					nPolyModelSave = gameData.models.nPolyModels;
	tRobotInfo	botInfoSave;
	char			szFile [FILENAME_LEN];

CFile::ChangeFilenameExtension (szFile, szLevel, ".hxm");
if (!cf.Open (szFile, szFolder ? szFolder : gameFolders.szDataDir, "rb", 0))		//no robot replacement file
	return 0;
t = cf.ReadInt ();			//read id "HXM!"
if (t!= MAKE_SIG ('!','X','M','H'))
	Warning (TXT_HXM_ID);
t = cf.ReadInt ();			//read version
if (t < 1)
	Warning (TXT_HXM_VERSION, t);
t = cf.ReadInt ();			//read number of robots
for (j = 0; j < t; j++) {
	i = cf.ReadInt ();		//read robot number
	if (bAddBots) {
		if (gameData.bots.nTypes [0] >= MAX_ROBOT_TYPES) {
			Warning (TXT_ROBOT_NO, szLevel, i, MAX_ROBOT_TYPES);
			return -1;
			}
		i = gameData.bots.nTypes [0]++;
		}
	else if (i < 0 || i >= gameData.bots.nTypes [0]) {
		Warning (TXT_ROBOT_NO, szLevel, i, gameData.bots.nTypes [0] - 1);
		gameData.bots.nTypes [0] = nBotTypeSave;
		gameData.bots.nJoints = nBotJointSave;
		gameData.models.nPolyModels = nPolyModelSave;
		return -1;
		}
	if (bAltModels)
		cf.Seek (sizeof (tRobotInfo), SEEK_CUR);
	else {
		botInfoSave = gameData.bots.info [0][i];
		ReadRobotInfos (gameData.bots.info [0], 1, cf, i);
		}
	}
t = cf.ReadInt ();			//read number of joints
for (j = 0; j < t; j++) {
	i = cf.ReadInt ();		//read joint number
	if (bAddBots) {
		if (gameData.bots.nJoints >= MAX_ROBOT_JOINTS) {
			Warning ("%s: Robots joint (%d) out of range (valid range = 0 - %d).",
						szLevel, i, MAX_ROBOT_JOINTS - 1);
			gameData.bots.nTypes [0] = nBotTypeSave;
			gameData.bots.nJoints = nBotJointSave;
			gameData.models.nPolyModels = nPolyModelSave;
			return -1;
			}
		else
			i = gameData.bots.nJoints++;
		}
	else if ((i < 0) || (i >= gameData.bots.nJoints)) {
		Warning ("%s: Robots joint (%d) out of range (valid range = 0 - %d).",
					szLevel, i, gameData.bots.nJoints - 1);
		gameData.bots.nTypes [0] = nBotTypeSave;
		gameData.bots.nJoints = nBotJointSave;
		gameData.models.nPolyModels = nPolyModelSave;
		return -1;
		}
	ReadJointPositions (gameData.bots.joints, 1, cf, i);
	}
t = cf.ReadInt ();			//read number of polygon models
for (j = 0; j < t; j++) {
	i = cf.ReadInt ();		//read model number
	if (bAddBots) {
		if (gameData.models.nPolyModels >= MAX_POLYGON_MODELS) {
			Warning ("%s: Polygon model (%d) out of range (valid range = 0 - %d).",
						szLevel, i, gameData.models.nPolyModels - 1);
			gameData.bots.nTypes [0] = nBotTypeSave;
			gameData.bots.nJoints = nBotJointSave;
			gameData.models.nPolyModels = nPolyModelSave;
			return -1;
			}
		else
			i = gameData.models.nPolyModels++;
		}
	else if ((i < 0) || (i >= gameData.models.nPolyModels)) {
		if (bAltModels) {
			if (i < MAX_POLYGON_MODELS) 
				gameData.models.nPolyModels = i + 1;
			else {
				Warning ("%s: Polygon model (%d) out of range (valid range = 0 - %d).",
							szLevel, i, gameData.models.nPolyModels - 1);
				gameData.bots.nTypes [0] = nBotTypeSave;
				gameData.bots.nJoints = nBotJointSave;
				gameData.models.nPolyModels = nPolyModelSave;
				return -1;
				}
			}
		else {
			Warning ("%s: Polygon model (%d) out of range (valid range = 0 - %d).",
						szLevel, i, gameData.models.nPolyModels - 1);
			gameData.bots.nTypes [0] = nBotTypeSave;
			gameData.bots.nJoints = nBotJointSave;
			gameData.models.nPolyModels = nPolyModelSave;
			return -1;
			}
		}
	modelP = bAltModels ? gameData.models.polyModels [2] + i : gameData.models.polyModels [0] + i;
	modelP->Destroy ();
	if (!modelP->Read (0, cf))
		return -1;
	modelP->ReadData (NULL, cf);
	modelP->SetType (bAltModels ? 1 : -1);
	modelP->SetRad (modelP->Size ());
	if (bAltModels) {
#if 0
		ubyte	*p = gameData.models.polyModels [1][i].modelData;
		gameData.models.polyModels [1][i] = gameData.models.polyModels [0][i];
		gameData.models.polyModels [1][i].modelData = p;
#else
		cf.ReadInt ();
		cf.ReadInt ();
#endif
		}
	else {
		gameData.models.nDyingModels [i] = cf.ReadInt ();
		gameData.models.nDeadModels [i] = cf.ReadInt ();
		}
	}

t = cf.ReadInt ();			//read number of objbitmaps
for (j = 0; j < t; j++) {
	i = cf.ReadInt ();		//read objbitmap number
	if (bAddBots) {
		}
	else if ((i < 0) || (i >= MAX_OBJ_BITMAPS)) {
		Warning ("%s: Object bitmap number (%d) out of range (valid range = 0 - %d).",
					szLevel, i, MAX_OBJ_BITMAPS - 1);
		gameData.bots.nTypes [0] = nBotTypeSave;
		gameData.bots.nJoints = nBotJointSave;
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
		gameData.bots.nTypes [0] = nBotTypeSave;
		gameData.bots.nJoints = nBotJointSave;
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
	int				iffError;		//reference parm to avoid warning message
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
	bmP->Remap (NULL, iff.TransparentColor (), 254);
else
	bmP->Remap (NULL, -1, 254);
bmP->AvgColorIndex ();
bmi.index = gameData.pig.tex.nExtraBitmaps;
gameData.pig.tex.bitmapP [gameData.pig.tex.nExtraBitmaps++] = *bmP;
return bmi;
}

//------------------------------------------------------------------------------

static CBitmap *LoadExitModelBitmap (const char *name)
{
	int i;
	tBitmapIndex	*bip = gameData.pig.tex.objBmIndex + gameData.pig.tex.nObjBitmaps;
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

void OglCachePolyModelTextures (int nModel);

int LoadExitModels (void)
{
	CFile cf;
	int start_num, i;
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
		if (!LoadExitModelBitmap (szExitBm [i]))
	 {
#if TRACE
		console.printf (CON_NORMAL, "Can't load exit models!\n");
#endif
		return 0;
		}

	if (cf.Open ("exit.ham", gameFolders.szDataDir, "rb", 0)) {
		gameData.endLevel.exit.nModel = gameData.models.nPolyModels++;
		gameData.endLevel.exit.nDestroyedModel = gameData.models.nPolyModels++;
		CPolyModel& exitModel = gameData.models.polyModels [0][gameData.endLevel.exit.nModel];
		CPolyModel& destrModel = gameData.models.polyModels [0][gameData.endLevel.exit.nDestroyedModel];
		if (!exitModel.Read (0, cf))
			return 0;
		if (!destrModel.Read (0, cf))
			return 0;
		exitModel.SetFirstTexture (start_num);
		exitModel.SetBuffer (NULL);
		destrModel.SetFirstTexture (start_num + 3);
		destrModel.SetBuffer (NULL);
		exitModel.ReadData (NULL, cf);
		destrModel.ReadData (NULL, cf);
		cf.Close ();
		}
	else if (cf.Exist ("exit01.pof", gameFolders.szDataDir, 0) && 
				cf.Exist ("exit01d.pof", gameFolders.szDataDir, 0)) {
		gameData.endLevel.exit.nModel = LoadPolyModel ("exit01.pof", 3, start_num, NULL);
		gameData.endLevel.exit.nDestroyedModel = LoadPolyModel ("exit01d.pof", 3, start_num + 3, NULL);
		OglCachePolyModelTextures (gameData.endLevel.exit.nModel);
		OglCachePolyModelTextures (gameData.endLevel.exit.nDestroyedModel);
	}
	else if (cf.Exist (D1_PIGFILE,gameFolders.szDataDir,0)) {
		int offset, offset2;

		cf.Open (D1_PIGFILE, gameFolders.szDataDir, "rb",0);
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
		if (!exitModel.Read (0, cf))
			return 0;
		if (!destrModel.Read (0, cf))
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
	int	i;
	ubyte	*p;

gameData.bots.info [0] = gameData.bots.defaultInfo;
gameData.bots.joints = gameData.bots.defaultJoints;
for (i = 0; i < gameData.models.nDefPolyModels; i++) {
#if DBG
	if (i == nDbgModel)
		nDbgModel = nDbgModel;
#endif
	p = gameData.models.polyModels [0][i].Buffer ();
	if (gameData.models.polyModels [1][i].DataSize () != gameData.models.polyModels [0][i].DataSize ()) {
		gameData.models.polyModels [0][i].Destroy ();
		p = NULL;
		}
	if (gameData.models.polyModels [1][i].Buffer ()) {
		gameData.models.polyModels [0][i].Destroy ();
		gameData.models.polyModels [0][i] = gameData.models.polyModels [1][i];
		}
	else if (p) {
		gameData.models.polyModels [0][i].Destroy ();
		}
	}
for (; i < gameData.models.nPolyModels; i++)
	gameData.models.polyModels [0][i].Destroy ();
gameData.bots.nTypes [0] = gameData.bots.nDefaultTypes;
gameData.bots.nJoints = gameData.bots.nDefaultJoints;
}

//------------------------------------------------------------------------------

#define MODEL_DATA_VERSION 1

typedef struct tModelDataHeader {
	int					nVersion;
} tModelDataHeader;

int LoadModelData (void)
{
	CFile					cf;
	tModelDataHeader	mdh;
	int					bOk;

if (!gameStates.app.bCacheModelData)
	return 0;
if (!cf.Open ("modeldata.d2x", gameFolders.szCacheDir, "rb", 0))
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

int SaveModelData (void)
{
	CFile					cf;
	tModelDataHeader	mdh = {MODEL_DATA_VERSION};
	int					bOk;

if (!gameStates.app.bCacheModelData)
	return 0;
if (!cf.Open ("modeldata.d2x", gameFolders.szCacheDir, "wb", 0))
	return 0;
bOk = (cf.Write (&mdh, sizeof (mdh), 1) == 1) &&
		(gameData.models.spheres.Write (cf) == gameData.models.spheres.Length ()) &&
cf.Close ();
return bOk;
}

//------------------------------------------------------------------------------
//eof

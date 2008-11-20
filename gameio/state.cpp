/* $Id: state.c,v 1.12 2003/11/27 00:21:04 btb Exp $ */
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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#ifndef _WIN32_WCE
#include <errno.h>
#endif

#ifdef _WIN32
#  include <windows.h>
#endif
#ifdef __macosx__
#  include <OpenGL/gl.h>
#else
#  include <GL/gl.h>
#endif

#ifndef _WIN32
#	include <arpa/inet.h>
#	include <netinet/in.h> /* for htons & co. */
#endif

#include "pstypes.h"
#include "mono.h"
#include "inferno.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "digi.h"
#include "gamemine.h"
#include "error.h"
#include "gameseg.h"
#include "menu.h"
#include "switch.h"
#include "game.h"
#include "screens.h"
#include "newmenu.h"
#include "gamepal.h"
#include "cfile.h"
#include "fuelcen.h"
#include "hash.h"
#include "key.h"
#include "piggy.h"
#include "player.h"
#include "reactor.h"
#include "morph.h"
#include "weapon.h"
#include "render.h"
#include "loadgame.h"
#include "gauges.h"
#include "newdemo.h"
#include "automap.h"
#include "piggy.h"
#include "paging.h"
#include "briefings.h"
#include "text.h"
#include "mission.h"
#include "pcx.h"
#include "u_mem.h"
#include "network.h"
#include "objeffects.h"
#include "args.h"
#include "ai.h"
#include "fireball.h"
#include "controls.h"
#include "laser.h"
#include "omega.h"
#include "multibot.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "state.h"
#include "strutil.h"
#include "light.h"
#include "dynlight.h"
#include "ipx.h"
#include "gr.h"

#undef DBG
#if DBG
#	define DBG(_expr)	_expr
#else
#	define DBG(_expr)
#endif

#define STATE_VERSION				39
#define STATE_COMPATIBLE_VERSION 20
// 0 - Put DGSS (Descent Game State Save) id at tof.
// 1 - Added Difficulty level save
// 2 - Added gameStates.app.cheats.bEnabled flag
// 3 - Added between levels save.
// 4 - Added mission support
// 5 - Mike changed ai and tObject structure.
// 6 - Added buggin' cheat save
// 7 - Added other cheat saves and game_id.
// 8 - Added AI stuff for escort and thief.
// 9 - Save palette with screen shot
// 12- Saved last_was_super array
// 13- Saved palette flash stuff
// 14- Save cloaking tWall stuff
// 15- Save additional ai info
// 16- Save gameData.render.lights.subtracted
// 17- New marker save
// 18- Took out saving of old cheat status
// 19- Saved cheats_enabled flag
// 20- gameStates.app.bFirstSecretVisit
// 22- gameData.omega.xCharge

#define NUM_SAVES		9
#define THUMBNAIL_W	100
#define THUMBNAIL_H	50
#define THUMBNAIL_LW 200
#define THUMBNAIL_LH 120
#define DESC_LENGTH	20

void SetFunctionMode (int);
void InitPlayerStatsNewShip (void);
void ShowLevelIntro (int level_num);
void DoCloakInvulSecretStuff (fix xOldGameTime);
void CopyDefaultsToRobot (tObject *objP);
void MultiInitiateSaveGame ();
void MultiInitiateRestoreGame ();
void ApplyAllChangedLight (void);
void DoLunacyOn (void);
void DoLunacyOff (void);

int sc_last_item= 0;
grsBitmap *sc_bmp [NUM_SAVES+1];

char dgss_id [4] = {'D', 'G', 'S', 'S'};

int state_default_item = 0;

void ComputeAllStaticLight (void);

static char szDesc [NUM_SAVES + 1][DESC_LENGTH + 16];
static char szTime [NUM_SAVES + 1][DESC_LENGTH + 16];

void GameRenderFrame (void);

//-------------------------------------------------------------------

#define NM_IMG_SPACE	6

static int bShowTime = 1;

int SaveStateMenuCallback (int nitems, tMenuItem *items, int *lastKey, int nCurItem)
{
	int	x, y, i = nCurItem - NM_IMG_SPACE;
	char	c = KeyToASCII (*lastKey);

if (nCurItem < 2)
	return nCurItem;
if ((c >= '1') && (c <= '9')) {
	for (i = 0; i < NUM_SAVES; i++)
		if (items [i + NM_IMG_SPACE].text [0] == c) {
			*lastKey = -2;
			return -(i + NM_IMG_SPACE) - 1;
			}
	}
if (!items [NM_IMG_SPACE - 1].text || strcmp (items [NM_IMG_SPACE - 1].text, szTime [i])) {
	items [NM_IMG_SPACE - 1].text = szTime [i];
	items [NM_IMG_SPACE - 1].rebuild = 1;
	}
if (!sc_bmp [i])
	return nCurItem;
if (gameStates.menus.bHires) {
	x = (grdCurCanv->cvBitmap.bmProps.w - sc_bmp [i]->bmProps.w) / 2;
	y = items [0].y - 16;
	if (gameStates.app.bGameRunning)
		GrPaletteStepLoad (NULL);
	GrBitmap (x, y, sc_bmp [i]);
	if (gameOpts->menus.nStyle) {
		GrSetColorRGBi (RGBA_PAL (0, 0, 32));
		GrUBox (x - 1, y - 1, x + sc_bmp [i]->bmProps.w + 1, y + sc_bmp [i]->bmProps.h + 1);
		}
	}
else {
	GrBitmap ((grdCurCanv->cvBitmap.bmProps.w-THUMBNAIL_W) / 2,items [0].y - 5, sc_bmp [nCurItem - 1]);
	}
return nCurItem;
}

//------------------------------------------------------------------------------

int RestoreStateMenuCallback (int nitems, tMenuItem *items, int *lastKey, int nCurItem)
{
	int	x, y, i = nCurItem - NM_IMG_SPACE;
	char	c = KeyToASCII (*lastKey);

if (nCurItem < 2)
	return nCurItem;
if ((c >= '1') && (c <= '9')) {
	for (i = 0; i < NUM_SAVES; i++)
		if (items [i].text [0] == c) {
			*lastKey = -2;
			return -i - 1;
			}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void rpad_string (char * string, int max_chars)
{
	int i, end_found;

	end_found = 0;
	for (i=0; i<max_chars; i++)	{
		if (*string == 0)
			end_found = 1;
		if (end_found)
			*string = ' ';
		string++;
	}
	*string = 0;		// NULL terminate
}

//------------------------------------------------------------------------------

int StateGetSaveFile (char * fname, char * dsc, int bMulti)
{
	CFILE cf;
	int i, menuRes, choice, sgVersion;
	tMenuItem m [NUM_SAVES+2];
	char filename [NUM_SAVES+1][30];
	char id [5];
	int valid=0;

memset (m, 0, sizeof (m));
for (i = 0; i < NUM_SAVES; i++)	{
	sc_bmp [i] = NULL;
	if (!bMulti)
		sprintf (filename [i], "%s.sg%x", LOCALPLAYER.callsign, i);
	else
		sprintf (filename [i], "%s.mg%x", LOCALPLAYER.callsign, i);
	valid = 0;
	if (CFOpen (&cf, filename [i], gameFolders.szSaveDir, "rb", 0)) {
		//Read id
		CFRead (id, sizeof (char)*4, 1, &cf);
		if (!memcmp (id, dgss_id, 4)) {
			//Read sgVersion
			CFRead (&sgVersion, sizeof (int), 1, &cf);
			if (sgVersion >= STATE_COMPATIBLE_VERSION)	{
				// Read description
				CFRead (szDesc [i], sizeof (char)*DESC_LENGTH, 1, &cf);
				valid = 1;
				}
			}
			CFClose (&cf);
		}
	if (!valid)
		strcpy (szDesc [i], TXT_EMPTY);
	ADD_INPUT_MENU (i, szDesc [i], DESC_LENGTH - 1, -1, NULL);
	}

sc_last_item = -1;
choice = state_default_item;
menuRes = ExecMenu1 (NULL, TXT_SAVE_GAME_MENU, NUM_SAVES, m, NULL, &choice);

for (i = 0; i < NUM_SAVES; i++)	{
	if (sc_bmp [i])
		GrFreeBitmap (sc_bmp [i]);
	}
if (menuRes < 0)
	return 0;
strcpy (fname, filename [choice]);
strcpy (dsc, szDesc [choice]);
state_default_item = choice;
return choice + 1;
}

//------------------------------------------------------------------------------

int bRestoringMenu = 0;

int StateGetRestoreFile (char * fname, int bMulti)
{
	CFILE			cf;
	int			i, j, choice = -1, sgVersion, nSaves;
	tMenuItem	m [NUM_SAVES + NM_IMG_SPACE + 1];
	char			filename [NUM_SAVES+1][30];
	char			id [5];
	ubyte			pal [256 * 3];
	int			valid;

	nSaves = 0;
	memset (m, 0, sizeof (m));
	for (i = 0; i < NM_IMG_SPACE; i++) {
		m [i].nType = NM_TYPE_TEXT; 
		m [i].text = (char *) "";
		m [i].noscroll = 1;
		}
	if (gameStates.app.bGameRunning) {
		GrPaletteStepLoad (NULL);
		}
	for (i = 0, j = 0; i < NUM_SAVES + 1; i++, j++) {
		sc_bmp [i] = NULL;
		sprintf (filename [i], bMulti ? "%s.mg%x" : "%s.sg%x", LOCALPLAYER.callsign, i);
		valid = 0;
		if (CFOpen (&cf, filename [i], gameFolders.szSaveDir, "rb", 0)) {
			//Read id
			CFRead (id, sizeof (char) * 4, 1, &cf);
			if (!memcmp (id, dgss_id, 4)) {
				//Read sgVersion
				CFRead (&sgVersion, sizeof (int), 1, &cf);
				if (sgVersion >= STATE_COMPATIBLE_VERSION)	{
					// Read description
					if (i < NUM_SAVES)
						sprintf (szDesc [j], "%d. ", i + 1);
					else
						strcpy (szDesc [j], "   ");
					CFRead (szDesc [j] + 3, sizeof (char) * DESC_LENGTH, 1, &cf);
					// rpad_string (szDesc [i], DESC_LENGTH-1);
					ADD_MENU (j + NM_IMG_SPACE, szDesc [j], (i < NUM_SAVES) ? -1 : 0, NULL);
					// Read thumbnail
					if (sgVersion < 26) {
						sc_bmp [i] = GrCreateBitmap (THUMBNAIL_W, THUMBNAIL_H, 1);
						CFRead (sc_bmp [i]->bmTexBuf, THUMBNAIL_W * THUMBNAIL_H, 1, &cf);
						}
					else {
						sc_bmp [i] = GrCreateBitmap (THUMBNAIL_LW, THUMBNAIL_LH, 1);
						CFRead (sc_bmp [i]->bmTexBuf, THUMBNAIL_LW * THUMBNAIL_LH, 1, &cf);
						}
					if (sgVersion >= 9) {
						CFRead (pal, 3, 256, &cf);
						GrRemapBitmapGood (sc_bmp [i], pal, -1, -1);
						}
					nSaves++;
					valid = 1;
					}
				}
			CFClose (&cf);
			}
		if (valid) {
			if (bShowTime) {
				struct tm	*t;
				int			h;
#ifdef _WIN32
				char	fn [FILENAME_LEN];

				struct _stat statBuf;
				sprintf (fn, "%s/%s", gameFolders.szSaveDir, filename [i]);
				h = _stat (fn, &statBuf);
#else
				struct stat statBuf;
				h = stat (filename [i], &statBuf);
#endif
				if (!h && (t = localtime (&statBuf.st_mtime)))
					sprintf (szTime [j], " [%d-%d-%d %d:%02d:%02d]",
						t->tm_mon + 1, t->tm_mday, t->tm_year + 1900,
						t->tm_hour, t->tm_min, t->tm_sec);
				}
			}
		else {
			strcpy (szDesc [j], TXT_EMPTY);
			//rpad_string (szDesc [i], DESC_LENGTH-1);
			m [j+NM_IMG_SPACE].nType = NM_TYPE_MENU; 
			m [j+NM_IMG_SPACE].text = szDesc [j];
		}
	}
	if (gameStates.app.bGameRunning) {
		GrPaletteStepLoad (NULL);
		}

	if (nSaves < 1)	{
		ExecMessageBox (NULL, NULL, 1, "Ok", TXT_NO_SAVEGAMES);
		return 0;
	}

	if (gameStates.video.nDisplayMode == 1)	//restore menu won't fit on 640x400
		gameStates.render.vr.nScreenFlags ^= VRF_COMPATIBLE_MENUS;

	sc_last_item = -1;

   bRestoringMenu = 1;
	choice = state_default_item + NM_IMG_SPACE;
	i = ExecMenu3 (NULL, TXT_LOAD_GAME_MENU, j + NM_IMG_SPACE, m, SaveStateMenuCallback, 
					   &choice, NULL, 190, -1);
	if (i < 0)
		return 0;
   bRestoringMenu = 0;
	choice -= NM_IMG_SPACE;

	for (i = 0; i < NUM_SAVES + 1; i++)	{
		if (sc_bmp [i])
			GrFreeBitmap (sc_bmp [i]);
	}

	if (choice >= 0) {
		strcpy (fname, filename [choice]);
		if (choice != NUM_SAVES+1)		//no new default when restore from autosave
			state_default_item = choice;
		return choice + 1;
	}
	return 0;
}

#define	DESC_OFFSET	8

#define	CF_BUF_SIZE	1024

#ifdef _WIN32_WCE
# define errno -1
# define strerror (x) "Unknown Error"
#endif


//	-----------------------------------------------------------------------------------
//	Imagine if C had a function to copy a file...
int copy_file (const char *old_file, const char *new_file)
{
	sbyte	buf [CF_BUF_SIZE];
	CFILE	cfIn, cfOut;

	if (!CFOpen (&cfOut, new_file, gameFolders.szSaveDir, "wb", 0))
		return -1;
	if (!CFOpen (&cfIn, old_file, gameFolders.szSaveDir, "rb", 0))
		return -2;
	while (!CFEoF (&cfIn)) {
		int bytes_read = (int) CFRead (buf, 1, CF_BUF_SIZE, &cfIn);
		if (CFError (&cfIn))
			Error (TXT_FILEREAD_ERROR, old_file, strerror (errno));
		Assert (bytes_read == CF_BUF_SIZE || CFEoF (&cfIn));
		CFWrite (buf, 1, bytes_read, &cfOut);
		if (CFError (&cfOut))
			Error (TXT_FILEWRITE_ERROR, new_file, strerror (errno));
	}

	if (CFClose (&cfIn)) {
		CFClose (&cfOut);
		return -3;
	}

	if (CFClose (&cfOut))
		return -4;

	return 0;
}

#define SECRETB_FILENAME	"secret.sgb"
#define SECRETC_FILENAME	"secret.sgc"

//	-----------------------------------------------------------------------------------
//	blind_save means don't prompt user for any info.

int StateSaveAll (int bBetweenLevels, int bSecretSave, int bQuick, const char *pszFilenameOverride)
{
	int	rval, filenum = -1;
	char	filename [128], szDesc [DESC_LENGTH+1];

Assert (bBetweenLevels == 0);	//between levels save ripped out
if (IsMultiGame) {
	MultiInitiateSaveGame ();
	return 0;
	}
if (!(bSecretSave || gameOpts->gameplay.bSecretSave || gameStates.app.bD1Mission) && 
	  (gameData.missions.nCurrentLevel < 0)) {
	HUDInitMessage (TXT_SECRET_SAVE_ERROR);
	return 0;
	}
if (gameStates.gameplay.bFinalBossIsDead)		//don't allow save while final boss is dying
	return 0;
//	If this is a secret save and the control center has been destroyed, don't allow
//	return to the base level.
if (bSecretSave && gameData.reactor.bDestroyed) {
	CFDelete (SECRETB_FILENAME, gameFolders.szSaveDir);
	return 0;
	}
StopTime ();
gameData.app.bGamePaused = 1;
if (bQuick)
	sprintf (filename, "%s.quick", LOCALPLAYER.callsign);
else {
	if (bSecretSave == 1) {
		pszFilenameOverride = filename;
		sprintf (filename, SECRETB_FILENAME);
		} 
	else if (bSecretSave == 2) {
		pszFilenameOverride = filename;
		sprintf (filename, SECRETC_FILENAME);
		} 
	else {
		if (pszFilenameOverride) {
			strcpy (filename, pszFilenameOverride);
			sprintf (szDesc, " [autosave backup]");
			}
		else if (!(filenum = StateGetSaveFile (filename, szDesc, 0))) {
			gameData.app.bGamePaused = 0;
			StartTime (1);
			return 0;
			}
		}
	//	MK, 1/1/96
	//	If not in multiplayer, do special secret level stuff.
	//	If secret.sgc exists, then copy it to Nsecret.sgc (where N = filenum).
	//	If it doesn't exist, then delete Nsecret.sgc
	if (!bSecretSave && !IsMultiGame) {
		int	rval;
		char	temp_fname [32], fc;

		if (filenum != -1) {
			if (filenum >= 10)
				fc = (filenum-10) + 'a';
			else
				fc = '0' + filenum;
			sprintf (temp_fname, "%csecret.sgc", fc);
			if (CFExist (temp_fname,gameFolders.szSaveDir,0)) {
				rval = CFDelete (temp_fname, gameFolders.szSaveDir);
				Assert (rval == 0);	//	Oops, error deleting file in temp_fname.
				}
			if (CFExist (SECRETC_FILENAME,gameFolders.szSaveDir,0)) {
				rval = copy_file (SECRETC_FILENAME, temp_fname);
				Assert (rval == 0);	//	Oops, error copying temp_fname to secret.sgc!
				}
			}
		}

		//	Save file we're going to save over in last slot and call " [autosave backup]"
	if (!pszFilenameOverride) {
		CFILE tfp;
		
		if (CFOpen (&tfp, filename, gameFolders.szSaveDir, "rb",0)) {
			char	newname [128];

			sprintf (newname, "%s.sg%x", LOCALPLAYER.callsign, NUM_SAVES);
			CFSeek (&tfp, DESC_OFFSET, SEEK_SET);
			CFWrite ((char *) " [autosave backup]", sizeof (char) * DESC_LENGTH, 1, &tfp);
			CFClose (&tfp);
			CFDelete (newname, gameFolders.szSaveDir);
			CFRename (filename, newname, gameFolders.szSaveDir);
			}
		}
	}
if ((rval = StateSaveAllSub (filename, szDesc, bBetweenLevels)))
	if (bQuick)
		HUDInitMessage (TXT_QUICKSAVE);
gameData.app.bGamePaused = 0;
StartTime (1);
return rval;
}

//------------------------------------------------------------------------------

void StateSaveBinGameData (CFILE cf, int bBetweenLevels)
{
	int		i, j;
	ushort	nWall, nTexture;
	short		nObjsWithTrigger, nObject, nFirstTrigger;
	tObject	*objP;

// Save the Between levels flag...
CFWrite (&bBetweenLevels, sizeof (int), 1, &cf);
// Save the mission info...
CFWrite (gameData.missions.list + gameData.missions.nCurrentMission, sizeof (char), 9, &cf);
//Save level info
CFWrite (&gameData.missions.nCurrentLevel, sizeof (int), 1, &cf);
CFWrite (&gameData.missions.nNextLevel, sizeof (int), 1, &cf);
//Save gameData.time.xGame
CFWrite (&gameData.time.xGame, sizeof (fix), 1, &cf);
// If coop save, save all
if (gameData.app.nGameMode & GM_MULTI_COOP) {
	CFWrite (&gameData.app.nStateGameId, sizeof (int), 1, &cf);
	CFWrite (&netGame, sizeof (tNetgameInfo), 1, &cf);
	CFWrite (&netPlayers, sizeof (tAllNetPlayersInfo), 1, &cf);
	CFWrite (&gameData.multiplayer.nPlayers, sizeof (int), 1, &cf);
	CFWrite (&gameData.multiplayer.nLocalPlayer, sizeof (int), 1, &cf);
	for (i = 0; i < gameData.multiplayer.nPlayers; i++)
		CFWrite (&gameData.multiplayer.players [i], sizeof (tPlayer), 1, &cf);
	}
//Save tPlayer info
CFWrite (&LOCALPLAYER, sizeof (tPlayer), 1, &cf);
// Save the current weapon info
CFWrite (&gameData.weapons.nPrimary, sizeof (sbyte), 1, &cf);
CFWrite (&gameData.weapons.nSecondary, sizeof (sbyte), 1, &cf);
// Save the difficulty level
CFWrite (&gameStates.app.nDifficultyLevel, sizeof (int), 1, &cf);
// Save cheats enabled
CFWrite (&gameStates.app.cheats.bEnabled, sizeof (int), 1, &cf);
if (!bBetweenLevels)	{
//Finish all morph OBJECTS
	FORALL_OBJS (objP, i) {
		if (objP->info.nType == OBJ_NONE) 
			continue;
		if (objP->info.nType == OBJ_CAMERA)
			objP->info.position.mOrient = gameData.cameras.cameras [gameData.objs.cameraRef [OBJ_IDX (objP)]].orient;
		else if (objP->info.renderType == RT_MORPH) {
			tMorphInfo *md = MorphFindData (objP);
			if (md) {
				tObject *mdObjP = md->objP;
				mdObjP->info.controlType = md->saveControlType;
				mdObjP->info.movementType = md->saveMovementType;
				mdObjP->info.renderType = RT_POLYOBJ;
				mdObjP->mType.physInfo = md->savePhysInfo;
				md->objP = NULL;
				} 
			else {						//maybe loaded half-morphed from disk
				KillObject (objP);
				objP->info.renderType = RT_POLYOBJ;
				objP->info.controlType = CT_NONE;
				objP->info.movementType = MT_NONE;
				}
			}
		}
//Save tObject info
	i = gameData.objs.nLastObject [0] + 1;
	CFWrite (&i, sizeof (int), 1, &cf);
	CFWrite (OBJECTS, sizeof (tObject), i, &cf);
//Save tWall info
	i = gameData.walls.nWalls;
	CFWrite (&i, sizeof (int), 1, &cf);
	CFWrite (gameData.walls.walls, sizeof (tWall), i, &cf);
//Save exploding wall info
	i = MAX_EXPLODING_WALLS;
	CFWrite (&i, sizeof (int), 1, &cf);
	CFWrite (gameData.walls.explWalls, sizeof (*gameData.walls.explWalls), i, &cf);
//Save door info
	i = gameData.walls.nOpenDoors;
	CFWrite (&i, sizeof (int), 1, &cf);
	CFWrite (gameData.walls.activeDoors, sizeof (tActiveDoor), i, &cf);
//Save cloaking tWall info
	i = gameData.walls.nCloaking;
	CFWrite (&i, sizeof (int), 1, &cf);
	CFWrite (gameData.walls.cloaking, sizeof (tCloakingWall), i, &cf);
//Save tTrigger info
	CFWrite (&gameData.trigs.nTriggers, sizeof (int), 1, &cf);
	CFWrite (gameData.trigs.triggers, sizeof (tTrigger), gameData.trigs.nTriggers, &cf);
	CFWrite (&gameData.trigs.nObjTriggers, sizeof (int), 1, &cf);
	CFWrite (gameData.trigs.objTriggers, sizeof (tTrigger), gameData.trigs.nObjTriggers, &cf);
	CFWrite (gameData.trigs.objTriggerRefs, sizeof (tObjTriggerRef), gameData.trigs.nObjTriggers, &cf);
	nObjsWithTrigger = 0;
	FORALL_OBJS (objP, nObject) {
		nObject = OBJ_IDX (objP);
		nFirstTrigger = gameData.trigs.firstObjTrigger [nObject];
		if ((nFirstTrigger >= 0) && (nFirstTrigger < gameData.trigs.nObjTriggers))
			nObjsWithTrigger++;
		}
	CFWrite (&nObjsWithTrigger, sizeof (nObjsWithTrigger), 1, &cf);
	FORALL_OBJS (objP, nObject) {
		nObject = OBJ_IDX (objP);
		nFirstTrigger = gameData.trigs.firstObjTrigger [nObject];
		if ((nFirstTrigger >= 0) && (nFirstTrigger < gameData.trigs.nObjTriggers)) {
			CFWrite (&nObject, sizeof (nObject), 1, &cf);
			CFWrite (&nFirstTrigger, sizeof (nFirstTrigger), 1, &cf);
			}
		}
//Save tmap info
	for (i = 0; i <= gameData.segs.nLastSegment; i++) {
		for (j = 0; j < 6; j++)	{
			nWall = WallNumI ((short) i, (short) j);
			CFWrite (&nWall, sizeof (short), 1, &cf);
			CFWrite (&gameData.segs.segments [i].sides [j].nBaseTex, sizeof (short), 1, &cf);
			nTexture = gameData.segs.segments [i].sides [j].nOvlTex | (gameData.segs.segments [i].sides [j].nOvlOrient << 14);
			CFWrite (&nTexture, sizeof (short), 1, &cf);
			}
		}
// Save the fuelcen info
	CFWrite (&gameData.reactor.bDestroyed, sizeof (int), 1, &cf);
	CFWrite (&gameData.reactor.countdown.nTimer, sizeof (int), 1, &cf);
	CFWrite (&gameData.matCens.nBotCenters, sizeof (int), 1, &cf);
	CFWrite (gameData.matCens.botGens, sizeof (tMatCenInfo), gameData.matCens.nBotCenters, &cf);
	CFWrite (&gameData.reactor.triggers, sizeof (tReactorTriggers), 1, &cf);
	CFWrite (&gameData.matCens.nFuelCenters, sizeof (int), 1, &cf);
	CFWrite (gameData.matCens.fuelCenters, sizeof (tFuelCenInfo), gameData.matCens.nFuelCenters, &cf);
	CFWrite (&gameData.matCens.nFuelCenters, sizeof (int), 1, &cf);
	CFWrite (gameData.matCens.fuelCenters, sizeof (tFuelCenInfo), gameData.matCens.nFuelCenters, &cf);
// Save the control cen info
	CFWrite (&gameData.reactor.bPresent, sizeof (int), 1, &cf);
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		CFWrite (&gameData.reactor.states [i].nObject, sizeof (int), 1, &cf);
		CFWrite (&gameData.reactor.states [i].bHit, sizeof (int), 1, &cf);
		CFWrite (&gameData.reactor.states [i].bSeenPlayer, sizeof (int), 1, &cf);
		CFWrite (&gameData.reactor.states [i].nNextFireTime, sizeof (int), 1, &cf);
		CFWrite (&gameData.reactor.states [i].nDeadObj, sizeof (int), 1, &cf);
		}
// Save the AI state
	AISaveBinState (&cf);

// Save the automap visited info
	CFWrite (gameData.render.mine.bAutomapVisited, sizeof (ushort) * MAX_SEGMENTS, 1, &cf);
	}
CFWrite (&gameData.app.nStateGameId, sizeof (uint), 1, &cf);
CFWrite (&gameStates.app.cheats.bLaserRapidFire, sizeof (int), 1, &cf);
CFWrite (&gameStates.app.bLunacy, sizeof (int), 1, &cf);		//	Yes, writing this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
CFWrite (&gameStates.app.bLunacy, sizeof (int), 1, &cf);
// Save automap marker info
CFWrite (gameData.marker.objects, sizeof (gameData.marker.objects), 1, &cf);
CFWrite (gameData.marker.nOwner, sizeof (gameData.marker.nOwner), 1, &cf);
CFWrite (gameData.marker.szMessage, sizeof (gameData.marker.szMessage), 1, &cf);
CFWrite (&gameData.physics.xAfterburnerCharge, sizeof (fix), 1, &cf);
//save last was super information
CFWrite (&bLastPrimaryWasSuper, sizeof (bLastPrimaryWasSuper), 1, &cf);
CFWrite (&bLastSecondaryWasSuper, sizeof (bLastSecondaryWasSuper), 1, &cf);
//	Save flash effect stuff
CFWrite (&gameData.render.xFlashEffect, sizeof (int), 1, &cf);
CFWrite (&gameData.render.xTimeFlashLastPlayed, sizeof (int), 1, &cf);
CFWrite (&gameStates.ogl.palAdd.red, sizeof (int), 1, &cf);
CFWrite (&gameStates.ogl.palAdd.green, sizeof (int), 1, &cf);
CFWrite (&gameStates.ogl.palAdd.blue, sizeof (int), 1, &cf);
CFWrite (gameData.render.lights.subtracted, sizeof (gameData.render.lights.subtracted [0]), MAX_SEGMENTS, &cf);
CFWrite (&gameStates.app.bFirstSecretVisit, sizeof (gameStates.app.bFirstSecretVisit), 1, &cf);
CFWrite (&gameData.omega.xCharge, sizeof (gameData.omega.xCharge), 1, &cf);
}

//------------------------------------------------------------------------------

void StateSaveNetGame (CFILE *cfP)
{
	int	i, j;

CFWriteByte (netGame.nType, cfP);
CFWriteInt (netGame.nSecurity, cfP);
CFWrite (netGame.szGameName, 1, NETGAME_NAME_LEN + 1, cfP);
CFWrite (netGame.szMissionTitle, 1, MISSION_NAME_LEN + 1, cfP);
CFWrite (netGame.szMissionName, 1, 9, cfP);
CFWriteInt (netGame.nLevel, cfP);
CFWriteByte ((sbyte) netGame.gameMode, cfP);
CFWriteByte ((sbyte) netGame.bRefusePlayers, cfP);
CFWriteByte ((sbyte) netGame.difficulty, cfP);
CFWriteByte ((sbyte) netGame.gameStatus, cfP);
CFWriteByte ((sbyte) netGame.nNumPlayers, cfP);
CFWriteByte ((sbyte) netGame.nMaxPlayers, cfP);
CFWriteByte ((sbyte) netGame.nConnected, cfP);
CFWriteByte ((sbyte) netGame.gameFlags, cfP);
CFWriteByte ((sbyte) netGame.protocolVersion, cfP);
CFWriteByte ((sbyte) netGame.versionMajor, cfP);
CFWriteByte ((sbyte) netGame.versionMinor, cfP);
CFWriteByte ((sbyte) netGame.teamVector, cfP);
CFWriteByte ((sbyte) netGame.DoMegas, cfP);
CFWriteByte ((sbyte) netGame.DoSmarts, cfP);
CFWriteByte ((sbyte) netGame.DoFusions, cfP);
CFWriteByte ((sbyte) netGame.DoHelix, cfP);
CFWriteByte ((sbyte) netGame.DoPhoenix, cfP);
CFWriteByte ((sbyte) netGame.DoAfterburner, cfP);
CFWriteByte ((sbyte) netGame.DoInvulnerability, cfP);
CFWriteByte ((sbyte) netGame.DoCloak, cfP);
CFWriteByte ((sbyte) netGame.DoGauss, cfP);
CFWriteByte ((sbyte) netGame.DoVulcan, cfP);
CFWriteByte ((sbyte) netGame.DoPlasma, cfP);
CFWriteByte ((sbyte) netGame.DoOmega, cfP);
CFWriteByte ((sbyte) netGame.DoSuperLaser, cfP);
CFWriteByte ((sbyte) netGame.DoProximity, cfP);
CFWriteByte ((sbyte) netGame.DoSpread, cfP);
CFWriteByte ((sbyte) netGame.DoSmartMine, cfP);
CFWriteByte ((sbyte) netGame.DoFlash, cfP);
CFWriteByte ((sbyte) netGame.DoGuided, cfP);
CFWriteByte ((sbyte) netGame.DoEarthShaker, cfP);
CFWriteByte ((sbyte) netGame.DoMercury, cfP);
CFWriteByte ((sbyte) netGame.bAllowMarkerView, cfP);
CFWriteByte ((sbyte) netGame.bIndestructibleLights, cfP);
CFWriteByte ((sbyte) netGame.DoAmmoRack, cfP);
CFWriteByte ((sbyte) netGame.DoConverter, cfP);
CFWriteByte ((sbyte) netGame.DoHeadlight, cfP);
CFWriteByte ((sbyte) netGame.DoHoming, cfP);
CFWriteByte ((sbyte) netGame.DoLaserUpgrade, cfP);
CFWriteByte ((sbyte) netGame.DoQuadLasers, cfP);
CFWriteByte ((sbyte) netGame.bShowAllNames, cfP);
CFWriteByte ((sbyte) netGame.BrightPlayers, cfP);
CFWriteByte ((sbyte) netGame.invul, cfP);
CFWriteByte ((sbyte) netGame.FriendlyFireOff, cfP);
for (i = 0; i < 2; i++)
	CFWrite (netGame.szTeamName [i], 1, CALLSIGN_LEN + 1, cfP);		// 18 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	CFWriteInt (netGame.locations [i], cfP);
for (i = 0; i < MAX_PLAYERS; i++)
	for (j = 0; j < MAX_PLAYERS; j++)
		CFWriteShort (netGame.kills [i][j], cfP);			// 128 bytes
CFWriteShort (netGame.nSegmentCheckSum, cfP);				// 2 bytes
for (i = 0; i < 2; i++)
	CFWriteShort (netGame.teamKills [i], cfP);				// 4 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	CFWriteShort (netGame.killed [i], cfP);					// 16 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	CFWriteShort (netGame.playerKills [i], cfP);			// 16 bytes
CFWriteInt (netGame.KillGoal, cfP);							// 4 bytes
CFWriteFix (netGame.xPlayTimeAllowed, cfP);					// 4 bytes
CFWriteFix (netGame.xLevelTime, cfP);							// 4 bytes
CFWriteInt (netGame.controlInvulTime, cfP);				// 4 bytes
CFWriteInt (netGame.monitorVector, cfP);					// 4 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	CFWriteInt (netGame.playerScore [i], cfP);				// 32 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	CFWriteByte ((sbyte) netGame.playerFlags [i], cfP);	// 8 bytes
CFWriteShort (PacketsPerSec (), cfP);					// 2 bytes
CFWriteByte ((sbyte) netGame.bShortPackets, cfP);			// 1 bytes
// 279 bytes
// 355 bytes total
CFWrite (netGame.AuxData, NETGAME_AUX_SIZE, 1, cfP);  // Storage for protocol-specific data (e.g., multicast session and port)
}

//------------------------------------------------------------------------------

void StateSaveNetPlayers (CFILE *cfP)
{
	int	i;

CFWriteByte ((sbyte) netPlayers.nType, cfP);
CFWriteInt (netPlayers.nSecurity, cfP);
for (i = 0; i < MAX_PLAYERS + 4; i++) {
	CFWrite (netPlayers.players [i].callsign, 1, CALLSIGN_LEN + 1, cfP);
	CFWrite (netPlayers.players [i].network.ipx.server, 1, 4, cfP);
	CFWrite (netPlayers.players [i].network.ipx.node, 1, 6, cfP);
	CFWriteByte ((sbyte) netPlayers.players [i].versionMajor, cfP);
	CFWriteByte ((sbyte) netPlayers.players [i].versionMinor, cfP);
	CFWriteByte ((sbyte) netPlayers.players [i].computerType, cfP);
	CFWriteByte (netPlayers.players [i].connected, cfP);
	CFWriteShort ((short) netPlayers.players [i].socket, cfP);
	CFWriteByte ((sbyte) netPlayers.players [i].rank, cfP);
	}
}

//------------------------------------------------------------------------------

void StateSavePlayer (tPlayer *playerP, CFILE *cfP)
{
	int	i;

CFWrite (playerP->callsign, 1, CALLSIGN_LEN + 1, cfP); // The callsign of this tPlayer, for net purposes.
CFWrite (playerP->netAddress, 1, 6, cfP);					// The network address of the player.
CFWriteByte (playerP->connected, cfP);            // Is the tPlayer connected or not?
CFWriteInt (playerP->nObject, cfP);                // What tObject number this tPlayer is. (made an int by mk because it's very often referenced)
CFWriteInt (playerP->nPacketsGot, cfP);         // How many packets we got from them
CFWriteInt (playerP->nPacketsSent, cfP);        // How many packets we sent to them
CFWriteInt ((int) playerP->flags, cfP);           // Powerup flags, see below...
CFWriteFix (playerP->energy, cfP);                // Amount of energy remaining.
CFWriteFix (playerP->shields, cfP);               // shields remaining (protection)
CFWriteByte (playerP->lives, cfP);                // Lives remaining, 0 = game over.
CFWriteByte (playerP->level, cfP);                // Current level tPlayer is playing. (must be signed for secret levels)
CFWriteByte ((sbyte) playerP->laserLevel, cfP);  // Current level of the laser.
CFWriteByte (playerP->startingLevel, cfP);       // What level the tPlayer started on.
CFWriteShort (playerP->nKillerObj, cfP);       // Who killed me.... (-1 if no one)
CFWriteShort ((short) playerP->primaryWeaponFlags, cfP);   // bit set indicates the tPlayer has this weapon.
CFWriteShort ((short) playerP->secondaryWeaponFlags, cfP); // bit set indicates the tPlayer has this weapon.
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	CFWriteShort ((short) playerP->primaryAmmo [i], cfP); // How much ammo of each nType.
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
	CFWriteShort ((short) playerP->secondaryAmmo [i], cfP); // How much ammo of each nType.
#if 1 //for inventory system
CFWriteByte ((sbyte) playerP->nInvuls, cfP);
CFWriteByte ((sbyte) playerP->nCloaks, cfP);
#endif
CFWriteInt (playerP->lastScore, cfP);             // Score at beginning of current level.
CFWriteInt (playerP->score, cfP);                  // Current score.
CFWriteFix (playerP->timeLevel, cfP);             // Level time played
CFWriteFix (playerP->timeTotal, cfP);             // Game time played (high word = seconds)
if (playerP->cloakTime == 0x7fffffff)				// cloak cheat active
	CFWriteFix (playerP->cloakTime, cfP);			// Time invulnerable
else
	CFWriteFix (playerP->cloakTime - gameData.time.xGame, cfP);      // Time invulnerable
if (playerP->invulnerableTime == 0x7fffffff)		// invul cheat active
	CFWriteFix (playerP->invulnerableTime, cfP);      // Time invulnerable
else
	CFWriteFix (playerP->invulnerableTime - gameData.time.xGame, cfP);      // Time invulnerable
CFWriteShort (playerP->nKillGoalCount, cfP);          // Num of players killed this level
CFWriteShort (playerP->netKilledTotal, cfP);       // Number of times killed total
CFWriteShort (playerP->netKillsTotal, cfP);        // Number of net kills total
CFWriteShort (playerP->numKillsLevel, cfP);        // Number of kills this level
CFWriteShort (playerP->numKillsTotal, cfP);        // Number of kills total
CFWriteShort (playerP->numRobotsLevel, cfP);       // Number of initial robots this level
CFWriteShort (playerP->numRobotsTotal, cfP);       // Number of robots total
CFWriteShort ((short) playerP->hostages.nRescued, cfP); // Total number of hostages rescued.
CFWriteShort ((short) playerP->hostages.nTotal, cfP);         // Total number of hostages.
CFWriteByte ((sbyte) playerP->hostages.nOnBoard, cfP);      // Number of hostages on ship.
CFWriteByte ((sbyte) playerP->hostages.nLevel, cfP);         // Number of hostages on this level.
CFWriteFix (playerP->homingObjectDist, cfP);     // Distance of nearest homing tObject.
CFWriteByte (playerP->hoursLevel, cfP);            // Hours played (since timeTotal can only go up to 9 hours)
CFWriteByte (playerP->hoursTotal, cfP);            // Hours played (since timeTotal can only go up to 9 hours)
}

//------------------------------------------------------------------------------

void StateSaveObject (tObject *objP, CFILE *cfP)
{
CFWriteInt (objP->info.nSignature, cfP);      
CFWriteByte ((sbyte) objP->info.nType, cfP); 
CFWriteByte ((sbyte) objP->info.nId, cfP);
CFWriteShort (objP->info.nNextInSeg, cfP);
CFWriteShort (objP->info.nPrevInSeg, cfP);
CFWriteByte ((sbyte) objP->info.controlType, cfP);
CFWriteByte ((sbyte) objP->info.movementType, cfP);
CFWriteByte ((sbyte) objP->info.renderType, cfP);
CFWriteByte ((sbyte) objP->info.nFlags, cfP);
CFWriteShort (objP->info.nSegment, cfP);
CFWriteShort (objP->info.nAttachedObj, cfP);
CFWriteVector (OBJPOS (objP)->vPos, cfP);     
CFWriteMatrix (OBJPOS (objP)->mOrient, cfP);  
CFWriteFix (objP->info.xSize, cfP); 
CFWriteFix (objP->info.xShields, cfP);
CFWriteVector (objP->info.vLastPos, cfP);  
CFWriteByte (objP->info.contains.nType, cfP); 
CFWriteByte (objP->info.contains.nId, cfP);   
CFWriteByte (objP->info.contains.nCount, cfP);
CFWriteByte (objP->info.nCreator, cfP);
CFWriteFix (objP->info.xLifeLeft, cfP);   
if (objP->info.movementType == MT_PHYSICS) {
	CFWriteVector (objP->mType.physInfo.velocity, cfP);   
	CFWriteVector (objP->mType.physInfo.thrust, cfP);     
	CFWriteFix (objP->mType.physInfo.mass, cfP);       
	CFWriteFix (objP->mType.physInfo.drag, cfP);       
	CFWriteFix (objP->mType.physInfo.brakes, cfP);     
	CFWriteVector (objP->mType.physInfo.rotVel, cfP);     
	CFWriteVector (objP->mType.physInfo.rotThrust, cfP);  
	CFWriteFixAng (objP->mType.physInfo.turnRoll, cfP);   
	CFWriteShort ((short) objP->mType.physInfo.flags, cfP);      
	}
else if (objP->info.movementType == MT_SPINNING) {
	CFWriteVector(objP->mType.spinRate, cfP);  
	}
switch (objP->info.controlType) {
	case CT_WEAPON:
		CFWriteShort (objP->cType.laserInfo.parent.nType, cfP);
		CFWriteShort (objP->cType.laserInfo.parent.nObject, cfP);
		CFWriteInt (objP->cType.laserInfo.parent.nSignature, cfP);
		CFWriteFix (objP->cType.laserInfo.xCreationTime, cfP);
		if (objP->cType.laserInfo.nLastHitObj)
			CFWriteShort (gameData.objs.nHitObjects [OBJ_IDX (objP) * MAX_HIT_OBJECTS + objP->cType.laserInfo.nLastHitObj - 1], cfP);
		else
			CFWriteShort (-1, cfP);
		CFWriteShort (objP->cType.laserInfo.nHomingTarget, cfP);
		CFWriteFix (objP->cType.laserInfo.xScale, cfP);
		break;

	case CT_EXPLOSION:
		CFWriteFix (objP->cType.explInfo.nSpawnTime, cfP);
		CFWriteFix (objP->cType.explInfo.nDeleteTime, cfP);
		CFWriteShort (objP->cType.explInfo.nDeleteObj, cfP);
		CFWriteShort (objP->cType.explInfo.attached.nParent, cfP);
		CFWriteShort (objP->cType.explInfo.attached.nPrev, cfP);
		CFWriteShort (objP->cType.explInfo.attached.nNext, cfP);
		break;

	case CT_AI:
		CFWriteByte ((sbyte) objP->cType.aiInfo.behavior, cfP);
		CFWrite (objP->cType.aiInfo.flags, 1, MAX_AI_FLAGS, cfP);
		CFWriteShort (objP->cType.aiInfo.nHideSegment, cfP);
		CFWriteShort (objP->cType.aiInfo.nHideIndex, cfP);
		CFWriteShort (objP->cType.aiInfo.nPathLength, cfP);
		CFWriteByte (objP->cType.aiInfo.nCurPathIndex, cfP);
		CFWriteByte (objP->cType.aiInfo.bDyingSoundPlaying, cfP);
		CFWriteShort (objP->cType.aiInfo.nDangerLaser, cfP);
		CFWriteInt (objP->cType.aiInfo.nDangerLaserSig, cfP);
		CFWriteFix (objP->cType.aiInfo.xDyingStartTime, cfP);
		break;

	case CT_LIGHT:
		CFWriteFix (objP->cType.lightInfo.intensity, cfP);
		break;

	case CT_POWERUP:
		CFWriteInt (objP->cType.powerupInfo.nCount, cfP);
		CFWriteFix (objP->cType.powerupInfo.xCreationTime, cfP);
		CFWriteInt (objP->cType.powerupInfo.nFlags, cfP);
		break;
	}
switch (objP->info.renderType) {
	case RT_MORPH:
	case RT_POLYOBJ: {
		int i;
		CFWriteInt (objP->rType.polyObjInfo.nModel, cfP);
		for (i = 0; i < MAX_SUBMODELS; i++)
			CFWriteAngVec (objP->rType.polyObjInfo.animAngles [i], cfP);
		CFWriteInt (objP->rType.polyObjInfo.nSubObjFlags, cfP);
		CFWriteInt (objP->rType.polyObjInfo.nTexOverride, cfP);
		CFWriteInt (objP->rType.polyObjInfo.nAltTextures, cfP);
		break;
		}
	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
	case RT_THRUSTER:
		CFWriteInt (objP->rType.vClipInfo.nClipIndex, cfP);
		CFWriteFix (objP->rType.vClipInfo.xFrameTime, cfP);
		CFWriteByte (objP->rType.vClipInfo.nCurFrame, cfP);
		break;

	case RT_LASER:
		break;
	}
}

//------------------------------------------------------------------------------

void StateSaveWall (tWall *wallP, CFILE *cfP)
{
CFWriteInt (wallP->nSegment, cfP);
CFWriteInt (wallP->nSide, cfP);
CFWriteFix (wallP->hps, cfP);    
CFWriteInt (wallP->nLinkedWall, cfP);
CFWriteByte ((sbyte) wallP->nType, cfP);       
CFWriteByte ((sbyte) wallP->flags, cfP);      
CFWriteByte ((sbyte) wallP->state, cfP);      
CFWriteByte ((sbyte) wallP->nTrigger, cfP);    
CFWriteByte (wallP->nClip, cfP);   
CFWriteByte ((sbyte) wallP->keys, cfP);       
CFWriteByte (wallP->controllingTrigger, cfP);
CFWriteByte (wallP->cloakValue, cfP); 
}

//------------------------------------------------------------------------------

void StateSaveExplWall (tExplWall *wallP, CFILE *cfP)
{
CFWriteInt (wallP->nSegment, cfP);
CFWriteInt (wallP->nSide, cfP);
CFWriteFix (wallP->time, cfP);    
}

//------------------------------------------------------------------------------

void StateSaveCloakingWall (tCloakingWall *wallP, CFILE *cfP)
{
	int	i;

CFWriteShort (wallP->nFrontWall, cfP);
CFWriteShort (wallP->nBackWall, cfP); 
for (i = 0; i < 4; i++) {
	CFWriteFix (wallP->front_ls [i], cfP); 
	CFWriteFix (wallP->back_ls [i], cfP);
	}
CFWriteFix (wallP->time, cfP);    
}

//------------------------------------------------------------------------------

void StateSaveActiveDoor (tActiveDoor *doorP, CFILE *cfP)
{
	int	i;

CFWriteInt (doorP->nPartCount, cfP);
for (i = 0; i < 2; i++) {
	CFWriteShort (doorP->nFrontWall [i], cfP);
	CFWriteShort (doorP->nBackWall [i], cfP);
	}
CFWriteFix (doorP->time, cfP);    
}

//------------------------------------------------------------------------------

void StateSaveTrigger (tTrigger *triggerP, CFILE *cfP)
{
	int	i;

CFWriteByte ((sbyte) triggerP->nType, cfP); 
CFWriteByte ((sbyte) triggerP->flags, cfP); 
CFWriteByte (triggerP->nLinks, cfP);
CFWriteFix (triggerP->value, cfP);
CFWriteFix (triggerP->time, cfP);
for (i = 0; i < MAX_TRIGGER_TARGETS; i++) {
	CFWriteShort (triggerP->nSegment [i], cfP);
	CFWriteShort (triggerP->nSide [i], cfP);
	}
}

//------------------------------------------------------------------------------

void StateSaveObjTriggerRef (tObjTriggerRef *refP, CFILE *cfP)
{
CFWriteShort (refP->prev, cfP);
CFWriteShort (refP->next, cfP);
CFWriteShort (refP->nObject, cfP);
}

//------------------------------------------------------------------------------

void StateSaveMatCen (tMatCenInfo *matcenP, CFILE *cfP)
{
	int	i;

for (i = 0; i < 2; i++)
	CFWriteInt (matcenP->objFlags [i], cfP);
CFWriteFix (matcenP->xHitPoints, cfP);
CFWriteFix (matcenP->xInterval, cfP);
CFWriteShort (matcenP->nSegment, cfP);
CFWriteShort (matcenP->nFuelCen, cfP);
}

//------------------------------------------------------------------------------

void StateSaveFuelCen (tFuelCenInfo *fuelcenP, CFILE *cfP)
{
CFWriteInt (fuelcenP->nType, cfP);
CFWriteInt (fuelcenP->nSegment, cfP);
CFWriteByte (fuelcenP->bFlag, cfP);
CFWriteByte (fuelcenP->bEnabled, cfP);
CFWriteByte (fuelcenP->nLives, cfP);
CFWriteFix (fuelcenP->xCapacity, cfP);
CFWriteFix (fuelcenP->xMaxCapacity, cfP);
CFWriteFix (fuelcenP->xTimer, cfP);
CFWriteFix (fuelcenP->xDisableTime, cfP);
CFWriteVector(fuelcenP->vCenter, cfP);
}

//------------------------------------------------------------------------------

void StateSaveReactorTrigger (tReactorTriggers *triggerP, CFILE *cfP)
{
	int	i;

CFWriteShort (triggerP->nLinks, cfP);
for (i = 0; i < MAX_CONTROLCEN_LINKS; i++) {
	CFWriteShort (triggerP->nSegment [i], cfP);
	CFWriteShort (triggerP->nSide [i], cfP);
	}
}

//------------------------------------------------------------------------------

void StateSaveSpawnPoint (int i, CFILE *cfP)
{
#if DBG
i = CFTell (cfP);
#endif
CFWriteVector (gameData.multiplayer.playerInit [i].position.vPos, cfP);     
CFWriteMatrix (gameData.multiplayer.playerInit [i].position.mOrient, cfP);  
CFWriteShort (gameData.multiplayer.playerInit [i].nSegment, cfP);
CFWriteShort (gameData.multiplayer.playerInit [i].nSegType, cfP);
}

//------------------------------------------------------------------------------

DBG (static int fPos);

void StateSaveUniGameData (CFILE *cfP, int bBetweenLevels)
{
	int		i, j;
	short		nObjsWithTrigger, nObject, nFirstTrigger;
	tObject	*objP;

CFWriteInt (gameData.segs.nMaxSegments, cfP);
// Save the Between levels flag...
CFWriteInt (bBetweenLevels, cfP);
// Save the mission info...
CFWrite (gameData.missions.list + gameData.missions.nCurrentMission, sizeof (char), 9, cfP);
//Save level info
CFWriteInt (gameData.missions.nCurrentLevel, cfP);
CFWriteInt (gameData.missions.nNextLevel, cfP);
//Save gameData.time.xGame
CFWriteFix (gameData.time.xGame, cfP);
// If coop save, save all
if (IsCoopGame) {
	CFWriteInt (gameData.app.nStateGameId, cfP);
	StateSaveNetGame (cfP);
	DBG (fPos = CFTell (cfP));
	StateSaveNetPlayers (cfP);
	DBG (fPos = CFTell (cfP));
	CFWriteInt (gameData.multiplayer.nPlayers, cfP);
	CFWriteInt (gameData.multiplayer.nLocalPlayer, cfP);
	for (i = 0; i < gameData.multiplayer.nPlayers; i++)
		StateSavePlayer (gameData.multiplayer.players + i, cfP);
	DBG (fPos = CFTell (cfP));
	}
//Save tPlayer info
StateSavePlayer (gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer, cfP);
// Save the current weapon info
CFWriteByte (gameData.weapons.nPrimary, cfP);
CFWriteByte (gameData.weapons.nSecondary, cfP);
// Save the difficulty level
CFWriteInt (gameStates.app.nDifficultyLevel, cfP);
// Save cheats enabled
CFWriteInt (gameStates.app.cheats.bEnabled, cfP);
for (i = 0; i < 2; i++) {
	CFWriteInt (F2X (gameStates.gameplay.slowmo [i].fSpeed), cfP);
	CFWriteInt (gameStates.gameplay.slowmo [i].nState, cfP);
	}
for (i = 0; i < MAX_PLAYERS; i++)
	CFWriteInt (gameData.multiplayer.weaponStates [i].bTripleFusion, cfP);
if (!bBetweenLevels)	{
//Finish all morph OBJECTS
	FORALL_OBJS (objP, i) {
	if (objP->info.nType == OBJ_NONE) 
			continue;
		if (objP->info.nType == OBJ_CAMERA)
			objP->info.position.mOrient = gameData.cameras.cameras [gameData.objs.cameraRef [OBJ_IDX (objP)]].orient;
		else if (objP->info.renderType == RT_MORPH) {
			tMorphInfo *md = MorphFindData (objP);
			if (md) {
				tObject *mdObjP = md->objP;
				mdObjP->info.controlType = md->saveControlType;
				mdObjP->info.movementType = md->saveMovementType;
				mdObjP->info.renderType = RT_POLYOBJ;
				mdObjP->mType.physInfo = md->savePhysInfo;
				md->objP = NULL;
				} 
			else {						//maybe loaded half-morphed from disk
				KillObject (objP);
				objP->info.renderType = RT_POLYOBJ;
				objP->info.controlType = CT_NONE;
				objP->info.movementType = MT_NONE;
				}
			}
		}
	DBG (fPos = CFTell (cfP));
//Save tObject info
	i = gameData.objs.nLastObject [0] + 1;
	CFWriteInt (i, cfP);
	for (j = 0; j < i; j++)
		StateSaveObject (OBJECTS + j, cfP);
	DBG (fPos = CFTell (cfP));
//Save tWall info
	i = gameData.walls.nWalls;
	CFWriteInt (i, cfP);
	for (j = 0; j < i; j++)
		StateSaveWall (gameData.walls.walls + j, cfP);
	DBG (fPos = CFTell (cfP));
//Save exploding wall info
	i = MAX_EXPLODING_WALLS;
	CFWriteInt (i, cfP);
	for (j = 0; j < i; j++)
		StateSaveExplWall (gameData.walls.explWalls + j, cfP);
	DBG (fPos = CFTell (cfP));
//Save door info
	i = gameData.walls.nOpenDoors;
	CFWriteInt (i, cfP);
	for (j = 0; j < i; j++)
		StateSaveActiveDoor (gameData.walls.activeDoors + j, cfP);
	DBG (fPos = CFTell (cfP));
//Save cloaking tWall info
	i = gameData.walls.nCloaking;
	CFWriteInt (i, cfP);
	for (j = 0; j < i; j++)
		StateSaveCloakingWall (gameData.walls.cloaking + j, cfP);
	DBG (fPos = CFTell (cfP));
//Save tTrigger info
	CFWriteInt (gameData.trigs.nTriggers, cfP);
	for (i = 0; i < gameData.trigs.nTriggers; i++)
		StateSaveTrigger (gameData.trigs.triggers + i, cfP);
	DBG (fPos = CFTell (cfP));
	CFWriteInt (gameData.trigs.nObjTriggers, cfP);
	if (!gameData.trigs.nObjTriggers)
		CFWriteShort (0, cfP);
	else {
		for (i = 0; i < gameData.trigs.nObjTriggers; i++)
			StateSaveTrigger (gameData.trigs.objTriggers + i, cfP);
		for (i = 0; i < gameData.trigs.nObjTriggers; i++)
			StateSaveObjTriggerRef (gameData.trigs.objTriggerRefs + i, cfP);
		nObjsWithTrigger = 0;
		FORALL_OBJS (objP, nObject) {
			nObject = OBJ_IDX (objP);
			nFirstTrigger = gameData.trigs.firstObjTrigger [nObject];
			if ((nFirstTrigger >= 0) && (nFirstTrigger < gameData.trigs.nObjTriggers))
				nObjsWithTrigger++;
			}
		CFWriteShort (nObjsWithTrigger, cfP);
		FORALL_OBJS (objP, nObject) {
			nObject = OBJ_IDX (objP);
			nFirstTrigger = gameData.trigs.firstObjTrigger [nObject];
			if ((nFirstTrigger >= 0) && (nFirstTrigger < gameData.trigs.nObjTriggers)) {
				CFWriteShort (nObject, cfP);
				CFWriteShort (nFirstTrigger, cfP);
				}
			}
		}
	DBG (fPos = CFTell (cfP));
//Save tmap info
	for (i = 0; i <= gameData.segs.nLastSegment; i++) {
		for (j = 0; j < 6; j++)	{
			ushort nWall = WallNumI ((short) i, (short) j);
			CFWriteShort (nWall, cfP);
			CFWriteShort (gameData.segs.segments [i].sides [j].nBaseTex, cfP);
			CFWriteShort (gameData.segs.segments [i].sides [j].nOvlTex | (gameData.segs.segments [i].sides [j].nOvlOrient << 14), cfP);
			}
		}
	DBG (fPos = CFTell (cfP));
// Save the fuelcen info
	CFWriteInt (gameData.reactor.bDestroyed, cfP);
	CFWriteFix (gameData.reactor.countdown.nTimer, cfP);
	DBG (fPos = CFTell (cfP));
	CFWriteInt (gameData.matCens.nBotCenters, cfP);
	for (i = 0; i < gameData.matCens.nBotCenters; i++)
		StateSaveMatCen (gameData.matCens.botGens + i, cfP);
	CFWriteInt (gameData.matCens.nEquipCenters, cfP);
	for (i = 0; i < gameData.matCens.nEquipCenters; i++)
		StateSaveMatCen (gameData.matCens.equipGens + i, cfP);
	StateSaveReactorTrigger (&gameData.reactor.triggers, cfP);
	CFWriteInt (gameData.matCens.nFuelCenters, cfP);
	for (i = 0; i < gameData.matCens.nFuelCenters; i++)
		StateSaveFuelCen (gameData.matCens.fuelCenters + i, cfP);
	DBG (fPos = CFTell (cfP));
// Save the control cen info
	CFWriteInt (gameData.reactor.bPresent, cfP);
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		CFWriteInt (gameData.reactor.states [i].nObject, cfP);
		CFWriteInt (gameData.reactor.states [i].bHit, cfP);
		CFWriteInt (gameData.reactor.states [i].bSeenPlayer, cfP);
		CFWriteInt (gameData.reactor.states [i].nNextFireTime, cfP);
		CFWriteInt (gameData.reactor.states [i].nDeadObj, cfP);
		}
	DBG (fPos = CFTell (cfP));
// Save the AI state
	AISaveUniState (cfP);

	DBG (fPos = CFTell (cfP));
// Save the automap visited info
	for (i = 0; i < MAX_SEGMENTS; i++)
		CFWriteShort (gameData.render.mine.bAutomapVisited [i], cfP);
	DBG (fPos = CFTell (cfP));
	}
CFWriteInt ((int) gameData.app.nStateGameId, cfP);
CFWriteInt (gameStates.app.cheats.bLaserRapidFire, cfP);
CFWriteInt (gameStates.app.bLunacy, cfP);		//	Yes, writing this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
CFWriteInt (gameStates.app.bLunacy, cfP);
// Save automap marker info
for (i = 0; i < NUM_MARKERS; i++)
	CFWriteShort (gameData.marker.objects [i], cfP);
CFWrite (gameData.marker.nOwner, sizeof (gameData.marker.nOwner), 1, cfP);
CFWrite (gameData.marker.szMessage, sizeof (gameData.marker.szMessage), 1, cfP);
CFWriteFix (gameData.physics.xAfterburnerCharge, cfP);
//save last was super information
CFWrite (bLastPrimaryWasSuper, sizeof (bLastPrimaryWasSuper), 1, cfP);
CFWrite (bLastSecondaryWasSuper, sizeof (bLastSecondaryWasSuper), 1, cfP);
//	Save flash effect stuff
CFWriteFix (gameData.render.xFlashEffect, cfP);
CFWriteFix (gameData.render.xTimeFlashLastPlayed, cfP);
CFWriteShort (gameStates.ogl.palAdd.red, cfP);
CFWriteShort (gameStates.ogl.palAdd.green, cfP);
CFWriteShort (gameStates.ogl.palAdd.blue, cfP);
CFWrite (gameData.render.lights.subtracted, sizeof (gameData.render.lights.subtracted [0]), MAX_SEGMENTS, cfP);
CFWriteInt (gameStates.app.bFirstSecretVisit, cfP);
CFWriteFix (gameData.omega.xCharge [0], cfP);
CFWriteShort (gameData.missions.nEnteredFromLevel, cfP);
for (i = 0; i < MAX_PLAYERS; i++)
	StateSaveSpawnPoint (i, cfP);
}

//------------------------------------------------------------------------------

int StateSaveAllSub (const char *filename, const char *szDesc, int bBetweenLevels)
{
	int			i;
	CFILE			cf;
	gsrCanvas	*cnv;

Assert (bBetweenLevels == 0);	//between levels save ripped out
StopTime ();
if (!CFOpen (&cf, filename, gameFolders.szSaveDir, "wb", 0)) {
	if (!IsMultiGame)
		ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_SAVE_ERROR2);
	StartTime (1);
	return 0;
	}

//Save id
CFWrite (dgss_id, sizeof (char) * 4, 1, &cf);
//Save sgVersion
i = STATE_VERSION;
CFWrite (&i, sizeof (int), 1, &cf);
//Save description
CFWrite (szDesc, sizeof (char) * DESC_LENGTH, 1, &cf);
// Save the current screen shot...
if ((cnv = GrCreateCanvas (THUMBNAIL_LW, THUMBNAIL_LH))) {
		grsBitmap	bm;
		int			k, x, y;
		gsrCanvas * cnv_save;
		cnv_save = grdCurCanv;
	
	GrSetCurrentCanvas (cnv);
	gameStates.render.nFrameFlipFlop = !gameStates.render.nFrameFlipFlop;
	
#if 1
	if (gameStates.ogl.nDrawBuffer == GL_BACK) {
		gameStates.render.nFrameFlipFlop = !gameStates.render.nFrameFlipFlop;
		GameRenderFrame ();
		}
	else
		RenderFrame (0, 0);
#endif
	bm.bmProps.w = (grdCurScreen->scWidth / THUMBNAIL_LW) * THUMBNAIL_LW;
	bm.bmProps.h = bm.bmProps.w * 3 / 5;	//force 5:3 aspect ratio
	if (bm.bmProps.h > grdCurScreen->scHeight) {
		bm.bmProps.h = (grdCurScreen->scHeight / THUMBNAIL_LH) * THUMBNAIL_LH;
		bm.bmProps.w = bm.bmProps.h * 5 / 3;
		}
	x = (grdCurScreen->scWidth - bm.bmProps.w) / 2;
	y = (grdCurScreen->scHeight - bm.bmProps.h) / 2;
	bm.bmBPP = 3;
	bm.bmProps.rowSize = bm.bmProps.w * bm.bmBPP;
	bm.bmTexBuf = (ubyte *) D2_ALLOC (bm.bmProps.w * bm.bmProps.h * bm.bmBPP);
	//glDisable (GL_TEXTURE_2D);
	OglSetReadBuffer (GL_FRONT, 1);
	glReadPixels (x, y, bm.bmProps.w, bm.bmProps.h, GL_RGB, GL_UNSIGNED_BYTE, bm.bmTexBuf);
	// do a nice, half-way smart (by merging pixel groups using their average color) image resize
	ShrinkTGA (&bm, bm.bmProps.w / THUMBNAIL_LW, bm.bmProps.h / THUMBNAIL_LH, 0);
	GrPaletteStepLoad (NULL);
	// convert the resized TGA to bmp
	for (y = 0; y < THUMBNAIL_LH; y++) {
		i = y * THUMBNAIL_LW * 3;
		k = (THUMBNAIL_LH - y - 1) * THUMBNAIL_LW;
		for (x = 0; x < THUMBNAIL_LW; x++, k++, i += 3)
			cnv->cvBitmap.bmTexBuf [k] = GrFindClosestColor (gamePalette, bm.bmTexBuf [i] / 4, bm.bmTexBuf [i+1] / 4, bm.bmTexBuf [i+2] / 4);
			}
	GrPaletteStepLoad (NULL);
	D2_FREE (bm.bmTexBuf);
	CFWrite (cnv->cvBitmap.bmTexBuf, THUMBNAIL_LW * THUMBNAIL_LH, 1, &cf);
	GrSetCurrentCanvas (cnv_save);
	GrFreeCanvas (cnv);
	CFWrite (gamePalette, 3, 256, &cf);
	}
else	{
 	ubyte color = 0;
 	for (i = 0; i < THUMBNAIL_LW * THUMBNAIL_LH; i++)
		CFWrite (&color, sizeof (ubyte), 1, &cf);
	}
StateSaveUniGameData (&cf, bBetweenLevels);
if (CFError (&cf)) {
	if (!IsMultiGame) {
		ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_SAVE_ERROR);
		CFClose (&cf);
		CFDelete (filename, gameFolders.szSaveDir);
		}
	}
else 
	CFClose (&cf);
StartTime (1);
return 1;
}

//	-----------------------------------------------------------------------------------
//	Set the tPlayer's position from the globals gameData.segs.secret.nReturnSegment and gameData.segs.secret.returnOrient.
void SetPosFromReturnSegment (int bRelink)
{
	int	nPlayerObj = LOCALPLAYER.nObject;

COMPUTE_SEGMENT_CENTER_I (&OBJECTS [nPlayerObj].info.position.vPos, 
							     gameData.segs.secret.nReturnSegment);
if (bRelink)
	RelinkObjToSeg (nPlayerObj, gameData.segs.secret.nReturnSegment);
ResetPlayerObject ();
OBJECTS [nPlayerObj].info.position.mOrient = gameData.segs.secret.returnOrient;
}

//	-----------------------------------------------------------------------------------

int StateRestoreAll (int bInGame, int bSecretRestore, int bQuick, const char *pszFilenameOverride)
{
	char	filename [128], saveFilename [128];
	int	i, nFile = -1;

if (IsMultiGame) {
#	ifdef MULTI_SAVE
	MultiInitiateRestoreGame ();
#	endif
	return 0;
	}
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDStopRecording ();
if (gameData.demo.nState != ND_STATE_NORMAL)
	return 0;
StopTime ();
gameData.app.bGamePaused = 1;
if (bQuick) {
	sprintf (filename, "%s.quick", LOCALPLAYER.callsign);
	if (!CFExist (filename, gameFolders.szSaveDir, 0))
		bQuick = 0;
	}
if (!bQuick) {
	if (pszFilenameOverride) {
		strcpy (filename, pszFilenameOverride);
		nFile = NUM_SAVES+1;		//	So we don't tTrigger autosave
		}
	else if (!(nFile = StateGetRestoreFile (filename, 0))) {
		gameData.app.bGamePaused = 0;
		StartTime (1);
		return 0;
		}
	//	MK, 1/1/96
	//	If not in multiplayer, do special secret level stuff.
	//	If Nsecret.sgc (where N = nFile) exists, then copy it to secret.sgc.
	//	If it doesn't exist, then delete secret.sgc

	if (!bSecretRestore && !IsMultiGame) {
		int	rval;
		char	temp_fname [32], fc;

		if (nFile != -1) {
			if (nFile >= 10)
				fc = (nFile-10) + 'a';
			else
				fc = '0' + nFile;
			sprintf (temp_fname, "%csecret.sgc", fc);
			if (CFExist (temp_fname, gameFolders.szSaveDir, 0)) {
				rval = copy_file (temp_fname, SECRETC_FILENAME);
				Assert (rval == 0);	//	Oops, error copying temp_fname to secret.sgc!
				}
			else
				CFDelete (SECRETC_FILENAME, gameFolders.szSaveDir);
			}
		}
		//	Changed, 11/15/95, MK, don't to autosave if restoring from main menu.
	if ((nFile != (NUM_SAVES + 1)) && bInGame) {
		sprintf (saveFilename, "%s.sg%x", LOCALPLAYER.callsign, NUM_SAVES);
		StateSaveAll (!bInGame, bSecretRestore, 0, saveFilename);
		}
	if (!bSecretRestore && bInGame) {
		int choice = ExecMessageBox (NULL, NULL, 2, TXT_YES, TXT_NO, TXT_CONFIRM_LOAD);
		if (choice != 0) {
			gameData.app.bGamePaused = 0;
			StartTime (1);
			return 0;
			}
		}
	}
gameStates.app.bGameRunning = 0;
i = StateRestoreAllSub (filename, 0, bSecretRestore);
gameData.app.bGamePaused = 0;
/*---*/PrintLog ("   rebuilding OpenGL texture data\n");
/*---*/PrintLog ("      rebuilding effects\n");
if (i) {
	RebuildRenderContext (1);
	if (bQuick)
		HUDInitMessage (TXT_QUICKLOAD);
	}
StartTime (1);
return i;
}

//------------------------------------------------------------------------------

int CFReadBoundedInt (int nMax, int *nVal, CFILE *cfP)
{
	int	i;

i = CFReadInt (cfP);
if ((i < 0) || (i > nMax)) {
	Warning (TXT_SAVE_CORRUPT);
	//CFClose (cfP);
	return 1;
	}
*nVal = i;
return 0;
}

//------------------------------------------------------------------------------

int StateLoadMission (CFILE *cfP)
{
	char	szMission [16];
	int	i, nVersionFilter = gameOpts->app.nVersionFilter;

CFRead (szMission, sizeof (char), 9, cfP);
szMission [9] = '\0';
gameOpts->app.nVersionFilter = 3;
i = LoadMissionByName (szMission, -1);
gameOpts->app.nVersionFilter = nVersionFilter;
if (i)
	return 1;
ExecMessageBox (NULL, NULL, 1, "Ok", TXT_MSN_LOAD_ERROR, szMission);
//CFClose (cfP);
return 0;
}

//------------------------------------------------------------------------------

void StateRestoreMultiGame (char *pszOrgCallSign, int bMulti, int bSecretRestore)
{
if (bMulti)
	strcpy (pszOrgCallSign, LOCALPLAYER.callsign);
else {
	gameData.app.nGameMode = GM_NORMAL;
	SetFunctionMode (FMODE_GAME);
	ChangePlayerNumTo (0);
	strcpy (pszOrgCallSign, gameData.multiplayer.players [0].callsign);
	gameData.multiplayer.nPlayers = 1;
	if (!bSecretRestore) {
		InitMultiPlayerObject ();	//make sure tPlayer's tObject set up
		InitPlayerStatsGame ();		//clear all stats
		}
	}
}

//------------------------------------------------------------------------------

int StateSetServerPlayer (tPlayer *restoredPlayers, int nPlayers, const char *pszServerCallSign,
								  int *pnOtherObjNum, int *pnServerObjNum)
{
	int	i,
			nServerPlayer = -1,
			nOtherObjNum = -1, 
			nServerObjNum = -1;

if (gameStates.multi.nGameType >= IPX_GAME) {
	if (gameStates.multi.bServer || NetworkIAmMaster ())
		nServerPlayer = gameData.multiplayer.nLocalPlayer;
	else {
		nServerPlayer = -1;
		for (i = 0; i < nPlayers; i++)
			if (!strcmp (pszServerCallSign, netPlayers.players [i].callsign)) {
				nServerPlayer = i;
				break;
				}
		}
	if (nServerPlayer > 0) {
		nOtherObjNum = restoredPlayers [0].nObject;
		nServerObjNum = restoredPlayers [nServerPlayer].nObject;
		{
		tNetPlayerInfo h = netPlayers.players [0];
		netPlayers.players [0] = netPlayers.players [nServerPlayer];
		netPlayers.players [nServerPlayer] = h;
		}
		{
		tPlayer h = restoredPlayers [0];
		restoredPlayers [0] = restoredPlayers [nServerPlayer];
		restoredPlayers [nServerPlayer] = h;
		}
		if (gameStates.multi.bServer || NetworkIAmMaster ())
			gameData.multiplayer.nLocalPlayer = 0;
		else if (!gameData.multiplayer.nLocalPlayer)
			gameData.multiplayer.nLocalPlayer = nServerPlayer;
		}
#if 0
	memcpy (netPlayers.players [gameData.multiplayer.nLocalPlayer].network.ipx.node, 
			 IpxGetMyLocalAddress (), 6);
	*((ushort *) (netPlayers.players [gameData.multiplayer.nLocalPlayer].network.ipx.node + 4)) = 
		htons (*((ushort *) (netPlayers.players [gameData.multiplayer.nLocalPlayer].network.ipx.node + 4)));
#endif
	}
*pnOtherObjNum = nOtherObjNum;
*pnServerObjNum = nServerObjNum;
return nServerPlayer;
}

//------------------------------------------------------------------------------

void StateGetConnectedPlayers (tPlayer *restoredPlayers, int nPlayers)
{
	int	i, j;

for (i = 0; i < nPlayers; i++) {
	for (j = 0; j < nPlayers; j++) {
		if (!stricmp (restoredPlayers [i].callsign, gameData.multiplayer.players [j].callsign)) {
			if (gameData.multiplayer.players [j].connected) {
				if (gameStates.multi.nGameType == UDP_GAME) {
					memcpy (restoredPlayers [i].netAddress, gameData.multiplayer.players [j].netAddress, 
							  sizeof (gameData.multiplayer.players [j].netAddress));
					memcpy (netPlayers.players [i].network.ipx.node, gameData.multiplayer.players [j].netAddress, 
							  sizeof (gameData.multiplayer.players [j].netAddress));
					}
				restoredPlayers [i].connected = 1;
				break;
				}
			}
		}
	}
for (i = 0; i < MAX_PLAYERS; i++) {
	if (!restoredPlayers [i].connected) {
		memset (restoredPlayers [i].netAddress, 0xFF, sizeof (restoredPlayers [i].netAddress));
		memset (netPlayers.players [i].network.ipx.node, 0xFF, sizeof (netPlayers.players [i].network.ipx.node));
		}
	}
memcpy (gameData.multiplayer.players, restoredPlayers, sizeof (tPlayer) * nPlayers);
gameData.multiplayer.nPlayers = nPlayers;
if (NetworkIAmMaster ()) {
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (i == gameData.multiplayer.nLocalPlayer)
			continue;
   	gameData.multiplayer.players [i].connected = 0;
		}
	}
}

//------------------------------------------------------------------------------

void StateFixNetworkObjects (int nServerPlayer, int nOtherObjNum, int nServerObjNum)
{
if (IsMultiGame && (gameStates.multi.nGameType >= IPX_GAME) && (nServerPlayer > 0)) {
	tObject h = OBJECTS [nServerObjNum];
	OBJECTS [nServerObjNum] = OBJECTS [nOtherObjNum];
	OBJECTS [nOtherObjNum] = h;
	OBJECTS [nServerObjNum].info.nId = nServerObjNum;
	OBJECTS [nOtherObjNum].info.nId = 0;
	if (gameData.multiplayer.nLocalPlayer == nServerObjNum) {
		OBJECTS [nServerObjNum].info.controlType = CT_FLYING;
		OBJECTS [nOtherObjNum].info.controlType = CT_REMOTE;
		}
	else if (gameData.multiplayer.nLocalPlayer == nOtherObjNum) {
		OBJECTS [nServerObjNum].info.controlType = CT_REMOTE;
		OBJECTS [nOtherObjNum].info.controlType = CT_FLYING;
		}
	}
}

//------------------------------------------------------------------------------

void StateFixObjects (void)
{
	tObject	*objP = OBJECTS;
	int		i, j, nSegment;

ConvertObjects ();
gameData.objs.nNextSignature = 0;
for (i = 0; i <= gameData.objs.nLastObject [0]; i++, objP++) {
	objP->rType.polyObjInfo.nAltTextures = -1;
	nSegment = objP->info.nSegment;
	// hack for a bug I haven't yet been able to fix 
	if ((objP->info.nType != OBJ_REACTOR) && (objP->info.xShields < 0)) {
		j = FindBoss (i);
		if ((j < 0) || (gameData.boss [j].nDying != i))
			objP->info.nType = OBJ_NONE;
		}
	objP->info.nNextInSeg = objP->info.nPrevInSeg = objP->info.nSegment = -1;
	if (objP->info.nType != OBJ_NONE) {
		LinkObjToSeg (i, nSegment);
		if (objP->info.nSignature > gameData.objs.nNextSignature)
			gameData.objs.nNextSignature = objP->info.nSignature;
		}
	//look for, and fix, boss with bogus shields
	if ((objP->info.nType == OBJ_ROBOT) && ROBOTINFO (objP->info.nId).bossFlag) {
		fix xShieldSave = objP->info.xShields;
		CopyDefaultsToRobot (objP);		//calculate starting shields
		//if in valid range, use loaded shield value
		if (xShieldSave > 0 && (xShieldSave <= objP->info.xShields))
			objP->info.xShields = xShieldSave;
		else
			objP->info.xShields /= 2;  //give tPlayer a break
		}
	}
}

//------------------------------------------------------------------------------

void StateAwardReturningPlayer (tPlayer *retPlayerP, fix xOldGameTime)
{
tPlayer *playerP = gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer;
playerP->level = retPlayerP->level;
playerP->lastScore = retPlayerP->lastScore;
playerP->timeLevel = retPlayerP->timeLevel;
playerP->flags |= (retPlayerP->flags & PLAYER_FLAGS_ALL_KEYS);
playerP->numRobotsLevel = retPlayerP->numRobotsLevel;
playerP->numRobotsTotal = retPlayerP->numRobotsTotal;
playerP->hostages.nRescued = retPlayerP->hostages.nRescued;
playerP->hostages.nTotal = retPlayerP->hostages.nTotal;
playerP->hostages.nOnBoard = retPlayerP->hostages.nOnBoard;
playerP->hostages.nLevel = retPlayerP->hostages.nLevel;
playerP->homingObjectDist = retPlayerP->homingObjectDist;
playerP->hoursLevel = retPlayerP->hoursLevel;
playerP->hoursTotal = retPlayerP->hoursTotal;
//playerP->nCloaks = retPlayerP->nCloaks;
//playerP->nInvuls = retPlayerP->nInvuls;
DoCloakInvulSecretStuff (xOldGameTime);
}

//------------------------------------------------------------------------------

void StateRestoreNetGame (CFILE *cfP)
{
	int	i, j;

netGame.nType = CFReadByte (cfP);
netGame.nSecurity = CFReadInt (cfP);
CFRead (netGame.szGameName, 1, NETGAME_NAME_LEN + 1, cfP);
CFRead (netGame.szMissionTitle, 1, MISSION_NAME_LEN + 1, cfP);
CFRead (netGame.szMissionName, 1, 9, cfP);
netGame.nLevel = CFReadInt (cfP);
netGame.gameMode = (ubyte) CFReadByte (cfP);
netGame.bRefusePlayers = (ubyte) CFReadByte (cfP);
netGame.difficulty = (ubyte) CFReadByte (cfP);
netGame.gameStatus = (ubyte) CFReadByte (cfP);
netGame.nNumPlayers = (ubyte) CFReadByte (cfP);
netGame.nMaxPlayers = (ubyte) CFReadByte (cfP);
netGame.nConnected = (ubyte) CFReadByte (cfP);
netGame.gameFlags = (ubyte) CFReadByte (cfP);
netGame.protocolVersion = (ubyte) CFReadByte (cfP);
netGame.versionMajor = (ubyte) CFReadByte (cfP);
netGame.versionMinor = (ubyte) CFReadByte (cfP);
netGame.teamVector = (ubyte) CFReadByte (cfP);
netGame.DoMegas = (ubyte) CFReadByte (cfP);
netGame.DoSmarts = (ubyte) CFReadByte (cfP);
netGame.DoFusions = (ubyte) CFReadByte (cfP);
netGame.DoHelix = (ubyte) CFReadByte (cfP);
netGame.DoPhoenix = (ubyte) CFReadByte (cfP);
netGame.DoAfterburner = (ubyte) CFReadByte (cfP);
netGame.DoInvulnerability = (ubyte) CFReadByte (cfP);
netGame.DoCloak = (ubyte) CFReadByte (cfP);
netGame.DoGauss = (ubyte) CFReadByte (cfP);
netGame.DoVulcan = (ubyte) CFReadByte (cfP);
netGame.DoPlasma = (ubyte) CFReadByte (cfP);
netGame.DoOmega = (ubyte) CFReadByte (cfP);
netGame.DoSuperLaser = (ubyte) CFReadByte (cfP);
netGame.DoProximity = (ubyte) CFReadByte (cfP);
netGame.DoSpread = (ubyte) CFReadByte (cfP);
netGame.DoSmartMine = (ubyte) CFReadByte (cfP);
netGame.DoFlash = (ubyte) CFReadByte (cfP);
netGame.DoGuided = (ubyte) CFReadByte (cfP);
netGame.DoEarthShaker = (ubyte) CFReadByte (cfP);
netGame.DoMercury = (ubyte) CFReadByte (cfP);
netGame.bAllowMarkerView = (ubyte) CFReadByte (cfP);
netGame.bIndestructibleLights = (ubyte) CFReadByte (cfP);
netGame.DoAmmoRack = (ubyte) CFReadByte (cfP);
netGame.DoConverter = (ubyte) CFReadByte (cfP);
netGame.DoHeadlight = (ubyte) CFReadByte (cfP);
netGame.DoHoming = (ubyte) CFReadByte (cfP);
netGame.DoLaserUpgrade = (ubyte) CFReadByte (cfP);
netGame.DoQuadLasers = (ubyte) CFReadByte (cfP);
netGame.bShowAllNames = (ubyte) CFReadByte (cfP);
netGame.BrightPlayers = (ubyte) CFReadByte (cfP);
netGame.invul = (ubyte) CFReadByte (cfP);
netGame.FriendlyFireOff = (ubyte) CFReadByte (cfP);
for (i = 0; i < 2; i++)
	CFRead (netGame.szTeamName [i], 1, CALLSIGN_LEN + 1, cfP);		// 18 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	netGame.locations [i] = CFReadInt (cfP);
for (i = 0; i < MAX_PLAYERS; i++)
	for (j = 0; j < MAX_PLAYERS; j++)
		netGame.kills [i][j] = CFReadShort (cfP);			// 128 bytes
netGame.nSegmentCheckSum = CFReadShort (cfP);				// 2 bytes
for (i = 0; i < 2; i++)
	netGame.teamKills [i] = CFReadShort (cfP);				// 4 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	netGame.killed [i] = CFReadShort (cfP);					// 16 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	netGame.playerKills [i] = CFReadShort (cfP);			// 16 bytes
netGame.KillGoal = CFReadInt (cfP);							// 4 bytes
netGame.xPlayTimeAllowed = CFReadFix (cfP);					// 4 bytes
netGame.xLevelTime = CFReadFix (cfP);							// 4 bytes
netGame.controlInvulTime = CFReadInt (cfP);				// 4 bytes
netGame.monitorVector = CFReadInt (cfP);					// 4 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	netGame.playerScore [i] = CFReadInt (cfP);				// 32 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	netGame.playerFlags [i] = (ubyte) CFReadByte (cfP);	// 8 bytes
netGame.nPacketsPerSec = CFReadShort (cfP);					// 2 bytes
netGame.bShortPackets = (ubyte) CFReadByte (cfP);			// 1 bytes
// 279 bytes
// 355 bytes total
CFRead (netGame.AuxData, NETGAME_AUX_SIZE, 1, cfP);  // Storage for protocol-specific data (e.g., multicast session and port)
}

//------------------------------------------------------------------------------

void StateRestoreNetPlayers (CFILE *cfP)
{
	int	i;

netPlayers.nType = (ubyte) CFReadByte (cfP);
netPlayers.nSecurity = CFReadInt (cfP);
for (i = 0; i < MAX_PLAYERS + 4; i++) {
	CFRead (netPlayers.players [i].callsign, 1, CALLSIGN_LEN + 1, cfP);
	CFRead (netPlayers.players [i].network.ipx.server, 1, 4, cfP);
	CFRead (netPlayers.players [i].network.ipx.node, 1, 6, cfP);
	netPlayers.players [i].versionMajor = (ubyte) CFReadByte (cfP);
	netPlayers.players [i].versionMinor = (ubyte) CFReadByte (cfP);
	netPlayers.players [i].computerType = (enum compType) CFReadByte (cfP);
	netPlayers.players [i].connected = CFReadByte (cfP);
	netPlayers.players [i].socket = (ushort) CFReadShort (cfP);
	netPlayers.players [i].rank = (ubyte) CFReadByte (cfP);
	}
}

//------------------------------------------------------------------------------

void StateRestorePlayer (tPlayer *playerP, CFILE *cfP)
{
	int	i;

CFRead (playerP->callsign, 1, CALLSIGN_LEN + 1, cfP); // The callsign of this tPlayer, for net purposes.
CFRead (playerP->netAddress, 1, 6, cfP);					// The network address of the player.
playerP->connected = CFReadByte (cfP);            // Is the tPlayer connected or not?
playerP->nObject = CFReadInt (cfP);                // What tObject number this tPlayer is. (made an int by mk because it's very often referenced)
playerP->nPacketsGot = CFReadInt (cfP);         // How many packets we got from them
playerP->nPacketsSent = CFReadInt (cfP);        // How many packets we sent to them
playerP->flags = (uint) CFReadInt (cfP);           // Powerup flags, see below...
playerP->energy = CFReadFix (cfP);                // Amount of energy remaining.
playerP->shields = CFReadFix (cfP);               // shields remaining (protection)
playerP->lives = CFReadByte (cfP);                // Lives remaining, 0 = game over.
playerP->level = CFReadByte (cfP);                // Current level tPlayer is playing. (must be signed for secret levels)
playerP->laserLevel = (ubyte) CFReadByte (cfP);  // Current level of the laser.
playerP->startingLevel = CFReadByte (cfP);       // What level the tPlayer started on.
playerP->nKillerObj = CFReadShort (cfP);       // Who killed me.... (-1 if no one)
playerP->primaryWeaponFlags = (ushort) CFReadShort (cfP);   // bit set indicates the tPlayer has this weapon.
playerP->secondaryWeaponFlags = (ushort) CFReadShort (cfP); // bit set indicates the tPlayer has this weapon.
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	playerP->primaryAmmo [i] = (ushort) CFReadShort (cfP); // How much ammo of each nType.
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
	playerP->secondaryAmmo [i] = (ushort) CFReadShort (cfP); // How much ammo of each nType.
#if 1 //for inventory system
playerP->nInvuls = (ubyte) CFReadByte (cfP);
playerP->nCloaks = (ubyte) CFReadByte (cfP);
#endif
playerP->lastScore = CFReadInt (cfP);           // Score at beginning of current level.
playerP->score = CFReadInt (cfP);                // Current score.
playerP->timeLevel = CFReadFix (cfP);            // Level time played
playerP->timeTotal = CFReadFix (cfP);				// Game time played (high word = seconds)
playerP->cloakTime = CFReadFix (cfP);					// Time cloaked
if (playerP->cloakTime != 0x7fffffff)
	playerP->cloakTime += gameData.time.xGame;
playerP->invulnerableTime = CFReadFix (cfP);      // Time invulnerable
if (playerP->invulnerableTime != 0x7fffffff)
	playerP->invulnerableTime += gameData.time.xGame;
playerP->nKillGoalCount = CFReadShort (cfP);          // Num of players killed this level
playerP->netKilledTotal = CFReadShort (cfP);       // Number of times killed total
playerP->netKillsTotal = CFReadShort (cfP);        // Number of net kills total
playerP->numKillsLevel = CFReadShort (cfP);        // Number of kills this level
playerP->numKillsTotal = CFReadShort (cfP);        // Number of kills total
playerP->numRobotsLevel = CFReadShort (cfP);       // Number of initial robots this level
playerP->numRobotsTotal = CFReadShort (cfP);       // Number of robots total
playerP->hostages.nRescued = (ushort) CFReadShort (cfP); // Total number of hostages rescued.
playerP->hostages.nTotal = (ushort) CFReadShort (cfP);         // Total number of hostages.
playerP->hostages.nOnBoard = (ubyte) CFReadByte (cfP);      // Number of hostages on ship.
playerP->hostages.nLevel = (ubyte) CFReadByte (cfP);         // Number of hostages on this level.
playerP->homingObjectDist = CFReadFix (cfP);     // Distance of nearest homing tObject.
playerP->hoursLevel = CFReadByte (cfP);            // Hours played (since timeTotal can only go up to 9 hours)
playerP->hoursTotal = CFReadByte (cfP);            // Hours played (since timeTotal can only go up to 9 hours)
}

//------------------------------------------------------------------------------

void StateRestoreObject (tObject *objP, CFILE *cfP, int sgVersion)
{
objP->info.nSignature = CFReadInt (cfP);      
objP->info.nType = (ubyte) CFReadByte (cfP); 
#if DBG
if (objP->info.nType == OBJ_REACTOR)
	objP->info.nType = objP->info.nType;
else 
#endif
if ((sgVersion < 32) && IS_BOSS (objP))
	gameData.boss [(int) extraGameInfo [0].nBossCount++].nObject = OBJ_IDX (objP);
objP->info.nId = (ubyte) CFReadByte (cfP);
objP->info.nNextInSeg = CFReadShort (cfP);
objP->info.nPrevInSeg = CFReadShort (cfP);
objP->info.controlType = (ubyte) CFReadByte (cfP);
objP->info.movementType = (ubyte) CFReadByte (cfP);
objP->info.renderType = (ubyte) CFReadByte (cfP);
objP->info.nFlags = (ubyte) CFReadByte (cfP);
objP->info.nSegment = CFReadShort (cfP);
objP->info.nAttachedObj = CFReadShort (cfP);
CFReadVector (objP->info.position.vPos, cfP);     
CFReadMatrix (objP->info.position.mOrient, cfP);  
objP->info.xSize = CFReadFix (cfP); 
objP->info.xShields = CFReadFix (cfP);
CFReadVector (objP->info.vLastPos, cfP);  
objP->info.contains.nType = CFReadByte (cfP); 
objP->info.contains.nId = CFReadByte (cfP);   
objP->info.contains.nCount = CFReadByte (cfP);
objP->info.nCreator = CFReadByte (cfP);
objP->info.xLifeLeft = CFReadFix (cfP);   
if (objP->info.movementType == MT_PHYSICS) {
	CFReadVector (objP->mType.physInfo.velocity, cfP);   
	CFReadVector (objP->mType.physInfo.thrust, cfP);     
	objP->mType.physInfo.mass = CFReadFix (cfP);       
	objP->mType.physInfo.drag = CFReadFix (cfP);       
	objP->mType.physInfo.brakes = CFReadFix (cfP);     
	CFReadVector (objP->mType.physInfo.rotVel, cfP);     
	CFReadVector (objP->mType.physInfo.rotThrust, cfP);  
	objP->mType.physInfo.turnRoll = CFReadFixAng (cfP);   
	objP->mType.physInfo.flags = (ushort) CFReadShort (cfP);      
	}
else if (objP->info.movementType == MT_SPINNING) {
	CFReadVector (objP->mType.spinRate, cfP);  
	}
switch (objP->info.controlType) {
	case CT_WEAPON:
		objP->cType.laserInfo.parent.nType = CFReadShort (cfP);
		objP->cType.laserInfo.parent.nObject = CFReadShort (cfP);
		objP->cType.laserInfo.parent.nSignature = CFReadInt (cfP);
		objP->cType.laserInfo.xCreationTime = CFReadFix (cfP);
		objP->cType.laserInfo.nLastHitObj = CFReadShort (cfP);
		if (objP->cType.laserInfo.nLastHitObj < 0)
			objP->cType.laserInfo.nLastHitObj = 0;
		else {
			gameData.objs.nHitObjects [OBJ_IDX (objP) * MAX_HIT_OBJECTS] = objP->cType.laserInfo.nLastHitObj;
			objP->cType.laserInfo.nLastHitObj = 1;
			}
		objP->cType.laserInfo.nHomingTarget = CFReadShort (cfP);
		objP->cType.laserInfo.xScale = CFReadFix (cfP);
		break;

	case CT_EXPLOSION:
		objP->cType.explInfo.nSpawnTime = CFReadFix (cfP);
		objP->cType.explInfo.nDeleteTime = CFReadFix (cfP);
		objP->cType.explInfo.nDeleteObj = CFReadShort (cfP);
		objP->cType.explInfo.attached.nParent = CFReadShort (cfP);
		objP->cType.explInfo.attached.nPrev = CFReadShort (cfP);
		objP->cType.explInfo.attached.nNext = CFReadShort (cfP);
		break;

	case CT_AI:
		objP->cType.aiInfo.behavior = (ubyte) CFReadByte (cfP);
		CFRead (objP->cType.aiInfo.flags, 1, MAX_AI_FLAGS, cfP);
		objP->cType.aiInfo.nHideSegment = CFReadShort (cfP);
		objP->cType.aiInfo.nHideIndex = CFReadShort (cfP);
		objP->cType.aiInfo.nPathLength = CFReadShort (cfP);
		objP->cType.aiInfo.nCurPathIndex = CFReadByte (cfP);
		objP->cType.aiInfo.bDyingSoundPlaying = CFReadByte (cfP);
		objP->cType.aiInfo.nDangerLaser = CFReadShort (cfP);
		objP->cType.aiInfo.nDangerLaserSig = CFReadInt (cfP);
		objP->cType.aiInfo.xDyingStartTime = CFReadFix (cfP);
		break;

	case CT_LIGHT:
		objP->cType.lightInfo.intensity = CFReadFix (cfP);
		break;

	case CT_POWERUP:
		objP->cType.powerupInfo.nCount = CFReadInt (cfP);
		objP->cType.powerupInfo.xCreationTime = CFReadFix (cfP);
		objP->cType.powerupInfo.nFlags = CFReadInt (cfP);
		break;
	}
switch (objP->info.renderType) {
	case RT_MORPH:
	case RT_POLYOBJ: {
		int i;
		objP->rType.polyObjInfo.nModel = CFReadInt (cfP);
		for (i = 0; i < MAX_SUBMODELS; i++)
			CFReadAngVec (objP->rType.polyObjInfo.animAngles [i], cfP);
		objP->rType.polyObjInfo.nSubObjFlags = CFReadInt (cfP);
		objP->rType.polyObjInfo.nTexOverride = CFReadInt (cfP);
		objP->rType.polyObjInfo.nAltTextures = CFReadInt (cfP);
		break;
		}
	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
	case RT_THRUSTER:
		objP->rType.vClipInfo.nClipIndex = CFReadInt (cfP);
		objP->rType.vClipInfo.xFrameTime = CFReadFix (cfP);
		objP->rType.vClipInfo.nCurFrame = CFReadByte (cfP);
		break;

	case RT_LASER:
		break;
	}
}

//------------------------------------------------------------------------------

void StateRestoreWall (tWall *wallP, CFILE *cfP)
{
wallP->nSegment = CFReadInt (cfP);
wallP->nSide = CFReadInt (cfP);
wallP->hps = CFReadFix (cfP);    
wallP->nLinkedWall = CFReadInt (cfP);
wallP->nType = (ubyte) CFReadByte (cfP);       
wallP->flags = (ubyte) CFReadByte (cfP);      
wallP->state = (ubyte) CFReadByte (cfP);      
wallP->nTrigger = (ubyte) CFReadByte (cfP);    
wallP->nClip = CFReadByte (cfP);   
wallP->keys = (ubyte) CFReadByte (cfP);       
wallP->controllingTrigger = CFReadByte (cfP);
wallP->cloakValue = CFReadByte (cfP); 
}

//------------------------------------------------------------------------------

void StateRestoreExplWall (tExplWall *wallP, CFILE *cfP)
{
wallP->nSegment = CFReadInt (cfP);
wallP->nSide = CFReadInt (cfP);
wallP->time = CFReadFix (cfP);    
}

//------------------------------------------------------------------------------

void StateRestoreCloakingWall (tCloakingWall *wallP, CFILE *cfP)
{
	int	i;

wallP->nFrontWall = CFReadShort (cfP);
wallP->nBackWall = CFReadShort (cfP); 
for (i = 0; i < 4; i++) {
	wallP->front_ls [i] = CFReadFix (cfP); 
	wallP->back_ls [i] = CFReadFix (cfP);
	}
wallP->time = CFReadFix (cfP);    
}

//------------------------------------------------------------------------------

void StateRestoreActiveDoor (tActiveDoor *doorP, CFILE *cfP)
{
	int	i;

doorP->nPartCount = CFReadInt (cfP);
for (i = 0; i < 2; i++) {
	doorP->nFrontWall [i] = CFReadShort (cfP);
	doorP->nBackWall [i] = CFReadShort (cfP);
	}
doorP->time = CFReadFix (cfP);    
}

//------------------------------------------------------------------------------

void StateRestoreTrigger (tTrigger *triggerP, CFILE *cfP)
{
	int	i;

i = CFTell (cfP);
triggerP->nType = (ubyte) CFReadByte (cfP); 
triggerP->flags = (ubyte) CFReadByte (cfP); 
triggerP->nLinks = CFReadByte (cfP);
triggerP->value = CFReadFix (cfP);
triggerP->time = CFReadFix (cfP);
for (i = 0; i < MAX_TRIGGER_TARGETS; i++) {
	triggerP->nSegment [i] = CFReadShort (cfP);
	triggerP->nSide [i] = CFReadShort (cfP);
	}
}

//------------------------------------------------------------------------------

void StateRestoreObjTriggerRef (tObjTriggerRef *refP, CFILE *cfP)
{
refP->prev = CFReadShort (cfP);
refP->next = CFReadShort (cfP);
refP->nObject = CFReadShort (cfP);
}

//------------------------------------------------------------------------------

void StateRestoreMatCen (tMatCenInfo *matcenP, CFILE *cfP)
{
	int	i;

for (i = 0; i < 2; i++)
	matcenP->objFlags [i] = CFReadInt (cfP);
matcenP->xHitPoints = CFReadFix (cfP);
matcenP->xInterval = CFReadFix (cfP);
matcenP->nSegment = CFReadShort (cfP);
matcenP->nFuelCen = CFReadShort (cfP);
}

//------------------------------------------------------------------------------

void StateRestoreFuelCen (tFuelCenInfo *fuelcenP, CFILE *cfP)
{
fuelcenP->nType = CFReadInt (cfP);
fuelcenP->nSegment = CFReadInt (cfP);
fuelcenP->bFlag = CFReadByte (cfP);
fuelcenP->bEnabled = CFReadByte (cfP);
fuelcenP->nLives = CFReadByte (cfP);
fuelcenP->xCapacity = CFReadFix (cfP);
fuelcenP->xMaxCapacity = CFReadFix (cfP);
fuelcenP->xTimer = CFReadFix (cfP);
fuelcenP->xDisableTime = CFReadFix (cfP);
CFReadVector (fuelcenP->vCenter, cfP);
}

//------------------------------------------------------------------------------

void StateRestoreReactorTrigger (tReactorTriggers *triggerP, CFILE *cfP)
{
	int	i;

triggerP->nLinks = CFReadShort (cfP);
for (i = 0; i < MAX_CONTROLCEN_LINKS; i++) {
	triggerP->nSegment [i] = CFReadShort (cfP);
	triggerP->nSide [i] = CFReadShort (cfP);
	}
}

//------------------------------------------------------------------------------

int StateRestoreSpawnPoint (int i, CFILE *cfP)
{
DBG (i = CFTell (cfP));
CFReadVector (gameData.multiplayer.playerInit [i].position.vPos, cfP);     
CFReadMatrix (gameData.multiplayer.playerInit [i].position.mOrient, cfP);  
gameData.multiplayer.playerInit [i].nSegment = CFReadShort (cfP);
gameData.multiplayer.playerInit [i].nSegType = CFReadShort (cfP);
return (gameData.multiplayer.playerInit [i].nSegment >= 0) &&
		 (gameData.multiplayer.playerInit [i].nSegment < gameData.segs.nSegments) &&
		 (gameData.multiplayer.playerInit [i].nSegment ==
		  FindSegByPos (gameData.multiplayer.playerInit [i].position.vPos, 
							 gameData.multiplayer.playerInit [i].nSegment, 1, 0));

}

//------------------------------------------------------------------------------

int StateRestoreUniGameData (CFILE *cfP, int sgVersion, int bMulti, int bSecretRestore, fix xOldGameTime, int *nLevel)
{
	tPlayer	restoredPlayers [MAX_PLAYERS];
	int		nPlayers, nServerPlayer = -1;
	int		nOtherObjNum = -1, nServerObjNum = -1, nLocalObjNum = -1, nSavedLocalPlayer = -1;
	int		bBetweenLevels;
	int		nCurrentLevel, nNextLevel;
	tWall		*wallP;
	char		szOrgCallSign [CALLSIGN_LEN+16];
	int		h, i, j;
	short		nTexture;

if (sgVersion >= 39) {
	h = CFReadInt (cfP);
	if (h != gameData.segs.nMaxSegments) {
		Warning (TXT_MAX_SEGS_WARNING, h);
		return 0;
		}
	}
bBetweenLevels = CFReadInt (cfP);
Assert (bBetweenLevels == 0);	//between levels save ripped out
// Read the mission info...
if (!StateLoadMission (cfP))
	return 0;
//Read level info
nCurrentLevel = CFReadInt (cfP);
nNextLevel = CFReadInt (cfP);
//Restore gameData.time.xGame
gameData.time.xGame = CFReadFix (cfP);
// Start new game....
StateRestoreMultiGame (szOrgCallSign, bMulti, bSecretRestore);
if (IsMultiGame) {
		char szServerCallSign [CALLSIGN_LEN + 1];

	strcpy (szServerCallSign, netPlayers.players [0].callsign);
	gameData.app.nStateGameId = CFReadInt (cfP);
	StateRestoreNetGame (cfP);
	DBG (fPos = CFTell (cfP));
	StateRestoreNetPlayers (cfP);
	DBG (fPos = CFTell (cfP));
	nPlayers = CFReadInt (cfP);
	nSavedLocalPlayer = gameData.multiplayer.nLocalPlayer;
	gameData.multiplayer.nLocalPlayer = CFReadInt (cfP);
	for (i = 0; i < nPlayers; i++) {
		StateRestorePlayer (restoredPlayers + i, cfP);
		restoredPlayers [i].connected = 0;
		}
	DBG (fPos = CFTell (cfP));
	// make sure the current game host is in tPlayer slot #0
	nServerPlayer = StateSetServerPlayer (restoredPlayers, nPlayers, szServerCallSign, &nOtherObjNum, &nServerObjNum);
	StateGetConnectedPlayers (restoredPlayers, nPlayers);
	}
//Read tPlayer info
if (!StartNewLevelSub (nCurrentLevel, 1, bSecretRestore, 1)) {
	CFClose (cfP);
	return 0;
	}
nLocalObjNum = LOCALPLAYER.nObject;
if (bSecretRestore != 1)	//either no secret restore, or tPlayer died in scret level
	StateRestorePlayer (gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer, cfP);
else {
	tPlayer	retPlayer;
	StateRestorePlayer (&retPlayer, cfP);
	StateAwardReturningPlayer (&retPlayer, xOldGameTime);
	}
LOCALPLAYER.nObject = nLocalObjNum;
strcpy (LOCALPLAYER.callsign, szOrgCallSign);
// Set the right level
if (bBetweenLevels)
	LOCALPLAYER.level = nNextLevel;
// Restore the weapon states
gameData.weapons.nPrimary = CFReadByte (cfP);
gameData.weapons.nSecondary = CFReadByte (cfP);
SelectWeapon (gameData.weapons.nPrimary, 0, 0, 0);
SelectWeapon (gameData.weapons.nSecondary, 1, 0, 0);
// Restore the difficulty level
gameStates.app.nDifficultyLevel = CFReadInt (cfP);
// Restore the cheats enabled flag
gameStates.app.cheats.bEnabled = CFReadInt (cfP);
for (i = 0; i < 2; i++) {
	if (sgVersion < 33) {
		gameStates.gameplay.slowmo [i].fSpeed = 1;
		gameStates.gameplay.slowmo [i].nState = 0;
		}
	else {
		gameStates.gameplay.slowmo [i].fSpeed = X2F (CFReadInt (cfP));
		gameStates.gameplay.slowmo [i].nState = CFReadInt (cfP);
		}
	}
if (sgVersion > 33) {
	for (i = 0; i < MAX_PLAYERS; i++)
	   if (i != gameData.multiplayer.nLocalPlayer)
		   gameData.multiplayer.weaponStates [i].bTripleFusion = CFReadInt (cfP);
   	else {
   	   gameData.weapons.bTripleFusion = CFReadInt (cfP);
		   gameData.multiplayer.weaponStates [i].bTripleFusion = !gameData.weapons.bTripleFusion;  //force MultiSendWeapons
		   }
	}
if (!bBetweenLevels)	{
	gameStates.render.bDoAppearanceEffect = 0;			// Don't do this for middle o' game stuff.
	//Clear out all the objects from the lvl file
	memset (gameData.segs.objects, 0xff, gameData.segs.nSegments * sizeof (short));
	ResetObjects (1);

	//Read objects, and pop 'em into their respective segments.
	DBG (fPos = CFTell (cfP));
	h = CFReadInt (cfP);
	gameData.objs.nLastObject [0] = h - 1;
	extraGameInfo [0].nBossCount = 0;
	for (i = 0; i < h; i++)
		StateRestoreObject (OBJECTS + i, cfP, sgVersion);
	DBG (fPos = CFTell (cfP));
	StateFixNetworkObjects (nServerPlayer, nOtherObjNum, nServerObjNum);
	gameData.objs.nNextSignature = 0;
	InitCamBots (1);
	gameData.objs.nNextSignature++;
	//	1 = Didn't die on secret level.
	//	2 = Died on secret level.
	if (bSecretRestore && (gameData.missions.nCurrentLevel >= 0)) {
		SetPosFromReturnSegment (0);
		if (bSecretRestore == 2)
			InitPlayerStatsNewShip ();
		}
	//Restore tWall info
	if (CFReadBoundedInt (MAX_WALLS, &gameData.walls.nWalls, cfP))
		return 0;
	for (i = 0, wallP = gameData.walls.walls; i < gameData.walls.nWalls; i++, wallP++) {
		StateRestoreWall (wallP, cfP);
		if (wallP->nType == WALL_OPEN)
			DigiKillSoundLinkedToSegment ((short) wallP->nSegment, (short) wallP->nSide, -1);	//-1 means kill any sound
		}
	DBG (fPos = CFTell (cfP));
	//Restore exploding wall info
	if (CFReadBoundedInt (MAX_EXPLODING_WALLS, &h, cfP))
		return 0;
	for (i = 0; i < h; i++)
		StateRestoreExplWall (gameData.walls.explWalls + i, cfP);
	DBG (fPos = CFTell (cfP));
	//Restore door info
	if (CFReadBoundedInt (MAX_DOORS, &gameData.walls.nOpenDoors, cfP))
		return 0;
	for (i = 0; i < gameData.walls.nOpenDoors; i++)
		StateRestoreActiveDoor (gameData.walls.activeDoors + i, cfP);
	DBG (fPos = CFTell (cfP));
	if (CFReadBoundedInt (MAX_WALLS, &gameData.walls.nCloaking, cfP))
		return 0;
	for (i = 0; i < gameData.walls.nCloaking; i++)
		StateRestoreCloakingWall (gameData.walls.cloaking + i, cfP);
	DBG (fPos = CFTell (cfP));
	//Restore tTrigger info
	if (CFReadBoundedInt (MAX_TRIGGERS, &gameData.trigs.nTriggers, cfP))
		return 0;
	for (i = 0; i < gameData.trigs.nTriggers; i++)
		StateRestoreTrigger (gameData.trigs.triggers + i, cfP);
	DBG (fPos = CFTell (cfP));
	//Restore tObject tTrigger info
	if (CFReadBoundedInt (MAX_TRIGGERS, &gameData.trigs.nObjTriggers, cfP))
		return 0;
	if (gameData.trigs.nObjTriggers > 0) {
		for (i = 0; i < gameData.trigs.nObjTriggers; i++)
			StateRestoreTrigger (gameData.trigs.objTriggers + i, cfP);
		for (i = 0; i < gameData.trigs.nObjTriggers; i++)
			StateRestoreObjTriggerRef (gameData.trigs.objTriggerRefs + i, cfP);
		if (sgVersion < 36) {
			j = (sgVersion < 35) ? 700 : MAX_OBJECTS_D2X;
			for (i = 0; i < j; i++)
				gameData.trigs.firstObjTrigger [i] = CFReadShort (cfP);
			}
		else {
			memset (gameData.trigs.firstObjTrigger, 0xff, sizeof (short) * MAX_OBJECTS_D2X);
			for (i = CFReadShort (cfP); i; i--) {
				j = CFReadShort (cfP);
				gameData.trigs.firstObjTrigger [j] = CFReadShort (cfP);
				}
			}
		}
	else if (sgVersion < 36)
		CFSeek (cfP, ((sgVersion < 35) ? 700 : MAX_OBJECTS_D2X) * sizeof (short), SEEK_CUR);
	else
		CFReadShort (cfP);
	DBG (fPos = CFTell (cfP));
	//Restore tmap info
	for (i = 0; i <= gameData.segs.nLastSegment; i++)	{
		for (j = 0; j < 6; j++)	{
			gameData.segs.segments [i].sides [j].nWall = CFReadShort (cfP);
			gameData.segs.segments [i].sides [j].nBaseTex = CFReadShort (cfP);
			nTexture = CFReadShort (cfP);
			gameData.segs.segments [i].sides [j].nOvlTex = nTexture & 0x3fff;
			gameData.segs.segments [i].sides [j].nOvlOrient = (nTexture >> 14) & 3;
			}
		}
	DBG (fPos = CFTell (cfP));
	//Restore the fuelcen info
	for (i = 0, wallP = gameData.walls.walls; i < gameData.walls.nWalls; i++, wallP++) {
		if ((wallP->nType == WALL_DOOR) && (wallP->flags & WALL_DOOR_OPENED))
			AnimateOpeningDoor (SEGMENTS + wallP->nSegment, wallP->nSide, -1);
		else if ((wallP->nType == WALL_BLASTABLE) && (wallP->flags & WALL_BLASTED))
			BlastBlastableWall (SEGMENTS + wallP->nSegment, wallP->nSide);
		}
	gameData.reactor.bDestroyed = CFReadInt (cfP);
	gameData.reactor.countdown.nTimer = CFReadFix (cfP);
	DBG (fPos = CFTell (cfP));
	if (CFReadBoundedInt (MAX_ROBOT_CENTERS, &gameData.matCens.nBotCenters, cfP))
		return 0;
	for (i = 0; i < gameData.matCens.nBotCenters; i++)
		StateRestoreMatCen (gameData.matCens.botGens + i, cfP);
	if (sgVersion >= 30) {
		if (CFReadBoundedInt (MAX_EQUIP_CENTERS, &gameData.matCens.nEquipCenters, cfP))
			return 0;
		for (i = 0; i < gameData.matCens.nEquipCenters; i++)
			StateRestoreMatCen (gameData.matCens.equipGens + i, cfP);
		}
	else {
		gameData.matCens.nBotCenters = 0;
		memset (gameData.matCens.botGens, 0, sizeof (gameData.matCens.botGens));
		}
	StateRestoreReactorTrigger (&gameData.reactor.triggers, cfP);
	if (CFReadBoundedInt (MAX_FUEL_CENTERS, &gameData.matCens.nFuelCenters, cfP))
		return 0;
	for (i = 0; i < gameData.matCens.nFuelCenters; i++)
		StateRestoreFuelCen (gameData.matCens.fuelCenters + i, cfP);
	DBG (fPos = CFTell (cfP));
	// Restore the control cen info
	if (sgVersion < 31) {
		gameData.reactor.states [0].bHit = CFReadInt (cfP);
		gameData.reactor.states [0].bSeenPlayer = CFReadInt (cfP);
		gameData.reactor.states [0].nNextFireTime = CFReadInt (cfP);
		gameData.reactor.bPresent = CFReadInt (cfP);
		gameData.reactor.states [0].nDeadObj = CFReadInt (cfP);
		}
	else {
		int	i;

		gameData.reactor.bPresent = CFReadInt (cfP);
		for (i = 0; i < MAX_BOSS_COUNT; i++) {
			gameData.reactor.states [i].nObject = CFReadInt (cfP);
			gameData.reactor.states [i].bHit = CFReadInt (cfP);
			gameData.reactor.states [i].bSeenPlayer = CFReadInt (cfP);
			gameData.reactor.states [i].nNextFireTime = CFReadInt (cfP);
			gameData.reactor.states [i].nDeadObj = CFReadInt (cfP);
			}
		}
	DBG (fPos = CFTell (cfP));
	// Restore the AI state
	AIRestoreUniState (cfP, sgVersion);
	// Restore the automap visited info
	DBG (fPos = CFTell (cfP));
	StateFixObjects ();
	SpecialResetObjects ();
	if (sgVersion > 37) {
		for (i = 0; i < MAX_SEGMENTS; i++)
			gameData.render.mine.bAutomapVisited [i] = (ushort) CFReadShort (cfP);
		}
	else {
		int	i, j = (sgVersion > 22) ? MAX_SEGMENTS : MAX_SEGMENTS_D2;
		for (i = 0; i < j; i++)
			gameData.render.mine.bAutomapVisited [i] = (ushort) CFReadByte (cfP);
		}
	DBG (fPos = CFTell (cfP));
	//	Restore hacked up weapon system stuff.
	gameData.fusion.xNextSoundTime = gameData.time.xGame;
	gameData.fusion.xAutoFireTime = 0;
	gameData.laser.xNextFireTime = 
	gameData.missiles.xNextFireTime = gameData.time.xGame;
	gameData.laser.xLastFiredTime = 
	gameData.missiles.xLastFiredTime = gameData.time.xGame;
	}
gameData.app.nStateGameId = 0;
gameData.app.nStateGameId = (uint) CFReadInt (cfP);
gameStates.app.cheats.bLaserRapidFire = CFReadInt (cfP);
gameStates.app.bLunacy = CFReadInt (cfP);		//	Yes, reading this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
gameStates.app.bLunacy = CFReadInt (cfP);
if (gameStates.app.bLunacy)
	DoLunacyOn ();

DBG (fPos = CFTell (cfP));
for (i = 0; i < NUM_MARKERS; i++)
	gameData.marker.objects [i] = CFReadShort (cfP);
CFRead (gameData.marker.nOwner, sizeof (gameData.marker.nOwner), 1, cfP);
CFRead (gameData.marker.szMessage, sizeof (gameData.marker.szMessage), 1, cfP);

if (bSecretRestore != 1)
	gameData.physics.xAfterburnerCharge = CFReadFix (cfP);
else {
	CFReadFix (cfP);
	}
//read last was super information
CFRead (&bLastPrimaryWasSuper, sizeof (bLastPrimaryWasSuper), 1, cfP);
CFRead (&bLastSecondaryWasSuper, sizeof (bLastSecondaryWasSuper), 1, cfP);
gameData.render.xFlashEffect = CFReadFix (cfP);
gameData.render.xTimeFlashLastPlayed = CFReadFix (cfP);
gameStates.ogl.palAdd.red = CFReadShort (cfP);
gameStates.ogl.palAdd.green = CFReadShort (cfP);
gameStates.ogl.palAdd.blue = CFReadShort (cfP);
CFRead (gameData.render.lights.subtracted, 
		  sizeof (gameData.render.lights.subtracted [0]), 
		  (sgVersion > 22) ? MAX_SEGMENTS : MAX_SEGMENTS_D2, cfP);
ApplyAllChangedLight ();
gameStates.app.bFirstSecretVisit = CFReadInt (cfP);
if (bSecretRestore) 
	gameStates.app.bFirstSecretVisit = 0;

if (bSecretRestore != 1)
	gameData.omega.xCharge [0] = 
	gameData.omega.xCharge [1] = CFReadFix (cfP);
else
	CFReadFix (cfP);
if (sgVersion > 27)
	gameData.missions.nEnteredFromLevel = CFReadShort (cfP);
*nLevel = nCurrentLevel;
if (sgVersion >= 37) {
	tObjPosition playerInitSave [MAX_PLAYERS];

	memcpy (playerInitSave, gameData.multiplayer.playerInit, sizeof (playerInitSave));
	for (h = 1, i = 0; i < MAX_PLAYERS; i++)
		if (!StateRestoreSpawnPoint (i, cfP))
			h = 0;
	if (!h)
		memcpy (gameData.multiplayer.playerInit, playerInitSave, sizeof (playerInitSave));
	}
return 1;
}

//------------------------------------------------------------------------------

int StateRestoreBinGameData (CFILE *cfP, int sgVersion, int bMulti, int bSecretRestore, fix xOldGameTime, int *nLevel)
{
	tPlayer	restoredPlayers [MAX_PLAYERS];
	int		nPlayers, nServerPlayer = -1;
	int		nOtherObjNum = -1, nServerObjNum = -1, nLocalObjNum = -1, nSavedLocalPlayer = -1;
	int		bBetweenLevels;
	int		nCurrentLevel, nNextLevel;
	tWall		*wallP;
	char		szOrgCallSign [CALLSIGN_LEN+16];
	int		i, j;
	short		nTexture;

CFRead (&bBetweenLevels, sizeof (int), 1, cfP);
Assert (bBetweenLevels == 0);	//between levels save ripped out
// Read the mission info...
if (!StateLoadMission (cfP))
	return 0;
//Read level info
CFRead (&nCurrentLevel, sizeof (int), 1, cfP);
CFRead (&nNextLevel, sizeof (int), 1, cfP);
//Restore gameData.time.xGame
CFRead (&gameData.time.xGame, sizeof (fix), 1, cfP);
// Start new game....
StateRestoreMultiGame (szOrgCallSign, bMulti, bSecretRestore);
if (gameData.app.nGameMode & GM_MULTI) {
		char szServerCallSign [CALLSIGN_LEN + 1];

	strcpy (szServerCallSign, netPlayers.players [0].callsign);
	CFRead (&gameData.app.nStateGameId, sizeof (int), 1, cfP);
	CFRead (&netGame, sizeof (tNetgameInfo), 1, cfP);
	CFRead (&netPlayers, sizeof (tAllNetPlayersInfo), 1, cfP);
	CFRead (&nPlayers, sizeof (gameData.multiplayer.nPlayers), 1, cfP);
	CFRead (&gameData.multiplayer.nLocalPlayer, sizeof (gameData.multiplayer.nLocalPlayer), 1, cfP);
	nSavedLocalPlayer = gameData.multiplayer.nLocalPlayer;
	for (i = 0; i < nPlayers; i++)
		CFRead (restoredPlayers + i, sizeof (tPlayer), 1, cfP);
	nServerPlayer = StateSetServerPlayer (restoredPlayers, nPlayers, szServerCallSign, &nOtherObjNum, &nServerObjNum);
	StateGetConnectedPlayers (restoredPlayers, nPlayers);
	}

//Read tPlayer info
if (!StartNewLevelSub (nCurrentLevel, 1, bSecretRestore, 1)) {
	CFClose (cfP);
	return 0;
	}
nLocalObjNum = LOCALPLAYER.nObject;
if (bSecretRestore != 1)	//either no secret restore, or tPlayer died in scret level
	CFRead (gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer, sizeof (tPlayer), 1, cfP);
else {
	tPlayer	retPlayer;
	CFRead (&retPlayer, sizeof (tPlayer), 1, cfP);
	StateAwardReturningPlayer (&retPlayer, xOldGameTime);
	}
LOCALPLAYER.nObject = nLocalObjNum;
strcpy (LOCALPLAYER.callsign, szOrgCallSign);
// Set the right level
if (bBetweenLevels)
	LOCALPLAYER.level = nNextLevel;
// Restore the weapon states
CFRead (&gameData.weapons.nPrimary, sizeof (sbyte), 1, cfP);
CFRead (&gameData.weapons.nSecondary, sizeof (sbyte), 1, cfP);
SelectWeapon (gameData.weapons.nPrimary, 0, 0, 0);
SelectWeapon (gameData.weapons.nSecondary, 1, 0, 0);
// Restore the difficulty level
CFRead (&gameStates.app.nDifficultyLevel, sizeof (int), 1, cfP);
// Restore the cheats enabled flag
CFRead (&gameStates.app.cheats.bEnabled, sizeof (int), 1, cfP);
if (!bBetweenLevels)	{
	gameStates.render.bDoAppearanceEffect = 0;			// Don't do this for middle o' game stuff.
	//Clear out all the OBJECTS from the lvl file
	memset (gameData.segs.objects, 0xff, gameData.segs.nSegments * sizeof (short));
	ResetObjects (1);
	//Read objects, and pop 'em into their respective segments.
	CFRead (&i, sizeof (int), 1, cfP);
	gameData.objs.nLastObject [0] = i - 1;
	CFRead (OBJECTS, sizeof (tObject), i, cfP);
	StateFixNetworkObjects (nServerPlayer, nOtherObjNum, nServerObjNum);
	StateFixObjects ();
	SpecialResetObjects ();
	InitCamBots (1);
	gameData.objs.nNextSignature++;
	//	1 = Didn't die on secret level.
	//	2 = Died on secret level.
	if (bSecretRestore && (gameData.missions.nCurrentLevel >= 0)) {
		SetPosFromReturnSegment (0);
		if (bSecretRestore == 2)
			InitPlayerStatsNewShip ();
		}
	//Restore tWall info
	if (CFReadBoundedInt (MAX_WALLS, &gameData.walls.nWalls, cfP))
		return 0;
	CFRead (gameData.walls.walls, sizeof (tWall), gameData.walls.nWalls, cfP);
	//now that we have the walls, check if any sounds are linked to
	//walls that are now open
	for (i = 0, wallP = gameData.walls.walls; i < gameData.walls.nWalls; i++, wallP++)
		if (wallP->nType == WALL_OPEN)
			DigiKillSoundLinkedToSegment ((short) wallP->nSegment, (short) wallP->nSide, -1);	//-1 means kill any sound
	//Restore exploding wall info
	if (sgVersion >= 10) {
		CFRead (&i, sizeof (int), 1, cfP);
		CFRead (gameData.walls.explWalls, sizeof (*gameData.walls.explWalls), i, cfP);
		}
	//Restore door info
	if (CFReadBoundedInt (MAX_DOORS, &gameData.walls.nOpenDoors, cfP))
		return 0;
	CFRead (gameData.walls.activeDoors, sizeof (tActiveDoor), gameData.walls.nOpenDoors, cfP);
	if (sgVersion >= 14) {		//Restore cloaking tWall info
		if (CFReadBoundedInt (MAX_WALLS, &gameData.walls.nCloaking, cfP))
			return 0;
		CFRead (gameData.walls.cloaking, sizeof (tCloakingWall), gameData.walls.nCloaking, cfP);
		}
	//Restore tTrigger info
	if (CFReadBoundedInt (MAX_TRIGGERS, &gameData.trigs.nTriggers, cfP))
		return 0;
	CFRead (gameData.trigs.triggers, sizeof (tTrigger), gameData.trigs.nTriggers, cfP);
	if (sgVersion >= 26) {
		//Restore tObject tTrigger info

		CFRead (&gameData.trigs.nObjTriggers, sizeof (gameData.trigs.nObjTriggers), 1, cfP);
		if (gameData.trigs.nObjTriggers > 0) {
			CFRead (gameData.trigs.objTriggers, sizeof (tTrigger), gameData.trigs.nObjTriggers, cfP);
			CFRead (gameData.trigs.objTriggerRefs, sizeof (tObjTriggerRef), gameData.trigs.nObjTriggers, cfP);
			CFRead (gameData.trigs.firstObjTrigger, sizeof (short), 700, cfP);
			}
		else
			CFSeek (cfP, (sgVersion < 35) ? 700 : MAX_OBJECTS_D2X * sizeof (short), SEEK_CUR);
		}
	//Restore tmap info
	for (i = 0; i <= gameData.segs.nLastSegment; i++)	{
		for (j = 0; j < 6; j++)	{
			gameData.segs.segments [i].sides [j].nWall = CFReadShort (cfP);
			gameData.segs.segments [i].sides [j].nBaseTex = CFReadShort (cfP);
			nTexture = CFReadShort (cfP);
			gameData.segs.segments [i].sides [j].nOvlTex = nTexture & 0x3fff;
			gameData.segs.segments [i].sides [j].nOvlOrient = (nTexture >> 14) & 3;
			}
		}
//Restore the fuelcen info
	gameData.reactor.bDestroyed = CFReadInt (cfP);
	gameData.reactor.countdown.nTimer = CFReadFix (cfP);
	if (CFReadBoundedInt (MAX_ROBOT_CENTERS, &gameData.matCens.nBotCenters, cfP))
		return 0;
	for (i = 0; i < gameData.matCens.nBotCenters; i++) {
		CFRead (gameData.matCens.botGens [i].objFlags, sizeof (int), 2, cfP);
		CFRead (&gameData.matCens.botGens [i].xHitPoints, sizeof (tMatCenInfo) - ((char *) &gameData.matCens.botGens [i].xHitPoints - (char *) &gameData.matCens.botGens [i]), 1, cfP);
		}
	CFRead (&gameData.reactor.triggers, sizeof (tReactorTriggers), 1, cfP);
	if (CFReadBoundedInt (MAX_FUEL_CENTERS, &gameData.matCens.nFuelCenters, cfP))
		return 0;
	CFRead (gameData.matCens.fuelCenters, sizeof (tFuelCenInfo), gameData.matCens.nFuelCenters, cfP);

	// Restore the control cen info
	gameData.reactor.states [0].bHit = CFReadInt (cfP);
	gameData.reactor.states [0].bSeenPlayer = CFReadInt (cfP);
	gameData.reactor.states [0].nNextFireTime = CFReadInt (cfP);
	gameData.reactor.bPresent = CFReadInt (cfP);
	gameData.reactor.states [0].nDeadObj = CFReadInt (cfP);
	// Restore the AI state
	AIRestoreBinState (cfP, sgVersion);
	// Restore the automap visited info
	if (sgVersion > 37)
		CFRead (gameData.render.mine.bAutomapVisited, sizeof (ushort), MAX_SEGMENTS, cfP);
	else {
		int	i, j = (sgVersion > 22) ? MAX_SEGMENTS : MAX_SEGMENTS_D2;
		for (i = 0; i < j; i++)
			gameData.render.mine.bAutomapVisited [i] = (ushort) CFReadByte (cfP);
		}
	
	CFRead (gameData.render.mine.bAutomapVisited, sizeof (ubyte), (sgVersion > 22) ? MAX_SEGMENTS : MAX_SEGMENTS_D2, cfP);

	//	Restore hacked up weapon system stuff.
	gameData.fusion.xNextSoundTime = gameData.time.xGame;
	gameData.fusion.xAutoFireTime = 0;
	gameData.laser.xNextFireTime = 
	gameData.missiles.xNextFireTime = gameData.time.xGame;
	gameData.laser.xLastFiredTime = 
	gameData.missiles.xLastFiredTime = gameData.time.xGame;
}
gameData.app.nStateGameId = 0;

if (sgVersion >= 7)	{
	CFRead (&gameData.app.nStateGameId, sizeof (uint), 1, cfP);
	CFRead (&gameStates.app.cheats.bLaserRapidFire, sizeof (int), 1, cfP);
	CFRead (&gameStates.app.bLunacy, sizeof (int), 1, cfP);		//	Yes, writing this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
	CFRead (&gameStates.app.bLunacy, sizeof (int), 1, cfP);
	if (gameStates.app.bLunacy)
		DoLunacyOn ();
}

if (sgVersion >= 17) {
	CFRead (gameData.marker.objects, sizeof (gameData.marker.objects), 1, cfP);
	CFRead (gameData.marker.nOwner, sizeof (gameData.marker.nOwner), 1, cfP);
	CFRead (gameData.marker.szMessage, sizeof (gameData.marker.szMessage), 1, cfP);
}
else {
	int num,dummy;

	// skip dummy info
	CFRead (&num, sizeof (int), 1, cfP);       //was NumOfMarkers
	CFRead (&dummy, sizeof (int), 1, cfP);     //was CurMarker
	CFSeek (cfP, num * (sizeof (vmsVector) + 40), SEEK_CUR);
	for (num = 0; num < NUM_MARKERS; num++)
		gameData.marker.objects [num] = -1;
}

if (sgVersion >= 11) {
	if (bSecretRestore != 1)
		CFRead (&gameData.physics.xAfterburnerCharge, sizeof (fix), 1, cfP);
	else {
		fix	dummy_fix;
		CFRead (&dummy_fix, sizeof (fix), 1, cfP);
	}
}
if (sgVersion >= 12) {
	//read last was super information
	CFRead (&bLastPrimaryWasSuper, sizeof (bLastPrimaryWasSuper), 1, cfP);
	CFRead (&bLastSecondaryWasSuper, sizeof (bLastSecondaryWasSuper), 1, cfP);
	CFRead (&gameData.render.xFlashEffect, sizeof (int), 1, cfP);
	CFRead (&gameData.render.xTimeFlashLastPlayed, sizeof (int), 1, cfP);
	CFRead (&gameStates.ogl.palAdd.red, sizeof (int), 1, cfP);
	CFRead (&gameStates.ogl.palAdd.green, sizeof (int), 1, cfP);
	CFRead (&gameStates.ogl.palAdd.blue, sizeof (int), 1, cfP);
	}
else {
	ResetPaletteAdd ();
	}

//	Load gameData.render.lights.subtracted
if (sgVersion >= 16) {
	CFRead (gameData.render.lights.subtracted, sizeof (gameData.render.lights.subtracted [0]), (sgVersion > 22) ? MAX_SEGMENTS : MAX_SEGMENTS_D2, cfP);
	ApplyAllChangedLight ();
	//ComputeAllStaticLight ();	//	set xAvgSegLight field in tSegment struct.  See note at that function.
	}
else
	memset (gameData.render.lights.subtracted, 0, sizeof (gameData.render.lights.subtracted));

if (bSecretRestore) 
	gameStates.app.bFirstSecretVisit = 0;
else if (sgVersion >= 20)
	CFRead (&gameStates.app.bFirstSecretVisit, sizeof (gameStates.app.bFirstSecretVisit), 1, cfP);
else
	gameStates.app.bFirstSecretVisit = 1;

if (sgVersion >= 22) {
	if (bSecretRestore != 1)
		CFRead (&gameData.omega.xCharge, sizeof (fix), 1, cfP);
	else {
		fix	dummy_fix;
		CFRead (&dummy_fix, sizeof (fix), 1, cfP);
		}
	}
*nLevel = nCurrentLevel;
return 1;
}

//------------------------------------------------------------------------------

int StateRestoreAllSub (const char *filename, int bMulti, int bSecretRestore)
{
	CFILE		cf;
	char		szDesc [DESC_LENGTH + 1];
	char		id [5];
	int		nLevel, sgVersion, i;
	fix		xOldGameTime = gameData.time.xGame;

if (!CFOpen (&cf, filename, gameFolders.szSaveDir, "rb", 0))
	return 0;
StopTime ();
//Read id
CFRead (id, sizeof (char)*4, 1, &cf);
if (memcmp (id, dgss_id, 4)) {
	CFClose (&cf);
	StartTime (1);
	return 0;
	}
//Read sgVersion
CFRead (&sgVersion, sizeof (int), 1, &cf);
if (sgVersion < STATE_COMPATIBLE_VERSION)	{
	CFClose (&cf);
	StartTime (1);
	return 0;
	}
// Read description
CFRead (szDesc, sizeof (char) * DESC_LENGTH, 1, &cf);
// Skip the current screen shot...
CFSeek (&cf, (sgVersion < 26) ? THUMBNAIL_W * THUMBNAIL_H : THUMBNAIL_LW * THUMBNAIL_LH, SEEK_CUR);
// And now...skip the goddamn palette stuff that somebody forgot to add
CFSeek (&cf, 768, SEEK_CUR);
if (sgVersion < 27)
	i = StateRestoreBinGameData (&cf, sgVersion, bMulti, bSecretRestore, xOldGameTime, &nLevel);
else
	i = StateRestoreUniGameData (&cf, sgVersion, bMulti, bSecretRestore, xOldGameTime, &nLevel);
CFClose (&cf);
if (!i) {
	StartTime (1);
	return 0;
	}
FixObjectSegs ();
FixObjectSizes ();
//ComputeNearestLights (nLevel);
ComputeStaticDynLighting (nLevel);
SetupEffects ();
InitReactorForLevel (1);
AddPlayerLoadout ();
SetMaxOmegaCharge ();
SetEquipGenStates ();
if (!IsMultiGame)
	InitEntropySettings (0);	//required for repair centers
else {
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	  if (!gameData.multiplayer.players [i].connected) {
			NetworkDisconnectPlayer (i);
  			CreatePlayerAppearanceEffect (OBJECTS + gameData.multiplayer.players [i].nObject);
	      }
		}
	}
gameData.objs.viewerP = 
gameData.objs.consoleP = OBJECTS + LOCALPLAYER.nObject;
StartTime (1);
if (!extraGameInfo [0].nBossCount && (!IsMultiGame || IsCoopGame) && OpenExits ())
	InitCountdown (NULL, 1, -1);
return 1;
}

//------------------------------------------------------------------------------
//	When loading a saved game, segp->xAvgSegLight is bogus.
//	This is because ApplyAllChangedLight, which is supposed to properly update this value,
//	cannot do so because it needs the original light cast from a light which is no longer there.
//	That is, a light has been blown out, so the texture remaining casts 0 light, but the static light
//	which is present in the xAvgSegLight field contains the light cast from that light.
void ComputeAllStaticLight (void)
{
	int		h, i, j, k;
	tSegment	*segP;
	tSide		*sideP;
	fix		total_light;

	for (i=0, segP = gameData.segs.segments; i<=gameData.segs.nLastSegment; i++, segP++) {
		total_light = 0;
		for (h = j = 0, sideP = segP->sides; j < MAX_SIDES_PER_SEGMENT; j++, sideP++) {
			if ((segP->children [j] < 0) || IS_WALL (sideP->nWall)) {
				h++;
				for (k = 0; k < 4; k++)
					total_light += sideP->uvls [k].l;
			}
		}
		gameData.segs.segment2s [i].xAvgSegLight = h ? total_light / (h * 4) : 0;
	}
}

//------------------------------------------------------------------------------

int StateGetGameId (char *filename)
{
	int sgVersion;
	CFILE cf;
	int bBetweenLevels;
	char mission [16];
	char szDesc [DESC_LENGTH+1];
	char id [5];
	int dumbint;

if (!CFOpen (&cf, filename, gameFolders.szSaveDir, "rb", 0))
	return 0;
//Read id
CFRead (id, sizeof (char)*4, 1, &cf);
if (memcmp (id, dgss_id, 4)) {
	CFClose (&cf);
	return 0;
	}
//Read sgVersion
CFRead (&sgVersion, sizeof (int), 1, &cf);
if (sgVersion < STATE_COMPATIBLE_VERSION)	{
CFClose (&cf);
return 0;
}
// Read description
CFRead (szDesc, sizeof (char)*DESC_LENGTH, 1, &cf);
// Skip the current screen shot...
CFSeek (&cf, (sgVersion < 26) ? THUMBNAIL_W * THUMBNAIL_H : THUMBNAIL_LW * THUMBNAIL_LH, SEEK_CUR);
// And now...skip the palette stuff that somebody forgot to add
CFSeek (&cf, 768, SEEK_CUR);
// Read the Between levels flag...
CFRead (&bBetweenLevels, sizeof (int), 1, &cf);
Assert (bBetweenLevels == 0);	//between levels save ripped out
// Read the mission info...
CFRead (mission, sizeof (char), 9, &cf);
//Read level info
CFRead (&dumbint, sizeof (int), 1, &cf);
CFRead (&dumbint, sizeof (int), 1, &cf);
//Restore gameData.time.xGame
CFRead (&dumbint, sizeof (fix), 1, &cf);
CFRead (&gameData.app.nStateGameId, sizeof (int), 1, &cf);
return (gameData.app.nStateGameId);
}

//------------------------------------------------------------------------------
//eof

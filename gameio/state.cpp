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
	int	i = nCurItem - NM_IMG_SPACE;
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
	CFile cf;
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
	if (cf.Open (filename [i], gameFolders.szSaveDir, "rb", 0)) {
		//Read id
		cf.Read (id, sizeof (char)*4, 1);
		if (!memcmp (id, dgss_id, 4)) {
			//Read sgVersion
			cf.Read (&sgVersion, sizeof (int), 1);
			if (sgVersion >= STATE_COMPATIBLE_VERSION)	{
				// Read description
				cf.Read (szDesc [i], sizeof (char)*DESC_LENGTH, 1);
				valid = 1;
				}
			}
			cf.Close ();
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
	CFile			cf;
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
		if (cf.Open (filename [i], gameFolders.szSaveDir, "rb", 0)) {
			//Read id
			cf.Read (id, sizeof (char) * 4, 1);
			if (!memcmp (id, dgss_id, 4)) {
				//Read sgVersion
				cf.Read (&sgVersion, sizeof (int), 1);
				if (sgVersion >= STATE_COMPATIBLE_VERSION)	{
					// Read description
					if (i < NUM_SAVES)
						sprintf (szDesc [j], "%d. ", i + 1);
					else
						strcpy (szDesc [j], "   ");
					cf.Read (szDesc [j] + 3, sizeof (char) * DESC_LENGTH, 1);
					// rpad_string (szDesc [i], DESC_LENGTH-1);
					ADD_MENU (j + NM_IMG_SPACE, szDesc [j], (i < NUM_SAVES) ? -1 : 0, NULL);
					// Read thumbnail
					if (sgVersion < 26) {
						sc_bmp [i] = GrCreateBitmap (THUMBNAIL_W, THUMBNAIL_H, 1);
						cf.Read (sc_bmp [i]->bmTexBuf, THUMBNAIL_W * THUMBNAIL_H, 1);
						}
					else {
						sc_bmp [i] = GrCreateBitmap (THUMBNAIL_LW, THUMBNAIL_LH, 1);
						cf.Read (sc_bmp [i]->bmTexBuf, THUMBNAIL_LW * THUMBNAIL_LH, 1);
						}
					if (sgVersion >= 9) {
						cf.Read (pal, 3, 256);
						GrRemapBitmapGood (sc_bmp [i], pal, -1, -1);
						}
					nSaves++;
					valid = 1;
					}
				}
			cf.Close ();
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

#ifdef _WIN32_WCE
# define errno -1
# define strerror (x) "Unknown Error"
#endif


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
	CFile::Delete (SECRETB_FILENAME, gameFolders.szSaveDir);
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
			CFile cf;
			if (cf.Exist (temp_fname,gameFolders.szSaveDir,0)) {
				rval = cf.Delete (temp_fname, gameFolders.szSaveDir);
				Assert (rval == 0);	//	Oops, error deleting file in temp_fname.
				}
			if (cf.Exist (SECRETC_FILENAME,gameFolders.szSaveDir,0)) {
				rval = cf.Copy (SECRETC_FILENAME, temp_fname);
				Assert (rval == 0);	//	Oops, error copying temp_fname to secret.sgc!
				}
			}
		}

		//	Save file we're going to save over in last slot and call " [autosave backup]"
	if (!pszFilenameOverride) {
		CFile cf;
		
		if (cf.Open (filename, gameFolders.szSaveDir, "rb",0)) {
			char	newname [128];

			sprintf (newname, "%s.sg%x", LOCALPLAYER.callsign, NUM_SAVES);
			cf.Seek (DESC_OFFSET, SEEK_SET);
			cf.Write ((char *) " [autosave backup]", sizeof (char) * DESC_LENGTH, 1);
			cf.Close ();
			cf.Delete (newname, gameFolders.szSaveDir);
			cf.Rename (filename, newname, gameFolders.szSaveDir);
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

void StateSaveBinGameData (CFile cf, int bBetweenLevels)
{
	int		i, j;
	ushort	nWall, nTexture;
	short		nObjsWithTrigger, nObject, nFirstTrigger;
	tObject	*objP;

// Save the Between levels flag...
cf.Write (&bBetweenLevels, sizeof (int), 1);
// Save the mission info...
cf.Write (gameData.missions.list + gameData.missions.nCurrentMission, sizeof (char), 9);
//Save level info
cf.Write (&gameData.missions.nCurrentLevel, sizeof (int), 1);
cf.Write (&gameData.missions.nNextLevel, sizeof (int), 1);
//Save gameData.time.xGame
cf.Write (&gameData.time.xGame, sizeof (fix), 1);
// If coop save, save all
if (gameData.app.nGameMode & GM_MULTI_COOP) {
	cf.Write (&gameData.app.nStateGameId, sizeof (int), 1);
	cf.Write (&netGame, sizeof (tNetgameInfo), 1);
	cf.Write (&netPlayers, sizeof (tAllNetPlayersInfo), 1);
	cf.Write (&gameData.multiplayer.nPlayers, sizeof (int), 1);
	cf.Write (&gameData.multiplayer.nLocalPlayer, sizeof (int), 1);
	for (i = 0; i < gameData.multiplayer.nPlayers; i++)
		cf.Write (&gameData.multiplayer.players [i], sizeof (tPlayer), 1);
	}
//Save tPlayer info
cf.Write (&LOCALPLAYER, sizeof (tPlayer), 1);
// Save the current weapon info
cf.Write (&gameData.weapons.nPrimary, sizeof (sbyte), 1);
cf.Write (&gameData.weapons.nSecondary, sizeof (sbyte), 1);
// Save the difficulty level
cf.Write (&gameStates.app.nDifficultyLevel, sizeof (int), 1);
// Save cheats enabled
cf.Write (&gameStates.app.cheats.bEnabled, sizeof (int), 1);
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
	cf.Write (&i, sizeof (int), 1);
	cf.Write (OBJECTS, sizeof (tObject), i);
//Save tWall info
	i = gameData.walls.nWalls;
	cf.Write (&i, sizeof (int), 1);
	cf.Write (gameData.walls.walls, sizeof (tWall), i);
//Save exploding wall info
	i = MAX_EXPLODING_WALLS;
	cf.Write (&i, sizeof (int), 1);
	cf.Write (gameData.walls.explWalls, sizeof (*gameData.walls.explWalls), i);
//Save door info
	i = gameData.walls.nOpenDoors;
	cf.Write (&i, sizeof (int), 1);
	cf.Write (gameData.walls.activeDoors, sizeof (tActiveDoor), i);
//Save cloaking tWall info
	i = gameData.walls.nCloaking;
	cf.Write (&i, sizeof (int), 1);
	cf.Write (gameData.walls.cloaking, sizeof (tCloakingWall), i);
//Save tTrigger info
	cf.Write (&gameData.trigs.nTriggers, sizeof (int), 1);
	cf.Write (gameData.trigs.triggers, sizeof (tTrigger), gameData.trigs.nTriggers);
	cf.Write (&gameData.trigs.nObjTriggers, sizeof (int), 1);
	cf.Write (gameData.trigs.objTriggers, sizeof (tTrigger), gameData.trigs.nObjTriggers);
	cf.Write (gameData.trigs.objTriggerRefs, sizeof (tObjTriggerRef), gameData.trigs.nObjTriggers);
	nObjsWithTrigger = 0;
	FORALL_OBJS (objP, nObject) {
		nObject = OBJ_IDX (objP);
		nFirstTrigger = gameData.trigs.firstObjTrigger [nObject];
		if ((nFirstTrigger >= 0) && (nFirstTrigger < gameData.trigs.nObjTriggers))
			nObjsWithTrigger++;
		}
	cf.Write (&nObjsWithTrigger, sizeof (nObjsWithTrigger), 1);
	FORALL_OBJS (objP, nObject) {
		nObject = OBJ_IDX (objP);
		nFirstTrigger = gameData.trigs.firstObjTrigger [nObject];
		if ((nFirstTrigger >= 0) && (nFirstTrigger < gameData.trigs.nObjTriggers)) {
			cf.Write (&nObject, sizeof (nObject), 1);
			cf.Write (&nFirstTrigger, sizeof (nFirstTrigger), 1);
			}
		}
//Save tmap info
	for (i = 0; i <= gameData.segs.nLastSegment; i++) {
		for (j = 0; j < 6; j++)	{
			nWall = WallNumI ((short) i, (short) j);
			cf.Write (&nWall, sizeof (short), 1);
			cf.Write (&gameData.segs.segments [i].sides [j].nBaseTex, sizeof (short), 1);
			nTexture = gameData.segs.segments [i].sides [j].nOvlTex | (gameData.segs.segments [i].sides [j].nOvlOrient << 14);
			cf.Write (&nTexture, sizeof (short), 1);
			}
		}
// Save the fuelcen info
	cf.Write (&gameData.reactor.bDestroyed, sizeof (int), 1);
	cf.Write (&gameData.reactor.countdown.nTimer, sizeof (int), 1);
	cf.Write (&gameData.matCens.nBotCenters, sizeof (int), 1);
	cf.Write (gameData.matCens.botGens, sizeof (tMatCenInfo), gameData.matCens.nBotCenters);
	cf.Write (&gameData.reactor.triggers, sizeof (tReactorTriggers), 1);
	cf.Write (&gameData.matCens.nFuelCenters, sizeof (int), 1);
	cf.Write (gameData.matCens.fuelCenters, sizeof (tFuelCenInfo), gameData.matCens.nFuelCenters);
	cf.Write (&gameData.matCens.nFuelCenters, sizeof (int), 1);
	cf.Write (gameData.matCens.fuelCenters, sizeof (tFuelCenInfo), gameData.matCens.nFuelCenters);
// Save the control cen info
	cf.Write (&gameData.reactor.bPresent, sizeof (int), 1);
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		cf.Write (&gameData.reactor.states [i].nObject, sizeof (int), 1);
		cf.Write (&gameData.reactor.states [i].bHit, sizeof (int), 1);
		cf.Write (&gameData.reactor.states [i].bSeenPlayer, sizeof (int), 1);
		cf.Write (&gameData.reactor.states [i].nNextFireTime, sizeof (int), 1);
		cf.Write (&gameData.reactor.states [i].nDeadObj, sizeof (int), 1);
		}
// Save the AI state
	AISaveBinState (cf);

// Save the automap visited info
	cf.Write (gameData.render.mine.bAutomapVisited, sizeof (ushort) * MAX_SEGMENTS, 1);
	}
cf.Write (&gameData.app.nStateGameId, sizeof (uint), 1);
cf.Write (&gameStates.app.cheats.bLaserRapidFire, sizeof (int), 1);
cf.Write (&gameStates.app.bLunacy, sizeof (int), 1);		//	Yes, writing this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
cf.Write (&gameStates.app.bLunacy, sizeof (int), 1);
// Save automap marker info
cf.Write (gameData.marker.objects, sizeof (gameData.marker.objects), 1);
cf.Write (gameData.marker.nOwner, sizeof (gameData.marker.nOwner), 1);
cf.Write (gameData.marker.szMessage, sizeof (gameData.marker.szMessage), 1);
cf.Write (&gameData.physics.xAfterburnerCharge, sizeof (fix), 1);
//save last was super information
cf.Write (&bLastPrimaryWasSuper, sizeof (bLastPrimaryWasSuper), 1);
cf.Write (&bLastSecondaryWasSuper, sizeof (bLastSecondaryWasSuper), 1);
//	Save flash effect stuff
cf.Write (&gameData.render.xFlashEffect, sizeof (int), 1);
cf.Write (&gameData.render.xTimeFlashLastPlayed, sizeof (int), 1);
cf.Write (&gameStates.ogl.palAdd.red, sizeof (int), 1);
cf.Write (&gameStates.ogl.palAdd.green, sizeof (int), 1);
cf.Write (&gameStates.ogl.palAdd.blue, sizeof (int), 1);
cf.Write (gameData.render.lights.subtracted, sizeof (gameData.render.lights.subtracted [0]), MAX_SEGMENTS);
cf.Write (&gameStates.app.bFirstSecretVisit, sizeof (gameStates.app.bFirstSecretVisit), 1);
cf.Write (&gameData.omega.xCharge, sizeof (gameData.omega.xCharge), 1);
}

//------------------------------------------------------------------------------

void StateSaveNetGame (CFile& cf)
{
	int	i, j;

cf.WriteByte (netGame.nType);
cf.WriteInt (netGame.nSecurity);
cf.Write (netGame.szGameName, 1, NETGAME_NAME_LEN + 1);
cf.Write (netGame.szMissionTitle, 1, MISSION_NAME_LEN + 1);
cf.Write (netGame.szMissionName, 1, 9);
cf.WriteInt (netGame.nLevel);
cf.WriteByte ((sbyte) netGame.gameMode);
cf.WriteByte ((sbyte) netGame.bRefusePlayers);
cf.WriteByte ((sbyte) netGame.difficulty);
cf.WriteByte ((sbyte) netGame.gameStatus);
cf.WriteByte ((sbyte) netGame.nNumPlayers);
cf.WriteByte ((sbyte) netGame.nMaxPlayers);
cf.WriteByte ((sbyte) netGame.nConnected);
cf.WriteByte ((sbyte) netGame.gameFlags);
cf.WriteByte ((sbyte) netGame.protocolVersion);
cf.WriteByte ((sbyte) netGame.versionMajor);
cf.WriteByte ((sbyte) netGame.versionMinor);
cf.WriteByte ((sbyte) netGame.teamVector);
cf.WriteByte ((sbyte) netGame.DoMegas);
cf.WriteByte ((sbyte) netGame.DoSmarts);
cf.WriteByte ((sbyte) netGame.DoFusions);
cf.WriteByte ((sbyte) netGame.DoHelix);
cf.WriteByte ((sbyte) netGame.DoPhoenix);
cf.WriteByte ((sbyte) netGame.DoAfterburner);
cf.WriteByte ((sbyte) netGame.DoInvulnerability);
cf.WriteByte ((sbyte) netGame.DoCloak);
cf.WriteByte ((sbyte) netGame.DoGauss);
cf.WriteByte ((sbyte) netGame.DoVulcan);
cf.WriteByte ((sbyte) netGame.DoPlasma);
cf.WriteByte ((sbyte) netGame.DoOmega);
cf.WriteByte ((sbyte) netGame.DoSuperLaser);
cf.WriteByte ((sbyte) netGame.DoProximity);
cf.WriteByte ((sbyte) netGame.DoSpread);
cf.WriteByte ((sbyte) netGame.DoSmartMine);
cf.WriteByte ((sbyte) netGame.DoFlash);
cf.WriteByte ((sbyte) netGame.DoGuided);
cf.WriteByte ((sbyte) netGame.DoEarthShaker);
cf.WriteByte ((sbyte) netGame.DoMercury);
cf.WriteByte ((sbyte) netGame.bAllowMarkerView);
cf.WriteByte ((sbyte) netGame.bIndestructibleLights);
cf.WriteByte ((sbyte) netGame.DoAmmoRack);
cf.WriteByte ((sbyte) netGame.DoConverter);
cf.WriteByte ((sbyte) netGame.DoHeadlight);
cf.WriteByte ((sbyte) netGame.DoHoming);
cf.WriteByte ((sbyte) netGame.DoLaserUpgrade);
cf.WriteByte ((sbyte) netGame.DoQuadLasers);
cf.WriteByte ((sbyte) netGame.bShowAllNames);
cf.WriteByte ((sbyte) netGame.BrightPlayers);
cf.WriteByte ((sbyte) netGame.invul);
cf.WriteByte ((sbyte) netGame.FriendlyFireOff);
for (i = 0; i < 2; i++)
	cf.Write (netGame.szTeamName [i], 1, CALLSIGN_LEN + 1);		// 18 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	cf.WriteInt (netGame.locations [i]);
for (i = 0; i < MAX_PLAYERS; i++)
	for (j = 0; j < MAX_PLAYERS; j++)
		cf.WriteShort (netGame.kills [i][j]);			// 128 bytes
cf.WriteShort (netGame.nSegmentCheckSum);				// 2 bytes
for (i = 0; i < 2; i++)
	cf.WriteShort (netGame.teamKills [i]);				// 4 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	cf.WriteShort (netGame.killed [i]);					// 16 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	cf.WriteShort (netGame.playerKills [i]);			// 16 bytes
cf.WriteInt (netGame.KillGoal);							// 4 bytes
cf.WriteFix (netGame.xPlayTimeAllowed);					// 4 bytes
cf.WriteFix (netGame.xLevelTime);							// 4 bytes
cf.WriteInt (netGame.controlInvulTime);				// 4 bytes
cf.WriteInt (netGame.monitorVector);					// 4 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	cf.WriteInt (netGame.playerScore [i]);				// 32 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	cf.WriteByte ((sbyte) netGame.playerFlags [i]);	// 8 bytes
cf.WriteShort (PacketsPerSec ());					// 2 bytes
cf.WriteByte ((sbyte) netGame.bShortPackets);			// 1 bytes
// 279 bytes
// 355 bytes total
cf.Write (netGame.AuxData, NETGAME_AUX_SIZE, 1);  // Storage for protocol-specific data (e.g., multicast session and port)
}

//------------------------------------------------------------------------------

void StateSaveNetPlayers (CFile& cf)
{
	int	i;

cf.WriteByte ((sbyte) netPlayers.nType);
cf.WriteInt (netPlayers.nSecurity);
for (i = 0; i < MAX_PLAYERS + 4; i++) {
	cf.Write (netPlayers.players [i].callsign, 1, CALLSIGN_LEN + 1);
	cf.Write (netPlayers.players [i].network.ipx.server, 1, 4);
	cf.Write (netPlayers.players [i].network.ipx.node, 1, 6);
	cf.WriteByte ((sbyte) netPlayers.players [i].versionMajor);
	cf.WriteByte ((sbyte) netPlayers.players [i].versionMinor);
	cf.WriteByte ((sbyte) netPlayers.players [i].computerType);
	cf.WriteByte (netPlayers.players [i].connected);
	cf.WriteShort ((short) netPlayers.players [i].socket);
	cf.WriteByte ((sbyte) netPlayers.players [i].rank);
	}
}

//------------------------------------------------------------------------------

void StateSavePlayer (tPlayer *playerP, CFile& cf)
{
	int	i;

cf.Write (playerP->callsign, 1, CALLSIGN_LEN + 1); // The callsign of this tPlayer, for net purposes.
cf.Write (playerP->netAddress, 1, 6);					// The network address of the player.
cf.WriteByte (playerP->connected);            // Is the tPlayer connected or not?
cf.WriteInt (playerP->nObject);                // What tObject number this tPlayer is. (made an int by mk because it's very often referenced)
cf.WriteInt (playerP->nPacketsGot);         // How many packets we got from them
cf.WriteInt (playerP->nPacketsSent);        // How many packets we sent to them
cf.WriteInt ((int) playerP->flags);           // Powerup flags, see below...
cf.WriteFix (playerP->energy);                // Amount of energy remaining.
cf.WriteFix (playerP->shields);               // shields remaining (protection)
cf.WriteByte (playerP->lives);                // Lives remaining, 0 = game over.
cf.WriteByte (playerP->level);                // Current level tPlayer is playing. (must be signed for secret levels)
cf.WriteByte ((sbyte) playerP->laserLevel);  // Current level of the laser.
cf.WriteByte (playerP->startingLevel);       // What level the tPlayer started on.
cf.WriteShort (playerP->nKillerObj);       // Who killed me.... (-1 if no one)
cf.WriteShort ((short) playerP->primaryWeaponFlags);   // bit set indicates the tPlayer has this weapon.
cf.WriteShort ((short) playerP->secondaryWeaponFlags); // bit set indicates the tPlayer has this weapon.
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	cf.WriteShort ((short) playerP->primaryAmmo [i]); // How much ammo of each nType.
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
	cf.WriteShort ((short) playerP->secondaryAmmo [i]); // How much ammo of each nType.
#if 1 //for inventory system
cf.WriteByte ((sbyte) playerP->nInvuls);
cf.WriteByte ((sbyte) playerP->nCloaks);
#endif
cf.WriteInt (playerP->lastScore);             // Score at beginning of current level.
cf.WriteInt (playerP->score);                  // Current score.
cf.WriteFix (playerP->timeLevel);             // Level time played
cf.WriteFix (playerP->timeTotal);             // Game time played (high word = seconds)
if (playerP->cloakTime == 0x7fffffff)				// cloak cheat active
	cf.WriteFix (playerP->cloakTime);			// Time invulnerable
else
	cf.WriteFix (playerP->cloakTime - gameData.time.xGame);      // Time invulnerable
if (playerP->invulnerableTime == 0x7fffffff)		// invul cheat active
	cf.WriteFix (playerP->invulnerableTime);      // Time invulnerable
else
	cf.WriteFix (playerP->invulnerableTime - gameData.time.xGame);      // Time invulnerable
cf.WriteShort (playerP->nKillGoalCount);          // Num of players killed this level
cf.WriteShort (playerP->netKilledTotal);       // Number of times killed total
cf.WriteShort (playerP->netKillsTotal);        // Number of net kills total
cf.WriteShort (playerP->numKillsLevel);        // Number of kills this level
cf.WriteShort (playerP->numKillsTotal);        // Number of kills total
cf.WriteShort (playerP->numRobotsLevel);       // Number of initial robots this level
cf.WriteShort (playerP->numRobotsTotal);       // Number of robots total
cf.WriteShort ((short) playerP->hostages.nRescued); // Total number of hostages rescued.
cf.WriteShort ((short) playerP->hostages.nTotal);         // Total number of hostages.
cf.WriteByte ((sbyte) playerP->hostages.nOnBoard);      // Number of hostages on ship.
cf.WriteByte ((sbyte) playerP->hostages.nLevel);         // Number of hostages on this level.
cf.WriteFix (playerP->homingObjectDist);     // Distance of nearest homing tObject.
cf.WriteByte (playerP->hoursLevel);            // Hours played (since timeTotal can only go up to 9 hours)
cf.WriteByte (playerP->hoursTotal);            // Hours played (since timeTotal can only go up to 9 hours)
}

//------------------------------------------------------------------------------

void StateSaveObject (tObject *objP, CFile& cf)
{
cf.WriteInt (objP->info.nSignature);      
cf.WriteByte ((sbyte) objP->info.nType); 
cf.WriteByte ((sbyte) objP->info.nId);
cf.WriteShort (objP->info.nNextInSeg);
cf.WriteShort (objP->info.nPrevInSeg);
cf.WriteByte ((sbyte) objP->info.controlType);
cf.WriteByte ((sbyte) objP->info.movementType);
cf.WriteByte ((sbyte) objP->info.renderType);
cf.WriteByte ((sbyte) objP->info.nFlags);
cf.WriteShort (objP->info.nSegment);
cf.WriteShort (objP->info.nAttachedObj);
cf.WriteVector (OBJPOS (objP)->vPos);     
cf.WriteMatrix (OBJPOS (objP)->mOrient);  
cf.WriteFix (objP->info.xSize); 
cf.WriteFix (objP->info.xShields);
cf.WriteVector (objP->info.vLastPos);  
cf.WriteByte (objP->info.contains.nType); 
cf.WriteByte (objP->info.contains.nId);   
cf.WriteByte (objP->info.contains.nCount);
cf.WriteByte (objP->info.nCreator);
cf.WriteFix (objP->info.xLifeLeft);   
if (objP->info.movementType == MT_PHYSICS) {
	cf.WriteVector (objP->mType.physInfo.velocity);   
	cf.WriteVector (objP->mType.physInfo.thrust);     
	cf.WriteFix (objP->mType.physInfo.mass);       
	cf.WriteFix (objP->mType.physInfo.drag);       
	cf.WriteFix (objP->mType.physInfo.brakes);     
	cf.WriteVector (objP->mType.physInfo.rotVel);     
	cf.WriteVector (objP->mType.physInfo.rotThrust);  
	cf.WriteFixAng (objP->mType.physInfo.turnRoll);   
	cf.WriteShort ((short) objP->mType.physInfo.flags);      
	}
else if (objP->info.movementType == MT_SPINNING) {
	cf.WriteVector(objP->mType.spinRate);  
	}
switch (objP->info.controlType) {
	case CT_WEAPON:
		cf.WriteShort (objP->cType.laserInfo.parent.nType);
		cf.WriteShort (objP->cType.laserInfo.parent.nObject);
		cf.WriteInt (objP->cType.laserInfo.parent.nSignature);
		cf.WriteFix (objP->cType.laserInfo.xCreationTime);
		if (objP->cType.laserInfo.nLastHitObj)
			cf.WriteShort (gameData.objs.nHitObjects [OBJ_IDX (objP) * MAX_HIT_OBJECTS + objP->cType.laserInfo.nLastHitObj - 1]);
		else
			cf.WriteShort (-1);
		cf.WriteShort (objP->cType.laserInfo.nHomingTarget);
		cf.WriteFix (objP->cType.laserInfo.xScale);
		break;

	case CT_EXPLOSION:
		cf.WriteFix (objP->cType.explInfo.nSpawnTime);
		cf.WriteFix (objP->cType.explInfo.nDeleteTime);
		cf.WriteShort (objP->cType.explInfo.nDeleteObj);
		cf.WriteShort (objP->cType.explInfo.attached.nParent);
		cf.WriteShort (objP->cType.explInfo.attached.nPrev);
		cf.WriteShort (objP->cType.explInfo.attached.nNext);
		break;

	case CT_AI:
		cf.WriteByte ((sbyte) objP->cType.aiInfo.behavior);
		cf.Write (objP->cType.aiInfo.flags, 1, MAX_AI_FLAGS);
		cf.WriteShort (objP->cType.aiInfo.nHideSegment);
		cf.WriteShort (objP->cType.aiInfo.nHideIndex);
		cf.WriteShort (objP->cType.aiInfo.nPathLength);
		cf.WriteByte (objP->cType.aiInfo.nCurPathIndex);
		cf.WriteByte (objP->cType.aiInfo.bDyingSoundPlaying);
		cf.WriteShort (objP->cType.aiInfo.nDangerLaser);
		cf.WriteInt (objP->cType.aiInfo.nDangerLaserSig);
		cf.WriteFix (objP->cType.aiInfo.xDyingStartTime);
		break;

	case CT_LIGHT:
		cf.WriteFix (objP->cType.lightInfo.intensity);
		break;

	case CT_POWERUP:
		cf.WriteInt (objP->cType.powerupInfo.nCount);
		cf.WriteFix (objP->cType.powerupInfo.xCreationTime);
		cf.WriteInt (objP->cType.powerupInfo.nFlags);
		break;
	}
switch (objP->info.renderType) {
	case RT_MORPH:
	case RT_POLYOBJ: {
		int i;
		cf.WriteInt (objP->rType.polyObjInfo.nModel);
		for (i = 0; i < MAX_SUBMODELS; i++)
			cf.WriteAngVec (objP->rType.polyObjInfo.animAngles [i]);
		cf.WriteInt (objP->rType.polyObjInfo.nSubObjFlags);
		cf.WriteInt (objP->rType.polyObjInfo.nTexOverride);
		cf.WriteInt (objP->rType.polyObjInfo.nAltTextures);
		break;
		}
	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
	case RT_THRUSTER:
		cf.WriteInt (objP->rType.vClipInfo.nClipIndex);
		cf.WriteFix (objP->rType.vClipInfo.xFrameTime);
		cf.WriteByte (objP->rType.vClipInfo.nCurFrame);
		break;

	case RT_LASER:
		break;
	}
}

//------------------------------------------------------------------------------

void StateSaveWall (tWall *wallP, CFile& cf)
{
cf.WriteInt (wallP->nSegment);
cf.WriteInt (wallP->nSide);
cf.WriteFix (wallP->hps);    
cf.WriteInt (wallP->nLinkedWall);
cf.WriteByte ((sbyte) wallP->nType);       
cf.WriteByte ((sbyte) wallP->flags);      
cf.WriteByte ((sbyte) wallP->state);      
cf.WriteByte ((sbyte) wallP->nTrigger);    
cf.WriteByte (wallP->nClip);   
cf.WriteByte ((sbyte) wallP->keys);       
cf.WriteByte (wallP->controllingTrigger);
cf.WriteByte (wallP->cloakValue); 
}

//------------------------------------------------------------------------------

void StateSaveExplWall (tExplWall *wallP, CFile& cf)
{
cf.WriteInt (wallP->nSegment);
cf.WriteInt (wallP->nSide);
cf.WriteFix (wallP->time);    
}

//------------------------------------------------------------------------------

void StateSaveCloakingWall (tCloakingWall *wallP, CFile& cf)
{
	int	i;

cf.WriteShort (wallP->nFrontWall);
cf.WriteShort (wallP->nBackWall); 
for (i = 0; i < 4; i++) {
	cf.WriteFix (wallP->front_ls [i]); 
	cf.WriteFix (wallP->back_ls [i]);
	}
cf.WriteFix (wallP->time);    
}

//------------------------------------------------------------------------------

void StateSaveActiveDoor (tActiveDoor *doorP, CFile& cf)
{
	int	i;

cf.WriteInt (doorP->nPartCount);
for (i = 0; i < 2; i++) {
	cf.WriteShort (doorP->nFrontWall [i]);
	cf.WriteShort (doorP->nBackWall [i]);
	}
cf.WriteFix (doorP->time);    
}

//------------------------------------------------------------------------------

void StateSaveTrigger (tTrigger *triggerP, CFile& cf)
{
	int	i;

cf.WriteByte ((sbyte) triggerP->nType); 
cf.WriteByte ((sbyte) triggerP->flags); 
cf.WriteByte (triggerP->nLinks);
cf.WriteFix (triggerP->value);
cf.WriteFix (triggerP->time);
for (i = 0; i < MAX_TRIGGER_TARGETS; i++) {
	cf.WriteShort (triggerP->nSegment [i]);
	cf.WriteShort (triggerP->nSide [i]);
	}
}

//------------------------------------------------------------------------------

void StateSaveObjTriggerRef (tObjTriggerRef *refP, CFile& cf)
{
cf.WriteShort (refP->prev);
cf.WriteShort (refP->next);
cf.WriteShort (refP->nObject);
}

//------------------------------------------------------------------------------

void StateSaveMatCen (tMatCenInfo *matcenP, CFile& cf)
{
	int	i;

for (i = 0; i < 2; i++)
	cf.WriteInt (matcenP->objFlags [i]);
cf.WriteFix (matcenP->xHitPoints);
cf.WriteFix (matcenP->xInterval);
cf.WriteShort (matcenP->nSegment);
cf.WriteShort (matcenP->nFuelCen);
}

//------------------------------------------------------------------------------

void StateSaveFuelCen (tFuelCenInfo *fuelcenP, CFile& cf)
{
cf.WriteInt (fuelcenP->nType);
cf.WriteInt (fuelcenP->nSegment);
cf.WriteByte (fuelcenP->bFlag);
cf.WriteByte (fuelcenP->bEnabled);
cf.WriteByte (fuelcenP->nLives);
cf.WriteFix (fuelcenP->xCapacity);
cf.WriteFix (fuelcenP->xMaxCapacity);
cf.WriteFix (fuelcenP->xTimer);
cf.WriteFix (fuelcenP->xDisableTime);
cf.WriteVector(fuelcenP->vCenter);
}

//------------------------------------------------------------------------------

void StateSaveReactorTrigger (tReactorTriggers *triggerP, CFile& cf)
{
	int	i;

cf.WriteShort (triggerP->nLinks);
for (i = 0; i < MAX_CONTROLCEN_LINKS; i++) {
	cf.WriteShort (triggerP->nSegment [i]);
	cf.WriteShort (triggerP->nSide [i]);
	}
}

//------------------------------------------------------------------------------

void StateSaveSpawnPoint (int i, CFile& cf)
{
#if DBG
i = cf.Tell;
#endif
cf.WriteVector (gameData.multiplayer.playerInit [i].position.vPos);     
cf.WriteMatrix (gameData.multiplayer.playerInit [i].position.mOrient);  
cf.WriteShort (gameData.multiplayer.playerInit [i].nSegment);
cf.WriteShort (gameData.multiplayer.playerInit [i].nSegType);
}

//------------------------------------------------------------------------------

DBG (static int fPos);

void StateSaveUniGameData (CFile& cf, int bBetweenLevels)
{
	int		i, j;
	short		nObjsWithTrigger, nObject, nFirstTrigger;
	tObject	*objP;

cf.WriteInt (gameData.segs.nMaxSegments);
// Save the Between levels flag...
cf.WriteInt (bBetweenLevels);
// Save the mission info...
cf.Write (gameData.missions.list + gameData.missions.nCurrentMission, sizeof (char), 9);
//Save level info
cf.WriteInt (gameData.missions.nCurrentLevel);
cf.WriteInt (gameData.missions.nNextLevel);
//Save gameData.time.xGame
cf.WriteFix (gameData.time.xGame);
// If coop save, save all
if (IsCoopGame) {
	cf.WriteInt (gameData.app.nStateGameId);
	StateSaveNetGame (cf);
	DBG (fPos = cf.Tell ());
	StateSaveNetPlayers (cf);
	DBG (fPos = cf.Tell ());
	cf.WriteInt (gameData.multiplayer.nPlayers);
	cf.WriteInt (gameData.multiplayer.nLocalPlayer);
	for (i = 0; i < gameData.multiplayer.nPlayers; i++)
		StateSavePlayer (gameData.multiplayer.players + i, cf);
	DBG (fPos = cf.Tell ());
	}
//Save tPlayer info
StateSavePlayer (gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer, cf);
// Save the current weapon info
cf.WriteByte (gameData.weapons.nPrimary);
cf.WriteByte (gameData.weapons.nSecondary);
// Save the difficulty level
cf.WriteInt (gameStates.app.nDifficultyLevel);
// Save cheats enabled
cf.WriteInt (gameStates.app.cheats.bEnabled);
for (i = 0; i < 2; i++) {
	cf.WriteInt (F2X (gameStates.gameplay.slowmo [i].fSpeed));
	cf.WriteInt (gameStates.gameplay.slowmo [i].nState);
	}
for (i = 0; i < MAX_PLAYERS; i++)
	cf.WriteInt (gameData.multiplayer.weaponStates [i].bTripleFusion);
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
	DBG (fPos = cf.Tell ());
//Save tObject info
	i = gameData.objs.nLastObject [0] + 1;
	cf.WriteInt (i);
	for (j = 0; j < i; j++)
		StateSaveObject (OBJECTS + j, cf);
	DBG (fPos = cf.Tell ());
//Save tWall info
	i = gameData.walls.nWalls;
	cf.WriteInt (i);
	for (j = 0; j < i; j++)
		StateSaveWall (gameData.walls.walls + j, cf);
	DBG (fPos = cf.Tell ());
//Save exploding wall info
	i = MAX_EXPLODING_WALLS;
	cf.WriteInt (i);
	for (j = 0; j < i; j++)
		StateSaveExplWall (gameData.walls.explWalls + j, cf);
	DBG (fPos = cf.Tell ());
//Save door info
	i = gameData.walls.nOpenDoors;
	cf.WriteInt (i);
	for (j = 0; j < i; j++)
		StateSaveActiveDoor (gameData.walls.activeDoors + j, cf);
	DBG (fPos = cf.Tell ());
//Save cloaking tWall info
	i = gameData.walls.nCloaking;
	cf.WriteInt (i);
	for (j = 0; j < i; j++)
		StateSaveCloakingWall (gameData.walls.cloaking + j, cf);
	DBG (fPos = cf.Tell ());
//Save tTrigger info
	cf.WriteInt (gameData.trigs.nTriggers);
	for (i = 0; i < gameData.trigs.nTriggers; i++)
		StateSaveTrigger (gameData.trigs.triggers + i, cf);
	DBG (fPos = cf.Tell ());
	cf.WriteInt (gameData.trigs.nObjTriggers);
	if (!gameData.trigs.nObjTriggers)
		cf.WriteShort (0);
	else {
		for (i = 0; i < gameData.trigs.nObjTriggers; i++)
			StateSaveTrigger (gameData.trigs.objTriggers + i, cf);
		for (i = 0; i < gameData.trigs.nObjTriggers; i++)
			StateSaveObjTriggerRef (gameData.trigs.objTriggerRefs + i, cf);
		nObjsWithTrigger = 0;
		FORALL_OBJS (objP, nObject) {
			nObject = OBJ_IDX (objP);
			nFirstTrigger = gameData.trigs.firstObjTrigger [nObject];
			if ((nFirstTrigger >= 0) && (nFirstTrigger < gameData.trigs.nObjTriggers))
				nObjsWithTrigger++;
			}
		cf.WriteShort (nObjsWithTrigger);
		FORALL_OBJS (objP, nObject) {
			nObject = OBJ_IDX (objP);
			nFirstTrigger = gameData.trigs.firstObjTrigger [nObject];
			if ((nFirstTrigger >= 0) && (nFirstTrigger < gameData.trigs.nObjTriggers)) {
				cf.WriteShort (nObject);
				cf.WriteShort (nFirstTrigger);
				}
			}
		}
	DBG (fPos = cf.Tell ());
//Save tmap info
	for (i = 0; i <= gameData.segs.nLastSegment; i++) {
		for (j = 0; j < 6; j++)	{
			ushort nWall = WallNumI ((short) i, (short) j);
			cf.WriteShort (nWall);
			cf.WriteShort (gameData.segs.segments [i].sides [j].nBaseTex);
			cf.WriteShort (gameData.segs.segments [i].sides [j].nOvlTex | (gameData.segs.segments [i].sides [j].nOvlOrient << 14));
			}
		}
	DBG (fPos = cf.Tell ());
// Save the fuelcen info
	cf.WriteInt (gameData.reactor.bDestroyed);
	cf.WriteFix (gameData.reactor.countdown.nTimer);
	DBG (fPos = cf.Tell ());
	cf.WriteInt (gameData.matCens.nBotCenters);
	for (i = 0; i < gameData.matCens.nBotCenters; i++)
		StateSaveMatCen (gameData.matCens.botGens + i, cf);
	cf.WriteInt (gameData.matCens.nEquipCenters);
	for (i = 0; i < gameData.matCens.nEquipCenters; i++)
		StateSaveMatCen (gameData.matCens.equipGens + i, cf);
	StateSaveReactorTrigger (&gameData.reactor.triggers, cf);
	cf.WriteInt (gameData.matCens.nFuelCenters);
	for (i = 0; i < gameData.matCens.nFuelCenters; i++)
		StateSaveFuelCen (gameData.matCens.fuelCenters + i, cf);
	DBG (fPos = cf.Tell ());
// Save the control cen info
	cf.WriteInt (gameData.reactor.bPresent);
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		cf.WriteInt (gameData.reactor.states [i].nObject);
		cf.WriteInt (gameData.reactor.states [i].bHit);
		cf.WriteInt (gameData.reactor.states [i].bSeenPlayer);
		cf.WriteInt (gameData.reactor.states [i].nNextFireTime);
		cf.WriteInt (gameData.reactor.states [i].nDeadObj);
		}
	DBG (fPos = cf.Tell ());
// Save the AI state
	AISaveUniState (cf);

	DBG (fPos = cf.Tell ());
// Save the automap visited info
	for (i = 0; i < MAX_SEGMENTS; i++)
		cf.WriteShort (gameData.render.mine.bAutomapVisited [i]);
	DBG (fPos = cf.Tell ());
	}
cf.WriteInt ((int) gameData.app.nStateGameId);
cf.WriteInt (gameStates.app.cheats.bLaserRapidFire);
cf.WriteInt (gameStates.app.bLunacy);		//	Yes, writing this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
cf.WriteInt (gameStates.app.bLunacy);
// Save automap marker info
for (i = 0; i < NUM_MARKERS; i++)
	cf.WriteShort (gameData.marker.objects [i]);
cf.Write (gameData.marker.nOwner, sizeof (gameData.marker.nOwner), 1);
cf.Write (gameData.marker.szMessage, sizeof (gameData.marker.szMessage), 1);
cf.WriteFix (gameData.physics.xAfterburnerCharge);
//save last was super information
cf.Write (bLastPrimaryWasSuper, sizeof (bLastPrimaryWasSuper), 1);
cf.Write (bLastSecondaryWasSuper, sizeof (bLastSecondaryWasSuper), 1);
//	Save flash effect stuff
cf.WriteFix (gameData.render.xFlashEffect);
cf.WriteFix (gameData.render.xTimeFlashLastPlayed);
cf.WriteShort (gameStates.ogl.palAdd.red);
cf.WriteShort (gameStates.ogl.palAdd.green);
cf.WriteShort (gameStates.ogl.palAdd.blue);
cf.Write (gameData.render.lights.subtracted, sizeof (gameData.render.lights.subtracted [0]), MAX_SEGMENTS);
cf.WriteInt (gameStates.app.bFirstSecretVisit);
cf.WriteFix (gameData.omega.xCharge [0]);
cf.WriteShort (gameData.missions.nEnteredFromLevel);
for (i = 0; i < MAX_PLAYERS; i++)
	StateSaveSpawnPoint (i, cf);
}

//------------------------------------------------------------------------------

int StateSaveAllSub (const char *filename, const char *szDesc, int bBetweenLevels)
{
	int			i;
	CFile			cf;
	gsrCanvas	*cnv;

Assert (bBetweenLevels == 0);	//between levels save ripped out
StopTime ();
if (!cf.Open (filename, gameFolders.szSaveDir, "wb", 0)) {
	if (!IsMultiGame)
		ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_SAVE_ERROR2);
	StartTime (1);
	return 0;
	}

//Save id
cf.Write (dgss_id, sizeof (char) * 4, 1);
//Save sgVersion
i = STATE_VERSION;
cf.Write (&i, sizeof (int), 1);
//Save description
cf.Write (szDesc, sizeof (char) * DESC_LENGTH, 1);
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
	cf.Write (cnv->cvBitmap.bmTexBuf, THUMBNAIL_LW * THUMBNAIL_LH, 1);
	GrSetCurrentCanvas (cnv_save);
	GrFreeCanvas (cnv);
	cf.Write (gamePalette, 3, 256);
	}
else	{
 	ubyte color = 0;
 	for (i = 0; i < THUMBNAIL_LW * THUMBNAIL_LH; i++)
		cf.Write (&color, sizeof (ubyte), 1);
	}
StateSaveUniGameData (cf, bBetweenLevels);
if (cf.Error ()) {
	if (!IsMultiGame) {
		ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_SAVE_ERROR);
		cf.Close ();
		cf.Delete (filename, gameFolders.szSaveDir);
		}
	}
else 
	cf.Close ();
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
	CFile	cf;

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
	if (!cf.Exist (filename, gameFolders.szSaveDir, 0))
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
			if (cf.Exist (temp_fname, gameFolders.szSaveDir, 0)) {
				rval = cf.Copy (temp_fname, SECRETC_FILENAME);
				Assert (rval == 0);	//	Oops, error copying temp_fname to secret.sgc!
				}
			else
				cf.Delete (SECRETC_FILENAME, gameFolders.szSaveDir);
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

int StateReadBoundedInt (int nMax, int *nVal, CFile& cf)
{
	int	i;

i = cf.ReadInt ();
if ((i < 0) || (i > nMax)) {
	Warning (TXT_SAVE_CORRUPT);
	//cf.Close ();
	return 1;
	}
*nVal = i;
return 0;
}

//------------------------------------------------------------------------------

int StateLoadMission (CFile& cf)
{
	char	szMission [16];
	int	i, nVersionFilter = gameOpts->app.nVersionFilter;

cf.Read (szMission, sizeof (char), 9);
szMission [9] = '\0';
gameOpts->app.nVersionFilter = 3;
i = LoadMissionByName (szMission, -1);
gameOpts->app.nVersionFilter = nVersionFilter;
if (i)
	return 1;
ExecMessageBox (NULL, NULL, 1, "Ok", TXT_MSN_LOAD_ERROR, szMission);
//cf.Close ();
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

void StateRestoreNetGame (CFile& cf)
{
	int	i, j;

netGame.nType = cf.ReadByte ();
netGame.nSecurity = cf.ReadInt ();
cf.Read (netGame.szGameName, 1, NETGAME_NAME_LEN + 1);
cf.Read (netGame.szMissionTitle, 1, MISSION_NAME_LEN + 1);
cf.Read (netGame.szMissionName, 1, 9);
netGame.nLevel = cf.ReadInt ();
netGame.gameMode = (ubyte) cf.ReadByte ();
netGame.bRefusePlayers = (ubyte) cf.ReadByte ();
netGame.difficulty = (ubyte) cf.ReadByte ();
netGame.gameStatus = (ubyte) cf.ReadByte ();
netGame.nNumPlayers = (ubyte) cf.ReadByte ();
netGame.nMaxPlayers = (ubyte) cf.ReadByte ();
netGame.nConnected = (ubyte) cf.ReadByte ();
netGame.gameFlags = (ubyte) cf.ReadByte ();
netGame.protocolVersion = (ubyte) cf.ReadByte ();
netGame.versionMajor = (ubyte) cf.ReadByte ();
netGame.versionMinor = (ubyte) cf.ReadByte ();
netGame.teamVector = (ubyte) cf.ReadByte ();
netGame.DoMegas = (ubyte) cf.ReadByte ();
netGame.DoSmarts = (ubyte) cf.ReadByte ();
netGame.DoFusions = (ubyte) cf.ReadByte ();
netGame.DoHelix = (ubyte) cf.ReadByte ();
netGame.DoPhoenix = (ubyte) cf.ReadByte ();
netGame.DoAfterburner = (ubyte) cf.ReadByte ();
netGame.DoInvulnerability = (ubyte) cf.ReadByte ();
netGame.DoCloak = (ubyte) cf.ReadByte ();
netGame.DoGauss = (ubyte) cf.ReadByte ();
netGame.DoVulcan = (ubyte) cf.ReadByte ();
netGame.DoPlasma = (ubyte) cf.ReadByte ();
netGame.DoOmega = (ubyte) cf.ReadByte ();
netGame.DoSuperLaser = (ubyte) cf.ReadByte ();
netGame.DoProximity = (ubyte) cf.ReadByte ();
netGame.DoSpread = (ubyte) cf.ReadByte ();
netGame.DoSmartMine = (ubyte) cf.ReadByte ();
netGame.DoFlash = (ubyte) cf.ReadByte ();
netGame.DoGuided = (ubyte) cf.ReadByte ();
netGame.DoEarthShaker = (ubyte) cf.ReadByte ();
netGame.DoMercury = (ubyte) cf.ReadByte ();
netGame.bAllowMarkerView = (ubyte) cf.ReadByte ();
netGame.bIndestructibleLights = (ubyte) cf.ReadByte ();
netGame.DoAmmoRack = (ubyte) cf.ReadByte ();
netGame.DoConverter = (ubyte) cf.ReadByte ();
netGame.DoHeadlight = (ubyte) cf.ReadByte ();
netGame.DoHoming = (ubyte) cf.ReadByte ();
netGame.DoLaserUpgrade = (ubyte) cf.ReadByte ();
netGame.DoQuadLasers = (ubyte) cf.ReadByte ();
netGame.bShowAllNames = (ubyte) cf.ReadByte ();
netGame.BrightPlayers = (ubyte) cf.ReadByte ();
netGame.invul = (ubyte) cf.ReadByte ();
netGame.FriendlyFireOff = (ubyte) cf.ReadByte ();
for (i = 0; i < 2; i++)
	cf.Read (netGame.szTeamName [i], 1, CALLSIGN_LEN + 1);		// 18 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	netGame.locations [i] = cf.ReadInt ();
for (i = 0; i < MAX_PLAYERS; i++)
	for (j = 0; j < MAX_PLAYERS; j++)
		netGame.kills [i][j] = cf.ReadShort ();			// 128 bytes
netGame.nSegmentCheckSum = cf.ReadShort ();				// 2 bytes
for (i = 0; i < 2; i++)
	netGame.teamKills [i] = cf.ReadShort ();				// 4 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	netGame.killed [i] = cf.ReadShort ();					// 16 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	netGame.playerKills [i] = cf.ReadShort ();			// 16 bytes
netGame.KillGoal = cf.ReadInt ();							// 4 bytes
netGame.xPlayTimeAllowed = cf.ReadFix ();					// 4 bytes
netGame.xLevelTime = cf.ReadFix ();							// 4 bytes
netGame.controlInvulTime = cf.ReadInt ();				// 4 bytes
netGame.monitorVector = cf.ReadInt ();					// 4 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	netGame.playerScore [i] = cf.ReadInt ();				// 32 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	netGame.playerFlags [i] = (ubyte) cf.ReadByte ();	// 8 bytes
netGame.nPacketsPerSec = cf.ReadShort ();					// 2 bytes
netGame.bShortPackets = (ubyte) cf.ReadByte ();			// 1 bytes
// 279 bytes
// 355 bytes total
cf.Read (netGame.AuxData, NETGAME_AUX_SIZE, 1);  // Storage for protocol-specific data (e.g., multicast session and port)
}

//------------------------------------------------------------------------------

void StateRestoreNetPlayers (CFile& cf)
{
	int	i;

netPlayers.nType = (ubyte) cf.ReadByte ();
netPlayers.nSecurity = cf.ReadInt ();
for (i = 0; i < MAX_PLAYERS + 4; i++) {
	cf.Read (netPlayers.players [i].callsign, 1, CALLSIGN_LEN + 1);
	cf.Read (netPlayers.players [i].network.ipx.server, 1, 4);
	cf.Read (netPlayers.players [i].network.ipx.node, 1, 6);
	netPlayers.players [i].versionMajor = (ubyte) cf.ReadByte ();
	netPlayers.players [i].versionMinor = (ubyte) cf.ReadByte ();
	netPlayers.players [i].computerType = (enum compType) cf.ReadByte ();
	netPlayers.players [i].connected = cf.ReadByte ();
	netPlayers.players [i].socket = (ushort) cf.ReadShort ();
	netPlayers.players [i].rank = (ubyte) cf.ReadByte ();
	}
}

//------------------------------------------------------------------------------

void StateRestorePlayer (tPlayer *playerP, CFile& cf)
{
	int	i;

cf.Read (playerP->callsign, 1, CALLSIGN_LEN + 1); // The callsign of this tPlayer, for net purposes.
cf.Read (playerP->netAddress, 1, 6);					// The network address of the player.
playerP->connected = cf.ReadByte ();            // Is the tPlayer connected or not?
playerP->nObject = cf.ReadInt ();                // What tObject number this tPlayer is. (made an int by mk because it's very often referenced)
playerP->nPacketsGot = cf.ReadInt ();         // How many packets we got from them
playerP->nPacketsSent = cf.ReadInt ();        // How many packets we sent to them
playerP->flags = (uint) cf.ReadInt ();           // Powerup flags, see below...
playerP->energy = cf.ReadFix ();                // Amount of energy remaining.
playerP->shields = cf.ReadFix ();               // shields remaining (protection)
playerP->lives = cf.ReadByte ();                // Lives remaining, 0 = game over.
playerP->level = cf.ReadByte ();                // Current level tPlayer is playing. (must be signed for secret levels)
playerP->laserLevel = (ubyte) cf.ReadByte ();  // Current level of the laser.
playerP->startingLevel = cf.ReadByte ();       // What level the tPlayer started on.
playerP->nKillerObj = cf.ReadShort ();       // Who killed me.... (-1 if no one)
playerP->primaryWeaponFlags = (ushort) cf.ReadShort ();   // bit set indicates the tPlayer has this weapon.
playerP->secondaryWeaponFlags = (ushort) cf.ReadShort (); // bit set indicates the tPlayer has this weapon.
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	playerP->primaryAmmo [i] = (ushort) cf.ReadShort (); // How much ammo of each nType.
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
	playerP->secondaryAmmo [i] = (ushort) cf.ReadShort (); // How much ammo of each nType.
#if 1 //for inventory system
playerP->nInvuls = (ubyte) cf.ReadByte ();
playerP->nCloaks = (ubyte) cf.ReadByte ();
#endif
playerP->lastScore = cf.ReadInt ();           // Score at beginning of current level.
playerP->score = cf.ReadInt ();                // Current score.
playerP->timeLevel = cf.ReadFix ();            // Level time played
playerP->timeTotal = cf.ReadFix ();				// Game time played (high word = seconds)
playerP->cloakTime = cf.ReadFix ();					// Time cloaked
if (playerP->cloakTime != 0x7fffffff)
	playerP->cloakTime += gameData.time.xGame;
playerP->invulnerableTime = cf.ReadFix ();      // Time invulnerable
if (playerP->invulnerableTime != 0x7fffffff)
	playerP->invulnerableTime += gameData.time.xGame;
playerP->nKillGoalCount = cf.ReadShort ();          // Num of players killed this level
playerP->netKilledTotal = cf.ReadShort ();       // Number of times killed total
playerP->netKillsTotal = cf.ReadShort ();        // Number of net kills total
playerP->numKillsLevel = cf.ReadShort ();        // Number of kills this level
playerP->numKillsTotal = cf.ReadShort ();        // Number of kills total
playerP->numRobotsLevel = cf.ReadShort ();       // Number of initial robots this level
playerP->numRobotsTotal = cf.ReadShort ();       // Number of robots total
playerP->hostages.nRescued = (ushort) cf.ReadShort (); // Total number of hostages rescued.
playerP->hostages.nTotal = (ushort) cf.ReadShort ();         // Total number of hostages.
playerP->hostages.nOnBoard = (ubyte) cf.ReadByte ();      // Number of hostages on ship.
playerP->hostages.nLevel = (ubyte) cf.ReadByte ();         // Number of hostages on this level.
playerP->homingObjectDist = cf.ReadFix ();     // Distance of nearest homing tObject.
playerP->hoursLevel = cf.ReadByte ();            // Hours played (since timeTotal can only go up to 9 hours)
playerP->hoursTotal = cf.ReadByte ();            // Hours played (since timeTotal can only go up to 9 hours)
}

//------------------------------------------------------------------------------

void StateRestoreObject (tObject *objP, CFile& cf, int sgVersion)
{
objP->info.nSignature = cf.ReadInt ();      
objP->info.nType = (ubyte) cf.ReadByte (); 
#if DBG
if (objP->info.nType == OBJ_REACTOR)
	objP->info.nType = objP->info.nType;
else 
#endif
if ((sgVersion < 32) && IS_BOSS (objP))
	gameData.boss [(int) extraGameInfo [0].nBossCount++].nObject = OBJ_IDX (objP);
objP->info.nId = (ubyte) cf.ReadByte ();
objP->info.nNextInSeg = cf.ReadShort ();
objP->info.nPrevInSeg = cf.ReadShort ();
objP->info.controlType = (ubyte) cf.ReadByte ();
objP->info.movementType = (ubyte) cf.ReadByte ();
objP->info.renderType = (ubyte) cf.ReadByte ();
objP->info.nFlags = (ubyte) cf.ReadByte ();
objP->info.nSegment = cf.ReadShort ();
objP->info.nAttachedObj = cf.ReadShort ();
cf.ReadVector (objP->info.position.vPos);     
cf.ReadMatrix (objP->info.position.mOrient);  
objP->info.xSize = cf.ReadFix (); 
objP->info.xShields = cf.ReadFix ();
cf.ReadVector (objP->info.vLastPos);  
objP->info.contains.nType = cf.ReadByte (); 
objP->info.contains.nId = cf.ReadByte ();   
objP->info.contains.nCount = cf.ReadByte ();
objP->info.nCreator = cf.ReadByte ();
objP->info.xLifeLeft = cf.ReadFix ();   
if (objP->info.movementType == MT_PHYSICS) {
	cf.ReadVector (objP->mType.physInfo.velocity);   
	cf.ReadVector (objP->mType.physInfo.thrust);     
	objP->mType.physInfo.mass = cf.ReadFix ();       
	objP->mType.physInfo.drag = cf.ReadFix ();       
	objP->mType.physInfo.brakes = cf.ReadFix ();     
	cf.ReadVector (objP->mType.physInfo.rotVel);     
	cf.ReadVector (objP->mType.physInfo.rotThrust);  
	objP->mType.physInfo.turnRoll = cf.ReadFixAng ();   
	objP->mType.physInfo.flags = (ushort) cf.ReadShort ();      
	}
else if (objP->info.movementType == MT_SPINNING) {
	cf.ReadVector (objP->mType.spinRate);  
	}
switch (objP->info.controlType) {
	case CT_WEAPON:
		objP->cType.laserInfo.parent.nType = cf.ReadShort ();
		objP->cType.laserInfo.parent.nObject = cf.ReadShort ();
		objP->cType.laserInfo.parent.nSignature = cf.ReadInt ();
		objP->cType.laserInfo.xCreationTime = cf.ReadFix ();
		objP->cType.laserInfo.nLastHitObj = cf.ReadShort ();
		if (objP->cType.laserInfo.nLastHitObj < 0)
			objP->cType.laserInfo.nLastHitObj = 0;
		else {
			gameData.objs.nHitObjects [OBJ_IDX (objP) * MAX_HIT_OBJECTS] = objP->cType.laserInfo.nLastHitObj;
			objP->cType.laserInfo.nLastHitObj = 1;
			}
		objP->cType.laserInfo.nHomingTarget = cf.ReadShort ();
		objP->cType.laserInfo.xScale = cf.ReadFix ();
		break;

	case CT_EXPLOSION:
		objP->cType.explInfo.nSpawnTime = cf.ReadFix ();
		objP->cType.explInfo.nDeleteTime = cf.ReadFix ();
		objP->cType.explInfo.nDeleteObj = cf.ReadShort ();
		objP->cType.explInfo.attached.nParent = cf.ReadShort ();
		objP->cType.explInfo.attached.nPrev = cf.ReadShort ();
		objP->cType.explInfo.attached.nNext = cf.ReadShort ();
		break;

	case CT_AI:
		objP->cType.aiInfo.behavior = (ubyte) cf.ReadByte ();
		cf.Read (objP->cType.aiInfo.flags, 1, MAX_AI_FLAGS);
		objP->cType.aiInfo.nHideSegment = cf.ReadShort ();
		objP->cType.aiInfo.nHideIndex = cf.ReadShort ();
		objP->cType.aiInfo.nPathLength = cf.ReadShort ();
		objP->cType.aiInfo.nCurPathIndex = cf.ReadByte ();
		objP->cType.aiInfo.bDyingSoundPlaying = cf.ReadByte ();
		objP->cType.aiInfo.nDangerLaser = cf.ReadShort ();
		objP->cType.aiInfo.nDangerLaserSig = cf.ReadInt ();
		objP->cType.aiInfo.xDyingStartTime = cf.ReadFix ();
		break;

	case CT_LIGHT:
		objP->cType.lightInfo.intensity = cf.ReadFix ();
		break;

	case CT_POWERUP:
		objP->cType.powerupInfo.nCount = cf.ReadInt ();
		objP->cType.powerupInfo.xCreationTime = cf.ReadFix ();
		objP->cType.powerupInfo.nFlags = cf.ReadInt ();
		break;
	}
switch (objP->info.renderType) {
	case RT_MORPH:
	case RT_POLYOBJ: {
		int i;
		objP->rType.polyObjInfo.nModel = cf.ReadInt ();
		for (i = 0; i < MAX_SUBMODELS; i++)
			cf.ReadAngVec (objP->rType.polyObjInfo.animAngles [i]);
		objP->rType.polyObjInfo.nSubObjFlags = cf.ReadInt ();
		objP->rType.polyObjInfo.nTexOverride = cf.ReadInt ();
		objP->rType.polyObjInfo.nAltTextures = cf.ReadInt ();
		break;
		}
	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
	case RT_THRUSTER:
		objP->rType.vClipInfo.nClipIndex = cf.ReadInt ();
		objP->rType.vClipInfo.xFrameTime = cf.ReadFix ();
		objP->rType.vClipInfo.nCurFrame = cf.ReadByte ();
		break;

	case RT_LASER:
		break;
	}
}

//------------------------------------------------------------------------------

void StateRestoreWall (tWall *wallP, CFile& cf)
{
wallP->nSegment = cf.ReadInt ();
wallP->nSide = cf.ReadInt ();
wallP->hps = cf.ReadFix ();    
wallP->nLinkedWall = cf.ReadInt ();
wallP->nType = (ubyte) cf.ReadByte ();       
wallP->flags = (ubyte) cf.ReadByte ();      
wallP->state = (ubyte) cf.ReadByte ();      
wallP->nTrigger = (ubyte) cf.ReadByte ();    
wallP->nClip = cf.ReadByte ();   
wallP->keys = (ubyte) cf.ReadByte ();       
wallP->controllingTrigger = cf.ReadByte ();
wallP->cloakValue = cf.ReadByte (); 
}

//------------------------------------------------------------------------------

void StateRestoreExplWall (tExplWall *wallP, CFile& cf)
{
wallP->nSegment = cf.ReadInt ();
wallP->nSide = cf.ReadInt ();
wallP->time = cf.ReadFix ();    
}

//------------------------------------------------------------------------------

void StateRestoreCloakingWall (tCloakingWall *wallP, CFile& cf)
{
	int	i;

wallP->nFrontWall = cf.ReadShort ();
wallP->nBackWall = cf.ReadShort (); 
for (i = 0; i < 4; i++) {
	wallP->front_ls [i] = cf.ReadFix (); 
	wallP->back_ls [i] = cf.ReadFix ();
	}
wallP->time = cf.ReadFix ();    
}

//------------------------------------------------------------------------------

void StateRestoreActiveDoor (tActiveDoor *doorP, CFile& cf)
{
	int	i;

doorP->nPartCount = cf.ReadInt ();
for (i = 0; i < 2; i++) {
	doorP->nFrontWall [i] = cf.ReadShort ();
	doorP->nBackWall [i] = cf.ReadShort ();
	}
doorP->time = cf.ReadFix ();    
}

//------------------------------------------------------------------------------

void StateRestoreTrigger (tTrigger *triggerP, CFile& cf)
{
	int	i;

i = cf.Tell ();
triggerP->nType = (ubyte) cf.ReadByte (); 
triggerP->flags = (ubyte) cf.ReadByte (); 
triggerP->nLinks = cf.ReadByte ();
triggerP->value = cf.ReadFix ();
triggerP->time = cf.ReadFix ();
for (i = 0; i < MAX_TRIGGER_TARGETS; i++) {
	triggerP->nSegment [i] = cf.ReadShort ();
	triggerP->nSide [i] = cf.ReadShort ();
	}
}

//------------------------------------------------------------------------------

void StateRestoreObjTriggerRef (tObjTriggerRef *refP, CFile& cf)
{
refP->prev = cf.ReadShort ();
refP->next = cf.ReadShort ();
refP->nObject = cf.ReadShort ();
}

//------------------------------------------------------------------------------

void StateRestoreMatCen (tMatCenInfo *matcenP, CFile& cf)
{
	int	i;

for (i = 0; i < 2; i++)
	matcenP->objFlags [i] = cf.ReadInt ();
matcenP->xHitPoints = cf.ReadFix ();
matcenP->xInterval = cf.ReadFix ();
matcenP->nSegment = cf.ReadShort ();
matcenP->nFuelCen = cf.ReadShort ();
}

//------------------------------------------------------------------------------

void StateRestoreFuelCen (tFuelCenInfo *fuelcenP, CFile& cf)
{
fuelcenP->nType = cf.ReadInt ();
fuelcenP->nSegment = cf.ReadInt ();
fuelcenP->bFlag = cf.ReadByte ();
fuelcenP->bEnabled = cf.ReadByte ();
fuelcenP->nLives = cf.ReadByte ();
fuelcenP->xCapacity = cf.ReadFix ();
fuelcenP->xMaxCapacity = cf.ReadFix ();
fuelcenP->xTimer = cf.ReadFix ();
fuelcenP->xDisableTime = cf.ReadFix ();
cf.ReadVector (fuelcenP->vCenter);
}

//------------------------------------------------------------------------------

void StateRestoreReactorTrigger (tReactorTriggers *triggerP, CFile& cf)
{
	int	i;

triggerP->nLinks = cf.ReadShort ();
for (i = 0; i < MAX_CONTROLCEN_LINKS; i++) {
	triggerP->nSegment [i] = cf.ReadShort ();
	triggerP->nSide [i] = cf.ReadShort ();
	}
}

//------------------------------------------------------------------------------

int StateRestoreSpawnPoint (int i, CFile& cf)
{
DBG (i = cf.Tell ());
cf.ReadVector (gameData.multiplayer.playerInit [i].position.vPos);     
cf.ReadMatrix (gameData.multiplayer.playerInit [i].position.mOrient);  
gameData.multiplayer.playerInit [i].nSegment = cf.ReadShort ();
gameData.multiplayer.playerInit [i].nSegType = cf.ReadShort ();
return (gameData.multiplayer.playerInit [i].nSegment >= 0) &&
		 (gameData.multiplayer.playerInit [i].nSegment < gameData.segs.nSegments) &&
		 (gameData.multiplayer.playerInit [i].nSegment ==
		  FindSegByPos (gameData.multiplayer.playerInit [i].position.vPos, 
							 gameData.multiplayer.playerInit [i].nSegment, 1, 0));

}

//------------------------------------------------------------------------------

int StateRestoreUniGameData (CFile& cf, int sgVersion, int bMulti, int bSecretRestore, fix xOldGameTime, int *nLevel)
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
	h = cf.ReadInt ();
	if (h != gameData.segs.nMaxSegments) {
		Warning (TXT_MAX_SEGS_WARNING, h);
		return 0;
		}
	}
bBetweenLevels = cf.ReadInt ();
Assert (bBetweenLevels == 0);	//between levels save ripped out
// Read the mission info...
if (!StateLoadMission (cf))
	return 0;
//Read level info
nCurrentLevel = cf.ReadInt ();
nNextLevel = cf.ReadInt ();
//Restore gameData.time.xGame
gameData.time.xGame = cf.ReadFix ();
// Start new game....
StateRestoreMultiGame (szOrgCallSign, bMulti, bSecretRestore);
if (IsMultiGame) {
		char szServerCallSign [CALLSIGN_LEN + 1];

	strcpy (szServerCallSign, netPlayers.players [0].callsign);
	gameData.app.nStateGameId = cf.ReadInt ();
	StateRestoreNetGame (cf);
	DBG (fPos = cf.Tell ());
	StateRestoreNetPlayers (cf);
	DBG (fPos = cf.Tell ());
	nPlayers = cf.ReadInt ();
	nSavedLocalPlayer = gameData.multiplayer.nLocalPlayer;
	gameData.multiplayer.nLocalPlayer = cf.ReadInt ();
	for (i = 0; i < nPlayers; i++) {
		StateRestorePlayer (restoredPlayers + i, cf);
		restoredPlayers [i].connected = 0;
		}
	DBG (fPos = cf.Tell ());
	// make sure the current game host is in tPlayer slot #0
	nServerPlayer = StateSetServerPlayer (restoredPlayers, nPlayers, szServerCallSign, &nOtherObjNum, &nServerObjNum);
	StateGetConnectedPlayers (restoredPlayers, nPlayers);
	}
//Read tPlayer info
if (!StartNewLevelSub (nCurrentLevel, 1, bSecretRestore, 1)) {
	cf.Close ();
	return 0;
	}
nLocalObjNum = LOCALPLAYER.nObject;
if (bSecretRestore != 1)	//either no secret restore, or tPlayer died in scret level
	StateRestorePlayer (gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer, cf);
else {
	tPlayer	retPlayer;
	StateRestorePlayer (&retPlayer, cf);
	StateAwardReturningPlayer (&retPlayer, xOldGameTime);
	}
LOCALPLAYER.nObject = nLocalObjNum;
strcpy (LOCALPLAYER.callsign, szOrgCallSign);
// Set the right level
if (bBetweenLevels)
	LOCALPLAYER.level = nNextLevel;
// Restore the weapon states
gameData.weapons.nPrimary = cf.ReadByte ();
gameData.weapons.nSecondary = cf.ReadByte ();
SelectWeapon (gameData.weapons.nPrimary, 0, 0, 0);
SelectWeapon (gameData.weapons.nSecondary, 1, 0, 0);
// Restore the difficulty level
gameStates.app.nDifficultyLevel = cf.ReadInt ();
// Restore the cheats enabled flag
gameStates.app.cheats.bEnabled = cf.ReadInt ();
for (i = 0; i < 2; i++) {
	if (sgVersion < 33) {
		gameStates.gameplay.slowmo [i].fSpeed = 1;
		gameStates.gameplay.slowmo [i].nState = 0;
		}
	else {
		gameStates.gameplay.slowmo [i].fSpeed = X2F (cf.ReadInt ());
		gameStates.gameplay.slowmo [i].nState = cf.ReadInt ();
		}
	}
if (sgVersion > 33) {
	for (i = 0; i < MAX_PLAYERS; i++)
	   if (i != gameData.multiplayer.nLocalPlayer)
		   gameData.multiplayer.weaponStates [i].bTripleFusion = cf.ReadInt ();
   	else {
   	   gameData.weapons.bTripleFusion = cf.ReadInt ();
		   gameData.multiplayer.weaponStates [i].bTripleFusion = !gameData.weapons.bTripleFusion;  //force MultiSendWeapons
		   }
	}
if (!bBetweenLevels)	{
	gameStates.render.bDoAppearanceEffect = 0;			// Don't do this for middle o' game stuff.
	//Clear out all the objects from the lvl file
	ResetSegObjLists ();
	ResetObjects (1);

	//Read objects, and pop 'em into their respective segments.
	DBG (fPos = cf.Tell ());
	h = cf.ReadInt ();
	gameData.objs.nLastObject [0] = h - 1;
	extraGameInfo [0].nBossCount = 0;
	for (i = 0; i < h; i++)
		StateRestoreObject (OBJECTS + i, cf, sgVersion);
	DBG (fPos = cf.Tell ());
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
	if (StateReadBoundedInt (MAX_WALLS, &gameData.walls.nWalls, cf))
		return 0;
	for (i = 0, wallP = gameData.walls.walls; i < gameData.walls.nWalls; i++, wallP++) {
		StateRestoreWall (wallP, cf);
		if (wallP->nType == WALL_OPEN)
			DigiKillSoundLinkedToSegment ((short) wallP->nSegment, (short) wallP->nSide, -1);	//-1 means kill any sound
		}
	DBG (fPos = cf.Tell ());
	//Restore exploding wall info
	if (StateReadBoundedInt (MAX_EXPLODING_WALLS, &h, cf))
		return 0;
	for (i = 0; i < h; i++)
		StateRestoreExplWall (gameData.walls.explWalls + i, cf);
	DBG (fPos = cf.Tell ());
	//Restore door info
	if (StateReadBoundedInt (MAX_DOORS, &gameData.walls.nOpenDoors, cf))
		return 0;
	for (i = 0; i < gameData.walls.nOpenDoors; i++)
		StateRestoreActiveDoor (gameData.walls.activeDoors + i, cf);
	DBG (fPos = cf.Tell ());
	if (StateReadBoundedInt (MAX_WALLS, &gameData.walls.nCloaking, cf))
		return 0;
	for (i = 0; i < gameData.walls.nCloaking; i++)
		StateRestoreCloakingWall (gameData.walls.cloaking + i, cf);
	DBG (fPos = cf.Tell ());
	//Restore tTrigger info
	if (StateReadBoundedInt (MAX_TRIGGERS, &gameData.trigs.nTriggers, cf))
		return 0;
	for (i = 0; i < gameData.trigs.nTriggers; i++)
		StateRestoreTrigger (gameData.trigs.triggers + i, cf);
	DBG (fPos = cf.Tell ());
	//Restore tObject tTrigger info
	if (StateReadBoundedInt (MAX_TRIGGERS, &gameData.trigs.nObjTriggers, cf))
		return 0;
	if (gameData.trigs.nObjTriggers > 0) {
		for (i = 0; i < gameData.trigs.nObjTriggers; i++)
			StateRestoreTrigger (gameData.trigs.objTriggers + i, cf);
		for (i = 0; i < gameData.trigs.nObjTriggers; i++)
			StateRestoreObjTriggerRef (gameData.trigs.objTriggerRefs + i, cf);
		if (sgVersion < 36) {
			j = (sgVersion < 35) ? 700 : MAX_OBJECTS_D2X;
			for (i = 0; i < j; i++)
				gameData.trigs.firstObjTrigger [i] = cf.ReadShort ();
			}
		else {
			memset (gameData.trigs.firstObjTrigger, 0xff, sizeof (short) * MAX_OBJECTS_D2X);
			for (i = cf.ReadShort (); i; i--) {
				j = cf.ReadShort ();
				gameData.trigs.firstObjTrigger [j] = cf.ReadShort ();
				}
			}
		}
	else if (sgVersion < 36)
		cf.Seek (((sgVersion < 35) ? 700 : MAX_OBJECTS_D2X) * sizeof (short), SEEK_CUR);
	else
		cf.ReadShort ();
	DBG (fPos = cf.Tell ());
	//Restore tmap info
	for (i = 0; i <= gameData.segs.nLastSegment; i++)	{
		for (j = 0; j < 6; j++)	{
			gameData.segs.segments [i].sides [j].nWall = cf.ReadShort ();
			gameData.segs.segments [i].sides [j].nBaseTex = cf.ReadShort ();
			nTexture = cf.ReadShort ();
			gameData.segs.segments [i].sides [j].nOvlTex = nTexture & 0x3fff;
			gameData.segs.segments [i].sides [j].nOvlOrient = (nTexture >> 14) & 3;
			}
		}
	DBG (fPos = cf.Tell ());
	//Restore the fuelcen info
	for (i = 0, wallP = gameData.walls.walls; i < gameData.walls.nWalls; i++, wallP++) {
		if ((wallP->nType == WALL_DOOR) && (wallP->flags & WALL_DOOR_OPENED))
			AnimateOpeningDoor (SEGMENTS + wallP->nSegment, wallP->nSide, -1);
		else if ((wallP->nType == WALL_BLASTABLE) && (wallP->flags & WALL_BLASTED))
			BlastBlastableWall (SEGMENTS + wallP->nSegment, wallP->nSide);
		}
	gameData.reactor.bDestroyed = cf.ReadInt ();
	gameData.reactor.countdown.nTimer = cf.ReadFix ();
	DBG (fPos = cf.Tell ());
	if (StateReadBoundedInt (MAX_ROBOT_CENTERS, &gameData.matCens.nBotCenters, cf))
		return 0;
	for (i = 0; i < gameData.matCens.nBotCenters; i++)
		StateRestoreMatCen (gameData.matCens.botGens + i, cf);
	if (sgVersion >= 30) {
		if (StateReadBoundedInt (MAX_EQUIP_CENTERS, &gameData.matCens.nEquipCenters, cf))
			return 0;
		for (i = 0; i < gameData.matCens.nEquipCenters; i++)
			StateRestoreMatCen (gameData.matCens.equipGens + i, cf);
		}
	else {
		gameData.matCens.nBotCenters = 0;
		memset (gameData.matCens.botGens, 0, sizeof (gameData.matCens.botGens));
		}
	StateRestoreReactorTrigger (&gameData.reactor.triggers, cf);
	if (StateReadBoundedInt (MAX_FUEL_CENTERS, &gameData.matCens.nFuelCenters, cf))
		return 0;
	for (i = 0; i < gameData.matCens.nFuelCenters; i++)
		StateRestoreFuelCen (gameData.matCens.fuelCenters + i, cf);
	DBG (fPos = cf.Tell ());
	// Restore the control cen info
	if (sgVersion < 31) {
		gameData.reactor.states [0].bHit = cf.ReadInt ();
		gameData.reactor.states [0].bSeenPlayer = cf.ReadInt ();
		gameData.reactor.states [0].nNextFireTime = cf.ReadInt ();
		gameData.reactor.bPresent = cf.ReadInt ();
		gameData.reactor.states [0].nDeadObj = cf.ReadInt ();
		}
	else {
		int	i;

		gameData.reactor.bPresent = cf.ReadInt ();
		for (i = 0; i < MAX_BOSS_COUNT; i++) {
			gameData.reactor.states [i].nObject = cf.ReadInt ();
			gameData.reactor.states [i].bHit = cf.ReadInt ();
			gameData.reactor.states [i].bSeenPlayer = cf.ReadInt ();
			gameData.reactor.states [i].nNextFireTime = cf.ReadInt ();
			gameData.reactor.states [i].nDeadObj = cf.ReadInt ();
			}
		}
	DBG (fPos = cf.Tell ());
	// Restore the AI state
	AIRestoreUniState (cf, sgVersion);
	// Restore the automap visited info
	DBG (fPos = cf.Tell ());
	StateFixObjects ();
	SpecialResetObjects ();
	if (sgVersion > 37) {
		for (i = 0; i < MAX_SEGMENTS; i++)
			gameData.render.mine.bAutomapVisited [i] = (ushort) cf.ReadShort ();
		}
	else {
		int	i, j = (sgVersion > 22) ? MAX_SEGMENTS : MAX_SEGMENTS_D2;
		for (i = 0; i < j; i++)
			gameData.render.mine.bAutomapVisited [i] = (ushort) cf.ReadByte ();
		}
	DBG (fPos = cf.Tell ());
	//	Restore hacked up weapon system stuff.
	gameData.fusion.xNextSoundTime = gameData.time.xGame;
	gameData.fusion.xAutoFireTime = 0;
	gameData.laser.xNextFireTime = 
	gameData.missiles.xNextFireTime = gameData.time.xGame;
	gameData.laser.xLastFiredTime = 
	gameData.missiles.xLastFiredTime = gameData.time.xGame;
	}
gameData.app.nStateGameId = 0;
gameData.app.nStateGameId = (uint) cf.ReadInt ();
gameStates.app.cheats.bLaserRapidFire = cf.ReadInt ();
gameStates.app.bLunacy = cf.ReadInt ();		//	Yes, reading this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
gameStates.app.bLunacy = cf.ReadInt ();
if (gameStates.app.bLunacy)
	DoLunacyOn ();

DBG (fPos = cf.Tell ());
for (i = 0; i < NUM_MARKERS; i++)
	gameData.marker.objects [i] = cf.ReadShort ();
cf.Read (gameData.marker.nOwner, sizeof (gameData.marker.nOwner), 1);
cf.Read (gameData.marker.szMessage, sizeof (gameData.marker.szMessage), 1);

if (bSecretRestore != 1)
	gameData.physics.xAfterburnerCharge = cf.ReadFix ();
else {
	cf.ReadFix ();
	}
//read last was super information
cf.Read (&bLastPrimaryWasSuper, sizeof (bLastPrimaryWasSuper), 1);
cf.Read (&bLastSecondaryWasSuper, sizeof (bLastSecondaryWasSuper), 1);
gameData.render.xFlashEffect = cf.ReadFix ();
gameData.render.xTimeFlashLastPlayed = cf.ReadFix ();
gameStates.ogl.palAdd.red = cf.ReadShort ();
gameStates.ogl.palAdd.green = cf.ReadShort ();
gameStates.ogl.palAdd.blue = cf.ReadShort ();
cf.Read (gameData.render.lights.subtracted, 
		  sizeof (gameData.render.lights.subtracted [0]), 
		  (sgVersion > 22) ? MAX_SEGMENTS : MAX_SEGMENTS_D2);
ApplyAllChangedLight ();
gameStates.app.bFirstSecretVisit = cf.ReadInt ();
if (bSecretRestore) 
	gameStates.app.bFirstSecretVisit = 0;

if (bSecretRestore != 1)
	gameData.omega.xCharge [0] = 
	gameData.omega.xCharge [1] = cf.ReadFix ();
else
	cf.ReadFix ();
if (sgVersion > 27)
	gameData.missions.nEnteredFromLevel = cf.ReadShort ();
*nLevel = nCurrentLevel;
if (sgVersion >= 37) {
	tObjPosition playerInitSave [MAX_PLAYERS];

	memcpy (playerInitSave, gameData.multiplayer.playerInit, sizeof (playerInitSave));
	for (h = 1, i = 0; i < MAX_PLAYERS; i++)
		if (!StateRestoreSpawnPoint (i, cf))
			h = 0;
	if (!h)
		memcpy (gameData.multiplayer.playerInit, playerInitSave, sizeof (playerInitSave));
	}
return 1;
}

//------------------------------------------------------------------------------

int StateRestoreBinGameData (CFile& cf, int sgVersion, int bMulti, int bSecretRestore, fix xOldGameTime, int *nLevel)
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

cf.Read (&bBetweenLevels, sizeof (int), 1);
Assert (bBetweenLevels == 0);	//between levels save ripped out
// Read the mission info...
if (!StateLoadMission (cf))
	return 0;
//Read level info
cf.Read (&nCurrentLevel, sizeof (int), 1);
cf.Read (&nNextLevel, sizeof (int), 1);
//Restore gameData.time.xGame
cf.Read (&gameData.time.xGame, sizeof (fix), 1);
// Start new game....
StateRestoreMultiGame (szOrgCallSign, bMulti, bSecretRestore);
if (gameData.app.nGameMode & GM_MULTI) {
		char szServerCallSign [CALLSIGN_LEN + 1];

	strcpy (szServerCallSign, netPlayers.players [0].callsign);
	cf.Read (&gameData.app.nStateGameId, sizeof (int), 1);
	cf.Read (&netGame, sizeof (tNetgameInfo), 1);
	cf.Read (&netPlayers, sizeof (tAllNetPlayersInfo), 1);
	cf.Read (&nPlayers, sizeof (gameData.multiplayer.nPlayers), 1);
	cf.Read (&gameData.multiplayer.nLocalPlayer, sizeof (gameData.multiplayer.nLocalPlayer), 1);
	nSavedLocalPlayer = gameData.multiplayer.nLocalPlayer;
	for (i = 0; i < nPlayers; i++)
		cf.Read (restoredPlayers + i, sizeof (tPlayer), 1);
	nServerPlayer = StateSetServerPlayer (restoredPlayers, nPlayers, szServerCallSign, &nOtherObjNum, &nServerObjNum);
	StateGetConnectedPlayers (restoredPlayers, nPlayers);
	}

//Read tPlayer info
if (!StartNewLevelSub (nCurrentLevel, 1, bSecretRestore, 1)) {
	cf.Close ();
	return 0;
	}
nLocalObjNum = LOCALPLAYER.nObject;
if (bSecretRestore != 1)	//either no secret restore, or tPlayer died in scret level
	cf.Read (gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer, sizeof (tPlayer), 1);
else {
	tPlayer	retPlayer;
	cf.Read (&retPlayer, sizeof (tPlayer), 1);
	StateAwardReturningPlayer (&retPlayer, xOldGameTime);
	}
LOCALPLAYER.nObject = nLocalObjNum;
strcpy (LOCALPLAYER.callsign, szOrgCallSign);
// Set the right level
if (bBetweenLevels)
	LOCALPLAYER.level = nNextLevel;
// Restore the weapon states
cf.Read (&gameData.weapons.nPrimary, sizeof (sbyte), 1);
cf.Read (&gameData.weapons.nSecondary, sizeof (sbyte), 1);
SelectWeapon (gameData.weapons.nPrimary, 0, 0, 0);
SelectWeapon (gameData.weapons.nSecondary, 1, 0, 0);
// Restore the difficulty level
cf.Read (&gameStates.app.nDifficultyLevel, sizeof (int), 1);
// Restore the cheats enabled flag
cf.Read (&gameStates.app.cheats.bEnabled, sizeof (int), 1);
if (!bBetweenLevels)	{
	gameStates.render.bDoAppearanceEffect = 0;			// Don't do this for middle o' game stuff.
	//Clear out all the OBJECTS from the lvl file
	ResetSegObjLists ();
	ResetObjects (1);
	//Read objects, and pop 'em into their respective segments.
	cf.Read (&i, sizeof (int), 1);
	gameData.objs.nLastObject [0] = i - 1;
	cf.Read (OBJECTS, sizeof (tObject), i);
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
	if (StateReadBoundedInt (MAX_WALLS, &gameData.walls.nWalls, cf))
		return 0;
	cf.Read (gameData.walls.walls, sizeof (tWall), gameData.walls.nWalls);
	//now that we have the walls, check if any sounds are linked to
	//walls that are now open
	for (i = 0, wallP = gameData.walls.walls; i < gameData.walls.nWalls; i++, wallP++)
		if (wallP->nType == WALL_OPEN)
			DigiKillSoundLinkedToSegment ((short) wallP->nSegment, (short) wallP->nSide, -1);	//-1 means kill any sound
	//Restore exploding wall info
	if (sgVersion >= 10) {
		cf.Read (&i, sizeof (int), 1);
		cf.Read (gameData.walls.explWalls, sizeof (*gameData.walls.explWalls), i);
		}
	//Restore door info
	if (StateReadBoundedInt (MAX_DOORS, &gameData.walls.nOpenDoors, cf))
		return 0;
	cf.Read (gameData.walls.activeDoors, sizeof (tActiveDoor), gameData.walls.nOpenDoors);
	if (sgVersion >= 14) {		//Restore cloaking tWall info
		if (StateReadBoundedInt (MAX_WALLS, &gameData.walls.nCloaking, cf))
			return 0;
		cf.Read (gameData.walls.cloaking, sizeof (tCloakingWall), gameData.walls.nCloaking);
		}
	//Restore tTrigger info
	if (StateReadBoundedInt (MAX_TRIGGERS, &gameData.trigs.nTriggers, cf))
		return 0;
	cf.Read (gameData.trigs.triggers, sizeof (tTrigger), gameData.trigs.nTriggers);
	if (sgVersion >= 26) {
		//Restore tObject tTrigger info

		cf.Read (&gameData.trigs.nObjTriggers, sizeof (gameData.trigs.nObjTriggers), 1);
		if (gameData.trigs.nObjTriggers > 0) {
			cf.Read (gameData.trigs.objTriggers, sizeof (tTrigger), gameData.trigs.nObjTriggers);
			cf.Read (gameData.trigs.objTriggerRefs, sizeof (tObjTriggerRef), gameData.trigs.nObjTriggers);
			cf.Read (gameData.trigs.firstObjTrigger, sizeof (short), 700);
			}
		else
			cf.Seek ((sgVersion < 35) ? 700 : MAX_OBJECTS_D2X * sizeof (short), SEEK_CUR);
		}
	//Restore tmap info
	for (i = 0; i <= gameData.segs.nLastSegment; i++)	{
		for (j = 0; j < 6; j++)	{
			gameData.segs.segments [i].sides [j].nWall = cf.ReadShort ();
			gameData.segs.segments [i].sides [j].nBaseTex = cf.ReadShort ();
			nTexture = cf.ReadShort ();
			gameData.segs.segments [i].sides [j].nOvlTex = nTexture & 0x3fff;
			gameData.segs.segments [i].sides [j].nOvlOrient = (nTexture >> 14) & 3;
			}
		}
//Restore the fuelcen info
	gameData.reactor.bDestroyed = cf.ReadInt ();
	gameData.reactor.countdown.nTimer = cf.ReadFix ();
	if (StateReadBoundedInt (MAX_ROBOT_CENTERS, &gameData.matCens.nBotCenters, cf))
		return 0;
	for (i = 0; i < gameData.matCens.nBotCenters; i++) {
		cf.Read (gameData.matCens.botGens [i].objFlags, sizeof (int), 2);
		cf.Read (&gameData.matCens.botGens [i].xHitPoints, sizeof (tMatCenInfo) - ((char *) &gameData.matCens.botGens [i].xHitPoints - (char *) &gameData.matCens.botGens [i]), 1);
		}
	cf.Read (&gameData.reactor.triggers, sizeof (tReactorTriggers), 1);
	if (StateReadBoundedInt (MAX_FUEL_CENTERS, &gameData.matCens.nFuelCenters, cf))
		return 0;
	cf.Read (gameData.matCens.fuelCenters, sizeof (tFuelCenInfo), gameData.matCens.nFuelCenters);

	// Restore the control cen info
	gameData.reactor.states [0].bHit = cf.ReadInt ();
	gameData.reactor.states [0].bSeenPlayer = cf.ReadInt ();
	gameData.reactor.states [0].nNextFireTime = cf.ReadInt ();
	gameData.reactor.bPresent = cf.ReadInt ();
	gameData.reactor.states [0].nDeadObj = cf.ReadInt ();
	// Restore the AI state
	AIRestoreBinState (cf, sgVersion);
	// Restore the automap visited info
	if (sgVersion > 37)
		cf.Read (gameData.render.mine.bAutomapVisited, sizeof (ushort), MAX_SEGMENTS);
	else {
		int	i, j = (sgVersion > 22) ? MAX_SEGMENTS : MAX_SEGMENTS_D2;
		for (i = 0; i < j; i++)
			gameData.render.mine.bAutomapVisited [i] = (ushort) cf.ReadByte ();
		}
	
	cf.Read (gameData.render.mine.bAutomapVisited, sizeof (ubyte), (sgVersion > 22) ? MAX_SEGMENTS : MAX_SEGMENTS_D2);

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
	cf.Read (&gameData.app.nStateGameId, sizeof (uint), 1);
	cf.Read (&gameStates.app.cheats.bLaserRapidFire, sizeof (int), 1);
	cf.Read (&gameStates.app.bLunacy, sizeof (int), 1);		//	Yes, writing this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
	cf.Read (&gameStates.app.bLunacy, sizeof (int), 1);
	if (gameStates.app.bLunacy)
		DoLunacyOn ();
}

if (sgVersion >= 17) {
	cf.Read (gameData.marker.objects, sizeof (gameData.marker.objects), 1);
	cf.Read (gameData.marker.nOwner, sizeof (gameData.marker.nOwner), 1);
	cf.Read (gameData.marker.szMessage, sizeof (gameData.marker.szMessage), 1);
}
else {
	int num,dummy;

	// skip dummy info
	cf.Read (&num, sizeof (int), 1);       //was NumOfMarkers
	cf.Read (&dummy, sizeof (int), 1);     //was CurMarker
	cf.Seek (num * (sizeof (vmsVector) + 40), SEEK_CUR);
	for (num = 0; num < NUM_MARKERS; num++)
		gameData.marker.objects [num] = -1;
}

if (sgVersion >= 11) {
	if (bSecretRestore != 1)
		cf.Read (&gameData.physics.xAfterburnerCharge, sizeof (fix), 1);
	else {
		fix	dummy_fix;
		cf.Read (&dummy_fix, sizeof (fix), 1);
	}
}
if (sgVersion >= 12) {
	//read last was super information
	cf.Read (&bLastPrimaryWasSuper, sizeof (bLastPrimaryWasSuper), 1);
	cf.Read (&bLastSecondaryWasSuper, sizeof (bLastSecondaryWasSuper), 1);
	cf.Read (&gameData.render.xFlashEffect, sizeof (int), 1);
	cf.Read (&gameData.render.xTimeFlashLastPlayed, sizeof (int), 1);
	cf.Read (&gameStates.ogl.palAdd.red, sizeof (int), 1);
	cf.Read (&gameStates.ogl.palAdd.green, sizeof (int), 1);
	cf.Read (&gameStates.ogl.palAdd.blue, sizeof (int), 1);
	}
else {
	ResetPaletteAdd ();
	}

//	Load gameData.render.lights.subtracted
if (sgVersion >= 16) {
	cf.Read (gameData.render.lights.subtracted, sizeof (gameData.render.lights.subtracted [0]), (sgVersion > 22) ? MAX_SEGMENTS : MAX_SEGMENTS_D2);
	ApplyAllChangedLight ();
	//ComputeAllStaticLight ();	//	set xAvgSegLight field in tSegment struct.  See note at that function.
	}
else
	memset (gameData.render.lights.subtracted, 0, sizeof (gameData.render.lights.subtracted));

if (bSecretRestore) 
	gameStates.app.bFirstSecretVisit = 0;
else if (sgVersion >= 20)
	cf.Read (&gameStates.app.bFirstSecretVisit, sizeof (gameStates.app.bFirstSecretVisit), 1);
else
	gameStates.app.bFirstSecretVisit = 1;

if (sgVersion >= 22) {
	if (bSecretRestore != 1)
		cf.Read (&gameData.omega.xCharge, sizeof (fix), 1);
	else {
		fix	dummy_fix;
		cf.Read (&dummy_fix, sizeof (fix), 1);
		}
	}
*nLevel = nCurrentLevel;
return 1;
}

//------------------------------------------------------------------------------

int StateRestoreAllSub (const char *filename, int bMulti, int bSecretRestore)
{
	CFile		cf;
	char		szDesc [DESC_LENGTH + 1];
	char		id [5];
	int		nLevel, sgVersion, i;
	fix		xOldGameTime = gameData.time.xGame;

if (!cf.Open (filename, gameFolders.szSaveDir, "rb", 0))
	return 0;
StopTime ();
//Read id
cf.Read (id, sizeof (char)*4, 1);
if (memcmp (id, dgss_id, 4)) {
	cf.Close ();
	StartTime (1);
	return 0;
	}
//Read sgVersion
cf.Read (&sgVersion, sizeof (int), 1);
if (sgVersion < STATE_COMPATIBLE_VERSION)	{
	cf.Close ();
	StartTime (1);
	return 0;
	}
// Read description
cf.Read (szDesc, sizeof (char) * DESC_LENGTH, 1);
// Skip the current screen shot...
cf.Seek ((sgVersion < 26) ? THUMBNAIL_W * THUMBNAIL_H : THUMBNAIL_LW * THUMBNAIL_LH, SEEK_CUR);
// And now...skip the goddamn palette stuff that somebody forgot to add
cf.Seek ( 768, SEEK_CUR);
if (sgVersion < 27)
	i = StateRestoreBinGameData (cf, sgVersion, bMulti, bSecretRestore, xOldGameTime, &nLevel);
else
	i = StateRestoreUniGameData (cf, sgVersion, bMulti, bSecretRestore, xOldGameTime, &nLevel);
cf.Close ();
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
	CFile cf;
	int bBetweenLevels;
	char mission [16];
	char szDesc [DESC_LENGTH+1];
	char id [5];
	int dumbint;

if (!cf.Open (filename, gameFolders.szSaveDir, "rb", 0))
	return 0;
//Read id
cf.Read (id, sizeof (char)*4, 1);
if (memcmp (id, dgss_id, 4)) {
	cf.Close ();
	return 0;
	}
//Read sgVersion
cf.Read (&sgVersion, sizeof (int), 1);
if (sgVersion < STATE_COMPATIBLE_VERSION)	{
cf.Close ();
return 0;
}
// Read description
cf.Read (szDesc, sizeof (char)*DESC_LENGTH, 1);
// Skip the current screen shot...
cf.Seek ((sgVersion < 26) ? THUMBNAIL_W * THUMBNAIL_H : THUMBNAIL_LW * THUMBNAIL_LH, SEEK_CUR);
// And now...skip the palette stuff that somebody forgot to add
cf.Seek ( 768, SEEK_CUR);
// Read the Between levels flag...
cf.Read (&bBetweenLevels, sizeof (int), 1);
Assert (bBetweenLevels == 0);	//between levels save ripped out
// Read the mission info...
cf.Read (mission, sizeof (char), 9);
//Read level info
cf.Read (&dumbint, sizeof (int), 1);
cf.Read (&dumbint, sizeof (int), 1);
//Restore gameData.time.xGame
cf.Read (&dumbint, sizeof (fix), 1);
cf.Read (&gameData.app.nStateGameId, sizeof (int), 1);
return (gameData.app.nStateGameId);
}

//------------------------------------------------------------------------------
//eof

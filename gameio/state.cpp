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
#ifdef _DEBUG
#	define DBG(_expr)	_expr
#else
#	define DBG(_expr)
#endif

#define STATE_VERSION				38
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

static char szDesc [NUM_SAVES + 1] [DESC_LENGTH + 16];
static char szTime [NUM_SAVES + 1] [DESC_LENGTH + 16];

void GameRenderFrame (void);

//-------------------------------------------------------------------

#define NM_IMG_SPACE	6

static int bShowTime = 1;

void state_callback (int nitems, tMenuItem *items, int *last_key, int citem)
{
	int x, y, i = citem - NM_IMG_SPACE;

if (citem < 2)
	return;
if (!items [NM_IMG_SPACE - 1].text || strcmp (items [NM_IMG_SPACE - 1].text, szTime [i])) {
	items [NM_IMG_SPACE - 1].text = szTime [i];
	items [NM_IMG_SPACE - 1].rebuild = 1;
	}
if (!sc_bmp [i])
	return;
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
	GrBitmap ((grdCurCanv->cvBitmap.bmProps.w-THUMBNAIL_W) / 2,items [0].y - 5, sc_bmp [citem - 1]);
	}
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
	char filename [NUM_SAVES+1] [30];
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
	m [i].nType = NM_TYPE_INPUT_MENU; 
	m [i].text = szDesc [i]; 
	m [i].text_len = DESC_LENGTH-1;
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
	char			filename [NUM_SAVES+1] [30];
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
					CFRead (szDesc [j], sizeof (char) * DESC_LENGTH, 1, &cf);
					// rpad_string (szDesc [i], DESC_LENGTH-1);
					m [j+NM_IMG_SPACE].nType = NM_TYPE_MENU; 
					m [j+NM_IMG_SPACE].text = szDesc [j];
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

   bRestoringMenu=1;
	choice = state_default_item + NM_IMG_SPACE;
	i = ExecMenu3 (NULL, TXT_LOAD_GAME_MENU, j + NM_IMG_SPACE, m, state_callback, 
					   &choice, NULL, 190, -1);
	if (i < 0)
		return 0;
   bRestoringMenu=0;
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

int StateSaveAll (int bBetweenLevels, int bSecretSave, const char *pszFilenameOverride)
{
	int	rval, filenum = -1;
	char	filename [128], szDesc [DESC_LENGTH+1];

Assert (bBetweenLevels == 0);	//between levels save ripped out
if (gameData.app.nGameMode & GM_MULTI)	{
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
rval = StateSaveAllSub (filename, szDesc, bBetweenLevels);
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
	for (i = 0; i <= gameData.objs.nLastObject; i++) {
		if (OBJECTS [i].nType == OBJ_NONE) 
			continue;
		if (OBJECTS [i].nType == OBJ_CAMERA)
			OBJECTS [i].position.mOrient = gameData.cameras.cameras [gameData.objs.cameraRef [i]].orient;
		else if (OBJECTS [i].renderType==RT_MORPH) {
			tMorphInfo *md = MorphFindData (OBJECTS + i);
			if (md) {
				tObject *objP = md->objP;
				objP->controlType = md->saveControlType;
				objP->movementType = md->saveMovementType;
				objP->renderType = RT_POLYOBJ;
				objP->mType.physInfo = md->savePhysInfo;
				md->objP = NULL;
				} 
			else {						//maybe loaded half-morphed from disk
				KillObject (OBJECTS + i);
				OBJECTS [i].renderType = RT_POLYOBJ;
				OBJECTS [i].controlType = CT_NONE;
				OBJECTS [i].movementType = MT_NONE;
				}
			}
		}
//Save tObject info
	i = gameData.objs.nLastObject + 1;
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
	for (nObject = 0, nObjsWithTrigger = 0; nObject <= gameData.objs.nLastObject; nObject++) {
		nFirstTrigger = gameData.trigs.firstObjTrigger [nObject];
		if ((nFirstTrigger >= 0) && (nFirstTrigger < gameData.trigs.nObjTriggers))
			nObjsWithTrigger++;
		}
	CFWrite (&nObjsWithTrigger, sizeof (nObjsWithTrigger), 1, &cf);
	for (nObject = 0; nObject <= gameData.objs.nLastObject; nObject++) {
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

void StateSaveNetGame (CFILE *cfp)
{
	int	i, j;

CFWriteByte (netGame.nType, cfp);
CFWriteInt (netGame.nSecurity, cfp);
CFWrite (netGame.szGameName, 1, NETGAME_NAME_LEN + 1, cfp);
CFWrite (netGame.szMissionTitle, 1, MISSION_NAME_LEN + 1, cfp);
CFWrite (netGame.szMissionName, 1, 9, cfp);
CFWriteInt (netGame.nLevel, cfp);
CFWriteByte ((sbyte) netGame.gameMode, cfp);
CFWriteByte ((sbyte) netGame.bRefusePlayers, cfp);
CFWriteByte ((sbyte) netGame.difficulty, cfp);
CFWriteByte ((sbyte) netGame.gameStatus, cfp);
CFWriteByte ((sbyte) netGame.nNumPlayers, cfp);
CFWriteByte ((sbyte) netGame.nMaxPlayers, cfp);
CFWriteByte ((sbyte) netGame.nConnected, cfp);
CFWriteByte ((sbyte) netGame.gameFlags, cfp);
CFWriteByte ((sbyte) netGame.protocolVersion, cfp);
CFWriteByte ((sbyte) netGame.versionMajor, cfp);
CFWriteByte ((sbyte) netGame.versionMinor, cfp);
CFWriteByte ((sbyte) netGame.teamVector, cfp);
CFWriteByte ((sbyte) netGame.DoMegas, cfp);
CFWriteByte ((sbyte) netGame.DoSmarts, cfp);
CFWriteByte ((sbyte) netGame.DoFusions, cfp);
CFWriteByte ((sbyte) netGame.DoHelix, cfp);
CFWriteByte ((sbyte) netGame.DoPhoenix, cfp);
CFWriteByte ((sbyte) netGame.DoAfterburner, cfp);
CFWriteByte ((sbyte) netGame.DoInvulnerability, cfp);
CFWriteByte ((sbyte) netGame.DoCloak, cfp);
CFWriteByte ((sbyte) netGame.DoGauss, cfp);
CFWriteByte ((sbyte) netGame.DoVulcan, cfp);
CFWriteByte ((sbyte) netGame.DoPlasma, cfp);
CFWriteByte ((sbyte) netGame.DoOmega, cfp);
CFWriteByte ((sbyte) netGame.DoSuperLaser, cfp);
CFWriteByte ((sbyte) netGame.DoProximity, cfp);
CFWriteByte ((sbyte) netGame.DoSpread, cfp);
CFWriteByte ((sbyte) netGame.DoSmartMine, cfp);
CFWriteByte ((sbyte) netGame.DoFlash, cfp);
CFWriteByte ((sbyte) netGame.DoGuided, cfp);
CFWriteByte ((sbyte) netGame.DoEarthShaker, cfp);
CFWriteByte ((sbyte) netGame.DoMercury, cfp);
CFWriteByte ((sbyte) netGame.bAllowMarkerView, cfp);
CFWriteByte ((sbyte) netGame.bIndestructibleLights, cfp);
CFWriteByte ((sbyte) netGame.DoAmmoRack, cfp);
CFWriteByte ((sbyte) netGame.DoConverter, cfp);
CFWriteByte ((sbyte) netGame.DoHeadlight, cfp);
CFWriteByte ((sbyte) netGame.DoHoming, cfp);
CFWriteByte ((sbyte) netGame.DoLaserUpgrade, cfp);
CFWriteByte ((sbyte) netGame.DoQuadLasers, cfp);
CFWriteByte ((sbyte) netGame.bShowAllNames, cfp);
CFWriteByte ((sbyte) netGame.BrightPlayers, cfp);
CFWriteByte ((sbyte) netGame.invul, cfp);
CFWriteByte ((sbyte) netGame.FriendlyFireOff, cfp);
for (i = 0; i < 2; i++)
	CFWrite (netGame.team_name [i], 1, CALLSIGN_LEN + 1, cfp);		// 18 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	CFWriteInt (netGame.locations [i], cfp);
for (i = 0; i < MAX_PLAYERS; i++)
	for (j = 0; j < MAX_PLAYERS; j++)
		CFWriteShort (netGame.kills [i] [j], cfp);			// 128 bytes
CFWriteShort (netGame.nSegmentCheckSum, cfp);				// 2 bytes
for (i = 0; i < 2; i++)
	CFWriteShort (netGame.teamKills [i], cfp);				// 4 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	CFWriteShort (netGame.killed [i], cfp);					// 16 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	CFWriteShort (netGame.playerKills [i], cfp);			// 16 bytes
CFWriteInt (netGame.KillGoal, cfp);							// 4 bytes
CFWriteFix (netGame.xPlayTimeAllowed, cfp);					// 4 bytes
CFWriteFix (netGame.xLevelTime, cfp);							// 4 bytes
CFWriteInt (netGame.control_invulTime, cfp);				// 4 bytes
CFWriteInt (netGame.monitor_vector, cfp);					// 4 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	CFWriteInt (netGame.player_score [i], cfp);				// 32 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	CFWriteByte ((sbyte) netGame.playerFlags [i], cfp);	// 8 bytes
CFWriteShort (PacketsPerSec (), cfp);					// 2 bytes
CFWriteByte ((sbyte) netGame.bShortPackets, cfp);			// 1 bytes
// 279 bytes
// 355 bytes total
CFWrite (netGame.AuxData, NETGAME_AUX_SIZE, 1, cfp);  // Storage for protocol-specific data (e.g., multicast session and port)
}

//------------------------------------------------------------------------------

void StateSaveNetPlayers (CFILE *cfp)
{
	int	i;

CFWriteByte ((sbyte) netPlayers.nType, cfp);
CFWriteInt (netPlayers.nSecurity, cfp);
for (i = 0; i < MAX_PLAYERS + 4; i++) {
	CFWrite (netPlayers.players [i].callsign, 1, CALLSIGN_LEN + 1, cfp);
	CFWrite (netPlayers.players [i].network.ipx.server, 1, 4, cfp);
	CFWrite (netPlayers.players [i].network.ipx.node, 1, 6, cfp);
	CFWriteByte ((sbyte) netPlayers.players [i].versionMajor, cfp);
	CFWriteByte ((sbyte) netPlayers.players [i].versionMinor, cfp);
	CFWriteByte ((sbyte) netPlayers.players [i].computerType, cfp);
	CFWriteByte (netPlayers.players [i].connected, cfp);
	CFWriteShort ((short) netPlayers.players [i].socket, cfp);
	CFWriteByte ((sbyte) netPlayers.players [i].rank, cfp);
	}
}

//------------------------------------------------------------------------------

void StateSavePlayer (tPlayer *playerP, CFILE *cfp)
{
	int	i;

CFWrite (playerP->callsign, 1, CALLSIGN_LEN + 1, cfp); // The callsign of this tPlayer, for net purposes.
CFWrite (playerP->netAddress, 1, 6, cfp);					// The network address of the player.
CFWriteByte (playerP->connected, cfp);            // Is the tPlayer connected or not?
CFWriteInt (playerP->nObject, cfp);                // What tObject number this tPlayer is. (made an int by mk because it's very often referenced)
CFWriteInt (playerP->nPacketsGot, cfp);         // How many packets we got from them
CFWriteInt (playerP->nPacketsSent, cfp);        // How many packets we sent to them
CFWriteInt ((int) playerP->flags, cfp);           // Powerup flags, see below...
CFWriteFix (playerP->energy, cfp);                // Amount of energy remaining.
CFWriteFix (playerP->shields, cfp);               // shields remaining (protection)
CFWriteByte (playerP->lives, cfp);                // Lives remaining, 0 = game over.
CFWriteByte (playerP->level, cfp);                // Current level tPlayer is playing. (must be signed for secret levels)
CFWriteByte ((sbyte) playerP->laserLevel, cfp);  // Current level of the laser.
CFWriteByte (playerP->startingLevel, cfp);       // What level the tPlayer started on.
CFWriteShort (playerP->nKillerObj, cfp);       // Who killed me.... (-1 if no one)
CFWriteShort ((short) playerP->primaryWeaponFlags, cfp);   // bit set indicates the tPlayer has this weapon.
CFWriteShort ((short) playerP->secondaryWeaponFlags, cfp); // bit set indicates the tPlayer has this weapon.
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	CFWriteShort ((short) playerP->primaryAmmo [i], cfp); // How much ammo of each nType.
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
	CFWriteShort ((short) playerP->secondaryAmmo [i], cfp); // How much ammo of each nType.
#if 1 //for inventory system
CFWriteByte ((sbyte) playerP->nInvuls, cfp);
CFWriteByte ((sbyte) playerP->nCloaks, cfp);
#endif
CFWriteInt (playerP->lastScore, cfp);             // Score at beginning of current level.
CFWriteInt (playerP->score, cfp);                  // Current score.
CFWriteFix (playerP->timeLevel, cfp);             // Level time played
CFWriteFix (playerP->timeTotal, cfp);             // Game time played (high word = seconds)
if (playerP->cloakTime == 0x7fffffff)				// cloak cheat active
	CFWriteFix (playerP->cloakTime, cfp);			// Time invulnerable
else
	CFWriteFix (playerP->cloakTime - gameData.time.xGame, cfp);      // Time invulnerable
if (playerP->invulnerableTime == 0x7fffffff)		// invul cheat active
	CFWriteFix (playerP->invulnerableTime, cfp);      // Time invulnerable
else
	CFWriteFix (playerP->invulnerableTime - gameData.time.xGame, cfp);      // Time invulnerable
CFWriteShort (playerP->nKillGoalCount, cfp);          // Num of players killed this level
CFWriteShort (playerP->netKilledTotal, cfp);       // Number of times killed total
CFWriteShort (playerP->netKillsTotal, cfp);        // Number of net kills total
CFWriteShort (playerP->numKillsLevel, cfp);        // Number of kills this level
CFWriteShort (playerP->numKillsTotal, cfp);        // Number of kills total
CFWriteShort (playerP->numRobotsLevel, cfp);       // Number of initial robots this level
CFWriteShort (playerP->numRobotsTotal, cfp);       // Number of robots total
CFWriteShort ((short) playerP->hostages.nRescued, cfp); // Total number of hostages rescued.
CFWriteShort ((short) playerP->hostages.nTotal, cfp);         // Total number of hostages.
CFWriteByte ((sbyte) playerP->hostages.nOnBoard, cfp);      // Number of hostages on ship.
CFWriteByte ((sbyte) playerP->hostages.nLevel, cfp);         // Number of hostages on this level.
CFWriteFix (playerP->homingObjectDist, cfp);     // Distance of nearest homing tObject.
CFWriteByte (playerP->hoursLevel, cfp);            // Hours played (since timeTotal can only go up to 9 hours)
CFWriteByte (playerP->hoursTotal, cfp);            // Hours played (since timeTotal can only go up to 9 hours)
}

//------------------------------------------------------------------------------

void StateSaveObject (tObject *objP, CFILE *cfp)
{
CFWriteInt (objP->nSignature, cfp);      
CFWriteByte ((sbyte) objP->nType, cfp); 
CFWriteByte ((sbyte) objP->id, cfp);
CFWriteShort (objP->next, cfp);
CFWriteShort (objP->prev, cfp);
CFWriteByte ((sbyte) objP->controlType, cfp);
CFWriteByte ((sbyte) objP->movementType, cfp);
CFWriteByte ((sbyte) objP->renderType, cfp);
CFWriteByte ((sbyte) objP->flags, cfp);
CFWriteShort (objP->nSegment, cfp);
CFWriteShort (objP->attachedObj, cfp);
CFWriteVector (&OBJPOS (objP)->vPos, cfp);     
CFWriteMatrix (&OBJPOS (objP)->mOrient, cfp);  
CFWriteFix (objP->size, cfp); 
CFWriteFix (objP->shields, cfp);
CFWriteVector (&objP->vLastPos, cfp);  
CFWriteByte (objP->containsType, cfp); 
CFWriteByte (objP->containsId, cfp);   
CFWriteByte (objP->containsCount, cfp);
CFWriteByte (objP->matCenCreator, cfp);
CFWriteFix (objP->lifeleft, cfp);   
if (objP->movementType == MT_PHYSICS) {
	CFWriteVector (&objP->mType.physInfo.velocity, cfp);   
	CFWriteVector (&objP->mType.physInfo.thrust, cfp);     
	CFWriteFix (objP->mType.physInfo.mass, cfp);       
	CFWriteFix (objP->mType.physInfo.drag, cfp);       
	CFWriteFix (objP->mType.physInfo.brakes, cfp);     
	CFWriteVector (&objP->mType.physInfo.rotVel, cfp);     
	CFWriteVector (&objP->mType.physInfo.rotThrust, cfp);  
	CFWriteFixAng (objP->mType.physInfo.turnRoll, cfp);   
	CFWriteShort ((short) objP->mType.physInfo.flags, cfp);      
	}
else if (objP->movementType == MT_SPINNING) {
	CFWriteVector (&objP->mType.spinRate, cfp);  
	}
switch (objP->controlType) {
	case CT_WEAPON:
		CFWriteShort (objP->cType.laserInfo.parentType, cfp);
		CFWriteShort (objP->cType.laserInfo.nParentObj, cfp);
		CFWriteInt (objP->cType.laserInfo.nParentSig, cfp);
		CFWriteFix (objP->cType.laserInfo.creationTime, cfp);
		if (objP->cType.laserInfo.nLastHitObj)
			CFWriteShort (gameData.objs.nHitObjects [OBJ_IDX (objP) * MAX_HIT_OBJECTS + objP->cType.laserInfo.nLastHitObj - 1], cfp);
		else
			CFWriteShort (-1, cfp);
		CFWriteShort (objP->cType.laserInfo.nMslLock, cfp);
		CFWriteFix (objP->cType.laserInfo.multiplier, cfp);
		break;

	case CT_EXPLOSION:
		CFWriteFix (objP->cType.explInfo.nSpawnTime, cfp);
		CFWriteFix (objP->cType.explInfo.nDeleteTime, cfp);
		CFWriteShort (objP->cType.explInfo.nDeleteObj, cfp);
		CFWriteShort (objP->cType.explInfo.nAttachParent, cfp);
		CFWriteShort (objP->cType.explInfo.nPrevAttach, cfp);
		CFWriteShort (objP->cType.explInfo.nNextAttach, cfp);
		break;

	case CT_AI:
		CFWriteByte ((sbyte) objP->cType.aiInfo.behavior, cfp);
		CFWrite (objP->cType.aiInfo.flags, 1, MAX_AI_FLAGS, cfp);
		CFWriteShort (objP->cType.aiInfo.nHideSegment, cfp);
		CFWriteShort (objP->cType.aiInfo.nHideIndex, cfp);
		CFWriteShort (objP->cType.aiInfo.nPathLength, cfp);
		CFWriteByte (objP->cType.aiInfo.nCurPathIndex, cfp);
		CFWriteByte (objP->cType.aiInfo.bDyingSoundPlaying, cfp);
		CFWriteShort (objP->cType.aiInfo.nDangerLaser, cfp);
		CFWriteInt (objP->cType.aiInfo.nDangerLaserSig, cfp);
		CFWriteFix (objP->cType.aiInfo.xDyingStartTime, cfp);
		break;

	case CT_LIGHT:
		CFWriteFix (objP->cType.lightInfo.intensity, cfp);
		break;

	case CT_POWERUP:
		CFWriteInt (objP->cType.powerupInfo.count, cfp);
		CFWriteFix (objP->cType.powerupInfo.creationTime, cfp);
		CFWriteInt (objP->cType.powerupInfo.flags, cfp);
		break;
	}
switch (objP->renderType) {
	case RT_MORPH:
	case RT_POLYOBJ: {
		int i;
		CFWriteInt (objP->rType.polyObjInfo.nModel, cfp);
		for (i = 0; i < MAX_SUBMODELS; i++)
			CFWriteAngVec (objP->rType.polyObjInfo.animAngles + i, cfp);
		CFWriteInt (objP->rType.polyObjInfo.nSubObjFlags, cfp);
		CFWriteInt (objP->rType.polyObjInfo.nTexOverride, cfp);
		CFWriteInt (objP->rType.polyObjInfo.nAltTextures, cfp);
		break;
		}
	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
	case RT_THRUSTER:
		CFWriteInt (objP->rType.vClipInfo.nClipIndex, cfp);
		CFWriteFix (objP->rType.vClipInfo.xFrameTime, cfp);
		CFWriteByte (objP->rType.vClipInfo.nCurFrame, cfp);
		break;

	case RT_LASER:
		break;
	}
}

//------------------------------------------------------------------------------

void StateSaveWall (tWall *wallP, CFILE *cfp)
{
CFWriteInt (wallP->nSegment, cfp);
CFWriteInt (wallP->nSide, cfp);
CFWriteFix (wallP->hps, cfp);    
CFWriteInt (wallP->nLinkedWall, cfp);
CFWriteByte ((sbyte) wallP->nType, cfp);       
CFWriteByte ((sbyte) wallP->flags, cfp);      
CFWriteByte ((sbyte) wallP->state, cfp);      
CFWriteByte ((sbyte) wallP->nTrigger, cfp);    
CFWriteByte (wallP->nClip, cfp);   
CFWriteByte ((sbyte) wallP->keys, cfp);       
CFWriteByte (wallP->controllingTrigger, cfp);
CFWriteByte (wallP->cloakValue, cfp); 
}

//------------------------------------------------------------------------------

void StateSaveExplWall (tExplWall *wallP, CFILE *cfp)
{
CFWriteInt (wallP->nSegment, cfp);
CFWriteInt (wallP->nSide, cfp);
CFWriteFix (wallP->time, cfp);    
}

//------------------------------------------------------------------------------

void StateSaveCloakingWall (tCloakingWall *wallP, CFILE *cfp)
{
	int	i;

CFWriteShort (wallP->nFrontWall, cfp);
CFWriteShort (wallP->nBackWall, cfp); 
for (i = 0; i < 4; i++) {
	CFWriteFix (wallP->front_ls [i], cfp); 
	CFWriteFix (wallP->back_ls [i], cfp);
	}
CFWriteFix (wallP->time, cfp);    
}

//------------------------------------------------------------------------------

void StateSaveActiveDoor (tActiveDoor *doorP, CFILE *cfp)
{
	int	i;

CFWriteInt (doorP->nPartCount, cfp);
for (i = 0; i < 2; i++) {
	CFWriteShort (doorP->nFrontWall [i], cfp);
	CFWriteShort (doorP->nBackWall [i], cfp);
	}
CFWriteFix (doorP->time, cfp);    
}

//------------------------------------------------------------------------------

void StateSaveTrigger (tTrigger *triggerP, CFILE *cfp)
{
	int	i;

CFWriteByte ((sbyte) triggerP->nType, cfp); 
CFWriteByte ((sbyte) triggerP->flags, cfp); 
CFWriteByte (triggerP->nLinks, cfp);
CFWriteFix (triggerP->value, cfp);
CFWriteFix (triggerP->time, cfp);
for (i = 0; i < MAX_TRIGGER_TARGETS; i++) {
	CFWriteShort (triggerP->nSegment [i], cfp);
	CFWriteShort (triggerP->nSide [i], cfp);
	}
}

//------------------------------------------------------------------------------

void StateSaveObjTriggerRef (tObjTriggerRef *refP, CFILE *cfp)
{
CFWriteShort (refP->prev, cfp);
CFWriteShort (refP->next, cfp);
CFWriteShort (refP->nObject, cfp);
}

//------------------------------------------------------------------------------

void StateSaveMatCen (tMatCenInfo *matcenP, CFILE *cfp)
{
	int	i;

for (i = 0; i < 2; i++)
	CFWriteInt (matcenP->objFlags [i], cfp);
CFWriteFix (matcenP->xHitPoints, cfp);
CFWriteFix (matcenP->xInterval, cfp);
CFWriteShort (matcenP->nSegment, cfp);
CFWriteShort (matcenP->nFuelCen, cfp);
}

//------------------------------------------------------------------------------

void StateSaveFuelCen (tFuelCenInfo *fuelcenP, CFILE *cfp)
{
CFWriteInt (fuelcenP->nType, cfp);
CFWriteInt (fuelcenP->nSegment, cfp);
CFWriteByte (fuelcenP->bFlag, cfp);
CFWriteByte (fuelcenP->bEnabled, cfp);
CFWriteByte (fuelcenP->nLives, cfp);
CFWriteFix (fuelcenP->xCapacity, cfp);
CFWriteFix (fuelcenP->xMaxCapacity, cfp);
CFWriteFix (fuelcenP->xTimer, cfp);
CFWriteFix (fuelcenP->xDisableTime, cfp);
CFWriteVector (&fuelcenP->vCenter, cfp);
}

//------------------------------------------------------------------------------

void StateSaveReactorTrigger (tReactorTriggers *triggerP, CFILE *cfp)
{
	int	i;

CFWriteShort (triggerP->nLinks, cfp);
for (i = 0; i < MAX_CONTROLCEN_LINKS; i++) {
	CFWriteShort (triggerP->nSegment [i], cfp);
	CFWriteShort (triggerP->nSide [i], cfp);
	}
}

//------------------------------------------------------------------------------

void StateSaveSpawnPoint (int i, CFILE *cfp)
{
CFWriteVector (&gameData.multiplayer.playerInit [i].position.vPos, cfp);     
CFWriteMatrix (&gameData.multiplayer.playerInit [i].position.mOrient, cfp);  
CFWriteShort (gameData.multiplayer.playerInit [i].nSegment, cfp);
CFWriteShort (gameData.multiplayer.playerInit [i].nSegType, cfp);
}

//------------------------------------------------------------------------------

DBG (static int fPos);

void StateSaveUniGameData (CFILE *cfp, int bBetweenLevels)
{
	int		i, j;
	short		nObjsWithTrigger, nObject, nFirstTrigger;

// Save the Between levels flag...
CFWriteInt (bBetweenLevels, cfp);
// Save the mission info...
CFWrite (gameData.missions.list + gameData.missions.nCurrentMission, sizeof (char), 9, cfp);
//Save level info
CFWriteInt (gameData.missions.nCurrentLevel, cfp);
CFWriteInt (gameData.missions.nNextLevel, cfp);
//Save gameData.time.xGame
CFWriteFix (gameData.time.xGame, cfp);
// If coop save, save all
if (IsCoopGame) {
	CFWriteInt (gameData.app.nStateGameId, cfp);
	StateSaveNetGame (cfp);
	DBG (fPos = CFTell (cfp));
	StateSaveNetPlayers (cfp);
	DBG (fPos = CFTell (cfp));
	CFWriteInt (gameData.multiplayer.nPlayers, cfp);
	CFWriteInt (gameData.multiplayer.nLocalPlayer, cfp);
	for (i = 0; i < gameData.multiplayer.nPlayers; i++)
		StateSavePlayer (gameData.multiplayer.players + i, cfp);
	DBG (fPos = CFTell (cfp));
	}
//Save tPlayer info
StateSavePlayer (gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer, cfp);
// Save the current weapon info
CFWriteByte (gameData.weapons.nPrimary, cfp);
CFWriteByte (gameData.weapons.nSecondary, cfp);
// Save the difficulty level
CFWriteInt (gameStates.app.nDifficultyLevel, cfp);
// Save cheats enabled
CFWriteInt (gameStates.app.cheats.bEnabled, cfp);
for (i = 0; i < 2; i++) {
	CFWriteInt (fl2f (gameStates.gameplay.slowmo [i].fSpeed), cfp);
	CFWriteInt (gameStates.gameplay.slowmo [i].nState, cfp);
	}
for (i = 0; i < MAX_PLAYERS; i++)
	CFWriteInt (gameData.multiplayer.weaponStates [i].bTripleFusion, cfp);
if (!bBetweenLevels)	{
//Finish all morph OBJECTS
	for (i = 0; i <= gameData.objs.nLastObject; i++) {
		if (OBJECTS [i].nType == OBJ_NONE) 
			continue;
		if (OBJECTS [i].nType == OBJ_CAMERA)
			OBJECTS [i].position.mOrient = gameData.cameras.cameras [gameData.objs.cameraRef [i]].orient;
		else if (OBJECTS [i].renderType == RT_MORPH) {
			tMorphInfo *md = MorphFindData (OBJECTS + i);
			if (md) {
				tObject *objP = md->objP;
				objP->controlType = md->saveControlType;
				objP->movementType = md->saveMovementType;
				objP->renderType = RT_POLYOBJ;
				objP->mType.physInfo = md->savePhysInfo;
				md->objP = NULL;
				} 
			else {						//maybe loaded half-morphed from disk
				KillObject (OBJECTS + i);
				OBJECTS [i].renderType = RT_POLYOBJ;
				OBJECTS [i].controlType = CT_NONE;
				OBJECTS [i].movementType = MT_NONE;
				}
			}
		}
	DBG (fPos = CFTell (cfp));
//Save tObject info
	i = gameData.objs.nLastObject + 1;
	CFWriteInt (i, cfp);
	for (j = 0; j < i; j++)
		StateSaveObject (OBJECTS + j, cfp);
	DBG (fPos = CFTell (cfp));
//Save tWall info
	i = gameData.walls.nWalls;
	CFWriteInt (i, cfp);
	for (j = 0; j < i; j++)
		StateSaveWall (gameData.walls.walls + j, cfp);
	DBG (fPos = CFTell (cfp));
//Save exploding wall info
	i = MAX_EXPLODING_WALLS;
	CFWriteInt (i, cfp);
	for (j = 0; j < i; j++)
		StateSaveExplWall (gameData.walls.explWalls + j, cfp);
	DBG (fPos = CFTell (cfp));
//Save door info
	i = gameData.walls.nOpenDoors;
	CFWriteInt (i, cfp);
	for (j = 0; j < i; j++)
		StateSaveActiveDoor (gameData.walls.activeDoors + j, cfp);
	DBG (fPos = CFTell (cfp));
//Save cloaking tWall info
	i = gameData.walls.nCloaking;
	CFWriteInt (i, cfp);
	for (j = 0; j < i; j++)
		StateSaveCloakingWall (gameData.walls.cloaking + j, cfp);
	DBG (fPos = CFTell (cfp));
//Save tTrigger info
	CFWriteInt (gameData.trigs.nTriggers, cfp);
	for (i = 0; i < gameData.trigs.nTriggers; i++)
		StateSaveTrigger (gameData.trigs.triggers + i, cfp);
	DBG (fPos = CFTell (cfp));
	CFWriteInt (gameData.trigs.nObjTriggers, cfp);
	if (!gameData.trigs.nObjTriggers)
		CFWriteShort (0, cfp);
	else {
		for (i = 0; i < gameData.trigs.nObjTriggers; i++)
			StateSaveTrigger (gameData.trigs.objTriggers + i, cfp);
		for (i = 0; i < gameData.trigs.nObjTriggers; i++)
			StateSaveObjTriggerRef (gameData.trigs.objTriggerRefs + i, cfp);
		for (nObject = 0, nObjsWithTrigger = 0; nObject <= gameData.objs.nLastObject; nObject++) {
			nFirstTrigger = gameData.trigs.firstObjTrigger [nObject];
			if ((nFirstTrigger >= 0) && (nFirstTrigger < gameData.trigs.nObjTriggers))
				nObjsWithTrigger++;
			}
		CFWriteShort (nObjsWithTrigger, cfp);
		for (nObject = 0; nObject <= gameData.objs.nLastObject; nObject++) {
			nFirstTrigger = gameData.trigs.firstObjTrigger [nObject];
			if ((nFirstTrigger >= 0) && (nFirstTrigger < gameData.trigs.nObjTriggers)) {
				CFWriteShort (nObject, cfp);
				CFWriteShort (nFirstTrigger, cfp);
				}
			}
		}
	DBG (fPos = CFTell (cfp));
//Save tmap info
	for (i = 0; i <= gameData.segs.nLastSegment; i++) {
		for (j = 0; j < 6; j++)	{
			ushort nWall = WallNumI ((short) i, (short) j);
			CFWriteShort (nWall, cfp);
			CFWriteShort (gameData.segs.segments [i].sides [j].nBaseTex, cfp);
			CFWriteShort (gameData.segs.segments [i].sides [j].nOvlTex | (gameData.segs.segments [i].sides [j].nOvlOrient << 14), cfp);
			}
		}
	DBG (fPos = CFTell (cfp));
// Save the fuelcen info
	CFWriteInt (gameData.reactor.bDestroyed, cfp);
	CFWriteFix (gameData.reactor.countdown.nTimer, cfp);
	DBG (fPos = CFTell (cfp));
	CFWriteInt (gameData.matCens.nBotCenters, cfp);
	for (i = 0; i < gameData.matCens.nBotCenters; i++)
		StateSaveMatCen (gameData.matCens.botGens + i, cfp);
	CFWriteInt (gameData.matCens.nEquipCenters, cfp);
	for (i = 0; i < gameData.matCens.nEquipCenters; i++)
		StateSaveMatCen (gameData.matCens.equipGens + i, cfp);
	StateSaveReactorTrigger (&gameData.reactor.triggers, cfp);
	CFWriteInt (gameData.matCens.nFuelCenters, cfp);
	for (i = 0; i < gameData.matCens.nFuelCenters; i++)
		StateSaveFuelCen (gameData.matCens.fuelCenters + i, cfp);
	DBG (fPos = CFTell (cfp));
// Save the control cen info
	CFWriteInt (gameData.reactor.bPresent, cfp);
	for (i = 0; i < MAX_BOSS_COUNT; i++) {
		CFWriteInt (gameData.reactor.states [i].nObject, cfp);
		CFWriteInt (gameData.reactor.states [i].bHit, cfp);
		CFWriteInt (gameData.reactor.states [i].bSeenPlayer, cfp);
		CFWriteInt (gameData.reactor.states [i].nNextFireTime, cfp);
		CFWriteInt (gameData.reactor.states [i].nDeadObj, cfp);
		}
	DBG (fPos = CFTell (cfp));
// Save the AI state
	AISaveUniState (cfp);

	DBG (fPos = CFTell (cfp));
// Save the automap visited info
	for (i = 0; i < MAX_SEGMENTS; i++)
		CFWriteShort (gameData.render.mine.bAutomapVisited [i], cfp);
	DBG (fPos = CFTell (cfp));
	}
CFWriteInt ((int) gameData.app.nStateGameId, cfp);
CFWriteInt (gameStates.app.cheats.bLaserRapidFire, cfp);
CFWriteInt (gameStates.app.bLunacy, cfp);		//	Yes, writing this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
CFWriteInt (gameStates.app.bLunacy, cfp);
// Save automap marker info
for (i = 0; i < NUM_MARKERS; i++)
	CFWriteShort (gameData.marker.objects [i], cfp);
CFWrite (gameData.marker.nOwner, sizeof (gameData.marker.nOwner), 1, cfp);
CFWrite (gameData.marker.szMessage, sizeof (gameData.marker.szMessage), 1, cfp);
CFWriteFix (gameData.physics.xAfterburnerCharge, cfp);
//save last was super information
CFWrite (bLastPrimaryWasSuper, sizeof (bLastPrimaryWasSuper), 1, cfp);
CFWrite (bLastSecondaryWasSuper, sizeof (bLastSecondaryWasSuper), 1, cfp);
//	Save flash effect stuff
CFWriteFix (gameData.render.xFlashEffect, cfp);
CFWriteFix (gameData.render.xTimeFlashLastPlayed, cfp);
CFWriteShort (gameStates.ogl.palAdd.red, cfp);
CFWriteShort (gameStates.ogl.palAdd.green, cfp);
CFWriteShort (gameStates.ogl.palAdd.blue, cfp);
CFWrite (gameData.render.lights.subtracted, sizeof (gameData.render.lights.subtracted [0]), MAX_SEGMENTS, cfp);
CFWriteInt (gameStates.app.bFirstSecretVisit, cfp);
CFWriteFix (gameData.omega.xCharge [0], cfp);
CFWriteShort (gameData.missions.nEnteredFromLevel, cfp);
for (i = 0; i < MAX_PLAYERS; i++)
	StateSaveSpawnPoint (i, cfp);
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
	if (curDrawBuffer == GL_BACK) {
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
	OglReadBuffer (GL_FRONT, 1);
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

COMPUTE_SEGMENT_CENTER_I (&OBJECTS [nPlayerObj].position.vPos, 
							     gameData.segs.secret.nReturnSegment);
if (bRelink)
	RelinkObject (nPlayerObj, gameData.segs.secret.nReturnSegment);
ResetPlayerObject ();
OBJECTS [nPlayerObj].position.mOrient = gameData.segs.secret.returnOrient;
}

//	-----------------------------------------------------------------------------------

int StateRestoreAll (int bInGame, int bSecretRestore, const char *pszFilenameOverride)
{
	char filename [128];
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
	char	temp_filename [128];
	sprintf (temp_filename, "%s.sg%x", LOCALPLAYER.callsign, NUM_SAVES);
	StateSaveAll (!bInGame, bSecretRestore, temp_filename);
	}
if (!bSecretRestore && bInGame) {
	int choice = ExecMessageBox (NULL, NULL, 2, TXT_YES, TXT_NO, TXT_CONFIRM_LOAD);
	if (choice != 0) {
		gameData.app.bGamePaused = 0;
		StartTime (1);
		return 0;
		}
	}
gameStates.app.bGameRunning = 0;
i = StateRestoreAllSub (filename, 0, bSecretRestore);
gameData.app.bGamePaused = 0;
/*---*/PrintLog ("   rebuilding OpenGL texture data\n");
/*---*/PrintLog ("      rebuilding effects\n");
if (i)
	RebuildRenderContext (1);
StartTime (1);
return i;
}

//------------------------------------------------------------------------------

int CFReadBoundedInt (int nMax, int *nVal, CFILE *cfp)
{
	int	i;

i = CFReadInt (cfp);
if ((i < 0) || (i > nMax)) {
	Warning (TXT_SAVE_CORRUPT);
	//CFClose (cfp);
	return 1;
	}
*nVal = i;
return 0;
}

//------------------------------------------------------------------------------

int StateLoadMission (CFILE *cfp)
{
	char	szMission [16];
	int	i, nVersionFilter = gameOpts->app.nVersionFilter;

CFRead (szMission, sizeof (char), 9, cfp);
szMission [9] = '\0';
gameOpts->app.nVersionFilter = 3;
i = LoadMissionByName (szMission, -1);
gameOpts->app.nVersionFilter = nVersionFilter;
if (i)
	return 1;
ExecMessageBox (NULL, NULL, 1, "Ok", TXT_MSN_LOAD_ERROR, szMission);
//CFClose (cfp);
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
	OBJECTS [nServerObjNum].id = nServerObjNum;
	OBJECTS [nOtherObjNum].id = 0;
	if (gameData.multiplayer.nLocalPlayer == nServerObjNum) {
		OBJECTS [nServerObjNum].controlType = CT_FLYING;
		OBJECTS [nOtherObjNum].controlType = CT_REMOTE;
		}
	else if (gameData.multiplayer.nLocalPlayer == nOtherObjNum) {
		OBJECTS [nServerObjNum].controlType = CT_REMOTE;
		OBJECTS [nOtherObjNum].controlType = CT_FLYING;
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
for (i = 0; i <= gameData.objs.nLastObject; i++, objP++) {
	objP->rType.polyObjInfo.nAltTextures = -1;
	nSegment = objP->nSegment;
	// hack for a bug I haven't yet been able to fix 
	if ((objP->nType != OBJ_REACTOR) && (objP->shields < 0)) {
		j = FindBoss (i);
		if ((j < 0) || (gameData.boss [j].nDying != i))
			objP->nType = OBJ_NONE;
		}
	objP->next = objP->prev = objP->nSegment = -1;
	if (objP->nType != OBJ_NONE) {
		LinkObject (i, nSegment);
		if (objP->nSignature > gameData.objs.nNextSignature)
			gameData.objs.nNextSignature = objP->nSignature;
		}
	//look for, and fix, boss with bogus shields
	if ((objP->nType == OBJ_ROBOT) && ROBOTINFO (objP->id).bossFlag) {
		fix xShieldSave = objP->shields;
		CopyDefaultsToRobot (objP);		//calculate starting shields
		//if in valid range, use loaded shield value
		if (xShieldSave > 0 && (xShieldSave <= objP->shields))
			objP->shields = xShieldSave;
		else
			objP->shields /= 2;  //give tPlayer a break
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

void StateRestoreNetGame (CFILE *cfp)
{
	int	i, j;

netGame.nType = CFReadByte (cfp);
netGame.nSecurity = CFReadInt (cfp);
CFRead (netGame.szGameName, 1, NETGAME_NAME_LEN + 1, cfp);
CFRead (netGame.szMissionTitle, 1, MISSION_NAME_LEN + 1, cfp);
CFRead (netGame.szMissionName, 1, 9, cfp);
netGame.nLevel = CFReadInt (cfp);
netGame.gameMode = (ubyte) CFReadByte (cfp);
netGame.bRefusePlayers = (ubyte) CFReadByte (cfp);
netGame.difficulty = (ubyte) CFReadByte (cfp);
netGame.gameStatus = (ubyte) CFReadByte (cfp);
netGame.nNumPlayers = (ubyte) CFReadByte (cfp);
netGame.nMaxPlayers = (ubyte) CFReadByte (cfp);
netGame.nConnected = (ubyte) CFReadByte (cfp);
netGame.gameFlags = (ubyte) CFReadByte (cfp);
netGame.protocolVersion = (ubyte) CFReadByte (cfp);
netGame.versionMajor = (ubyte) CFReadByte (cfp);
netGame.versionMinor = (ubyte) CFReadByte (cfp);
netGame.teamVector = (ubyte) CFReadByte (cfp);
netGame.DoMegas = (ubyte) CFReadByte (cfp);
netGame.DoSmarts = (ubyte) CFReadByte (cfp);
netGame.DoFusions = (ubyte) CFReadByte (cfp);
netGame.DoHelix = (ubyte) CFReadByte (cfp);
netGame.DoPhoenix = (ubyte) CFReadByte (cfp);
netGame.DoAfterburner = (ubyte) CFReadByte (cfp);
netGame.DoInvulnerability = (ubyte) CFReadByte (cfp);
netGame.DoCloak = (ubyte) CFReadByte (cfp);
netGame.DoGauss = (ubyte) CFReadByte (cfp);
netGame.DoVulcan = (ubyte) CFReadByte (cfp);
netGame.DoPlasma = (ubyte) CFReadByte (cfp);
netGame.DoOmega = (ubyte) CFReadByte (cfp);
netGame.DoSuperLaser = (ubyte) CFReadByte (cfp);
netGame.DoProximity = (ubyte) CFReadByte (cfp);
netGame.DoSpread = (ubyte) CFReadByte (cfp);
netGame.DoSmartMine = (ubyte) CFReadByte (cfp);
netGame.DoFlash = (ubyte) CFReadByte (cfp);
netGame.DoGuided = (ubyte) CFReadByte (cfp);
netGame.DoEarthShaker = (ubyte) CFReadByte (cfp);
netGame.DoMercury = (ubyte) CFReadByte (cfp);
netGame.bAllowMarkerView = (ubyte) CFReadByte (cfp);
netGame.bIndestructibleLights = (ubyte) CFReadByte (cfp);
netGame.DoAmmoRack = (ubyte) CFReadByte (cfp);
netGame.DoConverter = (ubyte) CFReadByte (cfp);
netGame.DoHeadlight = (ubyte) CFReadByte (cfp);
netGame.DoHoming = (ubyte) CFReadByte (cfp);
netGame.DoLaserUpgrade = (ubyte) CFReadByte (cfp);
netGame.DoQuadLasers = (ubyte) CFReadByte (cfp);
netGame.bShowAllNames = (ubyte) CFReadByte (cfp);
netGame.BrightPlayers = (ubyte) CFReadByte (cfp);
netGame.invul = (ubyte) CFReadByte (cfp);
netGame.FriendlyFireOff = (ubyte) CFReadByte (cfp);
for (i = 0; i < 2; i++)
	CFRead (netGame.team_name [i], 1, CALLSIGN_LEN + 1, cfp);		// 18 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	netGame.locations [i] = CFReadInt (cfp);
for (i = 0; i < MAX_PLAYERS; i++)
	for (j = 0; j < MAX_PLAYERS; j++)
		netGame.kills [i] [j] = CFReadShort (cfp);			// 128 bytes
netGame.nSegmentCheckSum = CFReadShort (cfp);				// 2 bytes
for (i = 0; i < 2; i++)
	netGame.teamKills [i] = CFReadShort (cfp);				// 4 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	netGame.killed [i] = CFReadShort (cfp);					// 16 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	netGame.playerKills [i] = CFReadShort (cfp);			// 16 bytes
netGame.KillGoal = CFReadInt (cfp);							// 4 bytes
netGame.xPlayTimeAllowed = CFReadFix (cfp);					// 4 bytes
netGame.xLevelTime = CFReadFix (cfp);							// 4 bytes
netGame.control_invulTime = CFReadInt (cfp);				// 4 bytes
netGame.monitor_vector = CFReadInt (cfp);					// 4 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	netGame.player_score [i] = CFReadInt (cfp);				// 32 bytes
for (i = 0; i < MAX_PLAYERS; i++)
	netGame.playerFlags [i] = (ubyte) CFReadByte (cfp);	// 8 bytes
netGame.nPacketsPerSec = CFReadShort (cfp);					// 2 bytes
netGame.bShortPackets = (ubyte) CFReadByte (cfp);			// 1 bytes
// 279 bytes
// 355 bytes total
CFRead (netGame.AuxData, NETGAME_AUX_SIZE, 1, cfp);  // Storage for protocol-specific data (e.g., multicast session and port)
}

//------------------------------------------------------------------------------

void StateRestoreNetPlayers (CFILE *cfp)
{
	int	i;

netPlayers.nType = (ubyte) CFReadByte (cfp);
netPlayers.nSecurity = CFReadInt (cfp);
for (i = 0; i < MAX_PLAYERS + 4; i++) {
	CFRead (netPlayers.players [i].callsign, 1, CALLSIGN_LEN + 1, cfp);
	CFRead (netPlayers.players [i].network.ipx.server, 1, 4, cfp);
	CFRead (netPlayers.players [i].network.ipx.node, 1, 6, cfp);
	netPlayers.players [i].versionMajor = (ubyte) CFReadByte (cfp);
	netPlayers.players [i].versionMinor = (ubyte) CFReadByte (cfp);
	netPlayers.players [i].computerType = (enum compType) CFReadByte (cfp);
	netPlayers.players [i].connected = CFReadByte (cfp);
	netPlayers.players [i].socket = (ushort) CFReadShort (cfp);
	netPlayers.players [i].rank = (ubyte) CFReadByte (cfp);
	}
}

//------------------------------------------------------------------------------

void StateRestorePlayer (tPlayer *playerP, CFILE *cfp)
{
	int	i;

CFRead (playerP->callsign, 1, CALLSIGN_LEN + 1, cfp); // The callsign of this tPlayer, for net purposes.
CFRead (playerP->netAddress, 1, 6, cfp);					// The network address of the player.
playerP->connected = CFReadByte (cfp);            // Is the tPlayer connected or not?
playerP->nObject = CFReadInt (cfp);                // What tObject number this tPlayer is. (made an int by mk because it's very often referenced)
playerP->nPacketsGot = CFReadInt (cfp);         // How many packets we got from them
playerP->nPacketsSent = CFReadInt (cfp);        // How many packets we sent to them
playerP->flags = (uint) CFReadInt (cfp);           // Powerup flags, see below...
playerP->energy = CFReadFix (cfp);                // Amount of energy remaining.
playerP->shields = CFReadFix (cfp);               // shields remaining (protection)
playerP->lives = CFReadByte (cfp);                // Lives remaining, 0 = game over.
playerP->level = CFReadByte (cfp);                // Current level tPlayer is playing. (must be signed for secret levels)
playerP->laserLevel = (ubyte) CFReadByte (cfp);  // Current level of the laser.
playerP->startingLevel = CFReadByte (cfp);       // What level the tPlayer started on.
playerP->nKillerObj = CFReadShort (cfp);       // Who killed me.... (-1 if no one)
playerP->primaryWeaponFlags = (ushort) CFReadShort (cfp);   // bit set indicates the tPlayer has this weapon.
playerP->secondaryWeaponFlags = (ushort) CFReadShort (cfp); // bit set indicates the tPlayer has this weapon.
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	playerP->primaryAmmo [i] = (ushort) CFReadShort (cfp); // How much ammo of each nType.
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
	playerP->secondaryAmmo [i] = (ushort) CFReadShort (cfp); // How much ammo of each nType.
#if 1 //for inventory system
playerP->nInvuls = (ubyte) CFReadByte (cfp);
playerP->nCloaks = (ubyte) CFReadByte (cfp);
#endif
playerP->lastScore = CFReadInt (cfp);           // Score at beginning of current level.
playerP->score = CFReadInt (cfp);                // Current score.
playerP->timeLevel = CFReadFix (cfp);            // Level time played
playerP->timeTotal = CFReadFix (cfp);				// Game time played (high word = seconds)
playerP->cloakTime = CFReadFix (cfp);					// Time cloaked
if (playerP->cloakTime != 0x7fffffff)
	playerP->cloakTime += gameData.time.xGame;
playerP->invulnerableTime = CFReadFix (cfp);      // Time invulnerable
if (playerP->invulnerableTime != 0x7fffffff)
	playerP->invulnerableTime += gameData.time.xGame;
playerP->nKillGoalCount = CFReadShort (cfp);          // Num of players killed this level
playerP->netKilledTotal = CFReadShort (cfp);       // Number of times killed total
playerP->netKillsTotal = CFReadShort (cfp);        // Number of net kills total
playerP->numKillsLevel = CFReadShort (cfp);        // Number of kills this level
playerP->numKillsTotal = CFReadShort (cfp);        // Number of kills total
playerP->numRobotsLevel = CFReadShort (cfp);       // Number of initial robots this level
playerP->numRobotsTotal = CFReadShort (cfp);       // Number of robots total
playerP->hostages.nRescued = (ushort) CFReadShort (cfp); // Total number of hostages rescued.
playerP->hostages.nTotal = (ushort) CFReadShort (cfp);         // Total number of hostages.
playerP->hostages.nOnBoard = (ubyte) CFReadByte (cfp);      // Number of hostages on ship.
playerP->hostages.nLevel = (ubyte) CFReadByte (cfp);         // Number of hostages on this level.
playerP->homingObjectDist = CFReadFix (cfp);     // Distance of nearest homing tObject.
playerP->hoursLevel = CFReadByte (cfp);            // Hours played (since timeTotal can only go up to 9 hours)
playerP->hoursTotal = CFReadByte (cfp);            // Hours played (since timeTotal can only go up to 9 hours)
}

//------------------------------------------------------------------------------

void StateRestoreObject (tObject *objP, CFILE *cfp, int sgVersion)
{
objP->nSignature = CFReadInt (cfp);      
objP->nType = (ubyte) CFReadByte (cfp); 
#ifdef _DEBUG
if (objP->nType == OBJ_REACTOR)
	objP->nType = objP->nType;
else 
#endif
if ((sgVersion < 32) && IS_BOSS (objP))
	gameData.boss [(int) extraGameInfo [0].nBossCount++].nObject = OBJ_IDX (objP);
objP->id = (ubyte) CFReadByte (cfp);
objP->next = CFReadShort (cfp);
objP->prev = CFReadShort (cfp);
objP->controlType = (ubyte) CFReadByte (cfp);
objP->movementType = (ubyte) CFReadByte (cfp);
objP->renderType = (ubyte) CFReadByte (cfp);
objP->flags = (ubyte) CFReadByte (cfp);
objP->nSegment = CFReadShort (cfp);
objP->attachedObj = CFReadShort (cfp);
CFReadVector (&objP->position.vPos, cfp);     
CFReadMatrix (&objP->position.mOrient, cfp);  
objP->size = CFReadFix (cfp); 
objP->shields = CFReadFix (cfp);
CFReadVector (&objP->vLastPos, cfp);  
objP->containsType = CFReadByte (cfp); 
objP->containsId = CFReadByte (cfp);   
objP->containsCount = CFReadByte (cfp);
objP->matCenCreator = CFReadByte (cfp);
objP->lifeleft = CFReadFix (cfp);   
if (objP->movementType == MT_PHYSICS) {
	CFReadVector (&objP->mType.physInfo.velocity, cfp);   
	CFReadVector (&objP->mType.physInfo.thrust, cfp);     
	objP->mType.physInfo.mass = CFReadFix (cfp);       
	objP->mType.physInfo.drag = CFReadFix (cfp);       
	objP->mType.physInfo.brakes = CFReadFix (cfp);     
	CFReadVector (&objP->mType.physInfo.rotVel, cfp);     
	CFReadVector (&objP->mType.physInfo.rotThrust, cfp);  
	objP->mType.physInfo.turnRoll = CFReadFixAng (cfp);   
	objP->mType.physInfo.flags = (ushort) CFReadShort (cfp);      
	}
else if (objP->movementType == MT_SPINNING) {
	CFReadVector (&objP->mType.spinRate, cfp);  
	}
switch (objP->controlType) {
	case CT_WEAPON:
		objP->cType.laserInfo.parentType = CFReadShort (cfp);
		objP->cType.laserInfo.nParentObj = CFReadShort (cfp);
		objP->cType.laserInfo.nParentSig = CFReadInt (cfp);
		objP->cType.laserInfo.creationTime = CFReadFix (cfp);
		objP->cType.laserInfo.nLastHitObj = CFReadShort (cfp);
		if (objP->cType.laserInfo.nLastHitObj < 0)
			objP->cType.laserInfo.nLastHitObj = 0;
		else {
			gameData.objs.nHitObjects [OBJ_IDX (objP) * MAX_HIT_OBJECTS] = objP->cType.laserInfo.nLastHitObj;
			objP->cType.laserInfo.nLastHitObj = 1;
			}
		objP->cType.laserInfo.nMslLock = CFReadShort (cfp);
		objP->cType.laserInfo.multiplier = CFReadFix (cfp);
		break;

	case CT_EXPLOSION:
		objP->cType.explInfo.nSpawnTime = CFReadFix (cfp);
		objP->cType.explInfo.nDeleteTime = CFReadFix (cfp);
		objP->cType.explInfo.nDeleteObj = CFReadShort (cfp);
		objP->cType.explInfo.nAttachParent = CFReadShort (cfp);
		objP->cType.explInfo.nPrevAttach = CFReadShort (cfp);
		objP->cType.explInfo.nNextAttach = CFReadShort (cfp);
		break;

	case CT_AI:
		objP->cType.aiInfo.behavior = (ubyte) CFReadByte (cfp);
		CFRead (objP->cType.aiInfo.flags, 1, MAX_AI_FLAGS, cfp);
		objP->cType.aiInfo.nHideSegment = CFReadShort (cfp);
		objP->cType.aiInfo.nHideIndex = CFReadShort (cfp);
		objP->cType.aiInfo.nPathLength = CFReadShort (cfp);
		objP->cType.aiInfo.nCurPathIndex = CFReadByte (cfp);
		objP->cType.aiInfo.bDyingSoundPlaying = CFReadByte (cfp);
		objP->cType.aiInfo.nDangerLaser = CFReadShort (cfp);
		objP->cType.aiInfo.nDangerLaserSig = CFReadInt (cfp);
		objP->cType.aiInfo.xDyingStartTime = CFReadFix (cfp);
		break;

	case CT_LIGHT:
		objP->cType.lightInfo.intensity = CFReadFix (cfp);
		break;

	case CT_POWERUP:
		objP->cType.powerupInfo.count = CFReadInt (cfp);
		objP->cType.powerupInfo.creationTime = CFReadFix (cfp);
		objP->cType.powerupInfo.flags = CFReadInt (cfp);
		break;
	}
switch (objP->renderType) {
	case RT_MORPH:
	case RT_POLYOBJ: {
		int i;
		objP->rType.polyObjInfo.nModel = CFReadInt (cfp);
		for (i = 0; i < MAX_SUBMODELS; i++)
			CFReadAngVec (objP->rType.polyObjInfo.animAngles + i, cfp);
		objP->rType.polyObjInfo.nSubObjFlags = CFReadInt (cfp);
		objP->rType.polyObjInfo.nTexOverride = CFReadInt (cfp);
		objP->rType.polyObjInfo.nAltTextures = CFReadInt (cfp);
		break;
		}
	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
	case RT_THRUSTER:
		objP->rType.vClipInfo.nClipIndex = CFReadInt (cfp);
		objP->rType.vClipInfo.xFrameTime = CFReadFix (cfp);
		objP->rType.vClipInfo.nCurFrame = CFReadByte (cfp);
		break;

	case RT_LASER:
		break;
	}
}

//------------------------------------------------------------------------------

void StateRestoreWall (tWall *wallP, CFILE *cfp)
{
wallP->nSegment = CFReadInt (cfp);
wallP->nSide = CFReadInt (cfp);
wallP->hps = CFReadFix (cfp);    
wallP->nLinkedWall = CFReadInt (cfp);
wallP->nType = (ubyte) CFReadByte (cfp);       
wallP->flags = (ubyte) CFReadByte (cfp);      
wallP->state = (ubyte) CFReadByte (cfp);      
wallP->nTrigger = (ubyte) CFReadByte (cfp);    
wallP->nClip = CFReadByte (cfp);   
wallP->keys = (ubyte) CFReadByte (cfp);       
wallP->controllingTrigger = CFReadByte (cfp);
wallP->cloakValue = CFReadByte (cfp); 
}

//------------------------------------------------------------------------------

void StateRestoreExplWall (tExplWall *wallP, CFILE *cfp)
{
wallP->nSegment = CFReadInt (cfp);
wallP->nSide = CFReadInt (cfp);
wallP->time = CFReadFix (cfp);    
}

//------------------------------------------------------------------------------

void StateRestoreCloakingWall (tCloakingWall *wallP, CFILE *cfp)
{
	int	i;

wallP->nFrontWall = CFReadShort (cfp);
wallP->nBackWall = CFReadShort (cfp); 
for (i = 0; i < 4; i++) {
	wallP->front_ls [i] = CFReadFix (cfp); 
	wallP->back_ls [i] = CFReadFix (cfp);
	}
wallP->time = CFReadFix (cfp);    
}

//------------------------------------------------------------------------------

void StateRestoreActiveDoor (tActiveDoor *doorP, CFILE *cfp)
{
	int	i;

doorP->nPartCount = CFReadInt (cfp);
for (i = 0; i < 2; i++) {
	doorP->nFrontWall [i] = CFReadShort (cfp);
	doorP->nBackWall [i] = CFReadShort (cfp);
	}
doorP->time = CFReadFix (cfp);    
}

//------------------------------------------------------------------------------

void StateRestoreTrigger (tTrigger *triggerP, CFILE *cfp)
{
	int	i;

i = CFTell (cfp);
triggerP->nType = (ubyte) CFReadByte (cfp); 
triggerP->flags = (ubyte) CFReadByte (cfp); 
triggerP->nLinks = CFReadByte (cfp);
triggerP->value = CFReadFix (cfp);
triggerP->time = CFReadFix (cfp);
for (i = 0; i < MAX_TRIGGER_TARGETS; i++) {
	triggerP->nSegment [i] = CFReadShort (cfp);
	triggerP->nSide [i] = CFReadShort (cfp);
	}
}

//------------------------------------------------------------------------------

void StateRestoreObjTriggerRef (tObjTriggerRef *refP, CFILE *cfp)
{
refP->prev = CFReadShort (cfp);
refP->next = CFReadShort (cfp);
refP->nObject = CFReadShort (cfp);
}

//------------------------------------------------------------------------------

void StateRestoreMatCen (tMatCenInfo *matcenP, CFILE *cfp)
{
	int	i;

for (i = 0; i < 2; i++)
	matcenP->objFlags [i] = CFReadInt (cfp);
matcenP->xHitPoints = CFReadFix (cfp);
matcenP->xInterval = CFReadFix (cfp);
matcenP->nSegment = CFReadShort (cfp);
matcenP->nFuelCen = CFReadShort (cfp);
}

//------------------------------------------------------------------------------

void StateRestoreFuelCen (tFuelCenInfo *fuelcenP, CFILE *cfp)
{
fuelcenP->nType = CFReadInt (cfp);
fuelcenP->nSegment = CFReadInt (cfp);
fuelcenP->bFlag = CFReadByte (cfp);
fuelcenP->bEnabled = CFReadByte (cfp);
fuelcenP->nLives = CFReadByte (cfp);
fuelcenP->xCapacity = CFReadFix (cfp);
fuelcenP->xMaxCapacity = CFReadFix (cfp);
fuelcenP->xTimer = CFReadFix (cfp);
fuelcenP->xDisableTime = CFReadFix (cfp);
CFReadVector (&fuelcenP->vCenter, cfp);
}

//------------------------------------------------------------------------------

void StateRestoreReactorTrigger (tReactorTriggers *triggerP, CFILE *cfp)
{
	int	i;

triggerP->nLinks = CFReadShort (cfp);
for (i = 0; i < MAX_CONTROLCEN_LINKS; i++) {
	triggerP->nSegment [i] = CFReadShort (cfp);
	triggerP->nSide [i] = CFReadShort (cfp);
	}
}

//------------------------------------------------------------------------------

void StateRestoreSpawnPoint (int i, CFILE *cfp)
{
CFReadVector (&gameData.multiplayer.playerInit [i].position.vPos, cfp);     
CFReadMatrix (&gameData.multiplayer.playerInit [i].position.mOrient, cfp);  
gameData.multiplayer.playerInit [i].nSegment = CFReadShort (cfp);
gameData.multiplayer.playerInit [i].nSegType = CFReadShort (cfp);
}

//------------------------------------------------------------------------------

int StateRestoreUniGameData (CFILE *cfp, int sgVersion, int bMulti, int bSecretRestore, fix xOldGameTime, int *nLevel)
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

bBetweenLevels = CFReadInt (cfp);
Assert (bBetweenLevels == 0);	//between levels save ripped out
// Read the mission info...
if (!StateLoadMission (cfp))
	return 0;
//Read level info
nCurrentLevel = CFReadInt (cfp);
nNextLevel = CFReadInt (cfp);
//Restore gameData.time.xGame
gameData.time.xGame = CFReadFix (cfp);
// Start new game....
StateRestoreMultiGame (szOrgCallSign, bMulti, bSecretRestore);
if (IsMultiGame) {
		char szServerCallSign [CALLSIGN_LEN + 1];

	strcpy (szServerCallSign, netPlayers.players [0].callsign);
	gameData.app.nStateGameId = CFReadInt (cfp);
	StateRestoreNetGame (cfp);
	DBG (fPos = CFTell (cfp));
	StateRestoreNetPlayers (cfp);
	DBG (fPos = CFTell (cfp));
	nPlayers = CFReadInt (cfp);
	nSavedLocalPlayer = gameData.multiplayer.nLocalPlayer;
	gameData.multiplayer.nLocalPlayer = CFReadInt (cfp);
	for (i = 0; i < nPlayers; i++) {
		StateRestorePlayer (restoredPlayers + i, cfp);
		restoredPlayers [i].connected = 0;
		}
	DBG (fPos = CFTell (cfp));
	// make sure the current game host is in tPlayer slot #0
	nServerPlayer = StateSetServerPlayer (restoredPlayers, nPlayers, szServerCallSign, &nOtherObjNum, &nServerObjNum);
	StateGetConnectedPlayers (restoredPlayers, nPlayers);
	}
//Read tPlayer info
if (!StartNewLevelSub (nCurrentLevel, 1, bSecretRestore, 1)) {
	CFClose (cfp);
	return 0;
	}
nLocalObjNum = LOCALPLAYER.nObject;
if (bSecretRestore != 1)	//either no secret restore, or tPlayer died in scret level
	StateRestorePlayer (gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer, cfp);
else {
	tPlayer	retPlayer;
	StateRestorePlayer (&retPlayer, cfp);
	StateAwardReturningPlayer (&retPlayer, xOldGameTime);
	}
LOCALPLAYER.nObject = nLocalObjNum;
strcpy (LOCALPLAYER.callsign, szOrgCallSign);
// Set the right level
if (bBetweenLevels)
	LOCALPLAYER.level = nNextLevel;
// Restore the weapon states
gameData.weapons.nPrimary = CFReadByte (cfp);
gameData.weapons.nSecondary = CFReadByte (cfp);
SelectWeapon (gameData.weapons.nPrimary, 0, 0, 0);
SelectWeapon (gameData.weapons.nSecondary, 1, 0, 0);
// Restore the difficulty level
gameStates.app.nDifficultyLevel = CFReadInt (cfp);
// Restore the cheats enabled flag
gameStates.app.cheats.bEnabled = CFReadInt (cfp);
for (i = 0; i < 2; i++) {
	if (sgVersion < 33) {
		gameStates.gameplay.slowmo [i].fSpeed = 1;
		gameStates.gameplay.slowmo [i].nState = 0;
		}
	else {
		gameStates.gameplay.slowmo [i].fSpeed = f2fl (CFReadInt (cfp));
		gameStates.gameplay.slowmo [i].nState = CFReadInt (cfp);
		}
	}
if (sgVersion > 33) {
	for (i = 0; i < MAX_PLAYERS; i++)
	   if (i != gameData.multiplayer.nLocalPlayer)
		   gameData.multiplayer.weaponStates [i].bTripleFusion = CFReadInt (cfp);
   	else {
   	   gameData.weapons.bTripleFusion = CFReadInt (cfp);
		   gameData.multiplayer.weaponStates [i].bTripleFusion = !gameData.weapons.bTripleFusion;  //force MultiSendWeapons
		   }
	}
if (!bBetweenLevels)	{
	gameStates.render.bDoAppearanceEffect = 0;			// Don't do this for middle o' game stuff.
	//Clear out all the objects from the lvl file
	memset (gameData.segs.objects, 0xff, gameData.segs.nSegments * sizeof (short));
	ResetObjects (1);

	//Read objects, and pop 'em into their respective segments.
	DBG (fPos = CFTell (cfp));
	h = CFReadInt (cfp);
	gameData.objs.nLastObject = h - 1;
	extraGameInfo [0].nBossCount = 0;
	for (i = 0; i < h; i++)
		StateRestoreObject (OBJECTS + i, cfp, sgVersion);
	DBG (fPos = CFTell (cfp));
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
	if (CFReadBoundedInt (MAX_WALLS, &gameData.walls.nWalls, cfp))
		return 0;
	for (i = 0, wallP = gameData.walls.walls; i < gameData.walls.nWalls; i++, wallP++) {
		StateRestoreWall (wallP, cfp);
		if (wallP->nType == WALL_OPEN)
			DigiKillSoundLinkedToSegment ((short) wallP->nSegment, (short) wallP->nSide, -1);	//-1 means kill any sound
		}
	DBG (fPos = CFTell (cfp));
	//Restore exploding wall info
	if (CFReadBoundedInt (MAX_EXPLODING_WALLS, &h, cfp))
		return 0;
	for (i = 0; i < h; i++)
		StateRestoreExplWall (gameData.walls.explWalls + i, cfp);
	DBG (fPos = CFTell (cfp));
	//Restore door info
	if (CFReadBoundedInt (MAX_DOORS, &gameData.walls.nOpenDoors, cfp))
		return 0;
	for (i = 0; i < gameData.walls.nOpenDoors; i++)
		StateRestoreActiveDoor (gameData.walls.activeDoors + i, cfp);
	DBG (fPos = CFTell (cfp));
	if (CFReadBoundedInt (MAX_WALLS, &gameData.walls.nCloaking, cfp))
		return 0;
	for (i = 0; i < gameData.walls.nCloaking; i++)
		StateRestoreCloakingWall (gameData.walls.cloaking + i, cfp);
	DBG (fPos = CFTell (cfp));
	//Restore tTrigger info
	if (CFReadBoundedInt (MAX_TRIGGERS, &gameData.trigs.nTriggers, cfp))
		return 0;
	for (i = 0; i < gameData.trigs.nTriggers; i++)
		StateRestoreTrigger (gameData.trigs.triggers + i, cfp);
	DBG (fPos = CFTell (cfp));
	//Restore tObject tTrigger info
	if (CFReadBoundedInt (MAX_TRIGGERS, &gameData.trigs.nObjTriggers, cfp))
		return 0;
	if (gameData.trigs.nObjTriggers > 0) {
		for (i = 0; i < gameData.trigs.nObjTriggers; i++)
			StateRestoreTrigger (gameData.trigs.objTriggers + i, cfp);
		for (i = 0; i < gameData.trigs.nObjTriggers; i++)
			StateRestoreObjTriggerRef (gameData.trigs.objTriggerRefs + i, cfp);
		if (sgVersion < 36) {
			j = (sgVersion < 35) ? 700 : MAX_OBJECTS_D2X;
			for (i = 0; i < j; i++)
				gameData.trigs.firstObjTrigger [i] = CFReadShort (cfp);
			}
		else {
			memset (gameData.trigs.firstObjTrigger, 0xff, sizeof (short) * MAX_OBJECTS_D2X);
			for (i = CFReadShort (cfp); i; i--) {
				j = CFReadShort (cfp);
				gameData.trigs.firstObjTrigger [j] = CFReadShort (cfp);
				}
			}
		}
	else if (sgVersion < 36)
		CFSeek (cfp, ((sgVersion < 35) ? 700 : MAX_OBJECTS_D2X) * sizeof (short), SEEK_CUR);
	else
		CFReadShort (cfp);
	DBG (fPos = CFTell (cfp));
	//Restore tmap info
	for (i = 0; i <= gameData.segs.nLastSegment; i++)	{
		for (j = 0; j < 6; j++)	{
			gameData.segs.segments [i].sides [j].nWall = CFReadShort (cfp);
			gameData.segs.segments [i].sides [j].nBaseTex = CFReadShort (cfp);
			nTexture = CFReadShort (cfp);
			gameData.segs.segments [i].sides [j].nOvlTex = nTexture & 0x3fff;
			gameData.segs.segments [i].sides [j].nOvlOrient = (nTexture >> 14) & 3;
			}
		}
	DBG (fPos = CFTell (cfp));
	//Restore the fuelcen info
	for (i = 0, wallP = gameData.walls.walls; i < gameData.walls.nWalls; i++, wallP++) {
		if ((wallP->nType == WALL_DOOR) && (wallP->flags & WALL_DOOR_OPENED))
			AnimateOpeningDoor (SEGMENTS + wallP->nSegment, wallP->nSide, -1);
		else if ((wallP->nType == WALL_BLASTABLE) && (wallP->flags & WALL_BLASTED))
			BlastBlastableWall (SEGMENTS + wallP->nSegment, wallP->nSide);
		}
	gameData.reactor.bDestroyed = CFReadInt (cfp);
	gameData.reactor.countdown.nTimer = CFReadFix (cfp);
	DBG (fPos = CFTell (cfp));
	if (CFReadBoundedInt (MAX_ROBOT_CENTERS, &gameData.matCens.nBotCenters, cfp))
		return 0;
	for (i = 0; i < gameData.matCens.nBotCenters; i++)
		StateRestoreMatCen (gameData.matCens.botGens + i, cfp);
	if (sgVersion >= 30) {
		if (CFReadBoundedInt (MAX_EQUIP_CENTERS, &gameData.matCens.nEquipCenters, cfp))
			return 0;
		for (i = 0; i < gameData.matCens.nEquipCenters; i++)
			StateRestoreMatCen (gameData.matCens.equipGens + i, cfp);
		}
	else {
		gameData.matCens.nBotCenters = 0;
		memset (gameData.matCens.botGens, 0, sizeof (gameData.matCens.botGens));
		}
	StateRestoreReactorTrigger (&gameData.reactor.triggers, cfp);
	if (CFReadBoundedInt (MAX_FUEL_CENTERS, &gameData.matCens.nFuelCenters, cfp))
		return 0;
	for (i = 0; i < gameData.matCens.nFuelCenters; i++)
		StateRestoreFuelCen (gameData.matCens.fuelCenters + i, cfp);
	DBG (fPos = CFTell (cfp));
	// Restore the control cen info
	if (sgVersion < 31) {
		gameData.reactor.states [0].bHit = CFReadInt (cfp);
		gameData.reactor.states [0].bSeenPlayer = CFReadInt (cfp);
		gameData.reactor.states [0].nNextFireTime = CFReadInt (cfp);
		gameData.reactor.bPresent = CFReadInt (cfp);
		gameData.reactor.states [0].nDeadObj = CFReadInt (cfp);
		}
	else {
		int	i;

		gameData.reactor.bPresent = CFReadInt (cfp);
		for (i = 0; i < MAX_BOSS_COUNT; i++) {
			gameData.reactor.states [i].nObject = CFReadInt (cfp);
			gameData.reactor.states [i].bHit = CFReadInt (cfp);
			gameData.reactor.states [i].bSeenPlayer = CFReadInt (cfp);
			gameData.reactor.states [i].nNextFireTime = CFReadInt (cfp);
			gameData.reactor.states [i].nDeadObj = CFReadInt (cfp);
			}
		}
	DBG (fPos = CFTell (cfp));
	// Restore the AI state
	AIRestoreUniState (cfp, sgVersion);
	// Restore the automap visited info
	DBG (fPos = CFTell (cfp));
	StateFixObjects ();
	SpecialResetObjects ();
	if (sgVersion > 37) {
		for (i = 0; i < MAX_SEGMENTS; i++)
			gameData.render.mine.bAutomapVisited [i] = (ushort) CFReadShort (cfp);
		}
	else {
		int	i, j = (sgVersion > 22) ? MAX_SEGMENTS : MAX_SEGMENTS_D2;
		for (i = 0; i < j; i++)
			gameData.render.mine.bAutomapVisited [i] = (ushort) CFReadByte (cfp);
		}
	DBG (fPos = CFTell (cfp));
	//	Restore hacked up weapon system stuff.
	gameData.fusion.xNextSoundTime = gameData.time.xGame;
	gameData.fusion.xAutoFireTime = 0;
	gameData.laser.xNextFireTime = 
	gameData.missiles.xNextFireTime = gameData.time.xGame;
	gameData.laser.xLastFiredTime = 
	gameData.missiles.xLastFiredTime = gameData.time.xGame;
	}
gameData.app.nStateGameId = 0;
gameData.app.nStateGameId = (uint) CFReadInt (cfp);
gameStates.app.cheats.bLaserRapidFire = CFReadInt (cfp);
gameStates.app.bLunacy = CFReadInt (cfp);		//	Yes, reading this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
gameStates.app.bLunacy = CFReadInt (cfp);
if (gameStates.app.bLunacy)
	DoLunacyOn ();

CFRead (gameData.marker.objects, sizeof (gameData.marker.objects), 1, cfp);
CFRead (gameData.marker.nOwner, sizeof (gameData.marker.nOwner), 1, cfp);
CFRead (gameData.marker.szMessage, sizeof (gameData.marker.szMessage), 1, cfp);

if (bSecretRestore != 1)
	gameData.physics.xAfterburnerCharge = CFReadFix (cfp);
else {
	CFReadFix (cfp);
	}
//read last was super information
CFRead (&bLastPrimaryWasSuper, sizeof (bLastPrimaryWasSuper), 1, cfp);
CFRead (&bLastSecondaryWasSuper, sizeof (bLastSecondaryWasSuper), 1, cfp);
gameData.render.xFlashEffect = CFReadFix (cfp);
gameData.render.xTimeFlashLastPlayed = CFReadFix (cfp);
gameStates.ogl.palAdd.red = CFReadShort (cfp);
gameStates.ogl.palAdd.green = CFReadShort (cfp);
gameStates.ogl.palAdd.blue = CFReadShort (cfp);
CFRead (gameData.render.lights.subtracted, 
		  sizeof (gameData.render.lights.subtracted [0]), 
		  (sgVersion > 22) ? MAX_SEGMENTS : MAX_SEGMENTS_D2, cfp);
ApplyAllChangedLight ();
if (bSecretRestore) 
	gameStates.app.bFirstSecretVisit = 0;
else
	gameStates.app.bFirstSecretVisit = CFReadInt (cfp);

if (bSecretRestore != 1)
	gameData.omega.xCharge [0] = 
	gameData.omega.xCharge [1] = CFReadFix (cfp);
else
	CFReadFix (cfp);
if (sgVersion > 27)
	gameData.missions.nEnteredFromLevel = CFReadShort (cfp);
*nLevel = nCurrentLevel;
if (sgVersion >= 37)
	for (i = 0; i < MAX_PLAYERS; i++)
		StateRestoreSpawnPoint (i, cfp);
return 1;
}

//------------------------------------------------------------------------------

int StateRestoreBinGameData (CFILE *cfp, int sgVersion, int bMulti, int bSecretRestore, fix xOldGameTime, int *nLevel)
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

CFRead (&bBetweenLevels, sizeof (int), 1, cfp);
Assert (bBetweenLevels == 0);	//between levels save ripped out
// Read the mission info...
if (!StateLoadMission (cfp))
	return 0;
//Read level info
CFRead (&nCurrentLevel, sizeof (int), 1, cfp);
CFRead (&nNextLevel, sizeof (int), 1, cfp);
//Restore gameData.time.xGame
CFRead (&gameData.time.xGame, sizeof (fix), 1, cfp);
// Start new game....
StateRestoreMultiGame (szOrgCallSign, bMulti, bSecretRestore);
if (gameData.app.nGameMode & GM_MULTI) {
		char szServerCallSign [CALLSIGN_LEN + 1];

	strcpy (szServerCallSign, netPlayers.players [0].callsign);
	CFRead (&gameData.app.nStateGameId, sizeof (int), 1, cfp);
	CFRead (&netGame, sizeof (tNetgameInfo), 1, cfp);
	CFRead (&netPlayers, sizeof (tAllNetPlayersInfo), 1, cfp);
	CFRead (&nPlayers, sizeof (gameData.multiplayer.nPlayers), 1, cfp);
	CFRead (&gameData.multiplayer.nLocalPlayer, sizeof (gameData.multiplayer.nLocalPlayer), 1, cfp);
	nSavedLocalPlayer = gameData.multiplayer.nLocalPlayer;
	for (i = 0; i < nPlayers; i++)
		CFRead (restoredPlayers + i, sizeof (tPlayer), 1, cfp);
	nServerPlayer = StateSetServerPlayer (restoredPlayers, nPlayers, szServerCallSign, &nOtherObjNum, &nServerObjNum);
	StateGetConnectedPlayers (restoredPlayers, nPlayers);
	}

//Read tPlayer info
if (!StartNewLevelSub (nCurrentLevel, 1, bSecretRestore, 1)) {
	CFClose (cfp);
	return 0;
	}
nLocalObjNum = LOCALPLAYER.nObject;
if (bSecretRestore != 1)	//either no secret restore, or tPlayer died in scret level
	CFRead (gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer, sizeof (tPlayer), 1, cfp);
else {
	tPlayer	retPlayer;
	CFRead (&retPlayer, sizeof (tPlayer), 1, cfp);
	StateAwardReturningPlayer (&retPlayer, xOldGameTime);
	}
LOCALPLAYER.nObject = nLocalObjNum;
strcpy (LOCALPLAYER.callsign, szOrgCallSign);
// Set the right level
if (bBetweenLevels)
	LOCALPLAYER.level = nNextLevel;
// Restore the weapon states
CFRead (&gameData.weapons.nPrimary, sizeof (sbyte), 1, cfp);
CFRead (&gameData.weapons.nSecondary, sizeof (sbyte), 1, cfp);
SelectWeapon (gameData.weapons.nPrimary, 0, 0, 0);
SelectWeapon (gameData.weapons.nSecondary, 1, 0, 0);
// Restore the difficulty level
CFRead (&gameStates.app.nDifficultyLevel, sizeof (int), 1, cfp);
// Restore the cheats enabled flag
CFRead (&gameStates.app.cheats.bEnabled, sizeof (int), 1, cfp);
if (!bBetweenLevels)	{
	gameStates.render.bDoAppearanceEffect = 0;			// Don't do this for middle o' game stuff.
	//Clear out all the OBJECTS from the lvl file
	memset (gameData.segs.objects, 0xff, gameData.segs.nSegments * sizeof (short));
	ResetObjects (1);
	//Read objects, and pop 'em into their respective segments.
	CFRead (&i, sizeof (int), 1, cfp);
	gameData.objs.nLastObject = i - 1;
	CFRead (OBJECTS, sizeof (tObject), i, cfp);
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
	if (CFReadBoundedInt (MAX_WALLS, &gameData.walls.nWalls, cfp))
		return 0;
	CFRead (gameData.walls.walls, sizeof (tWall), gameData.walls.nWalls, cfp);
	//now that we have the walls, check if any sounds are linked to
	//walls that are now open
	for (i = 0, wallP = gameData.walls.walls; i < gameData.walls.nWalls; i++, wallP++)
		if (wallP->nType == WALL_OPEN)
			DigiKillSoundLinkedToSegment ((short) wallP->nSegment, (short) wallP->nSide, -1);	//-1 means kill any sound
	//Restore exploding wall info
	if (sgVersion >= 10) {
		CFRead (&i, sizeof (int), 1, cfp);
		CFRead (gameData.walls.explWalls, sizeof (*gameData.walls.explWalls), i, cfp);
		}
	//Restore door info
	if (CFReadBoundedInt (MAX_DOORS, &gameData.walls.nOpenDoors, cfp))
		return 0;
	CFRead (gameData.walls.activeDoors, sizeof (tActiveDoor), gameData.walls.nOpenDoors, cfp);
	if (sgVersion >= 14) {		//Restore cloaking tWall info
		if (CFReadBoundedInt (MAX_WALLS, &gameData.walls.nCloaking, cfp))
			return 0;
		CFRead (gameData.walls.cloaking, sizeof (tCloakingWall), gameData.walls.nCloaking, cfp);
		}
	//Restore tTrigger info
	if (CFReadBoundedInt (MAX_TRIGGERS, &gameData.trigs.nTriggers, cfp))
		return 0;
	CFRead (gameData.trigs.triggers, sizeof (tTrigger), gameData.trigs.nTriggers, cfp);
	if (sgVersion >= 26) {
		//Restore tObject tTrigger info

		CFRead (&gameData.trigs.nObjTriggers, sizeof (gameData.trigs.nObjTriggers), 1, cfp);
		if (gameData.trigs.nObjTriggers > 0) {
			CFRead (gameData.trigs.objTriggers, sizeof (tTrigger), gameData.trigs.nObjTriggers, cfp);
			CFRead (gameData.trigs.objTriggerRefs, sizeof (tObjTriggerRef), gameData.trigs.nObjTriggers, cfp);
			CFRead (gameData.trigs.firstObjTrigger, sizeof (short), 700, cfp);
			}
		else
			CFSeek (cfp, (sgVersion < 35) ? 700 : MAX_OBJECTS_D2X * sizeof (short), SEEK_CUR);
		}
	//Restore tmap info
	for (i = 0; i <= gameData.segs.nLastSegment; i++)	{
		for (j = 0; j < 6; j++)	{
			gameData.segs.segments [i].sides [j].nWall = CFReadShort (cfp);
			gameData.segs.segments [i].sides [j].nBaseTex = CFReadShort (cfp);
			nTexture = CFReadShort (cfp);
			gameData.segs.segments [i].sides [j].nOvlTex = nTexture & 0x3fff;
			gameData.segs.segments [i].sides [j].nOvlOrient = (nTexture >> 14) & 3;
			}
		}
//Restore the fuelcen info
	gameData.reactor.bDestroyed = CFReadInt (cfp);
	gameData.reactor.countdown.nTimer = CFReadFix (cfp);
	if (CFReadBoundedInt (MAX_ROBOT_CENTERS, &gameData.matCens.nBotCenters, cfp))
		return 0;
	for (i = 0; i < gameData.matCens.nBotCenters; i++) {
		CFRead (gameData.matCens.botGens [i].objFlags, sizeof (int), 2, cfp);
		CFRead (&gameData.matCens.botGens [i].xHitPoints, sizeof (tMatCenInfo) - ((char *) &gameData.matCens.botGens [i].xHitPoints - (char *) &gameData.matCens.botGens [i]), 1, cfp);
		}
	CFRead (&gameData.reactor.triggers, sizeof (tReactorTriggers), 1, cfp);
	if (CFReadBoundedInt (MAX_FUEL_CENTERS, &gameData.matCens.nFuelCenters, cfp))
		return 0;
	CFRead (gameData.matCens.fuelCenters, sizeof (tFuelCenInfo), gameData.matCens.nFuelCenters, cfp);

	// Restore the control cen info
	gameData.reactor.states [0].bHit = CFReadInt (cfp);
	gameData.reactor.states [0].bSeenPlayer = CFReadInt (cfp);
	gameData.reactor.states [0].nNextFireTime = CFReadInt (cfp);
	gameData.reactor.bPresent = CFReadInt (cfp);
	gameData.reactor.states [0].nDeadObj = CFReadInt (cfp);
	// Restore the AI state
	AIRestoreBinState (cfp, sgVersion);
	// Restore the automap visited info
	if (sgVersion > 37)
		CFRead (gameData.render.mine.bAutomapVisited, sizeof (ushort), MAX_SEGMENTS, cfp);
	else {
		int	i, j = (sgVersion > 22) ? MAX_SEGMENTS : MAX_SEGMENTS_D2;
		for (i = 0; i < j; i++)
			gameData.render.mine.bAutomapVisited [i] = (ushort) CFReadByte (cfp);
		}
	
	CFRead (gameData.render.mine.bAutomapVisited, sizeof (ubyte), (sgVersion > 22) ? MAX_SEGMENTS : MAX_SEGMENTS_D2, cfp);

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
	CFRead (&gameData.app.nStateGameId, sizeof (uint), 1, cfp);
	CFRead (&gameStates.app.cheats.bLaserRapidFire, sizeof (int), 1, cfp);
	CFRead (&gameStates.app.bLunacy, sizeof (int), 1, cfp);		//	Yes, writing this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
	CFRead (&gameStates.app.bLunacy, sizeof (int), 1, cfp);
	if (gameStates.app.bLunacy)
		DoLunacyOn ();
}

if (sgVersion >= 17) {
	CFRead (gameData.marker.objects, sizeof (gameData.marker.objects), 1, cfp);
	CFRead (gameData.marker.nOwner, sizeof (gameData.marker.nOwner), 1, cfp);
	CFRead (gameData.marker.szMessage, sizeof (gameData.marker.szMessage), 1, cfp);
}
else {
	int num,dummy;

	// skip dummy info
	CFRead (&num, sizeof (int), 1, cfp);       //was NumOfMarkers
	CFRead (&dummy, sizeof (int), 1, cfp);     //was CurMarker
	CFSeek (cfp, num * (sizeof (vmsVector) + 40), SEEK_CUR);
	for (num = 0; num < NUM_MARKERS; num++)
		gameData.marker.objects [num] = -1;
}

if (sgVersion >= 11) {
	if (bSecretRestore != 1)
		CFRead (&gameData.physics.xAfterburnerCharge, sizeof (fix), 1, cfp);
	else {
		fix	dummy_fix;
		CFRead (&dummy_fix, sizeof (fix), 1, cfp);
	}
}
if (sgVersion >= 12) {
	//read last was super information
	CFRead (&bLastPrimaryWasSuper, sizeof (bLastPrimaryWasSuper), 1, cfp);
	CFRead (&bLastSecondaryWasSuper, sizeof (bLastSecondaryWasSuper), 1, cfp);
	CFRead (&gameData.render.xFlashEffect, sizeof (int), 1, cfp);
	CFRead (&gameData.render.xTimeFlashLastPlayed, sizeof (int), 1, cfp);
	CFRead (&gameStates.ogl.palAdd.red, sizeof (int), 1, cfp);
	CFRead (&gameStates.ogl.palAdd.green, sizeof (int), 1, cfp);
	CFRead (&gameStates.ogl.palAdd.blue, sizeof (int), 1, cfp);
	}
else {
	ResetPaletteAdd ();
	}

//	Load gameData.render.lights.subtracted
if (sgVersion >= 16) {
	CFRead (gameData.render.lights.subtracted, sizeof (gameData.render.lights.subtracted [0]), (sgVersion > 22) ? MAX_SEGMENTS : MAX_SEGMENTS_D2, cfp);
	ApplyAllChangedLight ();
	//ComputeAllStaticLight ();	//	set xAvgSegLight field in tSegment struct.  See note at that function.
	}
else
	memset (gameData.render.lights.subtracted, 0, sizeof (gameData.render.lights.subtracted));

if (bSecretRestore) 
	gameStates.app.bFirstSecretVisit = 0;
else if (sgVersion >= 20)
	CFRead (&gameStates.app.bFirstSecretVisit, sizeof (gameStates.app.bFirstSecretVisit), 1, cfp);
else
	gameStates.app.bFirstSecretVisit = 1;

if (sgVersion >= 22) {
	if (bSecretRestore != 1)
		CFRead (&gameData.omega.xCharge, sizeof (fix), 1, cfp);
	else {
		fix	dummy_fix;
		CFRead (&dummy_fix, sizeof (fix), 1, cfp);
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
InitReactorForLevel (1);
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
gameData.objs.viewer = 
gameData.objs.console = OBJECTS + LOCALPLAYER.nObject;
StartTime (1);
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

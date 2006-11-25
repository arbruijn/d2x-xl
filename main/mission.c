/* $Id: mission.c,v 1.24 2003/11/04 08:03:08 btb Exp $ */
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
#include <ctype.h>
#include <limits.h>

#include "pstypes.h"
#include "cfile.h"

#include "strutil.h"
#include "inferno.h"
#include "mission.h"
#include "gameseq.h"
#include "titles.h"
#include "songs.h"
#include "mono.h"
#include "error.h"
#include "findfile.h"
#include "polyobj.h"
#include "robot.h"
#include "newmenu.h"
#include "text.h"
#ifndef __findfile_h
#	error filefind missing!
#endif

//this stuff should get defined elsewhere

//values for d1 built-in mission
#define BIM_LAST_LEVEL          27
#define BIM_LAST_SECRET_LEVEL   -3
#define BIM_BRIEFING_FILE       "briefing.tex"
#define BIM_ENDING_FILE         "endreg.tex"

//------------------------------------------------------------------------------
//
//  Special versions of mission routines for d1 builtins
//

int load_mission_d1(int mission_num)
{
	int i;

	CFUseD1HogFile("descent.hog");

	gameData.missions.nCurrentMission = mission_num;
	gameStates.app.szCurrentMissionFile = gameData.missions.list [mission_num].filename;
	gameStates.app.szCurrentMission = gameData.missions.list [mission_num].mission_name;

	switch (gameData.missions.nD1BuiltinHogSize) {
	case D1_SHAREWARE_MISSION_HOGSIZE:
	case D1_SHAREWARE_10_MISSION_HOGSIZE:
		gameData.missions.nSecretLevels = 0;

		gameData.missions.nLastLevel = 7;
		gameData.missions.nLastSecretLevel = 0;

		//build level names
		for (i=0;i<gameData.missions.nLastLevel;i++)
			sprintf(gameData.missions.szLevelNames [i], "level%02d.sdl", i+1);

		break;
	case D1_MAC_SHARE_MISSION_HOGSIZE:
		gameData.missions.nSecretLevels = 0;

		gameData.missions.nLastLevel = 3;
		gameData.missions.nLastSecretLevel = 0;

		//build level names
		for (i=0;i<gameData.missions.nLastLevel;i++)
			sprintf(gameData.missions.szLevelNames [i], "level%02d.sdl", i+1);

		break;
	case D1_OEM_MISSION_HOGSIZE:
	case D1_OEM_10_MISSION_HOGSIZE:
		gameData.missions.nSecretLevels = 1;

		gameData.missions.nLastLevel = 15;
		gameData.missions.nLastSecretLevel = -1;

		//build level names
		for (i=0; i < gameData.missions.nLastLevel - 1; i++)
			sprintf(gameData.missions.szLevelNames [i], "level%02d.rdl", i+1);
		sprintf(gameData.missions.szLevelNames [i], "saturn%02d.rdl", i+1);
		for (i=0; i < -gameData.missions.nLastSecretLevel; i++)
			sprintf(gameData.missions.szSecretLevelNames [i], "levels%1d.rdl", i+1);

		gameData.missions.secretLevelTable [0] = 10;

		break;
	default:
		Int3(); // fall through
	case D1_MISSION_HOGSIZE:
	case D1_10_MISSION_HOGSIZE:
	case D1_15_MISSION_HOGSIZE:
	case D1_3DFX_MISSION_HOGSIZE:
	case D1_MAC_MISSION_HOGSIZE:
		gameData.missions.nSecretLevels = 3;

		gameData.missions.nLastLevel = BIM_LAST_LEVEL;
		gameData.missions.nLastSecretLevel = BIM_LAST_SECRET_LEVEL;

		//build level names
		for (i=0;i<gameData.missions.nLastLevel;i++)
			sprintf(gameData.missions.szLevelNames [i], "level%02d.rdl", i+1);
		for (i=0;i<-gameData.missions.nLastSecretLevel;i++)
			sprintf(gameData.missions.szSecretLevelNames [i], "levels%1d.rdl", i+1);

		gameData.missions.secretLevelTable [0] = 10;
		gameData.missions.secretLevelTable [1] = 21;
		gameData.missions.secretLevelTable [2] = 24;

		break;
	}
	strcpy(szBriefingTextFilename,BIM_BRIEFING_FILE);
	strcpy(szEndingTextFilename,BIM_ENDING_FILE);
	gameStates.app.bD1Mission = 1;
	return 1;
}


//------------------------------------------------------------------------------
//
//  Special versions of mission routines for shareware
//

int load_mission_shareware(int mission_num)
{
	gameData.missions.nCurrentMission = mission_num;
	gameStates.app.szCurrentMissionFile = gameData.missions.list [mission_num].filename;
	gameStates.app.szCurrentMission = gameData.missions.list [mission_num].mission_name;

	switch (gameData.missions.nBuiltinHogSize) {
	case MAC_SHARE_MISSION_HOGSIZE:
		gameData.missions.nSecretLevels = 1;

		gameData.missions.nLastLevel = 4;
		gameData.missions.nLastSecretLevel = -1;

		// mac demo is using the regular hog and rl2 files
		strcpy(gameData.missions.szLevelNames [0],"d2leva-1.rl2");
		strcpy(gameData.missions.szLevelNames [1],"d2leva-2.rl2");
		strcpy(gameData.missions.szLevelNames [2],"d2leva-3.rl2");
		strcpy(gameData.missions.szLevelNames [3],"d2leva-4.rl2");
		strcpy(gameData.missions.szSecretLevelNames [0],"d2leva-s.rl2");
		break;
	default:
		Int3(); // fall through
	case SHAREWARE_MISSION_HOGSIZE:
		gameData.missions.nSecretLevels = 0;

		gameData.missions.nLastLevel = 3;
		gameData.missions.nLastSecretLevel = 0;

		strcpy(gameData.missions.szLevelNames [0],"d2leva-1.sl2");
		strcpy(gameData.missions.szLevelNames [1],"d2leva-2.sl2");
		strcpy(gameData.missions.szLevelNames [2],"d2leva-3.sl2");
	}

	return 1;
}


//------------------------------------------------------------------------------
//
//  Special versions of mission routines for Diamond/S3 version
//

int load_mission_oem(int mission_num)
{
gameData.missions.nCurrentMission = mission_num;
gameStates.app.szCurrentMissionFile = gameData.missions.list [mission_num].filename;
gameStates.app.szCurrentMission = gameData.missions.list [mission_num].mission_name;
gameData.missions.nSecretLevels = 2;
gameData.missions.nLastLevel = 8;
gameData.missions.nLastSecretLevel = -2;
strcpy(gameData.missions.szLevelNames [0],"d2leva-1.rl2");
strcpy(gameData.missions.szLevelNames [1],"d2leva-2.rl2");
strcpy(gameData.missions.szLevelNames [2],"d2leva-3.rl2");
strcpy(gameData.missions.szLevelNames [3],"d2leva-4.rl2");
strcpy(gameData.missions.szSecretLevelNames [0],"d2leva-s.rl2");
strcpy(gameData.missions.szLevelNames [4],"d2levb-1.rl2");
strcpy(gameData.missions.szLevelNames [5],"d2levb-2.rl2");
strcpy(gameData.missions.szLevelNames [6],"d2levb-3.rl2");
strcpy(gameData.missions.szLevelNames [7],"d2levb-4.rl2");
strcpy(gameData.missions.szSecretLevelNames [1],"d2levb-s.rl2");
gameData.missions.secretLevelTable [0] = 1;
gameData.missions.secretLevelTable [1] = 5;
return 1;
}


//------------------------------------------------------------------------------
//strips damn newline from end of line
char *MsnGetS (char *s, int n, CFILE *fp)
{
	char *r;
	int	l;

r = CFGetS (s, n, fp);
if (r) {
	l = (int) strlen (s) - 1;
	if (s [l] == '\n')
		s [l] = 0;
	}
return r;
}

//------------------------------------------------------------------------------
//compare a string for a token. returns true if match
int MsnIsTok (char *buf,char *tok)
{
return !strnicmp (buf, tok, strlen (tok));
}

//------------------------------------------------------------------------------
//adds a terminating 0 after a string at the first white space
void MsnAddStrTerm (char *s)
{
while (*s && (*s != '\r') && (*s != '\n'))
	s++;
*s = 0;		//terminate!
}

//------------------------------------------------------------------------------
//returns ptr to string after '=' & white space, or NULL if no '='
//adds 0 after parm at first white space
char *MsnGetValue (char *buf)
{
char *t = strchr(buf, '=');

if (t) {
	t++;
	while (*t && isspace (*t)) 
		t++;
	if (*t)
		return t;
	}
return NULL;		//error!
}

//------------------------------------------------------------------------------
//reads a line, returns ptr to value of passed parm.  returns NULL if none
char *GetParmValue (char *parm,CFILE *f)
{
	static char buf [80];

if (!MsnGetS(buf,80,f))
	return NULL;
if (MsnIsTok (buf,parm))
	return MsnGetValue(buf);
else
	return NULL;
}

//------------------------------------------------------------------------------

int _CDECL_ ml_sort_func (mle *e0, mle *e1)
{
	char	*s1 = e0->mission_name,
			*s2 = e1->mission_name;

if (gameOpts->menus.bShowLevelVersion) {
	if (!(strncmp (s1, "[2] ", 4) && strncmp (s1, "[1] ", 4)))
		s1 += 4;
	if (!(strncmp (s2, "[2] ", 4) && strncmp (s2, "[1] ", 4)))
		s2 += 4;
	}
if ((*s1 == '[') == (*s2 == '['))
	return stricmp (s1, s2);
return (*s1 == '[') ? -1 : 1;
}

//------------------------------------------------------------------------------

extern char CDROM_dir [];

//returns 1 if file read ok, else 0
int ReadMissionFile (char *filename, int count, int location)
{
	char	filename2 [100];
	CFILE *fp;
	char	*p, temp [FILENAME_LEN],*t;

switch (location) {
	case ML_MISSIONDIR:
	case ML_ALTHOGDIR:
		strcpy(filename2, gameFolders.szMissionDirs [location]);
		break;

	case ML_CDROM:
		songs_stop_redbook();		//so we can read from the CD
		strcpy(filename2,CDROM_dir);
		break;

	default:
		Int3();		//fall through

	case ML_CURDIR:
		strcpy(filename2,"");
		break;
}
strcat(filename2, filename);
fp = CFOpen (filename2, "", "rb", 0);
if (!fp)
	return 0;

strcpy(temp,filename);
if ((t = strchr(temp, '.')) == NULL)
	return 0;	//missing extension
gameData.missions.list [count].descent_version = (t [3] == '2') ? 2 : 1;
*t = '\0';
// look if it's .mn2 or .msn
strcpy( gameData.missions.list [count].filename, temp);
gameData.missions.list [count].bAnarchyOnly = 0;
gameData.missions.list [count].location = location;

p = GetParmValue("name",fp);
if (!p) {		//try enhanced mission
	CFSeek(fp,0,SEEK_SET);
	p = GetParmValue ("xname", fp);
	}
if (!p) {       //try super-enhanced mission!
	CFSeek(fp,0,SEEK_SET);
	p = GetParmValue( "zname", fp);
	}
if (!p && (gameStates.app.bNostalgia < 3)) {       //try super-enhanced mission!
	CFSeek(fp,0,SEEK_SET);
	p = GetParmValue ("d2x-name", fp);
	}

if (p) {
	char *t;
	if (t = strchr (p, ';'))
		*t-- = 0;
	else
		t = p + strlen (p) - 1;
	while (isspace (*t) || (t - p > MISSION_NAME_LEN)) 
		*t-- = '\0';
	if (gameOpts->menus.bShowLevelVersion) {
		sprintf (gameData.missions.list [count].mission_name, "[%d] %s", gameData.missions.list [count].descent_version, p);
		strcat (temp, filename);
		}
	else
		strcpy(gameData.missions.list [count].mission_name,p);
	}
else {
	CFClose(fp);
	return 0;
	}
p = GetParmValue("nType",fp);
//get mission nType
if (p)
	gameData.missions.list [count].bAnarchyOnly = MsnIsTok (p,"anarchy");
CFClose(fp);
return 1;
}

//------------------------------------------------------------------------------

void add_d1_builtin_mission_to_list(int *count)
{
	if (!CFExist("descent.hog", gameFolders.szDataDir, 1))
		return;

	gameData.missions.nD1BuiltinHogSize = CFSize("descent.hog",gameFolders.szDataDir, 0);

	switch (gameData.missions.nD1BuiltinHogSize) {
	case D1_SHAREWARE_MISSION_HOGSIZE:
	case D1_SHAREWARE_10_MISSION_HOGSIZE:
	case D1_MAC_SHARE_MISSION_HOGSIZE:
		strcpy(gameData.missions.list [*count].filename, D1_MISSION_FILENAME);
		strcpy(gameData.missions.list [*count].mission_name, D1_SHAREWARE_MISSION_NAME);
		gameData.missions.list [*count].bAnarchyOnly = 0;
		break;
	case D1_OEM_MISSION_HOGSIZE:
	case D1_OEM_10_MISSION_HOGSIZE:
		strcpy(gameData.missions.list [*count].filename, D1_MISSION_FILENAME);
		strcpy(gameData.missions.list [*count].mission_name, D1_OEM_MISSION_NAME);
		gameData.missions.list [*count].bAnarchyOnly = 0;
		break;
	default:
#ifdef _DEBUG
		//Warning(TXT_D1_HOGSIZE, gameData.missions.nD1BuiltinHogSize);
		Int3();
#endif
		// fall through
	case D1_MISSION_HOGSIZE:
	case D1_10_MISSION_HOGSIZE:
	case D1_15_MISSION_HOGSIZE:
	case D1_3DFX_MISSION_HOGSIZE:
	case D1_MAC_MISSION_HOGSIZE:
		strcpy(gameData.missions.list [*count].filename, D1_MISSION_FILENAME);
		if (gameOpts->menus.bShowLevelVersion)
			strcpy(gameData.missions.list [*count].mission_name, "[1] " D1_MISSION_NAME);
		else
			strcpy(gameData.missions.list [*count].mission_name, D1_MISSION_NAME);
		gameData.missions.list [*count].bAnarchyOnly = 0;
		gameData.missions.list [*count].location = ML_MISSIONDIR;
		break;
	}

	strcpy(gameData.missions.szD1BuiltinMissionFilename, gameData.missions.list [*count].filename);
	gameData.missions.list [*count].descent_version = 1;
	gameData.missions.list [*count].bAnarchyOnly = 0;
	gameData.missions.list [*count].location = ML_DATADIR;
	++(*count);
}

//------------------------------------------------------------------------------

void add_d2x_builtin_mission_to_list(int *count)
{
if (CFExist ("d2x.hog", gameFolders.szMissionDir, 0)) {
	strcpy(gameData.missions.list [*count].filename,"d2x");
	if (gameOpts->menus.bShowLevelVersion)
		strcpy (gameData.missions.list [*count].mission_name,"[2] Descent 2: Vertigo");
	else
		strcpy (gameData.missions.list [*count].mission_name,"Descent 2: Vertigo");
	gameData.missions.list [*count].descent_version = 2;
	gameData.missions.list [*count].bAnarchyOnly = 0;
	gameData.missions.list [*count].location = ML_MSNROOTDIR;
	++(*count);
	}
}

//------------------------------------------------------------------------------

void add_builtin_mission_to_list(int *count)
{
gameData.missions.nBuiltinHogSize = CFSize("descent2.hog", gameFolders.szDataDir, 0);
if (gameData.missions.nBuiltinHogSize == -1)
	gameData.missions.nBuiltinHogSize = CFSize("d2demo.hog", gameFolders.szDataDir, 0);

retry:

switch (gameData.missions.nBuiltinHogSize) {
	case SHAREWARE_MISSION_HOGSIZE:
	case MAC_SHARE_MISSION_HOGSIZE:
		strcpy(gameData.missions.list [*count].filename,SHAREWARE_MISSION_FILENAME);
		strcpy(gameData.missions.list [*count].mission_name,SHAREWARE_MISSION_NAME);
		gameData.missions.list [*count].bAnarchyOnly = 0;
		break;
	case OEM_MISSION_HOGSIZE:
		strcpy(gameData.missions.list [*count].filename,OEM_MISSION_FILENAME);
		strcpy(gameData.missions.list [*count].mission_name,OEM_MISSION_NAME);
		gameData.missions.list [*count].bAnarchyOnly = 0;
		break;
	case FULL_MISSION_HOGSIZE:
	case FULL_10_MISSION_HOGSIZE:
	case MAC_FULL_MISSION_HOGSIZE:
		if (!ReadMissionFile(FULL_MISSION_FILENAME ".mn2", 0, ML_CURDIR))
			Error("Could not find required mission file <%s>", FULL_MISSION_FILENAME ".mn2");
		break;
	default:
#if 0//def _DEBUG
		Warning(TXT_HOGSIZE, gameData.missions.nBuiltinHogSize, FULL_MISSION_FILENAME ".mn2");
#endif
		gameData.missions.nBuiltinHogSize = FULL_MISSION_HOGSIZE;
		Int3(); //fall through
		goto retry;
	}
strcpy(gameData.missions.szBuiltinMissionFilename, gameData.missions.list [*count].filename);
gameData.missions.list [*count].descent_version = 2;
gameData.missions.list [*count].bAnarchyOnly = 0;
gameData.missions.list [*count].location = ML_DATADIR;
++(*count);
}

//------------------------------------------------------------------------------

int nBuiltIns = 0;

void AddMissionsToList 
	(int *count, int anarchy_mode, int bD1Mission, int bSubFolder, int bHaveSubFolders, int nLocation)
{
	FFS	ffs;
	int 	c = *count;
	char lvlName [255];
	char searchName [255];
	char *lvlExt = ".hog";
	char *altLvlExt [2] = {".rdl", ".rl2"};
	int	bFindDirs;

#ifdef _WIN32
	static char *pszExt [2][2] = {{"*.mn2", "*.msn"},{"*", "*"}};
#else
	static char *pszExt [2][2] = {{"*.mn2", "*.msn"},{"*", "*"}};
#endif

	if (c + bSubFolder >= MAX_MISSIONS) 
		return;
	if (bSubFolder && !bHaveSubFolders && !nLocation) {
		gameData.missions.list [c].descent_version = 0;	//directory
		*gameData.missions.list [c].filename = '\0';
		strcpy (gameData.missions.list [c].mission_name, "[..]");
		c++;
		}
	for (bFindDirs = !bHaveSubFolders && (c == nBuiltIns + bSubFolder); bFindDirs >= 0; bFindDirs--) {
		sprintf (searchName, "%s%s", gameFolders.szMissionDirs [nLocation], pszExt [bFindDirs][bD1Mission]);
		if (!FFF (searchName, &ffs, bFindDirs)) {
			do	{
				if (!(strcmp (ffs.name, ".") && strcmp (ffs.name, "..")))
					continue;
				if (bFindDirs) {
						gameData.missions.list [c].descent_version = 0;	//directory
						*gameData.missions.list [c].filename = '\0';
						sprintf (gameData.missions.list [c].mission_name, "[%s]", ffs.name);
						c++;
						}
				else {
					sprintf (lvlName, "%s%s", gameFolders.szMissionDirs [nLocation], ffs.name);
					memcpy (lvlName + strlen (lvlName) - 4, lvlExt, sizeof (lvlExt));
					if (CFExist (lvlName, "", 0))
						memcpy (lvlName + strlen (lvlName) - 4, altLvlExt [bD1Mission], sizeof (altLvlExt [bD1Mission]));
					if (!CFExist (lvlName, "", 0)) {
						if (ReadMissionFile (ffs.name, c, nLocation) &&
							 (anarchy_mode || !gameData.missions.list [c].bAnarchyOnly))
							c++;
						}
					}
			} while ((c < MAX_MISSIONS) && !FFN (&ffs, bFindDirs));
			FFC (&ffs);
			if (c >= MAX_MISSIONS) {
				gameData.missions.list [c].descent_version = 0;	//directory
				strcpy (gameData.missions.list [c].filename, TXT_MSN_OVERFLOW);

	#if TRACE
				con_printf (CON_DEBUG, "Warning: more missions than D2X-XL can handle\n");
	#endif			
			}
		}
	*count = c;
	}
}

//------------------------------------------------------------------------------
/* move <mission_name> to <place> on mission list, increment <place> */
void promote (char * mission_name, int * top_place, int num_missions)
{
	int i;
	char name [FILENAME_LEN], * t;
	strcpy(name, mission_name);
	if ((t = strchr(name,'.')) != NULL)
		*t = 0; //kill extension
	////printf("promoting: %s\n", name);
	for (i = *top_place; i < num_missions; i++)
		if (!stricmp(gameData.missions.list [i].filename, name)) {
			//swap mission positions
			mle temp;

			temp = gameData.missions.list [*top_place];
			gameData.missions.list [*top_place] = gameData.missions.list [i];
			gameData.missions.list [i] = temp;
			++(*top_place);
			break;
		}
}

//------------------------------------------------------------------------------
//fills in the global list of missions.  Returns the number of missions
//in the list.  If anarchy_mode set, don't include non-anarchy levels.

extern char CDROM_dir [];

void MoveMsnFolderUp (void)
{
int	i;

for (i = (int) strlen (gameFolders.szMsnSubFolder) - 2; i >= 0; i--)
	if (gameFolders.szMsnSubFolder [i] == '/')
		break;
gameFolders.szMsnSubFolder [i + 1] = '\0';
}

//------------------------------------------------------------------------------

void MoveMsnFolderDown (int nSubFolder)
{
strcat (gameFolders.szMsnSubFolder, gameData.missions.list [nSubFolder].mission_name + 1);
gameFolders.szMsnSubFolder [strlen (gameFolders.szMsnSubFolder) - 1] = '/';
}

//------------------------------------------------------------------------------

int BuildMissionList(int anarchy_mode, int nSubFolder)
{
	static int num_missions=-1;
	int count = 0;
	int top_place, bSubFolder, bHaveSubFolders;

	//now search for levels on disk

//@@Took out this code because after this routine was called once for
//@@a list of single-tPlayer missions, a subsequent call for a list of
//@@anarchy missions would not scan again, and thus would not ffs the
//@@anarchy-only missions.  If we retain the minimum level of install,
//@@we may want to put the code back in, having it always scan for all
//@@missions, and have the code that uses it sort out the ones it wants.
//@@	if (num_missions != -1) {
//@@		if (gameData.missions.nCurrentMission != 0)
//@@			LoadMission(0);				//set built-in mission as default
//@@		return num_missions;
//@@	}

	if (nSubFolder >= 0) {
		if (strcmp (gameData.missions.list [nSubFolder].mission_name, "[..]"))
			MoveMsnFolderDown (nSubFolder);
		else
			MoveMsnFolderUp ();
		}

	bSubFolder = (*gameFolders.szMsnSubFolder != '\0');
	if (!bSubFolder && gameOpts->app.bSinglePlayer) {
		strcpy (gameFolders.szMsnSubFolder, "single/");
//		bSubFolder = 1;
		}
	if (!bSubFolder) {// || (gameOpts->app.bSinglePlayer && !strcmp (gameFolders.szMsnSubFolder, "single"))) {
		if (gameOpts->app.nVersionFilter & 2) {
			add_builtin_mission_to_list(&count);  //read built-in first
			if (gameOpts->app.bSinglePlayer)
				add_d2x_builtin_mission_to_list(&count);  //read built-in first
			}
		if (gameOpts->app.nVersionFilter & 1)
			add_d1_builtin_mission_to_list(&count);
		}
	nBuiltIns = count;
	sprintf (gameFolders.szMissionDirs [0], "%s/%s", gameFolders.szMissionDir, gameFolders.szMsnSubFolder);
	bHaveSubFolders = 0;
	if (gameOpts->app.nVersionFilter & 2) {
		AddMissionsToList(&count, anarchy_mode, 0, bSubFolder, bHaveSubFolders, ML_MISSIONDIR);
		bHaveSubFolders = 1;
		}
	if (gameOpts->app.nVersionFilter & 1) {
		AddMissionsToList(&count, anarchy_mode, 1, bSubFolder, bHaveSubFolders, ML_MISSIONDIR);
		bHaveSubFolders = 1;
		}
	if (gameFolders.bAltHogDirInited && strcmp (gameFolders.szAltHogDir, gameFolders.szGameDir)) {
		bHaveSubFolders = 0;
		sprintf (gameFolders.szMissionDirs [1], "%s/%s%s", gameFolders.szAltHogDir, MISSION_DIR, gameFolders.szMsnSubFolder);
		if (gameOpts->app.nVersionFilter & 2) {
			AddMissionsToList(&count, anarchy_mode, 0, bSubFolder, bHaveSubFolders, ML_ALTHOGDIR);
			bHaveSubFolders = 1;
			}
		if (gameOpts->app.nVersionFilter & 1) {
			AddMissionsToList(&count, anarchy_mode, 1, bSubFolder, bHaveSubFolders, ML_ALTHOGDIR);
			bHaveSubFolders = 1;
		}
	}

	// move original missions (in story-chronological order)
	// to top of mission list
	top_place = 0;
	if (bSubFolder) {
		gameData.missions.nBuiltinMission =
		gameData.missions.nD1BuiltinMission = -1;
		}
	else {
		promote("descent", &top_place, count); // original descent 1 mission
		gameData.missions.nD1BuiltinMission = top_place - 1;
		promote(gameData.missions.szBuiltinMissionFilename, &top_place, count); // d2 or d2demo
		gameData.missions.nBuiltinMission = top_place - 1;
		promote("d2x", &top_place, count); // vertigo
		}
	if (count > top_place)
		qsort(gameData.missions.list + top_place,
		      count - top_place,
		      sizeof(*gameData.missions.list),
 				(int (_CDECL_ *)( const void *, const void * ))ml_sort_func);


	if (count > top_place)
		qsort(gameData.missions.list + top_place,
		      count - top_place,
		      sizeof(*gameData.missions.list),
		      (int (_CDECL_ *)( const void *, const void * ))ml_sort_func);

	//LoadMission(0);   //set built-in mission as default
	num_missions = count;
	return count;
}

//------------------------------------------------------------------------------

char *MsnTrimComment (char *buf)
{
	char *ps;

if (ps = strchr (buf, ';')) {
	while (ps > buf) {
		--ps;
		if (!isspace ((unsigned char) *ps)) {
			++ps;
			break;
			}
		}
	*ps = '\0';
	}
return buf;
}

//------------------------------------------------------------------------------

int ParseMissionFile (CFILE *fp)
{
	int	i;
	char	*t, *v;
	char	buf [80], *bufP;

LogErr ("   parsing mission file\n");
gameData.missions.nLastLevel = 0;
gameData.missions.nLastSecretLevel = 0;
szBriefingTextFilename [0] = '\0';
szEndingTextFilename [0] = '\0';
while (MsnGetS (buf, 80, fp)) {
	LogErr ("      '%s'\n", buf);
	MsnTrimComment (buf);
	if (MsnIsTok (buf, "name"))
		;						//already have name, go to next line
	else if (MsnIsTok (buf, "xname")) {
		gameData.missions.nEnhancedMission = 1;
		}
	else if (MsnIsTok (buf, "zname")) {
		gameData.missions.nEnhancedMission = 2;
		}
	else if (MsnIsTok (buf, "d2x-name")) {
		if (gameStates.app.bNostalgia > 2)
			return 0;
		gameData.missions.nEnhancedMission = 3;	
		}
	else if (MsnIsTok (buf, "type"))
		;	
	else if (MsnIsTok (buf, "hog")) {
		bufP = buf;
		while (*(bufP++) != '=')
			;
		while (*bufP == ' ')
			bufP++;
		CFUseAltHogFile (bufP);
		}
	else if (MsnIsTok (buf, "briefing")) {
		if (v = MsnGetValue (buf)) {
			MsnAddStrTerm (v);
			if (strlen (v) < 13)
				strcpy (szBriefingTextFilename, v);
			else
				LogErr ("      mission file: ignoring invalid briefing name\n");
			}
		}
	else if (MsnIsTok (buf, "ending")) {
		if (v = MsnGetValue (buf)) {
			MsnAddStrTerm (v);
			if (strlen (v) < 13)
				strcpy (szEndingTextFilename, v);
			else
				LogErr ("      mission file: ignoring invalid end briefing name\n");
			}
		}
	else if (MsnIsTok (buf, "num_levels")) {
		if (v = MsnGetValue (buf)) {
			int nLevels = atoi (v);
			if (nLevels)
				LogErr ("      parsing level list\n");
			for (i = 0; (i < nLevels) && MsnGetS (buf, 80, fp); i++) {
				LogErr ("         '%s'\n", buf);
				MsnTrimComment (buf);
				MsnAddStrTerm (buf);
				if (strlen (buf) > 12) {
					LogErr ("      mission file: invalid level name\n");
					return 0;
					}
				strcpy (gameData.missions.szLevelNames [i], buf);
				gameData.missions.nLastLevel++;
				}
			}
		}
	else if (MsnIsTok (buf,"num_secrets")) {
		LogErr ("      parsing secret level list\n");
		if (v = MsnGetValue (buf)) {
			gameData.missions.nSecretLevels = atoi (v);
			Assert(gameData.missions.nSecretLevels <= MAX_SECRET_LEVELS_PER_MISSION);
			for (i = 0; (i < gameData.missions.nSecretLevels) && MsnGetS (buf, 80, fp); i++) {
				LogErr ("         '%s'\n", buf);
				MsnTrimComment (buf);
				if (!(t = strchr (buf, ','))) {
					LogErr ("      mission file: secret level lacks link to base level\n");
					return 0;
					}
				*t++ = 0;
				MsnAddStrTerm (buf);
				if (strlen (buf) > 12) {
					LogErr ("      mission file: invalid level name\n");
					return 0;
					}
				strcpy (gameData.missions.szSecretLevelNames [i], buf);
				gameData.missions.secretLevelTable [i] = atoi (t);
				if ((gameData.missions.secretLevelTable [i] < 1) || 
					 (gameData.missions.secretLevelTable [i] > gameData.missions.nLastLevel)) {
					LogErr ("      mission file: invalid secret level base level number\n");
					return 0;
					}
				gameData.missions.nLastSecretLevel--;
				}
			}
		}
	}
return 1;
}

//------------------------------------------------------------------------------

void InitExtraRobotMovie(char *filename);

//values for built-in mission

//loads the specfied mission from the mission list.
//BuildMissionList() must have been called.
//Returns true if mission loaded ok, else false.
int LoadMission(int mission_num)
{
	CFILE		*fp;
	char		szFolder [FILENAME_LEN], szFile [FILENAME_LEN];
	int		i, bFoundHogFile;
	
gameData.missions.nEnhancedMission = 0;
if (mission_num == gameData.missions.nD1BuiltinMission) {
	CFUseD1HogFile("descent.hog");
	switch (gameData.missions.nD1BuiltinHogSize) {
	default:
		Int3(); // fall through
	case D1_MISSION_HOGSIZE:
	case D1_10_MISSION_HOGSIZE:
	case D1_15_MISSION_HOGSIZE:
	case D1_3DFX_MISSION_HOGSIZE:
	case D1_MAC_MISSION_HOGSIZE:
	case D1_OEM_MISSION_HOGSIZE:
	case D1_OEM_10_MISSION_HOGSIZE:
	case D1_SHAREWARE_MISSION_HOGSIZE:
	case D1_SHAREWARE_10_MISSION_HOGSIZE:
	case D1_MAC_SHARE_MISSION_HOGSIZE:
		return load_mission_d1(mission_num);
		break;
		}
	}

if (mission_num == gameData.missions.nBuiltinMission) {
	switch (gameData.missions.nBuiltinHogSize) {
	case SHAREWARE_MISSION_HOGSIZE:
	case MAC_SHARE_MISSION_HOGSIZE:
		return load_mission_shareware(mission_num);
		break;
	case OEM_MISSION_HOGSIZE:
		return load_mission_oem(mission_num);
		break;
	default:
		Int3(); // fall through
	case FULL_MISSION_HOGSIZE:
	case FULL_10_MISSION_HOGSIZE:
	case MAC_FULL_MISSION_HOGSIZE:
		// continue on... (use d2.mn2 from hogfile)
		break;
		}
	}
gameData.missions.nCurrentMission = mission_num;
#if TRACE
con_printf (CON_VERBOSE, "Loading mission %d\n", mission_num );
#endif
	//read mission from file
switch (gameData.missions.list [mission_num].location) {
	case ML_MISSIONDIR:
	case ML_ALTHOGDIR:
		strcpy (szFolder, gameFolders.szMissionDirs [gameData.missions.list [mission_num].location]);
		break;
	case ML_CDROM:
		sprintf (szFolder, "%s/", CDROM_dir);
		break;
	default:
		Int3();							//fall through
	case ML_CURDIR:
		*szFolder = '\0';
		break;
	case ML_MSNROOTDIR:
		sprintf (szFolder, "%s%s", gameFolders.szMissionDir, *gameFolders.szMissionDir ? "/" : "");
		break;
	case ML_DATADIR:
		sprintf (szFolder, "%s%s", gameFolders.szDataDir, *gameFolders.szDataDir ? "/" : "");
		break;
	}
sprintf (szFile, "%s%s", 
			gameData.missions.list [mission_num].filename,
			(gameData.missions.list [mission_num].descent_version == 2) ? ".mn2" : ".msn");
strlwr (szFile);
if (!(fp = CFOpen (szFile, szFolder, "rb", 0))) {
	gameData.missions.nCurrentMission = -1;
	return 0;		//error!
	}
//for non-builtin missions, load HOG
CFUseAltHogFile ("");
if (strcmp(gameData.missions.list [mission_num].filename, gameData.missions.szBuiltinMissionFilename)) {
	sprintf (szFile, "%s%s.hog", szFolder, gameData.missions.list [mission_num].filename);
	strlwr (szFile);
	bFoundHogFile = CFUseAltHogFile (szFile);
	if (bFoundHogFile) {
		// for Descent 1 missions, load descent.hog
		if ((gameData.missions.list [mission_num].descent_version == 1) && 
				strcmp (gameData.missions.list [mission_num].filename, "descent"))
			if (!CFUseD1HogFile ("descent.hog"))
				Warning(TXT_NO_HOG);
		}
	else {
		sprintf (szFile, "%s%s%s", 
					szFolder,
					gameData.missions.list [mission_num].filename,
					(gameData.missions.list [mission_num].descent_version == 2) ? ".rl2" : ".rdl");
		strlwr (szFile);
		bFoundHogFile = CFUseAltHogFile (szFile);
		CFClose (fp);
		if (bFoundHogFile) {
			strcpy (gameData.missions.szLevelNames [0], gameHogFiles.AltHogFiles.files [0].name);
			gameData.missions.nLastLevel = 1;
			}
		else
			gameData.missions.nCurrentMission = -1;
		return bFoundHogFile;
		}
	}
//init vars
i = ParseMissionFile (fp);
CFClose(fp);
if (!i) {
	gameData.missions.nCurrentMission = -1;
	ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_MSNFILE_ERROR);
	return 0;
	}
if (gameData.missions.nLastLevel <= 0) {
	gameData.missions.nCurrentMission = -1;		//no valid mission loaded
	return 0;
	}

gameStates.app.szCurrentMissionFile = gameData.missions.list [gameData.missions.nCurrentMission].filename;
gameStates.app.szCurrentMission = gameData.missions.list [gameData.missions.nCurrentMission].mission_name;
return 1;
}

//------------------------------------------------------------------------------
//loads the named mission if exists.
//Returns true if mission loaded ok, else false.
int LoadMissionByName(char *mission_name, int nSubFolder)
{
	int n, i, j;

	if (nSubFolder < 0) {
		*gameFolders.szMsnSubFolder = '\0';
		LogErr ("   searching mission '%s'\n", mission_name);
		}
	n = BuildMissionList (1, nSubFolder);
	for (i = 0; i < n; i++)
		if (!stricmp (mission_name, gameData.missions.list [i].filename))
			return LoadMission (i);
	for (i = 0; i < n; i++)
		if (!gameData.missions.list [i].descent_version && strcmp (gameData.missions.list [i].mission_name, "[..]")) {
			if (j = LoadMissionByName (mission_name, i))
				return j;
			MoveMsnFolderUp ();
			n = BuildMissionList (1, -1);
			}
	return 0;		//couldn't ffs mission
}

//------------------------------------------------------------------------------
//loads the named mission if exists.
//Returns true if mission loaded ok, else false.
int FindMissionByName(char *mission_name, int nSubFolder)
{
	int n,i,j;

	if (nSubFolder < 0)
		*gameFolders.szMsnSubFolder = '\0';
	n = BuildMissionList (1, nSubFolder);
	for (i = 0; i < n; i++) 
		if (!stricmp (mission_name, gameData.missions.list [i].filename))
			return gameData.missions.list [i].descent_version;
	for (i = 0; i < n; i++)
		if (!gameData.missions.list [i].descent_version && 
			 strcmp (gameData.missions.list [i].mission_name, "[..]")) {
			if (j = FindMissionByName (mission_name, i))
				return j;
			MoveMsnFolderUp ();
			n = BuildMissionList (1, -1);
			}
	return 0;		//couldn't ffs mission
}

//------------------------------------------------------------------------------
// eof

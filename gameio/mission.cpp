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

#include "descent.h"
#include "pstypes.h"
#include "cfile.h"
#include "hogfile.h"
#include "strutil.h"
#include "mission.h"
#include "loadgame.h"
#include "briefings.h"
#include "songs.h"
#include "mono.h"
#include "error.h"
#include "findfile.h"
#include "polymodel.h"
#include "robot.h"
#include "menu.h"
#include "text.h"
#ifndef __findfile_h
#	error filefind missing!
#endif

CMissionManager missionManager;

//values for d1 built-in mission
#define BIM_LAST_LEVEL          27
#define BIM_LAST_SECRET_LEVEL   -3
#define BIM_BRIEFING_FILE       "briefing.tex"
#define BIM_ENDING_FILE         "endreg.tex"

//------------------------------------------------------------------------------
//
//  Special versions of mission routines for d1 builtins
//

int CMissionManager::LoadD1 (int nMission)
{
	int i;

hogFileManager.UseD1 ("descent.hog");

nCurrentMission = nMission;
gameStates.app.szCurrentMissionFile = list [nMission].filename;
gameStates.app.szCurrentMission = list [nMission].szMissionName;

switch (nBuiltInHogSize [1]) {
case D1_SHAREWARE_MISSION_HOGSIZE:
case D1_SHAREWARE_10_MISSION_HOGSIZE:
	nSecretLevels = 0;

	nLastLevel = 7;
	nLastSecretLevel = 0;

	//build level names
	for (i=0;i<nLastLevel;i++)
		sprintf(szLevelNames [i], "level%02d.sdl", i+1);

	break;
case D1_MAC_SHARE_MISSION_HOGSIZE:
	nSecretLevels = 0;

	nLastLevel = 3;
	nLastSecretLevel = 0;

	//build level names
	for (i=0;i<nLastLevel;i++)
		sprintf(szLevelNames [i], "level%02d.sdl", i+1);

	break;
case D1_OEM_MISSION_HOGSIZE:
case D1_OEM_10_MISSION_HOGSIZE:
	nSecretLevels = 1;

	nLastLevel = 15;
	nLastSecretLevel = -1;

	//build level names
	for (i=0; i < nLastLevel - 1; i++)
		sprintf(szLevelNames [i], "level%02d.rdl", i+1);
	sprintf(szLevelNames [i], "saturn%02d.rdl", i+1);
	for (i=0; i < -nLastSecretLevel; i++)
		sprintf(szSecretLevelNames [i], "levels%1d.rdl", i+1);

	secretLevelTable [0] = 10;

	break;
default:
	Int3(); // fall through
case D1_MISSION_HOGSIZE:
case D1_10_MISSION_HOGSIZE:
case D1_15_MISSION_HOGSIZE:
case D1_3DFX_MISSION_HOGSIZE:
case D1_MAC_MISSION_HOGSIZE:
	nSecretLevels = 3;

	nLastLevel = BIM_LAST_LEVEL;
	nLastSecretLevel = BIM_LAST_SECRET_LEVEL;

	//build level names
	for (i=0;i<nLastLevel;i++)
		sprintf(szLevelNames [i], "level%02d.rdl", i+1);
	for (i=0;i<-nLastSecretLevel;i++)
		sprintf(szSecretLevelNames [i], "levels%1d.rdl", i+1);

	secretLevelTable [0] = 10;
	secretLevelTable [1] = 21;
	secretLevelTable [2] = 24;

	break;
	}
strcpy (szBriefingFilename [0], BIM_BRIEFING_FILE);
strcpy (szBriefingFilename [1], BIM_ENDING_FILE);
gameStates.app.bD1Mission = 1;
return 1;
}


//------------------------------------------------------------------------------
//
//  Special versions of mission routines for shareware
//

int  CMissionManager::LoadShareware (int nMission)
{
nCurrentMission = nMission;
gameStates.app.szCurrentMissionFile = list [nMission].filename;
gameStates.app.szCurrentMission = list [nMission].szMissionName;

switch (nBuiltInHogSize [0]) {
	case MAC_SHARE_MISSION_HOGSIZE:
		nSecretLevels = 1;

		nLastLevel = 4;
		nLastSecretLevel = -1;

		// mac demo is using the regular hog and rl2 files
		strcpy (szLevelNames [0],"d2leva-1.rl2");
		strcpy (szLevelNames [1],"d2leva-2.rl2");
		strcpy (szLevelNames [2],"d2leva-3.rl2");
		strcpy (szLevelNames [3],"d2leva-4.rl2");
		strcpy (szSecretLevelNames [0],"d2leva-s.rl2");
		break;
	default:
		Int3(); // fall through
	case SHAREWARE_MISSION_HOGSIZE:
		nSecretLevels = 0;

		nLastLevel = 3;
		nLastSecretLevel = 0;

		strcpy (szLevelNames [0],"d2leva-1.sl2");
		strcpy (szLevelNames [1],"d2leva-2.sl2");
		strcpy (szLevelNames [2],"d2leva-3.sl2");
	}
return 1;
}


//------------------------------------------------------------------------------
//
//  Special versions of mission routines for Diamond/S3 version
//

int  CMissionManager::LoadOEM (int nMission)
{
nCurrentMission = nMission;
gameStates.app.szCurrentMissionFile = list [nMission].filename;
gameStates.app.szCurrentMission = list [nMission].szMissionName;
nSecretLevels = 2;
nLastLevel = 8;
nLastSecretLevel = -2;
strcpy (szLevelNames [0],"d2leva-1.rl2");
strcpy (szLevelNames [1],"d2leva-2.rl2");
strcpy (szLevelNames [2],"d2leva-3.rl2");
strcpy (szLevelNames [3],"d2leva-4.rl2");
strcpy (szSecretLevelNames [0],"d2leva-s.rl2");
strcpy (szLevelNames [4],"d2levb-1.rl2");
strcpy (szLevelNames [5],"d2levb-2.rl2");
strcpy (szLevelNames [6],"d2levb-3.rl2");
strcpy (szLevelNames [7],"d2levb-4.rl2");
strcpy (szSecretLevelNames [1],"d2levb-s.rl2");
secretLevelTable [0] = 1;
secretLevelTable [1] = 5;
return 1;
}


//------------------------------------------------------------------------------
//strips damn newline from end of line
char *MsnGetS (char *s, int n, CFile& cf)
{
	char *r;
	int	l;

r = cf.GetS (s, n);
if (r) {
	l = (int) strlen (s) - 1;
	if (s [l] == '\n')
		s [l] = 0;
	}
return r;
}

//------------------------------------------------------------------------------
//compare a string for a token. returns true if match
int MsnIsTok (char *buf, const char *tok)
{
return !strnicmp (buf, tok, strlen (tok));
}

//------------------------------------------------------------------------------
//adds a terminating 0 after a string at the first white space
void MsnAddStrTerm (char *s)
{
while (*s && isprint (*s) && (*s != '\r') && (*s != '\n'))
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
	while (*t && ::isspace (*t)) 
		t++;
	if (*t)
		return t;
	}
return NULL;		//error!
}

//------------------------------------------------------------------------------
//reads a line, returns ptr to value of passed parm.  returns NULL if none
char *GetParmValue (const char *parm, CFile& cf)
{
	static char buf [80];

if (!MsnGetS (buf, 80, cf))
	return NULL;
if (MsnIsTok (buf, parm))
	return MsnGetValue (buf);
return NULL;
}

//------------------------------------------------------------------------------

int _CDECL_ MLSortFunc (tMsnListEntry *e0, tMsnListEntry *e1)
{
	char	*s1 = e0->szMissionName,
			*s2 = e1->szMissionName;

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
int CMissionManager::ReadFile (const char *filename, int count, int location)
{
	CFile		cf;
	char*		p, filename2 [FILENAME_LEN], szMissionName [FILENAME_LEN], temp [FILENAME_LEN], *t;

switch (location) {
	case ML_MISSIONDIR:
	case ML_ALTHOGDIR:
		strcpy (filename2, gameFolders.szMissionDirs [location]);
		break;

	case ML_CDROM:
		redbook.Stop();		//so we can read from the CD
		strcpy (filename2, CDROM_dir);
		break;

	default:
		Int3();		//fall through

	case ML_CURDIR:
		strcpy (filename2, "");
		break;
}
strcat (filename2, filename);
if (!cf.Open (filename2, "", "rb", 0))
	return 0;

strcpy (temp, filename);
if ((t = strchr (temp, '.')) == NULL)
	return 0;	//missing extension
list [count].nDescentVersion = (t [3] == '2') ? 2 : 1;
*t = '\0';
// look if it's .mn2 or .msn
strcpy (list [count].filename, temp);
list [count].bAnarchyOnly = 0;
list [count].location = location;

p = GetParmValue ("name", cf);
if (!p) {		//try enhanced mission
	cf.Seek (0, SEEK_SET);
	p = GetParmValue ("xname", cf);
	}
if (!p) {       //try super-enhanced mission!
	cf.Seek (0, SEEK_SET);
	p = GetParmValue ("zname", cf);
	}
if (!p && (gameStates.app.bNostalgia < 3)) {       //try super-enhanced mission!
	cf.Seek (0, SEEK_SET);
	p = GetParmValue ("d2x-name", cf);
	}

if (p) {
	char *t;
	if ((t = strchr (p, ';')))
		*t-- = 0;
	else
		t = p + strlen (p) - 1;
	while (::isspace (*t) || (t - p > MISSION_NAME_LEN)) 
		*t-- = '\0';
	if (gameOpts->menus.bShowLevelVersion) {
		sprintf (szMissionName, "[%d] %s", list [count].nDescentVersion, p);
		strncpy (list [count].szMissionName, szMissionName, sizeof (list [count].szMissionName));
		strcat (temp, filename);
		}
	else
		strncpy (list [count].szMissionName, p, sizeof (list [count].szMissionName));
	list [count].szMissionName [sizeof (list [count].szMissionName) - 1] = '\0';
	}
else {
	cf.Close ();
	return 0;
	}
p = GetParmValue ("nType", cf);
//get mission nType
if (p)
	list [count].bAnarchyOnly = MsnIsTok (p,"anarchy");
cf.Close ();
return 1;
}

//------------------------------------------------------------------------------

void CMissionManager::AddBuiltinD1Mission (int *count)
{
	CFile	cf;

if (!cf.Exist ("descent.hog", gameFolders.szDataDir, 1))
	return;
nBuiltInHogSize [1] = cf.Size ("descent.hog", gameFolders.szDataDir, 0);
switch (nBuiltInHogSize [1]) {
	case D1_SHAREWARE_MISSION_HOGSIZE:
	case D1_SHAREWARE_10_MISSION_HOGSIZE:
	case D1_MAC_SHARE_MISSION_HOGSIZE:
		strcpy (list [*count].filename, D1_MISSION_FILENAME);
		strncpy (list [*count].szMissionName, D1_SHAREWARE_MISSION_NAME, sizeof (list [*count].szMissionName));
		list [*count].bAnarchyOnly = 0;
		break;

	case D1_OEM_MISSION_HOGSIZE:
	case D1_OEM_10_MISSION_HOGSIZE:
		strcpy (list [*count].filename, D1_MISSION_FILENAME);
		strncpy (list [*count].szMissionName, D1_OEM_MISSION_NAME, sizeof (list [*count].szMissionName));
		list [*count].bAnarchyOnly = 0;
		break;

	default:
#if 0//DBG
		Warning (TXT_D1_HOGSIZE, nBuiltInHogSize [1]);
#endif
		// fall through
	case D1_MISSION_HOGSIZE:
	case D1_10_MISSION_HOGSIZE:
	case D1_15_MISSION_HOGSIZE:
	case D1_3DFX_MISSION_HOGSIZE:
	case D1_MAC_MISSION_HOGSIZE:
		strcpy (list [*count].filename, D1_MISSION_FILENAME);
		if (gameOpts->menus.bShowLevelVersion)
			strncpy (list [*count].szMissionName, "[1] " D1_MISSION_NAME, sizeof (list [*count].szMissionName));
		else
			strncpy (list [*count].szMissionName, D1_MISSION_NAME, sizeof (list [*count].szMissionName));
		list [*count].bAnarchyOnly = 0;
		list [*count].location = ML_MISSIONDIR;
		break;
	}

	strcpy (szBuiltinMissionFilename [1], list [*count].filename);
	list [*count].nDescentVersion = 1;
	list [*count].bAnarchyOnly = 0;
	list [*count].location = ML_DATADIR;
	++(*count);
}

//------------------------------------------------------------------------------

void CMissionManager::AddBuiltinD2XMission (int *count)
{
	CFile	cf;

if (cf.Exist ("d2x.hog", gameFolders.szMissionDir, 0)) {
	strcpy (list [*count].filename,"d2x");
	if (gameOpts->menus.bShowLevelVersion)
		strcpy (list [*count].szMissionName,"[2] Descent 2: Vertigo");
	else
		strcpy (list [*count].szMissionName,"Descent 2: Vertigo");
	list [*count].nDescentVersion = 2;
	list [*count].bAnarchyOnly = 0;
	list [*count].location = ML_MSNROOTDIR;
	++(*count);
	}
}

//------------------------------------------------------------------------------

void CMissionManager::AddBuiltinMission (int *count)
{
	CFile	cf;

nBuiltInHogSize [0] = cf.Size ("descent2.hog", gameFolders.szDataDir, 0);
if (nBuiltInHogSize [0] == -1)
	nBuiltInHogSize [0] = cf.Size ("d2demo.hog", gameFolders.szDataDir, 0);

switch (nBuiltInHogSize [0]) {
	case SHAREWARE_MISSION_HOGSIZE:
	case MAC_SHARE_MISSION_HOGSIZE:
		strcpy (list [*count].filename, SHAREWARE_MISSION_FILENAME);
		strcpy (list [*count].szMissionName, SHAREWARE_MISSION_NAME);
		list [*count].bAnarchyOnly = 0;
		break;

	case OEM_MISSION_HOGSIZE:
		strcpy (list [*count].filename, OEM_MISSION_FILENAME);
		strcpy (list [*count].szMissionName, OEM_MISSION_NAME);
		list [*count].bAnarchyOnly = 0;
		break;

	default:
#if 0//DBG
		Warning (TXT_HOGSIZE, nBuiltInHogSize [0], "descent2.hog");
#endif
		nBuiltInHogSize [0] = FULL_MISSION_HOGSIZE;	//fall through

	case FULL_MISSION_HOGSIZE:
	case FULL_10_MISSION_HOGSIZE:
	case MAC_FULL_MISSION_HOGSIZE:
		if (!ReadFile (FULL_MISSION_FILENAME ".mn2", 0, ML_CURDIR))
			Error ("Could not find required mission file <%s>", FULL_MISSION_FILENAME ".mn2");
		break;
		//goto retry;
	}
strcpy (szBuiltinMissionFilename [0], list [*count].filename);
list [*count].nDescentVersion = 2;
list [*count].bAnarchyOnly = 0;
list [*count].location = ML_DATADIR;
++(*count);
}

//------------------------------------------------------------------------------

int nBuiltIns = 0;

void CMissionManager::Add (int *count, int anarchy_mode, int bD1Mission, int bSubFolder, int bHaveSubFolders, int nLocation)
{
	CFile	cf;
	FFS	ffs;
	int 	c = *count;
	char lvlName [255];
	char searchName [255];
	const char *lvlExt = ".hog";
	int	bFindDirs;

	static const char *altLvlExt [2] = {".rdl", ".rl2"};

#ifdef _WIN32
	static const char *pszExt [2][2] = {{"*.mn2", "*.msn"},{"*", "*"}};
#else
	static const char *pszExt [2][2] = {{"*.mn2", "*.msn"},{"*", "*"}};
#endif

	if (c + bSubFolder >= MAX_MISSIONS) 
		return;
	if (bSubFolder && !bHaveSubFolders && !nLocation) {
		list [c].nDescentVersion = 0;	//directory
		*list [c].filename = '\0';
		strcpy (list [c].szMissionName, "[..]");
		c++;
		}
	for (bFindDirs = !bHaveSubFolders && (c == nBuiltIns + bSubFolder); bFindDirs >= 0; bFindDirs--) {
		sprintf (searchName, "%s%s", gameFolders.szMissionDirs [nLocation], pszExt [bFindDirs][bD1Mission]);
		if (!FFF (searchName, &ffs, bFindDirs)) {
			do {
				if (!(strcmp (ffs.name, ".") && strcmp (ffs.name, "..")))
					continue;
				if (bFindDirs) {
						list [c].nDescentVersion = 0;	//directory
						*list [c].filename = '\0';
						sprintf (list [c].szMissionName, "[%s]", ffs.name);
						c++;
						}
				else {
					sprintf (lvlName, "%s%s", gameFolders.szMissionDirs [nLocation], ffs.name);
					memcpy (lvlName + strlen (lvlName) - 4, lvlExt, sizeof (lvlExt));
					if (cf.Exist (lvlName, "", 0))
						memcpy (lvlName + strlen (lvlName) - 4, altLvlExt [bD1Mission], sizeof (altLvlExt [bD1Mission]));
					if (!cf.Exist (lvlName, "", 0)) {
						if (ReadFile (ffs.name, c, nLocation) &&
							 (anarchy_mode || !list [c].bAnarchyOnly))
							c++;
						}
					}
			} while ((c < MAX_MISSIONS) && !FFN (&ffs, bFindDirs));
			FFC (&ffs);
			if (c >= MAX_MISSIONS) {
				list [c].nDescentVersion = 0;	//directory
				strcpy (list [c].filename, TXT_MSN_OVERFLOW);

	#if TRACE
				console.printf (CON_DBG, "Warning: more missions than D2X-XL can handle\n");
	#endif		
			}
		}
	*count = c;
	}
}

//------------------------------------------------------------------------------
/* move <szMissionName> to <place> on mission list, increment <place> */
void CMissionManager::Promote (const char * szMissionName, int * nTopPlace, int nMissionCount)
{
	int i;
	char name [FILENAME_LEN], * t;
	strcpy (name, szMissionName);
	if ((t = strchr(name,'.')) != NULL)
		*t = 0; //kill extension
	////printf("promoting: %s\n", name);
	for (i = *nTopPlace; i < nMissionCount; i++)
		if (!stricmp(list [i].filename, name)) {
			//swap mission positions
			tMsnListEntry temp;

			temp = list [*nTopPlace];
			list [*nTopPlace] = list [i];
			list [i] = temp;
			++(*nTopPlace);
			break;
		}
}

//------------------------------------------------------------------------------
//fills in the global list of missions.  Returns the number of missions
//in the list.  If anarchy_mode set, don't include non-anarchy levels.

extern char CDROM_dir [];

void CMissionManager::MoveFolderUp (void)
{
int	i;

for (i = (int) strlen (gameFolders.szMsnSubDir) - 2; i >= 0; i--)
	if (gameFolders.szMsnSubDir [i] == '/')
		break;
gameFolders.szMsnSubDir [i + 1] = '\0';
}

//------------------------------------------------------------------------------

void CMissionManager::MoveFolderDown (int nSubFolder)
{
strcat (gameFolders.szMsnSubDir, list [nSubFolder].szMissionName + 1);
gameFolders.szMsnSubDir [strlen (gameFolders.szMsnSubDir) - 1] = '/';
}

//------------------------------------------------------------------------------

int CMissionManager::BuildList (int anarchy_mode, int nSubFolder)
{
	static int nMissionCount = -1;
	int count = 0;
	int nTopPlace, bSubFolder, bHaveSubFolders;

//now search for levels on disk
if (nSubFolder >= 0) {
	if (strcmp (list [nSubFolder].szMissionName, "[..]"))
		MoveFolderDown (nSubFolder);
	else
		MoveFolderUp ();
	}

bSubFolder = (*gameFolders.szMsnSubDir != '\0');
if (!bSubFolder && gameOpts->app.bSinglePlayer) {
	strcpy (gameFolders.szMsnSubDir, "single/");
//		bSubFolder = 1;
	}
if (gameStates.app.bDemoData) {
	AddBuiltinMission (&count);  //read built-in first
	nBuiltIns = count;
	}
else {
	if (!bSubFolder) {// || (gameOpts->app.bSinglePlayer && !strcmp (gameFolders.szMsnSubDir, "single"))) {
		if (gameOpts->app.nVersionFilter & 2) {
			AddBuiltinMission (&count);  //read built-in first
			if (gameOpts->app.bSinglePlayer)
				AddBuiltinD2XMission (&count);  //read built-in first
			}
		if (gameOpts->app.nVersionFilter & 1)
			AddBuiltinD1Mission (&count);
		}
	nBuiltIns = count;
	sprintf (gameFolders.szMissionDirs [0], "%s/%s", gameFolders.szMissionDir, gameFolders.szMsnSubDir);
	bHaveSubFolders = 0;
	if (gameOpts->app.nVersionFilter & 2) {
		Add (&count, anarchy_mode, 0, bSubFolder, bHaveSubFolders, ML_MISSIONDIR);
		bHaveSubFolders = 1;
		}
	if (gameOpts->app.nVersionFilter & 1) {
		Add (&count, anarchy_mode, 1, bSubFolder, bHaveSubFolders, ML_MISSIONDIR);
		bHaveSubFolders = 1;
		}
	if (gameFolders.bAltHogDirInited && strcmp (gameFolders.szAltHogDir, gameFolders.szGameDir)) {
		bHaveSubFolders = 0;
		sprintf (gameFolders.szMissionDirs [1], "%s/%s%s", gameFolders.szAltHogDir, MISSION_DIR, gameFolders.szMsnSubDir);
		if (gameOpts->app.nVersionFilter & 2) {
			Add (&count, anarchy_mode, 0, bSubFolder, bHaveSubFolders, ML_ALTHOGDIR);
			bHaveSubFolders = 1;
			}
		if (gameOpts->app.nVersionFilter & 1) {
			Add (&count, anarchy_mode, 1, bSubFolder, bHaveSubFolders, ML_ALTHOGDIR);
			bHaveSubFolders = 1;
			}
		}
	}
	// move original missions (in story-chronological order)
	// to top of mission list
nTopPlace = 0;
if (bSubFolder) {
	nBuiltInMission [0] =
	nBuiltInMission [1] = -1;
	}
else {
	Promote ("descent", &nTopPlace, count); // original descent 1 mission
	nBuiltInMission [1] = nTopPlace - 1;
	Promote (szBuiltinMissionFilename [0], &nTopPlace, count); // d2 or d2demo
	nBuiltInMission [0] = nTopPlace - 1;
	Promote ("d2x", &nTopPlace, count); // vertigo
	}
if (count > nTopPlace)
	qsort (list + nTopPlace,
			 count - nTopPlace,
			 sizeof (*list),
			 (int (_CDECL_ *)(const void *, const void *)) MLSortFunc);

if (count > nTopPlace)
	qsort(list + nTopPlace,
			count - nTopPlace,
			sizeof(*list),
			(int (_CDECL_ *) (const void *, const void * )) MLSortFunc);
//missionManager.Load(0);   //set built-in mission as default
nMissionCount = count;
return count;
}

//------------------------------------------------------------------------------

char *MsnTrimComment (char *buf)
{
	char *ps;

if ((ps = strchr (buf, ';'))) {
	while (ps > buf) {
		--ps;
		if (!::isspace ((ubyte) *ps)) {
			++ps;
			break;
			}
		}
	*ps = '\0';
	}
return buf;
}

//------------------------------------------------------------------------------

int CMissionManager::Parse (CFile& cf)
{
	int	i;
	char	*t, *v;
	char	buf [80], *bufP;

PrintLog ("   parsing mission file\n");
nLastLevel = 0;
nLastSecretLevel = 0;
*szBriefingFilename [0] = '\0';
*szBriefingFilename [1] = '\0';
while (MsnGetS (buf, 80, cf)) {
	PrintLog ("      '%s'\n", buf);
	MsnTrimComment (buf);
	if (MsnIsTok (buf, "name"))
		;						//already have name, go to next line
	else if (MsnIsTok (buf, "xname")) {
		nEnhancedMission = 1;
		}
	else if (MsnIsTok (buf, "zname")) {
		nEnhancedMission = 2;
		}
	else if (MsnIsTok (buf, "d2x-name")) {
		if (gameStates.app.bNostalgia > 2)
			return 0;
		nEnhancedMission = 3;
		}
	else if (MsnIsTok (buf, "type"))
		;
	else if (MsnIsTok (buf, "hog")) {
		bufP = buf;
		while (*(bufP++) != '=')
			;
		while (*bufP == ' ')
			bufP++;
		hogFileManager.UseMission (bufP);
		}
	else if (MsnIsTok (buf, "briefing")) {
		if ((v = MsnGetValue (buf))) {
			MsnAddStrTerm (v);
			if (strlen (v) < 13)
				strcpy (szBriefingFilename [0], v);
			else
				PrintLog ("      mission file: ignoring invalid briefing name\n");
			}
		}
	else if (MsnIsTok (buf, "ending")) {
		if ((v = MsnGetValue (buf))) {
			MsnAddStrTerm (v);
			if (strlen (v) < 13)
				strcpy (szBriefingFilename [1], v);
			else
				PrintLog ("      mission file: ignoring invalid end briefing name\n");
			}
		}
	else if (MsnIsTok (buf, "num_levels")) {
		if ((v = MsnGetValue (buf))) {
			int nLevels = atoi (v);
			if (nLevels)
				PrintLog ("      parsing level list\n");
			for (i = 0; (i < nLevels) && MsnGetS (buf, 80, cf); i++) {
				PrintLog ("         '%s'\n", buf);
				MsnTrimComment (buf);
				MsnAddStrTerm (buf);
				if (strlen (buf) > 12) {
					PrintLog ("      mission file: invalid level name\n");
					return 0;
					}
				strcpy (szLevelNames [i], buf);
				nLastLevel++;
				}
			}
		}
	else if (MsnIsTok (buf,"num_secrets")) {
		PrintLog ("      parsing secret level list\n");
		if ((v = MsnGetValue (buf))) {
			nSecretLevels = atoi (v);
			Assert(nSecretLevels <= MAX_SECRET_LEVELS_PER_MISSION);
			for (i = 0; (i < nSecretLevels) && MsnGetS (buf, 80, cf); i++) {
				PrintLog ("         '%s'\n", buf);
				MsnTrimComment (buf);
				if (!(t = strchr (buf, ','))) {
					PrintLog ("      mission file: secret level lacks link to base level\n");
					return 0;
					}
				*t++ = 0;
				MsnAddStrTerm (buf);
				if (strlen (buf) > 12) {
					PrintLog ("      mission file: invalid level name\n");
					return 0;
					}
				strcpy (szSecretLevelNames [i], buf);
				secretLevelTable [i] = atoi (t);
				if ((secretLevelTable [i] < 1) || 
					 (secretLevelTable [i] > nLastLevel)) {
					PrintLog ("      mission file: invalid secret level base level number\n");
					return 0;
					}
				nLastSecretLevel--;
				}
			}
		}
	}
return 1;
}

//------------------------------------------------------------------------------

void CMissionManager::LoadSongList (char *szFile)
{
	CFile	cf;
	char	pn [FILENAME_LEN], fn [FILENAME_LEN] = {'\x01'};
	int	i;

memset (szSongNames, 0, sizeof (szSongNames));
CFile::SplitPath (szFile, pn, fn + 1, NULL);
strcat (fn, ".sng");
if (!cf.Open (fn, pn, "rb", 0))
	return;
for (i = 0; cf.GetS (szSongNames [i], SHORT_FILENAME_LEN) && (i < MAX_LEVELS_PER_MISSION); i++)
	;
cf.Close ();
}

//------------------------------------------------------------------------------

void InitExtraRobotMovie (char *filename);

//values for built-in mission

//loads the specfied mission from the mission list.
//BuildMissionList() must have been called.
//Returns true if mission loaded ok, else false.
int CMissionManager::Load (int nMission)
{
	CFile		cf;
	char		szFolder [FILENAME_LEN] = {'\0'}, szFile [FILENAME_LEN] = {'\0'};
	int		i, bFoundHogFile = 0;

nEnhancedMission = 0;
if (nMission >= 0) {
	if (nMission == nBuiltInMission [1]) {
		hogFileManager.UseD1 ("descent.hog");
		switch (nBuiltInHogSize [1]) {
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
			return LoadD1 (nMission);
			}
		}

	if (nMission == nBuiltInMission [0]) {
		switch (nBuiltInHogSize [0]) {
		case SHAREWARE_MISSION_HOGSIZE:
		case MAC_SHARE_MISSION_HOGSIZE:
			return LoadShareware (nMission);
			break;
		case OEM_MISSION_HOGSIZE:
			return LoadOEM (nMission);
			break;
		default:
			Int3(); // fall through
		case FULL_MISSION_HOGSIZE:
		case FULL_10_MISSION_HOGSIZE:
		case MAC_FULL_MISSION_HOGSIZE:
			// continue on... (use d2.mn2 from tHogFile)
			break;
			}
		}
	}

nCurrentMission = nMission;
#if TRACE
console.printf (CON_VERBOSE, "Loading mission %d\n", nMission);
#endif
	//read mission from file
switch (list [nMission].location) {
	case ML_MISSIONDIR:
	case ML_ALTHOGDIR:
		strcpy (szFolder, gameFolders.szMissionDirs [list [nMission].location]);
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
			list [nMission].filename,
			(list [nMission].nDescentVersion == 2) ? ".mn2" : ".msn");
strlwr (szFile);
if (!cf.Open (szFile, szFolder, "rb", 0)) {
	nCurrentMission = -1;
	return 0;		//error!
	}
i = Parse (cf);
cf.Close ();
if (!i) {
	nCurrentMission = -1;
	MsgBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_MSNFILE_ERROR);
	return 0;
	}
//for non-builtin missions, load HOG
hogFileManager.UseMission ("");
if (!strcmp (list [nMission].filename, szBuiltinMissionFilename [0])) 
	bFoundHogFile = 1;
else {
	sprintf (szFile, "%s%s.hog", szFolder, list [nMission].filename);
	strlwr (szFile);
	bFoundHogFile = hogFileManager.UseMission (szFile);
	if (bFoundHogFile) {
		// for Descent 1 missions, load descent.hog
		if ((list [nMission].nDescentVersion == 1) && 
			 strcmp (list [nMission].filename, "descent"))
			if (!hogFileManager.UseD1 ("descent.hog"))
				Warning (TXT_NO_HOG);
		}
	else {
		sprintf (szFile, "%s%s%s", 
					szFolder,
					list [nMission].filename,
					(list [nMission].nDescentVersion == 2) ? ".rl2" : ".rdl");
		strlwr (szFile);
		bFoundHogFile = hogFileManager.UseMission (szFile);
		if (bFoundHogFile) {
			strcpy (szLevelNames [0], hogFileManager.AltFiles ().files [0].name);
			nLastLevel = 1;
			}
		else {
			sprintf (szFile, "%s%s", szFolder, szLevelNames [0]);
			strlwr (szFile);
			bFoundHogFile = hogFileManager.UseMission (szFile);
			}
		}
	}
//init vars
if (!bFoundHogFile || (nLastLevel <= 0)) {
	nCurrentMission = -1;		//no valid mission loaded
	return 0;
	}
gameStates.app.szCurrentMissionFile = list [nCurrentMission].filename;
gameStates.app.szCurrentMission = list [nCurrentMission].szMissionName;
LoadSongList (szFile);
return 1;
}

//------------------------------------------------------------------------------
//loads the named mission if exists.
//Returns true if mission loaded ok, else false.
int CMissionManager::LoadByName (char *szMissionName, int nSubFolder)
{
	int n, i, j;

if (nSubFolder < 0) {
	*gameFolders.szMsnSubDir = '\0';
	PrintLog ("   searching mission '%s'\n", szMissionName);
	}
n = BuildList (1, nSubFolder);
for (i = 0; i < n; i++)
	if (!stricmp (szMissionName, list [i].filename))
		return Load (i);
for (i = 0; i < n; i++)
	if (!list [i].nDescentVersion && strcmp (list [i].szMissionName, "[..]")) {
		if ((j = LoadByName (szMissionName, i)))
			return j;
		MoveFolderUp ();
		n = BuildList (1, -1);
		}
return 0;		//couldn't ffs mission
}

//------------------------------------------------------------------------------
//loads the named mission if exists.
//Returns true if mission loaded ok, else false.
int CMissionManager::FindByName (char *szMissionName, int nSubFolder)
{
	int n,i,j;

if (nSubFolder < 0)
	*gameFolders.szMsnSubDir = '\0';
n = BuildList (1, nSubFolder);
for (i = 0; i < n; i++) 
	if (!stricmp (szMissionName, list [i].filename))
		return list [i].nDescentVersion;
for (i = 0; i < n; i++)
	if (!list [i].nDescentVersion && 
			strcmp (list [i].szMissionName, "[..]")) {
		if ((j = FindByName (szMissionName, i)))
			return j;
		MoveFolderUp ();
		n = BuildList (1, -1);
		}
return 0;		//couldn't find mission
}

//------------------------------------------------------------------------------

int CMissionManager::IsBuiltIn (char* pszMission)
{
#if 0
if (*pszMission)
	return false;
#endif
pszMission = list [nCurrentMission].szMissionName;

return (strstr (pszMission, "Descent: First Strike") != NULL) ||
		 (strstr (pszMission, "Descent 2: Counterstrike!") != NULL) ||
		 (strstr (pszMission, "Descent 2: Vertigo") != NULL);
}

//------------------------------------------------------------------------------

char* CMissionManager::LevelStateName (char* szFile, int nLevel)
{
#if DBG
sprintf (szFile, "%s-player%d.level%d", 
			list [nCurrentMission].szMissionName + 4, IsMultiGame ? gameData.multiplayer.nLocalPlayer + 1 : 0, nLevel ? nLevel : nCurrentLevel);
#else
sprintf (szFile, "%s.level%d", list [nCurrentMission].szMissionName + 4, nLevel ? nLevel : nCurrentLevel);
#endif
return szFile;
}

//------------------------------------------------------------------------------

int CMissionManager::SaveLevelStates (void)
{
if (!gameStates.app.bReadOnly) {
		CFile		cf;
		char		szFile [FILENAME_LEN] = {'\0'};

	sprintf (szFile, "%s.state", list [nCurrentMission].szMissionName + ((list [nCurrentMission].szMissionName [0] == '[') ? 4 : 0));
	if (!cf.Open (szFile, gameFolders.szCacheDir, "wb", 0))
		return 0;
	for (int i = 0; i < MAX_LEVELS_PER_MISSION; i++)
		cf.WriteByte (sbyte (nLevelState [i]));
	cf.Close ();
	}
return 1;
}

//------------------------------------------------------------------------------

int CMissionManager::LoadLevelStates (void)
{
	CFile		cf;
	char		szFile [FILENAME_LEN] = {'\0'};

sprintf (szFile, "%s.state", list [nCurrentMission].szMissionName + ((list [nCurrentMission].szMissionName [0] == '[') ? 4 : 0));
if (!cf.Open (szFile, gameFolders.szCacheDir, "rb", 0))
	return 0;
for (int i = 0; i < MAX_LEVELS_PER_MISSION; i++)
	nLevelState [i] = char (cf.ReadByte ());
cf.Close ();
return 1;
}

//------------------------------------------------------------------------------

void CMissionManager::DeleteLevelStates (void)
{
	char	szFile [FILENAME_LEN] = {'\0'};

for (int i = 0; i < MAX_LEVELS_PER_MISSION; i++) {
	LevelStateName (szFile, i);
	if (CFile::Exist (szFile, gameFolders.szCacheDir, 0))
		CFile::Delete (szFile, gameFolders.szCacheDir);
	}
memset (nLevelState, 0, sizeof (nLevelState));
}

//------------------------------------------------------------------------------
// eof

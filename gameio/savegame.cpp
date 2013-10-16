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

#ifndef _WIN32
#	include <arpa/inet.h>
#	include <netinet/in.h> /* for htons & co. */
#endif

#include "pstypes.h"
#include "mono.h"
#include "descent.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "audio.h"
#include "loadgeometry.h"
#include "loadobjects.h"
#include "error.h"
#include "segmath.h"
#include "menu.h"
#include "trigger.h"
#include "game.h"
#include "screens.h"
#include "menu.h"
#include "gamepal.h"
#include "cfile.h"
#include "producers.h"
#include "hash.h"
#include "key.h"
#include "piggy.h"
#include "player.h"
#include "reactor.h"
#include "morph.h"
#include "weapon.h"
#include "rendermine.h"
#include "ogl_render.h"
#include "loadgame.h"
#include "cockpit.h"
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
#include "fireweapon.h"
#include "omega.h"
#include "multibot.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "savegame.h"
#include "strutil.h"
#include "light.h"
#include "dynlight.h"
#include "ipx.h"
#include "gr.h"
#include "marker.h"
#include "hogfile.h"

#define STATE_VERSION				58
#define STATE_COMPATIBLE_VERSION 20
// 0 - Put DGSS (Descent Game State Save) nId at tof.
// 1 - Added Difficulty level save
// 2 - Added gameStates.app.cheats.bEnabled flag
// 3 - Added between levels save.
// 4 - Added mission support
// 5 - Mike changed ai and CObject structure.
// 6 - Added buggin' cheat save
// 7 - Added other cheat saves and game_id.
// 8 - Added AI stuff for escort and thief.
// 9 - Save palette with screen shot
// 12- Saved last_was_super array
// 13- Saved palette flash stuff
// 14- Save cloaking CWall stuff
// 15- Save additional ai info
// 16- Save gameData.render.lights.subtracted
// 17- New marker save
// 18- Took out saving of old cheat status
// 19- Saved cheats_enabled flag
// 20- gameStates.app.bFirstSecretVisit
// 22- gameData.omega.xCharge

void SetFunctionMode (int);
void ShowLevelIntro (int level_num);
void DoCloakInvulSecretStuff (fix xOldGameTime);
void CopyDefaultsToRobot (CObject *objP);
void MultiInitiateSaveGame (int bSecret);
void MultiInitiateRestoreGame (int bSecret);
void ApplyAllChangedLight (void);
void DoLunacyOn (void);
void DoLunacyOff (void);
void FixWalls (void);

int m_nLastSlot= 0;

char dgss_id [4] = {'D', 'G', 'S', 'S'};

void ComputeAllStaticLight (void);

void GameRenderFrame (void);

#define	DESC_OFFSET	8

#ifdef _WIN32_WCE
# define errno -1
# define strerror (x) "Unknown Error"
#endif

#define SECRETB_FILENAME	"secret.sgb"
#define SECRETC_FILENAME	"secret.sgc"

CSaveGameManager saveGameManager;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

typedef struct tSaveGameInfo {
	char		szLabel [DESC_LENGTH + 16];
	char		szTime [DESC_LENGTH + 16];
	CBitmap	*image;
} tSaveGameInfo;

class CSaveGameInfo {
	private:
		tSaveGameInfo	m_info;
	public:
		CSaveGameInfo () { Init (); }
		~CSaveGameInfo () {};
		void Init (void);
		inline char* Label (void) { return m_info.szLabel; }
		inline char* Time (void) { return m_info.szTime; }
		inline CBitmap* Image (void) { return m_info.image; }
		bool Load (char *filename, int nSlot);
		void Destroy (void);
};


//------------------------------------------------------------------------------

void CSaveGameInfo::Init (void)
{
memset (&m_info, 0, sizeof (m_info));
strcpy (m_info.szLabel, TXT_EMPTY);
}

//------------------------------------------------------------------------------

void CSaveGameInfo::Destroy (void)
{
if (m_info.image) {
	delete m_info.image;
	m_info.image = NULL;
	}
}

//------------------------------------------------------------------------------

bool CSaveGameInfo::Load (char *filename, int nSlot)
{
	CFile	cf;
	int	nId, nVersion;

Init ();
if (!cf.Open (filename, gameFolders.szSaveDir, "rb", 0))
	return false;
	//Read nId
cf.Read (&nId, sizeof (char) * 4, 1);
if (memcmp (&nId, dgss_id, 4)) {
	cf.Close ();
	return false;
	}
cf.Read (&nVersion, sizeof (int), 1);
if (nVersion < STATE_COMPATIBLE_VERSION) {
	cf.Close ();
	return false;
	}
if (nSlot < 0)
	cf.Read (m_info.szLabel, DESC_LENGTH, 1);
else {
	if (nSlot < NUM_SAVES)
		sprintf (m_info.szLabel, "%d. ", nSlot + 1);
	else
		strcpy (m_info.szLabel, "   ");
	cf.Read (m_info.szLabel + 3, DESC_LENGTH, 1);
	if (nVersion < 26) {
		m_info.image = CBitmap::Create (0, THUMBNAIL_W, THUMBNAIL_H, 1);
		m_info.image->Read (cf, THUMBNAIL_W * THUMBNAIL_H);
		}
	else {
		m_info.image = CBitmap::Create (0, THUMBNAIL_LW, THUMBNAIL_LH, 1);
		m_info.image->Read (cf, THUMBNAIL_LW * THUMBNAIL_LH);
		}
	if (nVersion >= 9) {
		CPalette palette;
		palette.Read (cf);
		m_info.image->SetPalette (&palette, -1, -1);
		}
	struct tm	*t;
	int			h;
#ifdef _WIN32
	char	fn [FILENAME_LEN];

	struct _stat statBuf;
	sprintf (fn, "%s/%s", gameFolders.szSaveDir, filename);
	h = _stat (fn, &statBuf);
#else
	struct stat statBuf;
	h = stat (filename, &statBuf);
#endif
	if (!h && (t = localtime (&statBuf.st_mtime)))
		sprintf (m_info.szTime, " [%d-%d-%d %d:%02d:%02d]",
			t->tm_mon + 1, t->tm_mday, t->tm_year + 1900,
			t->tm_hour, t->tm_min, t->tm_sec);
	}
cf.Close ();
return true;
}

CSaveGameInfo saveGameInfo [NUM_SAVES + 1];

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#define NM_IMG_SPACE	6

int SaveStateMenuCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
	int		x, y, i = nCurItem - NM_IMG_SPACE;
	char		c = KeyToASCII (key);

if (nState) { // render
	if (i < 0)
		return nCurItem;
	CBitmap*	image = saveGameInfo [i].Image ();
	if (!image)
		return nCurItem;
	if (gameStates.menus.bHires) {
		int nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_NONE);
		x = gameData.X ((CCanvas::Current ()->Width () - CMenu::Scaled (image->Width ())) / 2);
		y = menu [0].m_y - CMenu::Scaled (16);
		//paletteManager.ResumeEffect (gameStates.app.bGameRunning);
		//BlitClipped (x, y, image);
		image->RenderFixed (NULL, x, y, CMenu::Scaled (image->Width ()), CMenu::Scaled (image->Height ()));
		if (gameOpts->menus.nStyle) {
			CCanvas::Current ()->SetColorRGBi (RGB_PAL (0, 0, 32));
			OglDrawEmptyRect (x - 1, y - 1, x + CMenu::Scaled (image->Width ()) + 1, y + CMenu::Scaled (image->Height ()) + 1);
			}
		gameData.SetStereoOffsetType (nOffsetSave);
		}
	else {
		saveGameInfo [nCurItem - 1].Image ()->BlitClipped ((CCanvas::Current ()->Width ()-THUMBNAIL_W) / 2, menu [0].m_y - 5);
		}
	}
else { // input
	if (nCurItem < 2)
		return nCurItem;
	if ((c >= '1') && (c <= '9')) {
		for (i = 0; i < NUM_SAVES; i++)
			if (menu [i + NM_IMG_SPACE].m_text [0] == c) {
				key = -2;
				return -(i + NM_IMG_SPACE) - 1;
				}
		}
	if (!menu [NM_IMG_SPACE - 1].m_text || strcmp (menu [NM_IMG_SPACE - 1].m_text, saveGameInfo [i].Time ())) {
		menu [NM_IMG_SPACE - 1].SetText (saveGameInfo [i].Time ());
		menu [NM_IMG_SPACE - 1].m_bRebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

int LoadStateMenuCallback (int nitems, CMenuItem *menu, int key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	int	i = nCurItem - NM_IMG_SPACE;
	char	c = KeyToASCII (key);

if (nCurItem < 2)
	return nCurItem;
if ((c >= '1') && (c <= '9')) {
	for (i = 0; i < NUM_SAVES; i++)
		if (menu [i].m_text [0] == c) {
			key = -2;
			return -i - 1;
			}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

char *strrpad (char * str, int len)
{
str += len;
*str = '\0';
for (; len; len--)
	if (*(--str))
		*str = ' ';
	else {
		*str = ' ';
		break;
		}
return str;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CSaveGameManager::Init (void)
{
m_nDefaultSlot = 0;
m_nLastSlot = 0;
}

//------------------------------------------------------------------------------

int CSaveGameManager::GetSaveFile (int bMulti)
{
	CMenu	m (NUM_SAVES + 2);
	int	i, menuRes;
	char	filename [NUM_SAVES + 1][30];

	static int choice = 0;

for (i = 0; i < NUM_SAVES; i++) {
	sprintf (filename [i], bMulti ? "%s.mg%x" : "%s.sg%x", LOCALPLAYER.callsign, i);
	saveGameInfo [i].Load (filename [i], -1);
	m.AddInputBox ("", saveGameInfo [i].Label (), DESC_LENGTH - 1, -1, NULL);
	}

m_nLastSlot = -1;
choice = m_nDefaultSlot;
menuRes = m.Menu (NULL, TXT_SAVE_GAME_MENU, NULL, &choice);

if (menuRes >= 0) {
	strcpy (m_filename, filename [choice]);
	strcpy (m_description, saveGameInfo [choice].Label ());
	if (!*m_description) {
		strncpy (m_description, missionManager.szCurrentLevel, sizeof (m_description) - 1);
		m_description [sizeof (m_description) - 1] = '\0';
		}
	}
for (i = 0; i < NUM_SAVES; i++)
	saveGameInfo [i].Destroy ();
m_nDefaultSlot = choice;
return (menuRes < 0) ? 0 : choice + 1;
}

//------------------------------------------------------------------------------

int bRestoringMenu = 0;

int CSaveGameManager::GetLoadFile (int bMulti)
{
	CMenu	m (NUM_SAVES + NM_IMG_SPACE + 1);
	int	i, choice = -1, nSaves;
	char	filename [NUM_SAVES + 1][30];

nSaves = 0;
for (i = 0; i < NM_IMG_SPACE; i++) {
	m.AddText ("", "");
	m.Top ()->m_bNoScroll = 1;
	}
for (i = 0; i < NUM_SAVES + 1; i++) {
	sprintf (filename [i], bMulti ? "%s.mg%x" : "%s.sg%x", LOCALPLAYER.callsign, i);
	if (saveGameInfo [i].Load (filename [i], i)) {
		m.AddMenu ("", saveGameInfo [i].Label (), (i < NUM_SAVES) ? -1 : 0, NULL);
		nSaves++;
		}
	else {
		m.AddMenu ("", saveGameInfo [i].Label ());
		}
	}
if (nSaves < 1) {
	MsgBox (NULL, BG_STANDARD, 1, "Ok", TXT_NO_SAVEGAMES);
	return 0;
	}
m_nLastSlot = -1;
bRestoringMenu = 1;
choice = m_nDefaultSlot + NM_IMG_SPACE;
i = m.Menu (NULL, TXT_LOAD_GAME_MENU, SaveStateMenuCallback, &choice, BG_SUBMENU, BG_STANDARD, 190, -1);
bRestoringMenu = 0;
if (i < 0)
	return 0;
choice -= NM_IMG_SPACE;
for (i = 0; i < NUM_SAVES + 1; i++)
	saveGameInfo [i].Destroy ();
if (choice >= 0) {
	strcpy (m_filename, filename [choice]);
	if (choice != NUM_SAVES + 1)		//no new default when restore from autosave
		m_nDefaultSlot = choice;
	return choice + 1;
	}
return 0;
}

//	-----------------------------------------------------------------------------------
//	Save file we're going to save over in last slot and call " [autosave backup]"

static char szBackup [DESC_LENGTH] = " [autosave backup]";

void CSaveGameManager::Backup (void)
{
if (!m_override) {
	CFile cf;
	
	if (cf.Exist (m_filename, gameFolders.szSaveDir, 0) && cf.Open (m_filename, gameFolders.szSaveDir, "wb", 0)) {
		char	newName [FILENAME_LEN];

		sprintf (newName, "%s.sg%x", LOCALPLAYER.callsign, NUM_SAVES);
		cf.Seek (DESC_OFFSET, SEEK_SET);
		int nWritten = (int) cf.Write (szBackup, 1, DESC_LENGTH);
		cf.Close ();
		if (nWritten == DESC_LENGTH) {
			cf.Delete (newName, gameFolders.szSaveDir);
			cf.Rename (m_filename, newName, gameFolders.szSaveDir);
			}
		}
	}
}

//	-----------------------------------------------------------------------------------
//	If not in multiplayer, do special secret level stuff.
//	If secret.sgc exists, then copy it to Nsecret.sgc (where N = nSaveSlot).
//	If it doesn't exist, then delete Nsecret.sgc

void CSaveGameManager::PushSecretSave (int nSaveSlot)
{
if ((nSaveSlot != -1) && !(m_bSecret || IsMultiGame)) {
	char	tempname [32];
	CFile m_cf;

	char fc = (nSaveSlot >= 10) ? (nSaveSlot-10) + 'a' : '0' + nSaveSlot;
	sprintf (tempname, "%csecret.sgc", fc);
	if (m_cf.Exist (tempname, gameFolders.szSaveDir, 0))
		m_cf.Delete (tempname, gameFolders.szSaveDir);
	if (m_cf.Exist (SECRETC_FILENAME, gameFolders.szSaveDir, 0))
		m_cf.Copy (SECRETC_FILENAME, tempname);
	}
}

//	-----------------------------------------------------------------------------------
//	blind_save means don't prompt user for any info.

int CSaveGameManager::Save (int bBetweenLevels, int bSecret, int bQuick, const char *pszFilenameOverride)
{
if (gameStates.app.bReadOnly)
	return 1;
if (gameStates.app.bPlayerIsDead)
	return 0;

	int	rval,nSaveSlot = -1;

m_override = pszFilenameOverride;
m_bBetweenLevels = bBetweenLevels;
m_bQuick = bQuick;
m_bSecret = bSecret;

if (IsMultiGame) {
	MultiInitiateSaveGame (m_bSecret);
	return 0;
	}

m_cf.Init ();
if ((missionManager.nCurrentLevel < 0) && !((m_bSecret > 0) || (gameOpts->gameplay.bSecretSave && missionConfig.m_bSecretSave) || gameStates.app.bD1Mission)) {
	HUDInitMessage (TXT_SECRET_SAVE_ERROR);
	return 0;
	}
if (gameStates.gameplay.bFinalBossIsDead)		//don't allow save while final boss is dying
	return 0;
//	If this is a secret save and the control center has been destroyed, don't allow
//	return to the base level.
if ((m_bSecret > 0) && gameData.reactor.bDestroyed) {
	CFile::Delete (SECRETB_FILENAME, gameFolders.szSaveDir);
	return 0;
	}
StopTime ();
gameData.app.bGamePaused = 1;
if (m_bQuick)
	sprintf (m_filename, "%s.quick", LOCALPLAYER.callsign);
else {
	if (m_bSecret == 1) {
		m_override = m_filename;
		sprintf (m_filename, SECRETB_FILENAME);
		} 
	else if (m_bSecret == 2) {
		m_override = m_filename;
		sprintf (m_filename, SECRETC_FILENAME);
		} 
	else {
		if (m_override) {
			strcpy (m_filename, m_override);
			sprintf (m_description, " [autosave backup]");
			}
		else if (!(nSaveSlot = GetSaveFile (0))) {
			gameData.app.bGamePaused = 0;
			StartTime (1);
			return 0;
			}
		}
	PushSecretSave (nSaveSlot);
	Backup ();
	}
if ((rval = SaveState (bSecret))) {
	if (bQuick)
		HUDInitMessage (TXT_QUICKSAVE);
	}
gameData.app.bGamePaused = 0;
StartTime (1);
return rval;
}

//------------------------------------------------------------------------------

void CSaveGameManager::SaveNetGame (void)
{
	int	i, j;

m_cf.WriteByte (netGame.m_info.nType);
m_cf.WriteInt (netGame.m_info.nSecurity);
m_cf.Write (netGame.m_info.szGameName, 1, NETGAME_NAME_LEN + 1);
m_cf.Write (netGame.m_info.szMissionTitle, 1, MISSION_NAME_LEN + 1);
m_cf.Write (netGame.m_info.szMissionName, 1, 9);
m_cf.WriteInt (netGame.m_info.GetLevel ());
m_cf.WriteByte ((sbyte) netGame.m_info.gameMode);
m_cf.WriteByte ((sbyte) netGame.m_info.bRefusePlayers);
m_cf.WriteByte ((sbyte) netGame.m_info.difficulty);
m_cf.WriteByte ((sbyte) netGame.m_info.gameStatus);
m_cf.WriteByte ((sbyte) netGame.m_info.nNumPlayers);
m_cf.WriteByte ((sbyte) netGame.m_info.nMaxPlayers);
m_cf.WriteByte ((sbyte) netGame.m_info.nConnected);
m_cf.WriteByte ((sbyte) netGame.m_info.gameFlags);
m_cf.WriteByte ((sbyte) netGame.m_info.protocolVersion);
m_cf.WriteByte ((sbyte) netGame.m_info.versionMajor);
m_cf.WriteByte ((sbyte) netGame.m_info.versionMinor);
m_cf.WriteByte ((sbyte) netGame.m_info.GetTeamVector ());
m_cf.WriteByte ((sbyte) netGame.m_info.DoMegas);
m_cf.WriteByte ((sbyte) netGame.m_info.DoSmarts);
m_cf.WriteByte ((sbyte) netGame.m_info.DoFusions);
m_cf.WriteByte ((sbyte) netGame.m_info.DoHelix);
m_cf.WriteByte ((sbyte) netGame.m_info.DoPhoenix);
m_cf.WriteByte ((sbyte) netGame.m_info.DoAfterburner);
m_cf.WriteByte ((sbyte) netGame.m_info.DoInvulnerability);
m_cf.WriteByte ((sbyte) netGame.m_info.DoCloak);
m_cf.WriteByte ((sbyte) netGame.m_info.DoGauss);
m_cf.WriteByte ((sbyte) netGame.m_info.DoVulcan);
m_cf.WriteByte ((sbyte) netGame.m_info.DoPlasma);
m_cf.WriteByte ((sbyte) netGame.m_info.DoOmega);
m_cf.WriteByte ((sbyte) netGame.m_info.DoSuperLaser);
m_cf.WriteByte ((sbyte) netGame.m_info.DoProximity);
m_cf.WriteByte ((sbyte) netGame.m_info.DoSpread);
m_cf.WriteByte ((sbyte) netGame.m_info.DoSmartMine);
m_cf.WriteByte ((sbyte) netGame.m_info.DoFlash);
m_cf.WriteByte ((sbyte) netGame.m_info.DoGuided);
m_cf.WriteByte ((sbyte) netGame.m_info.DoEarthShaker);
m_cf.WriteByte ((sbyte) netGame.m_info.DoMercury);
m_cf.WriteByte ((sbyte) netGame.m_info.bAllowMarkerView);
m_cf.WriteByte ((sbyte) netGame.m_info.bIndestructibleLights);
m_cf.WriteByte ((sbyte) netGame.m_info.DoAmmoRack);
m_cf.WriteByte ((sbyte) netGame.m_info.DoConverter);
m_cf.WriteByte ((sbyte) netGame.m_info.DoHeadlight);
m_cf.WriteByte ((sbyte) netGame.m_info.DoHoming);
m_cf.WriteByte ((sbyte) netGame.m_info.DoLaserUpgrade);
m_cf.WriteByte ((sbyte) netGame.m_info.DoQuadLasers);
m_cf.WriteByte ((sbyte) netGame.m_info.bShowAllNames);
m_cf.WriteByte ((sbyte) netGame.m_info.BrightPlayers);
m_cf.WriteByte ((sbyte) netGame.m_info.invul);
m_cf.WriteByte ((sbyte) netGame.m_info.FriendlyFireOff);
for (i = 0; i < 2; i++)
	m_cf.Write (netGame.m_info.szTeamName [i], 1, CALLSIGN_LEN + 1);		// 18 bytes
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	m_cf.WriteInt (*netGame.Locations (i));
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	for (j = 0; j < MAX_NUM_NET_PLAYERS; j++)
		m_cf.WriteShort (*netGame.Kills (i, j));			// 128 bytes
m_cf.WriteShort (netGame.GetSegmentCheckSum ());			// 2 bytes
for (i = 0; i < 2; i++)
	m_cf.WriteShort (*netGame.TeamKills (i));				// 4 bytes
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	m_cf.WriteShort (*netGame.Killed (i));					// 16 bytes
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	m_cf.WriteShort (*netGame.PlayerKills (i));			// 16 bytes
m_cf.WriteInt (netGame.GetScoreGoal ());						// 4 bytes
m_cf.WriteFix (netGame.GetPlayTimeAllowed ());				// 4 bytes
m_cf.WriteFix (netGame.GetLevelTime ());						// 4 bytes
m_cf.WriteInt (netGame.GetControlInvulTime ());				// 4 bytes
m_cf.WriteInt (netGame.GetMonitorVector ());		// 4 bytes
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	m_cf.WriteInt (*netGame.PlayerScore (i));				// 32 bytes
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	m_cf.WriteByte ((sbyte) *netGame.PlayerFlags (i));	// 8 bytes
m_cf.WriteShort (PacketsPerSec ());							// 2 bytes
m_cf.WriteByte (sbyte (netGame.GetShortPackets ()));		// 1 bytes
// 279 bytes
// 355 bytes total
m_cf.Write (netGame.AuxData (), NETGAME_AUX_SIZE, 1);  // Storage for protocol-specific data (e.g., multicast session and port)
}

//------------------------------------------------------------------------------

void CSaveGameManager::SaveNetPlayers (void)
{
	int	i;

m_cf.WriteByte ((sbyte) netPlayers [0].m_info.nType);
m_cf.WriteInt (netPlayers [0].m_info.nSecurity);
for (i = 0; i < MAX_NUM_NET_PLAYERS + 4; i++) {
	m_cf.Write (netPlayers [0].m_info.players [i].callsign, 1, CALLSIGN_LEN + 1);
	m_cf.Write (netPlayers [0].m_info.players [i].network.Server (), 1, 4);
	m_cf.Write (netPlayers [0].m_info.players [i].network.Node (), 1, 6);
	m_cf.WriteByte ((sbyte) netPlayers [0].m_info.players [i].versionMajor);
	m_cf.WriteByte ((sbyte) netPlayers [0].m_info.players [i].versionMinor);
	m_cf.WriteByte ((sbyte) netPlayers [0].m_info.players [i].computerType);
	m_cf.WriteByte (netPlayers [0].m_info.players [i].connected);
	m_cf.WriteShort ((short) netPlayers [0].m_info.players [i].socket);
	m_cf.WriteByte ((sbyte) netPlayers [0].m_info.players [i].rank);
	}
}

//------------------------------------------------------------------------------

void CSaveGameManager::SavePlayer (CPlayerData *playerP)
{
	int	i;

m_cf.Write (playerP->callsign, 1, CALLSIGN_LEN + 1); // The callsign of this CPlayerData, for net purposes.
m_cf.Write (playerP->netAddress, 1, 6);					// The network address of the player.
m_cf.WriteByte (playerP->connected);            // Is the player connected or not?
m_cf.WriteInt (playerP->nObject);                // What CObject number this CPlayerData is. (made an int by mk because it's very often referenced)
m_cf.WriteInt (playerP->nPacketsGot);         // How many packets we got from them
m_cf.WriteInt (playerP->nPacketsSent);        // How many packets we sent to them
m_cf.WriteInt ((int) playerP->flags);           // Powerup flags, see below...
m_cf.WriteFix (playerP->energy);                // Amount of energy remaining.
m_cf.WriteFix (playerP->Shield ());               // shield remaining (protection)
m_cf.WriteByte (playerP->lives);                // Lives remaining, 0 = game over.
m_cf.WriteByte (playerP->level);                // Current level CPlayerData is playing. (must be signed for secret levels)
m_cf.WriteByte ((sbyte) playerP->LaserLevel ());  // Current level of the laser.
m_cf.WriteByte (playerP->startingLevel);       // What level the player started on.
m_cf.WriteShort (playerP->nKillerObj);       // Who killed me.... (-1 if no one)
m_cf.WriteShort ((short) playerP->primaryWeaponFlags);   // bit set indicates the player has this weapon.
m_cf.WriteShort ((short) playerP->secondaryWeaponFlags); // bit set indicates the player has this weapon.
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	m_cf.WriteShort ((short) playerP->primaryAmmo [i]); // How much ammo of each nType.
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
	m_cf.WriteShort ((short) playerP->secondaryAmmo [i]); // How much ammo of each nType.
#if 1 //for inventory system
m_cf.WriteByte ((sbyte) playerP->nInvuls);
m_cf.WriteByte ((sbyte) playerP->nCloaks);
#endif
m_cf.WriteInt (playerP->lastScore);             // Score at beginning of current level.
m_cf.WriteInt (playerP->score);                  // Current score.
m_cf.WriteFix (playerP->timeLevel);             // Level time played
m_cf.WriteFix (playerP->timeTotal);             // Game time played (high word = seconds)
if (playerP->cloakTime == 0x7fffffff)				// cloak cheat active
	m_cf.WriteFix (playerP->cloakTime);			// Time invulnerable
else
	m_cf.WriteFix (playerP->cloakTime - gameData.time.xGame);      // Time invulnerable
if (playerP->invulnerableTime == 0x7fffffff)		// invul cheat active
	m_cf.WriteFix (playerP->invulnerableTime);      // Time invulnerable
else
	m_cf.WriteFix (playerP->invulnerableTime - gameData.time.xGame);      // Time invulnerable
m_cf.WriteShort (playerP->nScoreGoalCount);          // Num of players killed this level
m_cf.WriteShort (playerP->netKilledTotal);       // Number of times killed total
m_cf.WriteShort (playerP->netKillsTotal);        // Number of net kills total
m_cf.WriteShort (playerP->numKillsLevel);        // Number of kills this level
m_cf.WriteShort (playerP->numKillsTotal);        // Number of kills total
m_cf.WriteShort (playerP->numRobotsLevel);       // Number of initial robots this level
m_cf.WriteShort (playerP->numRobotsTotal);       // Number of robots total
m_cf.WriteShort ((short) playerP->hostages.nRescued); // Total number of hostages rescued.
m_cf.WriteShort ((short) playerP->hostages.nTotal);         // Total number of hostages.
m_cf.WriteByte ((sbyte) playerP->hostages.nOnBoard);      // Number of hostages on ship.
m_cf.WriteByte ((sbyte) playerP->hostages.nLevel);         // Number of hostages on this level.
m_cf.WriteFix (playerP->homingObjectDist);     // Distance of nearest homing CObject.
m_cf.WriteByte (playerP->hoursLevel);            // Hours played (since timeTotal can only go up to 9 hours)
m_cf.WriteByte (playerP->hoursTotal);            // Hours played (since timeTotal can only go up to 9 hours)
}

//------------------------------------------------------------------------------

#if 0

void CSaveGameManager::SaveObjTriggerRef (tObjTriggerRef *refP)
{
m_cf.WriteShort (refP->prev);
m_cf.WriteShort (refP->next);
m_cf.WriteShort (refP->nObject);
}

#endif

//------------------------------------------------------------------------------

void CSaveGameManager::SaveObjectProducer (tObjectProducerInfo *objProducerP)
{
	int	i;

for (i = 0; i < 2; i++)
	m_cf.WriteInt (objProducerP->objFlags [i]);
m_cf.WriteFix (objProducerP->xHitPoints);
m_cf.WriteFix (objProducerP->xInterval);
m_cf.WriteShort (objProducerP->nSegment);
m_cf.WriteShort (objProducerP->nProducer);
}

//------------------------------------------------------------------------------

void CSaveGameManager::SaveProducer (tProducerInfo *producerP)
{
m_cf.WriteInt (producerP->nType);
m_cf.WriteInt (producerP->nSegment);
m_cf.WriteByte (producerP->bFlag);
m_cf.WriteByte (producerP->bEnabled);
m_cf.WriteByte (producerP->nLives);
m_cf.WriteFix (producerP->xCapacity);
m_cf.WriteFix (producerP->xMaxCapacity);
m_cf.WriteFix (producerP->xTimer);
m_cf.WriteFix (producerP->xDisableTime);
m_cf.WriteVector(producerP->vCenter);
}

//------------------------------------------------------------------------------

void CSaveGameManager::SaveReactorTrigger (CTriggerTargets *triggerP)
{
m_cf.WriteShort (triggerP->m_nLinks);
triggerP->SaveState (m_cf);
}

//------------------------------------------------------------------------------

void CSaveGameManager::SaveReactorState (tReactorStates *stateP)
{
m_cf.WriteInt (stateP->nObject);
m_cf.WriteInt (stateP->bHit);
m_cf.WriteInt (stateP->bSeenPlayer);
m_cf.WriteInt (stateP->nNextFireTime);
m_cf.WriteInt (stateP->nDeadObj);
}

//------------------------------------------------------------------------------

void CSaveGameManager::SaveSpawnPoint (int i)
{
m_cf.WriteVector (gameData.multiplayer.playerInit [i].position.vPos);     
m_cf.WriteMatrix (gameData.multiplayer.playerInit [i].position.mOrient);  
m_cf.WriteShort (gameData.multiplayer.playerInit [i].nSegment);
m_cf.WriteShort (gameData.multiplayer.playerInit [i].nSegType);
}

//------------------------------------------------------------------------------

void CSaveGameManager::SaveGameData (void)
{
	int		i, j;
	CObject	*objP;

m_cf.WriteInt (gameData.segs.nMaxSegments);
// Save the Between levels flag...
m_cf.WriteInt (m_bBetweenLevels);
m_cf.WriteInt (gameOpts->app.bEnableMods);
// Save the mission info...
m_cf.Write (missionManager [missionManager.nCurrentMission].filename, sizeof (char), 9);
//Save level info
m_cf.WriteInt (missionManager.nCurrentLevel);
m_cf.WriteInt (missionManager.NextLevel ());
//Save gameData.time.xGame
m_cf.WriteFix (gameData.time.xGame);
// If coop save, save all
if (IsCoopGame) {
	m_cf.WriteInt (gameData.app.nStateGameId);
	SaveNetGame ();
	SaveNetPlayers ();
	m_cf.WriteInt (gameData.multiplayer.nPlayers);
	m_cf.WriteInt (N_LOCALPLAYER);
	for (i = 0; i < gameData.multiplayer.nPlayers; i++)
		SavePlayer (gameData.multiplayer.players + i);
	}
//Save CPlayerData info
SavePlayer (gameData.multiplayer.players + N_LOCALPLAYER);
// Save the current weapon info
m_cf.WriteByte (gameData.weapons.nPrimary);
m_cf.WriteByte (gameData.weapons.nSecondary);
// Save the difficulty level
m_cf.WriteInt (gameStates.app.nDifficultyLevel);
// Save cheats enabled
m_cf.WriteInt (gameStates.app.cheats.bEnabled);
if (STATE_VERSION < 55)
	m_cf.WriteInt (gameOpts->app.bEnableMods);
for (i = 0; i < 2; i++) {
	m_cf.WriteInt (F2X (gameStates.gameplay.slowmo [i].fSpeed));
	m_cf.WriteInt (gameStates.gameplay.slowmo [i].nState);
	}
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	m_cf.WriteInt (gameData.multiplayer.weaponStates [i].bTripleFusion);
if (!m_bBetweenLevels) {
//Finish all morph OBJECTS
	FORALL_OBJS (objP, i) {
	if (objP->info.nType == OBJ_NONE) 
			continue;
		if (objP->info.nType == OBJ_CAMERA)
			objP->info.position.mOrient = cameraManager.Camera (objP)->Orient ();
		else if (objP->info.renderType == RT_MORPH) {
			tMorphInfo *md = MorphFindData (objP);
			if (md) {
				CObject *mdObjP = md->objP;
				mdObjP->info.controlType = md->saveControlType;
				mdObjP->info.movementType = md->saveMovementType;
				mdObjP->info.renderType = RT_POLYOBJ;
				mdObjP->mType.physInfo = md->savePhysInfo;
				md->objP = NULL;
				} 
			else {						//maybe loaded half-morphed from disk
				objP->Die ();
				objP->info.renderType = RT_POLYOBJ;
				objP->info.controlType = CT_NONE;
				objP->info.movementType = MT_NONE;
				}
			}
		}
//Save CObject info
	i = gameData.objs.nLastObject [0] + 1;
	m_cf.WriteInt (i);
	for (j = 0; j < i; j++)
		OBJECTS [j].SaveState (m_cf);
//Save CWall info
	i = gameData.walls.nWalls;
	m_cf.WriteInt (i);
	for (j = 0; j < i; j++)
		WALLS [j].SaveState (m_cf);
//Save exploding wall info
	i = int (gameData.walls.exploding.ToS ());
	m_cf.WriteInt (i);
	for (j = 0; j < i; j++)
		gameData.walls.exploding [j].SaveState (m_cf);
//Save door info
	i = gameData.walls.activeDoors.ToS ();
	m_cf.WriteInt (i);
	for (j = 0; j < i; j++)
		gameData.walls.activeDoors [j].SaveState (m_cf);
//Save cloaking CWall info
	i = gameData.walls.cloaking.ToS ();
	m_cf.WriteInt (i);
	for (j = 0; j < i; j++)
		gameData.walls.cloaking [j].SaveState (m_cf);
//Save CTrigger info
	m_cf.WriteInt (gameData.trigs.m_nTriggers);
	for (i = 0; i < gameData.trigs.m_nTriggers; i++)
		TRIGGERS [i].SaveState (m_cf);
	m_cf.WriteInt (gameData.trigs.m_nObjTriggers);
	if (!gameData.trigs.m_nObjTriggers)
		m_cf.WriteShort (0);
	else {
		for (i = 0; i < gameData.trigs.m_nObjTriggers; i++)
			OBJTRIGGERS [i].SaveState (m_cf, true);
#if 0
		for (i = 0; i < gameData.trigs.m_nObjTriggers; i++)
			SaveObjTriggerRef (gameData.trigs.objTriggerRefs + i);
		nObjsWithTrigger = 0;
		FORALL_OBJS (objP, nObject) {
			nObject = objP->Index ();
			nFirstTrigger = gameData.trigs.firstObjTrigger [nObject];
			if ((nFirstTrigger >= 0) && (nFirstTrigger < gameData.trigs.m_nObjTriggers))
				nObjsWithTrigger++;
			}
		m_cf.WriteShort (nObjsWithTrigger);
		FORALL_OBJS (objP, nObject) {
			nObject = objP->Index ();
			nFirstTrigger = gameData.trigs.firstObjTrigger [nObject];
			if ((nFirstTrigger >= 0) && (nFirstTrigger < gameData.trigs.m_nObjTriggers)) {
				m_cf.WriteShort (nObject);
				m_cf.WriteShort (nFirstTrigger);
				}
			}
#endif
		}
//Save tmap info
	for (i = 0; i <= gameData.segs.nLastSegment; i++)
		SEGMENTS [i].SaveState (m_cf);
// Save the producer info
	m_cf.WriteInt (gameData.reactor.bDestroyed);
	m_cf.WriteFix (gameData.reactor.countdown.nTimer);
	m_cf.WriteInt (gameData.producers.nRobotMakers);
	for (i = 0; i < gameData.producers.nRobotMakers; i++)
		SaveObjectProducer (gameData.producers.robotMakers + i);
	m_cf.WriteInt (gameData.producers.nEquipmentMakers);
	for (i = 0; i < gameData.producers.nEquipmentMakers; i++)
		SaveObjectProducer (gameData.producers.equipmentMakers + i);
	SaveReactorTrigger (&gameData.reactor.triggers);
	m_cf.WriteInt (gameData.producers.nProducers);
	for (i = 0; i < gameData.producers.nProducers; i++)
		SaveProducer (gameData.producers.producers + i);
// Save the control cen info
	m_cf.WriteInt (gameData.reactor.bPresent);
	for (i = 0; i < MAX_BOSS_COUNT; i++)
		SaveReactorState (gameData.reactor.states + i);
// Save the AI state
	SaveAI ();

// Save the automap visited info
	for (i = 0; i < LEVEL_SEGMENTS; i++)
		m_cf.WriteShort (automap.m_visited [i]);
	}
m_cf.WriteInt ((int) gameData.app.nStateGameId);
m_cf.WriteInt (gameStates.app.cheats.bLaserRapidFire);
m_cf.WriteInt (gameStates.app.bLunacy);		//	Yes, writing this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
m_cf.WriteInt (gameStates.gameplay.bMineMineCheat);
// Save automap marker info
markerManager.SaveState (m_cf);
m_cf.WriteFix (gameData.physics.xAfterburnerCharge);
//save last was super information
m_cf.Write (bLastPrimaryWasSuper, sizeof (bLastPrimaryWasSuper), 1);
m_cf.Write (bLastSecondaryWasSuper, sizeof (bLastSecondaryWasSuper), 1);
//	Save flash effect stuff
m_cf.WriteFix (paletteManager.FlashDuration ());
m_cf.WriteFix (paletteManager.LastEffectTime ());
m_cf.WriteShort (paletteManager.RedEffect ());
m_cf.WriteShort (paletteManager.GreenEffect ());
m_cf.WriteShort (paletteManager.BlueEffect ());
gameData.render.lights.subtracted.Write (m_cf, LEVEL_SEGMENTS);
m_cf.WriteInt (gameStates.app.bFirstSecretVisit);
m_cf.WriteFix (gameData.omega.xCharge [0]);
m_cf.WriteShort (missionManager.nEntryLevel);
m_cf.WriteInt (gameStates.gameplay.seismic.nShakeFrequency);
m_cf.WriteInt (gameStates.gameplay.seismic.nShakeDuration);

for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	SaveSpawnPoint (i);
m_cf.WriteInt (gameOpts->gameplay.nShip [0]);
}

//------------------------------------------------------------------------------

void CSaveGameManager::SaveImage (void)
{
	CBitmap	bmIn, bmOut;
	int		x, y;

bmOut.SetWidth (THUMBNAIL_LW);
bmOut.SetHeight (THUMBNAIL_LH);
bmOut.SetBPP (1);
bmOut.CreateBuffer ();

bmIn.SetWidth ((gameData.render.screen.Width (false) / THUMBNAIL_LW) * THUMBNAIL_LW);
bmIn.SetHeight (bmIn.Width () * 3 / 5);	//force 5:3 aspect ratio
if (bmIn.Height () > gameData.render.screen.Height (false)) {
	bmIn.SetHeight ((gameData.render.screen.Height (false) / THUMBNAIL_LH) * THUMBNAIL_LH);
	bmIn.SetWidth (bmIn.Height () * 5 / 3);
	}
x = (gameData.render.screen.Width (false) - bmIn.Width ()) / 2;
y = (gameData.render.screen.Height (false) - bmIn.Height ()) / 2;
bmIn.SetBPP (3);
bmIn.CreateBuffer ();

if (bmIn.Buffer () && bmOut.Buffer ()) {
	gameStates.render.nFrameFlipFlop = !gameStates.render.nFrameFlipFlop;
	if (ogl.m_states.nDrawBuffer == GL_BACK) {
		gameStates.render.nFrameFlipFlop = !gameStates.render.nFrameFlipFlop;
		GameRenderFrame ();
		}
	else
		RenderFrame (0, 0);
	//ogl.SetTexturing (false);
	ogl.SetReadBuffer (GL_FRONT, 0);
	glReadPixels (x, y, bmIn.Width (), bmIn.Height (), GL_RGB, GL_UNSIGNED_BYTE, bmIn.Buffer ());
	// do a nice, half-way smart (by merging pixel groups using their average color) image resize
	CTGA tga (&bmIn);
	tga.Shrink (bmIn.Width () / THUMBNAIL_LW, bmIn.Height () / THUMBNAIL_LH, 0);
	//paletteManager.ResumeEffect ();
	// convert the resized TGA to bmp
	ubyte* inBuf = bmIn.Buffer ();
	ubyte* outBuf = bmOut.Buffer ();
	for (y = 0; y < THUMBNAIL_LH; y++) {
		int i = y * THUMBNAIL_LW * 3;
		int j = (THUMBNAIL_LH - y - 1) * THUMBNAIL_LW;
		for (x = 0; x < THUMBNAIL_LW; x++, j++, i += 3)
			outBuf [j] = paletteManager.Game ()->ClosestColor (inBuf [i] / 4, inBuf [i + 1] / 4, inBuf [i + 2] / 4);
		}
	//paletteManager.ResumeEffect ();
	bmIn.DestroyBuffer ();
	bmOut.Write (m_cf, THUMBNAIL_LW * THUMBNAIL_LH);
	CCanvas::Pop ();
	bmOut.Destroy ();
	m_cf.Write (paletteManager.Game (), 3, 256);
	}
else {
 	ubyte color = 0;
 	for (int i = 0; i < THUMBNAIL_LW * THUMBNAIL_LH; i++)
		m_cf.Write (&color, sizeof (ubyte), 1);
	}
}

//------------------------------------------------------------------------------

int CSaveGameManager::SaveState (int bSecret, char *filename, char *description)
{
	int	i;

StopTime ();
if (filename)
	strcpy (m_filename, filename);
if (description)
	strcpy (m_description, description);
m_bSecret = bSecret;
if (!m_cf.Open (m_filename, (bSecret < 0) ? gameFolders.szCacheDir : gameFolders.szSaveDir, "wb", 0)) {
	if (!IsMultiGame)
		MsgBox (NULL, BG_STANDARD, 1, TXT_OK, TXT_SAVE_ERROR2);
	StartTime (1);
	return 0;
	}

//Save nId
m_cf.Write (dgss_id, sizeof (char) * 4, 1);
//Save m_nVersion
i = STATE_VERSION;
m_cf.Write (&i, sizeof (int), 1);
//Save description
m_cf.Write (m_description, sizeof (char) * DESC_LENGTH, 1);
// Save the current screen shot...
SaveImage ();
SaveGameData ();
if (!bSecret) {
	for (int i = 0; i < MAX_LEVELS_PER_MISSION; i++)
		m_cf.WriteByte (sbyte (missionManager.GetLevelState (i)));
	}
if (m_cf.Error ()) {
	if (!IsMultiGame) {
		MsgBox (NULL, BG_STANDARD, 1, TXT_OK, TXT_SAVE_ERROR);
		m_cf.Close ();
		m_cf.Delete (m_filename, gameFolders.szSaveDir);
		}
	}
else 
	m_cf.Close ();
StartTime (1);
return 1;
}

//	-----------------------------------------------------------------------------------

void CSaveGameManager::AutoSave (int nSaveSlot)
{
if ((nSaveSlot != (NUM_SAVES + 1)) && m_bInGame) {
	char	filename [FILENAME_LEN];
	const char *pszOverride = m_override;
	strcpy (filename, m_filename);
	sprintf (m_filename, "%s.sg%x", LOCALPLAYER.callsign, NUM_SAVES);
	Save (0, m_bSecret, 0, m_filename);
	m_override = pszOverride;
	strcpy (m_filename, filename);
	}
}

//	-----------------------------------------------------------------------------------

void CSaveGameManager::PopSecretSave (int nSaveSlot)
{
if ((nSaveSlot != -1) && !(m_bSecret || IsMultiGame)) {
	char	tempname [32];

	char fc = (nSaveSlot >= 10) ? (nSaveSlot-10) + 'a' : '0' + nSaveSlot;
	sprintf (tempname, "%csecret.sgc", fc);
	if (m_cf.Exist (tempname, gameFolders.szSaveDir, 0))
		m_cf.Copy (tempname, SECRETC_FILENAME);
	else
		m_cf.Delete (SECRETC_FILENAME, gameFolders.szSaveDir);
	}
}

//	-----------------------------------------------------------------------------------

int CSaveGameManager::Load (int bInGame, int bSecret, int bQuick, const char *pszFilenameOverride)
{
if (gameStates.app.bPlayerIsDead)
	return 0;

	int	i, nSaveSlot = -1;

m_bInGame = bInGame;
m_bSecret = bSecret;
m_bQuick = bQuick;
m_override = pszFilenameOverride;
m_bBetweenLevels = 0;

if (IsMultiGame) {
	MultiInitiateRestoreGame (bSecret);
	return 0;
	}

if (gameData.demo.nState == ND_STATE_RECORDING)
	NDStopRecording ();
if (gameData.demo.nState != ND_STATE_NORMAL)
	return 0;
StopTime ();
gameData.app.bGamePaused = 1;
if (m_bQuick) {
	sprintf (m_filename, "%s.quick", LOCALPLAYER.callsign);
	if (!CFile::Exist (m_filename, gameFolders.szSaveDir, 0))
		m_bQuick = 0;
	}
if (!m_bQuick) {
	if (m_override) {
		strcpy (m_filename, m_override);
		nSaveSlot = NUM_SAVES + 1;		//	So we don't trigger autosave
		}
	else if (!(nSaveSlot = GetLoadFile (0))) {
		gameData.app.bGamePaused = 0;
		StartTime (1);
		return 0;
		}
	PopSecretSave (nSaveSlot);
	AutoSave (nSaveSlot);
	if (!bSecret && bInGame) {
		int choice = MsgBox (NULL, BG_STANDARD, 2, TXT_YES, TXT_NO, TXT_CONFIRM_LOAD);
		if (choice != 0) {
			gameData.app.bGamePaused = 0;
			StartTime (1);
			return 0;
			}
		}
	}
gameStates.app.bGameRunning = 0;
i = LoadState (0, bSecret);
gameData.app.bGamePaused = 0;
if (i) {
	/*---*/PrintLog (1, "rebuilding OpenGL context\n");
	ogl.SetRenderQuality ();
	ogl.RebuildContext (1);
	PrintLog (-1);
	if (bQuick)
		HUDInitMessage (TXT_QUICKLOAD);
	}
StartTime (1);
return i;
}

//------------------------------------------------------------------------------

int CSaveGameManager::ReadBoundedInt (int nMax, int *nVal)
{
	int	i;

i = m_cf.ReadInt ();
if ((i < 0) || (i > nMax)) {
	Warning (TXT_SAVE_CORRUPT);
	//m_cf.Close ();
	return 1;
	}
*nVal = i;
return 0;
}

//------------------------------------------------------------------------------

int CSaveGameManager::LoadMission (void)
{
	char	szMission [16];
	int	i, nVersionFilter = gameOpts->app.nVersionFilter;

m_cf.Read (szMission, sizeof (char), 9);
szMission [9] = '\0';
gameOpts->app.nVersionFilter = 3;
i = missionManager.LoadByName (szMission, 0, "downloads/");
if (!i)
	i = missionManager.LoadByName (szMission, -1);
gameOpts->app.nVersionFilter = nVersionFilter;
if (i)
	return 1;
MsgBox (NULL, BG_STANDARD, 1, "Ok", TXT_MSN_LOAD_ERROR, szMission);
return 0;
}

//------------------------------------------------------------------------------

void CSaveGameManager::LoadMulti (char *pszOrgCallSign, int bMulti)
{
if (bMulti)
	strcpy (pszOrgCallSign, LOCALPLAYER.callsign);
else {
	gameData.app.SetGameMode (GM_NORMAL);
	SetFunctionMode (FMODE_GAME);
	ChangePlayerNumTo (0);
	strcpy (pszOrgCallSign, gameData.multiplayer.players [0].callsign);
	gameData.multiplayer.nPlayers = 1;
	if (!m_bSecret) {
		InitMultiPlayerObject (0);	//make sure CPlayerData's CObject set up
		}
	}
}

//------------------------------------------------------------------------------

int CSaveGameManager::SetServerPlayer (
	CPlayerData *restoredPlayers, int nPlayers, const char *pszServerCallSign, int *pnOtherObjNum, int *pnServerObjNum)
{
	int	i,
			nServerPlayer = -1,
			nOtherObjNum = -1, 
			nServerObjNum = -1;

if (gameStates.multi.nGameType >= IPX_GAME) {
	if (IAmGameHost ())
		nServerPlayer = N_LOCALPLAYER;
	else {
		nServerPlayer = -1;
		for (i = 0; i < nPlayers; i++)
			if (!strcmp (pszServerCallSign, netPlayers [0].m_info.players [i].callsign)) {
				nServerPlayer = i;
				break;
				}
		}
	if (nServerPlayer > 0) {
		nOtherObjNum = restoredPlayers [0].nObject;
		nServerObjNum = restoredPlayers [nServerPlayer].nObject;
		Swap (netPlayers [0].m_info.players [0], netPlayers [0].m_info.players [nServerPlayer]);
		Swap (restoredPlayers [0], restoredPlayers [nServerPlayer]);
		if (IAmGameHost ())
			N_LOCALPLAYER = 0;
		else if (!N_LOCALPLAYER)
			N_LOCALPLAYER = nServerPlayer;
		}
#if 0
	memcpy (netPlayers [0].m_info.players [N_LOCALPLAYER].network.Node (), IpxGetMyLocalAddress (), 6);
	netPlayers [0].m_info.players [N_LOCALPLAYER].network.Port () = 
		htons (netPlayers [0].m_info.players [N_LOCALPLAYER].network.Port ());
#endif
	}
*pnOtherObjNum = nOtherObjNum;
*pnServerObjNum = nServerObjNum;
return nServerPlayer;
}

//------------------------------------------------------------------------------

void CSaveGameManager::GetConnectedPlayers (CPlayerData *restoredPlayers, int nPlayers)
{
	int	i, j;

for (i = 0; i < nPlayers; i++) {
	for (j = 0; j < nPlayers; j++) {
		if (!stricmp (restoredPlayers [i].callsign, gameData.multiplayer.players [j].callsign)) {
			if (gameData.multiplayer.players [j].connected) {
				if (gameStates.multi.nGameType == UDP_GAME) {
					memcpy (restoredPlayers [i].netAddress, gameData.multiplayer.players [j].netAddress, 
							  sizeof (gameData.multiplayer.players [j].netAddress));
					memcpy (netPlayers [0].m_info.players [i].network.Node (), gameData.multiplayer.players [j].netAddress, 
							  sizeof (gameData.multiplayer.players [j].netAddress));
					}
				restoredPlayers [i].Connect ((sbyte) CONNECT_PLAYING);
				break;
				}
			}
		}
	}
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++) {
	if (!restoredPlayers [i].connected) {
		memset (restoredPlayers [i].netAddress, 0xFF, sizeof (restoredPlayers [i].netAddress));
		memset (netPlayers [0].m_info.players [i].network.Node (), 0xFF, 6);
		}
	}
for (i = 0; i < nPlayers; i++) 
	(CPlayerInfo&) gameData.multiplayer.players [i] = (CPlayerInfo&) restoredPlayers [i];
gameData.multiplayer.nPlayers = nPlayers;
if (IAmGameHost ()) {
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (i == N_LOCALPLAYER)
			continue;
   	CONNECT (i, CONNECT_DISCONNECTED);
		}
	}
}

//------------------------------------------------------------------------------

void CSaveGameManager::FixNetworkObjects (int nServerPlayer, int nOtherObjNum, int nServerObjNum)
{
if (IsMultiGame && (gameStates.multi.nGameType >= IPX_GAME) && (nServerPlayer > 0)) {
	CObject h = OBJECTS [nServerObjNum];
	OBJECTS [nServerObjNum] = OBJECTS [nOtherObjNum];
	OBJECTS [nOtherObjNum] = h;
	OBJECTS [nServerObjNum].info.nId = nServerObjNum;
	OBJECTS [nOtherObjNum].info.nId = 0;
	if (N_LOCALPLAYER == nServerObjNum) {
		OBJECTS [nServerObjNum].info.controlType = CT_FLYING;
		OBJECTS [nOtherObjNum].info.controlType = CT_REMOTE;
		}
	else if (N_LOCALPLAYER == nOtherObjNum) {
		OBJECTS [nServerObjNum].info.controlType = CT_REMOTE;
		OBJECTS [nOtherObjNum].info.controlType = CT_FLYING;
		}
	}
}

//------------------------------------------------------------------------------

void CSaveGameManager::FixObjects (void)
{
	CObject	*objP = OBJECTS.Buffer ();
	int		i, j, nSegment;

ConvertObjects ();
gameData.objs.nNextSignature = 0;
for (i = 0; i <= gameData.objs.nLastObject [0]; i++, objP++) {
	objP->rType.polyObjInfo.nAltTextures = -1;
	nSegment = objP->info.nSegment;
	// hack for a bug I haven't yet been able to fix 
	if ((objP->info.nType != OBJ_REACTOR) && (objP->info.xShield < 0)) {
		j = gameData.bosses.Find (i);
		if ((j < 0) || (gameData.bosses [j].m_nDying != i))
			objP->info.nType = OBJ_NONE;
		}
	objP->info.nNextInSeg = objP->info.nPrevInSeg = objP->info.nSegment = -1;
	if (objP->info.nType != OBJ_NONE) {
		objP->LinkToSeg (nSegment);
		if (objP->info.nSignature > gameData.objs.nNextSignature)
			gameData.objs.nNextSignature = objP->info.nSignature;
		}
	//look for, and fix, boss with bogus shield
	if ((objP->info.nType == OBJ_ROBOT) && ROBOTINFO (objP->info.nId).bossFlag) {
		fix xShieldSave = objP->info.xShield;
		CopyDefaultsToRobot (objP);		//calculate starting shield
		//if in valid range, use loaded shield value
		if (xShieldSave > 0 && (xShieldSave <= objP->info.xShield))
			objP->SetShield (xShieldSave);
		else
			objP->info.xShield /= 2;  //give player a break
		}
	}
}

//------------------------------------------------------------------------------

void CSaveGameManager::AwardReturningPlayer (CPlayerData *retPlayerP, fix xOldGameTime)
{
CPlayerData *playerP = gameData.multiplayer.players + N_LOCALPLAYER;
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

void CSaveGameManager::LoadNetGame (void)
{
	int	i, j;

netGame.m_info.nType = m_cf.ReadByte ();
netGame.m_info.nSecurity = m_cf.ReadInt ();
m_cf.Read (netGame.m_info.szGameName, 1, NETGAME_NAME_LEN + 1);
m_cf.Read (netGame.m_info.szMissionTitle, 1, MISSION_NAME_LEN + 1);
m_cf.Read (netGame.m_info.szMissionName, 1, 9);
netGame.m_info.SetLevel (m_cf.ReadInt ());
netGame.m_info.gameMode = (ubyte) m_cf.ReadByte ();
netGame.m_info.bRefusePlayers = (ubyte) m_cf.ReadByte ();
netGame.m_info.difficulty = (ubyte) m_cf.ReadByte ();
netGame.m_info.gameStatus = (ubyte) m_cf.ReadByte ();
netGame.m_info.nNumPlayers = (ubyte) m_cf.ReadByte ();
netGame.m_info.nMaxPlayers = (ubyte) m_cf.ReadByte ();
netGame.m_info.nConnected = (ubyte) m_cf.ReadByte ();
netGame.m_info.gameFlags = (ubyte) m_cf.ReadByte ();
netGame.m_info.protocolVersion = (ubyte) m_cf.ReadByte ();
netGame.m_info.versionMajor = (ubyte) m_cf.ReadByte ();
netGame.m_info.versionMinor = (ubyte) m_cf.ReadByte ();
netGame.m_info.SetTeamVector ((ubyte) m_cf.ReadByte ());
netGame.m_info.DoMegas = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoSmarts = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoFusions = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoHelix = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoPhoenix = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoAfterburner = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoInvulnerability = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoCloak = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoGauss = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoVulcan = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoPlasma = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoOmega = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoSuperLaser = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoProximity = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoSpread = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoSmartMine = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoFlash = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoGuided = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoEarthShaker = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoMercury = (ubyte) m_cf.ReadByte ();
netGame.m_info.bAllowMarkerView = (ubyte) m_cf.ReadByte ();
netGame.m_info.bIndestructibleLights = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoAmmoRack = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoConverter = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoHeadlight = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoHoming = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoLaserUpgrade = (ubyte) m_cf.ReadByte ();
netGame.m_info.DoQuadLasers = (ubyte) m_cf.ReadByte ();
netGame.m_info.bShowAllNames = (ubyte) m_cf.ReadByte ();
netGame.m_info.BrightPlayers = (ubyte) m_cf.ReadByte ();
netGame.m_info.invul = (ubyte) m_cf.ReadByte ();
netGame.m_info.FriendlyFireOff = (ubyte) m_cf.ReadByte ();
for (i = 0; i < 2; i++)
	m_cf.Read (netGame.m_info.szTeamName [i], 1, CALLSIGN_LEN + 1);		// 18 bytes
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	*netGame.Locations (i) = m_cf.ReadInt ();
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	for (j = 0; j < MAX_NUM_NET_PLAYERS; j++)
		*netGame.Kills (i, j) = m_cf.ReadShort ();				// 128 bytes
netGame.SetSegmentCheckSum (m_cf.ReadShort ());				// 2 bytes
for (i = 0; i < 2; i++)
	*netGame.TeamKills (i) = m_cf.ReadShort ();				// 4 bytes
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	*netGame.Killed (i) = m_cf.ReadShort ();					// 16 bytes
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	*netGame.PlayerKills (i) = m_cf.ReadShort ();				// 16 bytes
netGame.SetScoreGoal (m_cf.ReadInt ());						// 4 bytes
netGame.SetPlayTimeAllowed (m_cf.ReadFix ());				// 4 bytes
netGame.SetLevelTime (m_cf.ReadFix ());						// 4 bytes
netGame.SetControlInvulTime (m_cf.ReadInt ());				// 4 bytes
netGame.SetMonitorVector (m_cf.ReadInt ());					// 4 bytes
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	*netGame.PlayerScore (i) = m_cf.ReadInt ();				// 32 bytes
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	*netGame.PlayerFlags (i) = ubyte (m_cf.ReadByte ());	// 8 bytes
netGame.SetPacketsPerSec (m_cf.ReadShort ());				// 2 bytes
netGame.SetShortPackets (ubyte (m_cf.ReadByte ()));		// 1 bytes
// 279 bytes
// 355 bytes total
m_cf.Read (netGame.AuxData (), NETGAME_AUX_SIZE, 1);  // Storage for protocol-specific data (e.g., multicast session and port)
}

//------------------------------------------------------------------------------

void CSaveGameManager::LoadNetPlayers (void)
{
netPlayers [0].m_info.nType = (ubyte) m_cf.ReadByte ();
netPlayers [0].m_info.nSecurity = m_cf.ReadInt ();
for (int i = 0; i < MAX_NUM_NET_PLAYERS + 4; i++) {
	m_cf.Read (netPlayers [0].m_info.players [i].callsign, 1, CALLSIGN_LEN + 1);
	m_cf.Read (netPlayers [0].m_info.players [i].network.Server (), 1, 4);
	m_cf.Read (netPlayers [0].m_info.players [i].network.Node (), 1, 6);
	netPlayers [0].m_info.players [i].versionMajor = (ubyte) m_cf.ReadByte ();
	netPlayers [0].m_info.players [i].versionMinor = (ubyte) m_cf.ReadByte ();
	netPlayers [0].m_info.players [i].computerType = m_cf.ReadByte ();
	netPlayers [0].m_info.players [i].connected = ((sbyte) m_cf.ReadByte ());
	netPlayers [0].m_info.players [i].socket = (ushort) m_cf.ReadShort ();
	netPlayers [0].m_info.players [i].rank = (ubyte) m_cf.ReadByte ();
	}
}

//------------------------------------------------------------------------------

void CSaveGameManager::LoadPlayer (CPlayerData *playerP)
{
	int	i;

m_cf.Read (playerP->callsign, 1, CALLSIGN_LEN + 1); // The callsign of this CPlayerData, for net purposes.
m_cf.Read (playerP->netAddress, 1, 6);					// The network address of the player.
playerP->Connect ((sbyte) m_cf.ReadByte ());			// Is the player connected or not?
playerP->nObject = m_cf.ReadInt ();						// What CObject number this CPlayerData is. (made an int by mk because it's very often referenced)
playerP->nPacketsGot = m_cf.ReadInt ();				// How many packets we got from them
playerP->nPacketsSent = m_cf.ReadInt ();				// How many packets we sent to them
playerP->flags = (uint) m_cf.ReadInt ();           // Powerup flags, see below...
playerP->Setup ();
playerP->SetEnergy (m_cf.ReadFix (), false);			// Amount of energy remaining.
playerP->SetShield (m_cf.ReadFix (), false);			// shield remaining (protection)
playerP->lives = m_cf.ReadByte ();						// Lives remaining, 0 = game over.
playerP->level = m_cf.ReadByte ();						// Current level CPlayerData is playing. (must be signed for secret levels)
playerP->ComputeLaserLevels (m_cf.ReadByte ());		// Current level of the laser.
playerP->startingLevel = m_cf.ReadByte ();			// What level the player started on.
playerP->nKillerObj = m_cf.ReadShort ();				// Who killed me.... (-1 if no one)
playerP->primaryWeaponFlags = (ushort) m_cf.ReadShort ();   // bit set indicates the player has this weapon.
playerP->secondaryWeaponFlags = (ushort) m_cf.ReadShort (); // bit set indicates the player has this weapon.
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	playerP->primaryAmmo [i] = (ushort) m_cf.ReadShort (); // How much ammo of each nType.
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
	playerP->secondaryAmmo [i] = (ushort) m_cf.ReadShort (); // How much ammo of each nType.
#if 1 //for inventory system
playerP->nInvuls = (ubyte) m_cf.ReadByte ();
playerP->nCloaks = (ubyte) m_cf.ReadByte ();
#endif
playerP->lastScore = m_cf.ReadInt ();					// Score at beginning of current level.
playerP->score = m_cf.ReadInt ();						// Current score.
playerP->timeLevel = m_cf.ReadFix ();					// Level time played
playerP->timeTotal = m_cf.ReadFix ();					// Game time played (high word = seconds)
playerP->cloakTime = m_cf.ReadFix ();					// Time cloaked
if (playerP->cloakTime != 0x7fffffff)
	playerP->cloakTime += gameData.time.xGame;
playerP->invulnerableTime = m_cf.ReadFix ();			// Time invulnerable
if (playerP->invulnerableTime != 0x7fffffff)
	playerP->invulnerableTime += gameData.time.xGame;
playerP->nScoreGoalCount = m_cf.ReadShort ();          // Num of players killed this level
playerP->netKilledTotal = m_cf.ReadShort ();       // Number of times killed total
playerP->netKillsTotal = m_cf.ReadShort ();        // Number of net kills total
playerP->numKillsLevel = m_cf.ReadShort ();        // Number of kills this level
playerP->numKillsTotal = m_cf.ReadShort ();        // Number of kills total
playerP->numRobotsLevel = m_cf.ReadShort ();       // Number of initial robots this level
playerP->numRobotsTotal = m_cf.ReadShort ();       // Number of robots total
playerP->hostages.nRescued = (ushort) m_cf.ReadShort (); // Total number of hostages rescued.
playerP->hostages.nTotal = (ushort) m_cf.ReadShort ();         // Total number of hostages.
playerP->hostages.nOnBoard = (ubyte) m_cf.ReadByte ();      // Number of hostages on ship.
playerP->hostages.nLevel = (ubyte) m_cf.ReadByte ();         // Number of hostages on this level.
playerP->homingObjectDist = m_cf.ReadFix ();			// Distance of nearest homing CObject.
playerP->hoursLevel = m_cf.ReadByte ();            // Hours played (since timeTotal can only go up to 9 hours)
playerP->hoursTotal = m_cf.ReadByte ();            // Hours played (since timeTotal can only go up to 9 hours)
}

//------------------------------------------------------------------------------

void CSaveGameManager::LoadObjTriggerRef (tObjTriggerRef *refP)
{
m_cf.ReadShort ();
m_cf.ReadShort ();
m_cf.ReadShort ();
}

//------------------------------------------------------------------------------

void CSaveGameManager::LoadObjectProducer (tObjectProducerInfo *objProducerP)
{
	int	i;

for (i = 0; i < 2; i++)
	objProducerP->objFlags [i] = m_cf.ReadInt ();
objProducerP->xHitPoints = m_cf.ReadFix ();
objProducerP->xInterval = m_cf.ReadFix ();
objProducerP->nSegment = m_cf.ReadShort ();
objProducerP->nProducer = m_cf.ReadShort ();
}

//------------------------------------------------------------------------------

void CSaveGameManager::LoadProducer (tProducerInfo *producerP)
{
producerP->nType = m_cf.ReadInt ();
producerP->nSegment = m_cf.ReadInt ();
producerP->bFlag = m_cf.ReadByte ();
producerP->bEnabled = m_cf.ReadByte ();
producerP->nLives = m_cf.ReadByte ();
producerP->xCapacity = m_cf.ReadFix ();
producerP->xMaxCapacity = m_cf.ReadFix ();
producerP->xTimer = m_cf.ReadFix ();
producerP->xDisableTime = m_cf.ReadFix ();
m_cf.ReadVector (producerP->vCenter);
}

//------------------------------------------------------------------------------

void CSaveGameManager::LoadReactorTrigger (CTriggerTargets *triggerP)
{
triggerP->m_nLinks = m_cf.ReadShort ();
triggerP->LoadState (m_cf);
}

//------------------------------------------------------------------------------

void CSaveGameManager::LoadReactorState (tReactorStates *stateP)
{
stateP->nObject = m_cf.ReadInt ();
stateP->bHit = m_cf.ReadInt ();
stateP->bSeenPlayer = m_cf.ReadInt ();
stateP->nNextFireTime = m_cf.ReadInt ();
stateP->nDeadObj = m_cf.ReadInt ();
}

//------------------------------------------------------------------------------

int CSaveGameManager::LoadSpawnPoint (int i)
{
m_cf.ReadVector (gameData.multiplayer.playerInit [i].position.vPos);     
m_cf.ReadMatrix (gameData.multiplayer.playerInit [i].position.mOrient);  
gameData.multiplayer.playerInit [i].nSegment = m_cf.ReadShort ();
gameData.multiplayer.playerInit [i].nSegType = m_cf.ReadShort ();
return (gameData.multiplayer.playerInit [i].nSegment >= 0) &&
		 (gameData.multiplayer.playerInit [i].nSegment < gameData.segs.nSegments) &&
		 (gameData.multiplayer.playerInit [i].nSegment ==
		  FindSegByPos (gameData.multiplayer.playerInit [i].position.vPos, 
							 gameData.multiplayer.playerInit [i].nSegment, 1, 0));

}

//------------------------------------------------------------------------------

int CSaveGameManager::LoadUniFormat (int bMulti, fix xOldGameTime, int *nLevel)
{
	CPlayerData	restoredPlayers [MAX_PLAYERS];
	int		nPlayers, nServerPlayer = -1;
	int		nOtherObjNum = -1, nServerObjNum = -1, nLocalObjNum = -1;
	int		nCurrentLevel, nNextLevel;
	CWall		*wallP;
	char		szOrgCallSign [CALLSIGN_LEN+16];
	int		h, i, j;

if (m_nVersion >= 39) {
	h = m_cf.ReadInt ();
#if 0
	if (h != gameData.segs.nMaxSegments) {
		Warning (TXT_MAX_SEGS_WARNING, h);
		return 0;
		}
#endif
	}

m_bBetweenLevels = m_cf.ReadInt ();
Assert (m_bBetweenLevels == 0);	//between levels save ripped out
// Read the mission info...
if (m_nVersion >= 55)
	gameOpts->app.bEnableMods = m_cf.ReadInt ();
if (!LoadMission ())
	return 0;
//Read level info
nCurrentLevel = m_cf.ReadInt ();
nNextLevel = m_cf.ReadInt ();
//Restore gameData.time.xGame
gameData.time.xGame = m_cf.ReadFix ();
// Start new game....
CSaveGameManager::LoadMulti (szOrgCallSign, bMulti);
if (IsMultiGame) {
		char szServerCallSign [CALLSIGN_LEN + 1];

	strcpy (szServerCallSign, netPlayers [0].m_info.players [0].callsign);
	gameData.app.nStateGameId = m_cf.ReadInt ();
	CSaveGameManager::LoadNetGame ();
	CSaveGameManager::LoadNetPlayers ();
	nPlayers = m_cf.ReadInt ();
	N_LOCALPLAYER = m_cf.ReadInt ();
	for (i = 0; i < nPlayers; i++) {
		CSaveGameManager::LoadPlayer (restoredPlayers + i);
		restoredPlayers [i].Connect ((sbyte) CONNECT_DISCONNECTED);
		}
	// make sure the current game host is in CPlayerData slot #0
	nServerPlayer = SetServerPlayer (restoredPlayers, nPlayers, szServerCallSign, &nOtherObjNum, &nServerObjNum);
	GetConnectedPlayers (restoredPlayers, nPlayers);
	}
//Read CPlayerData info
if (!PrepareLevel (nCurrentLevel, true, m_bSecret == 1, true, m_bSecret == 0)) {
	m_cf.Close ();
	return 0;
	}

#if 1
if (m_nVersion >= 39) {
	if (gameData.segs.nSegments > gameData.segs.nMaxSegments) {
		Warning (TXT_MAX_SEGS_WARNING, gameData.segs.nSegments);
		return 0;
		}
	}
#endif

nLocalObjNum = LOCALPLAYER.nObject;
if (m_bSecret != 1)	//either no secret restore, or player died in scret level
	LoadPlayer (gameData.multiplayer.players + N_LOCALPLAYER);
else {
	CPlayerData	retPlayer;
	LoadPlayer (&retPlayer);
	AwardReturningPlayer (&retPlayer, xOldGameTime);
	}
LOCALPLAYER.SetObject (nLocalObjNum);
strcpy (LOCALPLAYER.callsign, szOrgCallSign);
// Set the right level
if (m_bBetweenLevels)
	LOCALPLAYER.level = nNextLevel;
// Restore the weapon states
gameData.weapons.nPrimary = m_cf.ReadByte ();
gameData.weapons.nSecondary = m_cf.ReadByte ();
SelectWeapon (gameData.weapons.nPrimary, 0, 0, 0);
SelectWeapon (gameData.weapons.nSecondary, 1, 0, 0);
// Restore the difficulty level
gameStates.app.nDifficultyLevel = m_cf.ReadInt ();
// Restore the cheats enabled flag
gameStates.app.cheats.bEnabled = m_cf.ReadInt ();
if ((m_nVersion >= 52) && (m_nVersion <= 54))
	gameOpts->app.bEnableMods = m_cf.ReadInt ();
for (i = 0; i < 2; i++) {
	if (m_nVersion < 33) {
		gameStates.gameplay.slowmo [i].fSpeed = 1;
		gameStates.gameplay.slowmo [i].nState = 0;
		}
	else {
		gameStates.gameplay.slowmo [i].fSpeed = X2F (m_cf.ReadInt ());
		gameStates.gameplay.slowmo [i].nState = m_cf.ReadInt ();
		}
	}
if (m_nVersion > 33) {
	for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	   if (i != N_LOCALPLAYER)
		   gameData.multiplayer.weaponStates [i].bTripleFusion = m_cf.ReadInt ();
   	else {
   	   gameData.weapons.bTripleFusion = m_cf.ReadInt ();
		   gameData.multiplayer.weaponStates [i].bTripleFusion = !gameData.weapons.bTripleFusion;  //force MultiSendWeapons
		   }
	}
if (!m_bBetweenLevels) {
	gameStates.render.bDoAppearanceEffect = 0;			// Don't do this for middle o' game stuff.
	//Clear out all the objects from the lvl file
	ResetSegObjLists ();
	ResetObjects (1);

	//Read objects, and pop 'em into their respective segments.
	h = m_cf.ReadInt ();
	gameData.objs.nLastObject [0] = h - 1;
	extraGameInfo [0].nBossCount [0] = 0;
	for (i = 0; i < h; i++) {
		OBJECTS [i].LoadState (m_cf);
		if ((m_nVersion < 32) && IS_BOSS (OBJECTS + i))
			gameData.bosses.Add (i);
		}
	FixNetworkObjects (nServerPlayer, nOtherObjNum, nServerObjNum);
	gameData.objs.nNextSignature = 0;
	InitCamBots (1);
	gameData.objs.nNextSignature++;
	//	1 = Didn't die on secret level.
	//	2 = Died on secret level.
	if (m_bSecret && (missionManager.nCurrentLevel >= 0)) {
		SetPosFromReturnSegment (0);
		if (m_bSecret == 2)
			ResetShipData (true);
		}
	//Restore CWall info
	if (ReadBoundedInt (MAX_WALLS, &gameData.walls.nWalls))
		return 0;
	for (i = 0, wallP = WALLS.Buffer (); i < gameData.walls.nWalls; i++, wallP++) {
		wallP->LoadState (m_cf);
		if (wallP->nType == WALL_OPEN)
			audio.DestroySegmentSound ((short) wallP->nSegment, (short) wallP->nSide, -1);	//-1 means kill any sound
		}
	//Restore exploding wall info
	if (ReadBoundedInt (MAX_EXPLODING_WALLS, &i))
		return 0;
	gameData.walls.exploding.Reset ();
	for (; i; i--)
		if (gameData.walls.exploding.Grow ()) {
			gameData.walls.exploding.Top ()->LoadState (m_cf);
			if (gameData.walls.exploding.Top ()->nSegment < 0)
				gameData.walls.exploding.Shrink ();
			}
		else
			return 0;
	//Restore door info
	if (ReadBoundedInt (MAX_DOORS, &i))
		return 0;
	gameData.walls.activeDoors.Reset ();
	for (; i; i--)
		if (gameData.walls.activeDoors.Grow ())
			gameData.walls.activeDoors.Top ()->LoadState (m_cf);
		else
			return 0;
	if (ReadBoundedInt (MAX_CLOAKING_WALLS, &i))
		return 0;
	gameData.walls.cloaking.Reset ();
	for (; i; i--)
		if (gameData.walls.cloaking.Grow ())
			gameData.walls.cloaking.Top ()->LoadState (m_cf);
		else
			return 0;
	//Restore CTrigger info
	if (ReadBoundedInt (MAX_TRIGGERS, &gameData.trigs.m_nTriggers))
		return 0;
	for (i = 0; i < gameData.trigs.m_nTriggers; i++)
		TRIGGERS [i].LoadState (m_cf);
	//Restore CObject CTrigger info
	if (ReadBoundedInt (MAX_TRIGGERS, &gameData.trigs.m_nObjTriggers))
		return 0;
	gameData.trigs.objTriggerRefs.Clear (0xff);
	if (gameData.trigs.m_nObjTriggers > 0) {
		for (i = 0; i < gameData.trigs.m_nObjTriggers; i++)
			OBJTRIGGERS [i].LoadState (m_cf, true);
		if (m_nVersion < 51) {
			for (i = 0; i < gameData.trigs.m_nObjTriggers; i++) {
#if 1
				m_cf.Seek (2 * sizeof (short), SEEK_CUR);
				OBJTRIGGERS [i].m_info.nObject = m_cf.ReadShort ();
#else
				CSaveGameManager::LoadObjTriggerRef (gameData.trigs.objTriggerRefs + i);
#endif
				}
			if (m_nVersion < 36) {
#if 1
				m_cf.Seek (((m_nVersion < 35) ? 700 : MAX_OBJECTS_D2X) * sizeof (short), SEEK_CUR);
#else
				j = (m_nVersion < 35) ? 700 : MAX_OBJECTS_D2X;
				for (i = 0; i < j; i++)
					m_cf.ReadShort ();
#endif
				}
			else {
#if 1
				m_cf.Seek (m_cf.ReadShort () * 2 * sizeof (short), SEEK_CUR);
#else
				for (i = m_cf.ReadShort (); i; i--) {
					m_cf.ReadShort ();
					m_cf.ReadShort ();
					}
#endif
				}
			}
		BuildObjTriggerRef ();
		}
	else if (m_nVersion < 36)
		m_cf.Seek (((m_nVersion < 35) ? 700 : MAX_OBJECTS_D2X) * sizeof (short), SEEK_CUR);
	else
		m_cf.ReadShort ();
	//Restore tmap info
	for (i = 0; i <= gameData.segs.nLastSegment; i++)
		SEGMENTS [i].LoadState (m_cf);
	//Restore the producer info
	audio.Prepare ();
	SetupWalls ();
	gameData.reactor.bDestroyed = m_cf.ReadInt ();
	gameData.reactor.countdown.nTimer = m_cf.ReadFix ();
	if (ReadBoundedInt (MAX_ROBOT_CENTERS, &gameData.producers.nRobotMakers))
		return 0;
	for (i = 0; i < gameData.producers.nRobotMakers; i++)
		LoadObjectProducer (gameData.producers.robotMakers + i);
	if (m_nVersion >= 30) {
		if (ReadBoundedInt (MAX_EQUIP_CENTERS, &gameData.producers.nEquipmentMakers))
			return 0;
		for (i = 0; i < gameData.producers.nEquipmentMakers; i++)
			LoadObjectProducer (gameData.producers.equipmentMakers + i);
		}
	else {
		gameData.producers.nRobotMakers = 0;
		gameData.producers.robotMakers.Clear (0);
		}
	LoadReactorTrigger (&gameData.reactor.triggers);
	if (ReadBoundedInt (MAX_FUEL_CENTERS, &gameData.producers.nProducers))
		return 0;
	for (i = 0; i < gameData.producers.nProducers; i++)
		LoadProducer (gameData.producers.producers + i);
	// Restore the control cen info
	if (m_nVersion < 31) {
		gameData.reactor.states [0].bHit = m_cf.ReadInt ();
		gameData.reactor.states [0].bSeenPlayer = m_cf.ReadInt ();
		gameData.reactor.states [0].nNextFireTime = m_cf.ReadInt ();
		gameData.reactor.bPresent = m_cf.ReadInt ();
		gameData.reactor.states [0].nDeadObj = m_cf.ReadInt ();
		}
	else {
		int	i;

		gameData.reactor.bPresent = m_cf.ReadInt ();
		for (i = 0; i < MAX_BOSS_COUNT; i++)
			LoadReactorState (gameData.reactor.states + i);
		}
	// Restore the AI state
	LoadAIUniFormat ();
	// Restore the automap visited info
	FixObjects ();
	SpecialResetObjects ();
	if (m_nVersion > 39) {
		for (i = 0; i < LEVEL_SEGMENTS; i++)
			automap.m_visited [i] = (ushort) m_cf.ReadShort ();
		}
	else if (m_nVersion > 37) {
		for (i = 0; i < LEVEL_SEGMENTS; i++)
			automap.m_visited [i] = (ushort) m_cf.ReadShort ();
		if (MAX_SEGMENTS > LEVEL_SEGMENTS)
			m_cf.Seek ((MAX_SEGMENTS - LEVEL_SEGMENTS) * sizeof (ushort), SEEK_CUR);
		}
	else {
		int	i, j = (m_nVersion > 22) ? MAX_SEGMENTS : MAX_SEGMENTS_D2;
		for (i = 0; i < j; i++)
			automap.m_visited [i] = (ushort) m_cf.ReadByte ();
		if (j > LEVEL_SEGMENTS)
			m_cf.Seek ((j - LEVEL_SEGMENTS) * sizeof (ubyte), SEEK_CUR);
		}
	//	Restore hacked up weapon system stuff.
	gameData.fusion.xNextSoundTime = gameData.time.xGame;
	gameData.fusion.xAutoFireTime = 0;
	gameData.laser.xNextFireTime = 
	gameData.missiles.xNextFireTime = gameData.time.xGame;
	gameData.laser.xLastFiredTime = 
	gameData.missiles.xLastFiredTime = gameData.time.xGame;
	}
gameData.app.nStateGameId = (uint) m_cf.ReadInt ();
gameStates.app.cheats.bLaserRapidFire = m_cf.ReadInt ();
gameStates.app.bLunacy = m_cf.ReadInt ();		//	Yes, reading this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
gameStates.gameplay.bMineMineCheat = m_cf.ReadInt ();
if (gameStates.app.bLunacy)
	DoLunacyOn ();

markerManager.LoadState (m_cf);
if (m_bSecret != 1)
	gameData.physics.xAfterburnerCharge = m_cf.ReadFix ();
else {
	m_cf.ReadFix ();
	}
//read last was super information
m_cf.Read (&bLastPrimaryWasSuper, sizeof (bLastPrimaryWasSuper), 1);
m_cf.Read (&bLastSecondaryWasSuper, sizeof (bLastSecondaryWasSuper), 1);
paletteManager.SetFlashDuration (m_cf.ReadFix ());
paletteManager.SetLastEffectTime (m_cf.ReadFix ());
paletteManager.SetRedEffect ((ubyte) m_cf.ReadShort ());
paletteManager.SetGreenEffect ((ubyte) m_cf.ReadShort ());
paletteManager.SetBlueEffect ((ubyte) m_cf.ReadShort ());
h = (m_nVersion > 39) ? LEVEL_SEGMENTS : (m_nVersion > 22) ? MAX_SEGMENTS : MAX_SEGMENTS_D2;
j = min (h, LEVEL_SEGMENTS);
gameData.render.lights.subtracted.Read (m_cf, j);
if (h > j)
	m_cf.Seek ((h - j) * sizeof (gameData.render.lights.subtracted [0]), SEEK_CUR);
ApplyAllChangedLight ();
//FixWalls ();
gameStates.app.bFirstSecretVisit = m_cf.ReadInt ();
if (m_bSecret || (nCurrentLevel < 0))
	gameStates.app.bFirstSecretVisit = 0;

if (m_bSecret != 1)
	gameData.omega.xCharge [0] = 
	gameData.omega.xCharge [1] = m_cf.ReadFix ();
else
	m_cf.ReadFix ();
if (m_nVersion > 27)
	missionManager.nEntryLevel = m_cf.ReadShort ();
*nLevel = nCurrentLevel;
if (m_nVersion >= 57) {
	gameStates.gameplay.seismic.nShakeFrequency = m_cf.ReadInt ();
	gameStates.gameplay.seismic.nShakeDuration = m_cf.ReadInt ();
	}
if (m_nVersion >= 37) {
	tObjPosition playerInitSave [MAX_PLAYERS];

	memcpy (playerInitSave, gameData.multiplayer.playerInit, sizeof (playerInitSave));
	for (h = 1, i = 0; i < MAX_NUM_NET_PLAYERS; i++)
		if (!LoadSpawnPoint (i))
			h = 0;
	if (!h)
		memcpy (gameData.multiplayer.playerInit, playerInitSave, sizeof (playerInitSave));
	}
if (m_nVersion >= 54) {
	gameOpts->gameplay.nShip [0] = m_cf.ReadInt ();
	gameOpts->gameplay.nShip [1] = -1;
	gameData.multiplayer.weaponStates [N_LOCALPLAYER].nShip = ubyte (gameOpts->gameplay.nShip [0]);
	}
if (LOCALPLAYER.numRobotsLevel > LOCALPLAYER.numKillsLevel + CountRobotsInLevel ()) // fix for a bug affecting savegames
	LOCALPLAYER.numRobotsLevel = LOCALPLAYER.numKillsLevel + CountRobotsInLevel ();
/*---*/PrintLog (1, "initializing sound sources\n");
SetSoundSources ();
PrintLog (-1);
return 1;
}

//------------------------------------------------------------------------------

int CSaveGameManager::LoadBinFormat (int bMulti, fix xOldGameTime, int *nLevel)
{
	CPlayerData	restoredPlayers [MAX_PLAYERS];
	int		nPlayers, nServerPlayer = -1;
	int		nOtherObjNum = -1, nServerObjNum = -1, nLocalObjNum = -1;
	int		nCurrentLevel, nNextLevel;
	CWall		*wallP;
	char		szOrgCallSign [CALLSIGN_LEN+16];
	int		i, j;
	short		nTexture;

m_cf.Read (&m_bBetweenLevels, sizeof (int), 1);
Assert (m_bBetweenLevels == 0);	//between levels save ripped out
// Read the mission info...
if (!LoadMission ())
	return 0;
//Read level info
m_cf.Read (&nCurrentLevel, sizeof (int), 1);
m_cf.Read (&nNextLevel, sizeof (int), 1);
//Restore gameData.time.xGame
m_cf.Read (&gameData.time.xGame, sizeof (fix), 1);
// Start new game....
CSaveGameManager::LoadMulti (szOrgCallSign, bMulti);
if (IsMultiGame) {
		char szServerCallSign [CALLSIGN_LEN + 1];

	strcpy (szServerCallSign, netPlayers [0].m_info.players [0].callsign);
	m_cf.Read (&gameData.app.nStateGameId, sizeof (int), 1);
	m_cf.Read (&netGame, sizeof (tNetGameInfo), 1);
	m_cf.Read (&netPlayers [0], netPlayers [0].Size (), 1);
	m_cf.Read (&nPlayers, sizeof (gameData.multiplayer.nPlayers), 1);
	m_cf.Read (&N_LOCALPLAYER, sizeof (N_LOCALPLAYER), 1);
	for (i = 0; i < nPlayers; i++)
		m_cf.Read (restoredPlayers + i, sizeof (CPlayerData), 1);
	nServerPlayer = SetServerPlayer (restoredPlayers, nPlayers, szServerCallSign, &nOtherObjNum, &nServerObjNum);
	GetConnectedPlayers (restoredPlayers, nPlayers);
	}

//Read CPlayerData info
if (!PrepareLevel (nCurrentLevel, true, m_bSecret == 1, true, false)) {
	m_cf.Close ();
	return 0;
	}
nLocalObjNum = LOCALPLAYER.nObject;
if (m_bSecret != 1)	//either no secret restore, or CPlayerData died in scret level
	m_cf.Read (&gameData.multiplayer.players [N_LOCALPLAYER].callsign, sizeof (CPlayerInfo), 1);
else {
	CPlayerData	retPlayer;
	m_cf.Read (&retPlayer.callsign, sizeof (CPlayerInfo), 1);
	AwardReturningPlayer (&retPlayer, xOldGameTime);
	}
LOCALPLAYER.SetObject (nLocalObjNum);
strcpy (LOCALPLAYER.callsign, szOrgCallSign);
// Set the right level
if (m_bBetweenLevels)
	LOCALPLAYER.level = nNextLevel;
// Restore the weapon states
m_cf.Read (&gameData.weapons.nPrimary, sizeof (sbyte), 1);
m_cf.Read (&gameData.weapons.nSecondary, sizeof (sbyte), 1);
SelectWeapon (gameData.weapons.nPrimary, 0, 0, 0);
SelectWeapon (gameData.weapons.nSecondary, 1, 0, 0);
// Restore the difficulty level
m_cf.Read (&gameStates.app.nDifficultyLevel, sizeof (int), 1);
// Restore the cheats enabled flag
m_cf.Read (&gameStates.app.cheats.bEnabled, sizeof (int), 1);
if (!m_bBetweenLevels) {
	gameStates.render.bDoAppearanceEffect = 0;			// Don't do this for middle o' game stuff.
	//Clear out all the OBJECTS from the lvl file
	ResetSegObjLists ();
	ResetObjects (1);
	//Read objects, and pop 'em into their respective segments.
	m_cf.Read (&gameData.objs.nObjects, sizeof (int), 1);
	gameData.objs.nLastObject [0] = gameData.objs.nObjects - 1;
	for (i = 0; i < gameData.objs.nObjects; i++)
		m_cf.Read (&OBJECTS [i].info, sizeof (tBaseObject), 1);
	FixNetworkObjects (nServerPlayer, nOtherObjNum, nServerObjNum);
	FixObjects ();
	SpecialResetObjects ();
	InitCamBots (1);
	gameData.objs.nNextSignature++;
	//	1 = Didn't die on secret level.
	//	2 = Died on secret level.
	if (m_bSecret && (missionManager.nCurrentLevel >= 0)) {
		SetPosFromReturnSegment (0);
		if (m_bSecret == 2)
			ResetShipData (true);
		}
	//Restore CWall info
	if (ReadBoundedInt (MAX_WALLS, &gameData.walls.nWalls))
		return 0;
	for (i = 0; i < gameData.walls.nWalls; i++) {
		tCompatibleWall w;
		m_cf.Read (&w, sizeof (w), 1);
		WALLS [i].nSegment = w.nSegment;
		WALLS [i].nSide = w.nSide;
		WALLS [i].hps = w.hps;
		WALLS [i].nLinkedWall = w.nLinkedWall;
		WALLS [i].nType = w.nType;
		WALLS [i].flags = w.flags;
		WALLS [i].state = w.state;
		WALLS [i].nTrigger = w.nTrigger;
		WALLS [i].nClip = w.nClip;
		WALLS [i].keys = w.keys;
		WALLS [i].controllingTrigger = w.controllingTrigger;
		WALLS [i].cloakValue = w.cloakValue;
		WALLS [i].bVolatile = 0;
		}
	//now that we have the walls, check if any sounds are linked to
	//walls that are now open
	for (i = 0, wallP = WALLS.Buffer (); i < gameData.walls.nWalls; i++, wallP++)
		if (wallP->nType == WALL_OPEN)
			audio.DestroySegmentSound ((short) wallP->nSegment, (short) wallP->nSide, -1);	//-1 means kill any sound
	//Restore exploding wall info
	if (m_nVersion >= 10) {
		m_cf.Read (&i, sizeof (int), 1);
		if (i > 0) {
			gameData.walls.exploding.Grow (static_cast<uint> (i));
			gameData.walls.exploding.Read (m_cf, i);
			}
		}
	//Restore door info
	if (ReadBoundedInt (MAX_DOORS, &i))
		return 0;
	if (i > 0) {
		gameData.walls.activeDoors.Grow (static_cast<uint> (i));
		gameData.walls.activeDoors.Read (m_cf, gameData.walls.activeDoors.ToS ());
		}
	if (m_nVersion >= 14) {		//Restore cloaking CWall info
		if (ReadBoundedInt (MAX_WALLS, &i))
			return 0;
		if (i > 0) {
			gameData.walls.cloaking.Grow (static_cast<uint> (i));
			gameData.walls.cloaking.Read (m_cf, gameData.walls.cloaking.ToS ());
			}
		}
	//Restore CTrigger info
	if (ReadBoundedInt (MAX_TRIGGERS, &gameData.trigs.m_nTriggers))
		return 0;
	for (i = 0; i < gameData.trigs.m_nTriggers; i++) {
		tCompatibleTrigger t;
		m_cf.Read (&t, sizeof (t), 1);
		TRIGGERS [i].m_info.nType = t.type;
		TRIGGERS [i].m_info.flags = t.flags;
		TRIGGERS [i].m_info.value = t.value;
		TRIGGERS [i].m_info.time [0] = t.time;
		TRIGGERS [i].m_info.time [1] = -1;
		TRIGGERS [i].m_nLinks = t.num_links;
		memcpy (TRIGGERS [i].m_segments, t.seg, sizeof (t.seg));
		memcpy (TRIGGERS [i].m_sides, t.side, sizeof (t.side));
		}
	if (m_nVersion >= 26) {
		//Restore CObject CTrigger info

		m_cf.Read (&gameData.trigs.m_nObjTriggers, sizeof (gameData.trigs.m_nObjTriggers), 1);
		if (gameData.trigs.m_nObjTriggers > 0) {
			OBJTRIGGERS.Read (m_cf, gameData.trigs.m_nObjTriggers);
			m_cf.Seek (gameData.trigs.m_nObjTriggers * 6 * sizeof (short), SEEK_CUR);
			m_cf.Seek (700 * sizeof (short), SEEK_CUR);
			BuildObjTriggerRef ();
			}
		else
			m_cf.Seek (((m_nVersion > 39) ? LEVEL_OBJECTS : (m_nVersion > 34) ? MAX_OBJECTS_D2X : 700) * sizeof (short), SEEK_CUR);
		}
	//Restore tmap info
	CSegment* segP = SEGMENTS.Buffer ();
	CSide* sideP;
	for (i = 0; i <= gameData.segs.nLastSegment; i++, segP++) {
		sideP = segP->m_sides;
		for (j = 0; j < SEGMENT_SIDE_COUNT; j++, sideP++) {
			sideP->m_nWall = m_cf.ReadShort ();
			sideP->m_nBaseTex = m_cf.ReadShort ();
			nTexture = m_cf.ReadShort ();
			sideP->m_nOvlTex = nTexture & 0x3fff;
			sideP->m_nOvlOrient = (nTexture >> 14) & 3;
			}
		}
//Restore the producer info
	gameData.reactor.bDestroyed = m_cf.ReadInt ();
	gameData.reactor.countdown.nTimer = m_cf.ReadFix ();
	if (ReadBoundedInt (MAX_ROBOT_CENTERS, &gameData.producers.nRobotMakers))
		return 0;
	for (i = 0; i < gameData.producers.nRobotMakers; i++) {
		m_cf.Read (gameData.producers.robotMakers [i].objFlags, sizeof (int), 2);
		m_cf.Read (&gameData.producers.robotMakers [i].xHitPoints, 
						sizeof (tObjectProducerInfo) - (reinterpret_cast<char*> (&gameData.producers.robotMakers [i].xHitPoints) - reinterpret_cast<char*> (&gameData.producers.robotMakers [i])), 1);
		}
	m_cf.Read (&gameData.reactor.triggers, sizeof (CTriggerTargets), 1);
	if (ReadBoundedInt (MAX_FUEL_CENTERS, &gameData.producers.nProducers))
		return 0;
	gameData.producers.producers.Read (m_cf, gameData.producers.nProducers);

	// Restore the control cen info
	gameData.reactor.states [0].bHit = m_cf.ReadInt ();
	gameData.reactor.states [0].bSeenPlayer = m_cf.ReadInt ();
	gameData.reactor.states [0].nNextFireTime = m_cf.ReadInt ();
	gameData.reactor.bPresent = m_cf.ReadInt ();
	gameData.reactor.states [0].nDeadObj = m_cf.ReadInt ();
	// Restore the AI state
	LoadAIBinFormat ();
	// Restore the automap visited info
	if (m_nVersion > 39)
		automap.m_visited.Read (m_cf, LEVEL_SEGMENTS);
	if (m_nVersion > 37)
		automap.m_visited.Read (m_cf, MAX_SEGMENTS);
	else {
		int	i, j = (m_nVersion > 22) ? MAX_SEGMENTS : MAX_SEGMENTS_D2;
		for (i = 0; i < j; i++)
			automap.m_visited [i] = (ushort) m_cf.ReadByte ();
		}
	//	Restore hacked up weapon system stuff.
	gameData.fusion.xNextSoundTime = gameData.time.xGame;
	gameData.fusion.xAutoFireTime = 0;
	gameData.laser.xNextFireTime = 
	gameData.missiles.xNextFireTime = gameData.time.xGame;
	gameData.laser.xLastFiredTime = 
	gameData.missiles.xLastFiredTime = gameData.time.xGame;
	}
gameData.app.nStateGameId = 0;

if (m_nVersion >= 7) {
	m_cf.Read (&gameData.app.nStateGameId, sizeof (uint), 1);
	m_cf.Read (&gameStates.app.cheats.bLaserRapidFire, sizeof (int), 1);
	m_cf.Read (&gameStates.app.bLunacy, sizeof (int), 1);		//	Yes, writing this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
	m_cf.Read (&gameStates.app.bLunacy, sizeof (int), 1);
	if (gameStates.app.bLunacy)
		DoLunacyOn ();
}

if (m_nVersion >= 17)
	markerManager.LoadState (m_cf, true);
else {
	int num, dummy;

	// skip dummy info
	m_cf.Read (&num, sizeof (int), 1);       //was NumOfMarkers
	m_cf.Read (&dummy, sizeof (int), 1);     //was CurMarker
	m_cf.Seek (num * (sizeof (CFixVector) + 40), SEEK_CUR);
	for (num = 0; num < NUM_MARKERS; num++)
		markerManager.SetObject (num, -1);
}

if (m_nVersion >= 11) {
	if (m_bSecret != 1)
		m_cf.Read (&gameData.physics.xAfterburnerCharge, sizeof (fix), 1);
	else {
		fix	dummy_fix;
		m_cf.Read (&dummy_fix, sizeof (fix), 1);
	}
}
if (m_nVersion < 12)
	paletteManager.ResetEffect ();
else {
	//read last was super information
	m_cf.Read (&bLastPrimaryWasSuper, sizeof (bLastPrimaryWasSuper), 1);
	m_cf.Read (&bLastSecondaryWasSuper, sizeof (bLastSecondaryWasSuper), 1);
	paletteManager.SetFlashDuration ((fix) m_cf.ReadInt ());
	paletteManager.SetLastEffectTime ((fix) m_cf.ReadInt ());
	paletteManager.SetRedEffect ((ubyte) m_cf.ReadInt ());
	paletteManager.SetGreenEffect ((ubyte) m_cf.ReadInt ());
	paletteManager.SetBlueEffect ((ubyte) m_cf.ReadInt ());
	}

//	Load gameData.render.lights.subtracted
if (m_nVersion >= 16) {
	int h = (m_nVersion > 39) ? LEVEL_SEGMENTS : (m_nVersion > 22) ? MAX_SEGMENTS : MAX_SEGMENTS_D2;
	int j = min (LEVEL_SEGMENTS, h);
	gameData.render.lights.subtracted.Read (m_cf, j);
	if (j < h)
		m_cf.Seek ((h - j) * sizeof (ubyte), SEEK_CUR);
	ApplyAllChangedLight ();
	//ComputeAllStaticLight ();	//	set xAvgSegLight field in CSegment struct.  See note at that function.
	}
else
	gameData.render.lights.subtracted.Clear ();

if (m_bSecret) 
	gameStates.app.bFirstSecretVisit = 0;
else if (m_nVersion >= 20)
	m_cf.Read (&gameStates.app.bFirstSecretVisit, sizeof (gameStates.app.bFirstSecretVisit), 1);
else
	gameStates.app.bFirstSecretVisit = 1;

if (m_nVersion >= 22) {
	if (m_bSecret != 1)
		m_cf.Read (&gameData.omega.xCharge, sizeof (fix), 1);
	else {
		fix	dummy_fix;
		m_cf.Read (&dummy_fix, sizeof (fix), 1);
		}
	}
*nLevel = nCurrentLevel;
return 1;
}

//------------------------------------------------------------------------------

int CSaveGameManager::LoadState (int bMulti, int bSecret, char *filename)
{
	char		szDescription [DESC_LENGTH + 1];
	char		nId [5];
	int		nLevel, i;
	fix		xOldGameTime = gameData.time.xGame;

StopTime ();
if (filename)
	strcpy (m_filename, filename);
if (!m_cf.Open (m_filename, (bSecret < 0) ? gameFolders.szCacheDir : gameFolders.szSaveDir, "rb", 0)) {
	StartTime (1);
	return 0;
	}
m_bSecret = bSecret;
//Read nId
m_cf.Read (nId, sizeof (char)*4, 1);
if (memcmp (nId, dgss_id, 4)) {
	m_cf.Close ();
	StartTime (1);
	return 0;
	}
//Read m_nVersion
m_cf.Read (&m_nVersion, sizeof (int), 1);
if (m_nVersion < STATE_COMPATIBLE_VERSION) {
	m_cf.Close ();
	StartTime (1);
	return 0;
	}
// Read description
m_cf.Read (szDescription, sizeof (char) * DESC_LENGTH, 1);
// Skip the current screen shot...
m_cf.Seek ((m_nVersion < 26) ? THUMBNAIL_W * THUMBNAIL_H : THUMBNAIL_LW * THUMBNAIL_LH, SEEK_CUR);
// skip the palette
m_cf.Seek (768, SEEK_CUR);
if (m_nVersion < 27)
	i = LoadBinFormat (bMulti, xOldGameTime, &nLevel);
else
	i = LoadUniFormat (bMulti, xOldGameTime, &nLevel);
if (bSecret < 0)
	missionManager.LoadLevelStates ();
else if (!bSecret && (m_nVersion >= 53)) {
	for (int i = 0; i < MAX_LEVELS_PER_MISSION; i++)
		missionManager.SetLevelState (i, char (m_cf.ReadByte ()));
	missionManager.SaveLevelStates ();
	}

m_cf.Close ();
if (!i) {
	StartTime (1);
	return 0;
	}
FixObjectSegs ();
FixObjectSizes ();
//lightManager.Setup (nLevel);
//lightManager.AddGeometryLights ();	// redo to account for broken lights
//ComputeNearestLights (nLevel);
SetupEffects ();
InitReactorForLevel (1);
InitAIObjects ();
AddPlayerLoadout (true);
SetMaxOmegaCharge ();
SetEquipmentMakerStates ();
if (!IsMultiGame)
	InitEntropySettings (0);	//required for repair centers
else {
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	  if (!gameData.multiplayer.players [i].connected) {
			NetworkDisconnectPlayer (i);
  			OBJECTS [gameData.multiplayer.players [i].nObject].CreateAppearanceEffect ();
	      }
		}
	}
gameData.objs.viewerP = 
gameData.objs.consoleP = OBJECTS + LOCALPLAYER.nObject;
StartTriggeredSounds ();
StartTime (1);
if (!extraGameInfo [0].nBossCount [0] && (!IsMultiGame || IsCoopGame) && OpenExits ())
	InitCountdown (NULL, gameData.reactor.bDestroyed, gameData.reactor.countdown.nTimer);
return 1;
}

//------------------------------------------------------------------------------

int CSaveGameManager::GetGameId (char *filename, int bSecret)
{
	int	nId;

if (!m_cf.Open (filename, (bSecret < 0) ? gameFolders.szCacheDir : gameFolders.szSaveDir, "rb", 0))
	return 0;
//Read nId
m_cf.Read (&nId, sizeof (char) * 4, 1);
if (memcmp (&nId, dgss_id, 4)) {
	m_cf.Close ();
	return 0;
	}
//Read m_nVersion
m_nVersion = m_cf.ReadInt ();
if (m_nVersion < STATE_COMPATIBLE_VERSION) {
	m_cf.Close ();
	return 0;
	}
// skip the description, current image, palette, mission name, between levels flag and mission info
m_cf.Seek (DESC_LENGTH + ImageSize () + 768 + 9 * sizeof (char) + (5 + (m_nVersion >= 55)) * sizeof (int), SEEK_CUR);
m_nGameId = m_cf.ReadInt ();
m_cf.Close ();
return m_nGameId;
}

//------------------------------------------------------------------------------
//eof

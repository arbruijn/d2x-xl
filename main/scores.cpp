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
#include "error.h"
#include "key.h"
#include "gamefont.h"
#include "u_mem.h"
#include "menu.h"
#include "screens.h"
#include "mouse.h"
#include "joy.h"
#include "timer.h"
#include "text.h"
#include "strutil.h"
#include "menubackground.h"
#include "songs.h"
#include "ogl_lib.h"
#include "renderlib.h"
#include "cockpit.h"
#include "scores.h"

#define VERSION_NUMBER 		1
#define SCORES_FILENAME 	"descent.hi"
#define XX		(7)
#define YY		(-3)

#define LHX(x)	(gameStates.menus.bHires ? 2 * (x) : x)
#define LHY(y)	(gameStates.menus.bHires ? (24 * (y)) / 10 : y)

#define COOL_SAYING TXT_REGISTER_DESCENT

CScoreManager scoreManager;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CScoreManager::CScoreManager () : m_bHilite (false)
{
SetDefaultScores ();
}

//------------------------------------------------------------------------------

char* CScoreManager::GetFilename (void)
{
#if DBG
	// Only use the MINER variable for internal developement
	char *p;
	p=getenv ("MINER");
	if (p) {
		sprintf (m_filename, "%s\\game\\%s", p, SCORES_FILENAME);
		Assert (strlen (m_filename) < 128);
		return m_filename;
	}
#endif
	sprintf (m_filename, "%s", SCORES_FILENAME);
	return m_filename;
}

//------------------------------------------------------------------------------


void CScoreManager::SetDefaultScores (void)
{
sprintf (m_scores.szCoolSaying, COOL_SAYING);
sprintf (m_scores.stats[0].name, "Parallax");
sprintf (m_scores.stats[1].name, "Matt");
sprintf (m_scores.stats[2].name, "Mike");
sprintf (m_scores.stats[3].name, "Adam");
sprintf (m_scores.stats[4].name, "Mark");
sprintf (m_scores.stats[5].name, "Jasen");
sprintf (m_scores.stats[6].name, "Samir");
sprintf (m_scores.stats[7].name, "Doug");
sprintf (m_scores.stats[8].name, "Dan");
sprintf (m_scores.stats[9].name, "Jason");

for (int i = 0; i < 10; i++)
	m_scores.stats [i].score = (10 - i) * 1000;
}

//------------------------------------------------------------------------------

bool CScoreManager::Load (void)
{
	CFile cf;

	// clear score array...
memset (&m_scores, 0, sizeof (tScoreInfo));

if (!cf.Open (GetFilename (), gameFolders.game.szData [0], "rb", 0)) {
	// No error message needed, code will work without a m_scores file
	SetDefaultScores ();
	return false;
	}
	
int fSize = (int) cf.Length ();

if (fSize != sizeof (tScoreInfo)) {
	cf.Close ();
	return false;
	}
	// Load 'em in...
cf.Read (&m_scores, sizeof (tScoreInfo), 1);
cf.Close ();

if ((m_scores.version != VERSION_NUMBER) || (m_scores.nSignature [0] != 'D') || (m_scores.nSignature [1] != 'H') || (m_scores.nSignature [2] != 'S')) {
	SetDefaultScores ();
	return false;
	}
return true;
}

//------------------------------------------------------------------------------

bool CScoreManager::Save  (void)
{
	CFile cf;

if (!cf.Open (GetFilename (), gameFolders.game.szData [0], "wb", 0)) {
	TextBox (TXT_WARNING, BG_STANDARD, 1, TXT_OK, "%s\n'%s'", TXT_UNABLE_TO_OPEN, GetFilename ());
	return false;
	}

m_scores.nSignature [0] = 'D';
m_scores.nSignature [1] = 'H';
m_scores.nSignature [2] = 'S';
m_scores.version = VERSION_NUMBER;
cf.Write (&m_scores, sizeof (tScoreInfo), 1);
cf.Close ();
return true;
}

//------------------------------------------------------------------------------

char* CScoreManager::IntToString (int number, char *dest)
{
	char buffer[20];

sprintf (buffer, "%d", number);
int l = (int) strlen (buffer);
if (l <= 3) {
	// Don't bother with less than 3 digits
	sprintf (dest, "%d", number);
	return dest;
	}

int c = 0;
char *p = dest;
for (int i = l - 1; i >= 0; i--) {
	if (c == 3) {
		*p++ = ',';
		c = 0;
		}
	c++;
	*p++ = buffer[i];
	}
*p++ = '\0';
strrev (dest);
return dest;
}

//------------------------------------------------------------------------------

void CScoreManager::InitStats (tStatsInfo& stats)
{
strcpy (stats.name, LOCALPLAYER.callsign);
stats.score = LOCALPLAYER.score;
stats.endingLevel = LOCALPLAYER.level;
stats.killRatio = (LOCALPLAYER.numRobotsTotal > 0) ? (LOCALPLAYER.numKillsTotal * 100) / LOCALPLAYER.numRobotsTotal : 0;
stats.hostageRatio = (LOCALPLAYER.hostages.nTotal > 0) ? (LOCALPLAYER.hostages.nRescued*100)/LOCALPLAYER.hostages.nTotal : 0;
stats.seconds = X2I (LOCALPLAYER.timeTotal) +  (LOCALPLAYER.hoursTotal*3600);
stats.diffLevel = gameStates.app.nDifficultyLevel;
stats.startingLevel = LOCALPLAYER.startingLevel;
}

//------------------------------------------------------------------------------
//char * score_placement[10] = { TXT_1ST, TXT_2ND, TXT_3RD, TXT_4TH, TXT_5TH, TXT_6TH, TXT_7TH, TXT_8TH, TXT_9TH, TXT_10TH };

void CScoreManager::Add (int bAbort)
{
if (IsMultiGame && !IsCoopGame)
	return;

Load ();

int position = MAX_HIGH_SCORES;
for (int i = 0; i < MAX_HIGH_SCORES; i++) {
	if (LOCALPLAYER.score > m_scores.stats [i].score) {
		position = i;
		break;
		}
	}

if (position == MAX_HIGH_SCORES) {
	if (bAbort)
		return;
	InitStats (m_lastGame);
	} 
else {
	if (position == 0) {
			CMenu	m (10);
			char	text1 [COOL_MESSAGE_LEN + 10];

		strcpy (text1,  "");
		m.AddText ("", const_cast<char*> (TXT_COOL_SAYING));
		m.AddInput ("", text1, COOL_MESSAGE_LEN - 5);
		m.Menu (TXT_HIGH_SCORE, TXT_YOU_PLACED_1ST);
		strncpy (m_scores.szCoolSaying, text1, COOL_MESSAGE_LEN);
		if (strlen (m_scores.szCoolSaying)<1)
			sprintf (m_scores.szCoolSaying, TXT_NO_COMMENT);
		} 
	else {
		TextBox (TXT_HIGH_SCORE, BG_STANDARD, 1, TXT_OK, "%s %s!", TXT_YOU_PLACED, GAMETEXT (57 + position));
		}

	// move everyone down...
	for (int i = MAX_HIGH_SCORES - 1; i > position; i--) 
		m_scores.stats [i] = m_scores.stats [i - 1];
	InitStats(m_scores.stats [position]);
	Save ();
	}
Show (position);
}

//------------------------------------------------------------------------------

void _CDECL_ CScoreManager::RPrintF (int x, int y, const char * format, ...)
{
va_list	args;
char		buffer [128];

va_start (args, format);
vsprintf (buffer, format, args);
va_end (args);

//replace the digit '1' with special wider 1
for (char* p = buffer; *p; p++)
	if (*p == '1') 
		*p = (char) 132;

int w, h, aw;
fontManager.Current ()->StringSize (buffer, w, h, aw);
GrString (LHX (x) - w, LHY (y), buffer);
}

//------------------------------------------------------------------------------

void CScoreManager::RenderItem (int i, tStatsInfo& stats)
{
	char	buffer [20];
	int	y = 7 + 70 + i * 9;

if (i == 0) 
	y -= 8;
if (i == MAX_HIGH_SCORES) 
	y  += 8;
else
	RPrintF (17 + 33 + XX, y + YY, "%d.", i + 1);

if (!strlen (stats.name)) {
	GrPrintF (NULL, LHX (26 + 33 + XX), LHY (y + YY), TXT_EMPTY);
	return;
	}
GrPrintF (NULL, LHX (26 + 33 + XX), LHY (y + YY), "%s", stats.name);
IntToString (stats.score, buffer);
RPrintF (109 + 33 + XX, y + YY, "%s", buffer);
GrPrintF (NULL, LHX (125 + 33 + XX), LHY (y + YY), "%s", MENU_DIFFICULTY_TEXT (stats.diffLevel));
if ((stats.startingLevel > 0) && (stats.endingLevel > 0))
	RPrintF (192 + 33 + XX, y + YY, "%d-%d", stats.startingLevel, stats.endingLevel);
else if ((stats.startingLevel < 0) && (stats.endingLevel > 0))
	RPrintF (192 + 33 + XX, y + YY, "S%d-%d", -stats.startingLevel, stats.endingLevel);
else if ((stats.startingLevel < 0) && (stats.endingLevel < 0))
	RPrintF (192 + 33 + XX, y + YY, "S%d-S%d", -stats.startingLevel, -stats.endingLevel);
else if ((stats.startingLevel > 0) && (stats.endingLevel < 0))
	RPrintF (192 + 33 + XX, y + YY, "%d-S%d", stats.startingLevel, -stats.endingLevel);

int h = stats.seconds / 3600;
int s = stats.seconds % 3600;
int m = s / 60;
s = s % 60;
RPrintF (311 - 42 + XX, y + YY, "%d:%02d:%02d", h, m, s);
}

//------------------------------------------------------------------------------

void CScoreManager::Render (int nCurItem)
{
		sbyte fadeValues [64] = { 1,1,1,2,2,3,4,4,5,6,8,9,10,12,13,15,16,17,19,20,22,23,24,26,27,28,28,29,30,30,31,31,31,31,31,30,30,29,28,28,27,26,24,23,22,20,19,17,16,15,13,12,10,9,8,6,5,4,4,3,2,2,1,1 };

CFrameController fc;
for (fc.Begin (); fc.Continue (); fc.End ()) {
	backgroundManager.Activate (m_background);
	gameData.SetStereoOffsetType (STEREO_OFFSET_NONE);

	fontManager.SetCurrent (MEDIUM3_FONT);
	GrString (0x8000, LHY (15), TXT_HIGH_SCORES);
	fontManager.SetCurrent (SMALL_FONT);
	fontManager.SetColorRGBi (RGBA_PAL2 (31,26,5), 1, 0, 0);
	GrString (LHX (31 + 33 + XX), LHY (46 + 7 + YY), TXT_NAME);
	GrString (LHX (82 + 33 + XX), LHY (46 + 7 + YY), TXT_SCORE);
	GrString (LHX (127 + 33 + XX), LHY (46 + 7 + YY), TXT_SKILL);
	GrString (LHX (170 + 33 + XX), LHY (46 + 7 + YY), TXT_LEVELS);
	GrString (LHX (288-42 + XX), LHY (46 + 7 + YY), TXT_TIME);
	if (nCurItem < 0)
		GrString (0x8000, LHY (175), TXT_PRESS_CTRL_R);
	fontManager.SetColorRGBi (RGBA_PAL2 (28,28,28), 1, 0, 0);
	for (int i = 0; i < MAX_HIGH_SCORES; i++) {
		int c = 28 - i * 2;
		fontManager.SetColorRGBi (RGBA_PAL2 (c, c, c), 1, 0, 0);
		RenderItem (i, m_scores.stats [i]);
		}
	paletteManager.EnableEffect ();
	if ((nCurItem >= 0) && m_bHilite) {
		int c = 7 + fadeValues [m_nFade];
		fontManager.SetColorRGBi (RGBA_PAL2 (c, c, c), 1, 0, 0);
		if (++m_nFade > 63) 
			m_nFade = 0;
		if (nCurItem ==  MAX_HIGH_SCORES)
			RenderItem (MAX_HIGH_SCORES, m_lastGame);
		else
			RenderItem (nCurItem, m_scores.stats [nCurItem]);
		}	
	m_background.Deactivate ();
	}
ogl.Update (0);
}

//------------------------------------------------------------------------------

int CScoreManager::HandleInput (int nCurItem)
{
for (int i = 0; i < 4; i++)
	if (JoyGetButtonDownCnt (i) > 0) 
		return 0;
for (int i = 0; i < 3; i++)
	if (MouseButtonDownCount (i) > 0) 
		return 0;

int k = KeyInKey ();
switch (k) {
	case KEY_CTRLED + KEY_R:	
		if (nCurItem < 0)	{
			// Reset m_scores...
			if (TextBox (NULL, BG_STANDARD, 2,  TXT_NO, TXT_YES, TXT_RESET_HIGH_SCORES) == 1) {
				CFile::Delete (GetFilename (), gameFolders.game.szData [0]);
				paletteManager.DisableEffect ();
				Load ();
			}
		}
		break;

	case KEY_PRINT_SCREEN:		
		SaveScreenShot (NULL, 0); 
		break;
		
	case KEY_ENTER:
	case KEY_SPACEBAR:
	case KEY_ESC:
		return 0;
		break;
	}
return 1;
}

//------------------------------------------------------------------------------

void CScoreManager::Show (int nCurItem)
{

gameStates.render.nFlashScale = 0;
SetScreenMode (SCREEN_MENU);
Load ();
int nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_FIXED);
backgroundManager.Setup (m_background, 640, 480, -BG_TOPMENU, -BG_SCORES);

GameFlushInputs ();

fix t0 = 0, t;

m_nFade = 0;

do {
	m_bHilite = false;
	if (nCurItem > -1) {
		t	= SDL_GetTicks ();
		if ((m_bHilite = (t - t0 >= 10)))
			t0 = t;
		}
	Render (nCurItem);
	redbook.CheckRepeat ();
} while (HandleInput (nCurItem));
paletteManager.DisableEffect ();
GameFlushInputs ();
gameData.SetStereoOffsetType (nOffsetSave);
}

//------------------------------------------------------------------------------
//eof

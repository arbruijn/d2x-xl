/* $Id: scores.c,v 1.5 2003/10/04 02:58:23 btb Exp $ */
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

#include "scores.h"
#include "error.h"
#include "inferno.h"
#include "gr.h"
#include "mono.h"
#include "key.h"
#include "palette.h"
#include "game.h"
#include "gamefont.h"
#include "u_mem.h"
#include "songs.h"
#include "newmenu.h"
#include "menu.h"
#include "player.h"
#include "screens.h"
#include "gamefont.h"
#include "mouse.h"
#include "joy.h"
#include "timer.h"
#include "text.h"
#include "d_io.h"
#include "strutil.h"
#include "ogl_init.h"

#define VERSION_NUMBER 		1
#define SCORES_FILENAME 	"descent.hi"
#define COOL_MESSAGE_LEN 	50
#define MAX_HIGH_SCORES 	10

typedef struct stats_info {
  	char	name[CALLSIGN_LEN+1];
	int		score;
	sbyte   startingLevel;
	sbyte   endingLevel;
	sbyte   diffLevel;
	short 	kill_ratio;		// 0-100
	short	hostage_ratio;  // 
	int		seconds;		// How long it took in seconds...
} stats_info;

typedef struct all_scores {
	char			nSignature[3];			// DHS
	sbyte           version;				// version
	char			cool_saying[COOL_MESSAGE_LEN];
	stats_info	stats[MAX_HIGH_SCORES];
} all_scores;

static all_scores Scores;

stats_info Last_game;

static int xOffs = 0, yOffs = 0;

char scores_filename[128];

#define XX		(7)
#define YY		(-3)

#define LHX(x)	(gameStates.menus.bHires ? 2 * (x) : x)
#define LHY(y)	(gameStates.menus.bHires ? (24 * (y)) / 10 : y)

//------------------------------------------------------------------------------

char *get_scores_filename ()
{
#ifdef _DEBUG
	// Only use the MINER variable for internal developement
	char *p;
	p=getenv ("MINER");
	if (p)	{
		sprintf (scores_filename, "%s\\game\\%s", p, SCORES_FILENAME);
		Assert (strlen (scores_filename) < 128);
		return scores_filename;
	}
#endif
	sprintf (scores_filename, "%s", SCORES_FILENAME);
	return scores_filename;
}

//------------------------------------------------------------------------------

#define COOL_SAYING TXT_REGISTER_DESCENT

void scores_read ()
{
	CFILE *fp;
	int fsize;

	// clear score array...
	memset (&Scores, 0, sizeof (all_scores));

	fp = CFOpen (get_scores_filename (), gameFolders.szDataDir, "rb", 0);
	if (fp==NULL) {
		int i;

	 	// No error message needed, code will work without a scores file
		sprintf (Scores.cool_saying, COOL_SAYING);
		sprintf (Scores.stats[0].name, "Parallax");
		sprintf (Scores.stats[1].name, "Matt");
		sprintf (Scores.stats[2].name, "Mike");
		sprintf (Scores.stats[3].name, "Adam");
		sprintf (Scores.stats[4].name, "Mark");
		sprintf (Scores.stats[5].name, "Jasen");
		sprintf (Scores.stats[6].name, "Samir");
		sprintf (Scores.stats[7].name, "Doug");
		sprintf (Scores.stats[8].name, "Dan");
		sprintf (Scores.stats[9].name, "Jason");

		for (i=0; i<10; i++)
			Scores.stats[i].score = (10-i)*1000;
		return;
	}
		
	fsize = CFLength (fp,0);

	if (fsize != sizeof (all_scores))	{
		CFClose (fp);
		return;
	}
	// Read 'em in...
	CFRead (&Scores, sizeof (all_scores), 1, fp);
	CFClose (fp);

	if ((Scores.version!=VERSION_NUMBER)|| (Scores.nSignature[0]!='D')|| (Scores.nSignature[1]!='H')|| (Scores.nSignature[2]!='S'))	{
		memset (&Scores, 0, sizeof (all_scores));
		return;
	}
}

//------------------------------------------------------------------------------

void scores_write ()
{
	CFILE *fp;

	fp = CFOpen (get_scores_filename (), gameFolders.szDataDir, "wb", 0);
	if (fp==NULL) {
		ExecMessageBox (TXT_WARNING, NULL, 1, TXT_OK, "%s\n'%s'", TXT_UNABLE_TO_OPEN, get_scores_filename () );
		return;
	}

	Scores.nSignature[0]='D';
	Scores.nSignature[1]='H';
	Scores.nSignature[2]='S';
	Scores.version = VERSION_NUMBER;
	CFWrite (&Scores,sizeof (all_scores),1, fp);
	CFClose (fp);
}

//------------------------------------------------------------------------------

void int_to_string (int number, char *dest)
{
	int i,l,c;
	char buffer[20],*p;

	sprintf (buffer, "%d", number);

	l = (int) strlen (buffer);
	if (l<=3) {
		// Don't bother with less than 3 digits
		sprintf (dest, "%d", number);
		return;
	}

	c = 0;
	p=dest;
	for (i=l-1; i>=0; i--)	{
		if (c==3) {
			*p++=',';
			c = 0;
		}
		c++;
		*p++ = buffer[i];
	}
	*p++ = '\0';
	strrev (dest);
}

//------------------------------------------------------------------------------

void scores_fill_struct (stats_info * stats)
{
		strcpy (stats->name, LOCALPLAYER.callsign);
		stats->score = LOCALPLAYER.score;
		stats->endingLevel = LOCALPLAYER.level;
		if (LOCALPLAYER.numRobotsTotal > 0)	
			stats->kill_ratio = (LOCALPLAYER.numKillsTotal*100)/LOCALPLAYER.numRobotsTotal;
		else
			stats->kill_ratio = 0;

		if (LOCALPLAYER.hostagesTotal > 0)	
			stats->hostage_ratio = (LOCALPLAYER.hostages_rescuedTotal*100)/LOCALPLAYER.hostagesTotal;
		else
			stats->hostage_ratio = 0;

		stats->seconds = f2i (LOCALPLAYER.timeTotal)+ (LOCALPLAYER.hoursTotal*3600);

		stats->diffLevel = gameStates.app.nDifficultyLevel;
		stats->startingLevel = LOCALPLAYER.startingLevel;
}

//------------------------------------------------------------------------------
//char * score_placement[10] = { TXT_1ST, TXT_2ND, TXT_3RD, TXT_4TH, TXT_5TH, TXT_6TH, TXT_7TH, TXT_8TH, TXT_9TH, TXT_10TH };

void MaybeAddPlayerScore (int abortFlag)
{
	char text1[COOL_MESSAGE_LEN+10];
	tMenuItem m[10];
	int i,position;

	#ifdef APPLE_DEMO		// no high scores in apple oem version
	return;
	#endif

	if ((gameData.app.nGameMode & GM_MULTI) && ! (gameData.app.nGameMode & GM_MULTI_COOP))
		return;
  
	scores_read ();
	
	position = MAX_HIGH_SCORES;
	for (i=0; i<MAX_HIGH_SCORES; i++)	{
		if (LOCALPLAYER.score > Scores.stats[i].score)	{
			position = i;
			break;
		}
	}
	
	if (position == MAX_HIGH_SCORES) {
		if (abortFlag)
			return;
		scores_fill_struct (&Last_game);
	} else {
//--		if (gameStates.app.nDifficultyLevel < 1)	{
//--			ExecMessageBox ("GRADUATION TIME!", 1, "Ok", "If you would had been\nplaying at a higher difficulty\nlevel, you would have placed\n#%d on the high score list.", position+1);
//--			return;
//--		}

		memset (m, 0, sizeof (m));
		if (position==0)	{
			strcpy (text1,  "");
			m[0].nType = NM_TYPE_TEXT; m[0].text = TXT_COOL_SAYING;
			m[1].nType = NM_TYPE_INPUT; m[1].text = text1; m[1].text_len = COOL_MESSAGE_LEN-5;
			ExecMenu (TXT_HIGH_SCORE, TXT_YOU_PLACED_1ST, 2, m, NULL, NULL);
			strncpy (Scores.cool_saying, text1, COOL_MESSAGE_LEN);
			if (strlen (Scores.cool_saying)<1)
				sprintf (Scores.cool_saying, TXT_NO_COMMENT);
		} else {
			ExecMessageBox (TXT_HIGH_SCORE, NULL, 1, TXT_OK, "%s %s!", TXT_YOU_PLACED, GAMETEXT (57 + position));
		}
	
		// move everyone down...
		for (i=MAX_HIGH_SCORES-1; i>position; i--)	{
			Scores.stats[i] = Scores.stats[i-1];
		}

		scores_fill_struct (&Scores.stats[position]);
	
		scores_write ();

	}
	ScoresView (position);
}

//------------------------------------------------------------------------------

void _CDECL_ scores_rprintf (int x, int y, char * format, ...)
{
	va_list args;
	char buffer[128];
	int w, h, aw;
	char *p;

	va_start (args, format);
	vsprintf (buffer,format,args);
	va_end (args);

	//replace the digit '1' with special wider 1
	for (p=buffer;*p;p++)
		if (*p=='1') 
			*p= (char)132;

	GrGetStringSize (buffer, &w, &h, &aw);

	GrString (LHX (x)-w+xOffs, LHY (y)+yOffs, buffer, NULL);
}

//------------------------------------------------------------------------------

void scores_draw_item (int  i, stats_info * stats)
{
	char buffer[20];

		int y;

	WIN (DDGRLOCK (dd_grd_curcanv));
		y = 7+70+i*9;

		if (i==0) y -= 8;

		if (i==MAX_HIGH_SCORES) 	{
			y += 8;
			//scores_rprintf (17+33+XX, y+YY, "");
		} else {
			scores_rprintf (17+33+XX, y+YY, "%d.", i+1);
		}

		if (strlen (stats->name)==0) {
			GrPrintF (NULL, LHX (26+33+XX)+xOffs, LHY (y+YY)+yOffs, TXT_EMPTY);
			WIN (DDGRUNLOCK (dd_grd_curcanv));
			return;
		}
		GrPrintF (NULL, LHX (26+33+XX)+xOffs, LHY (y+YY)+yOffs, "%s", stats->name);
		int_to_string (stats->score, buffer);
		scores_rprintf (109+33+XX, y+YY, "%s", buffer);

		GrPrintF (NULL, LHX (125+33+XX)+xOffs, LHY (y+YY)+yOffs, "%s", MENU_DIFFICULTY_TEXT (stats->diffLevel));

		if ((stats->startingLevel > 0) && (stats->endingLevel > 0))
			scores_rprintf (192+33+XX, y+YY, "%d-%d", stats->startingLevel, stats->endingLevel);
		else if ((stats->startingLevel < 0) && (stats->endingLevel > 0))
			scores_rprintf (192+33+XX, y+YY, "S%d-%d", -stats->startingLevel, stats->endingLevel);
		else if ((stats->startingLevel < 0) && (stats->endingLevel < 0))
			scores_rprintf (192+33+XX, y+YY, "S%d-S%d", -stats->startingLevel, -stats->endingLevel);
		else if ((stats->startingLevel > 0) && (stats->endingLevel < 0))
			scores_rprintf (192+33+XX, y+YY, "%d-S%d", stats->startingLevel, -stats->endingLevel);

		{
			int h, m, s;
			h = stats->seconds/3600;
			s = stats->seconds%3600;
			m = s / 60;
			s = s % 60;
			scores_rprintf (311-42+XX, y+YY, "%d:%02d:%02d", h, m, s);
		}
	WIN (DDGRUNLOCK (dd_grd_curcanv));
}

//------------------------------------------------------------------------------

void ScoresView (int citem)
{
	fix t0 = 0, t1;
	int c,i,done,looper;
	int k, bRedraw = 0;
	sbyte fades[64] = { 1,1,1,2,2,3,4,4,5,6,8,9,10,12,13,15,16,17,19,20,22,23,24,26,27,28,28,29,30,30,31,31,31,31,31,30,30,29,28,28,27,26,24,23,22,20,19,17,16,15,13,12,10,9,8,6,5,4,4,3,2,2,1,1 };
	bkg bg;
	
	memset (&bg, 0, sizeof (bg));

ReshowScores:
	scores_read ();

	SetScreenMode (SCREEN_MENU);
 
	WINDOS (	DDGrSetCurrentCanvas (NULL),
				GrSetCurrentCanvas (NULL)
	);
	
	xOffs = (grdCurCanv->cvBitmap.bmProps.w - 640) / 2;
	yOffs = (grdCurCanv->cvBitmap.bmProps.h - 480) / 2;
	if (xOffs < 0)
		xOffs = 0;
	if (yOffs < 0)
		yOffs = 0; 

	GameFlushInputs ();

	done = 0;
	looper = 0;

	while (!done)	{
		if (!bRedraw || gameOpts->menus.nStyle) {
			NMDrawBackground (&bg,xOffs, yOffs, xOffs + 640, xOffs + 480, bRedraw);
			grdCurCanv->cvFont = MEDIUM3_FONT;

		WIN (DDGRLOCK (dd_grd_curcanv));
			GrString (0x8000, yOffs + LHY (15), TXT_HIGH_SCORES, NULL);
			grdCurCanv->cvFont = SMALL_FONT;
			GrSetFontColorRGBi (RGBA_PAL (31,26,5), 1, 0, 0);
			GrString ( xOffs + LHX (31+33+XX), yOffs + LHY (46+7+YY), TXT_NAME, NULL);
			GrString ( xOffs + LHX (82+33+XX), yOffs + LHY (46+7+YY), TXT_SCORE, NULL);
			GrString (xOffs + LHX (127+33+XX), yOffs + LHY (46+7+YY), TXT_SKILL, NULL);
			GrString (xOffs + LHX (170+33+XX), yOffs + LHY (46+7+YY), TXT_LEVELS, NULL);
		//	GrString (202, 46, "Kills");
		//	GrString (234, 46, "Rescues");
			GrString (xOffs + LHX (288-42+XX), yOffs + LHY (46+7+YY), TXT_TIME, NULL);
			if (citem < 0)	
				GrString (0x8000, yOffs + LHY (175), TXT_PRESS_CTRL_R, NULL);
			GrSetFontColorRGBi (RGBA_PAL (28,28,28), 1, 0, 0);
			//GrPrintF (NULL, 0x8000, yOffs + LHY (31), "%c%s%c  - %s", 34, Scores.cool_saying, 34, Scores.stats[0].name);
		WIN (DDGRUNLOCK (dd_grd_curcanv));	
			for (i = 0; i < MAX_HIGH_SCORES; i++) {
				//@@if (i==0)	{
				//@@	GrSetFontColorRGBi (RGBA_PAL (28,28,28), 1, 0, 0);
				//@@} else {
				//@@	GrSetFontColor (grFadeTable[BM_XRGB (28,28,28)+ ((28-i*2)*256)], 1, 0, 0);
				//@@}														 
				c = 28 - i * 2;
				GrSetFontColorRGBi (RGBA_PAL (c, c, c), 1, 0, 0);
				scores_draw_item (i, Scores.stats + i);
			}

			GrPaletteFadeIn (NULL,32, 0);

			if (citem < 0)
				GrUpdate (0);
			bRedraw = 1;
			}
		if (citem > -1)	{
	
			t1	= TimerGetFixedSeconds ();
			//if (t1 - t0 >= F1_0/128) 
			{
				t0 = t1;
				//@@GrSetFontColor (grFadeTable[fades[looper]*256+BM_XRGB (28,28,28)], -1);
				c = 7 + fades [looper];
				GrSetFontColorRGBi (RGBA_PAL (c, c, c), 1, 0, 0);
				if (++looper > 63) 
				 looper=0;
				if (citem ==  MAX_HIGH_SCORES)
					scores_draw_item (MAX_HIGH_SCORES, &Last_game);
				else
					scores_draw_item (citem, Scores.stats + citem);
				}
			GrUpdate (0);
		}

		for (i=0; i<4; i++)	
			if (JoyGetButtonDownCnt (i)>0) done=1;
		for (i=0; i<3; i++)	
			if (MouseButtonDownCount (i)>0) done=1;

		//see if redbook song needs to be restarted
		SongsCheckRedbookRepeat ();

		k = KeyInKey ();
		switch (k)	{
		case KEY_CTRLED+KEY_R:		
			if (citem < 0)		{
				// Reset scores...
				if (ExecMessageBox (NULL, NULL, 2,  TXT_NO, TXT_YES, TXT_RESET_HIGH_SCORES)==1)	{
					CFDelete (get_scores_filename (), gameFolders.szDataDir);
					GrPaletteFadeOut (NULL, 32, 0);
					goto ReshowScores;
				}
			}
			break;
		case KEY_BACKSP:				Int3 (); k = 0; break;
		case KEY_PRINT_SCREEN:		SaveScreenShot (NULL, 0); k = 0; break;
			
		case KEY_ENTER:
		case KEY_SPACEBAR:
		case KEY_ESC:
			done=1;
			break;
		}
	}

// Restore background and exit
	GrPaletteFadeOut (NULL, 32, 0);

	WINDOS (	DDGrSetCurrentCanvas (NULL),
				GrSetCurrentCanvas (NULL)
	);

	GameFlushInputs ();
	NMRemoveBackground (&bg);
	
}

//------------------------------------------------------------------------------
//eof

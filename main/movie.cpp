/* $Id: movie.c, v 1.33 2003/11/26 12:26:30 btb Exp $ */
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

#ifdef RCS
static char rcsid [] = "$Id: movie.c, v 1.33 2003/11/26 12:26:30 btb Exp $";
#endif

#define DEBUG_LEVEL CON_NORMAL

#include <string.h>
#ifndef _WIN32_WCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#ifndef _WIN32
#include <unistd.h>
#endif
#include <ctype.h>

#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
# if USE_SDL_MIXER
#  include <SDL_mixer.h>
# endif
#endif

#include "inferno.h"
#include "movie.h"
#include "console.h"
#include "args.h"
#include "key.h"
#include "digi.h"
#include "songs.h"
#include "palette.h"
#include "strutil.h"
#include "error.h"
#include "u_mem.h"
#include "byteswap.h"
#include "gr.h"
#include "ogl_defs.h"
#include "ogl_bitmap.h"
#include "gamefont.h"
#include "cfile.h"
#include "menu.h"
#include "libmve.h"
#include "text.h"
#include "screens.h"

extern char CDROM_dir [];

#define VID_PLAY 0
#define VID_PAUSE 1

int Vid_State;
// Subtitle data
typedef struct {
	short first_frame, last_frame;
	char *msg;
} subtitle;

#define MAX_SUBTITLES 500
#define MAX_ACTIVE_SUBTITLES 3

typedef struct tSubTitles {
	subtitle captions [MAX_SUBTITLES];
	int		nCaptions;
	ubyte		*rawDataP;
} tSubTitles;

tSubTitles subTitles;

// Movielib data
typedef struct {
	char name [FILENAME_LEN];
	int offset, len;
} ml_entry;

typedef struct {
	char     name [100]; // [FILENAME_LEN];
	int      n_movies;
	ubyte    flags;
	ubyte		pad [3];
	ml_entry *movies;
	int		bLittleEndian;
} tMovieLib;

char pszMovieLibs [][FILENAME_LEN] = {
	"intro-l.mvl", 
	"other-l.mvl", 
	"robots-l.mvl", 
	"d2x-l.mvl", 
	"extra1-l.mvl", 
	"extra2-l.mvl", 
	"extra3-l.mvl", 
	"extra4-l.mvl", 
	"extra5-l.mvl"};

#define MLF_ON_CD						1
#define MAX_MOVIES_PER_LIB			50    //determines size of D2_ALLOC

#define	FIRST_EXTRA_MOVIE_LIB	4
#define N_EXTRA_MOVIE_LIBS			5
#define N_BUILTIN_MOVIE_LIBS		(sizeof (pszMovieLibs) / sizeof (*pszMovieLibs))
#define N_MOVIE_LIBS					(N_BUILTIN_MOVIE_LIBS+1)
#define EXTRA_ROBOT_LIB				N_BUILTIN_MOVIE_LIBS

typedef struct tRobotMovie {
	CFILE		cf;
	int		nFilePos;
	int		bLittleEndian;
} tRobotMovie;

typedef struct tMovieData {
	tMovieLib	*libs [N_MOVIE_LIBS];
	ubyte			*palette;
	tRobotMovie	robot;
} tMovieData;

tMovieData	movies;

// Function Prototypes
int RunMovie (char *filename, int highresFlag, int allow_abort, int dx, int dy);

int OpenMovieFile (CFILE *cfp, char *filename, int bRequired);
int ResetMovieFile (CFILE *cfp);

void ChangeFilenameExt (char *dest, char *src, char *ext);
void DecodeTextLine (char *p);
void DrawSubTitles (int nFrame);
tMovieLib *FindMovieLib (char *pszTargetMovie);

// ----------------------------------------------------------------------

void *MPlayAlloc (unsigned size)
{
return D2_ALLOC (size);
}

// ----------------------------------------------------------------------

void MPlayFree (void *p)
{
D2_FREE (p);
}


//-----------------------------------------------------------------------

unsigned int FileRead (void *handle, void *buf, unsigned int count)
{
unsigned int numread = (unsigned int) CFRead (buf, 1, count, (CFILE *)handle);
return numread == count;
}


//-----------------------------------------------------------------------

//filename will actually get modified to be either low-res or high-res
//returns status.  see values in movie.h

int PlayMovie (const char *filename, int bRequired, int bForce, int bFullScreen)
{
	char name [FILENAME_LEN], *p;
	int c, ret;

#if 1//ndef _DEBUG
if (!bForce && (gameOpts->movies.nLevel < 2))
	return MOVIE_NOT_PLAYED;
#endif
strcpy (name, filename);
if (!(p = strchr (name, '.')))		//add extension, if missing
	strcat (name, ".mve");
//check for escape already pressed & abort if so
while ((c = KeyInKey ()))
	if (c == KEY_ESC)
		return MOVIE_ABORTED;
// Stop all digital sounds currently playing.
DigiExit ();
// Start sound
MVE_sndInit (gameStates.app.bUseSound ? 1 : -1);
gameOpts->movies.bFullScreen = bFullScreen;
ret = RunMovie (name, gameOpts->movies.bHires, bRequired, -1, -1);
DigiInit (1);
gameStates.video.nScreenMode = -1;		//force screen reset
return ret;
}

//-----------------------------------------------------------------------

void MovieShowFrame (ubyte *buf, uint bufw, uint bufh, uint sx, uint sy, 
							uint w, uint h, uint dstx, uint dsty)
{
	grsBitmap bmFrame;

Assert (bufw == w && bufh == h);

memset (&bmFrame, 0, sizeof (bmFrame));
bmFrame.bmProps.w = bmFrame.bmProps.rowSize = bufw;
bmFrame.bmProps.h = bufh;
bmFrame.bmProps.nType = BM_LINEAR;
bmFrame.bmTexBuf = buf;
bmFrame.bmPalette = movies.palette;

TRANSPARENCY_COLOR = -1;
if (gameOpts->menus.nStyle) {
	//memset (grPalette, 0, 7
);
	//GrPaletteStepLoad (NULL);
	}
if (gameOpts->movies.bFullScreen) {
	double r = (double) bufh / (double) bufw;
	int dh = (int) (grdCurCanv->cvBitmap.bmProps.w * r);
	int yOffs = (grdCurCanv->cvBitmap.bmProps.h - dh) / 2;

	glDisable (GL_BLEND);
	OglUBitBltI (grdCurCanv->cvBitmap.bmProps.w, dh, 0, yOffs, 
					 bufw, bufh, sx, sy, 
					 &bmFrame, &grdCurCanv->cvBitmap, 
					 gameOpts->movies.nQuality, 1, 1.0f);
	glEnable (GL_BLEND);
	}
else {
	int xOffs = (grdCurCanv->cvBitmap.bmProps.w - 640) / 2;
	int yOffs = (grdCurCanv->cvBitmap.bmProps.h - 480) / 2;

	if (xOffs < 0)
		xOffs = 0;
	if (yOffs < 0)
		yOffs = 0;
	dstx += xOffs;
	dsty += yOffs;
	if ((grdCurCanv->cvBitmap.bmProps.w > 640) || (grdCurCanv->cvBitmap.bmProps.h > 480)) {
		GrSetColorRGBi (RGBA_PAL (0, 0, 32));
		GrUBox (dstx-1, dsty, dstx+w, dsty+h+1);
		}
	GrBmUBitBlt (bufw, bufh, dstx, dsty, sx, sy, &bmFrame, &grdCurCanv->cvBitmap, 1);
	}
TRANSPARENCY_COLOR = DEFAULT_TRANSPARENCY_COLOR;
}

//-----------------------------------------------------------------------
//our routine to set the palette, called from the movie code
void MovieSetPalette (unsigned char *p, unsigned start, unsigned count)
{
	tPalette	palette;

if (count == 0)
	return;

//GrPaletteStepLoad (NULL);
//Color 0 should be black, and we get color 255
//movie libs palette into our array
Assert (start>=0 && start+count<=256);
memcpy (palette + start * 3, p + start * 3, count * 3);
//Set color 0 to be black
palette [0] = palette [1] = palette [2] = 0;
//Set color 255 to be our subtitle color
palette [765] = palette [766] = palette [767] = 50;
//finally set the palette in the hardware
movies.palette = AddPalette (palette);
GrPaletteStepLoad (NULL);
}

//-----------------------------------------------------------------------

#define BOX_BORDER (gameStates.menus.bHires ? 40 : 20)

void ShowPauseMessage (char *msg)
{
	int w, h, aw;
	int x, y;

GrSetCurrentCanvas (NULL);
GrSetCurFont (SMALL_FONT);
GrGetStringSize (msg, &w, &h, &aw);
x = (grdCurScreen->scWidth - w) / 2;
y = (grdCurScreen->scHeight - h) / 2;
#if 0
if (movie_bg.bmp) {
	GrFreeBitmap (movie_bg.bmp);
	movie_bg.bmp = NULL;
	}
// Save the background of the display
movie_bg.x=x; 
movie_bg.y=y; 
movie_bg.w=w; 
movie_bg.h=h;
movie_bg.bmp = GrCreateBitmap (w+BOX_BORDER, h+BOX_BORDER, 1);
GrBmUBitBlt (w+BOX_BORDER, h+BOX_BORDER, 0, 0, x-BOX_BORDER/2, y-BOX_BORDER/2, & (grdCurCanv->cvBitmap), movie_bg.bmp);
#endif
GrSetColorRGB (0, 0, 0, 255);
GrRect (x-BOX_BORDER/2, y-BOX_BORDER/2, x+w+BOX_BORDER/2-1, y+h+BOX_BORDER/2-1);
GrSetFontColor (255, -1);
GrUString (0x8000, y, msg);
GrUpdate (0);
}

//-----------------------------------------------------------------------

void ClearPauseMessage ()
{
#if 0
if (movie_bg.bmp) {
	GrBitmap (movie_bg.x-BOX_BORDER/2, movie_bg.y-BOX_BORDER/2, movie_bg.bmp);
	GrFreeBitmap (movie_bg.bmp);
	movie_bg.bmp = NULL;
	}
#endif
}

//-----------------------------------------------------------------------
//returns status.  see movie.h
int RunMovie (char *filename, int hiresFlag, int bRequired, int dx, int dy)
{
	CFILE			cf;
	int			result=1, aborted=0;
	int			track = 0;
	int			nFrame;
	int			key;
	tMovieLib	*libP = FindMovieLib (filename);

result = 1;
// Open Movie file.  If it doesn't exist, no movie, just return.
if (!(CFOpen (&cf, filename, gameFolders.szDataDir, "rb", 0) || OpenMovieFile (&cf, filename, bRequired))) {
	if (bRequired) {
#if TRACE
		con_printf (CON_NORMAL, "movie: RunMovie: Cannot open movie <%s>\n", filename);
#endif
		}
	return MOVIE_NOT_PLAYED;
	}
MVE_memCallbacks (MPlayAlloc, MPlayFree);
MVE_ioCallbacks (FileRead);
SetScreenMode (SCREEN_MENU);
GrPaletteStepLoad (NULL);
MVE_sfCallbacks (MovieShowFrame);
MVE_palCallbacks (MovieSetPalette);
if (MVE_rmPrepMovie ((void *) &cf, dx, dy, track, libP ? libP->bLittleEndian : 1)) {
	Int3 ();
	return MOVIE_NOT_PLAYED;
	}
nFrame = 0;
gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && hiresFlag;
while ((result = MVE_rmStepMovie ()) == 0) {
	DrawSubTitles (nFrame);
	GrPaletteStepLoad (NULL); // moved this here because of flashing
	GrUpdate (1);
	key = KeyInKey ();
	// If ESCAPE pressed, then quit movie.
	if (key == KEY_ESC) {
		result = aborted = 1;
		break;
		}
	// If PAUSE pressed, then pause movie
	if (key == KEY_PAUSE) {
		MVE_rmHoldMovie ();
		ShowPauseMessage (TXT_PAUSE);
		while (!KeyInKey ()) 
			;
		ClearPauseMessage ();
		}
	if ((key == KEY_ALTED+KEY_ENTER) || (key == KEY_ALTED+KEY_PADENTER))
		GrToggleFullScreen ();
	nFrame++;
	}
Assert (aborted || result == MVE_ERR_EOF);	 ///movie should be over
MVE_rmEndMovie ();
CFClose (&cf);                           // Close Movie File
// Restore old graphic state
gameStates.video.nScreenMode = -1;  //force reset of screen mode
GrPaletteStepLoad (NULL);
return (aborted ? MOVIE_ABORTED : MOVIE_PLAYED_FULL);
}

//-----------------------------------------------------------------------

int InitMovieBriefing ()
{
#if 0
if (gameStates.menus.bHires)
	GrSetMode (SM (640, 480);
else
	GrSetMode (SM (320, 200);
GrInitSubCanvas (&gameStates.render.vr.buffers.screenPages [0], &grdCurScreen->scCanvas, 0, 0, grdCurScreen->scWidth, grdCurScreen->scHeight);
GrInitSubCanvas (&gameStates.render.vr.buffers.screenPages [1], &grdCurScreen->scCanvas, 0, 0, grdCurScreen->scWidth, grdCurScreen->scHeight);
#endif
return 1;
}

//-----------------------------------------------------------------------
//returns 1 if frame updated ok
int RotateRobot ()
{
	int err;

gameOpts->movies.bFullScreen = 1;
if (curDrawBuffer == GL_BACK)
	GrPaletteStepLoad (NULL);
err = MVE_rmStepMovie ();
GrPaletteStepLoad (NULL);
if (err == MVE_ERR_EOF) {   //end of movie, so reset
	ResetMovieFile (&movies.robot.cf);
	if (MVE_rmPrepMovie ((void *) &movies.robot.cf, 
								gameStates.menus.bHires ? 280 : 140, 
								gameStates.menus.bHires ? 200 : 80, 0,
								movies.robot.bLittleEndian)) {
		Int3 ();
		return 0;
		}
	}
else if (err) {
	Int3 ();
	return 0;
	}
return 1;
}

//-----------------------------------------------------------------------

void DeInitRobotMovie (void)
{
MVE_rmEndMovie ();
CFClose (&movies.robot.cf);                           // Close Movie File
}

//-----------------------------------------------------------------------

int InitRobotMovie (char *filename)
{
	tMovieLib	*libP = FindMovieLib (filename);

if (gameOpts->movies.nLevel < 1)
	return 0;

#if TRACE
con_printf (DEBUG_LEVEL, "movies.robot.cf=%s\n", filename);
#endif
MVE_sndInit (-1);        //tell movies to play no sound for robots
if (!OpenMovieFile (&movies.robot.cf, filename, 1)) {
#ifdef _DEBUG
	Warning (TXT_MOVIE_ROBOT, filename);
#endif
	return MOVIE_NOT_PLAYED;
	}
Vid_State = VID_PLAY;
gameOpts->movies.bFullScreen = 1;
movies.robot.bLittleEndian = libP ? libP->bLittleEndian : 1;
MVE_memCallbacks (MPlayAlloc, MPlayFree);
MVE_ioCallbacks (FileRead);
MVE_sfCallbacks (MovieShowFrame);
MVE_palCallbacks (MovieSetPalette);
if (MVE_rmPrepMovie ((void *) &movies.robot.cf, 
							gameStates.menus.bHires ? 280 : 140, 
							gameStates.menus.bHires ? 200 : 80, 0,
							movies.robot.bLittleEndian)) {
	Int3 ();
	return 0;
	}
movies.robot.nFilePos = CFSeek (&movies.robot.cf, 0L, SEEK_CUR);
#if TRACE
con_printf (DEBUG_LEVEL, "movies.robot.nFilePos=%d!\n", movies.robot.nFilePos);
#endif
return 1;
}

//-----------------------------------------------------------------------
/*
 *		Subtitle system code
 */

//search for next field following whitespace 
ubyte *next_field (ubyte *p)
{
	while (*p && !::isspace (*p))
	p++;
if (!*p)
	return NULL;
while (*p && ::isspace (*p))
	p++;
if (!*p)
	return NULL;
return p;
}

//-----------------------------------------------------------------------

int InitSubTitles (char *filename)
{
	CFILE cf;
	int 	size, readCount;
	ubyte	*p;
	int 	bHaveBinary = 0;

subTitles.nCaptions = 0;
if (!gameOpts->movies.bSubTitles)
	return 0;
if (!CFOpen (&cf, filename, gameFolders.szDataDir, "rb", 0)) { // first try text version
	char filename2 [FILENAME_LEN];	//no text version, try binary version
	ChangeFilenameExt (filename2, filename, ".txb");
	if (!CFOpen (&cf, filename2, gameFolders.szDataDir, "rb", 0))
		return 0;
	bHaveBinary = 1;
	}

size = CFLength (&cf, 0);
MALLOC (subTitles.rawDataP, ubyte, size+1);
readCount = (int) CFRead (subTitles.rawDataP, 1, size, &cf);
CFClose (&cf);
subTitles.rawDataP [size] = 0;
if (readCount != size) {
	D2_FREE (subTitles.rawDataP);
	return 0;
	}
p = subTitles.rawDataP;
while (p && (p < subTitles.rawDataP + size)) {
	char *endp = strchr ((char *) p, '\n'); 

	if (endp) {
		if (endp [-1] == '\r')
			endp [-1] = 0;		//handle 0d0a pair
		*endp = 0;			//string termintor
		}
	if (bHaveBinary)
		DecodeTextLine ((char *) p);
	if (*p != ';') {
		subTitles.captions [subTitles.nCaptions].first_frame = atoi ((char *) p);
		if (!(p = next_field (p))) 
			continue;
		subTitles.captions [subTitles.nCaptions].last_frame = atoi ((char *) p);
		if (!(p = next_field (p)))
			continue;
		subTitles.captions [subTitles.nCaptions].msg = (char *) p;
		Assert (subTitles.nCaptions==0 || subTitles.captions [subTitles.nCaptions].first_frame >= subTitles.captions [subTitles.nCaptions-1].first_frame);
		Assert (subTitles.captions [subTitles.nCaptions].last_frame >= subTitles.captions [subTitles.nCaptions].first_frame);
		subTitles.nCaptions++;
		}
	p = (ubyte *) (endp + 1);
	}
return 1;
}

//-----------------------------------------------------------------------

void CloseSubTitles ()
{
if (subTitles.rawDataP)
	D2_FREE (subTitles.rawDataP);
subTitles.rawDataP = NULL;
subTitles.nCaptions = 0;
}

//-----------------------------------------------------------------------
//draw the subtitles for this frame
void DrawSubTitles (int nFrame)
{
	static int activeSubTitleList [MAX_ACTIVE_SUBTITLES];
	static int nActiveSubTitles, nNextSubTitle, nLineSpacing;
	int t, y;
	int bMustErase = 0;

if (nFrame == 0) {
	nActiveSubTitles = 0;
	nNextSubTitle = 0;
	GrSetCurFont (GAME_FONT);
	nLineSpacing = grdCurCanv->cvFont->ftHeight + (grdCurCanv->cvFont->ftHeight >> 2);
	GrSetFontColor (255, -1);
	}

//get rid of any subtitles that have expired
for (t = 0; t <nActiveSubTitles; )
	if (nFrame > subTitles.captions [activeSubTitleList [t]].last_frame) {
		int t2;
		for (t2 = t; t2 < nActiveSubTitles - 1; t2++)
			activeSubTitleList [t2] = activeSubTitleList [t2+1];
		nActiveSubTitles--;
		bMustErase = 1;
	}
	else
		t++;

//get any subtitles new for this frame 
while (nNextSubTitle < subTitles.nCaptions && nFrame >= subTitles.captions [nNextSubTitle].first_frame) {
	if (nActiveSubTitles >= MAX_ACTIVE_SUBTITLES)
		Error ("Too many active subtitles!");
	activeSubTitleList [nActiveSubTitles++] = nNextSubTitle;
	nNextSubTitle++;
	}

//find y coordinate for first line of subtitles
y = grdCurCanv->cvBitmap.bmProps.h- ((nLineSpacing+1)*MAX_ACTIVE_SUBTITLES+2);

//erase old subtitles if necessary
if (bMustErase) {
	GrSetColorRGB (0, 0, 0, 255);
	GrRect (0, y, grdCurCanv->cvBitmap.bmProps.w-1, grdCurCanv->cvBitmap.bmProps.h-1);
	}
//now draw the current subtitles
for (t=0;t<nActiveSubTitles;t++)
	if (activeSubTitleList [t] != -1) {
		GrString (0x8000, y, subTitles.captions [activeSubTitleList [t]].msg, NULL);
		y += nLineSpacing+1;
	}
}

//-----------------------------------------------------------------------

tMovieLib *InitNewMovieLib (char *filename, CFILE *fp)
{
	int		nFiles, offset;
	int		i, n, len, bLittleEndian = gameStates.app.bLittleEndian;
	tMovieLib *table;

	//read movie file header

nFiles = CFReadInt (fp);        //get number of files
if (nFiles > 255) {
	gameStates.app.bLittleEndian = 0;
	nFiles = SWAPINT (nFiles);
	}
//table = D2_ALLOC (sizeof (*table) + sizeof (ml_entry)*nFiles);
MALLOC (table, tMovieLib, 1);
MALLOC (table->movies, ml_entry, nFiles);
strcpy (table->name, filename);
table->bLittleEndian = gameStates.app.bLittleEndian;
table->n_movies = nFiles;
offset = 4 + 4 + nFiles * (13 + 4);	//id + nFiles + nFiles * (filename + size)
for (i = 0; i < nFiles; i++) {
	n = (int) CFRead (table->movies [i].name, 13, 1, fp);
	if (n != 1)
		break;		//end of file (probably)
	len = CFReadInt (fp);
	table->movies [i].len = len;
	table->movies [i].offset = offset;
	offset += table->movies [i].len;
	}
CFClose (fp);
table->flags = 0;
gameStates.app.bLittleEndian = bLittleEndian;
return table;
}

//-----------------------------------------------------------------------

tMovieLib *InitOldMovieLib (char *filename, CFILE *fp)
{
	int nFiles, size;
	int i, len;
	tMovieLib *table, *table2;

	nFiles = 0;

	//allocate big table
table = (tMovieLib *) D2_ALLOC (sizeof (*table) + sizeof (ml_entry) * MAX_MOVIES_PER_LIB);
while (1) {
	i = (int) CFRead (table->movies [nFiles].name, 13, 1, fp);
	if (i != 1)
		break;		//end of file (probably)
	i = (int) CFRead (&len, 4, 1, fp);
	if (i != 1) {
		Error ("error reading movie library <%s>", filename);
		return NULL;
		}
	table->movies [nFiles].len = INTEL_INT (len);
	table->movies [nFiles].offset = CFTell (fp);
	CFSeek (fp, INTEL_INT (len), SEEK_CUR);       //skip data
	nFiles++;
	}
	//allocate correct-sized table
size = sizeof (*table) + sizeof (ml_entry) * nFiles;
table2 = (tMovieLib *) D2_ALLOC (size);
memcpy (table2, table, size);
D2_FREE (table);
table = table2;
strcpy (table->name, filename);
table->n_movies = nFiles;
CFClose (fp);
table->flags = 0;
return table;
}

//-----------------------------------------------------------------------

//find the specified movie library, and read in list of movies in it
tMovieLib *InitMovieLib (char *filename)
{
	//note: this based on CFInitHogFile ()

	char id [4];
	CFILE cf;

if (!CFOpen (&cf, filename, gameFolders.szMovieDir, "rb", 0))
	return NULL;
CFRead (id, 4, 1, &cf);
if (!strncmp (id, "DMVL", 4))
	return InitNewMovieLib (filename, &cf);
else if (!strncmp (id, "DHF", 3)) {
	CFSeek (&cf, -1, SEEK_CUR);		//old file had 3 char id
	return InitOldMovieLib (filename, &cf);
	}
else {
	CFClose (&cf);
	return NULL;
	}
}

//-----------------------------------------------------------------------

//ask user to put the D2 CD in.
//returns -1 if ESC pressed, 0 if OK chosen
//CD may not have been inserted
int RequestCD (void)
{
#if 0
	ubyte save_pal [256*3];
	gsrCanvas *save_canv, *tcanv;
	int ret, was_faded=gameStates.render.bPaletteFadedOut;

	GrPaletteStepClear ();

	save_canv = grdCurCanv;
	tcanv = GrCreateCanvas (grdCurCanv->cv_w, grdCurCanv->cv_h);

	GrSetCurrentCanvas (tcanv);
	gr_ubitmap (0, 0, &save_canv->cvBitmap);
	GrSetCurrentCanvas (save_canv);

	GrClearCanvas (0);

 try_again:;

	ret = ExecMessageBox ("CD ERROR", 1, "Ok", "Please insert your Descent II CD");

	if (ret == -1) {
		int ret2;

		ret2 = ExecMessageBox ("CD ERROR", 2, "Try Again", "Leave Game", "You must insert your\nDescent II CD to Continue");

		if (ret2 == -1 || ret2 == 0)
			goto try_again;
	}
	force_rb_register = 1;  //disc has changed; force register new CD
	GrPaletteStepClear ();
	gr_ubitmap (0, 0, &tcanv->cvBitmap);
	if (!was_faded)
		GrPaletteStepLoad (NULL);
	GrFreeCanvas (tcanv);
	return ret;
#else
#if TRACE
	con_printf (DEBUG_LEVEL, "STUB: movie: RequestCD\n");
#endif
	return 0;
#endif
}

//-----------------------------------------------------------------------

void init_movie (char *filename, int libnum, int isRobots, int required)
{
	int bHighRes, nTries;
	char *res = strchr (filename, '.') - 1; // 'h' == high resolution, 'l' == low

#if 0//ndef RELEASE
	if (FindArg ("-nomovies")) {
		movies.libs [libnum] = NULL;
		return;
	}
#endif

	//for robots, load highres versions if highres menus set
	bHighRes = isRobots ? gameStates.menus.bHiresAvailable : gameOpts->movies.bHires;
	if (bHighRes)
		*res = 'h';
	for (nTries = 0; (movies.libs [libnum] = InitMovieLib (filename)) == NULL; nTries++) {
		char name2 [100];

		strcpy (name2, CDROM_dir);
		strcat (name2, filename);
		movies.libs [libnum] = InitMovieLib (name2);

		if (movies.libs [libnum]) {
			movies.libs [libnum]->flags |= MLF_ON_CD;
			break; // we found our movie on the CD
			}
		else {
			if (nTries == 0) { // first nTries
				if (*res == 'h') { // nTries low res instead
					*res = 'l';
					bHighRes = 0;
					}
				else if (*res == 'l') { // nTries high
					*res = 'h';
					bHighRes = 1;
					}
				else {
#ifdef _DEBUG
					if (required)
						Warning (TXT_MOVIE_FILE, filename);
#endif
					break;
					}
				}
			else { // nTries == 1
				if (required) {
					*res = '*';
#ifdef _DEBUG
					//Warning (TXT_MOVIE_ANY, filename);
#endif
				}
				break;
			}
		}
	}

	if (isRobots && movies.libs [libnum] != NULL)
		gameStates.movies.nRobots = bHighRes ? 2 : 1;
}

//-----------------------------------------------------------------------

void close_movie (int i)
{
if (movies.libs [i]) {
	D2_FREE (movies.libs [i]->movies);
	D2_FREE (movies.libs [i]);
	}
}

//-----------------------------------------------------------------------

void _CDECL_ CloseMovies (void)
{
	int i;

LogErr ("unloading movies\n");
for (i=0;i<N_MOVIE_LIBS;i++)
	close_movie (i);
}

//-----------------------------------------------------------------------

static int bMoviesInited = 0;
//find and initialize the movie libraries
void InitMovies ()
{
	int i, j;
	int isRobots;

	j = (gameStates.app.bHaveExtraMovies = !gameStates.app.bNostalgia) ? 
		 N_BUILTIN_MOVIE_LIBS : FIRST_EXTRA_MOVIE_LIB;
	for (i = 0; i < N_BUILTIN_MOVIE_LIBS; i++) {
		isRobots = !strnicmp (pszMovieLibs[i], "robot", 5);
		init_movie (pszMovieLibs[i], i, isRobots, 1);
		if (movies.libs [i])
			LogErr ("   found movie lib '%s'\n", pszMovieLibs[i]);
		else if ((i >= FIRST_EXTRA_MOVIE_LIB) && 
			 (i < FIRST_EXTRA_MOVIE_LIB + N_EXTRA_MOVIE_LIBS))
			gameStates.app.bHaveExtraMovies = 0;
		}

	movies.libs [EXTRA_ROBOT_LIB] = NULL;
	bMoviesInited = 1;
	atexit (CloseMovies);
}

//-----------------------------------------------------------------------

void InitExtraRobotMovie (char *filename)
{
	close_movie (EXTRA_ROBOT_LIB);
	init_movie (filename, EXTRA_ROBOT_LIB, 1, 0);
}

//-----------------------------------------------------------------------

CFILE cfMovies = {NULL, 0, 0, 0};
int nMovieStart = -1;

//looks through a movie library for a movie file
//returns filehandle, with fileposition at movie, or -1 if can't find
int SearchMovieLib (CFILE *cfp, tMovieLib *lib, char *filename, int bRequired)
{
	int i, bFromCD;

if (!lib)
	return 0;
for (i = 0; i < lib->n_movies; i++)
	if (!stricmp (filename, lib->movies [i].name)) {	//found the movie in a library 
		if ((bFromCD = (lib->flags & MLF_ON_CD)))
			SongsStopRedbook ();		//ready to read from CD
		do {		//keep trying until we get the file handle
			CFOpen (cfp, lib->name, gameFolders.szMovieDir, "rb", 0);
			if (bRequired && bFromCD && !cfp->file) {   //didn't get file!
				if (RequestCD () == -1)		//ESC from requester
					break;						//bail from here. will get error later
				}
			} while (bRequired && bFromCD && !cfp->file);
		if (cfp->file)
			CFSeek (cfp, nMovieStart = lib->movies [i].offset, SEEK_SET);
		return 1;
	}
return 0;
}

//-----------------------------------------------------------------------
//returns file handle
int OpenMovieFile (CFILE *cfp, char *filename, int bRequired)
{
	int i;

for (i = 0; i < N_MOVIE_LIBS; i++)
	if (SearchMovieLib (cfp, movies.libs [i], filename, bRequired))
		return 1;
return 0;    //couldn't find it
}

//-----------------------------------------------------------------------
//sets the file position to the start of this already-open file
int ResetMovieFile (CFILE *cfp)
{
CFSeek (cfp, nMovieStart, SEEK_SET);
return 0;       //everything is cool
}

//-----------------------------------------------------------------------

int GetNumMovieLibs (void)
{
	int	i;

for (i = 0; i < N_MOVIE_LIBS; i++)
	if (!movies.libs [i])
		return i;
return N_MOVIE_LIBS;
}

//-----------------------------------------------------------------------

int GetNumMovies (int nLib)
{
return ((nLib < N_MOVIE_LIBS) && movies.libs [nLib]) ? movies.libs [nLib]->n_movies : 0;
}

//-----------------------------------------------------------------------

char *GetMovieName (int nLib, int nMovie)
{
return (nLib < N_MOVIE_LIBS) && (nMovie < movies.libs [nLib]->n_movies) ? movies.libs [nLib]->movies [nMovie].name : NULL;
}

//-----------------------------------------------------------------------

char *CycleThroughMovies (int bRestart, int bPlayMovie)
{
	static int nMovieLibs = -1;
	static int iMovieLib = -1;
	static int nMovies = -1;
	static int iMovie = -1;
	char *pszMovieName;

if (bRestart)
	iMovieLib =
	nMovies = 
	iMovie = -1;

if (nMovieLibs < 0) {
	if (!bMoviesInited)
		InitMovies ();
	nMovieLibs = GetNumMovieLibs ();
	}
if (nMovieLibs) {
	if (++iMovie >= nMovies) {
		iMovieLib = (iMovieLib + 1) % nMovieLibs;
		if (iMovieLib == 2)	//skip the robot movies
			iMovieLib = (iMovieLib + 1) % nMovieLibs;
		nMovies = GetNumMovies (iMovieLib);
		iMovie = 0;
		}
	//InitSubTitles ("intro.tex");
	if ((pszMovieName = GetMovieName (iMovieLib, iMovie))) {
		gameStates.video.nScreenMode = -1;
		if (bPlayMovie)
			PlayMovie (pszMovieName, 1, 1, gameOpts->movies.bResize);
		}
	return pszMovieName;
	//CloseSubTitles ();
	}
return NULL;
}

//-----------------------------------------------------------------------

tMovieLib *FindMovieLib (char *pszTargetMovie)
{

	static int nMovieLibs = -1;

	int iMovieLib, iMovie, nMovies;
	char *pszMovieName;

if (nMovieLibs < 0) {
	if (!bMoviesInited)
		InitMovies ();
	nMovieLibs = GetNumMovieLibs ();
	}
if (nMovieLibs) {
	for (iMovieLib = 0; iMovieLib < nMovieLibs; iMovieLib++) {
		nMovies = GetNumMovies (iMovieLib);
		for (iMovie = 0; iMovie < nMovies; iMovie++) {
			if (!(pszMovieName = GetMovieName (iMovieLib, iMovie)))
				continue;
			if (!strcmp (pszMovieName, pszTargetMovie))
				return movies.libs [iMovieLib];
			}
		}
	}
return NULL;
}

//-----------------------------------------------------------------------

void PlayIntroMovie (void)
{
	static	int bHaveIntroMovie = 1;

if (bHaveIntroMovie) {
	InitSubTitles ("intro.tex");
	if (PlayMovie ("intro.mve", MOVIE_REQUIRED, 0, gameOpts->movies.bResize) == MOVIE_NOT_PLAYED)
		bHaveIntroMovie = 0;
	CloseSubTitles ();
	}
}

//-----------------------------------------------------------------------

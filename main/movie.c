/* $Id: movie.c,v 1.33 2003/11/26 12:26:30 btb Exp $ */
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
static char rcsid[] = "$Id: movie.c,v 1.33 2003/11/26 12:26:30 btb Exp $";
#endif

#define DEBUG_LEVEL CON_NORMAL

#include <string.h>
#ifndef _WIN32_WCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#ifndef _MSC_VER
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

#include "movie.h"
#include "console.h"
#include "args.h"
#include "key.h"
#include "digi.h"
#include "songs.h"
#include "inferno.h"
#include "palette.h"
#include "strutil.h"
#include "error.h"
#include "u_mem.h"
#include "byteswap.h"
#include "gr.h"
#include "ogl_init.h"
#include "gamefont.h"
#include "cfile.h"
#include "menu.h"
#include "libmve.h"
#include "text.h"
#include "screens.h"

extern char CDROM_dir[];

#define VID_PLAY 0
#define VID_PAUSE 1

int Vid_State;
// Subtitle data
typedef struct {
	short first_frame,last_frame;
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
	char name[FILENAME_LEN];
	int offset,len;
} ml_entry;

#define MLF_ON_CD    1
#define MAX_MOVIES_PER_LIB    50    //determines size of d_malloc

typedef struct {
	char     name[100]; //[FILENAME_LEN];
	int      n_movies;
	ubyte    flags,pad[3];
	ml_entry *movies;
} movielib;

#ifdef D2_OEM

char movielib_files[][FILENAME_LEN] = {
	"intro-l.mvl", 
	"other-l.mvl", 
	"robots-l.mvl", 
	"oem-l.mvl",
	"extra1-l.mvl",
	"extra2-l.mvl",
	"extra3-l.mvl",
	"extra4-l.mvl",
	"extra5-l.mvl"};

#define	FIRST_EXTRA_MOVIE_LIB	4

#else

char movielib_files[][FILENAME_LEN] = {
	"intro-l.mvl",
	"other-l.mvl",
	"robots-l.mvl",
	"extra1-l.mvl",
	"extra2-l.mvl",
	"extra3-l.mvl",
	"extra4-l.mvl",
	"extra5-l.mvl"};

#define	FIRST_EXTRA_MOVIE_LIB	3

#endif

#define N_EXTRA_MOVIE_LIBS	5
#define N_BUILTIN_MOVIE_LIBS (sizeof(movielib_files)/sizeof(*movielib_files))
#define N_MOVIE_LIBS (N_BUILTIN_MOVIE_LIBS+1)
#define EXTRA_ROBOT_LIB N_BUILTIN_MOVIE_LIBS
movielib *movie_libs[N_MOVIE_LIBS];


ubyte *moviePalette;

CFILE *RoboFile = NULL;
int RoboFilePos = 0;

// Function Prototypes
int RunMovie(char *filename, int highresFlag, int allow_abort,int dx,int dy);

CFILE *OpenMovieFile(char *filename, int must_have);
int ResetMovieFile(CFILE *handle);

void change_filename_ext( char *dest, char *src, char *ext );
void DecodeTextLine(char *p);
void DrawSubTitles(int nFrame);

// ----------------------------------------------------------------------

void* MPlayAlloc(unsigned size)
{
    return d_malloc(size);
}

void MPlayFree(void *p)
{
    d_free(p);
}


//-----------------------------------------------------------------------

unsigned int FileRead(void *handle, void *buf, unsigned int count)
{
    unsigned int numread = (unsigned int) CFRead(buf, 1, count, (CFILE *)handle);
    return (numread == count);
}


//-----------------------------------------------------------------------

//filename will actually get modified to be either low-res or high-res
//returns status.  see values in movie.h

int PlayMovie(const char *filename, int must_have, int bForce, int bFullScreen)
{
	char name[FILENAME_LEN],*p;
	int c, ret;

#if 1//ndef _DEBUG
	if (!bForce && (gameOpts->movies.nLevel < 2))
		return MOVIE_NOT_PLAYED;
#endif
	strcpy(name,filename);
	if ((p=strchr(name,'.')) == NULL)		//add extension, if missing
		strcat(name,".mve");
	//check for escape already pressed & abort if so
	while ((c=KeyInKey()) != 0)
		if (c == KEY_ESC)
			return MOVIE_ABORTED;
	// Stop all digital sounds currently playing.
	DigiStopAll();
	// Stop all songs
	SongsStopAll();
	DigiClose();
	// Start sound
	if (gameStates.app.bUseSound)
		MVE_sndInit(1);
	else
		MVE_sndInit(-1);
	gameOpts->movies.bFullScreen = bFullScreen;
	ret = RunMovie (name,gameOpts->movies.bHires,must_have,-1,-1);
	DigiInit();
	gameStates.video.nScreenMode = -1;		//force screen reset
	return ret;
}

//-----------------------------------------------------------------------

void MovieShowFrame (ubyte *buf, uint bufw, uint bufh, uint sx, uint sy,
							uint w, uint h, uint dstx, uint dsty)
{
	grsBitmap bmFrame;

Assert(bufw == w && bufh == h);

memset (&bmFrame, 0, sizeof (bmFrame));
bmFrame.bm_props.w = bmFrame.bm_props.rowsize = bufw;
bmFrame.bm_props.h = bufh;
bmFrame.bm_props.nType = BM_LINEAR;
bmFrame.bm_texBuf = buf;
bmFrame.bm_palette = moviePalette;

TRANSPARENCY_COLOR = -1;
#ifdef OGL
if (gameOpts->menus.nStyle) {
	//memset (grPalette, 0, 768);
	//GrPaletteStepLoad (NULL);
	}
if (gameOpts->movies.bFullScreen) {
		double r = (double) bufh / (double) bufw;
		int dh = (int) (grdCurCanv->cv_bitmap.bm_props.w * r);
		int yOffs = (grdCurCanv->cv_bitmap.bm_props.h - dh) / 2;

	glDisable (GL_BLEND);
	OglUBitBltI(grdCurCanv->cv_bitmap.bm_props.w, dh, 0, yOffs,
					  bufw, bufh, sx, sy,
					  &bmFrame,&grdCurCanv->cv_bitmap,
					  gameOpts->movies.nQuality);
	glEnable (GL_BLEND);
	}
else
#endif
	{
		int xOffs = (grdCurCanv->cv_bitmap.bm_props.w - 640) / 2;
		int yOffs = (grdCurCanv->cv_bitmap.bm_props.h - 480) / 2;

	if (xOffs < 0)
		xOffs = 0;
	if (yOffs < 0)
		yOffs = 0;

	dstx += xOffs;
	dsty += yOffs;
	WIN(DDGRLOCK(dd_grd_curcanv));
	if ((grdCurCanv->cv_bitmap.bm_props.w > 640) || (grdCurCanv->cv_bitmap.bm_props.h > 480)) {
		GrSetColorRGBi (RGBA_PAL (0, 0, 32));
		GrUBox(dstx-1,dsty,dstx+w,dsty+h+1);
		}
	WIN(DDGRUNLOCK(dd_grd_curcanv));
	GrBmUBitBlt(bufw,bufh,dstx,dsty,sx,sy,&bmFrame,&grdCurCanv->cv_bitmap);
	}
TRANSPARENCY_COLOR = DEFAULT_TRANSPARENCY_COLOR;
}

//-----------------------------------------------------------------------
//our routine to set the palette, called from the movie code
void MovieSetPalette(unsigned char *p, unsigned start, unsigned count)
{
	tPalette	palette;

if (count == 0)
	return;

//GrPaletteStepLoad (NULL);
//Color 0 should be black, and we get color 255
//movie libs palette into our array
Assert(start>=0 && start+count<=256);
memcpy (palette + start * 3, p + start * 3, count * 3);
//Set color 0 to be black
palette [0] = palette [1] = palette [2] = 0;
//Set color 255 to be our subtitle color
palette [765] = palette [766] = palette [767] = 50;
//finally set the palette in the hardware
moviePalette = AddPalette (palette);
GrPaletteStepLoad(NULL);
}

//-----------------------------------------------------------------------

#define BOX_BORDER (gameStates.menus.bHires?40:20)

void ShowPauseMessage(char *msg)
{
	int w,h,aw;
	int x,y;

	GrSetCurrentCanvas(NULL);
	GrSetCurFont( SMALL_FONT );

	GrGetStringSize(msg,&w,&h,&aw);

	x = (grdCurScreen->sc_w-w)/2;
	y = (grdCurScreen->sc_h-h)/2;

#if 0
	if (movie_bg.bmp) {
		GrFreeBitmap(movie_bg.bmp);
		movie_bg.bmp = NULL;
	}

	// Save the background of the display
	movie_bg.x=x; 
	movie_bg.y=y; 
	movie_bg.w=w; 
	movie_bg.h=h;
	movie_bg.bmp = GrCreateBitmap( w+BOX_BORDER, h+BOX_BORDER );
	GrBmUBitBlt(w+BOX_BORDER, h+BOX_BORDER, 0, 0, x-BOX_BORDER/2, y-BOX_BORDER/2, &(grdCurCanv->cv_bitmap), movie_bg.bmp );
#endif
	GrSetColorRGB (0, 0, 0, 255);
	GrRect(x-BOX_BORDER/2,y-BOX_BORDER/2,x+w+BOX_BORDER/2-1,y+h+BOX_BORDER/2-1);
	GrSetFontColor( 255, -1 );
	GrUString( 0x8000, y, msg );
	GrUpdate (0);
}

//-----------------------------------------------------------------------

void ClearPauseMessage()
{
#if 0
	if (movie_bg.bmp) {

		GrBitmap(movie_bg.x-BOX_BORDER/2, movie_bg.y-BOX_BORDER/2, movie_bg.bmp);

		GrFreeBitmap(movie_bg.bmp);
		movie_bg.bmp = NULL;
	}
#endif
}

//-----------------------------------------------------------------------
//returns status.  see movie.h
int RunMovie(char *filename, int hiresFlag, int must_have,int dx,int dy)
{
	CFILE *filehndl;
	int result=1,aborted=0;
	int track = 0;
	int nFrame;
	int key;

	result=1;

	// Open Movie file.  If it doesn't exist, no movie, just return.
	filehndl = OpenMovieFile(filename,must_have);
	if (filehndl == NULL)
	{
		if (must_have) {
#if TRACE
			con_printf(CON_NORMAL, "movie: RunMovie: Cannot open movie <%s>\n",filename);
#endif
			}
		return MOVIE_NOT_PLAYED;
	}
	MVE_memCallbacks(MPlayAlloc, MPlayFree);
	MVE_ioCallbacks(FileRead);
#if 0//ndef _WIN32
	if (hiresFlag) {
		GrSetMode(SM(640,480));
	} else {
		GrSetMode(SM(320,200));
	}
#endif
#ifdef OGL
	SetScreenMode(SCREEN_MENU);
	GrPaletteStepLoad(NULL);
#endif

#if !defined(POLY_ACC)
	MVE_sfCallbacks(MovieShowFrame);
	MVE_palCallbacks(MovieSetPalette);
#endif

	if (MVE_rmPrepMovie((void *)filehndl, dx, dy, track)) {
		Int3();
		return MOVIE_NOT_PLAYED;
	}
	nFrame = 0;
	gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && hiresFlag;
	while((result = MVE_rmStepMovie()) == 0) {
		DrawSubTitles(nFrame);
		GrPaletteStepLoad(NULL); // moved this here because of flashing
		GrUpdate (1);
		key = KeyInKey();
		// If ESCAPE pressed, then quit movie.
		if (key == KEY_ESC) {
			result = aborted = 1;
			break;
		}

		// If PAUSE pressed, then pause movie
		if (key == KEY_PAUSE) {
			MVE_rmHoldMovie();
			ShowPauseMessage(TXT_PAUSE);
			while (!KeyInKey()) ;
			ClearPauseMessage();
		}

#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE
		if ((key == KEY_ALTED+KEY_ENTER) ||
		    (key == KEY_ALTED+KEY_PADENTER))
			GrToggleFullScreen();
#endif

		nFrame++;
	}
	Assert(aborted || result == MVE_ERR_EOF);	 ///movie should be over
    MVE_rmEndMovie();
	CFClose(filehndl);                           // Close Movie File

	// Restore old graphic state
	gameStates.video.nScreenMode=-1;  //force reset of screen mode
#ifdef OGL
	GrPaletteStepLoad(NULL);
#endif
	return (aborted?MOVIE_ABORTED:MOVIE_PLAYED_FULL);
}

//-----------------------------------------------------------------------

int InitMovieBriefing()
{
#if 0
	if (gameStates.menus.bHires)
		GrSetMode(SM(640,480);
	else
		GrSetMode(SM(320,200);

	GrInitSubCanvas( &VR_screen_pages[0], &grdCurScreen->sc_canvas, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h );
	GrInitSubCanvas( &VR_screen_pages[1], &grdCurScreen->sc_canvas, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h );
#endif

	return 1;
}

//-----------------------------------------------------------------------
//returns 1 if frame updated ok
int RotateRobot()
{
	int err;

	gameOpts->movies.bFullScreen = 1;
	if (curDrawBuffer == GL_BACK)
		GrPaletteStepLoad (NULL);
	err = MVE_rmStepMovie();
	GrPaletteStepLoad (NULL);
	if (err == MVE_ERR_EOF) {   //end of movie, so reset
		ResetMovieFile(RoboFile);
		if (MVE_rmPrepMovie((void *)RoboFile, 
								  gameStates.menus.bHires ? 280 : 140, 
								  gameStates.menus.bHires ? 200 : 80, 0)) {
			Int3();
			return 0;
		}
	}
	else if (err) {
		Int3();
		return 0;
	}
	return 1;
}

//-----------------------------------------------------------------------

void DeInitRobotMovie(void)
{
	MVE_rmEndMovie();
	CFClose(RoboFile);                           // Close Movie File
}

//-----------------------------------------------------------------------

int InitRobotMovie(char *filename)
{
if (gameOpts->movies.nLevel < 1)
	return 0;

#if TRACE
	con_printf(DEBUG_LEVEL, "RoboFile=%s\n", filename);
#endif
	MVE_sndInit(-1);        //tell movies to play no sound for robots
	RoboFile = OpenMovieFile(filename, 1);
	if (RoboFile == NULL) {
#ifdef _DEBUG
		Warning(TXT_MOVIE_ROBOT,filename);
#endif
		return MOVIE_NOT_PLAYED;
	}
	Vid_State = VID_PLAY;
	gameOpts->movies.bFullScreen = 1;
	MVE_memCallbacks(MPlayAlloc, MPlayFree);
	MVE_ioCallbacks(FileRead);
	MVE_sfCallbacks(MovieShowFrame);
	MVE_palCallbacks(MovieSetPalette);
	if (MVE_rmPrepMovie((void *)RoboFile, gameStates.menus.bHires?280:140, gameStates.menus.bHires?200:80, 0)) {
		Int3();
		return 0;
	}
	RoboFilePos = CFSeek(RoboFile, 0L, SEEK_CUR);
#if TRACE
	con_printf(DEBUG_LEVEL, "RoboFilePos=%d!\n", RoboFilePos);
#endif
	return 1;
}

//-----------------------------------------------------------------------
/*
 *		Subtitle system code
 */

//search for next field following whitespace 
ubyte *next_field(ubyte *p)
{
while (*p && !isspace(*p))
	p++;
if (!*p)
	return NULL;
while (*p && isspace(*p))
	p++;
if (!*p)
	return NULL;
return p;
}

//-----------------------------------------------------------------------

int InitSubTitles(char *filename)
{
	CFILE *ifile;
	int size,read_count;
	ubyte *p;
	int have_binary = 0;

	subTitles.nCaptions = 0;

	if (!gameOpts->movies.bSubTitles)
		return 0;

	ifile = CFOpen(filename,gameFolders.szDataDir,"rb",0);		//try text version

	if (!ifile) {								//no text version, try binary version
		char filename2[FILENAME_LEN];
		change_filename_ext(filename2, filename,".txb");
		ifile = CFOpen(filename2, gameFolders.szDataDir,"rb",0);
		if (!ifile)
			return 0;
		have_binary = 1;
	}

	size = CFLength(ifile, 0);
	MALLOC (subTitles.rawDataP, ubyte, size+1);
	read_count = (int) CFRead(subTitles.rawDataP, 1, size, ifile);
	CFClose(ifile);
	subTitles.rawDataP[size] = 0;
	if (read_count != size) {
		d_free(subTitles.rawDataP);
		return 0;
	}
	p = subTitles.rawDataP;
	while (p && p < subTitles.rawDataP+size) {
		char *endp;

		endp = strchr(p,'\n'); 
		if (endp) {
			if (endp[-1] == '\r')
				endp[-1] = 0;		//handle 0d0a pair
			*endp = 0;			//string termintor
		}

		if (have_binary)
			DecodeTextLine(p);

		if (*p != ';') {
			subTitles.captions[subTitles.nCaptions].first_frame = atoi(p);
			p = next_field(p); if (!p) continue;
			subTitles.captions[subTitles.nCaptions].last_frame = atoi(p);
			p = next_field(p); if (!p) continue;
			subTitles.captions[subTitles.nCaptions].msg = p;

			Assert(subTitles.nCaptions==0 || subTitles.captions[subTitles.nCaptions].first_frame >= subTitles.captions[subTitles.nCaptions-1].first_frame);
			Assert(subTitles.captions[subTitles.nCaptions].last_frame >= subTitles.captions[subTitles.nCaptions].first_frame);

			subTitles.nCaptions++;
		}

		p = endp+1;

	}

	return 1;
}

//-----------------------------------------------------------------------

void CloseSubTitles()
{
	if (subTitles.rawDataP)
		d_free(subTitles.rawDataP);
	subTitles.rawDataP = NULL;
	subTitles.nCaptions = 0;
}

//-----------------------------------------------------------------------
//draw the subtitles for this frame
void DrawSubTitles(int nFrame)
{
	static int active_subtitles[MAX_ACTIVE_SUBTITLES];
	static int num_active_subtitles,next_subtitle,line_spacing;
	int t,y;
	int must_erase=0;

	if (nFrame == 0) {
		num_active_subtitles = 0;
		next_subtitle = 0;
		GrSetCurFont( GAME_FONT );
		line_spacing = grdCurCanv->cv_font->ft_h + (grdCurCanv->cv_font->ft_h >> 2);
		GrSetFontColor(255,-1);
	}

	//get rid of any subtitles that have expired
	for (t=0;t<num_active_subtitles;)
		if (nFrame > subTitles.captions[active_subtitles[t]].last_frame) {
			int t2;
			for (t2=t;t2<num_active_subtitles-1;t2++)
				active_subtitles[t2] = active_subtitles[t2+1];
			num_active_subtitles--;
			must_erase = 1;
		}
		else
			t++;

	//get any subtitles new for this frame 
	while (next_subtitle < subTitles.nCaptions && nFrame >= subTitles.captions[next_subtitle].first_frame) {
		if (num_active_subtitles >= MAX_ACTIVE_SUBTITLES)
			Error("Too many active subtitles!");
		active_subtitles[num_active_subtitles++] = next_subtitle;
		next_subtitle++;
	}

	//find y coordinate for first line of subtitles
	y = grdCurCanv->cv_bitmap.bm_props.h-((line_spacing+1)*MAX_ACTIVE_SUBTITLES+2);

	//erase old subtitles if necessary
	if (must_erase) {
		GrSetColorRGB (0, 0, 0, 255);
		GrRect(0,y,grdCurCanv->cv_bitmap.bm_props.w-1,grdCurCanv->cv_bitmap.bm_props.h-1);
	}

	//now draw the current subtitles
	for (t=0;t<num_active_subtitles;t++)
		if (active_subtitles[t] != -1) {
			GrString(0x8000,y,subTitles.captions[active_subtitles[t]].msg);
			y += line_spacing+1;
		}
}

//-----------------------------------------------------------------------

movielib *InitNewMovieLib(char *filename, CFILE *fp)
{
	int		nfiles,offset;
	int		i, n, len;
	movielib *table;

	//read movie file header

nfiles = CFReadInt(fp);        //get number of files
//table = d_malloc(sizeof(*table) + sizeof(ml_entry)*nfiles);
MALLOC(table, movielib, 1);
MALLOC(table->movies, ml_entry, nfiles);
strcpy(table->name,filename);
table->n_movies = nfiles;
offset = 4+4+nfiles*(13+4);	//id + nfiles + nfiles * (filename + size)
for (i=0;i<nfiles;i++) {
	n = (int) CFRead(table->movies[i].name, 13, 1, fp);
	if ( n != 1 )
		break;		//end of file (probably)
	len = CFReadInt(fp);
	table->movies[i].len = len;
	table->movies[i].offset = offset;
	offset += table->movies[i].len;
	}
CFClose(fp);
table->flags = 0;
return table;
}

//-----------------------------------------------------------------------

movielib *InitOldMovieLib(char *filename, CFILE *fp)
{
	int nfiles,size;
	int i, len;
	movielib *table,*table2;

	nfiles = 0;

	//allocate big table
table = d_malloc(sizeof(*table) + sizeof(ml_entry)*MAX_MOVIES_PER_LIB);
while (1) {
	i = (int) CFRead(table->movies[nfiles].name, 13, 1, fp);
	if (i != 1)
		break;		//end of file (probably)
	i = (int) CFRead(&len, 4, 1, fp);
	if (i != 1) {
		Error("error reading movie library <%s>",filename);
		return NULL;
		}
	table->movies[nfiles].len = INTEL_INT(len);
	table->movies[nfiles].offset = CFTell(fp);
	CFSeek(fp, INTEL_INT(len), SEEK_CUR);       //skip data
	nfiles++;
	}
	//allocate correct-sized table
size = sizeof(*table) + sizeof(ml_entry)*nfiles;
table2 = d_malloc(size);
memcpy(table2,table,size);
d_free(table);
table = table2;
strcpy(table->name,filename);
table->n_movies = nfiles;
CFClose(fp);
table->flags = 0;
return table;
}

//-----------------------------------------------------------------------

//find the specified movie library, and read in list of movies in it
movielib *InitMovieLib (char *filename)
{
	//note: this based on CFInitHogFile()

	char id[4];
	CFILE *fp;

if (!(fp = CFOpen(filename, gameFolders.szMovieDir, "rb", 0)))
	return NULL;
CFRead(id, 4, 1, fp);
if ( !strncmp( id, "DMVL", 4 ) )
	return InitNewMovieLib(filename,fp);
else if ( !strncmp( id, "DHF", 3 ) ) {
	CFSeek(fp,-1,SEEK_CUR);		//old file had 3 char id
	return InitOldMovieLib(filename,fp);
	}
else {
	CFClose(fp);
	return NULL;
	}
}

//-----------------------------------------------------------------------

//ask user to put the D2 CD in.
//returns -1 if ESC pressed, 0 if OK chosen
//CD may not have been inserted
int request_cd(void)
{
#if 0
	ubyte save_pal[256*3];
	grs_canvas *save_canv,*tcanv;
	int ret,was_faded=gameStates.render.bPaletteFadedOut;

	GrPaletteStepClear();

	save_canv = grdCurCanv;
	tcanv = GrCreateCanvas(grdCurCanv->cv_w,grdCurCanv->cv_h);

	GrSetCurrentCanvas(tcanv);
	gr_ubitmap(0,0,&save_canv->cv_bitmap);
	GrSetCurrentCanvas(save_canv);

	GrClearCanvas(0);

 try_again:;

	ret = ExecMessageBox( "CD ERROR", 1, "Ok", "Please insert your Descent II CD");

	if (ret == -1) {
		int ret2;

		ret2 = ExecMessageBox( "CD ERROR", 2, "Try Again", "Leave Game", "You must insert your\nDescent II CD to Continue");

		if (ret2 == -1 || ret2 == 0)
			goto try_again;
	}
	force_rb_register = 1;  //disc has changed; force register new CD
	GrPaletteStepClear();
	gr_ubitmap(0,0,&tcanv->cv_bitmap);
	if (!was_faded)
		GrPaletteStepLoad (NULL);
	GrFreeCanvas(tcanv);
	return ret;
#else
#if TRACE
	con_printf(DEBUG_LEVEL, "STUB: movie: request_cd\n");
#endif
	return 0;
#endif
}

//-----------------------------------------------------------------------

void init_movie(char *filename, int libnum, int isRobots, int required)
{
	int high_res, try;
	char *res = strchr(filename, '.') - 1; // 'h' == high resolution, 'l' == low

#if 0//ndef RELEASE
	if (FindArg("-nomovies")) {
		movie_libs[libnum] = NULL;
		return;
	}
#endif

	//for robots, load highres versions if highres menus set
	high_res = isRobots ? gameStates.menus.bHiresAvailable : gameOpts->movies.bHires;
	if (high_res)
		*res = 'h';
	for (try = 0; (movie_libs[libnum] = InitMovieLib(filename)) == NULL; try++) {
		char name2[100];

		strcpy(name2,CDROM_dir);
		strcat(name2,filename);
		movie_libs[libnum] = InitMovieLib(name2);

		if (movie_libs[libnum] != NULL) {
			movie_libs[libnum]->flags |= MLF_ON_CD;
			break; // we found our movie on the CD
		} else {
			if (try == 0) { // first try
				if (*res == 'h') { // try low res instead
					*res = 'l';
					high_res = 0;
				} else if (*res == 'l') { // try high
					*res = 'h';
					high_res = 1;
				} else {
#ifdef _DEBUG
					if (required)
						Warning(TXT_MOVIE_FILE,filename);
#endif
					break;
				}
			} else { // try == 1
				if (required) {
					*res = '*';
#ifdef _DEBUG
					//Warning(TXT_MOVIE_ANY, filename);
#endif
				}
				break;
			}
		}
	}

	if (isRobots && movie_libs[libnum]!=NULL)
		gameStates.movies.nRobots = high_res?2:1;
}

//-----------------------------------------------------------------------

void close_movie(int i)
{
	if (movie_libs[i]) {
		d_free(movie_libs[i]->movies);
		d_free(movie_libs[i]);
	}
}

//-----------------------------------------------------------------------

void _CDECL_ CloseMovies(void)
{
	int i;

LogErr ("unloading movies\n");
for (i=0;i<N_MOVIE_LIBS;i++)
	close_movie(i);
}

//-----------------------------------------------------------------------

static int bMoviesInited = 0;
//find and initialize the movie libraries
void InitMovies()
{
	int i, j;
	int isRobots;

	j = (gameStates.app.bHaveExtraMovies = !gameStates.app.bNostalgia) ? 
		 N_BUILTIN_MOVIE_LIBS : FIRST_EXTRA_MOVIE_LIB;
	for (i = 0; i < N_BUILTIN_MOVIE_LIBS; i++) {
		isRobots = !strnicmp (movielib_files [i], "robot", 5);
		init_movie (movielib_files [i], i, isRobots, 1);
		if (movie_libs [i])
			LogErr ("   found movie lib '%s'\n", movielib_files [i]);
		else if ((i >= FIRST_EXTRA_MOVIE_LIB) && 
			 (i < FIRST_EXTRA_MOVIE_LIB + N_EXTRA_MOVIE_LIBS))
			gameStates.app.bHaveExtraMovies = 0;
		}

	movie_libs[EXTRA_ROBOT_LIB] = NULL;
	bMoviesInited = 1;
	atexit(CloseMovies);
}

//-----------------------------------------------------------------------

void InitExtraRobotMovie(char *filename)
{
	close_movie(EXTRA_ROBOT_LIB);
	init_movie(filename,EXTRA_ROBOT_LIB,1,0);
}

//-----------------------------------------------------------------------

CFILE *movie_handle;
int movie_start;

//looks through a movie library for a movie file
//returns filehandle, with fileposition at movie, or -1 if can't find
CFILE *SearchMovieLib(movielib *lib, char *filename, int must_have)
{
	int i;
	CFILE *filehandle;

	if (lib == NULL)
		return NULL;
	for (i=0;i<lib->n_movies;i++)
		if (!stricmp(filename,lib->movies[i].name)) {	//found the movie in a library 
			int from_cd;
			from_cd = (lib->flags & MLF_ON_CD);
			if (from_cd)
				songs_stop_redbook();		//ready to read from CD
			do {		//keep trying until we get the file handle
				movie_handle = filehandle = CFOpen(lib->name, gameFolders.szMovieDir, "rb", 0);
				if (must_have && from_cd && filehandle == NULL)
				{   //didn't get file!
					if (request_cd() == -1)		//ESC from requester
						break;						//bail from here. will get error later
				}

			} while (must_have && from_cd && filehandle == NULL);

			if (filehandle)
				CFSeek(filehandle, (movie_start = lib->movies[i].offset), SEEK_SET);
			return filehandle;
		}
	return NULL;
}

//-----------------------------------------------------------------------
//returns file handle
CFILE *OpenMovieFile(char *filename, int must_have)
{
	CFILE *filehandle;
	int i;

	for (i=0;i<N_MOVIE_LIBS;i++) {
		if ((filehandle = SearchMovieLib(movie_libs[i], filename, must_have)) != NULL)
			return filehandle;
	}

	return NULL;    //couldn't find it
}

//-----------------------------------------------------------------------
//sets the file position to the start of this already-open file
int ResetMovieFile(CFILE *handle)
{
	Assert(handle == movie_handle);

	CFSeek(handle, movie_start, SEEK_SET);

	return 0;       //everything is cool
}

int GetNumMovieLibs (void)
{
	int	i;

for (i = 0; i < N_MOVIE_LIBS; i++)
	if (!movie_libs [i])
		return i;
return N_MOVIE_LIBS;
}

//-----------------------------------------------------------------------

int GetNumMovies (int nLib)
{
return ((nLib < N_MOVIE_LIBS) && movie_libs [nLib]) ? movie_libs [nLib]->n_movies : 0;
}

//-----------------------------------------------------------------------

char *GetMovieName (int nLib, int nMovie)
{
return (nLib < N_MOVIE_LIBS) && (nMovie < movie_libs [nLib]->n_movies) ? movie_libs [nLib]->movies [nMovie].name : NULL;
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
	//InitSubTitles("intro.tex");
	if (pszMovieName = GetMovieName (iMovieLib, iMovie)) {
#ifdef OGL
		gameStates.video.nScreenMode = -1;
#endif
		if (bPlayMovie)
			PlayMovie(pszMovieName, 1, 1, gameOpts->movies.bResize);
		}
	return pszMovieName;
	//CloseSubTitles();
	}
return NULL;
}

//-----------------------------------------------------------------------

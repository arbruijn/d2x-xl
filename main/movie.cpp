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

#define DEBUG_LEVEL CON_NORMAL

#include <string.h>
#ifndef _WIN32_WCE
#	include <sys/types.h>
#endif
#ifndef _WIN32
#	include <unistd.h>
#endif
#include <ctype.h>

#include "inferno.h"
#include "movie.h"
#include "key.h"
#include "strutil.h"
#include "error.h"
#include "u_mem.h"
#include "byteswap.h"
#include "ogl_bitmap.h"
#include "gamefont.h"
#include "libmve.h"
#include "text.h"
#include "screens.h"
#include "songs.h"

//-----------------------------------------------------------------------

extern char CDROM_dir [];

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

//-----------------------------------------------------------------------

#define MLF_ON_CD						1
#define MAX_MOVIES_PER_LIB			50    //determines size of alloc

#define	FIRST_EXTRA_MOVIE_LIB	4
#define N_EXTRA_MOVIE_LIBS			5
#define N_BUILTIN_MOVIE_LIBS		(sizeof (pszMovieLibs) / sizeof (*pszMovieLibs))
#define N_MOVIE_LIBS					(N_BUILTIN_MOVIE_LIBS+1)
#define EXTRA_ROBOT_LIB				N_BUILTIN_MOVIE_LIBS

#define MAX_ACTIVE_SUBTITLES		3

CSubTitles subTitles;


// Movielib data
CMovieManager movieManager;


void DecodeTextLine (char* p);

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
/*
 *		Subtitle system code
 */

//search for next field following whitespace 
ubyte* CSubTitles::NextField (ubyte* p)
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

int CSubTitles::Init (const char* filename)
{
	CFile cf;
	int 	size, readCount;
	ubyte	*p;
	int 	bHaveBinary = 0;

m_nCaptions = 0;
if (!gameOpts->movies.bSubTitles)
	return 0;
if (!cf.Open (filename, gameFolders.szDataDir, "rb", 0)) { // first try text version
	char filename2 [FILENAME_LEN];	//no text version, try binary version
	CFile::ChangeFilenameExtension (filename2, filename, ".txb");
	if (!cf.Open (filename2, gameFolders.szDataDir, "rb", 0))
		return 0;
	bHaveBinary = 1;
	}

size = cf.Length ();
m_rawDataP = new ubyte [size+1];
readCount = (int) cf.Read (m_rawDataP, 1, size);
cf.Close ();
m_rawDataP [size] = 0;
if (readCount != size) {
	delete[] m_rawDataP;
	return 0;
	}
p = m_rawDataP;
while (p && (p < m_rawDataP + size)) {
	char* endp = strchr (reinterpret_cast<char*> (p), '\n'); 

	if (endp) {
		if (endp [-1] == '\r')
			endp [-1] = 0;		//handle 0d0a pair
		*endp = 0;			//string termintor
		}
	if (bHaveBinary)
		DecodeTextLine (reinterpret_cast<char*> (p));
	if (*p != ';') {
		m_captions [m_nCaptions].first_frame = atoi (reinterpret_cast<char*> (p));
		if (!(p = NextField (p))) 
			continue;
		m_captions [m_nCaptions].last_frame = atoi (reinterpret_cast<char*> (p));
		if (!(p = NextField (p)))
			continue;
		m_captions [m_nCaptions].msg = reinterpret_cast<char*> (p);
		Assert (m_nCaptions==0 || m_captions [m_nCaptions].first_frame >= m_captions [m_nCaptions-1].first_frame);
		Assert (m_captions [m_nCaptions].last_frame >= m_captions [m_nCaptions].first_frame);
		m_nCaptions++;
		}
	p = reinterpret_cast<ubyte*> (endp + 1);
	}
return 1;
}

//-----------------------------------------------------------------------

void CSubTitles::Close (void)
{
if (m_rawDataP) {
	delete[] m_rawDataP;
	m_rawDataP = NULL;
	}
m_nCaptions = 0;
}

//-----------------------------------------------------------------------
//draw the subtitles for this frame
void CSubTitles::Draw (int nFrame)
{
	static int activeSubTitleList [MAX_ACTIVE_SUBTITLES];
	static int nActiveSubTitles, nNextSubTitle, nLineSpacing;
	int t, y;
	int bMustErase = 0;

if (nFrame == 0) {
	nActiveSubTitles = 0;
	nNextSubTitle = 0;
	fontManager.SetCurrent (GAME_FONT);
	nLineSpacing = CCanvas::Current ()->Font ()->Height () + (CCanvas::Current ()->Font ()->Height () / 4);
	fontManager.SetColorRGBi (WHITE_RGBA, 1, 0, 0);
	}

//get rid of any subtitles that have expired
for (t = 0; t <nActiveSubTitles; )
	if (nFrame > m_captions [activeSubTitleList [t]].last_frame) {
		int t2;
		for (t2 = t; t2 < nActiveSubTitles - 1; t2++)
			activeSubTitleList [t2] = activeSubTitleList [t2+1];
		nActiveSubTitles--;
		bMustErase = 1;
	}
	else
		t++;

//get any subtitles new for this frame 
while (nNextSubTitle < m_nCaptions && nFrame >= m_captions [nNextSubTitle].first_frame) {
	if (nActiveSubTitles >= MAX_ACTIVE_SUBTITLES)
		Error ("Too many active subtitles!");
	activeSubTitleList [nActiveSubTitles++] = nNextSubTitle;
	nNextSubTitle++;
	}

//find y coordinate for first line of subtitles
y = CCanvas::Current ()->Height () - ((nLineSpacing + 1) * MAX_ACTIVE_SUBTITLES + 2);

//erase old subtitles if necessary
if (bMustErase) {
	CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
	DrawFilledRect (0, y, CCanvas::Current ()->Width () - 1, CCanvas::Current ()->Height () - 1);
	}
//now draw the current subtitles
for (t = 0; t < nActiveSubTitles; t++)
	if (activeSubTitleList [t] != -1) {
		GrString (0x8000, y, m_captions [activeSubTitleList [t]].msg, NULL);
		y += nLineSpacing + 1;
	}
}

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

void* CMovie::Alloc (unsigned size)
{
return reinterpret_cast<void*> (new ubyte [size]);
}

// ----------------------------------------------------------------------

void CMovie::Free (void* p)
{
delete[] reinterpret_cast<ubyte*> (p);
}


//-----------------------------------------------------------------------

void CMovie::ShowFrame (ubyte* buf, uint bufw, uint bufh, uint sx, uint sy, uint w, uint h, uint dstx, uint dsty)
{
	CBitmap bmFrame;

bmFrame.Init (BM_LINEAR, 0, 0, bufw, bufh, 1, buf);
bmFrame.SetPalette (movieManager.m_palette);

TRANSPARENCY_COLOR = 0;
if (gameOpts->movies.bFullScreen) {
	double r = (double) bufh / (double) bufw;
	int dh = (int) (CCanvas::Current ()->Width () * r);
	int yOffs = (CCanvas::Current ()->Height () - dh) / 2;

	glDisable (GL_BLEND);
	bmFrame.Render (CCanvas::Current (), 0, yOffs, CCanvas::Current ()->Width (), dh, sx, sy, bufw, bufh, 1, 0, gameOpts->movies.nQuality);
	glEnable (GL_BLEND);
	}
else {
	int xOffs = (CCanvas::Current ()->Width () - 640) / 2;
	int yOffs = (CCanvas::Current ()->Height () - 480) / 2;

	if (xOffs < 0)
		xOffs = 0;
	if (yOffs < 0)
		yOffs = 0;
	dstx += xOffs;
	dsty += yOffs;
	GrBmUBitBlt (CCanvas::Current (), dstx, dsty, bufw, bufh, &bmFrame, sx, sy, 1);
	if ((CCanvas::Current ()->Width () > 640) || (CCanvas::Current ()->Height () > 480)) {
		CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 32));
		DrawEmptyRect (dstx - 1, dsty, dstx + w, dsty + h + 1);
		}
	}
TRANSPARENCY_COLOR = DEFAULT_TRANSPARENCY_COLOR;
bmFrame.SetBuffer (NULL);
}

//-----------------------------------------------------------------------
//our routine to set the palette, called from the movie code
void CMovie::SetPalette (ubyte* p, unsigned start, unsigned count)
{
	CPalette	palette;

if (count == 0)
	return;

//paletteManager.LoadEffect ();
//Color 0 should be black, and we get color 255
//movie libs palette into our array
if (start + count > PALETTE_SIZE)
	count = PALETTE_SIZE - start;
memcpy (palette.Color () + start, p + start * 3, count * 3);
//Set color 0 to be black
palette.SetBlack (0, 0, 0);
//Set color 255 to be our subtitle color
palette.SetTransparency (50, 50, 50);
//finally set the palette in the hardware
movieManager.m_palette = paletteManager.Add (palette);
paletteManager.LoadEffect ();
}

//-----------------------------------------------------------------------

uint CMovie::Read (void* handle, void* buf, uint count)
{
uint numread = (uint) (reinterpret_cast<CFile*> (handle))->Read (buf, 1, count);
return numread == count;
}

//-----------------------------------------------------------------------
//sets the file position to the start of this already-open file
void CMovie::Rewind (void)
{
if (m_cf.File ()) {
	m_cf.Seek (m_offset, SEEK_SET);
	m_pos = m_cf.Tell ();
	}
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

#define BOX_BORDER (gameStates.menus.bHires ? 40 : 20)

void ShowPauseMessage (const char* msg)
{
	int w, h, aw;
	int x, y;

CCanvas::SetCurrent (NULL);
fontManager.SetCurrent (SMALL_FONT);
fontManager.Current ()->StringSize (msg, w, h, aw);
x = (screen.Width () - w) / 2;
y = (screen.Height () - h) / 2;
CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
DrawFilledRect (x-BOX_BORDER/2, y-BOX_BORDER/2, x+w+BOX_BORDER/2-1, y+h+BOX_BORDER/2-1);
fontManager.SetColor (255, -1);
GrUString (0x8000, y, msg);
GrUpdate (0);
}

//-----------------------------------------------------------------------

void ClearPauseMessage ()
{
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

void CMovieLib::Init (void)
{
m_nMovies = 0;
m_flags = 0;
}

//-----------------------------------------------------------------------

void CMovieLib::Destroy (void)
{
m_movies.Destroy ();
Init ();
}

//-----------------------------------------------------------------------

int CMovieLib::SetupMVL (CFile& cf)
{
	int			nFiles, offset;
	int			i, len;

	//read movie file header

nFiles = cf.ReadInt ();        //get number of files
if (nFiles > 255) {
	gameStates.app.bLittleEndian = 0;
	nFiles = SWAPINT (nFiles);
	}
if (!m_movies.Create (nFiles))
	return 0;
m_nMovies = nFiles;
offset = 4 + 4 + nFiles * (13 + 4);	//id + nFiles + nFiles * (filename + size)
for (i = 0; i < nFiles; i++) {
	if (cf.Read (m_movies [i].m_name, 13, 1) != 1)
		break;		//end of file (probably)
	len = cf.ReadInt ();
	m_movies [i].m_len = len;
	m_movies [i].m_offset = offset;
	offset += m_movies [i].m_len;
	}
cf.Close ();
m_flags = 0;
return nFiles;
}

//-----------------------------------------------------------------------
// old format libs do not have length info at start of file
// so read and count movies

int CMovieLib::Count (CFile& cf)
{
	int	size = cf.Size ();
	int	nFiles = 0;
	int	fPos = cf.Tell ();

for (;;) {
	if (size < 17) // name + length
		return 0;
	cf.Seek (13, SEEK_CUR);
	cf.Seek (cf.ReadInt (), SEEK_CUR);       //skip data
	nFiles++;
	}
cf.Seek (fPos, SEEK_SET);
return nFiles;
}

//-----------------------------------------------------------------------

int CMovieLib::SetupHF (CFile& cf)
{
	int nFiles = Count (cf);

if (nFiles < 1)
	return 0;
if (!m_movies.Create (nFiles))
	return 0;
for (int i = 0; i < nFiles; i++) {
	cf.Read (m_movies [i].m_name, 13, 1);
	m_movies [i].m_len = cf.ReadInt ();
	m_movies [i].m_offset = cf.Tell ();
	cf.Seek (m_movies [i].m_len, SEEK_CUR);       //skip data
	}
return nFiles;
}

//-----------------------------------------------------------------------

//find the specified movie library, and read in list of movies in it
bool CMovieLib::Setup (const char* filename)
{
	//note: this based on CFInitHogFile ()

	char	id [4];
	CFile cf;
	int	nFiles = 0;

if (!cf.Open (filename, gameFolders.szMovieDir, "rb", 0))
	return false;
cf.Read (id, 4, 1);
if (!strncmp (id, "DMVL", 4))
	nFiles = SetupMVL (cf);
else if (!strncmp (id, "DHF", 3)) {
	cf.Seek (-1, SEEK_CUR);		//old file had 3 char id
	nFiles = SetupHF (cf);
	}
cf.Close ();
if (nFiles > 0)
	strcpy (m_name, filename);
return nFiles > 0;
}

//-----------------------------------------------------------------------

//looks through a movie library for a movie file
//returns filehandle, with fileposition at movie, or -1 if can't find
CMovie* CMovieLib::Open (char* filename, int bRequired)
{
	int i, bFromCD;

for (i = 0; i < m_nMovies; i++)
	if (!stricmp (filename, m_movies [i].m_name)) {	//found the movie in a library 
		if ((bFromCD = (m_flags & MLF_ON_CD)))
			redbook.Stop ();		//ready to read from CD
		do {		//keep trying until we get the file handle
			m_movies [i].m_cf.Open (m_name, gameFolders.szMovieDir, "rb", 0);
			if (bRequired && bFromCD && !m_movies [i].m_cf.File ()) {   //didn't get file!
				if (movieManager.RequestCD () == -1)		//ESC from requester
					break;						//bail from here. will get error later
				}
			} while (bRequired && bFromCD && !m_movies [i].m_cf.File ());
		m_movies [i].Rewind ();
		return m_movies + i;
	}
return NULL;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

void CMovieManager::Init (void)
{
m_nLibs = -1;
m_palette = NULL;
m_nRobots = 0;
m_bHaveIntro = 1;
m_bHaveExtras = 0;
m_nLib = -1;
m_nMovies = -1;
m_nMovie = -1;
m_libs.Create (N_MOVIE_LIBS);
}

//-----------------------------------------------------------------------

void CMovieManager::Destroy (void)
{
PrintLog ("unloading movies\n");
for (int i = 0; i < m_nLibs; i++)
	m_libs [i].Destroy ();
Init ();
}

//-----------------------------------------------------------------------
//ask user to put the D2 CD in.
//returns -1 if ESC pressed, 0 if OK chosen
//CD may not have been inserted
int CMovieManager::RequestCD (void)
{
#if 0
	ubyte save_pal [256*3];
	CCanvas *tcanv, canvP = CCanvas::Current ();
	int ret, was_faded=paletteManager.EffectDisabled ();

	GrPaletteStepClear ();

	CCanvas::Push ();
	tcanv = GrCreateCanvas (CCanvas::Current ()->bm.Width (), CCanvas::Current ()->bm.Height ());

	CCanvas::SetCurrent (tcanv);
	gr_ubitmap (0, 0, &canvP->Bitmap ());
	CCanvas::Pop ();

	GrClearCanvas (0);

 try_again:;

	ret = MsgBox ("CD ERROR", 1, "Ok", "Please insert your Descent II CD");

	if (ret == -1) {
		int ret2;

		ret2 = MsgBox ("CD ERROR", 2, "Try Again", "Leave Game", "You must insert your\nDescent II CD to Continue");

		if (ret2 == -1 || ret2 == 0)
			goto try_again;
	}
	force_rb_register = 1;  //disc has changed; force register new CD
	GrPaletteStepClear ();
	gr_ubitmap (0, 0, &tcanv->Bitmap ());
	if (!was_faded)
		paletteManager.LoadEffect ();
	GrFreeCanvas (tcanv);
	return ret;
#else
#if TRACE
	console.printf (DEBUG_LEVEL, "STUB: movie: RequestCD\n");
#endif
	return 0;
#endif
}

//-----------------------------------------------------------------------

void CMovieManager::InitLib (const char* pszFilename, int nLibrary, int bRobotMovie, int bRequired)
{
	int	bHires, nTries;
	char	filename [FILENAME_LEN];
	char	cdName [100];

strcpy (filename, pszFilename);
	
	char* pszRes = strchr (filename, '.') - 1; // 'h' == high pszResolution, 'l' == low

//for robots, load highpszRes versions if highpszRes menus set
bHires = bRobotMovie ? gameStates.menus.bHiresAvailable : gameOpts->movies.bHires;
if (bHires)
	*pszRes = 'h';
for (nTries = 0; !m_libs [nLibrary].Setup (filename); nTries++) {
	strcpy (cdName, CDROM_dir);
	strcat (cdName, filename);
	if (m_libs [nLibrary].Setup (cdName)) {
		m_libs [nLibrary].m_flags |= MLF_ON_CD;
		break; // we found our movie on the CD
		}
	if (nTries == 0) { // first attempt
		if (*pszRes == 'h') { // try low res instead
			*pszRes = 'l';
			bHires = 0;
			}
		else if (*pszRes == 'l') { // try hires
			*pszRes = 'h';
			bHires = 1;
			}
		else {
#if DBG
			if (bRequired)
				Warning (TXT_MOVIE_FILE, filename);
#endif
			break;
			}
		}
	else { // nTries == 1
		if (bRequired) {
			*pszRes = '*';
#if DBG
			//Warning (TXT_MOVIE_ANY, filename);
#endif
			}
		break;
		}
	}

if (bRobotMovie && m_libs [nLibrary].m_nMovies)
	m_nRobots = bHires ? 2 : 1;
}

//-----------------------------------------------------------------------

//find and initialize the movie libraries
void CMovieManager::InitLibs (void)
{
	int bRobotMovie;

m_nLibs = 0;

int j = (m_bHaveExtras = !gameStates.app.bNostalgia) 
		  ? N_BUILTIN_MOVIE_LIBS 
		  : FIRST_EXTRA_MOVIE_LIB;

for (int i = 0; i < j; i++) {
	bRobotMovie = !strnicmp (pszMovieLibs [i], "robot", 5);
	InitLib (pszMovieLibs [i], m_nLibs, bRobotMovie, 1);
	if (m_libs [m_nLibs].m_nMovies) {
		m_nLibs++;
		PrintLog ("   found movie lib '%s'\n", pszMovieLibs [i]);
		}
	else if ((i >= FIRST_EXTRA_MOVIE_LIB) && (i < FIRST_EXTRA_MOVIE_LIB + N_EXTRA_MOVIE_LIBS))
		m_bHaveExtras = 0;
	}

m_libs [EXTRA_ROBOT_LIB].m_nMovies = 0;
}

//-----------------------------------------------------------------------

void CMovieManager::InitExtraRobotLib (char* filename)
{
m_libs [EXTRA_ROBOT_LIB].Destroy ();
InitLib (filename, EXTRA_ROBOT_LIB, 1, 0);
}

//-----------------------------------------------------------------------
//returns file handle
CMovie* CMovieManager::Open (char* filename, int bRequired)
{
	CMovie*	movieP;

for (int i = 0; i < m_nLibs; i++)
	if ((movieP = m_libs [i].Open (filename, bRequired)))
		return movieP;
return NULL;    //couldn't find it
}

//-----------------------------------------------------------------------
//filename will actually get modified to be either low-res or high-res
//returns status.  see values in movie.h

int CMovieManager::Play (const char* filename, int bRequired, int bForce, int bFullScreen)
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
audio.Shutdown ();
// Start sound
MVE_sndInit (gameStates.app.bUseSound ? 1 : -1);
gameOpts->movies.bFullScreen = bFullScreen;
ret = Run (name, gameOpts->movies.bHires, bRequired, -1, -1);
audio.Setup (1);
gameStates.video.nScreenMode = -1;		//force screen reset
return ret;
}

//-----------------------------------------------------------------------
//returns status.  see movie.h
int CMovieManager::Run (char* filename, int bHires, int bRequired, int dx, int dy)
{
	CFile			cf;
	CMovie*		movieP = NULL;
	int			result = 1, aborted = 0;
	int			track = 0;
	int			nFrame;
	int			key;
	CMovieLib*	libP = Find (filename);

result = 1;
// Open Movie file.  If it doesn't exist, no movie, just return.
if (!(cf.Open (filename, gameFolders.szDataDir, "rb", 0) || (movieP = Open (filename, bRequired)))) {
	if (bRequired) {
#if TRACE
		console.printf (CON_NORMAL, "movie: RunMovie: Cannot open movie <%s>\n", filename);
#endif
		}
	return MOVIE_NOT_PLAYED;
	}
SetScreenMode (SCREEN_MENU);
paletteManager.LoadEffect ();
MVE_memCallbacks (CMovie::Alloc, CMovie::Free);
MVE_ioCallbacks (CMovie::Read);
MVE_sfCallbacks (CMovie::ShowFrame);
MVE_palCallbacks (CMovie::SetPalette);
if (MVE_rmPrepMovie (reinterpret_cast<void*> (movieP ? &movieP->m_cf: &cf), dx, dy, track, libP ? libP->m_bLittleEndian : 1)) {
	Int3 ();
	return MOVIE_NOT_PLAYED;
	}
nFrame = 0;
gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && bHires;
SetRenderQuality (gameOpts->movies.nQuality ? 5 : 0);
while ((result = MVE_rmStepMovie ()) == 0) {
	subTitles.Draw (nFrame);
	paletteManager.LoadEffect (); // moved this here because of flashing
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
if (movieP)
	movieP->Close ();
else
	cf.Close ();                           // Close Movie File
// Restore old graphic state
SetRenderQuality ();
gameStates.video.nScreenMode = -1;  //force reset of screen mode
paletteManager.LoadEffect ();
return (aborted ? MOVIE_ABORTED : MOVIE_PLAYED_FULL);
}

//-----------------------------------------------------------------------

char* CMovieManager::Cycle (int bRestart, int bPlayMovie)
{
	char* pszMovieName;

if (m_nLibs < 0) {
	InitLibs ();
	bRestart = 1;
	}

if (bRestart)
	m_nLib =
	m_nMovies = 
	m_nMovie = -1;

if (m_nLibs) {
	if (++m_nMovie >= m_nMovies) {
		m_nLib = (m_nLib + 1) % m_nLibs;
		if (m_nLib == 2)	//skip the robot movies
			m_nLib = (m_nLib + 1) % m_nLibs;
		m_nMovies = m_libs [m_nLib].m_nMovies;
		m_nMovie = 0;
		}
	//InitSubTitles ("intro.tex");
	if ((pszMovieName = m_libs [m_nLib].m_movies [m_nMovie].m_name)) {
		gameStates.video.nScreenMode = -1;
		if (bPlayMovie)
			movieManager.Play (pszMovieName, 1, 1, gameOpts->movies.bResize);
		}
	return pszMovieName;
	//CloseSubTitles ();
	}
return NULL;
}

//-----------------------------------------------------------------------

CMovieLib* CMovieManager::FindLib (const char* pszLib)
{
if (m_nLibs < 0)
	InitLibs ();
if (m_nLibs) 
	for (int i = 0; i < m_nLibs; i++)
		if (!strcmp (pszLib, m_libs [i].m_name))
			return m_libs + i;
return NULL;
}

//-----------------------------------------------------------------------

CMovieLib* CMovieManager::Find (const char* pszMovie)
{
if (m_nLibs < 0)
	InitLibs ();
if (m_nLibs)
	for (int i = 0; i < m_nLibs; i++)
		for (int j = 0; j < m_libs [i].m_nMovies; j++)
			if (!strcmp (pszMovie, m_libs [i].m_movies [j].m_name))
				return m_libs + i;
return NULL;
}

//-----------------------------------------------------------------------

void CMovieManager::PlayIntro (void)
{
if (m_bHaveIntro) {
	subTitles.Init ("intro.tex");
	if (Play ("intro.mve", MOVIE_REQUIRED, 0, gameOpts->movies.bResize) == MOVIE_NOT_PLAYED)
		m_bHaveIntro = 0;
	subTitles.Close ();
	}
}

//-----------------------------------------------------------------------

int CMovieManager::StartRobot (char* filename)
{
	CMovieLib*	libP = movieManager.Find (filename);

if (gameOpts->movies.nLevel < 1)
	return 0;

#if TRACE
console.printf (DEBUG_LEVEL, "movies.robot.cf=%s\n", filename);
#endif
MVE_sndInit (-1);        //tell movies to play no sound for robots
if (!(m_robotP = movieManager.Open (filename, 1))) {
#if DBG
	Warning (TXT_MOVIE_ROBOT, filename);
#endif
	return MOVIE_NOT_PLAYED;
	}
gameOpts->movies.bFullScreen = 1;
m_robotP->m_bLittleEndian = libP ? libP->m_bLittleEndian : 1;
MVE_memCallbacks (CMovie::Alloc, CMovie::Free);
MVE_ioCallbacks (CMovie::Read);
MVE_sfCallbacks (CMovie::ShowFrame);
MVE_palCallbacks (CMovie::SetPalette);
if (MVE_rmPrepMovie (reinterpret_cast<void*> (&m_robotP->m_cf), 
							gameStates.menus.bHires ? 280 : 140, 
							gameStates.menus.bHires ? 200 : 80, 0,
							m_robotP->m_bLittleEndian)) {
	Int3 ();
	return 0;
	}
return 1;
}

//-----------------------------------------------------------------------
//returns 1 if frame updated ok
int CMovieManager::RotateRobot (void)
{
if (!m_robotP)
	return 0;

	int res;

gameOpts->movies.bFullScreen = 1;
if (gameStates.ogl.nDrawBuffer == GL_BACK)
	paletteManager.LoadEffect ();
res = MVE_rmStepMovie ();
paletteManager.LoadEffect ();
if (res == MVE_ERR_EOF) {   //end of movie, so reset
	m_robotP->Rewind ();
	if (MVE_rmPrepMovie (reinterpret_cast<void*> (&m_robotP->m_cf), 
								gameStates.menus.bHires ? 280 : 140, 
								gameStates.menus.bHires ? 200 : 80, 0,
								m_robotP->m_bLittleEndian)) {
		return 0;
		}
	}
else if (res) {
	return 0;
	}
return 1;
}

//-----------------------------------------------------------------------

void CMovieManager::StopRobot (void)
{
if (m_robotP) {
	MVE_rmEndMovie ();
	m_robotP->Close ();                           // Close Movie File
	m_robotP = NULL;
	}
}

//-----------------------------------------------------------------------

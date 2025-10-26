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

#include "descent.h"
#include "movie.h"
#include "key.h"
#include "strutil.h"
#include "error.h"
#include "u_mem.h"
#include "byteswap.h"
#include "ogl_render.h"
#include "ogl_bitmap.h"
#include "gamefont.h"
#include "libmve.h"
#include "text.h"
#include "screens.h"
#include "midi.h"
#include "songs.h"
#include "config.h"
#include "cockpit.h"
#include "renderlib.h"

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
uint8_t* CSubTitles::NextField (uint8_t* p)
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

int32_t CSubTitles::Init (const char* filename)
{
	CFile cf;
	int32_t 	size, readCount;
	uint8_t	*p;
	int32_t 	bHaveBinary = 0;

m_nCaptions = 0;
if (!gameOpts->movies.bSubTitles)
	return 0;
if (!cf.Open (filename, gameFolders.game.szData [0], "rb", 0)) { // first try text version
	char filename2 [FILENAME_LEN];	//no text version, try binary version
	CFile::ChangeFilenameExtension (filename2, filename, ".txb");
	if (!cf.Open (filename2, gameFolders.game.szData [0], "rb", 0))
		return 0;
	bHaveBinary = 1;
	}

size = (int32_t) cf.Length ();
m_pRawData = NEW uint8_t [size+1];
readCount = (int32_t) cf.Read (m_pRawData, 1, size);
cf.Close ();
m_pRawData [size] = 0;
if (readCount != size) {
	delete[] m_pRawData;
	return 0;
	}
p = m_pRawData;
while (p && (p < m_pRawData + size)) {
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
	p = reinterpret_cast<uint8_t*> (endp + 1);
	}
return 1;
}

//-----------------------------------------------------------------------

void CSubTitles::Close (void)
{
if (m_pRawData) {
	delete[] m_pRawData;
	m_pRawData = NULL;
	}
m_nCaptions = 0;
}

//-----------------------------------------------------------------------
//draw the subtitles for this frame
void CSubTitles::Draw (int32_t nFrame)
{
	static int32_t activeSubTitleList [MAX_ACTIVE_SUBTITLES];
	static int32_t nActiveSubTitles, nNextSubTitle, nLineSpacing;
	int32_t t, y;
	int32_t bMustErase = 0;

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
		int32_t t2;
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

	CFrameController fc;
	for (fc.Begin (); fc.Continue (); fc.End ()) {
	if (ogl.VRActive ()) {
		int32_t w, h;
		cockpit->SetupSceneCenter (&gameData.renderData.frame, w, h);
		}
	else
		gameData.renderData.frame.Activate ("CSubTitles::Draw (frame)");

	//find y coordinate for first line of subtitles
	y = CCanvas::Current ()->Height () - ((nLineSpacing + 1) * MAX_ACTIVE_SUBTITLES + 2);

	//erase old subtitles if necessary
	if (bMustErase) {
		CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
		OglDrawFilledRect (0, y, CCanvas::Current ()->Width () - 1, CCanvas::Current ()->Height () - 1);
		}
	//now draw the current subtitles
	for (t = 0; t < nActiveSubTitles; t++)
		if (activeSubTitleList [t] != -1) {
			GrString (0x8000, y, m_captions [activeSubTitleList [t]].msg);
			y += nLineSpacing + 1;
		}
	if (ogl.VRActive ())
		gameData.renderData.window.Deactivate ();
	else
		gameData.renderData.frame.Deactivate ();
	}
}

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

void* CMovie::Alloc (uint32_t size)
{
return reinterpret_cast<void*> (NEW uint8_t [size]);
}

// ----------------------------------------------------------------------

void CMovie::Free (void* p)
{
delete[] reinterpret_cast<uint8_t*> (p);
}


//-----------------------------------------------------------------------

void CMovie::ShowFrame (uint8_t* buf, uint32_t wBuffer, uint32_t hBuffer, uint32_t xSrc, uint32_t ySrc, uint32_t w, uint32_t h, uint32_t xDest, uint32_t yDest)
{
	CBitmap bmFrame;

bmFrame.Init (BM_LINEAR, 0, 0, wBuffer, hBuffer, 1, buf);
bmFrame.SetPalette (movieManager.m_palette);

TRANSPARENCY_COLOR = 0;

CFrameController fc;
for (fc.Begin (); fc.Continue (); fc.End ()) {
	if (ogl.VRActive ()) {
		int32_t w, h;
		cockpit->SetupSceneCenter (&gameData.renderData.frame, w, h);
		}
	else
		gameData.renderData.frame.Activate ("CMovie::ShowFrame (frame)");

	if (gameOpts->movies.bFullScreen > 0) {
		double r = (double) hBuffer / (double) wBuffer;
		int32_t dh = (int32_t) (CCanvas::Current ()->Width () * r);
		int32_t yOffs = (CCanvas::Current ()->Height () - dh) / 2;

		ogl.SetBlending (false);
		bmFrame.AddFlags (BM_FLAG_OPAQUE);
		bmFrame.SetTranspType (0);
		bmFrame.Render (NULL, 0, yOffs, CCanvas::Current ()->Width (), dh, xSrc, ySrc, wBuffer, hBuffer, 0, 0, gameOpts->movies.nQuality);
		ogl.SetBlending (true);
		}
	else {
		if (gameOpts->movies.bFullScreen < 0) {
			w = gameData.renderData.frame.Width (false) * int32_t (w) / 640;
			h = gameData.renderData.frame.Width (false) * int32_t (h) / 640;
			}
		int32_t xOffs = (CCanvas::Current ()->Width () - w - xDest) / 2;
		int32_t yOffs = (CCanvas::Current ()->Height () - h - yDest) / 2;

		if (xOffs < 0)
			xOffs = 0;
		if (yOffs < 0)
			yOffs = 0;
		xOffs += xDest;
		yOffs += yDest;
		//bmFrame.Blit (CCanvas::Current (), xOffs, yOffs, bufw, bufh, xSrc, ySrc, 1);
		bmFrame.Render (NULL, xOffs, yOffs, w, h, xSrc, ySrc, wBuffer, hBuffer, 0, 0, gameOpts->movies.nQuality);
		if (!gameOpts->movies.bFullScreen && (((uint32_t) CCanvas::Current ()->Width () > w) || ((uint32_t) CCanvas::Current ()->Height () > h))) {
			CCanvas::Current ()->SetColorRGBi (RGB_PAL (0, 0, 32));
			OglDrawEmptyRect (xDest - 1, yDest, xDest + w, yDest + h + 1);
			}
		}
	if (ogl.VRActive ())
		gameData.renderData.window.Deactivate ();
	else
		gameData.renderData.frame.Deactivate ();
	}
TRANSPARENCY_COLOR = DEFAULT_TRANSPARENCY_COLOR;
bmFrame.SetBuffer (NULL);
}

//-----------------------------------------------------------------------
//our routine to set the palette, called from the movie code
void CMovie::SetPalette (uint8_t* p, uint32_t start, uint32_t count)
{
	CPalette	palette;

if (count == 0)
	return;

////paletteManager.ResumeEffect ();
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
//paletteManager.ResumeEffect ();
}

//-----------------------------------------------------------------------

uint32_t CMovie::Read (void* handle, void* buf, uint32_t count)
{
uint32_t numread = (uint32_t) (reinterpret_cast<CFile*> (handle))->Read (buf, 1, count);
return numread == count;
}

//-----------------------------------------------------------------------
//sets the file position to the start of this already-open file
void CMovie::Rewind (void)
{
if (m_cf.File ()) {
	m_cf.Seek (m_offset, SEEK_SET);
	m_pos = (int32_t) m_cf.Tell ();
	}
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

#define BOX_BORDER (gameStates.menus.bHires ? 40 : 20)

void ShowPauseMessage (const char* msg)
{
	int32_t w, h, aw;
	int32_t x, y;

fontManager.SetCurrent (SMALL_FONT);
fontManager.Current ()->StringSize (msg, w, h, aw);
x = (gameData.renderData.screen.Width () - w) / 2;
y = (gameData.renderData.screen.Height () - h) / 2;
CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
OglDrawFilledRect (x - BOX_BORDER / 2, y - BOX_BORDER / 2, x + w + BOX_BORDER / 2 - 1, y + h + BOX_BORDER / 2 - 1);
fontManager.SetColor (255, -1);
GrUString (0x8000, y, msg);
ogl.Update (0);
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

int32_t CMovieLib::SetupMVL (CFile& cf)
{
	int32_t			nFiles, offset;
	int32_t			i, len;

	//read movie file header

nFiles = cf.ReadInt ();        //get number of files
if (nFiles > 255) {
	gameStates.app.bLittleEndian = 0;
	nFiles = SWAPINT (nFiles);
	}
if (!m_movies.Create (nFiles, "CMovieLib::m_movies"))
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

int32_t CMovieLib::Count (CFile& cf)
{
	int32_t	size = (int32_t) cf.Size ();
	int32_t	nFiles = 0;
	int32_t	fPos = (int32_t) cf.Tell ();

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

int32_t CMovieLib::SetupHF (CFile& cf)
{
	int32_t nFiles = Count (cf);

if (nFiles < 1)
	return 0;
if (!m_movies.Create (nFiles))
	return 0;
for (int32_t i = 0; i < nFiles; i++) {
	cf.Read (m_movies [i].m_name, 13, 1);
	m_movies [i].m_len = cf.ReadInt ();
	m_movies [i].m_offset = (int32_t) cf.Tell ();
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
	int32_t	nFiles = 0;

if (!cf.Open (filename, gameFolders.game.szMovies, "rb", 0))
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
CMovie* CMovieLib::Open (char* filename, int32_t bRequired)
{
	int32_t i, bFromCD;

for (i = 0; i < m_nMovies; i++)
	if (!stricmp (filename, m_movies [i].m_name)) {	//found the movie in a library 
		if ((bFromCD = (m_flags & MLF_ON_CD)))
			redbook.Stop ();		//ready to read from CD
		do {		//keep trying until we get the file handle
			m_movies [i].m_cf.Open (m_name, gameFolders.game.szMovies, "rb", 0);
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
m_libs.Create (N_MOVIE_LIBS, "CMovieManager::m_libs");
}

//-----------------------------------------------------------------------

void CMovieManager::Destroy (void)
{
PrintLog (1, "unloading movies\n");
for (int32_t i = 0; i < m_nLibs; i++)
	m_libs [i].Destroy ();
Init ();
PrintLog (-1);
}

//-----------------------------------------------------------------------
//ask user to put the D2 CD in.
//returns -1 if ESC pressed, 0 if OK chosen
//CD may not have been inserted
int32_t CMovieManager::RequestCD (void)
{
return -1;
}

//-----------------------------------------------------------------------

void CMovieManager::InitLib (const char* pszFilename, int32_t nLibrary, int32_t bRobotMovie, int32_t bRequired)
{
	int32_t	bHires, nTries;
	char	filename [FILENAME_LEN];
	char	cdName [100];

strcpy (filename, pszFilename);
	
	char* pszRes = strchr (filename, '.') - 1; // 'h' == high pszResolution, 'l' == low

//for robots, load highpszRes versions if highpszRes menus set
bHires = bRobotMovie ? gameStates.menus.bHiresAvailable : gameOpts->movies.bHires;
if (bHires)
	*pszRes = 'h';
for (nTries = 0; !m_libs [nLibrary].Setup (filename) && (nTries < 4); nTries++) {
	strcpy (cdName, (nTries < 2) ? CDROM_dir : "d1/");
	strcat (cdName, filename);
	if (m_libs [nLibrary].Setup (cdName) && (nTries < 2)) {
		m_libs [nLibrary].m_flags |= MLF_ON_CD;
		break; // we found our movie on the CD
		}
	if (*pszRes == 'h') { // try low res instead
		*pszRes = 'l';
		bHires = 0;
		}
	else if (*pszRes == 'l') { // try hires
		*pszRes = 'h';
		bHires = 1;
		}
	}

#if 0 //DBG
if ((nTries == 4) && bRequired)
	Warning (TXT_MOVIE_FILE, filename);
#endif

if (bRobotMovie && m_libs [nLibrary].m_nMovies)
	m_nRobots = bHires ? 2 : 1;
}

//-----------------------------------------------------------------------

//find and initialize the movie libraries
void CMovieManager::InitLibs (void)
{
	int32_t bRobotMovie;

m_nLibs = 0;

int32_t j = (m_bHaveExtras = !gameStates.app.bNostalgia) 
		  ? N_BUILTIN_MOVIE_LIBS 
		  : FIRST_EXTRA_MOVIE_LIB;

for (int32_t i = 0; i < j; i++) {
	bRobotMovie = !strnicmp (pszMovieLibs [i], "robot", 5);
	InitLib (pszMovieLibs [i], m_nLibs, bRobotMovie, 1);
	if (m_libs [m_nLibs].m_nMovies) {
		m_nLibs++;
		PrintLog (0, "found movie lib '%s'\n", pszMovieLibs [i]);
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
CMovie* CMovieManager::Open (char* filename, int32_t bRequired)
{
	CMovie*	pMovie;

for (int32_t i = 0; i < m_nLibs; i++)
	if ((pMovie = m_libs [i].Open (filename, bRequired)))
		return pMovie;
return NULL;    //couldn't find it
}

//-----------------------------------------------------------------------
//filename will actually get modified to be either low-res or high-res
//returns status.  see values in movie.h

int32_t CMovieManager::Play (const char* filename, int32_t bRequired, int32_t bForce, int32_t bFullScreen)
{
	char name [FILENAME_LEN], *p;
	int32_t c, ret;

#if 1//!DBG
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
SetScreenMode (SCREEN_MENU);
audio.Shutdown ();
// Start sound
MVE_sndInit (gameStates.app.bUseSound ? 1 : -1);
midi.FixVolume (64);
gameOpts->movies.bFullScreen = bFullScreen;
ret = Run (name, gameOpts->movies.bHires, bRequired, -1, -1);
audio.Setup (1);
gameStates.video.nScreenMode = -1;		//force screen reset
return ret;
}

//-----------------------------------------------------------------------
//returns status.  see movie.h
int32_t CMovieManager::Run (char* filename, int32_t bHires, int32_t bRequired, int32_t dx, int32_t dy)
{
	CFile			cf;
	CMovie*		pMovie = NULL;
	int32_t		result = 1, aborted = 0;
	int32_t		track = 0;
	int32_t		nFrame;
	int32_t		key;
	CMovieLib*	pLib = Find (filename);

result = 1;
// Open Movie file.  If it doesn't exist, no movie, just return.
if (!(cf.Open (filename, gameFolders.game.szData [0], "rb", 0) || (pMovie = Open (filename, bRequired)))) {
	if (bRequired) {
#if TRACE
		console.printf (CON_NORMAL, "movie: RunMovie: Cannot open movie <%s>\n", filename);
#endif
		}
	return MOVIE_NOT_PLAYED;
	}
SetScreenMode (SCREEN_MENU);
//paletteManager.ResumeEffect ();
MVE_memCallbacks (CMovie::Alloc, CMovie::Free);
MVE_ioCallbacks (CMovie::Read);
MVE_sfCallbacks (CMovie::ShowFrame);
MVE_palCallbacks (CMovie::SetPalette);
if (MVE_rmPrepMovie (reinterpret_cast<void*> (pMovie ? &pMovie->m_cf: &cf), dx, dy, track, pLib ? pLib->m_bLittleEndian : 1)) {
	Int3 ();
	return MOVIE_NOT_PLAYED;
	}
nFrame = 0;
gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && bHires;
ogl.SetRenderQuality (gameOpts->movies.nQuality ? 5 : 0);
while ((result = MVE_rmStepMovie ()) == 0) {
	subTitles.Draw (nFrame);
	//paletteManager.ResumeEffect (); // moved this here because of flashing
	ogl.Update (1);
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
		ogl.ToggleFullScreen ();
	nFrame++;
	}
Assert (aborted || result == MVE_ERR_EOF);	 ///movie should be over
MVE_rmEndMovie ();
if (pMovie)
	pMovie->Close ();
else
	cf.Close ();                           // Close Movie File
// Restore old graphic state
ogl.SetRenderQuality ();
gameStates.video.nScreenMode = -1;  //force reset of screen mode
//paletteManager.ResumeEffect ();
return (aborted ? MOVIE_ABORTED : MOVIE_PLAYED_FULL);
}

//-----------------------------------------------------------------------

char* CMovieManager::Cycle (int32_t bRestart, int32_t bPlayMovie)
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
	for (int32_t i = 0; i < m_nLibs; i++)
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
	for (int32_t i = 0; i < m_nLibs; i++)
		for (int32_t j = 0; j < m_libs [i].m_nMovies; j++)
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

int32_t CMovieManager::StartRobot (char* filename)
{
	CMovieLib*	pLib = movieManager.Find (filename);

if (gameOpts->movies.nLevel < 1)
	return 0;

#if TRACE
console.printf (DEBUG_LEVEL, "movies.robot.cf=%s\n", filename);
#endif
MVE_sndInit (-1);        //tell movies to play no sound for robots
if (!(m_pRobot = movieManager.Open (filename, 1))) {
#if DBG
	Warning (TXT_MOVIE_ROBOT, filename);
#endif
	return MOVIE_NOT_PLAYED;
	}
int32_t bFullScreen = gameOpts->movies.bFullScreen;
gameOpts->movies.bFullScreen = -1;
m_pRobot->m_bLittleEndian = pLib ? pLib->m_bLittleEndian : 1;
MVE_memCallbacks (CMovie::Alloc, CMovie::Free);
MVE_ioCallbacks (CMovie::Read);
MVE_sfCallbacks (CMovie::ShowFrame);
MVE_palCallbacks (CMovie::SetPalette);
int32_t res = MVE_rmPrepMovie (reinterpret_cast<void*> (&m_pRobot->m_cf), 
							      gameStates.menus.bHires ? 280 : 140, 
									gameStates.menus.bHires ? 200 : 80, 0,
									m_pRobot->m_bLittleEndian);
gameOpts->movies.bFullScreen = bFullScreen;
return res ? 0 : 1;
}

//-----------------------------------------------------------------------
//returns 1 if frame updated ok
int32_t CMovieManager::RotateRobot (void)
{
if (!m_pRobot)
	return 0;

int32_t bFullScreen = gameOpts->movies.bFullScreen;
gameOpts->movies.bFullScreen = -1;
int32_t res = MVE_rmStepMovie ();
gameOpts->movies.bFullScreen = bFullScreen;

if (!res)
	return 1;
if (res == MVE_ERR_EOF) {   //end of movie, so reset
	m_pRobot->Rewind ();
	if (!MVE_rmPrepMovie (reinterpret_cast<void*> (&m_pRobot->m_cf), 
								 gameStates.menus.bHires ? 280 : 140, 
								 gameStates.menus.bHires ? 200 : 80, 0,
								 m_pRobot->m_bLittleEndian)) 
		return 1;
	}
return 0;
}

//-----------------------------------------------------------------------

void CMovieManager::StopRobot (void)
{
if (m_pRobot) {
	MVE_rmEndMovie ();
	m_pRobot->Close ();                           // Close Movie File
	m_pRobot = NULL;
	}
}

//-----------------------------------------------------------------------

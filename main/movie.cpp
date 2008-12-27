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

#define MAX_SUBTITLES 500
#define MAX_ACTIVE_SUBTITLES 3

//-----------------------------------------------------------------------
// Subtitle data
typedef struct {
	short first_frame, last_frame;
	char* msg;
} subtitle;

class CSubTitles {
	public:
		subtitle captions [MAX_SUBTITLES];
		int		nCaptions;
		ubyte*	rawDataP;

	public:
		int Init (const char* filename);
		void Close (void);
		void Draw (int nFrame);
	};

CSubTitles subTitles;


// Movielib data
class CMovie {
	public:
		CFile	m_cf;
		char	m_name [FILENAME_LEN];
		int	m_offset;
		int	m_len;
		int	m_pos;
		int	m_bLittleEndian;

	public:
		~CMovie () { Close (); }
		static void ShowFrame (ubyte* buf, uint bufw, uint bufh, uint sx, uint sy, uint w, uint h, uint dstx, uint dsty);
		static void SetPalette (ubyte* p, unsigned start, unsigned count);
		static void* Alloc (unsigned size);
		static void Free (void* p);
		static uint Read (void* handle, void* buf, uint count);
		void Rewind (void);
		inline void Close (void) { m_cf.Close (); }
	};


class CMovieLib {
	public:
		char				m_name [100]; // [FILENAME_LEN];
		int				m_nMovies;
		ubyte				m_flags;
		ubyte				m_pad [3];
		CArray<CMovie>	m_movies;
		int				m_bLittleEndian;

	public:
		CMovieLib () { Init (); }
		~CMovieLib () { Destroy (); }
		void Init (void);
		void Destroy (void);
		CMovie* Open (char* filename, int bRequired);
		bool Setup (char* filename);
		static CMovieLib* Create (char* filename);

	private:	
		int SetupMVL (CFile& cf);
		int SetupDF (CFile& cf)

};

class CMovieManager {
	public:
		CArray<CMovieLib>	m_libs; // [N_MOVIE_LIBS];
		int					m_nLibs;
		CPalette*			m_palette;
		CRobotMovie			m_robot;
		int					m_nRobots;
		int					m_bHaveIntro;
		int					m_bHaveExtras;

	private:
		int					m_nLib;
		int					m_nMovies;
		int					m_nMovie;
		CMovie*				m_robotP;

	public:
		CMovieManager () { Init (); }
		~CMovieManager () { Destroy (); }
		void Init (void);
		void InitExtraRobotLib (char* filename);
		void Destroy (void);
		void InitLibs (void);
		CMovieLib* FindLib (const char* pszLib);
		CMovieLib* Find (const char* pszMovie);
		int Run (char* filename, int bHires, int bAllowAbort, int dx, int dy);
		int Play (const char* filename, int bRequired, int bForce, int bFullScreen);
		CMovie* Open (char* filename, int bRequired);
		int RequestCD (void);
		char* Cycle (int bRestart, int bPlayMovie);
		void StartRobot (void);
		void RotateRobot (void);
		void EndRobot (void);
};

CMovieManager movieManager;


void DecodeTextLine (char* p);
void DrawSubTitles (int nFrame);

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
	delete [] m_rawDataP;
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
		if (!(p = next_field (p))) 
			continue;
		m_captions [m_nCaptions].last_frame = atoi (reinterpret_cast<char*> (p));
		if (!(p = next_field (p)))
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
	fontManager.SetColor (255, -1);
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
y = CCanvas::Current ()->Height () - ((nLineSpacing+1)*MAX_ACTIVE_SUBTITLES+2);

//erase old subtitles if necessary
if (bMustErase) {
	CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
	GrRect (0, y, CCanvas::Current ()->Width ()-1, CCanvas::Current ()->Height ()-1);
	}
//now draw the current subtitles
for (t = 0; t < nActiveSubTitles; t++)
	if (activeSubTitleList [t] != -1) {
		GrString (0x8000, y, m_captions [activeSubTitleList [t]].msg, NULL);
		y += nLineSpacing+1;
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
delete [] reinterpret_cast<ubyte*> (p);
}


//-----------------------------------------------------------------------

void CMovie::ShowFrame (ubyte* buf, uint bufw, uint bufh, uint sx, uint sy, uint w, uint h, uint dstx, uint dsty)
{
	CBitmap bmFrame;

bmFrame.Init (BM_LINEAR, 0, 0, bufw, bufh, 1, buf);
bmFrame.SetPalette (movies.palette);

TRANSPARENCY_COLOR = -1;
if (gameOpts->menus.nStyle) {
	//memset (grPalette, 0, 768);
	//paletteManager.LoadEffect ();
	}
if (gameOpts->movies.bFullScreen) {
	double r = (double) bufh / (double) bufw;
	int dh = (int) (CCanvas::Current ()->Width () * r);
	int yOffs = (CCanvas::Current ()->Height () - dh) / 2;

	glDisable (GL_BLEND);
	bmFrame.Render (CCanvas::Current (), 0, yOffs, CCanvas::Current ()->Width (), dh, 
						 sx, sy, bufw, bufh, 1, gameOpts->movies.nQuality);
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
	if ((CCanvas::Current ()->Width () > 640) || (CCanvas::Current ()->Height () > 480)) {
		CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 32));
		GrUBox (dstx-1, dsty, dstx+w, dsty+h+1);
		}
	GrBmUBitBlt (CCanvas::Current (), dstx, dsty, bufw, bufh, &bmFrame, sx, sy, 1);
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
movies.palette = paletteManager.Add (palette);
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
if (m_cf.File ())
	m_cf.Seek (m_offset, SEEK_SET);
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
GrRect (x-BOX_BORDER/2, y-BOX_BORDER/2, x+w+BOX_BORDER/2-1, y+h+BOX_BORDER/2-1);
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
	if (MVE_rmPrepMovie (reinterpret_cast<void*> (&movies.robot.cf), 
								gameStates.menus.bHires ? 280 : 140, 
								gameStates.menus.bHires ? 200 : 80, 0,
								movies.robot.bLittleEndian)) {
		return 0;
		}
	}
else if (err) {
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

int CMovieManager::StartRobot (char* filename)
{
	CMovieLib*	libP = movieManager.Find (filename);
	CFile&		cf;

if (gameOpts->movies.nLevel < 1)
	return 0;

#if TRACE
console.printf (DEBUG_LEVEL, "movies.robot.cf=%s\n", filename);
#endif
MVE_sndInit (-1);        //tell movies to play no sound for robots
if (!(m_robotP = movieManager.Open (cf, filename, 1))) {
#if DBG
	Warning (TXT_MOVIE_ROBOT, filename);
#endif
	return MOVIE_NOT_PLAYED;
	}
gameOpts->movies.bFullScreen = 1;
robotP->m_bLittleEndian = libP ? libP->m_bLittleEndian : 1;
MVE_memCallbacks (CMovie::Alloc, CMove::Free);
MVE_ioCallbacks (CMove::Read);
MVE_sfCallbacks (CMovie::ShowFrame);
MVE_palCallbacks (CMovie::SetPalette);
if (MVE_rmPrepMovie (reinterpret_cast<void*> (&cf), 
							gameStates.menus.bHires ? 280 : 140, 
							gameStates.menus.bHires ? 200 : 80, 0,
							movies.robot.bLittleEndian)) {
	Int3 ();
	return 0;
	}
m_robotP->m_pos = m_robotP->m_cf.Tell ();
#if TRACE
console.printf (DEBUG_LEVEL, "movies.robot.nFilePos=%d!\n", movies.robot.nFilePos);
#endif
return 1;
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
	int			i, len, bLittleEndian = gameStates.app.bLittleEndian;

	//read movie file header

nFiles = cf.ReadInt ();        //get number of files
if (nFiles > 255) {
	gameStates.app.bLittleEndian = 0;
	nFiles = SWAPINT (nFiles);
	}
if (!m_movies.Create (nFiles))
	return 0;
strcpy (m_name, filename);
m_nMovies = nFiles;
offset = 4 + 4 + nFiles * (13 + 4);	//id + nFiles + nFiles * (filename + size)
for (i = 0; i < nFiles; i++) {
	if (cf.Read (m_movies [i].name, 13, 1) != 1)
		break;		//end of file (probably)
	len = cf.ReadInt ();
	m_movies [i].len = len;
	m_movies [i].offset = offset;
	offset += m_movies [i].len;
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
	int	size = cf.m_cf.size;
	int	nFiles = 0;
	int	h;
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
	cf.Read (m_movies [i].name, 13, 1);
	m_movies [i].len = cf.ReadInt ();
	m_movies [i].offset = cf.Tell ();
	cf.Seek (m_movies [i].len, SEEK_CUR);       //skip data
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
	nFiles = SetupDF (cf);
	}
cf.Close ();
return nFiles > 0;
}

//-----------------------------------------------------------------------

CMovieLib* CMovieLib::Create (const char* filename)
{
	CMovieLib*	lib = new CMovieLib;

if (!lib)
	return NULL;
if (lib->Setup (filename))
	return lib;
delete lib;
return NULL;
}

//-----------------------------------------------------------------------

//looks through a movie library for a movie file
//returns filehandle, with fileposition at movie, or -1 if can't find
CMovie* CMovieLib::Open (char* filename, int bRequired)
{
	int i, bFromCD;

for (i = 0; i < m_nMovies; i++)
	if (!stricmp (filename, m_movies [i].name)) {	//found the movie in a library 
		if ((bFromCD = (m_flags & MLF_ON_CD)))
			SongsStopRedbook ();		//ready to read from CD
		do {		//keep trying until we get the file handle
			m_movies [i].m_cf.Open (m_name, gameFolders.szMovieDir, "rb", 0);
			if (bRequired && bFromCD && !m_movies [i].m_cf.File ()) {   //didn't get file!
				if (RequestCD () == -1)		//ESC from requester
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

	ret = ExecMessageBox ("CD ERROR", 1, "Ok", "Please insert your Descent II CD");

	if (ret == -1) {
		int ret2;

		ret2 = ExecMessageBox ("CD ERROR", 2, "Try Again", "Leave Game", "You must insert your\nDescent II CD to Continue");

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
bHires = bRobotMovie ? gameStates.menus.bHipszResAvailable : gameOpts->movies.bHipszRes;
if (bHires)
	*pszRes = 'h';
for (nTries = 0; !movies.libs [nLibrary].Setup (filename); nTries++) {
	strcpy (cdName, CDROM_dir);
	strcat (cdName, filename);
	if (movies.libs [nLibrary].Setup (cdName)) {
		movies.libs [nLibrary]->m_flags |= MLF_ON_CD;
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

if (bRobotMovie && movies.m_libs [nLibrary].m_nMovies)
	m_nRobots = bHires ? 2 : 1;
}

//-----------------------------------------------------------------------

//find and initialize the movie libraries
void CMovieManager::InitLibs (void)
{
	int bRobotMovie;

m_nLibs = 0;

int j = (bHaveExtras = !gameStates.app.bNostalgia) 
		  ? N_BUILTIN_MOVIE_LIBS 
		  : FIRST_EXTRA_MOVIE_LIB;

for (int i = 0; i < N_BUILTIN_MOVIE_LIBS; i++) {
	bRobotMovie = !strnicmp (pszMovieLibs [i], "robot", 5);
	InitLib (pszMovieLibs [i], m_nLibs, bRobotMovie, 1);
	if (m_movies.libs [m_nLibs].m_nMovies) {
		m_nLibs++;
		PrintLog ("   found movie lib '%s'\n", pszMovieLibs [i]);
		}
	else if ((i >= FIRST_EXTRA_MOVIE_LIB) && 
				(i < FIRST_EXTRA_MOVIE_LIB + N_EXTRA_MOVIE_LIBS))
		m_bHaveExtras = 0;
	}

m_movies.libs [EXTRA_ROBOT_LIB].m_nMovies = 0;
bMoviesInited = 1;
}

//-----------------------------------------------------------------------

void CMovieManager::InitExtraRobotLib (char* filename)
{
m_libs [EXTRA_ROBOT_LIB].Destroy ();
m_libs [EXTRA_ROBOT_LIB].Setup (filename, 1, 0);
}

//-----------------------------------------------------------------------
//returns file handle
CMovie* CMovieManager::Open (char* filename, int bRequired)
{
	CMovie*	movieP;

for (int i = 0; i < m_nLibs; i++)
	if ((movieP = movies.libs [i].Open (cf, filename, bRequired)))
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
DigiExit ();
// Start sound
MVE_sndInit (gameStates.app.bUseSound ? 1 : -1);
gameOpts->movies.bFullScreen = bFullScreen;
ret = Run (name, gameOpts->movies.bHires, bRequired, -1, -1);
DigiInit (1);
gameStates.video.nScreenMode = -1;		//force screen reset
return ret;
}

//-----------------------------------------------------------------------
//returns status.  see movie.h
int CMovieManager::Run (char* filename, int bHires, int bRequired, int dx, int dy)
{
	CFile			cf;
	CMovie*		movieP;
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
if (MVE_rmPrepMovie (reinterpret_cast<void*> (&cf), dx, dy, track, libP ? libP->bLittleEndian : 1)) {
	Int3 ();
	return MOVIE_NOT_PLAYED;
	}
nFrame = 0;
gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && bHires;
SetRenderQuality (0);
while ((result = MVE_rmStepMovie ()) == 0) {
	DrawSubTitles (nFrame);
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
cf.Close ();                           // Close Movie File
// Restore old graphic state
gameStates.video.nScreenMode = -1;  //force reset of screen mode
paletteManager.LoadEffect ();
return (aborted ? MOVIE_ABORTED : MOVIE_PLAYED_FULL);
}

//-----------------------------------------------------------------------

char* CMovieManager::Cycle (int bRestart, int bPlayMovie)
{
	char* pszMovieName;

if (m_nLibs < 0) {
	InitMovies ();
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
			PlayMovie (pszMovieName, 1, 1, gameOpts->movies.bResize);
		}
	return pszMovieName;
	//CloseSubTitles ();
	}
return NULL;
}

//-----------------------------------------------------------------------

CMovieLib* CMovieManager::FindLib (const char* pszLib)
{
	int i, j, nMovies;

if (m_nLibs < 0)
	InitMovies ();
if (m_nLibs) {
	for (i = 0; i < m_nLibs; i++) {
		if (!strcmp (pszLib, m_libs [i].m_name))
			return m_libs + i;
return NULL;
}

//-----------------------------------------------------------------------

CMovieLib* CMovieManager::Find (const char* pszMovie)
{
	int i, j, nMovies;
	char* pszMovieName;

if (m_nLibs < 0)
	InitMovies ();
if (m_nLibs) {
	for (i = 0; i < m_nLibs; i++) {
		for (j = 0; j < m_libs [i].m_nMovies; j++) {
			if (!strcmp (pszMovie, m_libs [i].m_movies [j].m_name))
				return movies.libs + i;
			}
		}
	}
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

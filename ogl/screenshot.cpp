/*
 *
 * OGL video functions. - Added 9/15/99 Matthew Mueller
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#endif

#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

#include "descent.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "ogl_texcache.h"
#include "sdlgl.h"

#include "hudmsgs.h"
#include "game.h"
#include "text.h"
#include "gr.h"
#include "gamefont.h"
#include "grdef.h"
#include "palette.h"
#include "u_mem.h"
#include "error.h"

#include "descent.h"
#include "screens.h"

#include "strutil.h"
#include "mono.h"
#include "args.h"
#include "key.h"
#include "cockpit.h"
#include "rendermine.h"
#include "particles.h"
#include "glare.h"
#include "menu.h"

extern int screenShotIntervals [];

//------------------------------------------------------------------------------

//writes out an uncompressed RGB .tga file
//if we got really spiffy, we could optionally link in libpng or something, and use that.
void WriteScreenShot (char *szSaveName, int w, int h, ubyte *buf, int nFormat)
{
	FILE *f = fopen (szSaveName, "wb");
if (!f) {
#if TRACE
	console.printf (CON_DBG,"screenshot error, couldn't open %s (err %i)\n",szSaveName,errno);
#endif
	}
else {
	static tBGR* outBuf = NULL;
	static int outBufSize = 0;

	int imgSize = w * h * sizeof (tBGR);

	if (outBufSize < imgSize) {
		outBufSize = imgSize;
		if (outBuf)
			delete outBuf;
		outBuf = new tBGR [outBufSize];
		}
	if (outBuf) {
		tTGAHeader hdr;
		memset (&hdr, 0, sizeof (hdr));
#ifdef _WIN32
		hdr.imageType = 2;
		hdr.width = w;
		hdr.height = h;
		hdr.bits = 24;
		//write .TGA header.
		fwrite (&hdr, sizeof (hdr), 1, f);
#else	// if only I knew how to control struct member packing with gcc or XCode ... ^_^
		*(reinterpret_cast<char*> (&hdr) + 2) = 2;
		*reinterpret_cast<short*> (reinterpret_cast<char*> (&hdr) + 12) = w;
		*reinterpret_cast<short*> (reinterpret_cast<char*> (&hdr) + 14) = h;
		*(reinterpret_cast<char*> (&hdr) + 16) = 24;
		//write .TGA header.
		fwrite (&hdr, 18, 1, f);
#endif
		tRGB* rgbP = ((tRGB *) buf);// + w * (h - 1);
		tBGR* bgrP = outBuf;
		for (int i = h; i; i--) {
			for (int j = w; j; j--, rgbP++, bgrP++) {
				bgrP->r = rgbP->r;
				bgrP->g = rgbP->g;
				bgrP->b = rgbP->b;
				}
			}
		fwrite (outBuf, imgSize, 1, f);
		}
	fclose (f);
	}
}

//------------------------------------------------------------------------------

#ifdef _WIN32
#  define   access   _access
#endif

extern char *pszSystemNames [];

void SaveScreenShot (ubyte *buf, int bAutomap)
{
	char				szMessage [1000];
	char				szSaveName [FILENAME_LEN], szLevelName [FILENAME_LEN];
	int				i, j, bTmpBuf;
	static int		nSaveNum = 0;
	GLenum			glErrCode;

if (!gameStates.app.bSaveScreenShot)
	return;
gameStates.app.bSaveScreenShot = 0;
// only use alpha-numeric characters and underscores from the level name
for (i = j = 0; missionManager.szCurrentLevel [i]; i++)
	if (isalnum (missionManager.szCurrentLevel [i]))
		szLevelName [j++] = missionManager.szCurrentLevel [i];
	else if ((missionManager.szCurrentLevel [i] == ' ') && j && (szLevelName [j - 1] != '_'))
		szLevelName [j++] = '_';
while (j && (szLevelName [j - 1] == '_'))
	j--;
if (j) {
	szLevelName [j++] = '-';
	szLevelName [j] = '\0';
	}
else
	strcpy (szLevelName, "scrn");
StopTime();
if (*gameFolders.user.szScreenshots)
	sprintf (szSaveName, "%s", gameFolders.user.szScreenshots);
else
	*szSaveName = '\0';
i = (int) strlen (szSaveName);
do {
	sprintf (szSaveName + i, "/%s%04d.tga", szLevelName, nSaveNum++);
	nSaveNum %= 9999;
	} while (!access (szSaveName, 0));

if ((bTmpBuf = (buf == NULL))) {
	buf = new ubyte [gameData.render.screen.Width () * gameData.render.screen.Height () * 3];
	ogl.SetTexturing (false);
	ogl.SetReadBuffer (GL_FRONT, 0);
	gameData.render.screen.Activate ("SaveScreenShot (screen)");
	glReadPixels (0, 0, gameData.render.screen.Width (), gameData.render.screen.Height (), GL_RGB, GL_UNSIGNED_BYTE, buf);
	gameData.render.screen.Deactivate ();
	glErrCode = glGetError ();
	glErrCode = GL_NO_ERROR;
	}
else
	glErrCode = GL_NO_ERROR;
if (glErrCode == GL_NO_ERROR) {
	WriteScreenShot (szSaveName, gameData.render.screen.Width (), gameData.render.screen.Height (), buf, 0);
	if (!(bAutomap || screenShotIntervals [gameOpts->app.nScreenShotInterval])) {
		sprintf (szMessage, "%s '%s'", TXT_DUMPING_SCREEN, szSaveName);
		HUDMessage (MSGC_GAME_FEEDBACK, szMessage);
		}
	}
if (bTmpBuf)
	delete[] buf;
//KeyFlush ();
StartTime (0);
}

//-----------------------------------------------------------------------------

int screenShotIntervals [] = {0, 1, 3, 5, 10, 15, 30, MAX_FRAMERATE};

void AutoScreenshot (void)
{
	int	h;

	static	time_t	t0 = 0;

if (gameData.app.bGamePaused || gameStates.menus.nInMenu)
	return;
if (!(h = screenShotIntervals [gameOpts->app.nScreenShotInterval]))
	return;
if (gameStates.app.nSDLTicks [0] - t0 < h * 1000)
	return;
t0 = gameStates.app.nSDLTicks [0];
gameStates.app.bSaveScreenShot = 1;
//SaveScreenShot (0, 0);
}

//------------------------------------------------------------------------------

// mac routine to drop contents of screen to a pict file using copybits
// save a PICT to a file
#ifdef MACINTOSH

void SavePictScreen (int multiplayer)
{
	OSErr err;
	int parid, i, count;
	char *pfilename, filename [50], buf[512], cwd[FILENAME_MAX];
	short fd;
	FSSpec spec;
	PicHandle pict_handle;
	static int multiCount = 0;
	StandardFileReply sf_reply;

// dump the contents of the GameWindow into a picture using copybits

	pict_handle = OpenPicture (&GameWindow->portRect);
	if (pict_handle == NULL)
		return;

	CopyBits (&GameWindow->portBits, &GameWindow->portBits, &GameWindow->portRect, &GameWindow->portRect, srcBic, NULL);
	ClosePicture ();

// get the cwd to restore with chdir when done -- this keeps the mac world sane
	if (!getcwd (cwd, FILENAME_MAX))
		Int3 ();
// create the fsspec

	sprintf (filename, "screen%d", multiCount++);
	pfilename = c2pstr (filename);
	if (!multiplayer) {
		show_cursor ();
		StandardPutFile ("\pSave PICT as:", pfilename, &sf_reply);
		if (!sf_reply.sfGood)
			goto end;
		memcpy (&spec, & (sf_reply.sfFile), sizeof (FSSpec));
		if (sf_reply.sfReplacing)
			FSpDelete (&spec);
		err = FSpCreate (&spec, 'ttxt', 'PICT', smSystemScript);
		if (err)
			goto end;
	} else {
//		parid = GetAppDirId ();
		err = FSMakeFSSpec (0, 0, pfilename, &spec);
		if (err == nsvErr)
			goto end;
		if (err != fnfErr)
			FSpDelete (&spec);
		err = FSpCreate (&spec, 'ttxt', 'PICT', smSystemScript);
		if (err != 0)
			goto end;
	}

// write the PICT file
	if (FSpOpenDF (&spec, fsRdWrPerm, &fd))
		goto end;
	memset (buf, 0, sizeof (buf);
	count = 512;
	if (FSWrite (fd, &count, buf))
		goto end;
	count = GetHandleSize ((Handle)pict_handle);
	HLock ((Handle)pict_handle);
	if (FSWrite (fd, &count, *pict_handle)) {
		FSClose (fd);
		FSpDelete (&spec);
	}

end:
	HUnlock ((Handle)pict_handle);
	DisposeHandle ((Handle)pict_handle);
	FSClose (fd);
	hide_cursor ();
	chdir (cwd);
}

#endif

//------------------------------------------------------------------------------


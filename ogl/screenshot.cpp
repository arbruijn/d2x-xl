/* $Id: gr.c,v 1.16 2003/11/27 04:59:49 btb Exp $ */
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

#include "inferno.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "ogl_texcache.h"
#include "sdlgl.h"

#include "hudmsg.h"
#include "game.h"
#include "text.h"
#include "gr.h"
#include "gamefont.h"
#include "grdef.h"
#include "palette.h"
#include "u_mem.h"
#include "error.h"

#include "inferno.h"
#include "screens.h"

#include "strutil.h"
#include "mono.h"
#include "args.h"
#include "key.h"
#include "gauges.h"
#include "render.h"
#include "particles.h"
#include "glare.h"
#include "newmenu.h"

extern int screenShotIntervals [];

//------------------------------------------------------------------------------

//writes out an uncompressed RGB .tga file
//if we got really spiffy, we could optionally link in libpng or something, and use that.
void WriteScreenShot (char *szSaveName, int w, int h, unsigned char *buf, int nFormat)
{
	FILE *f = fopen (szSaveName, "wb");
if (!f) {
#if TRACE
	con_printf (CONDBG,"screenshot error, couldn't open %s (err %i)\n",szSaveName,errno);
#endif
	}
else {
		tTgaHeader	hdr;
		int			i, j, r;
		tBGR			*outBuf = (tBGR *) D2_ALLOC (w * h * sizeof (tBGR));
		tBGR			*bgrP;
		tRGB			*rgbP;

	memset (&hdr, 0, sizeof (hdr));
#ifdef _WIN32
	hdr.imageType = 2;
	hdr.width = w;
	hdr.height = h;
	hdr.bits = 24;
	//write .TGA header.
	fwrite (&hdr, sizeof (hdr), 1, f);
#else	// if only I knew how to control struct member packing with gcc or XCode ... ^_^
	*(((char *) &hdr) + 2) = 2;
	*((short *) (((char *) &hdr) + 12)) = w;
	*((short *) (((char *) &hdr) + 14)) = h;
	*(((char *) &hdr) + 16) = 24;
	//write .TGA header.
	fwrite (&hdr, 18, 1, f);
#endif
	rgbP = ((tRGB *) buf);// + w * (h - 1);
	bgrP = outBuf;
	for (i = h; i; i--) {
		for (j = w; j; j--, rgbP++, bgrP++) {
			bgrP->r = rgbP->r;
			bgrP->g = rgbP->g;
			bgrP->b = rgbP->b;
			}
		}
	r = (int) fwrite (outBuf, w * h * 3, 1, f);
#if TRACE
	if (r <= 0)
		con_printf (CONDBG,"screenshot error, couldn't write to %s (err %i)\n",szSaveName,errno);
#endif
	fclose (f);
	}
}

//------------------------------------------------------------------------------

#ifdef _WIN32
#  define   access   _access
#endif

extern int bSaveScreenShot;
extern char *pszSystemNames [];

void SaveScreenShot (unsigned char *buf, int bAutomap)
{
	char				szMessage [100];
	char				szSaveName [FILENAME_LEN], szLevelName [128];
	int				i, j, bTmpBuf;
	static int		nSaveNum = 0;
	GLenum			glErrCode;

if (!bSaveScreenShot)
	return;
bSaveScreenShot = 0;
if (!gameStates.ogl.bReadPixels) {
	if (!bAutomap)
		HUDMessage (MSGC_GAME_FEEDBACK, "Screenshots not supported on your configuration");
	return;
	}
for (i = j = 0; gameData.missions.szCurrentLevel [i]; i++)
	if (isalnum (gameData.missions.szCurrentLevel [i]))
		szLevelName [j++] = gameData.missions.szCurrentLevel [i];
	else if ((gameData.missions.szCurrentLevel [i] == ' ') && j && (szLevelName [j - 1] != '_'))
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
if (*gameFolders.szScrShotDir)
	sprintf (szSaveName, "%s", gameFolders.szScrShotDir);
else
	*szSaveName = '\0';
i = (int) strlen (szSaveName);
do {
	sprintf (szSaveName + i, "/%s%04d.tga", szLevelName, nSaveNum++);
	nSaveNum %= 9999;
	} while (!access (szSaveName, 0));

if ((bTmpBuf = (buf == NULL))) {
	buf = (unsigned char *) D2_ALLOC (grdCurScreen->scWidth * grdCurScreen->scHeight * 3);
	glDisable (GL_TEXTURE_2D);
	OglReadBuffer (GL_FRONT, 1);
	glReadPixels (0, 0, grdCurScreen->scWidth, grdCurScreen->scHeight, GL_RGB, GL_UNSIGNED_BYTE, buf);
	glErrCode = glGetError ();
	glErrCode = GL_NO_ERROR;
	}
else
	glErrCode = GL_NO_ERROR;
if (glErrCode == GL_NO_ERROR) {
	WriteScreenShot (szSaveName, grdCurScreen->scWidth, grdCurScreen->scHeight, buf, 0);
	if (!(bAutomap || screenShotIntervals [gameOpts->app.nScreenShotInterval])) {
		sprintf (szMessage, "%s '%s'", TXT_DUMPING_SCREEN, szSaveName);
		HUDMessage (MSGC_GAME_FEEDBACK, szMessage);
		}
	}
if (bTmpBuf)
	D2_FREE (buf);
//KeyFlush ();
StartTime (0);
}

//------------------------------------------------------------------------------


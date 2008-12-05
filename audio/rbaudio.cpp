/*
 *
 * SDL CD Audio functions
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>

#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

#ifdef __linux__
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#endif

#include "inferno.h"
#include "strutil.h"
#include "error.h"
#include "args.h"
#include "rbaudio.h"
#include "text.h"

static SDL_CD *s_cd = NULL;
static int initialised = 0;

void _CDECL_ RBAExit(void)
{
PrintLog ("shutting down SDL CD service\n");
if (initialised) {
	SDL_CDStop(s_cd);
	SDL_CDClose(s_cd);
	}
}

void RBAInit()
{
	int	d, i, j;
	char	szDrive [FILENAME_LEN], sz [FILENAME_LEN];

if (initialised) 
	return;
if (FindArg("-nocdrom")) 
	return; 

if (SDL_Init(SDL_INIT_CDROM) < 0) {
	Warning(TXT_SDL_INIT_LIB,SDL_GetError());
	return;
	}

if ((j = SDL_CDNumDrives()) == 0) {
	Warning(TXT_CDROM_NONE);
	return;
	}

d = 0;
if ((i = FindArg ("-cdrom")) && *pszArgList [++i]) {
#ifdef _WIN32
	sprintf (szDrive, "%c:\\", *pszArgList [i]);
#else
	strncpy (szDrive, pszArgList [i], sizeof (szDrive));
#endif
	strlwr (szDrive);
	for (i = 0; i < j; i++) {
		strcpy (sz, SDL_CDName (i));
		strlwr (sz);
		if (!strcmp (szDrive, sz)) {
			d = i;
			break;
			}
		}
	}

if (!(s_cd = SDL_CDOpen(d))) {
	Warning(TXT_CDROM_OPEN);
	return;
	}
atexit(RBAExit);
initialised = 1;
}

int RBAEnabled()
{
	return 1;
}

void RBARegisterCD()
{

}

int RBAPlayTrack(int a)
{
	if (!initialised) return -1;

	if (CD_INDRIVE(SDL_CDStatus(s_cd)) ) {
		SDL_CDPlayTracks(s_cd, a-1, 0, 0, 0);
	}
	return a;
}

void _CDECL_ RBAStop()
{
	if (!initialised) return;
	SDL_CDStop(s_cd);
}

void RBASetVolume(int volume)
{
#ifdef __linux__
	int cdfile, level;
	struct cdrom_volctrl volctrl;

	if (!initialised) return;

	cdfile = s_cd->id;
	level = volume * 3;

	if ((level<0) || (level>255)) {
#ifndef WIN32
		fprintf(stderr, "illegal volume value (allowed values 0-255)\n");
#endif
		return;
	}

	volctrl.channel0
		= volctrl.channel1
		= volctrl.channel2
		= volctrl.channel3
		= level;
	if ( ioctl(cdfile, CDROMVOLCTRL, &volctrl) == -1 ) {
#ifndef WIN32
		fprintf(stderr, "CDROMVOLCTRL ioctl failed\n");
#endif
		return;
	}
#endif
}

void RBAPause()
{
	if (!initialised) 
		return;
	SDL_CDPause(s_cd);
}

int RBAResume()
{
	if (!initialised) 
		return -1;
	SDL_CDResume(s_cd);
	return 1;
}

int RBAGetNumberOfTracks()
{
	if (!initialised) 
		return -1;
	SDL_CDStatus(s_cd);
	return s_cd->numtracks;
}

// plays tracks first through last, inclusive
int RBAPlayTracks(int first, int last)
{
if (!initialised)
	return 0;
if (!CD_INDRIVE(SDL_CDStatus(s_cd)))
	return 0;
if (0 > SDL_CDPlayTracks(s_cd, first - 1, 0, last - first + 1, 0))
	return 0;
return 1;
}

// return the track number currently playing.  Useful if RBAPlayTracks()
// is called.  Returns 0 if no track playing, else track number
int RBAGetTrackNum()
{
	if (!initialised)
		return 0;

	if (SDL_CDStatus(s_cd) != CD_PLAYING)
		return 0;

	return s_cd->cur_track + 1;
}

int RBAPeekPlayStatus()
{
	return (SDL_CDStatus(s_cd) == CD_PLAYING);
}

int CD_blast_mixer()
{
	return 0;
}


static int cddb_sum(int n)
{
	int ret;

	/* For backward compatibility this algorithm must not change */

	ret = 0;

	while (n > 0) {
		ret = ret + (n % 10);
		n = n / 10;
	}

	return (ret);
}


uint RBAGetDiscID()
{
	int i, t = 0, n = 0;

	if (!initialised)
		return 0;

	/* For backward compatibility this algorithm must not change */

	i = 0;

	while (i < s_cd->numtracks) {
		n += cddb_sum(s_cd->track[i].offset / CD_FPS);
		i++;
	}

	t = (s_cd->track[s_cd->numtracks].offset / CD_FPS) -
	    (s_cd->track[0].offset / CD_FPS);

	return ((n % 0xff) << 24 | t << 8 | s_cd->numtracks);
}

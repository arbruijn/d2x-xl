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

#ifdef __linux__
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#endif

#include "descent.h"
#include "strutil.h"
#include "error.h"
#include "args.h"
#include "rbaudio.h"
#include "text.h"

CRBA rba;

//------------------------------------------------------------------------------

CRBA::CRBA ()
{
m_cdInfo = NULL;
m_bInitialized = 0;
}

//------------------------------------------------------------------------------

CRBA::~CRBA ()
{
Destroy ();
}

//------------------------------------------------------------------------------

void CRBA::Destroy (void)
{
if (m_bInitialized) {
	PrintLog (1, "shutting down SDL CD service\n");
	SDL_CDStop (m_cdInfo);
	SDL_CDClose (m_cdInfo);
	m_cdInfo = NULL;
	m_bInitialized = 0;
	PrintLog (-1);
	}
}

//------------------------------------------------------------------------------

void CRBA::Init (void)
{
	int	d, i, j;
	char	szDrive [FILENAME_LEN], sz [FILENAME_LEN];

if (m_bInitialized) 
	return;
if (FindArg ("-nocdrom")) 
	return; 

if (SDL_Init (SDL_INIT_CDROM) < 0) {
	Warning (TXT_SDL_INIT_LIB,SDL_GetError ());
	return;
	}

if ((j = SDL_CDNumDrives ()) == 0) {
	Warning (TXT_CDROM_NONE);
	return;
	}

d = 0;
if ((i = FindArg ("-cdrom")) && *appConfig [++i]) {
#ifdef _WIN32
	sprintf (szDrive, "%c:\\", *appConfig [i]);
#else
	strncpy (szDrive, appConfig [i], sizeof (szDrive));
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

if (! (m_cdInfo = SDL_CDOpen (d))) {
	Warning (TXT_CDROM_OPEN);
	return;
	}
m_bInitialized = 1;
}

//------------------------------------------------------------------------------

int CRBA::Enabled (void)
{
return 1;
}

//------------------------------------------------------------------------------

void CRBA::RegisterCD (void)
{
}

//------------------------------------------------------------------------------

int CRBA::PlayTrack (int a)
{
if (!m_bInitialized) 
	return -1;
if (CD_INDRIVE (SDL_CDStatus (m_cdInfo))) {
	SDL_CDPlayTracks (m_cdInfo, a - 1, 0, 0, 0);
	}
return a;
}

//------------------------------------------------------------------------------

void _CDECL_ CRBA::Stop (void)
{
if (m_bInitialized) 
	SDL_CDStop (m_cdInfo);
}

//------------------------------------------------------------------------------

void CRBA::SetVolume (int volume)
{
#ifdef __linux__
	int cdfile, level;
	struct cdrom_volctrl volctrl;

if (!m_bInitialized) 
	return;

cdfile = m_cdInfo->id;
level = volume * 3;

if ((level<0) || (level>255)) {
#ifndef _WIN32
	fprintf (stderr, "illegal volume value (allowed values 0-255)\n");
#endif
	return;
	}

volctrl.channel0 =
volctrl.channel1 =
volctrl.channel2 =
volctrl.channel3 = level;
if (ioctl (cdfile, CDROMVOLCTRL, &volctrl) == -1) {
#ifndef _WIN32
	fprintf (stderr, "CDROMVOLCTRL ioctl failed\n");
#endif
	return;
	}
#endif
}

//------------------------------------------------------------------------------

void CRBA::Pause (void)
{
if (m_bInitialized) 
	SDL_CDPause (m_cdInfo);
}

//------------------------------------------------------------------------------

int CRBA::Resume (void)
{
if (!m_bInitialized) 
	return -1;
SDL_CDResume (m_cdInfo);
return 1;
}

//------------------------------------------------------------------------------

int CRBA::GetNumberOfTracks (void)
{
if (!m_bInitialized) 
	return -1;
SDL_CDStatus (m_cdInfo);
return m_cdInfo->numtracks;
}

//------------------------------------------------------------------------------
// plays tracks first through last, inclusive
int CRBA::PlayTracks (int first, int last)
{
if (!m_bInitialized)
	return 0;
if (!CD_INDRIVE (SDL_CDStatus (m_cdInfo)))
	return 0;
if (0 > SDL_CDPlayTracks (m_cdInfo, first - 1, 0, last - first + 1, 0))
	return 0;
return 1;
}

//------------------------------------------------------------------------------
// return the track number currently playing.  Useful if CRBA::PlayTracks (void)
// is called.  Returns 0 if no track playing, else track number
int CRBA::GetTrackNum (void)
{
if (!m_bInitialized)
	return 0;
if (SDL_CDStatus (m_cdInfo) != CD_PLAYING)
	return 0;
return m_cdInfo->cur_track + 1;
}

//------------------------------------------------------------------------------

int CRBA::PeekPlayStatus (void)
{
return (SDL_CDStatus (m_cdInfo) == CD_PLAYING);
}

//------------------------------------------------------------------------------

int CD_blast_mixer (void)
{
return 0;
}

//------------------------------------------------------------------------------

int CRBA::cddb_sum (int n)
{
	int ret = 0;

// For backward compatibility this algorithm must not change
while (n > 0) {
	ret = ret + (n % 10);
	n = n / 10;
	}
return (ret);
}

//------------------------------------------------------------------------------

uint CRBA::GetDiscID (void)
{
	int i, t = 0, n = 0;

if (!m_bInitialized)
	return 0;
/* For backward compatibility this algorithm must not change */
i = 0;
while (i < m_cdInfo->numtracks) {
	n += cddb_sum (m_cdInfo->track[i].offset / CD_FPS);
	i++;
	}
t = (m_cdInfo->track [m_cdInfo->numtracks].offset / CD_FPS) - (m_cdInfo->track [0].offset / CD_FPS);
return (((n % 0xff) << 24) | (t << 8) | m_cdInfo->numtracks);
}

//------------------------------------------------------------------------------
//eof

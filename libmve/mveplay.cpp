/* $Id: mveplay.c,v 1.17 2003/11/26 12:26:28 btb Exp $ */
#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#define AUDIO
//#define DEBUG

#include <string.h>
#ifndef _WIN32
#	include <errno.h>
#	include <time.h>
#	include <sys/time.h>
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <fcntl.h>
#	include <unistd.h>
#else
#	include <windows.h>
#endif

#if defined (AUDIO)
#	ifdef __macosx__
#		include <SDL/SDL.h>
#	else
#		include <SDL.h>
#	endif
#endif

#include "mvelib.h"
#include "mve_audio.h"
#include "decoders.h"
#include "libmve.h"
#include "maths.h"
#include "pstypes.h"

#define MVE_OPCODE_ENDOFSTREAM          0x00
#define MVE_OPCODE_ENDOFCHUNK           0x01
#define MVE_OPCODE_CREATETIMER          0x02
#define MVE_OPCODE_INITAUDIOBUFFERS     0x03
#define MVE_OPCODE_STARTSTOPAUDIO       0x04
#define MVE_OPCODE_INITVIDEOBUFFERS     0x05

#define MVE_OPCODE_DISPLAYVIDEO         0x07
#define MVE_OPCODE_AUDIOFRAMEDATA       0x08
#define MVE_OPCODE_AUDIOFRAMESILENCE    0x09
#define MVE_OPCODE_INITVIDEOMODE        0x0A

#define MVE_OPCODE_SETPALETTE           0x0C
#define MVE_OPCODE_SETPALETTECOMPRESSED 0x0D

#define MVE_OPCODE_SETDECODINGMAP       0x0F

#define MVE_OPCODE_VIDEODATA            0x11

#define MVE_AUDIO_FLAGS_STEREO     1
#define MVE_AUDIO_FLAGS_16BIT      2
#define MVE_AUDIO_FLAGS_COMPRESSED 4

int g_spdFactorNum=0;
static int g_spdFactorDenom=10;
static int g_frameUpdated = 0;

//-----------------------------------------------------------------------

static short get_short (ubyte *data)
{
	short value = data[0] | (data[1] << 8);

return value;
}

//-----------------------------------------------------------------------

static ushort get_ushort (ubyte *data)
{
	ushort value = data[0] | (data[1] << 8);

return value;
}

//-----------------------------------------------------------------------

static int get_int (ubyte *data)
{
	int value = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);

return value;
}

//-----------------------------------------------------------------------

static uint unhandled_chunks[32*256];

static int default_seg_handler (ubyte major, ubyte minor, ubyte *data, int len, void *context)
{
unhandled_chunks[major<<8|minor]++;
//fprintf (stderr, "unknown chunk nType %02x/%02x\n", major, minor);
return 1;
}


/*************************
 * general handlers
 *************************/

static int end_movie_handler (ubyte major, ubyte minor, ubyte *data, int len, void *context)
{
return 0;
}

/*************************
 * timer handlers
 *************************/

#ifdef _WIN32_WCE
struct timeval
{
	long tv_sec;
	long tv_usec;
};
#endif

/*
 * timer variables
 */
static int timer_created = 0;
static int micro_frame_delay=0;
static int timer_started=0;
static struct timeval timer_expire = {0, 0};

#if !HAVE_STRUCT_TIMESPEC
struct timespec
{
	long int tv_sec;            /* Seconds.  */
	long int tv_nsec;           /* Nanoseconds.  */
};
#endif

//-----------------------------------------------------------------------

#ifdef _WIN32
int gettimeofday (struct timeval *tv, void *tz)
{
	static int counter = 0;
	DWORD now;

counter++;
now = GetTickCount ();
tv->tv_sec = now / 1000;
tv->tv_usec = (now % 1000) * 1000 + counter;
return 0;
}
#endif


//-----------------------------------------------------------------------

static int createTimer_handler (ubyte major, ubyte minor, ubyte *data, int len, void *context)
{
#ifndef _WIN32 //FIXME
	__extension__ long long temp;
#else
	QLONG temp;
#endif

if (timer_created)
	return 1;
timer_created = 1;
micro_frame_delay = get_int (data) * (int)get_short (data+4);
if (g_spdFactorNum != 0) {
	temp = micro_frame_delay;
	temp *= g_spdFactorNum;
	temp /= g_spdFactorDenom;
	micro_frame_delay = (int)temp;
	}
return 1;
}

//-----------------------------------------------------------------------

static void timer_stop (void)
{
timer_expire.tv_sec = 0;
timer_expire.tv_usec = 0;
timer_started = 0;
}

//-----------------------------------------------------------------------

static void timer_start (void)
{
	int nsec=0;

gettimeofday (&timer_expire, NULL);
timer_expire.tv_usec += micro_frame_delay;
if (timer_expire.tv_usec > 1000000) {
	nsec = timer_expire.tv_usec / 1000000;
	timer_expire.tv_sec += nsec;
	timer_expire.tv_usec -= nsec*1000000;
	}
timer_started=1;
}

//-----------------------------------------------------------------------

static void doTimer_wait (void)
{
	int nsec=0;
	struct timespec ts;
	struct timeval tv;
	if (! timer_started)
		return;

gettimeofday (&tv, NULL);
if (tv.tv_sec > timer_expire.tv_sec)
	goto end;
else if (tv.tv_sec == timer_expire.tv_sec  &&  tv.tv_usec >= timer_expire.tv_usec)
	goto end;
ts.tv_sec = timer_expire.tv_sec - tv.tv_sec;
ts.tv_nsec = 1000 * (timer_expire.tv_usec - tv.tv_usec);
if (ts.tv_nsec < 0) {
	ts.tv_nsec += 1000000000UL;
	--ts.tv_sec;
	}
#ifdef _WIN32
Sleep (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#else
if (nanosleep (&ts, NULL) == -1  &&  errno == EINTR)
	exit (1);
#endif
end:

timer_expire.tv_usec += micro_frame_delay;
if (timer_expire.tv_usec > 1000000) {
	nsec = timer_expire.tv_usec / 1000000;
	timer_expire.tv_sec += nsec;
	timer_expire.tv_usec -= nsec*1000000;
	}
}

/*************************
 * audio handlers
 *************************/
#ifdef AUDIO
#define TOTAL_AUDIO_BUFFERS 64

static int audiobuf_created = 0;
static void _CDECL_ mve_audio_callback (void *userdata, ubyte *stream, int len);
static short *mve_audio_buffers[TOTAL_AUDIO_BUFFERS];
static int    mve_audio_buflens[TOTAL_AUDIO_BUFFERS];
static int    mve_audio_curbuf_curpos=0;
static int mve_audio_bufhead=0;
static int mve_audio_buftail=0;
static int mve_audio_playing=0;
static int mve_audio_canplay=0;
static int mve_audio_compressed=0;
static int mve_audio_enabled = 1;
static SDL_AudioSpec *mve_audio_spec=NULL;

//-----------------------------------------------------------------------

static void _CDECL_ mve_audio_callback (void *userdata, ubyte *stream, int len)
{
	int total=0;
	int length;

if (mve_audio_bufhead == mve_audio_buftail)
	return /* 0 */;
while (mve_audio_bufhead != mve_audio_buftail                                           /* while we have more buffers  */
		 &&  len > (mve_audio_buflens[mve_audio_bufhead]-mve_audio_curbuf_curpos)) {      /* and while we need more data */
	length = mve_audio_buflens[mve_audio_bufhead]-mve_audio_curbuf_curpos;
	memcpy (stream, /* cur output position */
		      reinterpret_cast<ubyte*> (mve_audio_buffers[mve_audio_bufhead])+mve_audio_curbuf_curpos, /* cur input position  */
		      length);                                                                 /* cur input length    */
	total += length;
	stream += length;                                                               /* advance output */
	len -= length;                                                                  /* decrement avail ospace */
	mve_free (mve_audio_buffers[mve_audio_bufhead]);                                 /* free the buffer */
	mve_audio_buffers[mve_audio_bufhead]=NULL;                                      /* free the buffer */
	mve_audio_buflens[mve_audio_bufhead]=0;                                         /* free the buffer */
	if (++mve_audio_bufhead == TOTAL_AUDIO_BUFFERS)                                 /* next buffer */
		mve_audio_bufhead = 0;
	mve_audio_curbuf_curpos = 0;
	}
if ((len > 0) && (mve_audio_bufhead != mve_audio_buftail)) {								/* ospace remaining && buffers remaining */
	memcpy (stream, /* dest */
			 reinterpret_cast<ubyte*> (mve_audio_buffers[mve_audio_bufhead]) + mve_audio_curbuf_curpos, /* src */
			len);                                                                    /* length */
	mve_audio_curbuf_curpos += len;                                                 /* advance input */
	stream += len;                                                                  /* advance output (unnecessary) */
	len -= len;                                                                     /* advance output (unnecessary) */
	if (mve_audio_curbuf_curpos >= mve_audio_buflens[mve_audio_bufhead]) {          /* if this ends the current chunk */
		mve_free (mve_audio_buffers[mve_audio_bufhead]);                             /* D2_FREE buffer */
		mve_audio_buffers[mve_audio_bufhead]=NULL;
		mve_audio_buflens[mve_audio_bufhead]=0;
		if (++mve_audio_bufhead == TOTAL_AUDIO_BUFFERS)                             /* next buffer */
			mve_audio_bufhead = 0;
		mve_audio_curbuf_curpos = 0;
		}
	}
}
#endif

//-----------------------------------------------------------------------

static int create_audiobuf_handler (ubyte major, ubyte minor, ubyte *data, int len, void *context)
{
#ifdef AUDIO
	int flags;
	int sample_rate;
	int desired_buffer;
	int stereo;
	int bitsize;
	int compressed;
	int format;

if (!mve_audio_enabled)
	return 1;
if (audiobuf_created)
	return 1;
audiobuf_created = 1;
flags = get_ushort (data + 2);
sample_rate = get_ushort (data + 4);
desired_buffer = get_int (data + 6);
stereo = (flags & MVE_AUDIO_FLAGS_STEREO) ? 1 : 0;
bitsize = (flags & MVE_AUDIO_FLAGS_16BIT) ? 1 : 0;
compressed = (minor > 0) ? (flags & MVE_AUDIO_FLAGS_COMPRESSED ? 1 : 0) : 0;
mve_audio_compressed = compressed;
if (bitsize == 1) {
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	format = AUDIO_S16MSB;
#else
	format = AUDIO_S16LSB;
#endif
	}
else
	format = AUDIO_U8;
mve_audio_spec = reinterpret_cast<SDL_AudioSpec*> (mve_alloc (sizeof (SDL_AudioSpec)));
mve_audio_spec->freq = sample_rate;
mve_audio_spec->format = format;
mve_audio_spec->channels = (stereo) ? 2 : 1;
mve_audio_spec->samples = 4096;
mve_audio_spec->callback = mve_audio_callback;
mve_audio_spec->userdata = NULL;
if (SDL_OpenAudio (mve_audio_spec, NULL) >= 0) {
#if 0
	fprintf (stderr, "   success\n");
#endif
	mve_audio_canplay = 1;
	}
else {
#if 0
	fprintf (stderr, "   failure : %s\n", SDL_GetError ();
#endif
	mve_audio_canplay = 0;
	}
memset (mve_audio_buffers, 0, sizeof (mve_audio_buffers));
memset (mve_audio_buflens, 0, sizeof (mve_audio_buflens));
#endif
return 1;
}

//-----------------------------------------------------------------------

static int play_audio_handler (ubyte major, ubyte minor, ubyte *data, int len, void *context)
{
#ifdef AUDIO
if (mve_audio_canplay  &&  !mve_audio_playing  &&  mve_audio_bufhead != mve_audio_buftail) {
	SDL_PauseAudio (0);
	mve_audio_playing = 1;
	}
#endif
return 1;
}

//-----------------------------------------------------------------------

static int audio_data_handler (ubyte major, ubyte minor, ubyte *data, int len, void *context)
{
#ifdef AUDIO
	static const int selected_chan=1;
	int chan;
	int nsamp;

if (mve_audio_canplay) {
	if (mve_audio_playing)
		SDL_LockAudio ();
	chan = get_ushort (data + 2);
	nsamp = get_ushort (data + 4);
	if (chan & selected_chan) {
		/* HACK: +4 mveaudio_uncompress adds 4 more bytes */
		if (major == MVE_OPCODE_AUDIOFRAMEDATA) {
			if (mve_audio_compressed) {
				nsamp += 4;

				mve_audio_buflens[mve_audio_buftail] = nsamp;
				mve_audio_buffers[mve_audio_buftail] = reinterpret_cast<short*> (mve_alloc (nsamp));
				mveaudio_uncompress (mve_audio_buffers[mve_audio_buftail], data, -1); /* XXX */
				}
			else {
				nsamp -= 8;
				data += 8;
				mve_audio_buflens[mve_audio_buftail] = nsamp;
				mve_audio_buffers[mve_audio_buftail] = reinterpret_cast<short*> (mve_alloc (nsamp));
				memcpy (mve_audio_buffers[mve_audio_buftail], data, nsamp);
				}
			}
		else {
			mve_audio_buflens[mve_audio_buftail] = nsamp;
			mve_audio_buffers[mve_audio_buftail] = reinterpret_cast<short*> (mve_alloc (nsamp));
			memset (mve_audio_buffers[mve_audio_buftail], 0, nsamp); /* XXX */
			}
		if (++mve_audio_buftail == TOTAL_AUDIO_BUFFERS)
			mve_audio_buftail = 0;
#ifndef WIN32
		if (mve_audio_buftail == mve_audio_bufhead)
			fprintf (stderr, "d'oh!  buffer ring overrun (%d)\n", mve_audio_bufhead);
#endif
		}
	if (mve_audio_playing)
		SDL_UnlockAudio ();
	}
#endif
return 1;
}

/*************************
 * video handlers
 *************************/

static int videobuf_created = 0;
static int video_initialized = 0;
int g_width, g_height, g_currBuf;
void *g_vBackBuf [2] = {NULL, NULL};

static int g_destX, g_destY;
static int g_screenWidth, g_screenHeight;
static ubyte *g_pCurMap=NULL;
static int g_nMapLength=0;
static int g_truecolor;

//-----------------------------------------------------------------------

static int create_videobuf_handler (ubyte major, ubyte minor, ubyte *data, int len, void *context)
{
	short w, h;
	short count, truecolor;

if (videobuf_created)
	return 1;
videobuf_created = 1;
w = get_short (data);
h = get_short (data+2);
count = (minor > 0) ? get_short (data+4) : 1;
truecolor = (minor > 1) ? get_short (data+6) : 0;
g_width = w << 3;
g_height = h << 3;
/* TODO: * 4 causes crashes on some files */
/* only D2_ALLOC once */
if (g_vBackBuf [0] == NULL)
	g_vBackBuf [0] = mve_alloc (g_width * g_height * 4);
if (g_vBackBuf [1] == NULL)
	g_vBackBuf [1] = mve_alloc (g_width * g_height * 4);
memset (g_vBackBuf [0], 0, g_width * g_height * 4);
memset (g_vBackBuf [1], 0, g_width * g_height * 4);
g_currBuf = 0;
#ifdef DEBUG
# ifndef WIN32
fprintf (stderr, "DEBUG: w,h=%d,%d count=%d, tc=%d\n", w, h, count, truecolor);
# endif
#endif
g_truecolor = truecolor;
return 1;
}

//-----------------------------------------------------------------------

static int display_video_handler (ubyte major, ubyte minor, ubyte *data, int len, void *context)
{
if (g_destX == -1) // center it
	g_destX = (g_screenWidth - g_width) >> 1;
if (g_destY == -1) // center it
	g_destY = (g_screenHeight - g_height) >> 1;
mve_showframe (reinterpret_cast<ubyte*> (g_vBackBuf [g_currBuf]), g_width, g_height, 0, 0, g_width, g_height, g_destX, g_destY);
g_frameUpdated = 1;
return 1;
}

//-----------------------------------------------------------------------

static int init_video_handler (ubyte major, ubyte minor, ubyte *data, int len, void *context)
{
	short width, height;

if (video_initialized)
	return 1; /* maybe we actually need to change width/height here? */
video_initialized = 1;
width = get_short (data);
height = get_short (data+2);
g_screenWidth = width;
g_screenHeight = height;
return 1;
}

//-----------------------------------------------------------------------

static int video_palette_handler (ubyte major, ubyte minor, ubyte *data, int len, void *context)
{
	short start, count;
	ubyte *p;

start = get_short (data);
count = get_short (data+2);
p = data + 4;
mve_setpalette (p - 3*start, start, count);
return 1;
}

//-----------------------------------------------------------------------

static int video_codemap_handler (ubyte major, ubyte minor, ubyte *data, int len, void *context)
{
g_pCurMap = data;
g_nMapLength = len;
return 1;
}

//-----------------------------------------------------------------------

static int video_data_handler (ubyte major, ubyte minor, ubyte *data, int len, void *context)
{
	short nFrameHot, nFrameCold;
	short nXoffset, nYoffset;
	short nXsize, nYsize;
	ushort nFlags;

nFrameHot  = get_short (data);
nFrameCold = get_short (data+2);
nXoffset   = get_short (data+4);
nYoffset   = get_short (data+6);
nXsize     = get_short (data+8);
nYsize     = get_short (data+10);
nFlags     = get_ushort (data+12);
if (nFlags & 1) {
	g_currBuf = !g_currBuf;
	}
/* convert the frame */
if (g_truecolor)
	decodeFrame16 (reinterpret_cast<ubyte*> (g_vBackBuf [g_currBuf]), g_pCurMap, g_nMapLength, data+14, len-14);
else
	decodeFrame8 (reinterpret_cast<ubyte*> (g_vBackBuf [g_currBuf]), g_pCurMap, g_nMapLength, data+14, len-14);
return 1;
}

//-----------------------------------------------------------------------

static int end_chunk_handler (ubyte major, ubyte minor, ubyte *data, int len, void *context)
{
g_pCurMap=NULL;
return 1;
}


//-----------------------------------------------------------------------

static MVESTREAM *mve = NULL;

void MVE_ioCallbacks (mve_cb_Read io_read)
{
mve_read = io_read;
}

//-----------------------------------------------------------------------

void MVE_memCallbacks (mve_cb_Alloc mem_alloc, mve_cb_Free MemFree)
{
mve_alloc = MVE_Alloc;
mve_free = MVE_Free;
}

//-----------------------------------------------------------------------

void MVE_sfCallbacks (mve_cb_ShowFrame showframe)
{
mve_showframe = showframe;
}

//-----------------------------------------------------------------------

void MVE_palCallbacks (mve_cb_SetPalette setpalette)
{
mve_setpalette = setpalette;
}

//-----------------------------------------------------------------------

int MVE_rmPrepMovie (void *src, int x, int y, int track, int bLittleEndian)
{
	ubyte i;

if (mve) {
	mve_reset (mve);
	return 0;
	}
mve = mve_open (src, bLittleEndian);
if (!mve)
	return 1;
g_destX = x;
g_destY = y;
for (i = 0; i < 32; i++)
	mve_set_handler (mve, i, default_seg_handler);
mve_set_handler (mve, MVE_OPCODE_ENDOFSTREAM, end_movie_handler);
mve_set_handler (mve, MVE_OPCODE_ENDOFCHUNK, end_chunk_handler);
mve_set_handler (mve, MVE_OPCODE_CREATETIMER, createTimer_handler);
mve_set_handler (mve, MVE_OPCODE_INITAUDIOBUFFERS, create_audiobuf_handler);
mve_set_handler (mve, MVE_OPCODE_STARTSTOPAUDIO, play_audio_handler);
mve_set_handler (mve, MVE_OPCODE_INITVIDEOBUFFERS, create_videobuf_handler);
mve_set_handler (mve, MVE_OPCODE_DISPLAYVIDEO, display_video_handler);
mve_set_handler (mve, MVE_OPCODE_AUDIOFRAMEDATA, audio_data_handler);
mve_set_handler (mve, MVE_OPCODE_AUDIOFRAMESILENCE, audio_data_handler);
mve_set_handler (mve, MVE_OPCODE_INITVIDEOMODE, init_video_handler);
mve_set_handler (mve, MVE_OPCODE_SETPALETTE, video_palette_handler);
mve_set_handler (mve, MVE_OPCODE_SETPALETTECOMPRESSED, default_seg_handler);
mve_set_handler (mve, MVE_OPCODE_SETDECODINGMAP, video_codemap_handler);
mve_set_handler (mve, MVE_OPCODE_VIDEODATA, video_data_handler);
mve_play_next_chunk (mve); /* video initialization chunk */
mve_play_next_chunk (mve); /* audio initialization chunk */
return 0;
}

//-----------------------------------------------------------------------

void MVE_getVideoSpec (MVE_videoSpec *vSpec)
{
vSpec->screenWidth = g_screenWidth;
vSpec->screenHeight = g_screenHeight;
vSpec->width = g_width;
vSpec->height = g_height;
vSpec->truecolor = g_truecolor;
}

//-----------------------------------------------------------------------

int MVE_rmStepMovie ()
{
	static int initTimer=0;
	int cont=1;
int j = 0;
if (!timer_started)
	timer_start ();
while (cont && !g_frameUpdated) // make a "step" be a frame, not a chunk...
{
	if (++j == 15)
		j = j;
	cont = mve_play_next_chunk (mve);
}
g_frameUpdated = 0;
if (micro_frame_delay  && !initTimer) {
	timer_start ();
	initTimer = 1;
	}
doTimer_wait ();
return cont ? 0 : MVE_ERR_EOF;
}

//-----------------------------------------------------------------------

void MVE_rmEndMovie ()
{
#ifdef AUDIO
	int i;
#endif

	timer_stop ();
	timer_created = 0;

#ifdef AUDIO
if (mve_audio_canplay) {
	// only close audio if we opened it
	SDL_CloseAudio ();
	mve_audio_canplay = 0;
	}
for (i = 0; i < TOTAL_AUDIO_BUFFERS; i++)
	if (mve_audio_buffers[i] != NULL)
		mve_free (mve_audio_buffers[i]);
memset (mve_audio_buffers, 0, sizeof (mve_audio_buffers));
memset (mve_audio_buflens, 0, sizeof (mve_audio_buflens));
mve_audio_curbuf_curpos=0;
mve_audio_bufhead=0;
mve_audio_buftail=0;
mve_audio_playing=0;
mve_audio_canplay=0;
mve_audio_compressed=0;
if (mve_audio_spec)
	mve_free (mve_audio_spec);
mve_audio_spec=NULL;
audiobuf_created = 0;
#endif
mve_free (g_vBackBuf [0]);
mve_free (g_vBackBuf [1]);
g_vBackBuf [0] =
g_vBackBuf [1] = NULL;
g_pCurMap = NULL;
g_nMapLength=0;
videobuf_created = 0;
video_initialized = 0;
mve_close (mve);
mve = NULL;
}

//-----------------------------------------------------------------------

void MVE_rmHoldMovie ()
{
timer_started = 0;
}

//-----------------------------------------------------------------------

void MVE_sndInit (int x)
{
#ifdef AUDIO
mve_audio_enabled = (x != -1);
#endif
}

//-----------------------------------------------------------------------
//eof


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

int32_t g_spdFactorNum=0;
static int32_t g_spdFactorDenom=10;
static int32_t g_frameUpdated = 0;

//-----------------------------------------------------------------------

static int16_t get_short (uint8_t *data)
{
	int16_t value = data[0] | (data[1] << 8);

return value;
}

//-----------------------------------------------------------------------

static uint16_t get_ushort (uint8_t *data)
{
	uint16_t value = data[0] | (data[1] << 8);

return value;
}

//-----------------------------------------------------------------------

static int32_t get_int (uint8_t *data)
{
	int32_t value = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);

return value;
}

//-----------------------------------------------------------------------

static uint32_t unhandled_chunks[32*256];

static int32_t default_seg_handler (uint8_t major, uint8_t minor, uint8_t *data, int32_t len, void *context)
{
unhandled_chunks[major<<8|minor]++;
return 1;
}


/*************************
 * general handlers
 *************************/

static int32_t end_movie_handler (uint8_t major, uint8_t minor, uint8_t *data, int32_t len, void *context)
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
static int32_t timer_created = 0;
static int32_t micro_frame_delay=0;
static int32_t timer_started=0;
static struct timeval timer_expire = {0, 0};

#if !HAVE_STRUCT_TIMESPEC
struct timespec
{
	uint32_t tv_sec;            /* Seconds.  */
	uint32_t tv_nsec;           /* Nanoseconds.  */
};
#endif

//-----------------------------------------------------------------------

#ifdef _WIN32
int32_t gettimeofday (struct timeval *tv, void *tz)
{
	static int32_t counter = 0;
	DWORD now;

counter++;
now = GetTickCount ();
tv->tv_sec = now / 1000;
tv->tv_usec = (now % 1000) * 1000 + counter;
return 0;
}
#endif


//-----------------------------------------------------------------------

static int32_t createTimer_handler (uint8_t major, uint8_t minor, uint8_t *data, int32_t len, void *context)
{
#ifndef _WIN32 //FIXME
	__extension__ long long temp;
#else
	int64_t temp;
#endif

if (timer_created)
	return 1;
timer_created = 1;
micro_frame_delay = get_int (data) * (int32_t)get_short (data+4);
if (g_spdFactorNum != 0) {
	temp = micro_frame_delay;
	temp *= g_spdFactorNum;
	temp /= g_spdFactorDenom;
	micro_frame_delay = (int32_t)temp;
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
	int32_t nsec=0;

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
	int32_t nsec=0;
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

static int32_t audiobuf_created = 0;
static void _CDECL_ mve_audio_callback (void *userdata, Uint8 *stream, int32_t len);
static int16_t *mve_audio_buffers[TOTAL_AUDIO_BUFFERS];
static int32_t    mve_audio_buflens[TOTAL_AUDIO_BUFFERS];
static int32_t    mve_audio_curbuf_curpos=0;
static int32_t mve_audio_bufhead=0;
static int32_t mve_audio_buftail=0;
static int32_t mve_audio_playing=0;
static int32_t mve_audio_canplay=0;
static int32_t mve_audio_compressed=0;
static int32_t mve_audio_enabled = 1;
static SDL_AudioSpec *mve_audio_spec=NULL;

//-----------------------------------------------------------------------

static void _CDECL_ mve_audio_callback (void *userdata, Uint8* stream, int32_t len)
{
	int32_t total=0;
	int32_t length;

if (mve_audio_bufhead == mve_audio_buftail)
	return /* 0 */;
while (mve_audio_bufhead != mve_audio_buftail                                           /* while we have more buffers  */
		 &&  len > (mve_audio_buflens[mve_audio_bufhead]-mve_audio_curbuf_curpos)) {      /* and while we need more data */
	length = mve_audio_buflens[mve_audio_bufhead]-mve_audio_curbuf_curpos;
	memcpy (stream, /* cur output position */
		      reinterpret_cast<uint8_t*> (mve_audio_buffers[mve_audio_bufhead])+mve_audio_curbuf_curpos, /* cur input position  */
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
			 reinterpret_cast<uint8_t*> (mve_audio_buffers[mve_audio_bufhead]) + mve_audio_curbuf_curpos, /* src */
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

static int32_t create_audiobuf_handler (uint8_t major, uint8_t minor, uint8_t *data, int32_t len, void *context)
{
#ifdef AUDIO
	int32_t flags;
	int32_t sample_rate;
	int32_t stereo;
	int32_t bitsize;
	int32_t compressed;
	int32_t format;

if (!mve_audio_enabled)
	return 1;
if (audiobuf_created)
	return 1;
audiobuf_created = 1;
flags = get_ushort (data + 2);
sample_rate = get_ushort (data + 4);
get_int (data + 6);
stereo = (flags & MVE_AUDIO_FLAGS_STEREO) ? 1 : 0;
bitsize = (flags & MVE_AUDIO_FLAGS_16BIT) ? 1 : 0;
compressed = (minor > 0) ? (flags & MVE_AUDIO_FLAGS_COMPRESSED ? 1 : 0) : 0;
mve_audio_compressed = compressed;
if (bitsize == 1) {
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	format = AUDIO_S16MSB;
#else
	format = AUDIO_S16SYS;
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
	mve_audio_canplay = 1;
	}
else {
	mve_audio_canplay = 0;
	}
memset (mve_audio_buffers, 0, sizeof (mve_audio_buffers));
memset (mve_audio_buflens, 0, sizeof (mve_audio_buflens));
#endif
return 1;
}

//-----------------------------------------------------------------------

static int32_t play_audio_handler (uint8_t major, uint8_t minor, uint8_t *data, int32_t len, void *context)
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

static int32_t audio_data_handler (uint8_t major, uint8_t minor, uint8_t *data, int32_t len, void *context)
{
#ifdef AUDIO
	static const int32_t selected_chan=1;
	int32_t chan;
	int32_t nsamp;

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
				mve_audio_buffers[mve_audio_buftail] = reinterpret_cast<int16_t*> (mve_alloc (nsamp));
				mveaudio_uncompress (mve_audio_buffers[mve_audio_buftail], data, -1); /* XXX */
				}
			else {
				nsamp -= 8;
				data += 8;
				mve_audio_buflens[mve_audio_buftail] = nsamp;
				mve_audio_buffers[mve_audio_buftail] = reinterpret_cast<int16_t*> (mve_alloc (nsamp));
				memcpy (mve_audio_buffers[mve_audio_buftail], data, nsamp);
				}
			}
		else {
			mve_audio_buflens[mve_audio_buftail] = nsamp;
			mve_audio_buffers[mve_audio_buftail] = reinterpret_cast<int16_t*> (mve_alloc (nsamp));
			memset (mve_audio_buffers[mve_audio_buftail], 0, nsamp); /* XXX */
			}
		if (++mve_audio_buftail == TOTAL_AUDIO_BUFFERS)
			mve_audio_buftail = 0;
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

static int32_t videobuf_created = 0;
static int32_t video_initialized = 0;
int32_t g_width, g_height;
void *g_vBuffers = NULL, *g_vBackBuf1, *g_vBackBuf2;

static int32_t g_destX, g_destY;
static int32_t g_screenWidth, g_screenHeight;
static uint8_t *g_pCurMap=NULL;
static int32_t g_nMapLength=0;
static int32_t g_truecolor;

//-----------------------------------------------------------------------

static int32_t create_videobuf_handler (uint8_t major, uint8_t minor, uint8_t *data, int32_t len, void *context)
{
	int16_t w, h;
	int16_t truecolor;

if (videobuf_created)
	return 1;
videobuf_created = 1;
w = get_short (data);
h = get_short (data+2);
if (minor > 0)
	get_short (data+4);
truecolor = (minor > 1) ? get_short (data+6) : 0;
g_width = w << 3;
g_height = h << 3;
if (g_vBuffers == NULL)
	g_vBackBuf1 = g_vBuffers = mve_alloc (g_width * g_height * 8);
if (truecolor)
	g_vBackBuf2 = reinterpret_cast<uint16_t*> (g_vBackBuf1) + (g_width * g_height);
else
	g_vBackBuf2 = reinterpret_cast<uint8_t*> (g_vBackBuf1) + (g_width * g_height);
memset (g_vBackBuf1, 0, g_width * g_height * 4);
g_truecolor = truecolor;
return 1;
}

//-----------------------------------------------------------------------

static int32_t display_video_handler (uint8_t major, uint8_t minor, uint8_t *data, int32_t len, void *context)
{
if (g_destX == -1) // center it
	g_destX = (g_screenWidth - g_width) >> 1;
if (g_destY == -1) // center it
	g_destY = (g_screenHeight - g_height) >> 1;
mve_showframe (reinterpret_cast<uint8_t*> (g_vBackBuf1), g_width, g_height, 0, 0, g_width, g_height, g_destX, g_destY);
g_frameUpdated = 1;
return 1;
}

//-----------------------------------------------------------------------

static int32_t init_video_handler (uint8_t major, uint8_t minor, uint8_t *data, int32_t len, void *context)
{
	int16_t width, height;

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

static int32_t video_palette_handler (uint8_t major, uint8_t minor, uint8_t *data, int32_t len, void *context)
{
	int16_t start, count;
	uint8_t *p;

start = get_short (data);
count = get_short (data+2);
p = data + 4;
mve_setpalette (p - 3*start, start, count);
return 1;
}

//-----------------------------------------------------------------------

static int32_t video_codemap_handler (uint8_t major, uint8_t minor, uint8_t *data, int32_t len, void *context)
{
g_pCurMap = data;
g_nMapLength = len;
return 1;
}

//-----------------------------------------------------------------------

static int32_t video_data_handler (uint8_t major, uint8_t minor, uint8_t *data, int32_t len, void *context)
{
//int16_t nFrameHot  = get_short (data);
//int16_t nFrameCold = get_short (data+2);
//int16_t nXoffset   = get_short (data+4);
//int16_t nYoffset   = get_short (data+6);
//int16_t nXsize     = get_short (data+8);
//int16_t nYsize     = get_short (data+10);
uint16_t nFlags     = get_ushort (data+12);
if (nFlags & 1) {
	uint8_t* temp = reinterpret_cast<uint8_t*> (g_vBackBuf1);
	g_vBackBuf1 = g_vBackBuf2;
	g_vBackBuf2 = temp;
	}
/* convert the frame */
if (g_truecolor)
	decodeFrame16 (reinterpret_cast<uint8_t*> (g_vBackBuf1), g_pCurMap, g_nMapLength, data+14, len-14);
else
	decodeFrame8 (reinterpret_cast<uint8_t*> (g_vBackBuf1), g_pCurMap, g_nMapLength, data+14, len-14);
return 1;
}

//-----------------------------------------------------------------------

static int32_t end_chunk_handler (uint8_t major, uint8_t minor, uint8_t *data, int32_t len, void *context)
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

int32_t MVE_rmPrepMovie (void *src, int32_t x, int32_t y, int32_t track, int32_t bLittleEndian)
{
	uint8_t i;

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

int32_t MVE_rmStepMovie ()
{
	static int32_t initTimer=0;
	int32_t cont=1;

if (!timer_started)
	timer_start ();
while (cont && !g_frameUpdated) // make a "step" be a frame, not a chunk...
	cont = mve_play_next_chunk (mve);
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
	int32_t i;
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
mve_free (g_vBuffers);
g_vBuffers = NULL;
g_pCurMap=NULL;
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

void MVE_sndInit (int32_t x)
{
#ifdef AUDIO
mve_audio_enabled = (x != -1);
#endif
}

//-----------------------------------------------------------------------
//eof


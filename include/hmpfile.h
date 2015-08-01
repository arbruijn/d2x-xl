#ifndef _HMPFILE_H
#define _HMPFILE_H

#ifdef _WIN32
#	include <windows.h>
#	include <mmsystem.h>
#endif
#include "pstypes.h"

#define HMP_TRACKS 32
#define HMP_BUFFERS 4
#define HMP_BUFSIZE 1024

typedef struct event {
	uint32_t		delta;
	uint8_t		msg[3];
	uint8_t*	data;
	uint32_t		datalen;
} event;

typedef struct hmp_track {
	uint8_t*	data;
	uint32_t		len;
	uint8_t*	cur;
	uint32_t		left;
	uint32_t	curTime;
} hmp_track;

typedef struct hmp_file {
	int32_t			num_trks;
	hmp_track	trks[HMP_TRACKS];
	uint32_t			curTime;
	int32_t			tempo;
#ifdef _WIN32
	MIDIHDR*		evbuf;
   HMIDISTRM	hmidi;
	UINT			devid;
#endif
	uint8_t*		pending;
	uint32_t			pending_size;
	uint32_t			pending_event;
	int32_t			stop;		/* 1 -> don't send more data */
	int32_t			bufs_in_mm;	/* number of queued buffers */
	int32_t			bLoop;
	uint32_t			midi_division;
} hmp_file;


#define HMP_INVALID_FILE -1
#define HMP_OUT_OF_MEM -2
#define HMP_MM_ERR -3
#define HMP_EOF 1

hmp_file *hmp_open (const char *filename, int32_t bUseD1Hog);
int32_t hmp_play (hmp_file *hmp, int32_t bLoop);
void hmp_close (hmp_file *hmp);
int32_t hmp_to_midi (hmp_file *hmp, char *pszFn);

#endif

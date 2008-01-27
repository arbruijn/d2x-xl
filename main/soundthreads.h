/* $Id: digi.h,v 1.6 2004/11/29 06:09:06 btb Exp $ */
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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _SOUNDTHREADS_H
#define _SOUNDTHREADS_H

#include "inferno.h"

//------------------------------------------------------------------------------

typedef enum tSoundTask {
	stOpenAudio,
	stCloseAudio,
	stReconfigureAudio
	} tSoundTask;

typedef struct tSoundThreadInfo {
	tSoundTask		nTask;
	float				fSlowDown;
	tThreadInfo		ti;
} tSoundThreadInfo;

void StartSoundThread (void);
void StartEndThread (void);
int RunSoundThread (tSoundTask nTask);

extern tSoundThreadInfo tiSound;

//------------------------------------------------------------------------------

#endif

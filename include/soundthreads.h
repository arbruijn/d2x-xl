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

#include "descent.h"

//------------------------------------------------------------------------------

typedef enum tSoundTask {
	stOpenAudio,
	stCloseAudio,
	stReconfigureAudio
	} tSoundTask;

class CSoundThreadInfo : public CThreadInfo {
	public:
		tSoundTask	nTask;
		float			fSlowDown;
};

void CreateSoundThread (void);
void DestroySoundThread (void);
void WaitForSoundThread (time_t nTimeout = -1);
int32_t StartSoundThread (tSoundTask nTask);
bool HaveSoundThread (void);

extern CSoundThreadInfo tiSound;

//------------------------------------------------------------------------------

#endif

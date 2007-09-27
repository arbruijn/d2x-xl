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



#ifndef _RBAUDIO_H
#define _RBAUDIO_H

#define RBA_MEDIA_CHANGED	-1

typedef struct RBACHANNELCTL {
	unsigned int out0in, out0vol;
	unsigned int out1in, out1vol;
	unsigned int out2in, out2vol;
	unsigned int out3in, out3vol;
} RBACHANNELCTL;

void RBAInit(void);	//drive a == 0, drive b == 1
void RBARegisterCD(void);
long RBAGetDeviceStatus(void);
int RBAPlayTrack(int track);
int RBAPlayTracks(int first, int last);	//plays tracks first through last, inclusive
int RBACheckMediaChange();
long	RBAGetHeadLoc(int *min, int *sec, int *frame);
int	RBAPeekPlayStatus(void);
void _CDECL_ RBAStop(void);
void RBASetStereoAudio(RBACHANNELCTL *channels);
void RBASetQuadAudio(RBACHANNELCTL *channels);
void RBAGetAudioInfo(RBACHANNELCTL *channels);
void RBASetChannelVolume(int channel, int volume);
void RBASetVolume(int volume);
int	RBAEnabled(void);
void RBADisable(void);
void RBAEnable(void);
int	RBAGetNumberOfTracks(void);
void	RBAPause();
int	RBAResume();

//return the track number currently playing.  Useful if RBAPlayTracks() 
//is called.  Returns 0 if no track playing, else track number
int RBAGetTrackNum();

// get the cddb discid for the current cd.
unsigned int RBAGetDiscID();

#endif

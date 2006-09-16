/* $Id: digiobj.c,v 1.1 2004/11/29 05:25:58 btb Exp $ */
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


#ifdef RCS
static char rcsid [] = "$Id: digiobj.c,v 1.1 2004/11/29 05:25:58 btb Exp $";
#endif

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h> 

#ifdef __MSDOS__
# include <dos.h>
# include <bios.h>
# include <io.h>
# include <conio.h> 
#endif

#include <string.h>
#include <ctype.h>

#include "fix.h"
#include "inferno.h"
#include "mono.h"
#include "timer.h"
#include "joy.h"
#include "digi.h"
#include "sounds.h"
#include "args.h"
#include "key.h"
#include "newdemo.h"
#include "game.h"
#ifdef __MSDOS__
#include "dpmi.h"
#endif
#include "error.h"
#include "wall.h"
#include "cfile.h"
#include "piggy.h"
#include "text.h"
#include "kconfig.h"
#include "gameseg.h"

#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
# if USE_SDL_MIXER
#  include <SDL_mixer.h>
# endif
#endif

#define SOF_USED				1 		// Set if this sample is used
#define SOF_PLAYING			2		// Set if this sample is playing on a channel
#define SOF_LINK_TO_OBJ		4		// Sound is linked to a moving object. If object dies, then finishes play and quits.
#define SOF_LINK_TO_POS		8		// Sound is linked to segment, pos
#define SOF_PLAY_FOREVER	16		// Play forever (or until level is stopped), otherwise plays once
#define SOF_PERMANENT		32		// Part of the level, like a waterfall or fan

typedef struct sound_object {
	short			signature;		// A unique signature to this sound
	ubyte			flags;			// Used to tell if this slot is used and/or currently playing, and how long.
	ubyte			pad;				//	Keep alignment
	fix			max_volume;		// Max volume that this sound is playing at
	fix			max_distance;	// The max distance that this sound can be heard at...
	int			volume;			// Volume that this sound is playing at
	int			pan;				// Pan value that this sound is playing at
	int			channel;			// What channel this is playing on, -1 if not playing
	short			soundnum;		// The sound number that is playing
	int			loop_start;		// The start point of the loop. -1 means no loop
	int			loop_end;		// The end point of the loop
	union {	
		struct {
			short			segnum;				// Used if SOF_LINK_TO_POS field is used
			short			sidenum;				
			vms_vector	position;
		} pos;
		struct {
			short			objnum;				// Used if SOF_LINK_TO_OBJ field is used
			short			objsignature;
		} obj;
	} link_type;
} sound_object;

#define MAX_SOUND_OBJECTS 150
sound_object SoundObjects [MAX_SOUND_OBJECTS];

//------------------------------------------------------------------------------
/* Find the sound which actually equates to a sound number */
short DigiXlatSound (short soundno)
{
if (soundno < 0)
	return -1;
if (gameStates.sound.digi.bLoMem) {
	soundno = AltSounds [gameOpts->sound.bD1Sound] [soundno];
	if (soundno == 255)
		return -1;
	}
//Assert (Sounds [gameOpts->sound.bD1Sound] [soundno] != 255);	//if hit this, probably using undefined sound
if (Sounds [gameOpts->sound.bD1Sound] [soundno] == 255)
	return -1;
return Sounds [gameOpts->sound.bD1Sound] [soundno];
}

//------------------------------------------------------------------------------

int DigiUnXlatSound (int soundno)
{
	int i;
	ubyte *table = (gameStates.sound.digi.bLoMem ? AltSounds [gameOpts->sound.bD1Sound] :Sounds [gameOpts->sound.bD1Sound]);

	if (soundno < 0) return -1;

	for (i=0;i<MAX_SOUNDS;i++)
		if (table [i] == soundno)
			return i;

	Int3 ();
	return 0;
}

//------------------------------------------------------------------------------

void DigiGetSoundLoc (
	vms_matrix * listener, vms_vector * listener_pos, short listener_seg, vms_vector * sound_pos, 
	short sound_seg, fix max_volume, int *volume, int *pan, fix max_distance)	
{	  

	vms_vector	vector_to_sound;
	fix angle_from_ear, cosang,sinang;
	fix distance;
	fix path_distance;

	*volume = 0;
	*pan = 0;

	max_distance = (max_distance*5)/4;		// Make all sounds travel 1.25 times as far.

	//	Warning: Made the VmVecNormalizedDir be VmVecNormalizedDirQuick and got illegal values to acos in the fang computation.
	distance = VmVecNormalizedDirQuick (&vector_to_sound, sound_pos, listener_pos);
		
	if (distance < max_distance)	{
		int num_search_segs = f2i (max_distance/20);
		if (num_search_segs < 1) num_search_segs = 1;

		path_distance = FindConnectedDistance (listener_pos, listener_seg, sound_pos, sound_seg, num_search_segs, WID_RENDPAST_FLAG+WID_FLY_FLAG);
		if (path_distance > -1)	{
			*volume = max_volume - fixdiv (path_distance,max_distance);
			////mprintf ((0, "Sound path distance %.2f, volume is %d / %d\n", f2fl (distance), *volume, max_volume));
			if (*volume > 0)	{
				angle_from_ear = VmVecDeltaAngNorm (&listener->rvec,&vector_to_sound,&listener->uvec);
				fix_sincos (angle_from_ear,&sinang,&cosang);
				////mprintf ((0, "volume is %.2f\n", f2fl (*volume)));
				if (gameConfig.bReverseChannels) cosang *= -1;
				*pan = (cosang + F1_0)/2;
			} else {
				*volume = 0;
			}
		}
	}																					  

}

//------------------------------------------------------------------------------

void DigiPlaySampleOnce (short soundno, fix max_volume)	
{
	int channel;

#ifdef NEWDEMO
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordSound (soundno);
#endif
soundno = DigiXlatSound (soundno);
if (soundno < 0) 
	return;
channel=DigiFindChannel (soundno);
if (channel > -1)
	DigiStopSound (channel);
// start the sample playing
DigiStartSound (soundno, max_volume, 0xffff/2, 0, -1, -1, -1, F1_0, NULL);
}

//------------------------------------------------------------------------------

int DigiPlaySampleSpeed (short soundno, fix max_volume, int nSpeed)
{
#ifdef NEWDEMO
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordSound (soundno);
#endif
soundno = (soundno < 0) ? - soundno : DigiXlatSound (soundno);
if (soundno < 0) 
	return -1;
// start the sample playing
return DigiStartSound (soundno, max_volume, 0xffff/2, 0, -1, -1, -1, nSpeed, NULL);
}

//------------------------------------------------------------------------------

int DigiPlaySample (short soundno, fix max_volume)
{
return DigiPlaySampleSpeed (soundno, max_volume, F1_0);
}

//------------------------------------------------------------------------------

void DigiPlaySample3D (short soundno, int angle, int volume, int no_dups)	
{

	no_dups = 1;

#ifdef NEWDEMO
	if (gameData.demo.nState == ND_STATE_RECORDING)		{
		if (no_dups)
			NDRecordSound3DOnce (soundno, angle, volume);
		else
			NDRecordSound3D (soundno, angle, volume);
	}
#endif
	soundno = DigiXlatSound (soundno);

	if (soundno < 0) return;

	if (volume < 10) return;

   // start the sample playing
	DigiStartSound (soundno, volume, angle, 0, -1, -1, -1, F1_0, NULL);
}

//------------------------------------------------------------------------------

void SoundQInit ();
void SoundQProcess ();
void SoundQPause ();

void DigiInitSounds ()
{
	int i;

	SoundQInit ();

	DigiStopAllChannels ();

	DigiStopLoopingSound ();
	for (i=0; i<MAX_SOUND_OBJECTS; i++)	{
		SoundObjects [i].channel = -1;
		SoundObjects [i].flags = 0;	// Mark as dead, so some other sound can use this sound
	}
	gameStates.sound.digi.nActiveObjects = 0;
	gameStates.sound.digi.bSoundsInitialized = 1;
}

//------------------------------------------------------------------------------

// plays a sample that loops forever.  
// Call digi_stop_channe (channel) to stop it.
// Call DigiSetChannelVolume (channel, volume) to change volume.
// if loop_start is -1, entire sample loops
// Returns the channel that sound is playing on, or -1 if can't play.
// This could happen because of no sound drivers loaded or not enough channels.

void DigiPlaySampleLoopingSub ()
{
if (gameStates.sound.digi.nLoopingSound > -1)
	gameStates.sound.digi.nLoopingChannel  = 
		DigiStartSound (gameStates.sound.digi.nLoopingSound, 
								gameStates.sound.digi.nLoopingVolume, 
								0xFFFF/2, 
								1, 
								gameStates.sound.digi.nLoopingStart, 
								gameStates.sound.digi.nLoopingEnd, 
								-1, 
								F1_0, NULL);
}

//------------------------------------------------------------------------------

void DigiPlaySampleLooping (short soundno, fix max_volume,int loop_start, int loop_end)
{
	soundno = DigiXlatSound (soundno);

	if (soundno < 0) return;

	if (gameStates.sound.digi.nLoopingChannel>-1)
		DigiStopSound (gameStates.sound.digi.nLoopingChannel);

	gameStates.sound.digi.nLoopingSound = soundno;
	gameStates.sound.digi.nLoopingVolume = max_volume;
	gameStates.sound.digi.nLoopingStart = loop_start;
	gameStates.sound.digi.nLoopingEnd = loop_end;
	DigiPlaySampleLoopingSub ();
}

//------------------------------------------------------------------------------

void DigiChangeLoopingVolume (fix volume)
{
	if (gameStates.sound.digi.nLoopingChannel > -1)
		DigiSetChannelVolume (gameStates.sound.digi.nLoopingChannel, volume);
	gameStates.sound.digi.nLoopingVolume = volume;
}

//------------------------------------------------------------------------------

void DigiStopLoopingSound ()
{
	if (gameStates.sound.digi.nLoopingChannel > -1)
		DigiStopSound (gameStates.sound.digi.nLoopingChannel);
	gameStates.sound.digi.nLoopingChannel = -1;
	gameStates.sound.digi.nLoopingSound = -1;
}
	
//------------------------------------------------------------------------------

void DigiPauseLoopingSound ()
{
	if (gameStates.sound.digi.nLoopingChannel > -1)
		DigiStopSound (gameStates.sound.digi.nLoopingChannel);
	gameStates.sound.digi.nLoopingChannel = -1;
}

//------------------------------------------------------------------------------

void DigiResumeLoopingSound ()
{
	DigiPlaySampleLoopingSub ();
}

//------------------------------------------------------------------------------
//hack to not start object when loading level

void DigiStartSoundObject (int i)
{
	// start sample structures 
SoundObjects [i].channel =  -1;
if (SoundObjects [i].volume <= 0)
	return;
if (gameStates.sound.bDontStartObjects)
	return;
// -- MK, 2/22/96 -- 	if (gameData.demo.nState == ND_STATE_RECORDING)
// -- MK, 2/22/96 -- 		NDRecordSound3DOnce (DigiUnXlatSound (SoundObjects [i].soundnum), SoundObjects [i].pan, SoundObjects [i].volume);
// only use up to half the sound channels for "permanent" sounts
if ((SoundObjects [i].flags & SOF_PERMANENT) && 
	 (gameStates.sound.digi.nActiveObjects >= max (1, DigiGetMaxChannels () / 4)))
	return;
// start the sample playing
SoundObjects [i].channel = 
	DigiStartSound (
		SoundObjects [i].soundnum, 
		SoundObjects [i].volume, 
		SoundObjects [i].pan, 
		SoundObjects [i].flags & SOF_PLAY_FOREVER, 
		SoundObjects [i].loop_start, 
		SoundObjects [i].loop_end, i, F1_0, NULL);
if (SoundObjects [i].channel > -1)
	gameStates.sound.digi.nActiveObjects++;
}

//------------------------------------------------------------------------------
//sounds longer than this get their 3d aspects updated
#define SOUND_3D_THRESHHOLD  (gameOpts->sound.digiSampleRate * 3 / 2)	//1.5 seconds

int DigiLinkSoundToObject3 (
	short org_soundnum, short objnum, int forever, fix max_volume, fix  max_distance, 
	int loop_start, int loop_end)
{
	int				i, volume, pan;
	object			*objP;
	short				soundnum;
	sound_object *sop;

soundnum = DigiXlatSound (org_soundnum);
if (max_volume < 0) 
	return -1;
//	if (max_volume > F1_0) max_volume = F1_0;
if (soundnum < 0) 
	return -1;
if (!gameData.pig.snd.sounds [gameOpts->sound.bD1Sound] [soundnum].data) {
	Int3 ();
	return -1;
	}
if ((objnum<0)|| (objnum>gameData.objs.nLastObject))
	return -1;
if (!forever) { 		// && gameData.pig.snd.sounds [soundnum - SOUND_OFFSET].length < SOUND_3D_THRESHHOLD)	{
	// Hack to keep sounds from building up...
	DigiGetSoundLoc (
		&gameData.objs.viewer->orient, &gameData.objs.viewer->pos, gameData.objs.viewer->segnum, &gameData.objs.objects [objnum].pos, 
		gameData.objs.objects [objnum].segnum, max_volume,&volume, &pan, max_distance);
	DigiPlaySample3D (org_soundnum, pan, volume, 0);
	return -1;
	}
#ifdef NEWDEMO
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordLinkSoundToObject3 (
		org_soundnum, objnum, max_volume, max_distance, loop_start, loop_end);
#endif
for (i=0, sop = SoundObjects; i<MAX_SOUND_OBJECTS; i++, sop++)
	if (sop->flags==0)
		break;
if (i == MAX_SOUND_OBJECTS) {
	//mprintf ((1, "Too many sound gameData.objs.objects!\n"));
	return -1;
	}
sop->signature=gameStates.sound.digi.nNextSignature++;
sop->flags = SOF_USED | SOF_LINK_TO_OBJ;
if (forever)
	sop->flags |= SOF_PLAY_FOREVER;
sop->link_type.obj.objnum = objnum;
sop->link_type.obj.objsignature = gameData.objs.objects [objnum].signature;
sop->max_volume = max_volume;
sop->max_distance = max_distance;
sop->volume = 0;
sop->pan = 0;
sop->soundnum = soundnum;
sop->loop_start = loop_start;
sop->loop_end = loop_end;
if (gameStates.sound.bDontStartObjects) { 		//started at level start
	sop->flags |= SOF_PERMANENT;
	sop->channel =  -1;
	}
else {
	objP = gameData.objs.objects + sop->link_type.obj.objnum;
	DigiGetSoundLoc (
		&gameData.objs.viewer->orient, &gameData.objs.viewer->pos, gameData.objs.viewer->segnum, 
		&objP->pos, objP->segnum, sop->max_volume,
      &sop->volume, &sop->pan, sop->max_distance);
	DigiStartSoundObject (i);
	// If it's a one-shot sound effect, and it can't start right away, then
	// just cancel it and be done with it.
	if ((sop->channel < 0) && (! (sop->flags & SOF_PLAY_FOREVER))) {
		sop->flags = 0;
		return -1;
		}
	}
return sop->signature;
}

//------------------------------------------------------------------------------

int DigiLinkSoundToObject2 (
	short org_soundnum, short objnum, int forever, fix max_volume, fix  max_distance)
{		
return DigiLinkSoundToObject3 (org_soundnum, objnum, forever, max_volume, max_distance, -1, -1);
}

//------------------------------------------------------------------------------

int DigiLinkSoundToObject (short soundnum, short objnum, int forever, fix max_volume)
{		
return DigiLinkSoundToObject2 (soundnum, objnum, forever, max_volume, 256*F1_0 );
}

//------------------------------------------------------------------------------

int DigiLinkSoundToPos2 (
	short org_soundnum, short segnum, short sidenum, vms_vector * pos, int forever, 
	fix max_volume, fix max_distance)
{

	int i, volume, pan;
	int soundnum;
	sound_object *sop;

soundnum = DigiXlatSound (org_soundnum);
if (max_volume < 0) 
	return -1;
//	if (max_volume > F1_0) max_volume = F1_0;
if (soundnum < 0) 
	return -1;
if (!gameData.pig.snd.sounds [gameOpts->sound.bD1Sound] [soundnum].data) {
	Int3 ();
	return -1;
	}
if ((segnum<0)|| (segnum>gameData.segs.nLastSegment))
	return -1;
if (!forever) { 	//&& gameData.pig.snd.sounds [soundnum - SOUND_OFFSET].length < SOUND_3D_THRESHHOLD)	{
	// Hack to keep sounds from building up...
	DigiGetSoundLoc (&gameData.objs.viewer->orient, &gameData.objs.viewer->pos, gameData.objs.viewer->segnum, pos, segnum, max_volume, &volume, &pan, max_distance);
	DigiPlaySample3D (org_soundnum, pan, volume, 0);
	return -1;
	}
for (i=0, sop = SoundObjects; i<MAX_SOUND_OBJECTS; i++, sop++)
	if (sop->flags == 0)
		break;
if (i == MAX_SOUND_OBJECTS) {
	//mprintf ((1, "Too many sound gameData.objs.objects!\n"));
	return -1;
	}
sop->signature = gameStates.sound.digi.nNextSignature++;
sop->flags = SOF_USED | SOF_LINK_TO_POS;
if (forever)
	sop->flags |= SOF_PLAY_FOREVER;
sop->link_type.pos.segnum = segnum;
sop->link_type.pos.sidenum = sidenum;
sop->link_type.pos.position = *pos;
sop->soundnum = soundnum;
sop->max_volume = max_volume;
sop->max_distance = max_distance;
sop->volume = 0;
sop->pan = 0;
sop->loop_start = sop->loop_end = -1;
if (gameStates.sound.bDontStartObjects) {		//started at level start
	sop->flags |= SOF_PERMANENT;
	sop->channel = -1;
	}
else {
	DigiGetSoundLoc (
		&gameData.objs.viewer->orient, &gameData.objs.viewer->pos, gameData.objs.viewer->segnum, 
      &sop->link_type.pos.position, sop->link_type.pos.segnum, sop->max_volume,
      &sop->volume, &sop->pan, sop->max_distance);
	DigiStartSoundObject (i);
	// If it's a one-shot sound effect, and it can't start right away, then
	// just cancel it and be done with it.
	if ((sop->channel < 0) && (! (sop->flags & SOF_PLAY_FOREVER)))    {
		sop->flags = 0;
		return -1;
		}
	}
return sop->signature;
}

//------------------------------------------------------------------------------

int DigiLinkSoundToPos (
	short soundnum, short segnum, short sidenum, vms_vector * pos, int forever, fix max_volume)
{
return DigiLinkSoundToPos2 (soundnum, segnum, sidenum, pos, forever, max_volume, F1_0 * 256);
}

//------------------------------------------------------------------------------
//if soundnum==-1, kill any sound
int DigiKillSoundLinkedToSegment (short segnum, short sidenum, short soundnum)
{
	int i,killed;
	sound_object	*soP;

	if (soundnum != -1)
		soundnum = DigiXlatSound (soundnum);


	killed = 0;

	for (i=0, soP = SoundObjects; i<MAX_SOUND_OBJECTS; i++, soP++)	{
		if ((soP->flags & (SOF_USED | SOF_LINK_TO_POS)) == (SOF_USED | SOF_LINK_TO_POS))	{
			if ((soP->link_type.pos.segnum == segnum) && (soP->link_type.pos.sidenum==sidenum) && 
				 (soundnum==-1 || soP->soundnum==soundnum))	{
				if (soP->channel > -1)	{
					DigiStopSound (soP->channel);
					gameStates.sound.digi.nActiveObjects--;
				}
				soP->channel = -1;
				soP->flags = 0;	// Mark as dead, so some other sound can use this sound
				killed++;
			}
		}
	}
#ifdef _DEBUG
	// If this assert happens, it means that there were 2 sounds
	// that got deleted. Weird, get John.
	if (killed > 1)	{
		//mprintf ((1, "ERROR: More than 1 sounds were deleted from seg %d\n", segnum));
	}
#endif
return (killed > 0);
}

//------------------------------------------------------------------------------

int DigiKillSoundLinkedToObject (int objnum)
{

	int i,killed;
	sound_object	*soP;

	killed = 0;

#ifdef NEWDEMO
	if (gameData.demo.nState == ND_STATE_RECORDING)		{
		NDRecordKillSoundLinkedToObject (objnum);
	}
#endif

	for (i=0, soP = SoundObjects; i<MAX_SOUND_OBJECTS; i++, soP++)	{
		if ((soP->flags & (SOF_USED | SOF_LINK_TO_OBJ)) == (SOF_USED | SOF_LINK_TO_OBJ))	{
			if (soP->link_type.obj.objnum == objnum)	{
				if (soP->channel > -1)	{
					DigiStopSound (soP->channel);
					gameStates.sound.digi.nActiveObjects--;
				}
				soP->channel = -1;
				soP->flags = 0;	// Mark as dead, so some other sound can use this sound
				killed++;
			}
		}
	}
#ifdef _DEBUG
	// If this assert happens, it means that there were 2 sounds
	// that got deleted. Weird, get John.
	if (killed > 1)	{
		//mprintf ((1, "ERROR: More than 1 sounds were deleted from object %d\n", objnum));
	}
#endif
return (killed > 0);
}

//------------------------------------------------------------------------------
//	John's new function, 2/22/96.
void DigiRecordSoundObjects ()
{
	int i;

	for (i=0; i<MAX_SOUND_OBJECTS; i++)	{
		if ((SoundObjects [i].flags & (SOF_USED | SOF_LINK_TO_OBJ | SOF_PLAY_FOREVER))
			 == (SOF_USED | SOF_LINK_TO_OBJ | SOF_PLAY_FOREVER))
		{
			NDRecordLinkSoundToObject3 (DigiUnXlatSound (SoundObjects [i].soundnum), SoundObjects [i].link_type.obj.objnum, 
			SoundObjects [i].max_volume, SoundObjects [i].max_distance, SoundObjects [i].loop_start, SoundObjects [i].loop_end);
		}
	}
}

//------------------------------------------------------------------------------

void Mix_VolPan (int channel, int vol, int pan);

void DigiSyncSounds ()
{
	int i;
	int oldvolume, oldpan;
	sound_object *sop;

if (gameData.demo.nState == ND_STATE_RECORDING)	{
	if (!gameStates.sound.bWasRecording)
		DigiRecordSoundObjects ();
	gameStates.sound.bWasRecording = 1;
	}
else
	gameStates.sound.bWasRecording = 0;

SoundQProcess ();

for (i = 0, sop = SoundObjects; i < MAX_SOUND_OBJECTS; i++, sop++) {
	if (sop->flags & SOF_USED)	{
		oldvolume = sop->volume;
		oldpan = sop->pan;
		// Check if its done.
		if (!(sop->flags & SOF_PLAY_FOREVER) && 
			 (sop->channel > -1) && 
			 !DigiIsChannelPlaying (sop->channel)) {
			DigiEndSound (sop->channel);
			sop->flags = 0;	// Mark as dead, so some other sound can use this sound
			gameStates.sound.digi.nActiveObjects--;
			continue;		// Go on to next sound...
			}			
		if (sop->flags & SOF_LINK_TO_POS) {
			DigiGetSoundLoc (
				&gameData.objs.viewer->orient, &gameData.objs.viewer->pos, gameData.objs.viewer->segnum, 
				&sop->link_type.pos.position, sop->link_type.pos.segnum, sop->max_volume,
				&sop->volume, &sop->pan, sop->max_distance);
#if USE_SDL_MIXER
			//if (sop->volume)
				Mix_VolPan (sop->channel, sop->volume, sop->pan);
#endif
			}
		else if (sop->flags & SOF_LINK_TO_OBJ) {
			object * objP;
			if (gameData.demo.nState == ND_STATE_PLAYBACK) {
				int objnum = NDFindObject (sop->link_type.obj.objsignature);
				objP = gameData.objs.objects + ((objnum > -1) ? objnum : 0);
				}
			else 
				objP = gameData.objs.objects + sop->link_type.obj.objnum;
			if ((objP->type == OBJ_NONE) || (objP->signature != sop->link_type.obj.objsignature)) {
			// The object that this is linked to is dead, so just end this sound if it is looping.
				if (sop->channel > -1)	{
					if (sop->flags & SOF_PLAY_FOREVER)
						DigiStopSound (sop->channel);
					else
						DigiEndSound (sop->channel);
					gameStates.sound.digi.nActiveObjects--;
					}
				sop->flags = 0;	// Mark as dead, so some other sound can use this sound
				continue;		// Go on to next sound...
				}
			else {
				DigiGetSoundLoc (
					&gameData.objs.viewer->orient, &gameData.objs.viewer->pos, gameData.objs.viewer->segnum, 
					&objP->pos, objP->segnum, sop->max_volume,
					&sop->volume, &sop->pan, sop->max_distance);
#if USE_SDL_MIXER
			//if (sop->volume)
				Mix_VolPan (sop->channel, sop->volume, sop->pan);
#endif
				}
			}
		if (oldvolume != sop->volume) {
			if (sop->volume < 1)	{
			// Sound is too far away, so stop it from playing.
				if (sop->channel > -1)	{
					if (sop->flags & SOF_PLAY_FOREVER)
						DigiStopSound (sop->channel);
					else
						DigiEndSound (sop->channel);
					gameStates.sound.digi.nActiveObjects--;
					sop->channel = -1;
					}
				if (!(sop->flags & SOF_PLAY_FOREVER)) {
					sop->flags = 0;	// Mark as dead, so some other sound can use this sound
					continue;
					}
				}
			else {
				if (sop->channel < 0)
					DigiStartSoundObject (i);
				else
					DigiSetChannelVolume (sop->channel, sop->volume);
				}
			}
		if ((oldpan != sop->pan) && (sop->channel > -1))
			DigiSetChannelPan (sop->channel, sop->pan);
	}
}
	
//------------------------------------------------------------------------------

#ifndef NDEBUG
//	DigiSoundDebug ();
#endif
}

void DigiPauseDigiSounds ()
{

	int i;

	DigiPauseLoopingSound ();

	for (i=0; i<MAX_SOUND_OBJECTS; i++)	{
		if ((SoundObjects [i].flags & SOF_USED) && (SoundObjects [i].channel>-1))	{
			DigiStopSound (SoundObjects [i].channel);
			if (! (SoundObjects [i].flags & SOF_PLAY_FOREVER))
				SoundObjects [i].flags = 0;	// Mark as dead, so some other sound can use this sound
			gameStates.sound.digi.nActiveObjects--;
			SoundObjects [i].channel = -1;
		}			
	}

	DigiStopAllChannels ();
	SoundQPause ();
}

//------------------------------------------------------------------------------

void DigiPauseAll ()
{
	digi_pause_midi ();
	DigiPauseDigiSounds ();
}

//------------------------------------------------------------------------------

void DigiResumeDigiSounds ()
{
	DigiSyncSounds ();	//don't think we really need to do this, but can't hurt
	DigiResumeLoopingSound ();
}

//------------------------------------------------------------------------------

extern void DigiResumeMidi ();

void DigiResumeAll ()
{
	DigiResumeMidi ();
	DigiResumeLoopingSound ();
}

//------------------------------------------------------------------------------
// Called by the code in digi.c when another sound takes this sound object's
// slot because the sound was done playing.
void DigiEndSoundObj (int i)
{
	Assert (SoundObjects [i].flags & SOF_USED);
	Assert (SoundObjects [i].channel > -1);
	
	gameStates.sound.digi.nActiveObjects--;
	SoundObjects [i].channel = -1;
}

//------------------------------------------------------------------------------

void DigiStopDigiSounds ()
{
	int i;

	DigiStopLoopingSound ();

	for (i=0; i<MAX_SOUND_OBJECTS; i++)	{
		if (SoundObjects [i].flags & SOF_USED)	{
			if (SoundObjects [i].channel > -1)	{
				DigiStopSound (SoundObjects [i].channel);
				gameStates.sound.digi.nActiveObjects--;
			}
			SoundObjects [i].flags = 0;	// Mark as dead, so some other sound can use this sound
		}
	}

	DigiStopAllChannels ();
	SoundQInit ();
}

//------------------------------------------------------------------------------

void DigiStopAll ()
{
	DigiStopCurrentSong ();

	DigiStopDigiSounds ();
}

//------------------------------------------------------------------------------

#ifndef NDEBUG
int VerifySoundChannelFree (int channel)
{
	int i;
	for (i=0; i<MAX_SOUND_OBJECTS; i++)	{
		if (SoundObjects [i].flags & SOF_USED)	{
			if (SoundObjects [i].channel == channel)	{
				//mprintf ((0, "ERROR STARTING SOUND CHANNEL ON USED SLOT!!\n"));
				Int3 ();	// Get John!
			}
		}
	}
	return 0;
}

//------------------------------------------------------------------------------

void DigiSoundDebug ()
{
	int i;
	int n_active_sound_objs=0;
	int n_sound_objs=0;

	for (i=0; i<MAX_SOUND_OBJECTS; i++)	{
		if (SoundObjects [i].flags & SOF_USED) 	{
			n_sound_objs++;
			if (SoundObjects [i].channel > -1)
				n_active_sound_objs++;
		}
	}
	//mprintf_at ((0, 0, 0, "DIGI: Active Sound gameData.objs.objects:  %d,%d/%d (%d max)             \n", n_active_sound_objs,gameStates.sound.digi.nActiveObjects, n_sound_objs, MAX_SOUND_OBJECTS));
	//mprintf_at ((0, 1, 0, "DIGI: Looping sound:  %s, snd=%d, vol=%d, ch=%d            \n", (gameStates.sound.digi.nLoopingSound>-1?"ON":"OFF"), gameStates.sound.digi.nLoopingSound, gameStates.sound.digi.nLoopingVolume, gameStates.sound.digi.nLoopingChannel ));

	DigiDebug ();
}
#endif

//------------------------------------------------------------------------------

typedef struct sound_q {
	fix	time_added;
	short soundnum;
} sound_q;

#define MAX_Q 32
#define MAX_LIFE F1_0*30		// After being queued for 30 seconds, don't play it
sound_q SoundQ [MAX_Q];
int SoundQ_head, SoundQ_tail, SoundQ_num;
int SoundQ_channel;

//------------------------------------------------------------------------------

void SoundQInit ()
{
	SoundQ_head = SoundQ_tail = 0;
	SoundQ_num = 0;
	SoundQ_channel = -1;
}

//------------------------------------------------------------------------------

void SoundQPause ()
{	
	SoundQ_channel = -1;
}

//------------------------------------------------------------------------------

void SoundQEnd ()
{
	// Current playing sound is stopped, so take it off the Queue
	SoundQ_head = (SoundQ_head+1);
	if (SoundQ_head >= MAX_Q) SoundQ_head = 0;
	SoundQ_num--;
	SoundQ_channel = -1;
}

//------------------------------------------------------------------------------

void SoundQProcess ()
{	
	fix curtime = TimerGetApproxSeconds ();

	if (SoundQ_channel > -1)	{
		if (DigiIsChannelPlaying (SoundQ_channel))
			return;
		SoundQEnd ();
	}

	if (SoundQ_num > 0)	{
		//mprintf ((0, "SQ:%d ", SoundQ_num));
	}

	while (SoundQ_head != SoundQ_tail)	{
		sound_q * q = SoundQ + SoundQ_head;
	
		if (q->time_added+MAX_LIFE > curtime)	{
			SoundQ_channel = DigiStartSound (q->soundnum, F1_0+1, 0xFFFF/2, 0, -1, -1, -1, F1_0, NULL);
			return;
		} else {
			// expired; remove from Queue
	  		SoundQEnd ();
		}
	}
}

//------------------------------------------------------------------------------

void DigiStartSoundQueued (short soundnum, fix volume)
{
	int i;

soundnum = DigiXlatSound (soundnum);
if (soundnum < 0) 
	return;
i = SoundQ_tail+1;
if (i>=MAX_Q) 
	i = 0;
// Make sure its loud so it doesn't get cancelled!
if (volume < F1_0+1)
	volume = F1_0 + 1;
if (i != SoundQ_head)	{
	SoundQ [SoundQ_tail].time_added = TimerGetApproxSeconds ();
	SoundQ [SoundQ_tail].soundnum = soundnum;
	SoundQ_num++;
	SoundQ_tail = i;
	}
else {
	//mprintf ((0, "Sound Q full!\n"));
}
// Try to start it!
SoundQProcess ();
}

//------------------------------------------------------------------------------
//eof

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

#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

#define RBA_MEDIA_CHANGED	-1

typedef struct RBACHANNELCTL {
	uint out0in, out0vol;
	uint out1in, out1vol;
	uint out2in, out2vol;
	uint out3in, out3vol;
} RBACHANNELCTL;

class CRBA {
	private:
		SDL_CD*	m_cdInfo;
		int		m_bInitialized;

	public:
		CRBA ();
		~CRBA ();
		void Init (void);	//drive a == 0, drive b == 1
		void Destroy (void);
		void RegisterCD (void);
		long GetDeviceStatus (void);
		int PlayTrack (int track);
		int PlayTracks (int first, int last);	//plays tracks first through last, inclusive
		int CheckMediaChange (void);
		long GetHeadLoc (int *min, int *sec, int *frame);
		int PeekPlayStatus (void);
		void _CDECL_ Stop (void);
		void SetStereoAudio (RBACHANNELCTL* channels);
		void SetQuadAudio (RBACHANNELCTL* channels);
		void GetAudioInfo (RBACHANNELCTL* channels);
		void SetChannelVolume (int channel, int volume);
		void SetVolume (int volume);
		int Enabled (void);
		void Disable (void);
		void Enable (void);
		int GetNumberOfTracks (void);
		void Pause (void);
		int Resume (void);

		//return the track number currently playing.  Useful if RBAPlayTracks() 
		//is called.  Returns 0 if no track playing, else track number
		int GetTrackNum (void);
		// get the cddb discid for the current cd.
		uint GetDiscID (void);

	private:
		int cddb_sum (int n);
};

extern class CRBA rba;

#endif

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
	uint32_t out0in, out0vol;
	uint32_t out1in, out1vol;
	uint32_t out2in, out2vol;
	uint32_t out3in, out3vol;
} RBACHANNELCTL;

class CRBA {
	private:
		SDL_CD*	m_cdInfo;
		int32_t		m_bInitialized;

	public:
		CRBA ();
		~CRBA ();
		void Init (void);	//drive a == 0, drive b == 1
		void Destroy (void);
		void RegisterCD (void);
		long GetDeviceStatus (void);
		int32_t PlayTrack (int32_t track);
		int32_t PlayTracks (int32_t first, int32_t last);	//plays tracks first through last, inclusive
		int32_t CheckMediaChange (void);
		long GetHeadLoc (int32_t *min, int32_t *sec, int32_t *frame);
		int32_t PeekPlayStatus (void);
		void _CDECL_ Stop (void);
		void SetStereoAudio (RBACHANNELCTL* channels);
		void SetQuadAudio (RBACHANNELCTL* channels);
		void GetAudioInfo (RBACHANNELCTL* channels);
		void SetChannelVolume (int32_t channel, int32_t volume);
		void SetVolume (int32_t volume);
		int32_t Enabled (void);
		void Disable (void);
		void Enable (void);
		int32_t GetNumberOfTracks (void);
		void Pause (void);
		int32_t Resume (void);

		//return the track number currently playing.  Useful if RBAPlayTracks() 
		//is called.  Returns 0 if no track playing, else track number
		int32_t GetTrackNum (void);
		// get the cddb discid for the current cd.
		uint32_t GetDiscID (void);

	private:
		int32_t cddb_sum (int32_t n);
};

extern class CRBA rba;

#endif

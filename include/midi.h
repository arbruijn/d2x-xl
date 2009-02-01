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

/*
 *
 * Include file for sound hardware.
 *
 */

#ifndef _MIDI_H
#define _MIDI_H

#include "pstypes.h"
#include "vecmat.h"
#include "carray.h"
#include "cfile.h"
#include "hmpfile.h"
#include "audio.h"

//------------------------------------------------------------------------------

class CMidi {
	private:
		int			m_nVolume;
		int			m_nPaused;
		Mix_Music*	m_music;
		hmp_file*	m_hmp;

	public:
		CMidi () { Init (); }
		~CMidi () { Shutdown (); }
		void Init (void);
		void Shutdown (void);
		int SetVolume (int nVolume);
		int PlaySong (char* pszSong, char* melodicBank, char* drumBank, int bLoop, int bD1Song);
		void Pause (void);
		void Resume (void);
		void Fadeout (void);
	};

extern CMidi midi;

//------------------------------------------------------------------------------

#endif


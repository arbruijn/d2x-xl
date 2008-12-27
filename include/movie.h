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

/*
 *
 * Header for movie.c
 *
 */

#ifndef _MOVIE_H
#define _MOVIE_H

//-----------------------------------------------------------------------

#define MOVIE_ABORT_ON  1
#define MOVIE_ABORT_OFF 0

#define MOVIE_NOT_PLAYED    0		// movie wasn't present
#define MOVIE_PLAYED_FULL   1		// movie was played all the way through
#define MOVIE_ABORTED       2		// movie started by was aborted

#define MOVIE_OPTIONAL		 0		//false
#define MOVIE_REQUIRED		 1		//true


#	define ENDMOVIE		"end"
#	define D1_ENDMOVIE	"final"

//-----------------------------------------------------------------------

class CMovie {
	public:
		CFile	m_cf;
		char	m_name [FILENAME_LEN];
		int	m_offset;
		int	m_len;
		int	m_pos;
		int	m_bLittleEndian;

	public:
		~CMovie () { Close (); }
		static void ShowFrame (ubyte* buf, uint bufw, uint bufh, uint sx, uint sy, uint w, uint h, uint dstx, uint dsty);
		static void SetPalette (ubyte* p, unsigned start, unsigned count);
		static void* Alloc (unsigned size);
		static void Free (void* p);
		static uint Read (void* handle, void* buf, uint count);
		void Rewind (void);
		inline void Close (void) { m_cf.Close (); }
	};

//-----------------------------------------------------------------------

class CMovieLib {
	public:
		char				m_name [100]; // [FILENAME_LEN];
		int				m_nMovies;
		ubyte				m_flags;
		ubyte				m_pad [3];
		CArray<CMovie>	m_movies;
		int				m_bLittleEndian;

	public:
		CMovieLib () { Init (); }
		~CMovieLib () { Destroy (); }
		void Init (void);
		void Destroy (void);
		CMovie* Open (char* filename, int bRequired);
		bool Setup (const char* filename);
		static CMovieLib* Create (char* filename);

	private:	
		int SetupMVL (CFile& cf);
		int SetupHF (CFile& cf);
		int Count (CFile& cf);
	};

//-----------------------------------------------------------------------

class CMovieManager {
	public:
		CArray<CMovieLib>	m_libs; // [N_MOVIE_LIBS];
		int					m_nLibs;
		CPalette*			m_palette;
		int					m_nRobots;
		int					m_bHaveIntro;
		int					m_bHaveExtras;

	private:
		int					m_nLib;
		int					m_nMovies;
		int					m_nMovie;
		CMovie*				m_robotP;

	public:
		CMovieManager () { Init (); }
		~CMovieManager () { Destroy (); }
		void Init (void);
		void InitLibs (void);
		void InitExtraRobotLib (char* filename);
		void Destroy (void);
		CMovieLib* FindLib (const char* pszLib);
		CMovieLib* Find (const char* pszMovie);
		int Run (char* filename, int bHires, int bAllowAbort, int dx, int dy);
		int Play (const char* filename, int bRequired, int bForce, int bFullScreen);
		CMovie* Open (char* filename, int bRequired);
		int RequestCD (void);
		char* Cycle (int bRestart, int bPlayMovie);
		void PlayIntro (void);
		int StartRobot (char* filename);
		int RotateRobot (void);
		void StopRobot (void);

	private:
		void CMovieManager::InitLib (const char* pszFilename, int nLibrary, int bRobotMovie, int bRequired);
};

//-----------------------------------------------------------------------
// Subtitle data

#define MAX_SUBTITLES				500

typedef struct {
	short first_frame, last_frame;
	char* msg;
} subtitle;

class CSubTitles {
	public:
		subtitle m_captions [MAX_SUBTITLES];
		int		m_nCaptions;
		ubyte*	m_rawDataP;

	public:
		int Init (const char* filename);
		void Close (void);
		void Draw (int nFrame);

	private:
		ubyte* NextField (ubyte* p);
	};

//-----------------------------------------------------------------------

extern CMovieManager movieManager;
extern CSubTitles subTitles;

//-----------------------------------------------------------------------

#endif /* _MOVIE_H */

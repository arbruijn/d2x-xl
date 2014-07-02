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
		int32_t	m_offset;
		int32_t	m_len;
		int32_t	m_pos;
		int32_t	m_bLittleEndian;

	public:
		~CMovie () { Close (); }
		static void ShowFrame (uint8_t* buf, uint32_t bufw, uint32_t bufh, uint32_t sx, uint32_t sy, uint32_t w, uint32_t h, uint32_t dstx, uint32_t dsty);
		static void SetPalette (uint8_t* p, uint32_t start, uint32_t count);
		static void* Alloc (uint32_t size);
		static void Free (void* p);
		static uint32_t Read (void* handle, void* buf, uint32_t count);
		void Rewind (void);
		inline void Close (void) { m_cf.Close (); }
	};

//-----------------------------------------------------------------------

class CMovieLib {
	public:
		char				m_name [100]; // [FILENAME_LEN];
		int32_t				m_nMovies;
		uint8_t				m_flags;
		uint8_t				m_pad [3];
		CArray<CMovie>	m_movies;
		int32_t				m_bLittleEndian;

	public:
		CMovieLib () { Init (); }
		~CMovieLib () { Destroy (); }
		void Init (void);
		void Destroy (void);
		CMovie* Open (char* filename, int32_t bRequired);
		bool Setup (const char* filename);
		static CMovieLib* Create (char* filename);

	private:	
		int32_t SetupMVL (CFile& cf);
		int32_t SetupHF (CFile& cf);
		int32_t Count (CFile& cf);
	};

//-----------------------------------------------------------------------

class CMovieManager {
	public:
		CArray<CMovieLib>	m_libs; // [N_MOVIE_LIBS];
		int32_t					m_nLibs;
		CPalette*			m_palette;
		int32_t					m_nRobots;
		int32_t					m_bHaveIntro;
		int32_t					m_bHaveExtras;

	private:
		int32_t					m_nLib;
		int32_t					m_nMovies;
		int32_t					m_nMovie;
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
		int32_t Run (char* filename, int32_t bHires, int32_t bAllowAbort, int32_t dx, int32_t dy);
		int32_t Play (const char* filename, int32_t bRequired, int32_t bForce, int32_t bFullScreen);
		CMovie* Open (char* filename, int32_t bRequired);
		int32_t RequestCD (void);
		char* Cycle (int32_t bRestart, int32_t bPlayMovie);
		void PlayIntro (void);
		int32_t StartRobot (char* filename);
		int32_t RotateRobot (void);
		void StopRobot (void);

	private:
		void InitLib (const char* pszFilename, int32_t nLibrary, int32_t bRobotMovie, int32_t bRequired);
};

//-----------------------------------------------------------------------
// Subtitle data

#define MAX_SUBTITLES				500

typedef struct {
	int16_t first_frame, last_frame;
	char* msg;
} subtitle;

class CSubTitles {
	public:
		subtitle m_captions [MAX_SUBTITLES];
		int32_t		m_nCaptions;
		uint8_t*	m_rawDataP;

	public:
		int32_t Init (const char* filename);
		void Close (void);
		void Draw (int32_t nFrame);

	private:
		uint8_t* NextField (uint8_t* p);
	};

//-----------------------------------------------------------------------

extern CMovieManager movieManager;
extern CSubTitles subTitles;

//-----------------------------------------------------------------------

#endif /* _MOVIE_H */

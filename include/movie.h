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

#define MOVIE_ABORT_ON  1
#define MOVIE_ABORT_OFF 0

#define MOVIE_NOT_PLAYED    0		// movie wasn't present
#define MOVIE_PLAYED_FULL   1		// movie was played all the way through
#define MOVIE_ABORTED       2		// movie started by was aborted

#define MOVIE_OPTIONAL		 0		//false
#define MOVIE_REQUIRED		 1		//true


#	define ENDMOVIE		"end"
#	define D1_ENDMOVIE	"final"

int PlayMovie(const char *filename, int allow_abort, int bForce, int bFullScreen);
int PlayMovies(int num_files, const char *filename[], int graphmode, int allow_abort);
int InitRobotMovie(char *filename);
int RotateRobot();
void DeInitRobotMovie(void);
void InitExtraRobotMovie(char *filename);
void PlayIntroMovie (void);

// find and initialize the movie libraries
void InitMovies();

int InitSubTitles (const char *filename);
void CloseSubTitles();

int GetNumMovieLibs (void);
int GetNumMovies (int nLib);
char *GetMovieName (int nLib, int nMovie);
char *CycleThroughMovies (int bRestart, int bPlayMovie);

extern int MovieHires;      // specifies whether movies use low or high res
extern int bDrawSubTitles;
extern int bResizeMovies;

#endif /* _MOVIE_H */

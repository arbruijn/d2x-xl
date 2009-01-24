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
 * Common types for use in Descent
 *
 */

#ifndef _PSTYPES_H
#define _PSTYPES_H

// define a dboolean
typedef int dboolean;

//define a signed byte
typedef signed char sbyte;

//define unsigned types;
typedef unsigned char ubyte;
#if defined(_WIN32)
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
#endif

#if defined(_WIN32) || defined(__sun__) // platforms missing (u_)int??_t
# include <SDL_types.h>
#endif
#ifndef __MINGW32__
#	if defined(_WIN32)// platforms missing int??_t
 		typedef Sint16 int16_t;
 		typedef Sint32 int32_t;
 		typedef Sint64 int64_t;
#	endif // defined(_WIN32)
#endif
#if 1//ndef __MINGW32__
#	if defined(_WIN32) || defined(__sun__) // platforms missing u_int??_t
 		typedef Uint16 u_int16_t;
 		typedef Uint32 u_int32_t;
 		typedef Uint64 u_int64_t;
#	endif // defined(_WIN32) || defined(__sun__)
#endif

#ifdef _WIN32
# include <stdlib.h> // this is where minand max are defined
#endif
//#ifndef min
//#	define min(a,b) (((a)>(b))?(b):(a))
//#endif
//#ifndef max
//#	define max(a,b) (((a)<(b))?(b):(a))
//#endif

#if defined(_WIN32)
# ifdef __MINGW32__
#  include <sys/types.h>
# else
#  define PATH_MAX _MAX_PATH
# endif
# define FNAME_MAX 256
#elif defined(__unix__) || defined(__macosx__)
# include <sys/types.h>
# ifndef PATH_MAX
#  define PATH_MAX 1024
# endif
# define FNAME_MAX 256
#endif

#ifdef __macosx__
#	define ushort ushort
#endif

#ifndef NULL
#	define NULL 0
#endif

// the following stuff has nothing to do with types but needed everywhere,
// and since this file is included everywhere, it's here.
#ifdef __GNUC__
# define __pack__ __attribute__((packed))
#elif defined(_WIN32)
# pragma pack(push, packing)
# pragma pack(1)
# define __pack__
#else
# error d2x will not work without packed structures
#endif

#ifdef _WIN32
# define inline __inline
#endif

#ifdef _WIN32
#	ifndef _CDECL_
#		define _CDECL_	_cdecl
#	endif
#	else
#	define _CDECL_
#endif

#endif //_PSTYPES_H


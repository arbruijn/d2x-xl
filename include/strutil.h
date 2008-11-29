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

#ifndef _STRUTILS_H
#define _STRUTILS_H

#if defined(__unix__) || defined(__macosx__)
#	include <string.h>
#	define stricmp(a,b) strcasecmp(a,b)
#	define strnicmp(a,b,c) strncasecmp(a,b,c)
#else
#	define stricmp		_stricmp
#	define strlwr		_strlwr
#	define strnicmp	_strnicmp
#	define strlwr 		_strlwr
#	define strupr 		_strupr
#	define strdup 		_strdup
#	define strrev 		_strrev
#endif

#ifndef _WIN32
#	ifndef __DJGPP__
char *strupr( char *s1 );
char *strlwr( char *s1 );
#	endif

char *strrev( char *s1 );
#endif

char *strcompress (char *str);

#if !(defined(_WIN32) && !defined(_WIN32_WCE))
void _splitpath(char *name, char *drive, char *path, char *base, char *ext);
#endif

#endif /* _STRUTILS_H */

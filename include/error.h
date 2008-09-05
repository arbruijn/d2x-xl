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

#ifndef _ERROR_H
#define _ERROR_H

#include <stdio.h>
#include "pstypes.h"
#include "gr.h"

#ifdef __GNUC__
#define __noreturn __attribute__ ((noreturn))
#define __format __attribute__ ((format (printf, 1, 2)))
#else
#define __noreturn
#define __format
#endif

int _CDECL_ error_init(void (*func)(const char *), const char *fmt,...);    //init error system, set default message, returns 0=ok
void _CDECL_ set_exit_message(const char *fmt,...);	//specify message to print at exit
void _CDECL_ Warning(const char *fmt,...);				//print out warning message to user
void SetWarnFunc(void (*f)(const char *s));//specifies the function to call with warning messages
void ClearWarnFunc(void (*f)(const char *s));//say this function no longer valid
void _Assert(int expr, const char *expr_text, const char *filename, int linenum)
#ifdef _DEBUG
void _CDECL_ Error(const char *fmt,...);				//exit with error code=1, print message
#else
void _CDECL_ Error(const char *fmt,...) __noreturn __format;				//exit with error code=1, print message
#endif
void Assert(int expr);
void _CDECL_ PrintLog (const char *fmt, ...);
void Int3();

#if 1//def _DEBUG

extern short nDbgSeg, nDbgSide, nDbgFace, nDbgObj, nDbgObjType, nDbgObjId, nDbgModel;
extern int nDbgVertex, nDbgBaseTex, nDbgOvlTex;

#endif

#ifdef _DEBUG

int TrapSeg (short nSegment);
int TrapSegSide (short nSegment, short nSide);
int TrapVert (int nVertex);
int TrapTex (int nBaseTex, int nOvlTex);
int TrapBmp (grsBitmap *bmP, char *pszName);

#else

#define TrapSeg (short nSegment)
#define TrapSegSide (short nSegment, short nSide)
#define TrapVert (int nVertex)
#define TrapTex (int nBaseTex, int nOvlTex)
#define TrapBmp (grsBitmap *bmP, char *pszName)

#endif


#ifdef _DEBUG		//macros for debugging

#	define Int3() ((void)0)
#	define Assert(expr) ((expr)?(void)0:(void)_Assert(0,#expr,__FILE__,__LINE__))

#else

#	define Int3()
#	define Assert(expr)

#endif

extern FILE *fErr;

#ifdef _WIN32
#	ifdef _DEBUG
#		define	CBP(_cond)	if (_cond) _asm int 3;
#	else
#		define	CBP(_cond)
#	endif
#else
#	define	CBP(_cond)
#endif

#endif /* _ERROR_H */

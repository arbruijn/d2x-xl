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
//#include "gr.h"

#ifdef __GNUC__
#define __noreturn __attribute__ ((noreturn))
#define __format __attribute__ ((format (printf, 1, 2)))
#else
#define __noreturn
#define __format
#endif

int32_t _CDECL_ error_init(void (*func)(const char *), const char *fmt,...);    //init error system, set default message, returns 0=ok
void _CDECL_ set_exit_message(const char *fmt,...);	//specify message to print at exit
void _CDECL_ Warning(const char *fmt,...);				//print out warning message to user
void SetWarnFunc(void (*f)(const char *s));//specifies the function to call with warning messages
void ClearWarnFunc(void (*f)(const char *s));//say this function no longer valid
void _Assert(int32_t expr, const char *expr_text, const char *filename, int32_t linenum);
#if DBG
void _CDECL_ Error(const char *fmt,...);				//exit with error code=1, print message
#else
void _CDECL_ Error(const char *fmt,...) __noreturn __format;				//exit with error code=1, print message
#endif
void Assert(int32_t expr);
void OpenLogFile (void);
void _CDECL_ PrintLog (const int32_t nIndent, const char *fmt = NULL, ...);
void TraceCallStack (const int32_t nDirection, const int32_t nLevel, const char *pszFunction, const int32_t nThread, const char *pszFile, const int32_t nLine);
void PrintCallStack (void);
void Int3();

#if 1//DBG

extern int16_t nDbgSeg, nDbgSide, nDbgFace, nDbgObj, nDbgObjType, nDbgObjId, nDbgModel, nDbgSound, nDbgChannel;
extern int32_t nDbgVertex, nDbgBaseTex, nDbgOvlTex, nDbgTexture, nDbgLight;

#endif

#define TrapSeg (int16_t nSegment)
#define TrapSegSide (int16_t nSegment, int16_t nSide)
#define TrapVert (int32_t nVertex)
#define TrapTex (int32_t nBaseTex, int32_t nOvlTex)
#define TrapBmp (CBitmap *pBm, char *pszName)

//	-----------------------------------------------------------------------------------------------------------

#if DBG		//macros for debugging

#	define Int3() ((void)0)
#	define Assert(expr) ((expr)?(void)0:(void)_Assert(0,#expr,__FILE__,__LINE__))

#else

#	define Int3()
#	define Assert(expr)

#endif

extern FILE *fLog;

void Breakpoint (void);

#if DBG
#	define BRP	Breakpoint ()
#else
#	define BRP
#endif

//	-----------------------------------------------------------------------------------------------------------

#if 1 //DBG

#ifdef _WIN32
#	define	__FUNC__	__FUNCSIG__
#else
#	define	__FUNC__	__PRETTY_FUNCTION__
#endif

#	define ENTER(_nLevel,_thread)		const int32_t __THREAD__ = (_thread); const int32_t __LEVEL__ = (_nLevel); TraceCallStack (1, __LEVEL__, __FUNC__, __THREAD__, __FILE__, __LINE__)
#	define LEAVE					{ TraceCallStack (-1, __LEVEL__, __FUNC__, __THREAD__, __FILE__, __LINE__); return; }
#	define RETURN(_retVal)		{ TraceCallStack (-1, __LEVEL__, __FUNC__, __THREAD__, __FILE__, __LINE__); return (_retVal); }

#else

#	define ENTER(_thread)				
#	define LEAVE				
#	define RETURN(_retVal)	return (_retVal);

#endif

//	-----------------------------------------------------------------------------------------------------------

#endif /* _ERROR_H */

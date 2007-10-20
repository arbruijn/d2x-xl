/* $Id: error.h,v 1.10 2003/11/26 12:26:28 btb Exp $ */
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

int _CDECL_ error_init(void (*func)(char *), char *fmt,...);    //init error system, set default message, returns 0=ok
void _CDECL_ set_exit_message(char *fmt,...);	//specify message to print at exit
void _CDECL_ Warning(char *fmt,...);				//print out warning message to user
void SetWarnFunc(void (*f)(char *s));//specifies the function to call with warning messages
void ClearWarnFunc(void (*f)(char *s));//say this function no longer valid
void _Assert(int expr,char *expr_text,char *filename,int linenum);	//assert func
void _CDECL_ Error(char *fmt,...) __noreturn __format;				//exit with error code=1, print message
void Assert(int expr);
void _CDECL_ LogErr (char *fmt, ...);
void Int3();

#ifdef _DEBUG

extern short nDbgSeg, nDbgSide;
extern int nDbgVertex, nDbgBaseTex, nDbgOvlTex;

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


#ifndef NDEBUG		//macros for debugging

#ifdef NO_ASM
//# define Int3() Error("int 3 %s:%i\n",__FILE__,__LINE__);
//# define Int3() {volatile int a=0,b=1/a;}
# define Int3() ((void)0)

#else // NO_ASM

#ifdef __GNUC__
#ifdef SDL_INPUT
#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif
#endif
#include "args.h"
static inline void _Int3()
{
	if (FindArg("-debug")) {
#ifdef SDL_INPUT
		SDL_WM_GrabInput(SDL_GRAB_OFF);
#endif
		asm("int $3");
	}
}
#define Int3() _Int3()

#elif defined __WATCOMC__
void Int3(void);								      //generate int3
#pragma aux Int3 = "int 3h";

#elif defined _MSC_VER
static __inline void _Int3()
{
	__asm { int 3 }
}
#define Int3() _Int3()

#else
#error Unknown Compiler!
#endif

#endif // NO_ASM

#define Assert(expr) ((expr)?(void)0:(void)_Assert(0,#expr,__FILE__,__LINE__))

#ifdef __GNUC__
//#define Error(format, args...) ({ /*Int3();*/ Error(format , ## args); })
#elif defined __WATCOMC__
//make error do int3, then call func
#pragma aux Error aborts = \
	"int	3"	\
	"jmp Error";

//#pragma aux Error aborts;
#else
// DPH: I'm not going to bother... it's not needed... :-)
#endif

#ifdef __WATCOMC__
//make assert do int3 (if expr false), then call func
#pragma aux _Assert parm [eax] [edx] [ebx] [ecx] = \
	"test eax,eax"		\
	"jnz	no_int3"		\
	"int	3"				\
	"no_int3:"			\
	"call _Assert";
#endif

#else					//macros for real game

#ifdef __WATCOMC__
#pragma aux Error aborts;
#endif
//Changed Assert and Int3 because I couldn't get the macros to compile -KRB
#define Assert(__ignore) ((void)0)
//void Assert(int expr);
#define Int3() ((void)0)
//void Int3();
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

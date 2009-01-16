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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "inferno.h"
#include "pstypes.h"
#include "console.h"
#include "mono.h"
#include "error.h"
#include "gr.h"
#include "text.h"
#include "cfile.h"
#include "u_mem.h"
#ifdef __macosx__
#	include "MacErrorMsg.h"
#endif

#define MAX_MSG_LEN 1024

FILE *fErr = NULL;

//edited 05/17/99 Matt Mueller added err_ prefix to prevent conflicts with statically linking SDL
int err_initialized=0;
//end edit -MM

static void (*ErrorPrintFunc) (const char *);

char szExitMsg[MAX_MSG_LEN]="";
char szWarnMsg[MAX_MSG_LEN];

void ShowInGameWarning (const char *s);

//------------------------------------------------------------------------------
//takes string in register, calls //printf with string on stack
void warn_printf(const char *s)
{
#if TRACE
	console.printf(CON_URGENT, "%s\n",s);
#endif
}

#ifdef _WIN32
void (*pWarnFunc)(const char *s) = NULL;
#else
void (*pWarnFunc)(const char *s) = warn_printf;
#endif

//------------------------------------------------------------------------------
//provides a function to call with warning messages
void SetWarnFunc(void (*f)(const char *s))
{
pWarnFunc = f;
}

//------------------------------------------------------------------------------
//uninstall warning function - install default //printf
void ClearWarnFunc (void (*f)(const char *s))
{
#ifdef _WIN32
pWarnFunc = NULL;
#else
pWarnFunc = warn_printf;
#endif
}

//------------------------------------------------------------------------------

void _CDECL_ set_exit_message(const char *fmt,...)
{
	va_list arglist;
	int len;

va_start (arglist,fmt);
len = vsprintf (szExitMsg,fmt,arglist);
va_end (arglist);
if (len==-1 || len>MAX_MSG_LEN) 
	Error("Message too long in set_exit_message (len=%d, max=%d)",len,MAX_MSG_LEN);
}

//------------------------------------------------------------------------------

void _Assert(int expr, const char *expr_text, const char *filename, int linenum)
{
if (!(expr)) {
#if defined (_DEBUG) && defined (_WIN32)
	//_asm int 3;
#else
	Int3();
#endif
	Error("Assertion failed:\n%s,\nfile %s,\nline %d",expr_text,filename,linenum);
	}
}

//------------------------------------------------------------------------------

void _CDECL_ print_exit_message(void)
{
if (*szExitMsg) {
	if (ErrorPrintFunc) {
		(*ErrorPrintFunc)(szExitMsg);
		}
	else {
#if TRACE
		console.printf(CON_CRITICAL, "%s\n",szExitMsg);
#endif
		}
	}
}

//------------------------------------------------------------------------------

#ifndef MB_ICONWARNING
#	define MB_ICONWARNING 0
#endif
#ifndef MB_ICONERROR
#	define MB_ICONERROR 0
#endif

void D2MsgBox (const char *pszMsg, uint nType)
{
gameData.app.bGamePaused = 1;
if (screen.Width () && screen.Height () && pWarnFunc)
	(*pWarnFunc)(pszMsg);
#if defined (WIN32)
else 
	MessageBox (NULL, pszMsg, "D2X-XL", nType | MB_OK);
#elif defined (__linux__)
	fprintf (stderr, "D2X-XL: %s\n", pszMsg);
#elif defined (__macosx__)
	NativeMacOSXMessageBox (pszMsg);
#endif
gameData.app.bGamePaused = 0;
}

//------------------------------------------------------------------------------
//terminates with error code 1, printing message
void _CDECL_ Error (const char *fmt,...)
{
	va_list arglist;

#if !DBG
strcpy (szExitMsg, TXT_TITLE_ERROR); // don't put the new line in for dialog output
#else
sprintf (szExitMsg, "\n%s", TXT_TITLE_ERROR);
#endif
va_start (arglist,fmt);
vsprintf (szExitMsg + strlen (szExitMsg), fmt, arglist);
va_end(arglist);
PrintLog ("ERROR: %s\n", szExitMsg);
gameStates.app.bShowError = 1;
D2MsgBox (szExitMsg, MB_ICONERROR);
gameStates.app.bShowError = 0;
Int3();
if (!err_initialized) 
	print_exit_message();
#if !DBG
exit (1);
#endif
}

//------------------------------------------------------------------------------

void _CDECL_ PrintLog (const char *fmt, ...)
{
 if (fErr) {
		va_list arglist;
		static char	szErr [100000];
    
	va_start (arglist, fmt);
	vsprintf (szErr, fmt, arglist);
	va_end (arglist);
	fprintf (fErr, szErr);
	fflush (fErr);
	}
}

//------------------------------------------------------------------------------
//print out warning message to user
void _CDECL_ Warning (const char *fmt, ...)
{
	va_list arglist;

*szWarnMsg = '\0';
va_start (arglist, fmt);
vsprintf (szWarnMsg + strlen (szWarnMsg), fmt, arglist);
va_end (arglist);
	//PrintLog (szWarnMsg);
gameStates.app.bShowError = 1;
D2MsgBox (szWarnMsg, MB_ICONWARNING);
gameStates.app.bShowError = 0;
}

//------------------------------------------------------------------------------
//initialize error handling system, and set default message. returns 0=ok
int _CDECL_ error_init (void (*func)(const char *), const char *fmt, ...)
{
	va_list arglist;
	int len;

atexit(print_exit_message);		//last thing at exit is print message
ErrorPrintFunc = func;          // Set Error Print Functions
if (fmt != NULL) {
	va_start(arglist,fmt);
	len = vsprintf(szExitMsg,fmt,arglist);
	va_end(arglist);
	if (len==-1 || len>MAX_MSG_LEN) 
		Error("Message too long in error_init (len=%d, max=%d)",len,MAX_MSG_LEN);
	}
err_initialized=1;
return 0;
}

//------------------------------------------------------------------------------

#if 1//def _DEBUG

short nDbgSeg = -1;
short nDbgSide = -1;
short nDbgFace = -1;
short nDbgObj = -1;
short nDbgObjType = -1;
short nDbgObjId = -1;
short nDbgModel = -1;
int nDbgVertex = -1;
int nDbgBaseTex = -1;
int nDbgOvlTex = -1;
int nDbgTexture = -1;

#endif

#if DBG

int TrapSeg (short nSegment)
{
if (nSegment = nDbgSeg)
	return 1;
return 0;
}


int TrapSegSide (short nSegment, short nSide)
{
if ((nSegment = nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	return 1;
return 0;
}

int TrapVert (int nVertex)
{
if (nVertex == nDbgVertex)
	return 1;
return 0;
}


int TrapBmp (CBitmap *bmP, char *pszName)
{
if (strstr (bmP->Name (), pszName))
	return 1;
return 0;
}


int TrapTex (int nBaseTex, int nOvlTex)
{
if (((nBaseTex < 0) || (nBaseTex == nDbgBaseTex)) && ((nDbgOvlTex < 0) || (nOvlTex == nDbgOvlTex)))
	return 1;
return 0;
}

#endif

//------------------------------------------------------------------------------
//eof

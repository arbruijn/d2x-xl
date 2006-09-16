/* $Id: error.c,v 1.6 2003/04/08 00:59:17 btb Exp $ */
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
 * Error handling/printing/exiting code
 *
 * Old Log:
 * Revision 1.12  1994/12/07  18:49:39  matt
 * error_init() can now take NULL as parm
 *
 * Revision 1.11  1994/11/29  15:42:07  matt
 * Added newline before error message
 *
 * Revision 1.10  1994/11/27  23:20:39  matt
 * Made changes for new con_printf calling convention
 *
 * Revision 1.9  1994/06/20  21:20:56  matt
 * Allow NULL for warn func, to kill warnings
 *
 * Revision 1.8  1994/05/20  15:11:35  mike
 * con_printf Warning message so you can actually see it.
 *
 * Revision 1.7  1994/02/10  18:02:38  matt
 * Changed 'if DEBUG_ON' to 'ifndef NDEBUG'
 *
 * Revision 1.6  1993/10/17  18:19:10  matt
 * If error_init() not called, Error() now prints the error message before
 * calling exit()
 *
 * Revision 1.5  1993/10/14  15:29:11  matt
 * Added new function clear_warn_func()
 *
 * Revision 1.4  1993/10/08  16:17:19  matt
 * Made Assert() call function _Assert(), rather to do 'if...' inline.
 *
 * Revision 1.3  1993/09/28  12:45:25  matt
 * Fixed wrong print call, and made Warning() not append a CR to string
 *
 * Revision 1.2  1993/09/27  11:46:35  matt
 * Added function SetWarnFunc()
 *
 * Revision 1.1  1993/09/23  20:17:33  matt
 * Initial revision
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: error.c,v 1.6 2003/04/08 00:59:17 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "pstypes.h"
#include "console.h"
#include "mono.h"
#include "error.h"
#include "gr.h"
#include "inferno.h"
#include "text.h"
#include "cfile.h"
#ifdef __macosx__
#	include "MacErrorMsg.h"
#endif

#define MAX_MSG_LEN 1024

FILE *fErr = NULL;

//edited 05/17/99 Matt Mueller added err_ prefix to prevent conflicts with statically linking SDL
int err_initialized=0;
//end edit -MM

static void (*ErrorPrintFunc)(char *);

char szExitMsg[MAX_MSG_LEN]="";
char szWarnMsg[MAX_MSG_LEN];

extern void ShowInGameWarning(char *s);

//------------------------------------------------------------------------------
//takes string in register, calls //printf with string on stack
void warn_printf(char *s)
{
#if TRACE
	con_printf(CON_URGENT, "%s\n",s);
#endif
}

#ifdef _WIN32
void (*pWarnFunc)(char *s) = NULL;
#else
void (*pWarnFunc)(char *s) = warn_printf;
#endif

//------------------------------------------------------------------------------
//provides a function to call with warning messages
void SetWarnFunc(void (*f)(char *s))
{
	pWarnFunc = f;
}

//------------------------------------------------------------------------------
//uninstall warning function - install default //printf
void clear_warn_func(void (*f)(char *s))
{
#ifdef _WIN32
pWarnFunc = NULL;
#else
pWarnFunc = warn_printf;
#endif
}

//------------------------------------------------------------------------------

void _CDECL_ set_exit_message(char *fmt,...)
{
	va_list arglist;
	int len;

va_start(arglist,fmt);
len = vsprintf(szExitMsg,fmt,arglist);
va_end(arglist);
if (len==-1 || len>MAX_MSG_LEN) 
	Error("Message too long in set_exit_message (len=%d, max=%d)",len,MAX_MSG_LEN);
}

//------------------------------------------------------------------------------

void _Assert(int expr,char *expr_text,char *filename,int linenum)
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
		con_printf(CON_CRITICAL, "%s\n",szExitMsg);
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

void D2MsgBox (char *pszMsg, unsigned int nType)
{
if (grdCurScreen && pWarnFunc)
	(*pWarnFunc)(pszMsg);
#if defined (WIN32)
else
	MessageBox (NULL, pszMsg, "D2X-XL", nType | MB_OK);
#elif defined (__linux__)
	fprintf (stderr, "D2X-XL: %s\n", pszMsg);
#elif defined (__macosx__)
	NativeMacOSXMessageBox (pszMsg);
#endif
}

//------------------------------------------------------------------------------
//terminates with error code 1, printing message
void _CDECL_ Error (char *fmt,...)
{
	va_list arglist;

#if (defined(NDEBUG) && defined(RELEASE))
strcpy(szExitMsg, TXT_TITLE_ERROR); // don't put the new line in for dialog output
#else
sprintf (szExitMsg, "\n%s", TXT_TITLE_ERROR);
#endif
va_start (arglist,fmt);
vsprintf (szExitMsg + strlen (szExitMsg), fmt, arglist);
va_end(arglist);
LogErr ("ERROR: %s\n", szExitMsg);
D2MsgBox (szExitMsg, MB_ICONERROR);
Int3();
if (!err_initialized) 
	print_exit_message();
#ifdef RELEASE
exit (2);
#endif
}

//------------------------------------------------------------------------------

void _CDECL_ LogErr (char *fmt, ...)
{
 if (fErr) {
		va_list arglist;
		static char	szErr [1024];
    
	va_start (arglist, fmt);
	vsprintf (szErr, fmt, arglist);
	va_end (arglist);
	fprintf (fErr, szErr);
	fflush (fErr);
	}
}

//------------------------------------------------------------------------------
//print out warning message to user
void _CDECL_ Warning (char *fmt, ...)
{
	va_list arglist;

*szWarnMsg = '\0';
va_start (arglist, fmt);
vsprintf (szWarnMsg + strlen (szWarnMsg), fmt, arglist);
va_end (arglist);
	//LogErr (szWarnMsg);
D2MsgBox (szWarnMsg, MB_ICONWARNING);
}

//------------------------------------------------------------------------------
//initialize error handling system, and set default message. returns 0=ok
int _CDECL_ error_init(void (*func)(char *), char *fmt, ...)
{
	va_list arglist;
	int len;

atexit(print_exit_message);		//last thing at exit is print message
ErrorPrintFunc = func;          // Set Error Print Functions
if (fmt != NULL) {
	va_start(arglist,fmt);
	len = vsprintf(szExitMsg,fmt,arglist);
	va_end(arglist);
	if (len==-1 || len>MAX_MSG_LEN) Error("Message too long in error_init (len=%d, max=%d)",len,MAX_MSG_LEN);
	}
err_initialized=1;
return 0;
}

//------------------------------------------------------------------------------
//eof
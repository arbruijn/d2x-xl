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

#include "descent.h"
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

int nLogDate = 0;

void ShowInGameWarning (const char *s);

//------------------------------------------------------------------------------

void ArrayError (const char* pszMsg)
{
PrintLog (pszMsg);
}

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

#if 1 //defined(__unix__)

#include <Xm/Xm.h>
#include <Xm/MwmUtil.h>
#include <Xm/MainW.h>
#include <Xm/CascadeB.h>
#include <Xm/MessageB.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>

//------------------------------------------------------------------------------

Boolean SetCloseCallBack (Widget shell, void (*callback) (Widget, XtPointer, XtPointer))
{
extern Atom XmInternAtom (Display *, char *, Boolean);

if (!shell)
	return False;
Atom disp = XtDisplay (shell);
if (!disp)
	return False;
// Retrieve Window Manager Protocol Property 
Atom prop = XmInternAtom (disp, "WM_PROTOCOLS", False);
if (!prop)
	return False;
// Retrieve Window Manager Delete Window Property
Atom prot = XmInternAtom (disp, "WM_DELETE_WINDOW", True);
if (!prot)
	return False;
// Ensure that Shell has the Delete Window Property 
// NB: Necessary since some Window managers are not 
// Fully XWM Compilant (olwm for instance is not)   
XmAddProtocols (shell, prop, &prot, 1);
// Now add our callback into the Protocol Callback List 
XmAddProtocolCallback (shell, prop, prot, callback, NULL);
return True;
}

//------------------------------------------------------------------------------

static XtAppContext	appShell;
static int bCloseMsgBox = 0;

void XmCloseMsgBox (Widget w, XtPointer clientData, XtPointer callData)
{
XtAppSetExitFlag (appShell);
}

//------------------------------------------------------------------------------

int _CDECL_ MsgBoxThread (void *pThreadId)
{
XtAppMainLoop (appShell);
return 0;
}

//------------------------------------------------------------------------------

void MsgBoxEvents (Widget w, XtPointer clientData, XEvent* event, Boolean *continueToDispatch)
{
*continueToDispatch = 1;
}

//------------------------------------------------------------------------------

void XmMessageBox (const char* pszMsg, bool bError)
{
#if 1

 /* initialize */
Widget topWid = XtVaAppInitialize (&appShell, bError ? "Error" : "Warning", NULL, 0, &gameData.app.argC, gameData.app.argV, NULL, NULL);
//Widget msgBox = XtVaCreateManagedWidget ("d2x-xl-msgbox", xmMainWindowWidgetClass, topWid, /* XmNscrollingPolicy, XmVARIABLE, */NULL);
// Create ScrolledText -- this is work area for the MainWindow
Arg args [4];
XtSetArg (args [0], XmNrows,      30);
XtSetArg (args [1], XmNcolumns,   121);
XtSetArg (args [2], XmNeditable,  False);
XtSetArg (args [3], XmNeditMode,  XmMULTI_LINE_EDIT);
Widget msgBox = XmCreateScrolledText (topWid, const_cast<char*>("d2x-xl-msg"), args, 4);
#if 0
Widget menuBar = XmCreateMenuBar (msgBox, const_cast<char*>("d2x-xl-menu"), NULL, 0);
XtManageChild (menuBar);
Widget closeWid = XtVaCreateManagedWidget ("Close", xmCascadeButtonWidgetClass, menuBar, XmNmnemonic, 'C', NULL);
XtAddCallback (closeWid, XmNactivateCallback, XmCloseMsgBox, NULL);
#endif
XtAddCallback (msgBox, XmNdestroyCallback, XmCloseMsgBox, NULL);
XtManageChild (msgBox);
XmTextSetString (msgBox       , const_cast<char*>(pszMsg));
#if 0
int decor;
XtVaGetValues (msgBox, XmNmwmDecorations, &decor, NULL);
XtVaSetValues (msgBox, XmNmwmDecorations, decor & ~(MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE | MWM_DECOR_MENU), NULL);
#else
XtVaSetValues (topWid, XmNmwmDecorations, MWM_DECOR_BORDER | MWM_DECOR_TITLE, NULL);
XtVaSetValues (msgBox, XmNmwmDecorations, MWM_DECOR_BORDER | MWM_DECOR_TITLE, NULL);
#endif
SetCloseCallBack (topWid, XmCloseMsgBox);
SetCloseCallBack (msgBox, XmCloseMsgBox);

#else

XmString xmString = XmStringCreateLocalized (const_cast<char*>(pszMsg));
Widget topWid = XtVaAppInitialize (&appShell, "D2X-XL", NULL, 0, &gameData.app.argC, gameData.app.argV, NULL, NULL);
// setup message box text
Arg args [1];
XtSetArg (args [0], XmNmessageString, xmString);
// create and label message box
Widget xMsgBox = bError
				 ? XmCreateErrorDialog (topWid, const_cast<char*>("Error"), args, 1)
				 : XmCreateWarningDialog (topWid, const_cast<char*>("Warning"), args, 1);
// remove text resource
XmStringFree (xmString);
// remove help and cancel buttons
XtUnmanageChild (XmMessageBoxGetChild (xMsgBox, XmDIALOG_CANCEL_BUTTON));
XtUnmanageChild (XmMessageBoxGetChild (xMsgBox, XmDIALOG_HELP_BUTTON));
XtAddCallback (xMsgBox, XmNokCallback, XmCloseMsgBox, NULL);
XtManageChild (xMsgBox);

#endif

bCloseMsgBox = 0;
XtRealizeWidget (topWid);
// display message box
SDL_Thread* threadP = SDL_CreateThread (MsgBoxThread, NULL);
while (!XtAppGetExitFlag (appShell))
	G3_SLEEP (0);
XtUnrealizeWidget (topWid);
XtDestroyApplicationContext (appShell);
if (bCloseMsgBox)
	SDL_KillThread (threadP);
}

#endif

//------------------------------------------------------------------------------

#ifndef MB_ICONWARNING
#	define MB_ICONWARNING	0
#endif
#ifndef MB_ICONERROR
#	define MB_ICONERROR		1
#endif

void D2MsgBox (const char *pszMsg, uint nType)
{
gameData.app.bGamePaused = 1;
if (screen.Width () && screen.Height () && pWarnFunc)
	(*pWarnFunc)(pszMsg);
#if defined (_WIN32)
else
	MessageBox (NULL, pszMsg, "D2X-XL", nType | MB_OK);
#elif defined(__linux__)
#	if 1
	XmMessageBox (pszMsg, nType == MB_ICONERROR);
#	else
	fprintf (stderr, "D2X-XL: %s\n", pszMsg);
#	endif
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

static void SetLogDate (void)
{
   time_t      t;
   struct tm*	h;

time (&t);
h = localtime (&t);
nLogDate = int (h->tm_year + 1900) * 65536 + int (h->tm_mon) * 256 + h->tm_mday;
}

//------------------------------------------------------------------------------
//initialize error handling system, and set default message. returns 0=ok
int _CDECL_ error_init (void (*func)(const char *), const char *fmt, ...)
{
	va_list arglist;
	int len;

atexit (print_exit_message);		//last thing at exit is print message
SetLogDate ();
ErrorPrintFunc = func;          // Set Error Print Functions
if (fmt != NULL) {
	va_start (arglist,fmt);
	len = vsprintf (szExitMsg, fmt, arglist);
	va_end (arglist);
	if ((len == -1) || (len > MAX_MSG_LEN))
		PrintLog ("Message too long in error_init (len=%d, max=%d)", len, MAX_MSG_LEN);
	}
err_initialized = 1;
return 0;
}

//------------------------------------------------------------------------------

#if 1//DBG

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
if (nSegment == nDbgSeg)
	return 1;
return 0;
}


int TrapSegSide (short nSegment, short nSide)
{
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
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

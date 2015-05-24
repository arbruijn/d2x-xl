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

FILE *fLog = NULL;

//edited 05/17/99 Matt Mueller added err_ prefix to prevent conflicts with statically linking SDL
int32_t err_initialized=0;
//end edit -MM

static void (*ErrorPrintFunc) (const char *);

char szExitMsg[MAX_MSG_LEN]="";
char szWarnMsg[MAX_MSG_LEN];

int32_t nLogDate = 0;
int32_t glHWHash = 0;

#ifdef __unix__
#	define LINUX_MSGBOX	0
#else
#	define LINUX_MSGBOX	0
#endif

//------------------------------------------------------------------------------

void ArrayError (const char* pszMsg)
{
PrintLog (0, pszMsg);
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
	int32_t len;

va_start (arglist,fmt);
len = vsprintf (szExitMsg,fmt,arglist);
va_end (arglist);
if (len==-1 || len>MAX_MSG_LEN)
	Error("Message too long in set_exit_message (len=%d, max=%d)",len,MAX_MSG_LEN);
}

//------------------------------------------------------------------------------

void _Assert(int32_t expr, const char *expr_text, const char *filename, int32_t linenum)
{
if (!(expr)) {
#if defined (_DEBUG) && defined (_WIN32)
	//_asm int32_t 3;
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

#if LINUX_MSGBOX

#include <Xm/Xm.h>
#include <Xm/MwmUtil.h>
#include <Xm/MainW.h>
#include <Xm/CascadeB.h>
#include <Xm/MessageB.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/PushBG.h>
#include <Xm/LabelG.h>
#include <Xm/PanedW.h>
#include <Xm/Text.h>
#include <Xm/DialogS.h>
#include <Xm/Command.h>
#include <Xm/Protocols.h>

static XtAppContext		appShell;

//------------------------------------------------------------------------------
// intercept the window manager's close signal

#if 0

Boolean SetCloseCallBack (Widget shell, void (*callback) (Widget, XtPointer, XtPointer))
{
extern Atom XmInternAtom (Display *, char *, Boolean);

if (!shell)
	return False;
Display* disp = XtDisplay (shell);
if (!disp)
	return False;
// Retrieve Window Manager Protocol Property
Atom prop = XmInternAtom (disp, const_cast<char*>("WM_PROTOCOLS"), False);
if (!prop)
	return False;
// Retrieve Window Manager Delete Window Property
Atom prot = XmInternAtom (disp, const_cast<char*>("WM_DELETE_WINDOW"), True);
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

#endif

//------------------------------------------------------------------------------

static int32_t MsgSize (char* pszMsg, int32_t& nCols)
{
if (!(pszMsg && *pszMsg))
	return 0;
int32_t nRows = 1;
nCols = 0;
for (char* p = pszMsg; *p && (pszMsg = strchr (p, '\n')); nRows++, p = ++pszMsg) {
	if (nCols < pszMsg - p)
		nCols = pszMsg - p;
	}
return nRows;
}

//------------------------------------------------------------------------------
// The callback function for the "OK" button. Since this is not a
// predefined Motif dialog, the "widget" parameter is not the dialog
// itself. That is only done by Motif dialog callbacks. Here in the
// real world, the callback routine is called directly by the widget
// that was invoked. Thus, we must pass the dialog as the client
// data to get its handle. (We could get it using GetTopShell(),
// but this way is quicker, since it's immediately available.)

void DestroyShell (Widget widget, XtPointer clientData, XtPointer callData)
{
Widget shell = (Widget) clientData;
XtDestroyWidget (shell);
XtAppSetExitFlag (appShell);
}

//------------------------------------------------------------------------------
// Build a dialog containing a scrollable, non editable text widget and a close button.
// Taken from the Motif programmer's manual and slightly adapted (no icon, single button),
// minimal window decoration).

Widget XmMessageDialog (const char* pszMsg, int32_t nRows, int32_t nCols, bool bError)
{
    Widget       msgBox, pane, msgText, form, /*sep, label,*/ widget;
    void         DestroyShell(Widget, XtPointer, XtPointer);
    //Pixmap       pixmap;
    Property          args [10];
    int32_t          n = 0;
    int32_t          i;
    Dimension    h;
// Set up a DialogShell as a popup window. Set the delete window protocol response to XmDESTROY to make sure that
// the window goes away appropriately. Otherwise, it's XmUNMAP which means it'd be lost forever, since we're not storing
// the widget globally or statically to this function.
Widget topWid = XtVaAppInitialize (&appShell, "D2X-XL", NULL, 0, &gameData.app.argC, gameData.app.argV, NULL, NULL);
XtSetArg (args [0], XmNdeleteResponse, XmDESTROY);
msgBox = XmCreateDialogShell (topWid, bError ? const_cast<char*>("Error") : const_cast<char*>("Warning"), args, 1);
XtVaGetValues (msgBox, XmNmwmDecorations, &i, NULL);
i &= ~(MWM_DECOR_ALL | MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE | MWM_DECOR_MENU);
XtVaSetValues (msgBox, XmNmwmDecorations, i, NULL);
XtVaGetValues (msgBox, XmNmwmFunctions, &i, NULL);
i &= ~(MWM_FUNC_ALL | MWM_FUNC_MINIMIZE | MWM_FUNC_MAXIMIZE | MWM_FUNC_CLOSE);
XtVaSetValues (msgBox, XmNmwmFunctions, i, NULL);
// Create a PanedWindow to manage the stuff in this dialog. PanedWindow won't let us set these to 0!
XtSetArg (args [0], XmNsashWidth, 1);
// Make small so user doesn't try to resize
XtSetArg (args [1], XmNsashHeight, 1);
pane = XmCreatePanedWindow (msgBox, const_cast<char*>("pane"), args, 2);
// Create a RowColumn in the form for Label and Text widgets. This is the control area.
form = XmCreateForm (pane, const_cast<char*>("form1"), NULL, 0);

#if 0
Pixel fg, bg;
XtVaGetValues (form, /* once created, we can get its colors */
					XmNforeground, &fg,
					XmNbackground, &bg,
					NULL);
// create the pixmap of the appropriate depth using the colors that will be used by the parent (form).
pixmap = XCreatePixmapFromBitmapData (
	 XtDisplay (form),
	 RootWindowOfScreen (XtScreen (form)),
		(char *) info_bits, info_width, info_height,
		fg, bg,
		DefaultDepthOfScreen (XtScreen (form)));
// Create a label gadget using this pixmap
n = 0;
XtSetArg (args [n], XmNlabelType, XmPIXMAP); n++;
XtSetArg (args [n], XmNlabelPixmap, pixmap); n++;
XtSetArg (args [n], XmNleftAttachment, XmATTACH_FORM); n++;
XtSetArg (args [n], XmNtopAttachment, XmATTACH_FORM); n++;
XtSetArg (args [n], XmNbottomAttachment, XmATTACH_FORM); n++;
label = XmCreateLabelGadget (form, "label", args, n);
XtManageChild (label);
#endif

// prepare the text for display in the ScrolledText object we are about to create.
n = 0;
XtSetArg (args [n], XmNscrollVertical, True); n++;
XtSetArg (args [n], XmNscrollHorizontal, False); n++;
XtSetArg (args [n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
XtSetArg (args [n], XmNeditable, False); n++;
XtSetArg (args [n], XmNcursorPositionVisible, False); n++;
XtSetArg (args [n], XmNwordWrap, True); n++;
XtSetArg (args [n], XmNvalue, pszMsg); n++;
XtSetArg (args [n], XmNrows, Min (nRows, 30)); n++;
XtSetArg (args [n], XmNcolumns, Min (nCols, 120)); n++;
msgText = XmCreateScrolledText (form, const_cast<char*>("help_text"), args, n);
// Attachment values must be set on the Text widget's PARENT, the ScrolledWindow. This is the object that is positioned.
XtVaSetValues (XtParent (msgText),
			   XmNleftAttachment, XmATTACH_FORM,
			   XmNtopAttachment, XmATTACH_FORM,
			   XmNrightAttachment, XmATTACH_FORM,
			   XmNbottomAttachment, XmATTACH_FORM,
			   NULL);
XtManageChild (msgText);
XtManageChild (form);
// Create another form to act as the action area for the dialog
XtSetArg (args [0], XmNfractionBase, 5);
form = XmCreateForm (pane, const_cast<char*>("form2"), args, 1);
// The OK button is under the pane's separator and is attached to the left edge of the form. It spreads from
// position 0 to 1 along the bottom (the form is split into 5 separate grids via XmNfractionBase upon creation).
widget = XmCreatePushButtonGadget (form, const_cast<char*>("Close"), NULL, 0);
XtVaSetValues (widget,
			   XmNtopAttachment, XmATTACH_FORM,
			   XmNbottomAttachment, XmATTACH_FORM,
			   XmNleftAttachment, XmATTACH_POSITION,
			   XmNleftPosition, 2,
			   XmNrightAttachment, XmATTACH_POSITION,
			   XmNrightPosition, 3,
			   XmNshowAsDefault, True,
			   XmNdefaultButtonShadowThickness, 1,
			   NULL);
XtManageChild (widget);
XtAddCallback (widget, XmNactivateCallback, DestroyShell, (XtPointer) msgBox);
/* Fix the action area pane to its current height -- never let it resize */
XtManageChild (form);
XtVaGetValues (widget, XmNheight, &h, NULL);
XtVaSetValues (form, XmNpaneMaximum, h, XmNpaneMinimum, h, NULL);
// This also pops up the dialog, as it is the child of a DialogShell
XtManageChild (pane);
return topWid;
}

//------------------------------------------------------------------------------

void XmCloseMsgBox (Widget w, XtPointer clientData, XtPointer callData)
{
XtAppSetExitFlag (appShell);
}

//------------------------------------------------------------------------------

void XmMessageBox (const char* pszMsg, bool bError)
{
	Widget	topWid;
	int32_t		nRows, nCols;

nRows = MsgSize (const_cast<char*>(pszMsg), nCols);
if ((nRows > 3) || (nCols > 360))
	topWid = XmMessageDialog (pszMsg, nRows, nCols, bError);
else { // use the built-in message box
	topWid = XtVaAppInitialize (&appShell, "D2X-XL", NULL, 0, &gameData.app.argC, gameData.app.argV, NULL, NULL);
	// setup message box text
	Property args [1];
	XmString xmString = XmStringCreateLocalized (const_cast<char*>(pszMsg));
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
	// add callback to the "close" button that signals closing of the message box
	XtAddCallback (xMsgBox, XmNokCallback, XmCloseMsgBox, NULL);
	XtManageChild (xMsgBox);
	XtRealizeWidget (topWid);
	}
XtAppMainLoop (appShell);
XtUnrealizeWidget (topWid);
XtDestroyApplicationContext (appShell);
}

#endif //LINUX_MSGBOX

//------------------------------------------------------------------------------

#ifndef MB_ICONWARNING
#	define MB_ICONWARNING	0
#endif
#ifndef MB_ICONERROR
#	define MB_ICONERROR		1
#endif

void D2MsgBox (const char *pszMsg, uint32_t nType)
{
gameData.app.bGamePaused = 1;
if (gameData.render.screen.Width () && gameData.render.screen.Height () && pWarnFunc)
	(*pWarnFunc)(pszMsg);
#if defined (_WIN32)
else
	MessageBox (NULL, pszMsg, "D2X-XL", nType | MB_OK);
#elif defined(__linux__)
#	if LINUX_MSGBOX
	if (gameStates.app.bLinuxMsgBox)
		XmMessageBox (pszMsg, nType == MB_ICONERROR);
	else
#	endif
	PrintLog (0, "D2X-XL: %s\n", pszMsg);
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
PrintLog (0, "ERROR: %s\n", szExitMsg);
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

static int32_t nLogIndent = 0;
static int32_t nLastIndent = 0;

void IndentLog (int32_t nIndent)
{
if (nIndent) {
	if (abs (nIndent) == 1)
		nIndent *= 3; // default indentation
	nLastIndent = abs (nIndent);
	}
nLogIndent += nIndent;
if (nLogIndent < 0) {
	PrintLog (0, "Log indentation error!\n");
	nLogIndent = 0;
	}
else if (nLogIndent > 30) {
	PrintLog (0, "Log indentation error!\n");
	nLogIndent = 30;
	}
}

//------------------------------------------------------------------------------

int32_t SetIndent (int32_t nIndent)
{
int32_t nOldIndent = nLogIndent;
nLogIndent = nIndent;
return nOldIndent;
}

//------------------------------------------------------------------------------

#ifdef _WIN32
#	include "share.h"
#endif

void OpenLogFile (void)
{
if (*gameFolders.user.szCache && (gameStates.app.nLogLevel > 0)) {
	if (CFile::Exist ("d2x.log.bak", gameFolders.user.szCache, 0))
		CFile::Delete ("d2x.log.bak", gameFolders.user.szCache);
	if (CFile::Exist ("d2x.log", gameFolders.user.szCache, 0))
		CFile::Rename ("d2x.log", "d2x.log.bak", gameFolders.user.szCache);

	char fnErr [FILENAME_LEN];
	sprintf (fnErr, "%sd2x.log", gameFolders.user.szCache);
#ifdef _WIN32
	fLog = _fsopen (fnErr, "wt", _SH_DENYWR);
#else
	fLog = fopen (fnErr, "wt");
#endif
	}
}

//------------------------------------------------------------------------------

void _CDECL_ PrintLog (const int32_t nIndent, const char *fmt, ...)
{
if (fLog && (gameStates.app.nTraceLevel < 0)) {
	if (fmt && *fmt) {
		va_list arglist;
			static char	szLogLine [2][100000] = {{'\0'}, {'\0'}};
			static int32_t nLogLine = 0;

		va_start (arglist, fmt);
		if (nLogIndent > 0)
			memset (szLogLine [nLogLine], ' ', nLogIndent);
		else if (nLogIndent < 0)
			nLogIndent = 0;
		nLogLine &= 3;
		vsprintf (szLogLine [nLogLine] + nLogIndent, fmt, arglist);
		va_end (arglist);
#if 0
		if (strstr (szLogLine [nLogLine], "transformation"))
#endif
		if (strcmp (szLogLine [nLogLine], szLogLine [!nLogLine])) {
			fprintf (fLog, szLogLine [nLogLine]);
			fflush (fLog);
			nLogLine = !nLogLine;
			}
		}
	if (nIndent)
		IndentLog (nIndent);
	}
}

//------------------------------------------------------------------------------

void _CDECL_ StackTrace (const int32_t nLevel, const int32_t nIndent, const char *fmt, ...)
{
#pragma omp critical
if ((nLevel <= gameStates.app.nTraceLevel) && fmt && *fmt) {
	va_list arglist;
	static char	szLogLine [100000] = {'\0'};

	va_start (arglist, fmt);
	vsprintf (szLogLine, fmt, arglist);
	va_end (arglist);
	int32_t nTraceLevel = gameStates.app.nTraceLevel;
	gameStates.app.nTraceLevel = -1;
	if (nIndent < 0)
		PrintLog (nIndent);
	PrintLog (0, szLogLine);
	if (nIndent > 0)
		PrintLog (nIndent);
	gameStates.app.nTraceLevel = nTraceLevel;
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
//PrintLog (0, szWarnMsg);
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
nLogDate = int32_t (h->tm_year + 1900) * 65536 + int32_t (h->tm_mon) * 256 + h->tm_mday;
glHWHash = ~nLogDate;
}

//------------------------------------------------------------------------------
//initialize error handling system, and set default message. returns 0=ok
int32_t _CDECL_ error_init (void (*func)(const char *), const char *fmt, ...)
{
	va_list arglist;
	int32_t len;

atexit (print_exit_message);		//last thing at exit is print message
SetLogDate ();
ErrorPrintFunc = func;          // Set Error Print Functions
if (fmt != NULL) {
	va_start (arglist,fmt);
	len = vsprintf (szExitMsg, fmt, arglist);
	va_end (arglist);
	if ((len == -1) || (len > MAX_MSG_LEN))
		PrintLog (0, "Message too long in error_init (len=%d, max=%d)", len, MAX_MSG_LEN);
	}
err_initialized = 1;
return 0;
}

//------------------------------------------------------------------------------

#if 1//DBG

int16_t nDbgSeg = -1;
int16_t nDbgSide = -1;
int16_t nDbgFace = -1;
int16_t nDbgObj = -1;
int16_t nDbgObjType = -1;
int16_t nDbgObjId = -1;
int16_t nDbgModel = -1;
int32_t nDbgVertex = -1;
int32_t nDbgBaseTex = -1;
int32_t nDbgOvlTex = -1;
int32_t nDbgTexture = -1;
int16_t nDbgSound = -1;
int16_t nDbgChannel = -1;
int32_t nDbgLight = -1;

#endif

//------------------------------------------------------------------------------

void Breakpoint (void)
{
PrintLog (0, NULL);
}

//------------------------------------------------------------------------------
//eof

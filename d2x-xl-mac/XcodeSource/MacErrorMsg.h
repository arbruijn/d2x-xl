/******************************************************************************
 *
 *	File:			MacErrorMsg.h
 *
 *	Function:		Declarations for MacErrorMsg.mm
 *
 *	Author(s):		Created by Simone Manganelli on 2006-08-02.
 *
 *	Copyright:		Public Domain
 *
 *	Source:			Original.
 *
 *	Notes:			Similar to FolderDetector.h/.mm. Although MacErrorMsg.mm
 *                  is Objective C++ this is really a C header, and is used once,
 *                  in error.cpp. The interface section is pushed
 *					into MacErrorMsg.mm to allow MacErrorMsg.h to compile
 *					in the rest of the (non-Mac) code.
 *
 *	Change History:
 *			2013_12_04_itaylo	Standard format.
 *	
 ******************************************************************************/

#ifndef MACERRORMSG_H_INCLUDED
#define MACERRORMSG_H_INCLUDED

void NativeMacOSXMessageBox(const char *pszMsg);

#endif // MACERRORMSG_H_INCLUDED

// EOF - MacErrorMsg.h

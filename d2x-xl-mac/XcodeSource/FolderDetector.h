/******************************************************************************
 *
 *	File:			FolderDetector.h
 *
 *	Function:		Declarations for FolderDetector.mm
 *
 *	Author(s):		Created by Simone Manganelli on 2006-01-09.
 *
 *	Copyright:		Public Domain
 *
 *	Source:			Original.
 *
 *	Notes:			Although Folder Detector.mm is Objective C++ this
 *					is really a C header. The interface section is pushed
 *					into FolderDetector.mm to allow FolderDetector.h to compile
 *					in the rest of the (non-Mac) code.
 *                  Similar to MacErrorMsg.h/.mm.
 *
 *	Change History:
 *			2013_12_04_itaylo	Standard format.
 *	
 ******************************************************************************/

#ifndef FOLDERDETECTOR_H_INCLUDED
#define FOLDERDETECTOR_H_INCLUDED

char *GetMacOSXCacheFolder();
void GetOSXAppFolder(char* szDataRootDir, char* szGameDir); 

#endif // FOLDERDETECTOR_H_INCLUDED

// EOF - FolderDetector.h

/******************************************************************************
 *
 *	File:			FolderDetector.mm
 *
 *	Function:		Sets up cache and other folders for D2X-XL
 *
 *	Author(s):		Simone Manganelli
 *
 *	Copyright:		Public Domain
 *
 *	Source:			Created by Simone Manganelli on 2006-01-09.
 *
 *	Notes:			Most recent update replaces deprecated
 *					Mac OS X API Calls.
 *
 *                  Really C code as there is no alloc or init for the object
 *                  and the methods are used as C function calls. But ObjC calls
 *                  are made into Foundation. So a bit of a mix!
 *
 *	Change History:
 *			2013_11_13 itaylo	Updated Mac OS X API calls.
 *	
 *****************************************************************************/

#import "FolderDetector.h"
#import <Foundation/Foundation.h>

#include "cfile.h"
#include "error.h"

@interface FolderDetector : NSObject {	
}

// Only used in this file - so leave them here.

char *GetContainingFolder();
char *GetMacOSXCacheFolder();
char *CheckForMacOSXFolders();

@end

@implementation FolderDetector

// We want to make sure that the cache folder exists, so we check to see if it's there,
// and create it if it isn't
//
// This method should ideally be called only once, otherwise it will check for the existence
// of  the cache folders and create them every time it is called

char *CheckForMacOSXFolders() 
{	
	NSFileManager *manager = [NSFileManager defaultManager];
	NSString *returnString = nil;
	
	NSString *appContainingFolderPath
        = [[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent];
    
	// So if the app lives at /Games/D2X-XL/d2x-xl.app,
    // for example, appContainingFolderPath would be /Games/D2X-XL/
	NSString *executableContainingFolderPath
        = [[[NSBundle mainBundle] executablePath] stringByDeletingLastPathComponent];

	// Here, if the app lives at /Games/D2X-XL/d2x-xl.app, executableContainingFolderPath
    // would be /Games/D2X-XL/d2x-xl.app/Contents/MacOS/	
	NSString *testPath
        = [appContainingFolderPath stringByAppendingPathComponent:@"data/descent2.hog"];
	if ([manager fileExistsAtPath:testPath])
	{
		// the Descent 2 data files are inside /Games/D2X-XL/data/, as per the above example
		returnString = testPath;
	}
	else 
	{
		testPath = [appContainingFolderPath stringByAppendingPathComponent:@"data/data/descent2.hog"];
		if ([manager fileExistsAtPath:testPath])
		{
			// Descent 2 data files live inside /Games/D2X-XL/data/data/ this exists so you
			// can collect all the other folders (e.g. config, textures, movies, but not the missions folder)
			// in one big data folder
			// 'cause it's kind of annoying to have them all in your main game folder
            
			returnString = testPath;	
		}
		else 
		{
			testPath = [executableContainingFolderPath stringByAppendingPathComponent:@"data/descent2.hog"];
			if ([manager fileExistsAtPath:testPath])
            {
				// Descent 2 data files live inside /Games/D2X-XL/d2x-xl.app/Contents/MacOS/data/
				// this is useful if you want to package up d2x-xl so no datafiles are ever visible to the user
                
				returnString = testPath;		
			} 
			else 
			{
				testPath = [executableContainingFolderPath stringByAppendingPathComponent:@"data/data/descent2.hog"];
				if ([manager fileExistsAtPath:testPath]) 
				{
					// Descent 2 data files live inside /Games/D2X-XL/d2x-xl.app/Contents/MacOS/data/data/
					// this is useful if you want to package up d2x-xl so no datafiles are ever visible to the user
                    
					returnString = testPath;
				}
			}
		}
	}
	
	if (returnString == nil) 
	{
		return (char *)"-1";
	} 
	else 
	{
		return (char *)[[[returnString stringByDeletingLastPathComponent] stringByDeletingLastPathComponent] UTF8String];
	}
}

char *GetContainingFolder()
{
	return (char *)[[[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent] UTF8String];
}

// void GetOSXAppFolder(char* szDataRootDir, char* szGameDir)
// {
// 	if (! *szGameDir) 
// 	{
// 		char *containingFolder = GetContainingFolder();
// 		strcpy(szGameDir, containingFolder);
// 	}
// 
// 	char *path = CheckForMacOSXFolders ();
// 	if (strcmp (path, "-1") == 0)
// 		strcpy (szDataRootDir, szGameDir);
// 	else
// 		strcpy (szDataRootDir, path);
// }



void GetOSXAppFolder(char* szDataRootDir, char* szGameDir)
{
	if (! *szGameDir) {
		char *containingFolder = GetContainingFolder();
		strcpy(szGameDir, containingFolder);
	}
    
	char *path = CheckForMacOSXFolders ();
	if (strcmp (path, "-1") != 0)
		strcpy (szDataRootDir, path);
	else if (szDataRootDir != szGameDir) // leave untouched if identical
		strcpy (szDataRootDir, szGameDir);
	// LogErr ("expected Mac OS X data root folder = '%s'\n", szDataRootDir);	itaylo 11MAY2008
}

char *GetMacOSXCacheFolder()
{
	BOOL isDirectory = NO;
	NSString *folderTestString;
	NSFileManager *manager = [NSFileManager defaultManager];
	
	// Check if the global caches folder exists and create it if it doesn't
    folderTestString = [@"~/Library/Caches" stringByExpandingTildeInPath];
    
	BOOL dirExists = [manager fileExistsAtPath:folderTestString isDirectory:&isDirectory];
	if (! (dirExists && isDirectory)) 
	{
        [manager createDirectoryAtPath:folderTestString withIntermediateDirectories:YES attributes:nil error:nil];
	}
	
	// Check if the D2X-XL-specific cache folder exists and create it if it doesn't
    folderTestString = [@"~/Library/Caches/D2X-XL" stringByExpandingTildeInPath];
	dirExists = [manager fileExistsAtPath:folderTestString isDirectory:&isDirectory];
	
	if (! (dirExists && isDirectory)) 
	{
        [manager createDirectoryAtPath:folderTestString withIntermediateDirectories:YES attributes:nil error:nil];
	}
	
	return (char *)[[@"~/Library/Caches/D2X-XL" stringByExpandingTildeInPath] UTF8String];
}

@end

// EOF - FolderDetector.mm


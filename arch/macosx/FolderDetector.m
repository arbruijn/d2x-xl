//
//  FolderDetector.m
//  d2x-xl
//
//  Created by Simone Manganelli on 2006-01-09.
//

#import "FolderDetector.h"
#import <Foundation/Foundation.h>

#define __obj_c 1

#include "cfile.h"
#include "error.h"


@interface FolderDetector : NSObject {
	
}

char *ReturnContainingFolder();
char *CheckForMacOSXFolders();

@end

@implementation FolderDetector

char *ReturnContainingFolder() {
	return [[[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent] cString];
}

char *CheckForMacOSXFolders() {	
	NSFileManager *manager = [NSFileManager defaultManager];
	NSString *returnString = nil;
	
	NSString *appContainingFolderPath = [[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent];
	// so if the app lives at /Games/D2X-XL/d2x-xl.app, for example, appContainingFolderPath would be /Games/D2X-XL/
	
	NSString *executableContainingFolderPath = [[[NSBundle mainBundle] executablePath] stringByDeletingLastPathComponent];
	// here, if the app lives at /Games/D2X-XL/d2x-xl.app, executableContainingFolderPath would be /Games/D2X-XL/d2x-xl.app/Contents/MacOS/
	
	NSString *testPath = [appContainingFolderPath stringByAppendingPathComponent:@"data/descent2.hog"];
	if ([manager fileExistsAtPath:testPath]) {
		// the Descent 2 data files are inside /Games/D2X-XL/data/, as per the above example
		returnString = testPath;
		
		
	
	} else {
		testPath = [appContainingFolderPath stringByAppendingPathComponent:@"data/data/descent2.hog"];
		if ([manager fileExistsAtPath:testPath]) {
			// Descent 2 data files live inside /Games/D2X-XL/data/data/
			// this exists so you can collect all the other folders (e.g. config, textures, movies, but not the missions folder) in one big data folder
			// 'cause it's kind of annoying to have them all in your main game folder
			returnString = testPath;
			
			
			
			
			
		} else {
			testPath = [executableContainingFolderPath stringByAppendingPathComponent:@"data/descent2.hog"];
			if ([manager fileExistsAtPath:testPath]) {
				// Descent 2 data files live inside /Games/D2X-XL/d2x-xl.app/Contents/MacOS/data/
				// this is useful if you want to package up d2x-xl so no datafiles are ever visible to the user
				returnString = testPath;
				
				
				
				
				
			} else {
				testPath = [executableContainingFolderPath stringByAppendingPathComponent:@"data/data/descent2.hog"];
				if ([manager fileExistsAtPath:testPath]) {
				// Descent 2 data files live inside /Games/D2X-XL/d2x-xl.app/Contents/MacOS/data/data/
				// this is useful if you want to package up d2x-xl so no datafiles are ever visible to the user
					returnString = testPath;
				}
			}
		}
	}
	
	
	if (returnString == nil) {
		return "-1";
	} else {
		return [[[returnString stringByDeletingLastPathComponent] stringByDeletingLastPathComponent] cString];
	}
}

@end



void GetOSXAppFolder(char* szDataRootDir, char* szGameDir)
{
	if (! *szGameDir) {
		char *containingFolder = ReturnContainingFolder();
		strcpy(szGameDir, containingFolder);
	}
	// check for the data in '../../..', '../../../data', './' and './data'
	char *path = CheckForMacOSXFolders ();
	if (strcmp (path, "-1") == 0)
		strcpy (szDataRootDir, szGameDir);
	else
		strcpy (szDataRootDir, path);
	LogErr ("expected Mac OS X data root folder = '%s'\n", szDataRootDir);	
}

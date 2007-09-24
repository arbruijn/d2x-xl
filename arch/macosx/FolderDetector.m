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
	//NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	return [[[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent] cString];
	//[pool release];
}

char *CheckForMacOSXFolders() {	
	//NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	// check for the data in '../../..', '../../../data', './' and './data'
	
	NSFileManager *manager = [NSFileManager defaultManager];
	
	NSString *testPath = [[[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent] stringByAppendingPathComponent:@"descent2.hog"];
	if ([manager fileExistsAtPath:testPath]) {
	// the data dir is at "../../.." in relation to the executable file
		return [[testPath stringByDeletingLastPathComponent] cString];
	} else {
		testPath = [[[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent] stringByAppendingPathComponent:@"data/descent2.hog"];
		if ([manager fileExistsAtPath:testPath]) {
		// the data dir is at "../../../data" in relation to the executable file
			return [[testPath stringByDeletingLastPathComponent] cString];
		} else {
			testPath = [[[[NSBundle mainBundle] executablePath] stringByDeletingLastPathComponent] stringByAppendingPathComponent:@"descent2.hog"];
			if ([manager fileExistsAtPath:testPath]) {
			// the data dir is at "./" in relation to the executable file
				return [[testPath stringByDeletingLastPathComponent] cString];
			} else {
				testPath = [[[[NSBundle mainBundle] executablePath] stringByDeletingLastPathComponent] stringByAppendingPathComponent:@"data/descent2.hog"];
				if ([manager fileExistsAtPath:testPath]) {
				// the data dir is at "./Data" in relation to the executable file
					return [[testPath stringByDeletingLastPathComponent] cString];
				} else {
				// the data is not in any of the Mac OS X-specific locations
					return "-1";
				}
			}
		}
	}
	
	//[pool release];
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

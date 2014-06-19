/******************************************************************************
 *
 *	File:			MacErrorMsg.mm
 *
 *	Function:		Error message routine D2X-XL
 *
 *	Author(s):		Simone Manganelli
 *
 *	Copyright:		Public Domain
 *
 *	Source:			Created by Simone Manganelli on 2006-08-02.
 *
 *	Notes:			Most recent update replaces deprecated
 *					Mac OS X API Calls.
 *
 *                  Really C code as there is no alloc or init for the object
 *                  and the methods are used as C function calls. But ObjC calls
 *                  are made into Cocoa. So a bit of a mix!
 *
 *	Change History:
 *			2013_11_13 itaylo	Updated Mac OS X API Calls.
 *
 *****************************************************************************/

#import "MacErrorMsg.h"
#import <Cocoa/Cocoa.h>

#include "cfile.h"
#include "error.h"

@interface MacErrorMsg : NSObject {
    
}

@end

@implementation MacErrorMsg

void NativeMacOSXMessageBox(const char *pszMsg)
{
    NSRunAlertPanel(@"D2X-XL has encountered an error",[NSString stringWithUTF8String:pszMsg],@"OK",nil,nil);
}

@end

// EOF - MacErrorMsg.mm


//
//  MacErrorMsg.m
//  d2x-xl
//
//  Created by Simone Manganelli on 2006-08-02.
//

#import "MacErrorMsg.h"
#import <Cocoa/Cocoa.h>

#define __obj_c 1

#include "cfile.h"
#include "error.h"

@interface MacErrorMsg : NSObject {

}

@end

@implementation MacErrorMsg

void NativeMacOSXMessageBox(const char *pszMsg) {
NSRunAlertPanel(@"D2X-XL has encountered an error",[NSString stringWithCString:pszMsg],@"OK",nil,nil);
}

@end

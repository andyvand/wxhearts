// macOS Help Book launcher. Called from the Help menu handler.
// Uses NSHelpManager / showHelp: so the bundled wxhearts.help book
// is opened in Help Viewer, keyed off CFBundleHelpBookName in the
// app's Info.plist.

// Work around a Homebrew e2fsprogs-libs conflict: /usr/local/include/uuid/uuid.h
// (shipped by Homebrew's libuuid) defines the _UUID_UUID_H guard but does
// NOT define Apple's uuid_string_t. When the macOS SDK's hfs_format.h
// (pulled in via AppKit -> CoreServices -> CarbonCore -> HFSVolumes.h)
// then does #include <uuid/uuid.h>, the guard short-circuits Apple's
// real header and uuid_string_t stays undefined, breaking the build.
//
// Pre-define the typedef ourselves so the symbol is visible regardless
// of which uuid/uuid.h the preprocessor picks up first.
#include <sys/_types.h>
#ifndef _UUID_STRING_T
#define _UUID_STRING_T
typedef __darwin_uuid_string_t uuid_string_t;
#endif

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

extern "C" void ShowMacHelpBook(const char *anchor)
{
    @autoreleasepool {
        NSString *book = [[NSBundle mainBundle]
            objectForInfoDictionaryKey:@"CFBundleHelpBookName"];

        NSHelpManager *mgr = [NSHelpManager sharedHelpManager];

        if (anchor && *anchor && book) {
            [mgr openHelpAnchor:[NSString stringWithUTF8String:anchor]
                         inBook:book];
        } else {
            [[NSApplication sharedApplication] showHelp:nil];
        }
    }
}

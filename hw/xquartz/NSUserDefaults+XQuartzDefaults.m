//
//  NSUserDefaults+XQuartzDefaults.m
//  XQuartz
//
//  Created by Jeremy Huddleston Sequoia on 2021.02.19.
//  Copyright (c) 2021 Apple Inc. All rights reserved.
//

#include "NSUserDefaults+XQuartzDefaults.h"
#include "osxcompat.h"

#ifdef HAS_LIBDISPATCH
#include <dispatch/dispatch.h>
#else
#include <pthread.h>
#endif

NSString * const XQuartzPrefKeyAppsMenu = @"apps_menu";
NSString * const XQuartzPrefKeyFakeButtons = @"enable_fake_buttons";
NSString * const XQuartzPrefKeyFakeButton2 = @"fake_button2";
NSString * const XQuartzPrefKeyFakeButton3 = @"fake_button3";
NSString * const XQuartzPrefKeyKeyEquivs = @"enable_key_equivalents";
NSString * const XQuartzPrefKeyFullscreenHotkeys = @"fullscreen_hotkeys";
NSString * const XQuartzPrefKeyFullscreenMenu = @"fullscreen_menu";
NSString * const XQuartzPrefKeySyncKeymap = @"sync_keymap";
NSString * const XQuartzPrefKeyDepth = @"depth";
NSString * const XQuartzPrefKeyNoAuth = @"no_auth";
NSString * const XQuartzPrefKeyNoTCP = @"nolisten_tcp";
NSString * const XQuartzPrefKeyDoneXinitCheck = @"done_xinit_check";
NSString * const XQuartzPrefKeyNoQuitAlert = @"no_quit_alert";
NSString * const XQuartzPrefKeyNoRANDRAlert = @"no_randr_alert";
NSString * const XQuartzPrefKeyOptionSendsAlt = @"option_sends_alt";
NSString * const XQuartzPrefKeyAppKitModifiers = @"appkit_modifiers";
NSString * const XQuartzPrefKeyWindowItemModifiers = @"window_item_modifiers";
NSString * const XQuartzPrefKeyRootless = @"rootless";
NSString * const XQuartzPrefKeyRENDERExtension = @"enable_render_extension";
NSString * const XQuartzPrefKeyTESTExtension = @"enable_test_extensions";
NSString * const XQuartzPrefKeyLoginShell = @"login_shell";
NSString * const XQuartzPrefKeyUpdateFeed = @"update_feed";
NSString * const XQuartzPrefKeyClickThrough = @"wm_click_through";
NSString * const XQuartzPrefKeyFocusFollowsMouse = @"wm_ffm";
NSString * const XQuartzPrefKeyFocusOnNewWindow = @"wm_focus_on_new_window";

NSString * const XQuartzPrefKeyScrollInDeviceDirection = @"scroll_in_device_direction";
NSString * const XQuartzPrefKeySyncPasteboard = @"sync_pasteboard";
NSString * const XQuartzPrefKeySyncPasteboardToClipboard = @"sync_pasteboard_to_clipboard";
NSString * const XQuartzPrefKeySyncPasteboardToPrimary = @"sync_pasteboard_to_primary";
NSString * const XQuartzPrefKeySyncClipboardToPasteBoard = @"sync_clipboard_to_pasteboard";
NSString * const XQuartzPrefKeySyncPrimaryOnSelect = @"sync_primary_on_select";

/* Helper functions, part of removing Apple blocks. */
static void
globalDefaultsOnce(void *arg)
{
    NSUserDefaults **defaults = arg;
    NSString * const defaultsDomain = @".GlobalPreferences";
    *defaults = [[[NSUserDefaults alloc] initWithSuiteName:defaultsDomain] retain];

    NSMutableDictionary *defaultDefaultsDict = [[NSMutableDictionary alloc] init];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:YES] forKey:@"AppleSpacesSwitchOnActivate"];

    [*defaults registerDefaults:defaultDefaultsDict];
}

static void
dockDefaultsOnce(void *arg)
{
    NSUserDefaults **defaults = arg;
    NSString * const defaultsDomain = @"com.apple.dock";
    *defaults = [[[NSUserDefaults alloc] initWithSuiteName:defaultsDomain] retain];

    NSMutableDictionary *defaultDefaultsDict = [[NSMutableDictionary alloc] init];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:NO] forKey:@"workspaces"];

    [*defaults registerDefaults:defaultDefaultsDict];
}

static void
xquartzDefaultsOnce(void *arg)
{
    NSUserDefaults **defaults = (NSUserDefaults **)arg;
    NSString *const defaultsDomain = [NSString stringWithFormat:@"%s.X11", BUNDLE_ID_PREFIX];
    NSString *const defaultDefaultsDomain = [[NSBundle mainBundle] bundleIdentifier];

    if ([defaultsDomain isEqualToString:defaultDefaultsDomain]) {
        *defaults = [[NSUserDefaults standardUserDefaults] retain];
    } else {
        *defaults = [[[NSUserDefaults alloc] initWithSuiteName:defaultsDomain] retain];
    }

    NSString *defaultWindowItemModifiers = @"command";
    NSString * const defaultWindowItemModifiersLocalized =
        NSLocalizedString(@"window item modifiers", @"window item modifiers");

    if (![defaultWindowItemModifiersLocalized isEqualToString:@"window item modifiers"]) {
        defaultWindowItemModifiers = defaultWindowItemModifiersLocalized;
    }

    NSMutableDictionary *defaultDefaultsDict = [[NSMutableDictionary alloc] init];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:NO]   forKey:XQuartzPrefKeyFakeButtons];
    // XQuartzPrefKeyFakeButton2 and 3 left as nil (no setObject)
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:YES]  forKey:XQuartzPrefKeyKeyEquivs];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:NO]   forKey:XQuartzPrefKeyFullscreenHotkeys];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:NO]   forKey:XQuartzPrefKeyFullscreenMenu];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:NO]   forKey:XQuartzPrefKeySyncKeymap];
    [defaultDefaultsDict setObject:[NSNumber numberWithInt:-1]    forKey:XQuartzPrefKeyDepth];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:NO]   forKey:XQuartzPrefKeyNoAuth];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:NO]   forKey:XQuartzPrefKeyNoTCP];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:NO]   forKey:XQuartzPrefKeyDoneXinitCheck];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:NO]   forKey:XQuartzPrefKeyNoQuitAlert];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:NO]   forKey:XQuartzPrefKeyNoRANDRAlert];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:NO]   forKey:XQuartzPrefKeyOptionSendsAlt];
    // XQuartzPrefKeyAppKitModifiers is nil
    [defaultDefaultsDict setObject:defaultWindowItemModifiers     forKey:XQuartzPrefKeyWindowItemModifiers];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:YES]  forKey:XQuartzPrefKeyRootless];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:YES]  forKey:XQuartzPrefKeyRENDERExtension];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:NO]   forKey:XQuartzPrefKeyTESTExtension];
    [defaultDefaultsDict setObject:@"/bin/sh"                     forKey:XQuartzPrefKeyLoginShell];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:NO]   forKey:XQuartzPrefKeyClickThrough];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:NO]   forKey:XQuartzPrefKeyFocusFollowsMouse];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:YES]  forKey:XQuartzPrefKeyFocusOnNewWindow];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:NO]   forKey:XQuartzPrefKeyScrollInDeviceDirection];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:YES]  forKey:XQuartzPrefKeySyncPasteboard];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:YES]  forKey:XQuartzPrefKeySyncPasteboardToClipboard];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:YES]  forKey:XQuartzPrefKeySyncPasteboardToPrimary];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:YES]  forKey:XQuartzPrefKeySyncClipboardToPasteBoard];
    [defaultDefaultsDict setObject:[NSNumber numberWithBool:NO]   forKey:XQuartzPrefKeySyncPrimaryOnSelect];

    [*defaults registerDefaults:defaultDefaultsDict];
    [defaultDefaultsDict release];

    NSString * const systemDefaultsPlistPath = [[NSString stringWithUTF8String:XQUARTZ_DATA_DIR] stringByAppendingPathComponent:@"defaults.plist"];
    NSDictionary * const systemDefaultsDict = [NSDictionary dictionaryWithContentsOfFile:systemDefaultsPlistPath];
    [*defaults registerDefaults:systemDefaultsDict];
}

#ifdef HAS_LIBDISPATCH
    typedef dispatch_once_t compat_once_t;
    #define COMPAT_ONCE_INIT 0
    #define compat_once_f dispatch_once_f
#else
    typedef pthread_once_t compat_once_t;
    #define COMPAT_ONCE_INIT PTHREAD_ONCE_INIT
    static inline void compat_once_f(compat_once_t *once, void *context, void (*func)(void *)) {
        // pthread_once only takes a void(*)(void), so wrap context in a static
        static void *compat_context;
        static void (*compat_func)(void *);
        compat_context = context;
        compat_func = func;
        void wrapper(void) { compat_func(compat_context); }
        pthread_once(once, wrapper);
    }
#endif

@implementation NSUserDefaults (XQuartzDefaults)

+ (NSUserDefaults *)globalDefaults
{
    static compat_once_t once = COMPAT_ONCE_INIT;
    static NSUserDefaults *defaults;

    compat_once_f(&once, &defaults, globalDefaultsOnce);
    return defaults;
}

+ (NSUserDefaults *)dockDefaults
{
    static compat_once_t once = COMPAT_ONCE_INIT;
    static NSUserDefaults *defaults;

    compat_once_f(&once, &defaults, dockDefaultsOnce);
    return defaults;
}

+ (NSUserDefaults *)xquartzDefaults
{
    static compat_once_t once = COMPAT_ONCE_INIT;
    static NSUserDefaults *defaults;

    compat_once_f(&once, &defaults, xquartzDefaultsOnce);
    return defaults;
}

@end

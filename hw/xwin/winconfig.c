/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors: Alexander Gottwald	
 */
#include <xwin-config.h>

#include "win.h"
#include "winconfig.h"
#include "winmsg.h"
#include "globals.h"

#include "xkbsrv.h"

WinCmdlineRec g_cmdline = {
    NULL,                       /* fontPath */
    NULL,                       /* xkbRules */
    NULL,                       /* xkbModel */
    NULL,                       /* xkbLayout */
    NULL,                       /* xkbVariant */
    NULL,                       /* xkbOptions */
    NULL,                       /* screenname */
    NULL,                       /* mousename */
    FALSE,                      /* emulate3Buttons */
    0                           /* emulate3Timeout */
};

winInfoRec g_winInfo = {
    {                           /* keyboard */
     0,                         /* leds */
     500,                       /* delay */
     30                         /* rate */
     }
    ,
    {                           /* xkb */
     NULL,                      /* rules */
     NULL,                      /* model */
     NULL,                      /* layout */
     NULL,                      /* variant */
     NULL,                      /* options */
     }
    ,
    {
     FALSE,
     50}
};

#define NULL_IF_EMPTY(x) (winNameCompare(x,"")?x:NULL)

/* load layout definitions */
#include "winlayouts.h"

/* Set the keyboard configuration */
Bool
winConfigKeyboard(DeviceIntPtr pDevice)
{
    char layoutName[KL_NAMELENGTH];
    unsigned char layoutFriendlyName[256];
    unsigned int layoutNum = 0;
    unsigned int deviceIdentifier = 0;
    int keyboardType;
    MessageType from = X_DEFAULT;
    char *s = NULL;

    /* Setup defaults */
    XkbGetRulesDflts(&g_winInfo.xkb);

    /*
     * Query the windows autorepeat settings and change the xserver defaults.
     */
    {
        int kbd_delay;
        DWORD kbd_speed;

        if (SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &kbd_delay, 0) &&
            SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &kbd_speed, 0)) {
            switch (kbd_delay) {
            case 0:
                g_winInfo.keyboard.delay = 250;
                break;
            case 1:
                g_winInfo.keyboard.delay = 500;
                break;
            case 2:
                g_winInfo.keyboard.delay = 750;
                break;
            default:
            case 3:
                g_winInfo.keyboard.delay = 1000;
                break;
            }
            g_winInfo.keyboard.rate = (kbd_speed > 0) ? kbd_speed : 1;
            winMsg(X_PROBED, "Setting autorepeat to delay=%ld, rate=%ld\n",
                   g_winInfo.keyboard.delay, g_winInfo.keyboard.rate);

        }
    }

    keyboardType = GetKeyboardType(0);
    if (keyboardType > 0 && GetKeyboardLayoutName(layoutName)) {
        WinKBLayoutPtr pLayout;
        Bool bfound = FALSE;
        int pass;

        layoutNum = strtoul(layoutName, (char **) NULL, 16);
        if ((layoutNum & 0xffff) == 0x411) {
            if (keyboardType == 7) {
                /* Japanese layouts have problems with key event messages
                   such as the lack of WM_KEYUP for Caps Lock key.
                   Loading US layout fixes this problem. */
                if (LoadKeyboardLayout("00000409", KLF_ACTIVATE) != NULL)
                    winMsg(X_INFO, "Loading US keyboard layout.\n");
                else
                    winMsg(X_ERROR, "LoadKeyboardLayout failed.\n");
            }
        }

        /* Discover the friendly name of the current layout */
        {
            HKEY regkey = NULL;
            const char regtempl[] =
                "SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\";
            DWORD namesize = sizeof(layoutFriendlyName);

            char *regpath = calloc(1, sizeof(regtempl) + KL_NAMELENGTH + 1);
            strcpy(regpath, regtempl);
            strcat(regpath, layoutName);

            if (!RegOpenKey(HKEY_LOCAL_MACHINE, regpath, &regkey))
                RegQueryValueEx(regkey, "Layout Text", 0, NULL,
                                layoutFriendlyName, &namesize);

            /* Close registry key */
            if (regkey)
                RegCloseKey(regkey);
            free(regpath);
        }

        winMsg(X_PROBED,
               "Windows keyboard layout: \"%s\" (%08x) \"%s\", type %d\n",
               layoutName, layoutNum, layoutFriendlyName, keyboardType);

        deviceIdentifier = layoutNum >> 16;
        for (pass = 0; pass < 2; pass++) {
            /* If we didn't find an exact match for the input locale identifier,
               try to find an match on the language identifier part only  */
            if (pass == 1)
                layoutNum = (layoutNum & 0xffff);

            for (pLayout = winKBLayouts; pLayout->winlayout != -1; pLayout++) {
                if (pLayout->winlayout != layoutNum)
                    continue;
                if (pLayout->winkbtype > 0 && pLayout->winkbtype != keyboardType)
                    continue;

                bfound = TRUE;
                winMsg(X_PROBED,
                       "Found matching XKB configuration \"%s\"\n",
                       pLayout->layoutname);

                winMsg(X_PROBED,
                       "Model = \"%s\" Layout = \"%s\""
                       " Variant = \"%s\" Options = \"%s\"\n",
                       pLayout->xkbmodel ? pLayout->xkbmodel : "none",
                       pLayout->xkblayout ? pLayout->xkblayout : "none",
                       pLayout->xkbvariant ? pLayout->xkbvariant : "none",
                       pLayout->xkboptions ? pLayout->xkboptions : "none");

                /* need the typecast to (char*) in order to silence const warning */
                g_winInfo.xkb.model = (char*)pLayout->xkbmodel;
                g_winInfo.xkb.layout = (char*)pLayout->xkblayout;
                g_winInfo.xkb.variant = (char*)pLayout->xkbvariant;
                g_winInfo.xkb.options = (char*)pLayout->xkboptions;

                if (deviceIdentifier == 0xa000) {
                    winMsg(X_PROBED, "Windows keyboard layout device identifier indicates Macintosh, setting Model = \"macintosh\"");
                    g_winInfo.xkb.model = (char*)"macintosh";
                }

                break;
            }

            if (bfound)
                break;
        }

        if (!bfound) {
            winMsg(X_ERROR,
                   "Keyboardlayout \"%s\" (%s) is unknown, using X server default layout\n",
                   layoutFriendlyName, layoutName);
        }
    }

    /* parse the configuration */
    s = NULL;
    if (g_cmdline.xkbRules) {
        s = g_cmdline.xkbRules;
        from = X_CMDLINE;
    }

    if (s) {
        g_winInfo.xkb.rules = NULL_IF_EMPTY(s);
        winMsg(from, "XKB: rules: \"%s\"\n", s);
    }

    s = NULL;
    if (g_cmdline.xkbModel) {
        s = g_cmdline.xkbModel;
        from = X_CMDLINE;
    }

    if (s) {
        g_winInfo.xkb.model = NULL_IF_EMPTY(s);
        winMsg(from, "XKB: model: \"%s\"\n", s);
    }

    s = NULL;
    if (g_cmdline.xkbLayout) {
        s = g_cmdline.xkbLayout;
        from = X_CMDLINE;
    }

    if (s) {
        g_winInfo.xkb.layout = NULL_IF_EMPTY(s);
        winMsg(from, "XKB: layout: \"%s\"\n", s);
    }

    s = NULL;
    if (g_cmdline.xkbVariant) {
        s = g_cmdline.xkbVariant;
        from = X_CMDLINE;
    }

    if (s) {
        g_winInfo.xkb.variant = NULL_IF_EMPTY(s);
        winMsg(from, "XKB: variant: \"%s\"\n", s);
    }

    s = NULL;
    if (g_cmdline.xkbOptions) {
        s = g_cmdline.xkbOptions;
        from = X_CMDLINE;
    }

    if (s) {
        g_winInfo.xkb.options = NULL_IF_EMPTY(s);
        winMsg(from, "XKB: options: \"%s\"\n", s);
    }

    return TRUE;
}

Bool
winConfigFiles(void)
{
    /* Fontpath */
    if (g_cmdline.fontPath) {
        defaultFontPath = g_cmdline.fontPath;
        winMsg(X_CMDLINE, "FontPath set to \"%s\"\n", defaultFontPath);
    }

    return TRUE;
}

Bool
winConfigOptions(void)
{
    return TRUE;
}

Bool
winConfigScreens(void)
{
    return TRUE;
}

/*
 * Compare two strings for equality. This is caseinsensitive  and
 * The characters '_', ' ' (space) and '\t' (tab) are treated as
 * not existing.
 */

int
winNameCompare(const char *s1, const char *s2)
{
    char c1, c2;

    if (!s1 || *s1 == 0) {
        if (!s2 || *s2 == 0)
            return 0;
        else
            return 1;
    }

    while (*s1 == '_' || *s1 == ' ' || *s1 == '\t')
        s1++;
    while (*s2 == '_' || *s2 == ' ' || *s2 == '\t')
        s2++;

    c1 = (isupper((int) *s1) ? tolower((int) *s1) : *s1);
    c2 = (isupper((int) *s2) ? tolower((int) *s2) : *s2);

    while (c1 == c2) {
        if (c1 == 0)
            return 0;
        s1++;
        s2++;

        while (*s1 == '_' || *s1 == ' ' || *s1 == '\t')
            s1++;
        while (*s2 == '_' || *s2 == ' ' || *s2 == '\t')
            s2++;

        c1 = (isupper((int) *s1) ? tolower((int) *s1) : *s1);
        c2 = (isupper((int) *s2) ? tolower((int) *s2) : *s2);
    }
    return c1 - c2;
}

char *
winNormalizeName(const char *s)
{
    char *q;
    const char *p;

    if (s == NULL)
        return NULL;

    char *ret = calloc(1, strlen(s) + 1);
    for (p = s, q = ret; *p != 0; p++) {
        switch (*p) {
        case '_':
        case ' ':
        case '\t':
            continue;
        default:
            if (isupper((int) *p))
                *q++ = tolower((int) *p);
            else
                *q++ = *p;
        }
    }
    *q = '\0';
    return ret;
}

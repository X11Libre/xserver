#define HOOK_NAME "initroot"

#include <dix-config.h>

#include <inttypes.h>
#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>

#include "dix/window_priv.h"

#include "namespace.h"
#include "hooks.h"

static inline int setWinStrProp(WindowPtr pWin, Atom name, const char *text) {
    return dixChangeWindowProperty(serverClient, pWin, name, XA_STRING,
                                   8, PropModeReplace, strlen(text), text, TRUE);
}

void hookInitRootWindow(CallbackListPtr *pcbl, void *data, void *screen)
{
    ScreenPtr pScreen = (ScreenPtr)screen;

    // only act on first screen
    if (pScreen->myNum)
        return;

    /* create the virtual root windows */
    WindowPtr realRoot = pScreen->root;

    assert(realRoot);

    struct Xnamespace *walk;

    xorg_list_for_each_entry(walk, &ns_list, entry) {
        if (strcmp(walk->name, NS_NAME_ROOT)==0) {
            walk->rootWindow = realRoot;
            XNS_LOG("<%s> actual root 0x%0" PRIx32 "\n", walk->name, walk->rootWindow->drawable.id);
            continue;
        }

        int rc = 0;
        WindowPtr pWin = dixCreateWindow(
            FakeClientID(0), realRoot, 0, 0, 23, 23,
            0, /* bw */
            InputOutput,
            0, /* vmask */
            NULL, /* vlist */
            0, /* depth */
            serverClient,
            wVisual(realRoot), /* visual */
            &rc);

        if (!pWin)
            FatalError("hookInitRootWindow: cant create per-namespace root window for %s\n", walk->name);

        Mask mask = pWin->eventMask;
        pWin->eventMask = 0;    /* subterfuge in case AddResource fails */
        if (!AddResource(pWin->drawable.id, X11_RESTYPE_WINDOW, (void *) pWin))
            FatalError("hookInitRootWindow: cant add per-namespace root window as resource\n");
        pWin->eventMask = mask;

        walk->rootWindow = pWin;

        // set window name
        char buf[PATH_MAX] = { 0 };
        snprintf(buf, sizeof(buf)-1, "XNS-ROOT:%s", walk->name);
        setWinStrProp(pWin, XA_WM_NAME, buf);

        XNS_LOG("<%s> virtual root 0x%0" PRIx32 "\n", walk->name, walk->rootWindow->drawable.id);
    }
}

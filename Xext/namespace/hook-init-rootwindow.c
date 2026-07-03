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

/**
 * @brief Create a namespace's virtual root window, parented to the real root.
 *
 * Used both at boot (for namespaces known from the config file, one pass
 * over ns_list from hookInitRootWindow below) and at runtime (for a
 * namespace created later via the CreateNamespace protocol request, which
 * has no other opportunity to get a virtual root - PostInitRootWindowCallback
 * only ever fires once, before any client can reach the extension).
 *
 * @param realRoot the screen's actual root window to parent the new window to
 * @param name     the namespace's name, used only for the window's WM_NAME
 * @return the new window, or NULL on failure
 */
WindowPtr XnsCreateVirtualRoot(WindowPtr realRoot, const char *name)
{
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
        return NULL;

    Mask mask = pWin->eventMask;
    pWin->eventMask = 0;    /* subterfuge in case AddResource fails */
    if (!AddResource(pWin->drawable.id, X11_RESTYPE_WINDOW, (void *) pWin))
        return NULL;
    pWin->eventMask = mask;

    // set window name
    char buf[PATH_MAX] = { 0 };
    snprintf(buf, sizeof(buf)-1, "XNS-ROOT:%s", name);
    setWinStrProp(pWin, XA_WM_NAME, buf);

    return pWin;
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
            XNS_LOG("<%s> actual root 0x%0llx\n", walk->name, (unsigned long long)walk->rootWindow->drawable.id);
            continue;
        }

        walk->rootWindow = XnsCreateVirtualRoot(realRoot, walk->name);
        if (!walk->rootWindow)
            FatalError("hookInitRootWindow: cant create per-namespace root window for %s\n", walk->name);

        XNS_LOG("<%s> virtual root 0x%0llx\n", walk->name, (unsigned long long)walk->rootWindow->drawable.id);
    }
}

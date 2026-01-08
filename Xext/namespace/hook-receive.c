#define HOOK_NAME "recieve"

#include <dix-config.h>

#include <X11/Xmd.h>

#include <X11/extensions/XIproto.h>
#include <X11/extensions/XI2proto.h>
#include "dix/extension_priv.h"
#include "dix/registry_priv.h"
#include "dix/resource_priv.h"
#include "Xext/xacestr.h"

#include "present/present_priv.h"

#include "namespace.h"
#include "hooks.h"

static inline Bool isRootWin(WindowPtr pWin) {
    return (pWin->parent == NullWindow && dixClientForWindow(pWin) == serverClient);
}

void
hookReceive(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(XaceReceiveAccessRec);
    struct XnamespaceClientPriv *obj = XnsClientPriv(dixClientForWindow(param->pWin));

    // send and receive within same namespace permitted without restrictions
    if (subj->ns->superPower || XnsClientSameNS(subj, obj))
        goto pass;

    for (int i=0; i<param->count; i++) {
        const int type = param->events[i].u.u.type;

        // catch messages for root namespace
        if (obj->ns->isRoot) {
            const char* evname = LookupEventName(type);
            if (strcmp(evname,LookupEventName(ClientMessage))==0)
                goto pass;
            if (strcmp(evname,LookupEventName(UnmapNotify))==0)
                goto pass;
            // tricky types that don't get caught by the switch
            switch (type) {
                case ColormapNotify:
                case ConfigureNotify:
                case CreateNotify:
                case DestroyNotify:
                case MapNotify:
                case PropertyNotify:
                case ReparentNotify:
                case EnterNotify:
                case FocusIn:
                case FocusOut:
                case LeaveNotify:
                    goto pass;

                case GenericEvent: {
                    xGenericEvent *gev = (xGenericEvent*)&param->events[i].u;
                    if (gev->extension == EXTENSION_MAJOR_XINPUT) {
                        switch (gev->evtype) {
                            case X_InternAtom:
                                goto pass;
                            // exposes the entire screen
                            case X_PresentPixmap:
                                if (subj->ns->perms.allowScreen)
                                    goto pass;
                            // simply allow? seems pointless to deny
                            case X_ChangeGC:
                                goto pass;
                        }
                    }
                }
                // mostly for global keypresses
                case X_XIQueryDevice:
                    if (subj->ns->perms.allowGlobalKeyboard)
                        goto pass;
            }
        }

        switch (type) {
            case GenericEvent: {
                xGenericEvent *gev = (xGenericEvent*)&param->events[i].u;
                if (gev->extension == EXTENSION_MAJOR_XINPUT) {
                    switch (gev->evtype) {
                        case XI_RawMotion:
                            if ((!subj->ns->perms.allowMouseMotion) || !isRootWin(param->pWin))
                                goto reject;
                            continue;
                        case XI_RawKeyPress:
                        case XI_RawKeyRelease:
                            if ((!subj->ns->perms.allowGlobalKeyboard) || !isRootWin(param->pWin))
                                goto reject;
                            continue;
                        default:
                            XNS_HOOK_LOG("XI unknown %d\n", gev->evtype);
                            goto reject;
                    }
                }
                XNS_HOOK_LOG("BLOCKED #%d generic event extension=%d\n", i, gev->extension);
                goto reject;
            }
            break;

            case XI_ButtonPress:
            case XI_ButtonRelease:
                if ((!subj->ns->perms.allowXInput) || !isRootWin(param->pWin))
                    goto reject;
            continue;

            default:
                XNS_HOOK_LOG("BLOCKED event type #%d 0%0x 0%0x %s %s%s\n", i, type, param->events[i].u.u.detail,
                    LookupEventName(type), (type & 128) ? "fake" : "",
                    isRootWin(param->pWin) ? " (root window)" : "");
                goto reject;
            break;
        }
    }

pass:
    return;

reject:
    param->status = BadAccess;
    XNS_HOOK_LOG("BLOCKED client %d [NS %s] receiving event sent to window 0x%lx of client %d [NS %s]\n",
        client->index,
        subj->ns->name,
        (unsigned long)param->pWin->drawable.id,
        dixClientForWindow(param->pWin)->index,
        obj->ns->name);
    return;
}

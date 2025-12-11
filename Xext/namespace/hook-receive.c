#define HOOK_NAME "recieve"

#include <dix-config.h>

#include <X11/Xmd.h>

#include "dix/extension_priv.h"
#include "dix/registry_priv.h"
#include "dix/resource_priv.h"
#include "Xext/xacestr.h"

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
        if (strcmp(obj->ns->name,"root")==0) {
            const char* evname = LookupEventName(type);
            if (strcmp(evname,LookupEventName(ClientMessage))==0)
                goto pass;
            if (strcmp(evname,LookupEventName(UnmapNotify))==0)
                goto pass;
            // tricky types that don't get caught by the switch
            switch (type)
                case ConfigureNotify:
                case CreateNotify:
                case ReparentNotify:
                case PropertyNotify:
                case MapNotify:
                case ColormapNotify:
                case ClientMessage:
                case UnmapNotify:
                case DestroyNotify:
                case FocusOut:
                case FocusIn:
                case EnterNotify:
                case LeaveNotify:
                    goto pass;
        }

        switch (type) {
            case GenericEvent: {
                xGenericEvent *gev = (xGenericEvent*)&param->events[i].u;
                if (gev->extension == EXTENSION_MAJOR_XINPUT) {
                    switch (gev->evtype) {
                        case XI_RawMotion:
                            if ((!subj->ns->allowMouseMotion) || !isRootWin(param->pWin))
                                goto reject;
                            continue;
                        case XI_RawKeyPress:
                        case XI_RawKeyRelease:
                            if ((!subj->ns->allowGlobalKeyboard) || !isRootWin(param->pWin))
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
            case XI_ButtonRelease:
            case XI_ButtonPress:
                if ((!subj->ns->allowXInput) || !isRootWin(param->pWin))
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

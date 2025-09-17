#define HOOK_NAME "ext-access"

#include <dix-config.h>

#include "dix/dix_priv.h"
#include "dix/extension_priv.h"
#include "dix/registry_priv.h"
#include "Xext/xacestr.h"

#include "namespace.h"
#include "hooks.h"

/* called on X_QueryExtension */
void hookExtAccess(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(ExtensionAccessCallbackParam);

    /* root NS has super powers */
    if (subj->ns->superPower)
        goto pass;

    switch (param->ext->index + EXTENSION_BASE) {
        /* unrestricted access */
        case EXTENSION_MAJOR_BIG_REQUESTS:
        case EXTENSION_MAJOR_DAMAGE:
        case EXTENSION_MAJOR_DOUBLE_BUFFER:
        case EXTENSION_MAJOR_GENERIC_EVENT:
        case EXTENSION_MAJOR_PRESENT:
        case EXTENSION_MAJOR_SYNC:
        case EXTENSION_MAJOR_XC_MISC:
        case EXTENSION_MAJOR_XFIXES:
        case EXTENSION_MAJOR_XKEYBOARD:
        case EXTENSION_MAJOR_XRESOURCE:
            goto pass;

        /* really blacklisted */
        case EXTENSION_MAJOR_MIT_SCREEN_SAVER:
        case EXTENSION_MAJOR_RECORD:
        case EXTENSION_MAJOR_SECURITY:
        case EXTENSION_MAJOR_XTEST:
        case EXTENSION_MAJOR_XVIDEO:
            goto reject;

        /* only allowed if namespace has flag set */
        case EXTENSION_MAJOR_SHAPE:
            if (subj->ns->allowShape)
                goto pass;
            goto reject;

        /* only allowed if namespace has flag set */
        case EXTENSION_MAJOR_XINPUT:
            if (subj->ns->allowXInput)
                goto pass;
            goto reject;
    }

    XNS_HOOK_LOG("unhandled extension query: %s (%d)\n", param->ext->name, param->ext->index);
    return;

reject:
    param->status = BadAccess;
    return;

pass:
    param->status = Success;
    return;
}


#ifndef __HOOKS_H
#define __HOOKS_h

#include <dix-config.h>

#include "windowstr.h"
#include "propertyst.h"
#include "xacestr.h"
#include "registry.h"
#include "extinit.h"
#include "extnsionst.h"
#include "protocol-versions.h"

void handleReceive(CallbackListPtr *pcbl, void *unused, void *calldata);
void hookClientState(CallbackListPtr *pcbl, void *unused, void *calldata);
void hookSend(CallbackListPtr *pcbl, void *unused, void *calldata);

#endif

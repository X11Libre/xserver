/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include <dix-config.h>

#include <X11/X.h>
#include <X11/Xproto.h>

#include "dix/dix_priv.h"
#include "dix/extension_priv.h"
#include "dix/registry_priv.h"
#include "dix/request_priv.h"
#include "os/mathx_priv.h"

#include "dixstruct.h"
#include "extnsionst.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "dispatch.h"
#include "privates.h"
#include "xace.h"

#define LAST_ERROR 255

CallbackListPtr ExtensionAccessCallback = NULL;
CallbackListPtr ExtensionDispatchCallback = NULL;

static ExtensionEntry **extensions = (ExtensionEntry **) NULL;

int lastEvent = EXTENSION_EVENT_BASE;
static int lastError = FirstExtensionError;
static unsigned int NumExtensions = RESERVED_EXTENSIONS;

static struct { const char *name; int id; } reservedExt[] = {
    { "BIG-REQUESTS",               EXTENSION_MAJOR_BIG_REQUESTS },
    { "Apple-WM",                   EXTENSION_MAJOR_APPLE_WM },
    { "Apple-DRI",                  EXTENSION_MAJOR_APPLE_DRI },
    { "Composite",                  EXTENSION_MAJOR_COMPOSITE },
    { "DAMAGE",                     EXTENSION_MAJOR_DAMAGE },
    { "DOUBLE-BUFFER",              EXTENSION_MAJOR_DOUBLE_BUFFER },
    { "DPMS",                       EXTENSION_MAJOR_DPMS },
    { "DRI2",                       EXTENSION_MAJOR_DRI2 },
    { "DRI3",                       EXTENSION_MAJOR_DRI3 },
    { "Generic Event Extension",    EXTENSION_MAJOR_GENERIC_EVENT },
    { "GLX",                        EXTENSION_MAJOR_GLX },
    { "MIT-SCREEN-SAVER",           EXTENSION_MAJOR_MIT_SCREEN_SAVER },
    { "NAMESPACE",                  EXTENSION_MAJOR_NAMESPACE },
    { "Present",                    EXTENSION_MAJOR_PRESENT },
    { "RANDR",                      EXTENSION_MAJOR_RANDR },
    { "RECORD",                     EXTENSION_MAJOR_RECORD },
    { "RENDER",                     EXTENSION_MAJOR_RENDER },
    { "SECURITY",                   EXTENSION_MAJOR_SECURITY },
    { "SELinux",                    EXTENSION_MAJOR_SELINUX },
    { "SHAPE",                      EXTENSION_MAJOR_SHAPE },
    { "MIT-SHM",                    EXTENSION_MAJOR_SHM },
    { "SYNC",                       EXTENSION_MAJOR_SYNC },
    { "Windows-DRI",                EXTENSION_MAJOR_WINDOWS_DRI },
    { "XFIXES",                     EXTENSION_MAJOR_XFIXES },
    { "XFree86-Bigfont",            EXTENSION_MAJOR_XF86_BIGFONT },
    { "XFree86-DGA",                EXTENSION_MAJOR_XF86_DGA },
    { "XFree86-DRI",                EXTENSION_MAJOR_XF86_DRI },
    { "XFree86-VidModeExtension",   EXTENSION_MAJOR_XF86_VIDMODE },
    { "XC-MISC",                    EXTENSION_MAJOR_XC_MISC },
    { "XInputExtension",            EXTENSION_MAJOR_XINPUT },
    { "XINERAMA",                   EXTENSION_MAJOR_XINERAMA },
    { "XKEYBOARD",                  EXTENSION_MAJOR_XKEYBOARD },
    { "X-Resource",                 EXTENSION_MAJOR_XRESOURCE },
    { "XTEST",                      EXTENSION_MAJOR_XTEST },
    { "XVideo",                     EXTENSION_MAJOR_XVIDEO },
    { "XVideo-MotionCompensation",  EXTENSION_MAJOR_XVMC },
};

static int checkReserved(const char* name)
{
    for (int i=0; i<ARRAY_SIZE(reservedExt); i++) {
        if (strcmp(name, reservedExt[i].name) == 0) {
            if (reservedExt[i].id < (RESERVED_EXTENSIONS + EXTENSION_BASE))
                return reservedExt[i].id;
            FatalError("BUG: RESERVED_EXTENSIONS too small for %d\n", reservedExt[i].id);
        }
    }
    return -1;
}

ExtensionEntry *
AddExtension(const char *name, int NumEvents, int NumErrors,
             int (*MainProc) (ClientPtr c1),
             int (*SwappedMainProc) (ClientPtr c2),
             void (*CloseDownProc) (ExtensionEntry * e),
             unsigned short (*MinorOpcodeProc) (ClientPtr c3))
{
    if (!extensions)
        extensions = calloc(NumExtensions, sizeof(ExtensionEntry*));
    if (!extensions)
        return NULL;

    if (!MainProc || !SwappedMainProc || !MinorOpcodeProc)
        return ((ExtensionEntry *) NULL);
    if ((lastEvent + NumEvents > MAXEVENTS) ||
        (unsigned) (lastError + NumErrors > LAST_ERROR)) {
        LogMessage(X_ERROR, "Not enabling extension %s: maximum number of "
                   "events or errors exceeded.\n", name);
        return ((ExtensionEntry *) NULL);
    }

    ExtensionEntry *ext = calloc(1, sizeof(ExtensionEntry));
    if (!ext)
        return NULL;
    if (!dixAllocatePrivates(&ext->devPrivates, PRIVATE_EXTENSION))
        goto badalloc;
    ext->name = strdup(name);
    if (!ext->name)
        goto badalloc;

    int i = checkReserved(ext->name);
    if (i == -1) {
        i = NumExtensions;
        ExtensionEntry **newexts = reallocarray(extensions, i + 1, sizeof(ExtensionEntry *));
        if (!newexts)
            goto badalloc;

        NumExtensions++;
        extensions = newexts;
    } else {
        i = i - EXTENSION_BASE;
    }

    extensions[i] = ext;
    ext->index = i;
    ext->base = i + EXTENSION_BASE;
    ext->CloseDown = CloseDownProc;
    ext->MinorOpcode = MinorOpcodeProc;
    ProcVector[i + EXTENSION_BASE] = MainProc;
    SwappedProcVector[i + EXTENSION_BASE] = SwappedMainProc;
    if (NumEvents) {
        ext->eventBase = lastEvent;
        ext->eventLast = lastEvent + NumEvents;
        lastEvent += NumEvents;
    }
    else {
        ext->eventBase = 0;
        ext->eventLast = 0;
    }
    if (NumErrors) {
        ext->errorBase = lastError;
        ext->errorLast = lastError + NumErrors;
        lastError += NumErrors;
    }
    else {
        ext->errorBase = 0;
        ext->errorLast = 0;
    }

#ifdef X_REGISTRY_REQUEST
    RegisterExtensionNames(ext);
#endif
    return ext;

badalloc:
    if (ext) {
        free((char*)ext->name);
        dixFreePrivates(ext->devPrivates, PRIVATE_EXTENSION);
        free(ext);
    }
    return NULL;
}

/*
 * CheckExtension returns the extensions[] entry for the requested
 * extension name.  Maybe this could just return a Bool instead?
 */
ExtensionEntry *
CheckExtension(const char *extname)
{
    if (!extensions)
        return NULL;

    for (int i = 0; i < NumExtensions; i++) {
        if (extensions[i] &&
            extensions[i]->name &&
            strcmp(extensions[i]->name, extname) == 0) {
            return extensions[i];
        }
    }
    return NULL;
}

/*
 * Added as part of Xace.
 */
ExtensionEntry *
GetExtensionEntry(int major)
{
    if ((major < EXTENSION_BASE) || !extensions)
        return NULL;
    major -= EXTENSION_BASE;
    if (major >= NumExtensions)
        return NULL;
    return extensions[major];
}

unsigned short
StandardMinorOpcode(ClientPtr client)
{
    return ((xReq *) client->requestBuffer)->data;
}

void
CloseDownExtensions(void)
{
    if (!extensions)
        return;

    for (int i = NumExtensions - 1; i >= 0; i--) {
        if (!extensions[i])
            continue;
        if (extensions[i]->CloseDown)
            extensions[i]->CloseDown(extensions[i]);
        NumExtensions = i;
        free((void *) extensions[i]->name);
        dixFreePrivates(extensions[i]->devPrivates, PRIVATE_EXTENSION);
        free(extensions[i]);
        extensions[i] = NULL;
    }
    free(extensions);
    extensions = (ExtensionEntry **) NULL;
    NumExtensions = RESERVED_EXTENSIONS;
    lastEvent = EXTENSION_EVENT_BASE;
    lastError = FirstExtensionError;
}

static Bool
ExtensionAvailable(ClientPtr client, ExtensionEntry *ext)
{
    if (!ext)
        return FALSE;

    ExtensionAccessCallbackParam rec = { client, ext, DixGetAttrAccess, Success };
    CallCallbacks(&ExtensionAccessCallback, &rec);
    if (rec.status != Success)
        return FALSE;

    if (!ext->base)
        return FALSE;
    return TRUE;
}

int
ProcQueryExtension(ClientPtr client)
{
    REQUEST(xQueryExtensionReq);
    REQUEST_FIXED_SIZE(xQueryExtensionReq, stuff->nbytes);

    xQueryExtensionReply rep = { 0 };

    if (!NumExtensions || !extensions)
        rep.present = xFalse;
    else {
        char extname[PATH_MAX] = { 0 };
        strncpy(extname, (char *) &stuff[1], MIN(stuff->nbytes, sizeof(extname)-1));
        ExtensionEntry *extEntry = CheckExtension(extname);

        if (!extEntry || !ExtensionAvailable(client, extEntry))
            rep.present = xFalse;
        else {
            rep.present = xTrue;
            rep.major_opcode = extEntry->base;
            rep.first_event = extEntry->eventBase;
            rep.first_error = extEntry->errorBase;
        }
    }

    return X_SEND_REPLY_SIMPLE(client, rep);
}

int
ProcListExtensions(ClientPtr client)
{
    REQUEST_SIZE_MATCH(xReq);

    xListExtensionsReply rep = { 0 };

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    if (NumExtensions && extensions) {
        for (int i = 0; i < NumExtensions; i++) {
            if (!ExtensionAvailable(client, extensions[i]))
                continue;

            int len = strlen(extensions[i]->name);

            rep.nExtensions++;

            /* write a pascal string */
            x_rpcbuf_write_CARD8(&rpcbuf, len);
            x_rpcbuf_write_CARD8s(&rpcbuf, (CARD8*)extensions[i]->name, len);
        }
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, rep, rpcbuf);
}

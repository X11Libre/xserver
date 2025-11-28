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

/* The panoramix components contained the following notice */
/*****************************************************************

Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.

******************************************************************/

#include <dix-config.h>
#include <version-config.h>

#include <pixman.h>
#include <X11/X.h>
#include <X11/Xos.h>            /* for unistd.h  */
#include <X11/Xproto.h>
#include <X11/fonts/font.h>
#include <X11/fonts/fontstruct.h>
#include <X11/fonts/libxfont2.h>

#include "config/hotplug_priv.h"
#include "dix/atom_priv.h"
#include "dix/callback_priv.h"
#include "dix/cursor_priv.h"
#include "dix/dix_priv.h"
#include "dix/extension_priv.h"
#include "dix/input_priv.h"
#include "dix/gc_priv.h"
#include "dix/registry_priv.h"
#include "dix/screensaver_priv.h"
#include "dix/selection_priv.h"
#include "dix/server_priv.h"
#include "include/extinit.h"
#include "os/audit_priv.h"
#include "os/auth.h"
#include "os/client_priv.h"
#include "os/cmdline.h"
#include "os/ddx_priv.h"
#include "os/osdep.h"
#include "os/screensaver.h"
#include "os/serverlock.h"
#include "Xext/panoramiXsrv.h"

#include "scrnintstr.h"
#include "misc.h"
#include "os.h"
#include "windowstr.h"
#include "resource.h"
#include "dixstruct.h"
#include "gcstruct.h"
#include "extension.h"
#include "cursorstr.h"
#include "servermd.h"
#include "dixfont.h"
#include "extnsionst.h"
#include "privates.h"
#include "exevents.h"

#ifdef DPMSExtension
#include <X11/extensions/dpmsconst.h>
#include "dpmsproc.h"
#endif

extern void Dispatch(void);

CallbackListPtr RootWindowFinalizeCallback = NULL;
CallbackListPtr PostInitRootWindowCallback = NULL;

int
dix_main(int argc, char *argv[], char *envp[])
{
    HWEventQueueType alwaysCheckForInput[2];

    display = "0";

    InitRegions();

    CheckUserParameters(argc, argv, envp);

    CheckUserAuthorization();

    ProcessCommandLine(argc, argv);

    alwaysCheckForInput[0] = 0;
    alwaysCheckForInput[1] = 1;

        ScreenSaverTime = defaultScreenSaverTime;
        ScreenSaverInterval = defaultScreenSaverInterval;
        ScreenSaverBlanking = defaultScreenSaverBlanking;
        ScreenSaverAllowExposures = defaultScreenSaverAllowExposures;

        InitBlockAndWakeupHandlers();
        /* Perform any operating system dependent initializations you'd like */
        OsInit();

            CreateWellKnownSockets();
            for (int i = 1; i < LimitClients; i++)
                clients[i] = NULL;
            serverClient = calloc(1, sizeof(ClientRec));
            if (!serverClient)
                FatalError("couldn't create server client");
            InitClient(serverClient, 0, (void *) NULL);

        clients[0] = serverClient;
        currentMaxClients = 1;

        /* clear any existing selections */
        InitSelections();

        /* Initialize privates before first allocation */
        dixResetPrivates();

        /* Initialize server client devPrivates, to be reallocated as
         * more client privates are registered
         */
        if (!dixAllocatePrivates(&serverClient->devPrivates, PRIVATE_CLIENT))
            FatalError("failed to create server client privates");

        if (!InitClientResources(serverClient)) /* for root resources */
            FatalError("couldn't init server resources");

        SetInputCheck(&alwaysCheckForInput[0], &alwaysCheckForInput[1]);
        screenInfo.numScreens = 0;

        InitAtoms();
        InitEvents();
        xfont2_init_glyph_caching();
        dixResetRegistry();
        InitFonts();
        InitCallbackManager();
        InitOutput(argc, argv);

        if (screenInfo.numScreens < 1)
            FatalError("no screens found");
        LogMessageVerb(X_INFO, 1, "Output(s) initialized\n");

        InitExtensions(argc, argv);
        LogMessageVerb(X_INFO, 1, "Extensions initialized\n");

        DIX_FOR_EACH_GPU_SCREEN({
            if (!PixmapScreenInit(walkScreen))
                FatalError("failed to create screen pixmap properties");
            if (!dixScreenRaiseCreateResources(walkScreen))
                FatalError("failed to create screen resources");
        });

        DIX_FOR_EACH_SCREEN({
            if (!PixmapScreenInit(walkScreen))
                FatalError("failed to create screen pixmap properties");
            if (!dixScreenRaiseCreateResources(walkScreen))
                FatalError("failed to create screen resources");
            if (!CreateGCperDepth(walkScreen))
                FatalError("failed to create scratch GCs");
            if (!CreateDefaultStipple(walkScreen))
                FatalError("failed to create default stipple");
            if (!CreateRootWindow(walkScreen))
                FatalError("failed to create root window");
            CallCallbacks(&RootWindowFinalizeCallback, walkScreen);
        });

        if (SetDefaultFontPath(defaultFontPath) != Success) {
            ErrorF("[dix] failed to set default font path '%s'",
                   defaultFontPath);
        }
        if (!SetDefaultFont("fixed")) {
            FatalError("could not open default font");
        }

        if (!(rootCursor = CreateRootCursor())) {
            FatalError("could not open default cursor font");
        }

        rootCursor = RefCursor(rootCursor);

#ifdef XINERAMA
        /*
         * Consolidate window and colourmap information for each screen
         */
        if (!noPanoramiXExtension)
            PanoramiXConsolidate();
#endif /* XINERAMA */

        DIX_FOR_EACH_SCREEN({
            InitRootWindow(walkScreen->root);
            CallCallbacks(&PostInitRootWindowCallback, walkScreen);
        });

        LogMessageVerb(X_INFO, 1, "Screen(s) initialized\n");

        InitCoreDevices();
        InitInput(argc, argv);
        InitAndStartDevices();
        LogMessageVerb(X_INFO, 1, "Input(s) initialized\n");

        ReserveClientIds(serverClient);

        dixSaveScreens(serverClient, SCREEN_SAVER_FORCER, ScreenSaverReset);

        dixCloseRegistry();

#ifdef XINERAMA
        if (!noPanoramiXExtension) {
            if (!PanoramiXCreateConnectionBlock()) {
                FatalError("could not create connection block info");
            }
        }
        else
#endif /* XINERAMA */
        {
            if (!CreateConnectionBlock()) {
                FatalError("could not create connection block info");
            }
        }

        NotifyParentProcess();

        InputThreadInit();

        Dispatch();

        UnrefCursor(rootCursor);

        UndisplayDevices();
        DisableAllDevices();

        /* Now free up whatever must be freed */
        if (screenIsSaved == SCREEN_SAVER_ON)
            dixSaveScreens(serverClient, SCREEN_SAVER_OFF, ScreenSaverReset);
        FreeScreenSaverTimer();
        CloseDownExtensions();

#ifdef XINERAMA
        {
            Bool remember_it = noPanoramiXExtension;

            noPanoramiXExtension = TRUE;
            FreeAllResources();
            noPanoramiXExtension = remember_it;
        }
#else
        FreeAllResources();
#endif /* XINERAMA */

        CloseInput();

        InputThreadFini();

        DIX_FOR_EACH_SCREEN({ walkScreen->root = NullWindow; });

        CloseDownDevices();

        CloseDownEvents();

        if (screenInfo.numGPUScreens > 0) {
            for (int walkScreenIdx = screenInfo.numGPUScreens - 1; walkScreenIdx >= 0; walkScreenIdx--) {
                ScreenPtr walkScreen = screenInfo.gpuscreens[walkScreenIdx];
                dixFreeScreen(walkScreen);
                screenInfo.numGPUScreens = walkScreenIdx;
            }
        }
        memset(&screenInfo.gpuscreens, 0, sizeof(screenInfo.gpuscreens));

        if (screenInfo.numScreens > 0) {
            for (int walkScreenIdx = screenInfo.numScreens - 1; walkScreenIdx >= 0; walkScreenIdx--) {
                ScreenPtr walkScreen = screenInfo.screens[walkScreenIdx];
                dixFreeScreen(walkScreen);
                screenInfo.numScreens = walkScreenIdx;
            }
        }
        memset(&screenInfo.screens, 0, sizeof(screenInfo.screens));

        ReleaseClientIds(serverClient);
        dixFreePrivates(serverClient->devPrivates, PRIVATE_CLIENT);
        serverClient->devPrivates = NULL;

	dixFreeRegistry();

        FreeFonts();

        FreeAllAtoms();

        FreeAuditTimer();

        DeleteCallbackManager();

        ClearWorkQueue();

        CloseWellKnownConnections();
        UnlockServer();

        ddxGiveUp(EXIT_NO_ERROR);

        free(ConnectionInfo);
        ConnectionInfo = NULL;

    return 0;
}

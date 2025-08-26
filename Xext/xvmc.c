
#include <dix-config.h>

#include "dix/request_priv.h"
#include "dix/screen_hooks_priv.h"
#include "miext/extinit_priv.h"

#include "xvmcext.h"

#define DR_CLIENT_DRIVER_NAME_SIZE 48
#define DR_BUSID_SIZE 48

static DevPrivateKeyRec XvMCScreenKeyRec;

#define XvMCScreenKey (&XvMCScreenKeyRec)

typedef struct {
    int num_adaptors;
    XvMCAdaptorPtr adaptors;
    char clientDriverName[DR_CLIENT_DRIVER_NAME_SIZE];
    char busID[DR_BUSID_SIZE];
    int major;
    int minor;
    int patchLevel;
} XvMCScreenRec, *XvMCScreenPtr;

#define XVMC_GET_PRIVATE(pScreen) \
    (XvMCScreenPtr)(dixLookupPrivate(&(pScreen)->devPrivates, XvMCScreenKey))

void
XvMCExtensionInit(void)
{
    /* keep the resource types intact within this ABI cylce, since some
       drivers might still need them */
    if (!dixPrivateKeyRegistered(XvMCScreenKey))
        return;
}

static void XvMCScreenClose(CallbackListPtr *pcbl, ScreenPtr pScreen, void *unused)
{
    XvMCScreenPtr pScreenPriv = XVMC_GET_PRIVATE(pScreen);
    free(pScreenPriv);
    dixSetPrivate(&pScreen->devPrivates, XvMCScreenKey, NULL);
    dixScreenUnhookClose(pScreen, XvMCScreenClose);
}

int
XvMCScreenInit(ScreenPtr pScreen, int num, XvMCAdaptorPtr pAdapt)
{
    XvMCScreenPtr pScreenPriv;

    if (!dixRegisterPrivateKey(&XvMCScreenKeyRec, PRIVATE_SCREEN, 0))
        return BadAlloc;

    if (!(pScreenPriv = calloc(1, sizeof(XvMCScreenRec))))
        return BadAlloc;

    dixSetPrivate(&pScreen->devPrivates, XvMCScreenKey, pScreenPriv);

    dixScreenHookClose(pScreen, XvMCScreenClose);

    pScreenPriv->num_adaptors = num;
    pScreenPriv->adaptors = pAdapt;
    pScreenPriv->clientDriverName[0] = 0;
    pScreenPriv->busID[0] = 0;
    pScreenPriv->major = 0;
    pScreenPriv->minor = 0;
    pScreenPriv->patchLevel = 0;

    return Success;
}

int
xf86XvMCRegisterDRInfo(ScreenPtr pScreen, const char *name,
                       const char *busID, int major, int minor, int patchLevel)
{
    XvMCScreenPtr pScreenPriv = XVMC_GET_PRIVATE(pScreen);

    strlcpy(pScreenPriv->clientDriverName, name, DR_CLIENT_DRIVER_NAME_SIZE);
    strlcpy(pScreenPriv->busID, busID, DR_BUSID_SIZE);
    pScreenPriv->major = major;
    pScreenPriv->minor = minor;
    pScreenPriv->patchLevel = patchLevel;
    return Success;
}

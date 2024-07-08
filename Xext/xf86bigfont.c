/*
 * BIGFONT extension for sharing font metrics between clients (if possible)
 * and for transmitting font metrics to clients in a compressed form.
 *
 * Copyright (c) 1999-2000  Bruno Haible
 * Copyright (c) 1999-2000  The XFree86 Project, Inc.
 */

/* THIS IS NOT AN X CONSORTIUM STANDARD */

/*
 * Big fonts suffer from the following: All clients that have opened a
 * font can access the complete glyph metrics array (the XFontStruct member
 * `per_char') directly, without going through a macro. Moreover these
 * glyph metrics are ink metrics, i.e. are not redundant even for a
 * fixed-width font. For a Unicode font, the size of this array is 768 KB.
 *
 * Problems: 1. It eats a lot of memory in each client. 2. All this glyph
 * metrics data is piped through the socket when the font is opened.
 *
 * This extension addresses these two problems for local clients, by using
 * shared memory. It also addresses the second problem for non-local clients,
 * by compressing the data before transmit by a factor of nearly 6.
 *
 * If you use this extension, your OS ought to nicely support shared memory.
 * This means: Shared memory should be swappable to the swap, and the limits
 * should be high enough (SHMMNI at least 64, SHMMAX at least 768 KB,
 * SHMALL at least 48 MB). It is a plus if your OS allows shmat() calls
 * on segments that have already been marked "removed", because it permits
 * these segments to be cleaned up by the OS if the X server is killed with
 * signal SIGKILL.
 *
 * This extension is transparently exploited by Xlib (functions XQueryFont,
 * XLoadQueryFont).
 */

#include <dix-config.h>

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#ifdef CONFIG_MITSHM
#include <sys/sysmacros.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#endif /* CONFIG_MITSHM */

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/xf86bigfproto.h>
#include <X11/fonts/fontstruct.h>
#include <X11/fonts/libxfont2.h>

#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "miext/extinit_priv.h"

#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "gcstruct.h"
#include "dixfontstr.h"
#include "extnsionst.h"
#include "protocol-versions.h"

#include "xf86bigfontsrv.h"

Bool noXFree86BigfontExtension = FALSE;

static void XF86BigfontResetProc(ExtensionEntry *extEntry );

#ifdef CONFIG_MITSHM

/* A random signature, transmitted to the clients so they can verify that the
   shared memory segment they are attaching to was really established by the
   X server they are talking to. */
static CARD32 signature;

/* Index for additional information stored in a FontRec's devPrivates array. */
static int FontShmdescIndex;

static unsigned int pagesize;

static Bool badSysCall = FALSE;

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)

static void
SigSysHandler(int signo)
{
    badSysCall = TRUE;
}

static Bool
CheckForShmSyscall(void)
{
    void (*oldHandler) (int);
    int shmid = -1;

    /* If no SHM support in the kernel, the bad syscall will generate SIGSYS */
    oldHandler = OsSignal(SIGSYS, SigSysHandler);

    badSysCall = FALSE;
    shmid = shmget(IPC_PRIVATE, 4096, IPC_CREAT);
    if (shmid != -1) {
        /* Successful allocation - clean up */
        shmctl(shmid, IPC_RMID, NULL);
    }
    else {
        /* Allocation failed */
        badSysCall = TRUE;
    }
    OsSignal(SIGSYS, oldHandler);
    return !badSysCall;
}

#define MUST_CHECK_FOR_SHM_SYSCALL

#endif

/* ========== Management of shared memory segments ========== */

#ifdef __linux__
/* On Linux, shared memory marked as "removed" can still be attached.
   Nice feature, because the kernel will automatically free the associated
   storage when the server and all clients are gone. */
#define EARLY_REMOVE
#endif

typedef struct _ShmDesc {
    struct _ShmDesc *next;
    struct _ShmDesc **prev;
    int shmid;
    char *attach_addr;
} ShmDescRec, *ShmDescPtr;

static ShmDescPtr ShmList = (ShmDescPtr) NULL;

static ShmDescPtr
shmalloc(unsigned int size)
{
    int shmid;
    char *addr;

#ifdef MUST_CHECK_FOR_SHM_SYSCALL
    if (pagesize == 0)
        return (ShmDescPtr) NULL;
#endif

    /* On some older Linux systems, the number of shared memory segments
       system-wide is 127. In Linux 2.4, it is 4095.
       Therefore there is a tradeoff to be made between allocating a
       shared memory segment on one hand, and allocating memory and piping
       the glyph metrics on the other hand. If the glyph metrics size is
       small, we prefer the traditional way. */
    if (size < 3500)
        return (ShmDescPtr) NULL;

    ShmDescPtr pDesc = calloc(1, sizeof(ShmDescRec));
    if (!pDesc)
        return (ShmDescPtr) NULL;

    size = (size + pagesize - 1) & -pagesize;
    shmid = shmget(IPC_PRIVATE, size, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
    if (shmid == -1) {
        ErrorF(XF86BIGFONTNAME " extension: shmget() failed, size = %u, %s\n",
               size, strerror(errno));
        free(pDesc);
        return (ShmDescPtr) NULL;
    }

    if ((addr = shmat(shmid, 0, 0)) == (char *) -1) {
        ErrorF(XF86BIGFONTNAME " extension: shmat() failed, size = %u, %s\n",
               size, strerror(errno));
        shmctl(shmid, IPC_RMID, (void *) 0);
        free(pDesc);
        return (ShmDescPtr) NULL;
    }

#ifdef EARLY_REMOVE
    shmctl(shmid, IPC_RMID, (void *) 0);
#endif

    pDesc->shmid = shmid;
    pDesc->attach_addr = addr;
    if (ShmList)
        ShmList->prev = &pDesc->next;
    pDesc->next = ShmList;
    pDesc->prev = &ShmList;
    ShmList = pDesc;

    return pDesc;
}

static void
shmdealloc(ShmDescPtr pDesc)
{
#ifndef EARLY_REMOVE
    shmctl(pDesc->shmid, IPC_RMID, (void *) 0);
#endif
    shmdt(pDesc->attach_addr);

    if (pDesc->next)
        pDesc->next->prev = pDesc->prev;
    *pDesc->prev = pDesc->next;
    free(pDesc);
}

/* Called when a font is closed. */
void
XF86BigfontFreeFontShm(FontPtr pFont)
{
    ShmDescPtr pDesc;

    /* If during shutdown of the server, XF86BigfontCleanup() has already
     * called shmdealloc() for all segments, we don't need to do it here.
     */
    if (!ShmList)
        return;

    pDesc = (ShmDescPtr) FontGetPrivate(pFont, FontShmdescIndex);
    if (pDesc)
        shmdealloc(pDesc);
}

/* Called upon fatal signal. */
void
XF86BigfontCleanup(void)
{
    while (ShmList)
        shmdealloc(ShmList);
}

#else /* CONFIG_MITSHM */

void XF86BigfontFreeFontShm(FontPtr pFont) { }
void XF86BigfontCleanup(void) { }

#endif /* CONFIG_MITSHM */

/* Called when a server generation dies. */
static void
XF86BigfontResetProc(ExtensionEntry * extEntry)
{
    /* This function is normally called from CloseDownExtensions(), called
     * from main(). It will be followed by a call to FreeAllResources(),
     * which will call XF86BigfontFreeFontShm() for each font. Thus it
     * appears that we do not need to do anything in this function. --
     * But I prefer to write robust code, and not keep shared memory lying
     * around when it's not needed any more. (Someone might close down the
     * extension without calling FreeAllResources()...)
     */
    XF86BigfontCleanup();
}

/* ========== Handling of extension specific requests ========== */

static int
ProcXF86BigfontQueryVersion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXF86BigfontQueryVersionReq);

    xXF86BigfontQueryVersionReply reply = {
        .majorVersion = SERVER_XF86BIGFONT_MAJOR_VERSION,
        .minorVersion = SERVER_XF86BIGFONT_MINOR_VERSION,
        .uid = geteuid(),
        .gid = getegid(),
#ifdef CONFIG_MITSHM
        .signature = signature,
        .capabilities = (client->local && !client->swapped)
                         ? XF86Bigfont_CAP_LocalShm : 0
#endif /* CONFIG_MITSHM */
    };
    if (client->swapped) {
        swaps(&reply.majorVersion);
        swaps(&reply.minorVersion);
        swapl(&reply.uid);
        swapl(&reply.gid);
        swapl(&reply.signature);
    }
    return X_SEND_REPLY_SIMPLE(client, reply);
}

static void
swapCharInfo(xCharInfo * pCI)
{
    swaps(&pCI->leftSideBearing);
    swaps(&pCI->rightSideBearing);
    swaps(&pCI->characterWidth);
    swaps(&pCI->ascent);
    swaps(&pCI->descent);
    swaps(&pCI->attributes);
}

static inline void writeCharInfo(x_rpcbuf_t *rpcbuf, xCharInfo CI) {
    x_rpcbuf_write_INT16(rpcbuf, CI.leftSideBearing);
    x_rpcbuf_write_INT16(rpcbuf, CI.rightSideBearing);
    x_rpcbuf_write_INT16(rpcbuf, CI.characterWidth);
    x_rpcbuf_write_INT16(rpcbuf, CI.ascent);
    x_rpcbuf_write_INT16(rpcbuf, CI.descent);
    x_rpcbuf_write_CARD16(rpcbuf, CI.attributes);
}

/* static CARD32 hashCI (xCharInfo *p); */
#define hashCI(p) \
	(CARD32)(((p->leftSideBearing << 27) + (p->leftSideBearing >> 5) + \
	          (p->rightSideBearing << 23) + (p->rightSideBearing >> 9) + \
	          (p->characterWidth << 16) + \
	          (p->ascent << 11) + (p->descent << 6)) ^ p->attributes)

static int
ProcXF86BigfontQueryFont(ClientPtr client)
{
    X_REQUEST_HEAD_NO_CHECK(xXF86BigfontQueryFontReq);
    X_REQUEST_FIELD_CARD32(id);

    FontPtr pFont;
    CARD32 stuff_flags;
    xCharInfo *pmax;
    xCharInfo *pmin;
    int nCharInfos;
    int shmid;

#ifdef CONFIG_MITSHM
    ShmDescPtr pDesc = NULL;
#else
#define pDesc 0
#endif /* CONFIG_MITSHM */
    xCharInfo *pCI;
    CARD16 *pIndex2UniqIndex;
    CARD16 *pUniqIndex2Index;
    CARD32 nUniqCharInfos;

    /* protocol version is decided based on request packet size */
    switch (client->req_len) {
    case 2:                    /* client with version 1.0 libX11 */
        stuff_flags = (client->local &&
                       !client->swapped ? XF86Bigfont_FLAGS_Shm : 0);
        break;
    case 3:                    /* client with version 1.1 libX11 */
        stuff_flags = stuff->flags;
        break;
    default:
        return BadLength;
    }

    if (dixLookupFontable(&pFont, stuff->id, client, DixGetAttrAccess) !=
        Success)
        return BadFont;         /* procotol spec says only error is BadFont */

    pmax = FONTINKMAX(pFont);
    pmin = FONTINKMIN(pFont);
    nCharInfos =
        (pmax->rightSideBearing == pmin->rightSideBearing
         && pmax->leftSideBearing == pmin->leftSideBearing
         && pmax->descent == pmin->descent
         && pmax->ascent == pmin->ascent
         && pmax->characterWidth == pmin->characterWidth)
        ? 0 : N2dChars(pFont);
    shmid = -1;
    pCI = NULL;
    pIndex2UniqIndex = NULL;
    pUniqIndex2Index = NULL;
    nUniqCharInfos = 0;

    if (nCharInfos > 0) {
#ifdef CONFIG_MITSHM
        if (!badSysCall)
            pDesc = (ShmDescPtr) FontGetPrivate(pFont, FontShmdescIndex);
        if (pDesc) {
            pCI = (xCharInfo *) pDesc->attach_addr;
            if (stuff_flags & XF86Bigfont_FLAGS_Shm)
                shmid = pDesc->shmid;
        }
        else {
            if (stuff_flags & XF86Bigfont_FLAGS_Shm && !badSysCall)
                pDesc = shmalloc(nCharInfos * sizeof(xCharInfo)
                                 + sizeof(CARD32));
            if (pDesc) {
                pCI = (xCharInfo *) pDesc->attach_addr;
                shmid = pDesc->shmid;
            }
            else {
#endif /* CONFIG_MITSHM */
                pCI = calloc(nCharInfos, sizeof(xCharInfo));
                if (!pCI)
                    return BadAlloc;
#ifdef CONFIG_MITSHM
            }
#endif /* CONFIG_MITSHM */
            /* Fill nCharInfos starting at pCI. */
            {
                xCharInfo *prCI = pCI;
                int ninfos = 0;
                int ncols = pFont->info.lastCol - pFont->info.firstCol + 1;
                int row;

                for (row = pFont->info.firstRow;
                     row <= pFont->info.lastRow && ninfos < nCharInfos; row++) {
                    unsigned char chars[512];
                    xCharInfo *tmpCharInfos[256];
                    unsigned long count;
                    int col;
                    unsigned long i;

                    i = 0;
                    for (col = pFont->info.firstCol;
                         col <= pFont->info.lastCol; col++) {
                        chars[i++] = row;
                        chars[i++] = col;
                    }
                    (*pFont->get_metrics) (pFont, ncols, chars, TwoD16Bit,
                                           &count, tmpCharInfos);
                    for (i = 0; i < count && ninfos < nCharInfos; i++) {
                        *prCI++ = *tmpCharInfos[i];
                        ninfos++;
                    }
                }
            }
#ifdef CONFIG_MITSHM
            if (pDesc && !badSysCall) {
                *(CARD32 *) (pCI + nCharInfos) = signature;
                if (!xfont2_font_set_private(pFont, FontShmdescIndex, pDesc)) {
                    shmdealloc(pDesc);
                    return BadAlloc;
                }
            }
        }
#endif /* CONFIG_MITSHM */
        if (shmid == -1) {
            /* Cannot use shared memory, so remove-duplicates the xCharInfos
               using a temporary hash table. */
            /* Note that CARD16 is suitable as index type, because
               nCharInfos <= 0x10000. */
            CARD32 hashModulus;
            CARD16 *pHash2UniqIndex;
            CARD16 *pUniqIndex2NextUniqIndex;
            CARD32 NextIndex;
            CARD32 NextUniqIndex;
            CARD16 *tmp;
            CARD32 i, j;

            hashModulus = 67;
            if (hashModulus > nCharInfos + 1)
                hashModulus = nCharInfos + 1;

            tmp = calloc(4 * nCharInfos + 1, sizeof(CARD16));
            if (!tmp) {
                if (!pDesc)
                    free(pCI);
                return BadAlloc;
            }
            pIndex2UniqIndex = tmp;
            /* nCharInfos elements */
            pUniqIndex2Index = tmp + nCharInfos;
            /* max. nCharInfos elements */
            pUniqIndex2NextUniqIndex = tmp + 2 * nCharInfos;
            /* max. nCharInfos elements */
            pHash2UniqIndex = tmp + 3 * nCharInfos;
            /* hashModulus (<= nCharInfos+1) elements */

            /* Note that we can use 0xffff as end-of-list indicator, because
               even if nCharInfos = 0x10000, 0xffff can not occur as valid
               entry before the last element has been inserted. And once the
               last element has been inserted, we don't need the hash table
               any more. */
            for (j = 0; j < hashModulus; j++)
                pHash2UniqIndex[j] = (CARD16) (-1);

            NextUniqIndex = 0;
            for (NextIndex = 0; NextIndex < nCharInfos; NextIndex++) {
                xCharInfo *p = &pCI[NextIndex];
                CARD32 hashCode = hashCI(p) % hashModulus;

                for (i = pHash2UniqIndex[hashCode];
                     i != (CARD16) (-1); i = pUniqIndex2NextUniqIndex[i]) {
                    j = pUniqIndex2Index[i];
                    if (pCI[j].leftSideBearing == p->leftSideBearing
                        && pCI[j].rightSideBearing == p->rightSideBearing
                        && pCI[j].characterWidth == p->characterWidth
                        && pCI[j].ascent == p->ascent
                        && pCI[j].descent == p->descent
                        && pCI[j].attributes == p->attributes)
                        break;
                }
                if (i != (CARD16) (-1)) {
                    /* Found *p at Index j, UniqIndex i */
                    pIndex2UniqIndex[NextIndex] = i;
                }
                else {
                    /* Allocate a new entry in the Uniq table */
                    if (hashModulus <= 2 * NextUniqIndex
                        && hashModulus < nCharInfos + 1) {
                        /* Time to increate hash table size */
                        hashModulus = 2 * hashModulus + 1;
                        if (hashModulus > nCharInfos + 1)
                            hashModulus = nCharInfos + 1;
                        for (j = 0; j < hashModulus; j++)
                            pHash2UniqIndex[j] = (CARD16) (-1);
                        for (i = 0; i < NextUniqIndex; i++)
                            pUniqIndex2NextUniqIndex[i] = (CARD16) (-1);
                        for (i = 0; i < NextUniqIndex; i++) {
                            j = pUniqIndex2Index[i];
                            p = &pCI[j];
                            hashCode = hashCI(p) % hashModulus;
                            pUniqIndex2NextUniqIndex[i] =
                                pHash2UniqIndex[hashCode];
                            pHash2UniqIndex[hashCode] = i;
                        }
                        p = &pCI[NextIndex];
                        hashCode = hashCI(p) % hashModulus;
                    }
                    i = NextUniqIndex++;
                    pUniqIndex2NextUniqIndex[i] = pHash2UniqIndex[hashCode];
                    pHash2UniqIndex[hashCode] = i;
                    pUniqIndex2Index[i] = NextIndex;
                    pIndex2UniqIndex[NextIndex] = i;
                }
            }
            nUniqCharInfos = NextUniqIndex;
            /* fprintf(stderr, "font metrics: nCharInfos = %d, nUniqCharInfos = %d, hashModulus = %d\n", nCharInfos, nUniqCharInfos, hashModulus); */
        }
    }

    {
        int nfontprops = pFont->info.nprops;
        xXF86BigfontQueryFontReply reply = {
            .minBounds = pFont->info.ink_minbounds,
            .maxBounds = pFont->info.ink_maxbounds,
            .minCharOrByte2 = pFont->info.firstCol,
            .maxCharOrByte2 = pFont->info.lastCol,
            .defaultChar = pFont->info.defaultCh,
            .nFontProps = pFont->info.nprops,
            .drawDirection = pFont->info.drawDirection,
            .minByte1 = pFont->info.firstRow,
            .maxByte1 = pFont->info.lastRow,
            .allCharsExist = pFont->info.allExist,
            .fontAscent = pFont->info.fontAscent,
            .fontDescent = pFont->info.fontDescent,
            .nCharInfos = nCharInfos,
            .nUniqCharInfos = nUniqCharInfos,
            .shmid = shmid,
        };

        if (client->swapped) {
            swapCharInfo(&reply.minBounds);
            swapCharInfo(&reply.maxBounds);
            swaps(&reply.minCharOrByte2);
            swaps(&reply.maxCharOrByte2);
            swaps(&reply.defaultChar);
            swaps(&reply.nFontProps);
            swaps(&reply.fontAscent);
            swaps(&reply.fontDescent);
            swapl(&reply.nCharInfos);
            swapl(&reply.nUniqCharInfos);
            swapl(&reply.shmid);
            swapl(&reply.shmsegoffset);
        }

        int rc = Success;

        x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

        for (int i = 0; i < nfontprops; i++) {
            x_rpcbuf_write_CARD32(&rpcbuf, pFont->info.props[i].name);
            x_rpcbuf_write_CARD32(&rpcbuf, pFont->info.props[i].value);
        }

        if (nCharInfos > 0 && shmid == -1) {
            for (int i = 0; i < nUniqCharInfos; i++)
                writeCharInfo(&rpcbuf, pCI[pUniqIndex2Index[i]]);
            x_rpcbuf_write_CARD16s(&rpcbuf, pIndex2UniqIndex, nCharInfos);
        }

        rc = X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);

        if (nCharInfos > 0) {
            if (shmid == -1)
                free(pIndex2UniqIndex);
            if (!pDesc)
                free(pCI);
        }
        return rc;
    }
}

static int
ProcXF86BigfontDispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_XF86BigfontQueryVersion:
        return ProcXF86BigfontQueryVersion(client);
    case X_XF86BigfontQueryFont:
        return ProcXF86BigfontQueryFont(client);
    default:
        return BadRequest;
    }
}

void
XFree86BigfontExtensionInit(void)
{
    if (AddExtension(XF86BIGFONTNAME,
                     XF86BigfontNumberEvents,
                     XF86BigfontNumberErrors,
                     ProcXF86BigfontDispatch,
                     ProcXF86BigfontDispatch,
                     XF86BigfontResetProc, StandardMinorOpcode)) {
#ifdef CONFIG_MITSHM
#ifdef MUST_CHECK_FOR_SHM_SYSCALL
        /*
         * Note: Local-clients will not be optimized without shared memory
         * support. Remote-client optimization does not depend on shared
         * memory support.  Thus, the extension is still registered even
         * when shared memory support is not functional.
         */
        if (!CheckForShmSyscall()) {
            ErrorF(XF86BIGFONTNAME
                   " extension local-client optimization disabled due to lack of shared memory support in the kernel\n");
            return;
        }
#endif

        srand((unsigned int) time(NULL));
        signature = ((unsigned int) (65536.0 / (RAND_MAX + 1.0) * rand()) << 16)
            + (unsigned int) (65536.0 / (RAND_MAX + 1.0) * rand());
        /* fprintf(stderr, "signature = 0x%08X\n", signature); */

        FontShmdescIndex = xfont2_allocate_font_private_index();

#if !defined(CSRG_BASED)
        pagesize = SHMLBA;
#else
#ifdef _SC_PAGESIZE
        pagesize = sysconf(_SC_PAGESIZE);
#else
        pagesize = getpagesize();
#endif
#endif
#endif /* CONFIG_MITSHM */
    }
}

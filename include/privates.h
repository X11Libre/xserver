/***********************************************************

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

// FIXME: move out non-exported functions

#ifndef PRIVATES_H
#define PRIVATES_H 1

#include <X11/Xdefs.h>
#include <X11/Xosdefs.h>
#include <X11/Xfuncproto.h>
#include <assert.h>
#include "misc.h"

/*****************************************************************
 * STUFF FOR PRIVATES
 *****************************************************************/

typedef struct _Private PrivateRec, *PrivatePtr;

/* WARNING: the values, as well as the total number are part of public ABI.
   Adding a new one will lead to increased size as well as different field
   offsets within ScreenRec.
*/
typedef enum {
    /* XSELinux uses the same private keys for numerous objects

       This black magic - keys of this type have very special handling:
       their corresponding space is allocated at the top of the private
       areas, in *several* object types (see xselinux_private[] array),
       and xselinux uses the same keys for all object types
    */
    PRIVATE_XSELINUX,

    /* Otherwise, you get a private in just the requested structure
     */
    /* These can have objects created before all of the keys are registered */
    PRIVATE_SCREEN,
    PRIVATE_EXTENSION,
    PRIVATE_COLORMAP,
    PRIVATE_DEVICE,

    /* These cannot have any objects before all relevant keys are registered */
    PRIVATE_CLIENT,
    PRIVATE_PROPERTY,
    PRIVATE_SELECTION,
    PRIVATE_WINDOW,
    PRIVATE_PIXMAP,
    PRIVATE_GC,
    PRIVATE_CURSOR,
    PRIVATE_CURSOR_BITS,

    /* extension privates */
    PRIVATE_GLYPH,
    PRIVATE_GLYPHSET,
    PRIVATE_PICTURE,
    PRIVATE_SYNC_FENCE,

    /* last private type */
    PRIVATE_LAST,
} DevPrivateType;

typedef struct _DevPrivateKeyRec {
    int offset;
    int size;
    Bool initialized;
    Bool allocated;
    DevPrivateType type;
    struct _DevPrivateKeyRec *next;
} DevPrivateKeyRec, *DevPrivateKey;

typedef struct _DevPrivateSetRec {
    DevPrivateKey key;
    unsigned offset;
    int created;
    int allocated;
} DevPrivateSetRec, *DevPrivateSetPtr;

typedef struct _DevScreenPrivateKeyRec {
    DevPrivateKeyRec screenKey;
} DevScreenPrivateKeyRec, *DevScreenPrivateKeyPtr;

/*
 * Let drivers know how to initialize private keys
 */

#define HAS_DEVPRIVATEKEYREC		1
#define HAS_DIXREGISTERPRIVATEKEY	1

/*
 * @brief Register a new private index for the private type.
 *
 * This initializes the specified key and optionally requests pre-allocated
 * private space for your driver/module. If you request no extra space, you
 * may set and get a single pointer value using this private key. Otherwise,
 * you can get the address of the extra space and store whatever data you like
 * there.
 *
 * Maybe called multiple times on the same key, but the size and type must
 * match or the server will abort.
 *
 * Note: this may move around the private storage area to different address,
 * thus any pointers taken by GetPrivateAddr() et al have to be considered
 * invalid after calling this function.
 *
 * @param key   pointer to key (will be written to)
 * @param type  the object type the key is used for
 * @param size  size of the storage reserved for that key (zero => void*)
 * @return      FALSE if it fails to allocate memory during its operation.
 */
_X_EXPORT Bool  dixRegisterPrivateKey(DevPrivateKey key, DevPrivateType type, unsigned size);

/*
 * Check whether a private key has been registered
 */
static inline Bool
dixPrivateKeyRegistered(DevPrivateKey key)
{
    return key->initialized;
}

/*
 * Get the address of the private storage.
 *
 * For keys with pre-defined storage, this gets the base of that storage
 * Otherwise, it returns the place where the private pointer is stored.
 */
static inline void *
dixGetPrivateAddr(PrivatePtr *privates, const DevPrivateKey key)
{
    assert(key->initialized);
    return (char *) (*privates) + key->offset;
}

/*
 * Fetch a private pointer stored in the object
 *
 * Returns the pointer stored with dixSetPrivate.
 * This must only be used with keys that have
 * no pre-defined storage
 */
static inline void *
dixGetPrivate(PrivatePtr *privates, const DevPrivateKey key)
{
    assert(key->size == 0);
    return *(void **) dixGetPrivateAddr(privates, key);
}

/*
 * Associate 'val' with 'key' in 'privates' so that later calls to
 * dixLookupPrivate(privates, key) will return 'val'.
 */
static inline void
dixSetPrivate(PrivatePtr *privates, const DevPrivateKey key, void *val)
{
    assert(key->size == 0);
    *(void **) dixGetPrivateAddr(privates, key) = val;
}

#include "dix.h"
#include "resource.h"

/*
 * Lookup a pointer to the private record.
 *
 * For privates with defined storage, return the address of the
 * storage. For privates without defined storage, return the pointer
 * contents
 */
static inline void *
dixLookupPrivate(PrivatePtr *privates, const DevPrivateKey key)
{
    if (key->size)
        return dixGetPrivateAddr(privates, key);
    else
        return dixGetPrivate(privates, key);
}

/*
 * Look up the address of the pointer to the storage
 *
 * This returns the place where the private pointer is stored,
 * which is only valid for privates without predefined storage.
 */
static inline void **
dixLookupPrivateAddr(PrivatePtr *privates, const DevPrivateKey key)
{
    assert(key->size == 0);
    return (void **) dixGetPrivateAddr(privates, key);
}

extern _X_EXPORT Bool

dixRegisterScreenPrivateKey(DevScreenPrivateKeyPtr key, ScreenPtr pScreen,
                            DevPrivateType type, unsigned size);

extern _X_EXPORT DevPrivateKey
 _dixGetScreenPrivateKey(const DevScreenPrivateKeyPtr key, ScreenPtr pScreen);

static inline void *
dixGetScreenPrivateAddr(PrivatePtr *privates, const DevScreenPrivateKeyPtr key,
                        ScreenPtr pScreen)
{
    return dixGetPrivateAddr(privates, _dixGetScreenPrivateKey(key, pScreen));
}

static inline void *
dixGetScreenPrivate(PrivatePtr *privates, const DevScreenPrivateKeyPtr key,
                    ScreenPtr pScreen)
{
    return dixGetPrivate(privates, _dixGetScreenPrivateKey(key, pScreen));
}

static inline void
dixSetScreenPrivate(PrivatePtr *privates, const DevScreenPrivateKeyPtr key,
                    ScreenPtr pScreen, void *val)
{
    dixSetPrivate(privates, _dixGetScreenPrivateKey(key, pScreen), val);
}

static inline void *
dixLookupScreenPrivate(PrivatePtr *privates, const DevScreenPrivateKeyPtr key,
                       ScreenPtr pScreen)
{
    return dixLookupPrivate(privates, _dixGetScreenPrivateKey(key, pScreen));
}

static inline void **
dixLookupScreenPrivateAddr(PrivatePtr *privates, const DevScreenPrivateKeyPtr key,
                           ScreenPtr pScreen)
{
    return dixLookupPrivateAddr(privates,
                                _dixGetScreenPrivateKey(key, pScreen));
}

/*
 * These functions relate to allocations related to a specific screen;
 * space will only be available for objects allocated for use on that
 * screen. As such, only objects which are related directly to a specific
 * screen are candidates for allocation this way, this includes
 * windows, pixmaps, gcs, pictures and colormaps. This key is
 * used just like any other key using dixGetPrivate and friends.
 *
 * This is distinctly different from the ScreenPrivateKeys above which
 * allocate space in global objects like cursor bits for a specific
 * screen, allowing multiple screen-related chunks of storage in a
 * single global object.
 */

#define HAVE_SCREEN_SPECIFIC_PRIVATE_KEYS       1

Bool
dixRegisterScreenSpecificPrivateKey(ScreenPtr pScreen, DevPrivateKey key,
                                    DevPrivateType type, unsigned size);

/* Clean up screen-specific privates before CloseScreen */
extern void
dixFreeScreenSpecificPrivates(ScreenPtr pScreen);

/* Initialize screen-specific privates in AddScreen */
extern void
dixInitScreenSpecificPrivates(ScreenPtr pScreen);

/* is this private created - so hotplug can avoid crashing */
Bool dixPrivatesCreated(DevPrivateType type);

extern _X_EXPORT void *
_dixAllocateScreenObjectWithPrivates(ScreenPtr pScreen,
                                     unsigned size,
                                     unsigned offset,
                                     DevPrivateType type);

#define dixAllocateScreenObjectWithPrivates(s, t, type) _dixAllocateScreenObjectWithPrivates(s, sizeof(t), offsetof(t, devPrivates), type)

extern _X_EXPORT int
dixScreenSpecificPrivatesSize(ScreenPtr pScreen, DevPrivateType type);

void
_dixInitScreenPrivates(ScreenPtr pScreen, PrivatePtr *privates, void *addr, DevPrivateType type);

#define dixInitScreenPrivates(s, o, v, type) _dixInitScreenPrivates(s, &(o)->devPrivates, (v), type);

/*
 * Allocates private data separately from main object.
 *
 * For objects created during server initialization, this allows those
 * privates to be re-allocated as new private keys are registered.
 *
 * This includes screens, the serverClient, default colormaps and
 * extensions entries.
 */
Bool
 dixAllocatePrivates(PrivatePtr *privates, DevPrivateType type);

/*
 * Frees separately allocated private data
 */
// FIXME: intel driver
extern _X_EXPORT void
 dixFreePrivates(PrivatePtr privates, DevPrivateType type);

/*
 * Initialize privates by zeroing them
 */
void
_dixInitPrivates(PrivatePtr *privates, void *addr, DevPrivateType type);

#define dixInitPrivates(o, v, type) _dixInitPrivates(&(o)->devPrivates, (v), type);

/*
 * Clean up privates
 */
void
 _dixFiniPrivates(PrivatePtr privates, DevPrivateType type);

#define dixFiniPrivates(o,t)	_dixFiniPrivates((o)->devPrivates,t)

/*
 * Allocates private data at object creation time. Required
 * for almost all objects, except for the list described
 * above for dixAllocatePrivates.
 */
void *_dixAllocateObjectWithPrivates(unsigned size,
                                                      unsigned clear,
                                                      unsigned offset,
                                                      DevPrivateType type);

#define dixAllocateObjectWithPrivates(t, type) (t *) _dixAllocateObjectWithPrivates(sizeof(t), sizeof(t), offsetof(t, devPrivates), type)

void
_dixFreeObjectWithPrivates(void *object, PrivatePtr privates,
                           DevPrivateType type);

#define dixFreeObjectWithPrivates(o,t) _dixFreeObjectWithPrivates(o, (o)->devPrivates, t)

/*
 * Return size of privates for the specified type
 */
int
 dixPrivatesSize(DevPrivateType type);

/*
 * Dump out private stats to ErrorF
 */
extern void
 dixPrivateUsage(void);

/*
 * Resets the privates subsystem.  dixResetPrivates is called from the main loop
 * before each server generation.  This function must only be called by main().
 */
void dixResetPrivates(void);

/*
 * Looks up the offset where the devPrivates field is located.
 *
 * Returns -1 if the specified resource has no dev privates.
 * The position of the devPrivates field varies by structure
 * and calling code might only know the resource type, not the
 * structure definition.
 */
int
 dixLookupPrivateOffset(RESTYPE type);

/*
 * Convenience macro for adding an offset to an object pointer
 * when making a call to one of the devPrivates functions
 */
// unexport
#define DEVPRIV_AT(ptr, offset) ((PrivatePtr *)((char *)(ptr) + offset))

#endif                          /* PRIVATES_H */

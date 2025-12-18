/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */

#include <dix-config.h>

#include "fb/fb_priv.h"

/* compat symbols for NVidia v.340 legacy driver */
extern _X_EXPORT DevPrivateKey fbGetGCPrivateKey(GCPtr pGC);
extern _X_EXPORT DevPrivateKey wfbGetGCPrivateKey(GCPtr pGC);

DevPrivateKey fbGetGCPrivateKey(GCPtr pGC)
{
    return &fbGetScreenPrivate((pGC)->pScreen)->gcPrivateKeyRec;
}

DevPrivateKey wfbGetGCPrivateKey(GCPtr pGC)
{
    return &fbGetScreenPrivate((pGC)->pScreen)->gcPrivateKeyRec;
}

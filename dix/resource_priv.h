/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_RESOURCE_PRIV_H
#define _XSERVER_DIX_RESOURCE_PRIV_H

#include "include/dix.h"
#include "include/resource.h"

#define SameClient(obj,client) \
        (CLIENT_BITS((obj)->resource) == (client)->clientAsMask)

/*
 * Resource IDs having that bit set still belonging to some client,
 * but are server-internal, thus invisible to clients.
 */
#define SERVER_BIT           (Mask)0x40000000        /* use illegal bit */

/* client field */
#define RESOURCE_CLIENT_MASK   (((1 << ResourceClientBits()) - 1) << CLIENTOFFSET)

/* bits and fields within a resource id */
#define RESOURCE_AND_CLIENT_COUNT   29  /* 29 bits for XIDs */
#define CLIENTOFFSET     (RESOURCE_AND_CLIENT_COUNT - ResourceClientBits())

/* extract the client mask from an XID */
#define CLIENT_BITS(id) ((id) & RESOURCE_CLIENT_MASK)

/* resource field */
#define RESOURCE_ID_MASK        ((1 << CLIENTOFFSET) - 1)

/*
 * @brief retrieve client that owns given window
 *
 * XIDs carry the ID of the client who created/owns the resource in upper bits.
 * (every client so is assigned a range of XIDs it may use for resource creation)
 *
 * @param WindowPtr to the window whose client shall be retrieved
 * @return pointer to ClientRec structure or NullClient (NULL)
 */
ClientPtr dixClientForWindow(WindowPtr pWin);

/*
 * @brief retrieve client that owns given grab
 *
 * XIDs carry the ID of the client who created/owns the resource in upper bits.
 * (every client so is assigned a range of XIDs it may use for resource creation)
 *
 * @param GrabPtr to the grab whose owning client shall be retrieved
 * @return pointer to ClientRec structure or NullClient (NULL)
 */
ClientPtr dixClientForGrab(GrabPtr pGrab);

/*
 * @brief retrieve client that owns InputClients
 *
 * XIDs carry the ID of the client who created/owns the resource in upper bits.
 * (every client so is assigned a range of XIDs it may use for resource creation)
 *
 * @param GrabPtr to the InputClients whose owning client shall be retrieved
 * @return pointer to ClientRec structure or NullClient (NULL)
 */
ClientPtr dixClientForInputClients(InputClientsPtr pInputClients);

/*
 * @brief retrieve client that owns OtherClients
 *
 * XIDs carry the ID of the client who created/owns the resource in upper bits.
 * (every client so is assigned a range of XIDs it may use for resource creation)
 *
 * @param GrabPtr to the OtherClients whose owning client shall be retrieved
 * @return pointer to ClientRec structure or NullClient (NULL)
 */
ClientPtr dixClientForOtherClients(OtherClientsPtr pOtherClients);

/*
 * @brief extract client ID from XID
 *
 * XIDs carry the ID of the client who created/owns the resource in upper bits.
 * (every client so is assigned a range of XIDs it may use for resource creation)
 *
 * This ID is frequently used as table index, eg. for client or resource lookup.
 *
 * @param XID the ID of the resource whose client is retrieved
 * @return index of the client (within client or resource table)
 */
static inline int dixClientIdForXID(XID xid) {
    return ((int)(CLIENT_BITS(xid) >> CLIENTOFFSET));
}

/*
 * @brief retrieve client pointer from XID
 *
 * XIDs carry the ID of the client who created/owns the resource in upper bits.
 * (every client so is assigned a range of XIDs it may use for resource creation)
 *
 * @param XID the ID of the resource whose client is retrieved
 * @return pointer to ClientRec structure or NullClient (NULL)
 */
static inline ClientPtr dixClientForXID(XID xid) {
    const int idx = dixClientIdForXID(xid);
    if (idx < MAXCLIENTS)
        return clients[idx];
    return NullClient;
}

/*
 * @brief check whether resource is owned by server
 *
 * @param XID the ID of the resource to check
 * @return TRUE if resource is server owned
 */
static inline Bool dixResouceIsServerOwned(XID xid) {
    return (dixClientForXID(xid) == serverClient);
}

#endif /* _XSERVER_DIX_RESOURCE_PRIV_H */

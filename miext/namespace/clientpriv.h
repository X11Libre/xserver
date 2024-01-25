#ifndef __XSERVER_NAMESPACE_CLIENTPRIV_H
#define __XSERVER_NAMESPACE_CLIENTPRIV_H

#include <stdbool.h>

#include "windowstr.h"

struct Xnamespace;

/* This is what we store as client security state */
struct client_priv {
    bool isServer;
    XID authId;
    struct Xnamespace* ns;
};

/* Private state record */
static DevPrivateKeyRec stateKeyRec;

/* retrieve per client private structure */
static inline struct client_priv *clientPriv(ClientPtr client) {
    return dixLookupPrivate(&client->devPrivates, &stateKeyRec);
}

void clientAssignNS(struct client_priv *priv, struct Xnamespace *newns);
void clientAssignNSByName(struct client_priv *priv, const char *nsname);
bool clientSameNS(struct client_priv *p1, struct client_priv *p2);
int isServer(ClientPtr client);
int isSameContainer(ClientPtr client1, int client2);

#endif /* __XSERVER_NAMESPACE_CLIENTPRIV_H */

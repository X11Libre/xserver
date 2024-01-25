
#include <dix-config.h>

#include "windowstr.h"
#include "propertyst.h"

#include "namespace.h"
#include "clientpriv.h"

void clientAssignNS(struct client_priv *priv, struct Xnamespace *newns) {
    if (priv->ns != NULL)
        priv->ns->refcnt--;

    priv->ns = newns;
    if (newns != NULL)
        newns->refcnt++;
}

void clientAssignNSByName(struct client_priv *priv, const char *nsname)
{
    struct Xnamespace *ns = nsFindByName(nsname);

    // FIXME: is this a good thing to do ?
    if (ns == NULL)
        ns = &namespaces[NS_ANONYMOUS];

    clientAssignNS(priv, ns);
}

bool clientSameNS(struct client_priv *p1, struct client_priv *p2)
{
    return (p1->ns == p2->ns);
}

int isServer(ClientPtr client) {
    struct client_priv *subj = clientPriv(client);
    return subj->isServer;
}

// fixme
int isSameContainer(ClientPtr client1, int client2) {
    // same client means same container
    if (client1->index == client2) {
        return 1;
    }
    // server itself has super power
    return isServer(client1);
}

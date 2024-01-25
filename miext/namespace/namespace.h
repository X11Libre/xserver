#ifndef __XSERVER_NAMESPACE_H
#define __XSERVER_NAMESPACE_H

#include <dix-config.h>

#include "windowstr.h"
#include "propertyst.h"
#include "xacestr.h"
#include "registry.h"
#include "extinit.h"
#include "extnsionst.h"
#include "protocol-versions.h"

/* X namespace structure */
struct Xnamespace {
    const char *id;
    const char *parentId;
    const char *authProto;
    const char *authToken;
    char builtin:1;
    char isolateObjects:1;
    char isolatePointer:1;
    char superPower:1;
    char allow_mouse_motion:1;
    int refcnt;
};

// FIXME: need a dynamic list
// FIXME: support multiple auth tokens per container
#define MAX_NAMESPACE 64
extern struct Xnamespace namespaces[MAX_NAMESPACE];
extern int namespace_cnt;

#define NS_ROOT 0
#define NS_ANONYMOUS 1

// FIXME: make them _X_INTERNAL ?
struct Xnamespace *nsFindByName(const char* nsname);
struct Xnamespace* findNSByAuth(const char *proto, const char *token);
struct Xnamespace* findNSByAuth2(int proto_n, const char* auth_proto, int string_n, const char* auth_string);
void loadConfig(void);

#endif /* __XSERVER_NAMESPACE_H */

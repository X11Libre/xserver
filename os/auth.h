#ifndef _XSERVER_OS_AUTH_H
#define _XSERVER_OS_AUTH_H

#include <X11/X.h>

#include "dix.h"

#define XAUTH_PROTO_MIT "MIT-MAGIC-COOKIE-1"
#define XAUTH_PROTO_XDM "XDM-AUTHORIZATION-1"

#define AuthInitArgs void
typedef void (*AuthInitFunc) (AuthInitArgs);

#define AuthAddCArgs unsigned short data_length, const char *data
typedef XID (*AuthAddCFunc) (AuthAddCArgs);

#define AuthCheckArgs unsigned short data_length, const char *data, ClientPtr client, const char **reason
typedef XID (*AuthCheckFunc) (AuthCheckArgs);

#define AuthFromIDArgs XID id, unsigned short *data_lenp, char **datap
typedef int (*AuthFromIDFunc) (AuthFromIDArgs);

#define AuthGenCArgs unsigned data_length, const char *data, unsigned *data_length_return, char **data_return
typedef XID (*AuthGenCFunc) (AuthGenCArgs);

#define AuthRemCArgs unsigned short data_length, const char *data
typedef int (*AuthRemCFunc) (AuthRemCArgs);

#define AuthRstCArgs void
typedef int (*AuthRstCFunc) (AuthRstCArgs);

int set_font_authorizations(char **authorizations,
                            int *authlen,
                            void *client);

#define LCC_UID_SET     (1 << 0)
#define LCC_GID_SET     (1 << 1)
#define LCC_PID_SET     (1 << 2)
#define LCC_ZID_SET     (1 << 3)

typedef struct {
    int fieldsSet;              /* Bit mask of fields set */
    int euid;                   /* Effective uid */
    int egid;                   /* Primary effective group id */
    int nSuppGids;              /* Number of supplementary group ids */
    int *pSuppGids;             /* Array of supplementary group ids */
    int pid;                    /* Process id */
    int zoneid;                 /* Only set on Solaris 10 & later */
} LocalClientCredRec;

int GetLocalClientCreds(ClientPtr, LocalClientCredRec **);
void FreeLocalClientCreds(LocalClientCredRec *);

void EnableLocalAccess(void);
void DisableLocalAccess(void);

void LocalAccessScopeUser(void);

void InitAuthorization(const char *filename);

int AuthorizationFromID(XID id,
                        unsigned short *name_lenp,
                        const char **namep,
                        unsigned short *data_lenp, char **datap);

XID CheckAuthorization(unsigned int namelength,
                       const char *name,
                       unsigned int datalength,
                       const char *data,
                       ClientPtr client,
                       const char **reason);

void ResetAuthorization(void);

int RemoveAuthorization(unsigned short name_length,
                        const char *name,
                        unsigned short data_length, const char *data);

int AddAuthorization(unsigned int name_length,
                     const char *name,
                     unsigned int data_length,
                     char *data);

XID GenerateAuthorization(unsigned int name_length,
                          const char *name,
                          unsigned int data_length,
                          const char *data,
                          unsigned int *data_length_return,
                          char **data_return);

void RegisterAuthorizations(void);

void CheckUserAuthorization(void);

typedef struct sockaddr *sockaddrPtr;

int AddHost(ClientPtr client, int family, unsigned length, const void *pAddr);
Bool ForEachHostInFamily(int family,
                         Bool (*func)(unsigned char *addr, short len, void *closure),
                         void *closure);
int RemoveHost(ClientPtr client, int family, unsigned length, void *pAddr);
int GetHosts(void **data, int *pnHosts, int *pLen, BOOL *pEnabled);
int InvalidHost(sockaddrPtr saddr, int len, ClientPtr client);
void AddLocalHosts(void);
void ResetHosts(const char *display);

/* register local hosts entries for outself, based on listening fd */
void DefineSelf(int fd);

/* check whether given addr belongs to ourself */
void AugmentSelf(void *from, int len);

int ChangeAccessControl(ClientPtr client, int fEnabled);

void AccessUsingXdmcp(void);

extern Bool defeatAccessControl;

#endif /* _XSERVER_OS_AUTH_H */

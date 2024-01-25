#include <dix-config.h>

#include <X11/Xfuncproto.h>

#define TRANS_REOPEN
#define TRANS_SERVER
#define XSERV_t
#include <X11/Xtrans/transport.c>

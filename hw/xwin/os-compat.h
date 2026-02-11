#ifndef __XWIN_OS_COMPAT_H
#define __XWIN_OS_COMPAT_H

#include <stdlib.h>
#include <errno.h>

/* special workaround for mingw lacking setenv() */
#ifndef HAVE_SETENV
static inline int setenv(const char *name, const char *value, int overwrite)
{
    size_t name_len = strlen(name);
    size_t value_len = strlen(value);
    size_t bufsz = name_len + value_len + 1;
    char *buf = malloc(bufsz);
    if (!buf) {
        errno = ENOMEM;
        return -1;
    }
    memcpy(buf, name, name_len);
    memcpy(buf+name_len, value, value_len);
    buf[name_len+value_len] = 0;
    putenv(buf);
    return 0;
}
#endif

#endif /* __XWIN_OS_COMPAT_H */

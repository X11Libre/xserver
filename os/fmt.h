/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_OS_FMT_H
#define _XSERVER_OS_FMT_H

#include <stdint.h>
#include <X11/Xfuncproto.h>

void FormatInt64(int64_t num, char *string);
void FormatUInt64(uint64_t num, char *string);
void FormatUInt64Hex(uint64_t num, char *string);
void FormatDouble(double dbl, char *string);

/**
 * Compare the two version numbers comprising of major.minor.
 *
 * @return A value less than 0 if a is less than b, 0 if a is equal to b,
 * or a value greater than 0
 */
static inline int
version_compare(uint32_t a_major, uint32_t a_minor,
                uint32_t b_major, uint32_t b_minor)
{
    if (a_major > b_major)
        return 1;
    if (a_major < b_major)
        return -1;
    if (a_minor > b_minor)
        return 1;
    if (a_minor < b_minor)
        return -1;

    return 0;
}

/* still needed as long as modesetting is a module */
_X_EXPORT char **xstrtokenize(const char *str, const char *separators);

#endif /* _XSERVER_OS_FMT_H */

#include <dix-config.h>

#include <X11/Xfuncproto.h>

#undef Ones

/*
 * this is specifically for NVidia proprietary driver: they're again lagging
 * behind a year, doing at least some minimal cleanup of their code base.
 * All attempts to get in direct contact with them have failed.
 */

/*
 * this is only needed for the 570.x nvidia drivers
 */

_X_EXPORT int Ones(unsigned long /*mask */ );

int
Ones(unsigned long mask)
{                               /* HACKMEM 169 */
    /* can't add a message here because this should be fast */
#if __has_builtin(__builtin_popcountl)
    return __builtin_popcountl (mask);
#else
    unsigned long y;

    y = (mask >> 1) & 033333333333;
    y = mask - y - ((y >> 1) & 033333333333);
    return (((y + (y >> 3)) & 030707070707) % 077);
#endif
}

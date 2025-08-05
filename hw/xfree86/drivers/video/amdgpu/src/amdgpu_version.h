/*
 * Copyright 2000 through 2004 by Marc Aurele La France (TSI @ UQV), tsi@xfree86.org
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of Marc Aurele La France not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Marc Aurele La France makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as-is" without express or implied warranty.
 *
 * MARC AURELE LA FRANCE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL MARC AURELE LA FRANCE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _AMDGPU_VERSION_H_
#define _AMDGPU_VERSION_H_ 1

#undef  AMDGPU_NAME
#undef  AMDGPU_DRIVER_NAME
#undef  R200_DRIVER_NAME
#undef  AMDGPU_VERSION_MAJOR
#undef  AMDGPU_VERSION_MINOR
#undef  AMDGPU_VERSION_PATCH
#undef  AMDGPU_VERSION_CURRENT
#undef  AMDGPU_VERSION_EVALUATE
#undef  AMDGPU_VERSION_STRINGIFY
#undef  AMDGPU_VERSION_NAME

#define AMDGPU_NAME          "AMDGPU"
#define AMDGPU_DRIVER_NAME   "amdgpu"
#define SI_DRIVER_NAME       "radeonsi"

#define AMDGPU_VERSION_MAJOR PACKAGE_VERSION_MAJOR
#define AMDGPU_VERSION_MINOR PACKAGE_VERSION_MINOR
#define AMDGPU_VERSION_PATCH PACKAGE_VERSION_PATCHLEVEL

#ifndef AMDGPU_VERSION_EXTRA
#define AMDGPU_VERSION_EXTRA ""
#endif

#define AMDGPU_VERSION_CURRENT \
    ((AMDGPU_VERSION_MAJOR << 20) | \
     (AMDGPU_VERSION_MINOR << 10) | \
     (AMDGPU_VERSION_PATCH))

#define AMDGPU_VERSION_EVALUATE(__x) #__x
#define AMDGPU_VERSION_STRINGIFY(_x) AMDGPU_VERSION_EVALUATE(_x)
#define AMDGPU_VERSION_NAME                                             \
    AMDGPU_VERSION_STRINGIFY(AMDGPU_VERSION_MAJOR) "."                  \
    AMDGPU_VERSION_STRINGIFY(AMDGPU_VERSION_MINOR) "."                  \
    AMDGPU_VERSION_STRINGIFY(AMDGPU_VERSION_PATCH) AMDGPU_VERSION_EXTRA

#endif /* _AMDGPU_VERSION_H_ */

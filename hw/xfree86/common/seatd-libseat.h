/*
 * Copyright © 2022-2024 Mark Hindley, Ralph Ronnquist.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors: Mark Hindley <mark@hindley.org.uk>
 *          Ralph Ronnquist <ralph.ronnquist@gmail.com>
 */

#ifndef SEATD_LIBSEAT_H
#define SEATD_LIBSEAT_H

#ifdef SEATD_LIBSEAT
#include <xf86Xinput.h>
extern int seatd_libseat_init(BOOL KeepTty_state);
extern void seatd_libseat_fini(void);
extern _X_EXPORT int seatd_libseat_open_graphics(const char *path);
extern void seatd_libseat_open_device(InputInfoPtr p,int *fd,Bool *paus);
extern void seatd_libseat_close_device(InputInfoPtr p);
extern int seatd_libseat_switch_session(int session);
extern _X_EXPORT Bool seatd_libseat_controls_session(void);
#else

static inline int seatd_libseat_init(BOOL KeepTty_state) {(void)KeepTty_state; return -1; };
static inline void seatd_libseat_fini(void) {};
static inline int seatd_libseat_open_graphics(const char *path) {(void)path; return -1; }
static inline void seatd_libseat_open_device(void *p,int *fd, Bool *paus) { (void)p;(void)fd;(void)paus; };
static inline void seatd_libseat_close_device(void *p) { (void)p;};
static inline int seatd_libseat_switch_session(int session) { return -1; };
static inline Bool seatd_libseat_controls_session(void) { return FALSE; };

#endif

#endif

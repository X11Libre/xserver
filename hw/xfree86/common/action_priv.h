/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef __XSERVER_XFREE86_ACTION_PRIV_H
#define __XSERVER_XFREE86_ACTION_PRIV_H

typedef enum {
    ACTION_TERMINATE          = 0,    /* Terminate Server */
    ACTION_NEXT_MODE          = 10,   /* Switch to next video mode */
    ACTION_PREV_MODE,
    ACTION_SWITCHSCREEN       = 100,  /* VT switch */
    ACTION_SWITCHSCREEN_NEXT,
    ACTION_SWITCHSCREEN_PREV,
} ActionEvent;

void xf86ProcessActionEvent(ActionEvent action, void *arg);

#endif /* __XSERVER_XFREE86_ACTION_PRIV_H */

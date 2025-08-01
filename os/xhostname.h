/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_OS_XHOSTNAME_H_
#define _XSERVER_OS_XHOSTNAME_H_

#define XHOSTNAME_MAX 2048

struct xhostname {
    char name[XHOSTNAME_MAX];
};

/*
 * retrieve host's nodename. basically a safer way of gethostname() / uname()
 * making sure that the nodename is always zero-terminated.
 *
 * @hn pointer to struct xhostname that will be filled
 * @return 0 on success
 */
int xhostname(struct xhostname* hn);

#endif /* _XSERVER_OS_XHOSTNAME_H_ */

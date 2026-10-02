/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_CONNSETUP_PRIV_H
#define _XSERVER_DIX_CONNSETUP_PRIV_H

void dixSendConnAbort(ClientPtr pClient, const char *reason);

/*
 * send an aborting connection setup packet to given client
 *
 * @param client    the client to send to
 * @param reason    the abort reason
 */
void dixSendConnAbort(ClientPtr pClient, const char *reason);

/*
 * create ConnectionInfo block. aborts the server on failure
 */
void dixInitConnectionBlock(void);

/*
 * write xWindowRoot protocol structure into rpcbuf
 */
void x_rpcbuf_write_xWindowRoot(x_rpcbuf_t *rpcbuf, ScreenPtr pScreen);

/*
 * write xDepth protocol structure into rpcbuf
 */
void x_rpcbuf_write_xDepth(x_rpcbuf_t *rpcbuf, DepthPtr pDepth);

/*
 * write xVisualInfo protocol structure into rpcbuf
 */
void x_rpcbuf_write_xVisualInfo(x_rpcbuf_t *rpcbuf, VisualPtr pVisual);

/*
 * write xPixmapFormat protocol structure into rpcbuf
 */
void x_rpcbuf_write_xPixmapFormat(x_rpcbuf_t *rpcbuf, PixmapFormatPtr pPixmapFormat);

/*
 * build the connection info block and return it as x_rpcbuf_t
 *
 * @param   maxscreens       only process so much screens (=0 -> do them all)
 * @return  x_rpcbuf_t holding the connection info data (caller has ownerhip)
 */
x_rpcbuf_t dixBuildConnectionBlock(int maxscreens);

/*
 * generate a ConnectionInfo block and return it as a separate malloc'ed buffer
 */
char *dixNewConnectionInfoBlock(ClientPtr pClient, size_t *sz, size_t *scrOffset);

#endif /* _XSERVER_DIX_CONNSETUP_PRIV_H */

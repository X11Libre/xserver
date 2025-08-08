/************************************************************

Copyright 1996 by Thomas E. Dickey <dickey@clark.net>

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of the above listed
copyright holder(s) not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.

THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM ALL WARRANTIES WITH REGARD
TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE
LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#ifndef SWAPREP_H
#define SWAPREP_H 1

void SwapFont(xQueryFontReply * pr, Bool hasGlyphs);

extern void Swap32Write(ClientPtr /* pClient */ ,
                        int /* size */ ,
                        CARD32 * /* pbuf */ );

extern void CopySwap32Write(ClientPtr /* pClient */ ,
                            int /* size */ ,
                            CARD32 * /* pbuf */ );

extern void CopySwap16Write(ClientPtr /* pClient */ ,
                            int /* size */ ,
                            short * /* pbuf */ );

extern void SGenericReply(ClientPtr /* pClient */ ,
                          int /* size */ ,
                          xGenericReply * /* pRep */ );

extern void SErrorEvent(xError * /* from */ ,
                        xError * /* to */ );

extern void SwapConnSetupInfo(char * /* pInfo */ ,
                              char * /* pInfoTBase */ );

extern void WriteSConnectionInfo(ClientPtr /* pClient */ ,
                                 unsigned long /* size */ ,
                                 char * /* pInfo */ );

extern void SwapConnSetupPrefix(xConnSetupPrefix * /* pcspFrom */ ,
                                xConnSetupPrefix * /* pcspTo */ );

extern void WriteSConnSetupPrefix(ClientPtr /* pClient */ ,
                                  xConnSetupPrefix * /* pcsp */ );

#undef SWAPREP_PROC
#define SWAPREP_PROC(func) extern void func(xEvent * /* from */, xEvent * /* to */)

SWAPREP_PROC(SCirculateEvent);
SWAPREP_PROC(SClientMessageEvent);
SWAPREP_PROC(SColormapEvent);
SWAPREP_PROC(SConfigureNotifyEvent);
SWAPREP_PROC(SConfigureRequestEvent);
SWAPREP_PROC(SCreateNotifyEvent);
SWAPREP_PROC(SDestroyNotifyEvent);
SWAPREP_PROC(SEnterLeaveEvent);
SWAPREP_PROC(SExposeEvent);
SWAPREP_PROC(SFocusEvent);
SWAPREP_PROC(SGraphicsExposureEvent);
SWAPREP_PROC(SGravityEvent);
SWAPREP_PROC(SKeyButtonPtrEvent);
SWAPREP_PROC(SKeymapNotifyEvent);
SWAPREP_PROC(SMapNotifyEvent);
SWAPREP_PROC(SMapRequestEvent);
SWAPREP_PROC(SMappingEvent);
SWAPREP_PROC(SNoExposureEvent);
SWAPREP_PROC(SPropertyEvent);
SWAPREP_PROC(SReparentEvent);
SWAPREP_PROC(SResizeRequestEvent);
SWAPREP_PROC(SSelectionClearEvent);
SWAPREP_PROC(SSelectionNotifyEvent);
SWAPREP_PROC(SSelectionRequestEvent);
SWAPREP_PROC(SUnmapNotifyEvent);
SWAPREP_PROC(SVisibilityEvent);

#undef SWAPREP_PROC

#endif                          /* SWAPREP_H */

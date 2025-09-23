/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_OS_AUDIT_H
#define _XSERVER_OS_AUDIT_H

#include <stdarg.h>
#include <X11/Xfuncproto.h>

extern int auditTrailLevel;

void FreeAuditTimer(void);

void AuditF(const char *f, ...) _X_ATTRIBUTE_PRINTF(1, 2);
void VAuditF(const char *f, va_list args) _X_ATTRIBUTE_PRINTF(1, 0);

#endif /* _XSERVER_OS_AUDIT_H */

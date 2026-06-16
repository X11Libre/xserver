/* SPDX-License-Identifier: MIT OR X11 OR AGPLv3
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * ProcVector monitoring interface.
 */

#ifndef PROCVECTOR_MONITOR_H
#define PROCVECTOR_MONITOR_H

extern void ProcVectorMonitorInit(void);
extern void ProcVectorMonitorFini(void);
extern void ProcVectorMonitorCheckAfterModule(const char *module_name);
extern void ProcVectorMonitorCheckNVIDIA(void);

#endif /* PROCVECTOR_MONITOR_H */

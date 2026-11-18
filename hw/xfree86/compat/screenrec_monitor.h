/* SPDX-License-Identifier: MIT OR X11 OR AGPLv3
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * ScreenRec monitoring interface.
 */

#ifndef SCREENREC_MONITOR_H
#define SCREENREC_MONITOR_H

extern void ScreenRecMonitorInit(void);
extern void ScreenRecMonitorFini(void);
extern void ScreenRecMonitorCheckAfterModule(const char *module_name);

#endif /* SCREENREC_MONITOR_H */

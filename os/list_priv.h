/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XLIBRE_OS_LIST_PRIV_H_
#define _XLIBRE_OS_LIST_PRIV_H_

#include "include/list.h"

/**
 * @brief iterate @p head, executing @p action after each @p cond match,
 *        restarting the iteration from the head afterwards.
 *
 * xorg_list_for_each_entry_safe() only protects against removal of the
 * current element (@p pos): it caches the next pointer before the loop body
 * runs and resumes from it. That is unsafe if the loop body -- or a callback
 * it invokes (e.g. a vblank/pageflip handler) -- removes and frees arbitrary
 * other list entries, in particular the cached next element. Resuming from
 * the freed element would then dereference memory that was already free()'d
 * (use-after-free).
 *
 * This macro instead restarts the scan from the list head after every
 * match, so it is correct whenever such callbacks may mutate the list
 * arbitrarily. Each match (re)scans the remaining list from the head, i.e.
 * the cost is O(n) per match (quadratic for n matches), which is acceptable
 * for short lists with few matches.
 *
 * @warning state that must be re-evaluated across restarts has to be derived
 *          from @p cond, not assumed from the loop position (iteration always
 *          begins again at the first list entry after a match).
 *
 * @param pos    Iterator variable of the type of the list elements.
 * @param head   List head.
 * @param member Member name of the struct xorg_list in the list elements.
 * @param cond   Condition selecting entries to act on (re-evaluated each pass).
 * @param action Statement to execute for each matching entry.
 */
#define xlibre_list_for_each_entry_restart(pos, head, member, cond, action) \
    do {                                                                   \
        int _xlibre_list_restarted_;                                       \
        do {                                                               \
            _xlibre_list_restarted_ = 0;                                   \
            xorg_list_for_each_entry(pos, head, member) {                  \
                if (cond) {                                                \
                    action;                                                \
                    _xlibre_list_restarted_ = 1;                           \
                    break;                                                 \
                }                                                          \
            }                                                              \
        } while (_xlibre_list_restarted_);                                 \
    } while (0)

#endif /* _XLIBRE_OS_LIST_PRIV_H_ */

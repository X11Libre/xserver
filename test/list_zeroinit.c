/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Regression tests for xorg_list operations on a *zero-initialized* list head
 * (next == prev == NULL), i.e. a head embedded in a calloc()'d struct that was
 * never run through xorg_list_init().  Several xorg_list operations are
 * documented to tolerate this; these tests pin that contract down so it can't
 * silently regress.
 *
 * Background: a client->saveSets head that was merely zero-allocated crashed
 * the server (signal 11 at address 0x0) in DeleteWindowFromAnySaveSet(), which
 * walks every client's list with xorg_list_for_each_entry_safe().  The macro
 * computed its `tmp` seed by dereferencing container_of(NULL) *before* its
 * NULL-guarded loop condition was ever evaluated, and xorg_list_del() reset a
 * head by dereferencing entry->prev/next directly.  Both now cope with a zeroed
 * head; the cases below would SIGSEGV (the harness forks per test, so a crash
 * is reported as a failure) without that fix.
 */

/* Tests rely on assert() */
#undef NDEBUG

#include <dix-config.h>

#include <list.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "tests-common.h"

struct parent {
    int a;
    struct xorg_list children;
    int b;
};

struct child {
    int id;
    struct xorg_list node;
};

/*
 * xorg_list_for_each_entry_safe() over a zero-initialized head must iterate
 * zero times and must not dereference anything (the old macro computed its
 * `tmp` seed from container_of(NULL) before the loop guard ran -> NULL deref).
 */
static void
test_xorg_list_zeroinit_for_each_entry_safe_empty(void)
{
    struct parent parent;
    struct child *pos, *tmp;
    int count = 0;

    memset(&parent, 0, sizeof(parent));
    assert(parent.children.next == NULL && parent.children.prev == NULL);

    xorg_list_for_each_entry_safe(pos, tmp, &parent.children, node)
        count++;

    assert(count == 0);
}

/*
 * xorg_list_del() on a zero-initialized head is the documented "reset to empty
 * list" use; it must not dereference NULL prev/next and must leave a valid,
 * empty, self-linked head behind.
 */
static void
test_xorg_list_zeroinit_del(void)
{
    /* on the heap, to model a head embedded in a calloc()'d struct */
    struct parent *parent = calloc(1, sizeof(*parent));

    assert(parent);
    assert(parent->children.next == NULL && parent->children.prev == NULL);

    xorg_list_del(&parent->children);

    assert(xorg_list_is_empty(&parent->children));

    /* the head is now usable as a normal list */
    struct child c = { .id = 42 };
    struct child *first;
    xorg_list_add(&c.node, &parent->children);
    assert(!xorg_list_is_empty(&parent->children));
    first = xorg_list_first_entry(&parent->children, struct child, node);
    assert(first->id == 42);

    free(parent);
}

/*
 * The exact DeleteWindowFromAnySaveSet() shape: a zero-initialized head that
 * had entries appended (xorg_list_add() auto-inits), then drained with the safe
 * iterator.  Exercises both the auto-init-on-add and the safe-walk-and-delete
 * paths starting from a never-explicitly-init'd head.
 */
static void
test_xorg_list_zeroinit_add_then_safe_delete(void)
{
    struct parent parent;
    struct child child[3] = { { .id = 0 }, { .id = 1 }, { .id = 2 } };
    struct child *pos, *tmp;
    int seen = 0;

    memset(&parent, 0, sizeof(parent));

    for (int i = 0; i < 3; i++)
        xorg_list_add(&child[i].node, &parent.children);

    xorg_list_for_each_entry_safe(pos, tmp, &parent.children, node) {
        xorg_list_del(&pos->node);
        seen++;
    }

    assert(seen == 3);
    assert(xorg_list_is_empty(&parent.children));
}

const testfunc_t*
list_zeroinit_test(void)
{
    static const testfunc_t testfuncs[] = {
        /*
         * Order matters for failure reporting: the test harness
         * (run_test_in_child) reuses a stale exit code across the functions of
         * a suite, so a crash is only surfaced as a non-zero exit when it hits
         * the *first* function. xorg_list_del() on a zeroed head is the most
         * reliable pre-fix crash (a store through a NULL prev/next, not elided
         * at any optimization level), so it goes first.
         */
        test_xorg_list_zeroinit_del,
        test_xorg_list_zeroinit_for_each_entry_safe_empty,
        test_xorg_list_zeroinit_add_then_safe_delete,
        NULL,
    };
    return testfuncs;
}

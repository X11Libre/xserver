#include <dix-config.h>

#include <misc.h>
#include <stdlib.h>
#include <stdio.h>
#include <libxht/xht.h>
#include "resource.h"

#include "tests-common.h"

static void
test1(void)
{
    xht_t *h;
    int c;
    int ok = 1;
    const int numKeys = 420;

    dbg("test1\n");
    h = xht_create_int_table(numKeys);

    for (c = 0; c < numKeys; ++c) {
      /* Start with 1 to avoid storing NULL */
      int val = 2 * c + 1;
      XID id = c;
      xht_set_int(h, id, (void *)(uintptr_t)val);
    }

    for (c = 0; c < numKeys; ++c) {
      XID id = c;
      void *p = xht_get_int(h, id);
      int expected = 2 * c + 1;
      if (p) {
        int v = (int)(uintptr_t)p;
        if (v != expected) {
          dbg("Key %d doesn't have expected value %d but has %d instead\n",
                 c, expected, v);
          ok = 0;
        }
      } else {
        ok = 0;
        dbg("Cannot find key %d\n", c);
      }
    }

    if (ok) {
      dbg("%d keys inserted and found\n", c);

      for (c = 0; c < numKeys; ++c) {
        XID id = c;
        xht_delete_int(h, id);
      }
    }

    xht_destroy_int_table(h);

    assert(ok);
}

static void
test2(void)
{
    xht_t *h;
    int c;
    int ok = 1;
    const int numKeys = 420;

    dbg("test2\n");
    h = xht_create_int_table(numKeys);

    for (c = 0; c < numKeys; ++c) {
      XID id = c;
      xht_set_int(h, id, (void *)(uintptr_t)1);
    }

    for (c = 0; c < numKeys; ++c) {
      XID id = c;
      if (!xht_get_int(h, id)) {
        ok = 0;
        dbg("Cannot find key %d\n", c);
      }
    }

    {
        XID id = c + 1;
        if (xht_get_int(h, id)) {
            ok = 0;
            dbg("Could find a key that shouldn't be there\n");
        }
    }

    xht_destroy_int_table(h);

    if (ok) {
        dbg("Test with empty keys OK\n");
    } else {
        dbg("Test with empty keys FAILED\n");
    }

    assert(ok);
}

static void
test3(void)
{
    int ok = 1;
    xht_t *h;
    dbg("test3\n");
    h = xht_create_string_table(2, false);

    if (!xht_set_string(h, "helo", (void *)(uintptr_t)1) ||
        !xht_set_string(h, "wrld", (void *)(uintptr_t)1)) {
        dbg("Could not insert keys\n");
    }

    if (!xht_get_string(h, "helo") ||
        !xht_get_string(h, "wrld")) {
        ok = 0;
        dbg("Could not find inserted keys\n");
    }

    xht_destroy_string_table(h);

    assert(ok);
}

const testfunc_t*
hashtabletest_test(void)
{
    static const testfunc_t testfuncs[] = {
        test1,
        test2,
        test3,
        NULL,
    };
    return testfuncs;
}

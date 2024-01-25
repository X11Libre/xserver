#include <dix-config.h>

#include <misc.h>
#include <stdlib.h>
#include <stdio.h>
#include "hashtable.h"
#include "resource.h"

#include "tests-common.h"

static void
print_xid(void* ptr, void* v)
{
    XID *x = v;
    dbg("%ld", (long)(*x));
}

static void
print_int(void* ptr, void* v)
{
    int *x = v;
    dbg("%d", *x);
}

static void
test1(void)
{
    HashTable h;
    int c;
    int ok = 1;
    const int numKeys = 420;

    dbg("test1\n");
    h = ht_create(sizeof(XID), sizeof(int), ht_resourceid_hash, ht_resourceid_compare, NULL);

    for (c = 0; c < numKeys; ++c) {
      int *dest;
      XID id = c;
      dest = ht_add(h, &id);
      if (dest) {
        *dest = 2 * c;
      }
    }

    if (verbose) {
      dbg("Distribution after insertion\n");
      ht_dump_distribution(h);
      ht_dump_contents(h, print_xid, print_int, NULL);
    }

    for (c = 0; c < numKeys; ++c) {
      XID id = c;
      int* v = ht_find(h, &id);
      if (v) {
        if (*v == 2 * c) {
          // ok
        } else {
          dbg("Key %d doesn't have expected value %d but has %d instead\n",
                 c, 2 * c, *v);
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
        ht_remove(h, &id);
      }

      if (verbose) {
        dbg("Distribution after deletion\n");
        ht_dump_distribution(h);
      }
    }

    ht_destroy(h);

    assert(ok);
}

static void
test2(void)
{
    HashTable h;
    int c;
    int ok = 1;
    const int numKeys = 420;

    dbg("test2\n");
    h = ht_create(sizeof(XID), 0, ht_resourceid_hash, ht_resourceid_compare, NULL);

    for (c = 0; c < numKeys; ++c) {
      XID id = c;
      ht_add(h, &id);
    }

    for (c = 0; c < numKeys; ++c) {
      XID id = c;
      if (!ht_find(h, &id)) {
        ok = 0;
        dbg("Cannot find key %d\n", c);
      }
    }

    {
        XID id = c + 1;
        if (ht_find(h, &id)) {
            ok = 0;
            dbg("Could find a key that shouldn't be there\n");
        }
    }

    ht_destroy(h);

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
    HtGenericHashSetupRec hashSetup = {
        .keySize = 4
    };
    HashTable h;
    dbg("test3\n");
    h = ht_create(4, 0, ht_generic_hash, ht_generic_compare, &hashSetup);

    if (!ht_add(h, "helo") ||
        !ht_add(h, "wrld")) {
        dbg("Could not insert keys\n");
    }

    if (!ht_find(h, "helo") ||
        !ht_find(h, "wrld")) {
        ok = 0;
        dbg("Could not find inserted keys\n");
    }

    if (verbose) {
       dbg("Hash distribution with two strings\n");
       ht_dump_distribution(h);
    }

    ht_destroy(h);

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

/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include <dix-config.h>

#include <stdio.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xatom.h>

#include "dix/atom_priv.h"
#include "dix/dix_priv.h"

#include "misc.h"
#include "resource.h"
#include "dix.h"
#include <libxht/xht.h>

#define InitialTableSize 256

static Atom lastAtom = None;
static xht_2way_t *atom_table;

Atom
MakeAtom(const char *string, unsigned len, Bool makeit)
{
    char *str;
    uint64_t atom_id;

    /* The X protocol limits atom names to US-ASCII characters */
    str = strndup(string, len);
    if (!str)
        return BAD_RESOURCE;

    atom_id = xht_atom_table_get_id(atom_table, str);
    if (atom_id != 0) { // 0 is None
        free(str);
        return (Atom)atom_id;
    }

    if (!makeit) {
        free(str);
        return None;
    }

    lastAtom++;
    if (!xht_atom_table_set(atom_table, str, lastAtom)) {
        free(str); // Free str if set fails
        return BAD_RESOURCE;
    }
    free(str); // Free str after successful set, as xht_atom_table_set copies it.

    return lastAtom;
}

Bool
ValidAtom(Atom atom)
{
    return (atom != None) && (atom <= lastAtom);
}

const char *
NameForAtom(Atom atom)
{
    if (atom > lastAtom)
        return NULL;
    return xht_atom_table_get_string(atom_table, atom);
}

void
FreeAllAtoms(void)
{
    if (!atom_table)
        return;

    xht_atom_table_destroy(atom_table);
    atom_table = NULL;
    lastAtom = None;
}

void
InitAtoms(void)
{
    FreeAllAtoms();
    atom_table = xht_atom_table_create(InitialTableSize);
    if (!atom_table)
        FatalError("creating atom table");

    MakePredeclaredAtoms();
    if (lastAtom != XA_LAST_PREDEFINED)
        FatalError("builtin atom number mismatch");
}

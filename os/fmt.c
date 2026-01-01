/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 * Copyright © 1987, 1998  The Open Group
 * Copyright © 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
 * Copyright © 1994 Quarterdeck Office Systems.
 */

#include <stdint.h>

#include "os/fmt.h"

/* Format a signed number into a string in a signal safe manner. The string
 * should be at least 21 characters in order to handle all int64_t values.
 */
void
FormatInt64(int64_t num, char *string)
{
    uint64_t unum = num;

    if (num < 0) {
        string[0] = '-';
        unum = num * -1;
        string++;
    }
    FormatUInt64(unum, string);
}

/* Format a number into a string in a signal safe manner. The string should be
 * at least 21 characters in order to handle all uint64_t values. */
void
FormatUInt64(uint64_t num, char *string)
{
    uint64_t divisor;
    int len;
    int i;

    for (len = 1, divisor = 10;
         len < 20 && num / divisor;
         len++, divisor *= 10);

    for (i = len, divisor = 1; i > 0; i--, divisor *= 10)
        string[i - 1] = '0' + ((num / divisor) % 10);

    string[len] = '\0';
}

/**
 * Format a double number as %.2f.
 */
void
FormatDouble(double dbl, char *string)
{
    int slen = 0;
    uint64_t frac;

    frac = (dbl > 0 ? dbl : -dbl) * 100.0 + 0.5;
    frac %= 100;

    /* write decimal part to string */
    if (dbl < 0 && dbl > -1)
        string[slen++] = '-';
    FormatInt64((int64_t)dbl, &string[slen]);

    while(string[slen] != '\0')
        slen++;

    /* append fractional part, but only if we have enough characters. We
     * expect string to be 21 chars (incl trailing \0) */
    if (slen <= 17) {
        string[slen++] = '.';
        if (frac < 10)
            string[slen++] = '0';

        FormatUInt64(frac, &string[slen]);
    }
}


/* Format a number into a hexadecimal string in a signal safe manner. The string
 * should be at least 17 characters in order to handle all uint64_t values. */
void
FormatUInt64Hex(uint64_t num, char *string)
{
    uint64_t divisor;
    int len;
    int i;

    for (len = 1, divisor = 0x10;
         len < 16 && num / divisor;
         len++, divisor *= 0x10);

    for (i = len, divisor = 1; i > 0; i--, divisor *= 0x10) {
        int val = (num / divisor) % 0x10;

        if (val < 10)
            string[i - 1] = '0' + val;
        else
            string[i - 1] = 'a' + val - 10;
    }

    string[len] = '\0';
}

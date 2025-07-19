/*
 * Copyright (c) 2025 Oleh Nykyforchyn. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "misc.h"
#include "xf86Parser.h"
#include "configProcs.h"

/*
 *  Utilities used by InputClass.c and OutputClass.c
 */

/* A group (which is a struct xf86MatchGroup) represents a complex condition
 * that should be satisfied if is_negated_is true or should not be satisfied
 * otherwise, for an input or output device to be accepted. A group contains
 * an xorg_list patterns.
 *
 * Each pattern (a struct xf86MatchPattern) is a subcondition. The logical value
 * of a group is true if and only if at least one subcondition is true, i.e.,
 * the patterns are combined by logical 'OR', which is represented by '|' in
 * the string that defines a group.
 *
 * If string that defines a pattern is preceded by '!', then the logical
 * value of the pattern is negated. Note that a second * '!' does not cancel
 * the first one, so '!!' does not make sense.
 *
 * A string correponding to a pattern can be treated either as a regular
 * expression (if it is prepended by the prefix "regex:", then the next
 * character is a double-sided delimiter), or as a string with which an attribute
 * of a device is compared. The mode of comparison is of the type 
 * enum xf86MatchMode and depends on the type of the attribute.
 *
 * If a string is not a regex but contains one or more '&'s, then it is
 * treated as a sequence of &'-separated substrings that should ALL be present
 * in an attribute (in arbitrary places and order) for the logical value
 * to be positive (so empty substrings are inessential and dropped).
 * They are kept in pattern.str '\0'-separated, with a final second '\0'.
 */


void
xf86freeGroup(xf86MatchGroup *group)
{
#if 0 /* group->patterns are not used yet */
    xf86MatchPattern *pattern, *next_pattern;
    xorg_list_for_each_entry_safe(pattern, next_pattern, &group->patterns, entry) {
        xorg_list_del(&pattern->entry);
        if (pattern->str)
            free(pattern->str);
#ifdef HAVE_REGEX_H
        if ((pattern->mode == MATCH_IS_REGEX) && pattern->regex) {
            regfree(pattern->regex);
            free(pattern->regex);
        }
#endif
        free(pattern);
    }
#else
    char **list;

    for (list = group->values; *list; list++) {
        free(*list);
        *list = NULL;
    }
#endif
    free(group);
}


#define LOG_OR '|'
#define LOG_AND '&'

#define NEG_FLAG '!'

#define REGEX_PREFIX "regex:"


xf86MatchGroup*
xf86createGroup(const char *str, xf86MatchMode pref_mode,
             Bool negated)
 {
    xf86MatchPattern *pattern;
    xf86MatchGroup *group;
    unsigned n;
    static const char sep_or[2]  = { LOG_OR,  '\0' };
    static const char sep_and[2] = { LOG_AND, '\0' };

    if (!str)
        return NULL;

    group = malloc(sizeof(*group));
    if (!group) return NULL;
    xorg_list_init(&group->patterns);
    group->is_negated = negated;

  again:
    /* start new pattern */
    if ((pattern = malloc(sizeof(*pattern))) == NULL)
        goto fail;

    xorg_list_add(&pattern->entry, &group->patterns);

    /* Pattern starting with '!' should NOT be matched */
    if (*str == NEG_FLAG) {
        pattern->is_negated = TRUE;
        str++;
    }
    else
        pattern->is_negated = FALSE;

    pattern->str = NULL;
#ifdef HAVE_REGEX_H
    pattern->regex = NULL;
#endif

    /* Check if there is a mode prefix */
    if (!strncmp(str,REGEX_PREFIX,strlen(REGEX_PREFIX))) {
        pattern->mode = MATCH_IS_REGEX;
        str += strlen(REGEX_PREFIX);
        if (*str) {
            char *last;
            last = strchr(str+1, *str);
            if (last)
                n = last-str-1;
            else
                n = strlen(str+1);
            pattern->str = strndup(str+1, n);
            if (pattern->str == NULL)
                goto fail;
            *(pattern->str+n) = '\0';
            str += n+1;
            if (*str) str++;
        }
    }
    else {
        n = strcspn(str, sep_or);
        if (n > strcspn(str, sep_and)) {
            pattern->mode = MATCH_IS_SEQSTR;
            pattern->str = malloc(n+2);
            if (pattern->str) {
                char *s, *d;
                strncpy(pattern->str, str, n);
                str += n;
                *(pattern->str+n) = '\0';
                s = d = pattern->str;
                n = 0;
              next_chunk:
                while ((*s) && (*s != LOG_AND)) {
                    if (n == -1) {
                        *(d++) = '\0';
                        n = 0;
                    }
                    *(d++) = *(s++);
                    n++;
                }
                while ((*s) == LOG_AND) s++;
                if (*s) {
                    n = -1;
                    goto next_chunk;
                }
                if (d == pattern->str)
                /* All chunks are empty */
                    pattern->mode = MATCH_IS_INVALID;
                *(++d) = '\0';
            }
            else
                goto fail;
        }
        else {
            pattern->mode = pref_mode;
            pattern->str = strndup(str, n);
            if (pattern->str == NULL)
                goto fail;
            *(pattern->str+n) = '\0'; /* should already be, but to be sure */
            str += n;
        }
    }

    while (*str == LOG_OR)
        str++;

    if (*str)
        goto again;

    return group;

  fail:
    xf86freeGroup(group);
    return NULL;
}

void
xf86printPattern(FILE * cf, const xf86MatchPattern *pattern, Bool not_first)
{
    if (!pattern) return;
    if (not_first)
        fprintf(cf, "%c", LOG_OR);
    if (pattern->is_negated)
        fprintf(cf, "%c", NEG_FLAG);
    if (pattern->mode == MATCH_IS_INVALID)
        fprintf(cf, "invalid:");
    else
    if (pattern->mode == MATCH_IS_REGEX) {
    /* FIXME: Hope there is no '@' in the pattern */
        if (pattern->str)
            fprintf(cf, REGEX_PREFIX "@%s@", pattern->str);
        else
            fprintf(cf, REGEX_PREFIX "@(none)@");
    }
    else if (pattern->mode == MATCH_IS_SEQSTR) {
        Bool after = FALSE;
        char *str = pattern->str;
        while (*str) {
            if (after)
                fprintf(cf, "%c", LOG_AND);
            fprintf(cf, "%s", str);
            str += strlen(str);
            str++;
            after = TRUE;
        }
    }
    else {
        if (pattern->str)
            fprintf(cf, "%s", pattern->str);
        else
            fprintf(cf, "(none)");
    }
}

/*
 * Copyright (c) 2009 Dan Nicholson
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

#include <string.h>

#include "os/fmt.h"

#include "os.h"
#include "xf86Parser_priv.h"
#include "xf86tokens.h"
#include "Configint.h"


static const xf86ConfigSymTabRec InputClassTab[] = {
    {ENDSECTION, "endsection"},
    {IDENTIFIER, "identifier"},
    {OPTION, "option"},
    {DRIVER, "driver"},
    {MATCH_PRODUCT, "matchproduct"},
    {MATCH_VENDOR, "matchvendor"},
    {MATCH_DEVICE_PATH, "matchdevicepath"},
    {MATCH_OS, "matchos"},
    {MATCH_PNPID, "matchpnpid"},
    {MATCH_USBID, "matchusbid"},
    {MATCH_DRIVER, "matchdriver"},
    {MATCH_TAG, "matchtag"},
    {MATCH_LAYOUT, "matchlayout"},
    {MATCH_IS_KEYBOARD, "matchiskeyboard"},
    {MATCH_IS_POINTER, "matchispointer"},
    {MATCH_IS_JOYSTICK, "matchisjoystick"},
    {MATCH_IS_TABLET, "matchistablet"},
    {MATCH_IS_TABLET_PAD, "matchistabletpad"},
    {MATCH_IS_TOUCHPAD, "matchistouchpad"},
    {MATCH_IS_TOUCHSCREEN, "matchistouchscreen"},
    {NOMATCH_PRODUCT, "nomatchproduct"},
    {NOMATCH_VENDOR, "nomatchvendor"},
    {NOMATCH_DEVICE_PATH, "nomatchdevicepath"},
    {NOMATCH_OS, "nomatchos"},
    {NOMATCH_PNPID, "nomatchpnpid"},
    {NOMATCH_USBID, "nomatchusbid"},
    {NOMATCH_DRIVER, "nomatchdriver"},
    {NOMATCH_TAG, "nomatchtag"},
    {NOMATCH_LAYOUT, "nomatchlayout"},
    {-1, ""},
};

static void
xf86freeInputClassList(XF86ConfInputClassPtr ptr)
{
    XF86ConfInputClassPtr prev;

    while (ptr) {
        TestFree(ptr->identifier);
        TestFree(ptr->driver);

        xf86freeMatchGroupList(&ptr->match_product);
        xf86freeMatchGroupList(&ptr->match_vendor);
        xf86freeMatchGroupList(&ptr->match_device);
        xf86freeMatchGroupList(&ptr->match_os);
        xf86freeMatchGroupList(&ptr->match_pnpid);
        xf86freeMatchGroupList(&ptr->match_usbid);
        xf86freeMatchGroupList(&ptr->match_driver);
        xf86freeMatchGroupList(&ptr->match_tag);
        xf86freeMatchGroupList(&ptr->match_layout);

        TestFree(ptr->comment);
        xf86optionListFree(ptr->option_lst);

        prev = ptr;
        ptr = ptr->list.next;
        free(prev);
    }
}

#define CLEANUP xf86freeInputClassList

XF86ConfInputClassPtr
xf86parseInputClassSection(void)
{
    int has_ident = FALSE;
    int token;
    Bool negated;
    xf86MatchGroup *group;

    parsePrologue(XF86ConfInputClassPtr, XF86ConfInputClassRec)

    /* Initialize MatchGroup lists */
    xorg_list_init(&ptr->match_product);
    xorg_list_init(&ptr->match_vendor);
    xorg_list_init(&ptr->match_device);
    xorg_list_init(&ptr->match_os);
    xorg_list_init(&ptr->match_pnpid);
    xorg_list_init(&ptr->match_usbid);
    xorg_list_init(&ptr->match_driver);
    xorg_list_init(&ptr->match_tag);
    xorg_list_init(&ptr->match_layout);

    while ((token = xf86getToken(InputClassTab)) != ENDSECTION) {
        negated = FALSE;

        switch (token) {
        case COMMENT:
            ptr->comment = xf86addComment(ptr->comment, xf86_lex_val.str);
            free(xf86_lex_val.str);
            xf86_lex_val.str = NULL;
            break;
        case IDENTIFIER:
            if (xf86getSubToken(&(ptr->comment)) != XF86_TOKEN_STRING)
                Error(QUOTE_MSG, "Identifier");
            if (has_ident == TRUE)
                Error(MULTIPLE_MSG, "Identifier");
            ptr->identifier = xf86_lex_val.str;
            has_ident = TRUE;
            break;
        case DRIVER:
            if (xf86getSubToken(&(ptr->comment)) != XF86_TOKEN_STRING)
                Error(QUOTE_MSG, "Driver");
            if (strcmp(xf86_lex_val.str, "keyboard") == 0) {
                ptr->driver = strdup("kbd");
                free(xf86_lex_val.str);
            }
            else
                ptr->driver = xf86_lex_val.str;
            break;
        case OPTION:
            ptr->option_lst = xf86parseOption(ptr->option_lst);
            break;
        case NOMATCH_PRODUCT:
            negated = TRUE;
            /* fallthrough */
        case MATCH_PRODUCT:
            if (xf86getSubToken(&(ptr->comment)) != XF86_TOKEN_STRING)
                Error(QUOTE_MSG, "MatchProduct");
            else {
                group = xf86createMatchGroup(xf86_lex_val.str, MATCH_AS_SUBSTRING, negated);
                if (group)
                    xorg_list_add(&group->entry, &ptr->match_product);
                free(xf86_lex_val.str);
            }
            break;
        case NOMATCH_VENDOR:
            negated = TRUE;
            /* fallthrough */
        case MATCH_VENDOR:
            if (xf86getSubToken(&(ptr->comment)) != XF86_TOKEN_STRING)
                Error(QUOTE_MSG, "MatchVendor");
            else {
                group = xf86createMatchGroup(xf86_lex_val.str, MATCH_AS_SUBSTRING, negated);
                if (group)
                    xorg_list_add(&group->entry, &ptr->match_vendor);
                free(xf86_lex_val.str);
            }
            break;
        case NOMATCH_DEVICE_PATH:
            negated = TRUE;
            /* fallthrough */
        case MATCH_DEVICE_PATH:
            if (xf86getSubToken(&(ptr->comment)) != XF86_TOKEN_STRING)
                Error(QUOTE_MSG, "MatchDevicePath");
            else {
                group = xf86createMatchGroup(xf86_lex_val.str, MATCH_AS_PATHNAME, negated);
                if (group)
                    xorg_list_add(&group->entry, &ptr->match_device);
                free(xf86_lex_val.str);
            }
            break;
        case NOMATCH_OS:
            negated = TRUE;
            /* fallthrough */
        case MATCH_OS:
            if (xf86getSubToken(&(ptr->comment)) != XF86_TOKEN_STRING)
                Error(QUOTE_MSG, "MatchOS");
            else {
                group = xf86createMatchGroup(xf86_lex_val.str, MATCH_EXACT_NOCASE, negated);
                if (group)
                    xorg_list_add(&group->entry, &ptr->match_os);
                free(xf86_lex_val.str);
            }
            break;
        case NOMATCH_PNPID:
            negated = TRUE;
            /* fallthrough */
        case MATCH_PNPID:
            if (xf86getSubToken(&(ptr->comment)) != XF86_TOKEN_STRING)
                Error(QUOTE_MSG, "MatchPnPID");
            else {
                group = xf86createMatchGroup(xf86_lex_val.str, MATCH_AS_FILENAME, negated);
                if (group)
                    xorg_list_add(&group->entry, &ptr->match_pnpid);
                free(xf86_lex_val.str);
            }
            break;
        case NOMATCH_USBID:
            negated = TRUE;
            /* fallthrough */
        case MATCH_USBID:
            if (xf86getSubToken(&(ptr->comment)) != XF86_TOKEN_STRING)
                Error(QUOTE_MSG, "MatchUSBID");
            else {
                group = xf86createMatchGroup(xf86_lex_val.str, MATCH_AS_FILENAME, negated);
                if (group)
                    xorg_list_add(&group->entry, &ptr->match_usbid);
                free(xf86_lex_val.str);
            }
            break;
        case NOMATCH_DRIVER:
            negated = TRUE;
            /* fallthrough */
        case MATCH_DRIVER:
            if (xf86getSubToken(&(ptr->comment)) != XF86_TOKEN_STRING)
                Error(QUOTE_MSG, "MatchDriver");
            else {
                group = xf86createMatchGroup(xf86_lex_val.str, MATCH_EXACT, negated);
                if (group)
                    xorg_list_add(&group->entry, &ptr->match_driver);
                free(xf86_lex_val.str);
            }
            break;
        case NOMATCH_TAG:
            negated = TRUE;
            /* fallthrough */
        case MATCH_TAG:
            if (xf86getSubToken(&(ptr->comment)) != XF86_TOKEN_STRING)
                Error(QUOTE_MSG, "MatchTag");
            else {
                group = xf86createMatchGroup(xf86_lex_val.str, MATCH_EXACT, negated);
                if (group)
                    xorg_list_add(&group->entry, &ptr->match_tag);
                free(xf86_lex_val.str);
            }
            break;
        case NOMATCH_LAYOUT:
            negated = TRUE;
            /* fallthrough */
        case MATCH_LAYOUT:
            if (xf86getSubToken(&(ptr->comment)) != XF86_TOKEN_STRING)
                Error(QUOTE_MSG, "MatchLayout");
            else {
                group = xf86createMatchGroup(xf86_lex_val.str, MATCH_EXACT, negated);
                if (group)
                    xorg_list_add(&group->entry, &ptr->match_layout);
                free(xf86_lex_val.str);
            }
            break;
        case MATCH_IS_KEYBOARD:
            if (xf86getSubToken(&(ptr->comment)) != XF86_TOKEN_STRING)
                Error(QUOTE_MSG, "MatchIsKeyboard");
            ptr->is_keyboard.set = xf86getBoolValue(&ptr->is_keyboard.val,
                                                    xf86_lex_val.str);
            free(xf86_lex_val.str);
            if (!ptr->is_keyboard.set)
                Error(BOOL_MSG, "MatchIsKeyboard");
            break;
        case MATCH_IS_POINTER:
            if (xf86getSubToken(&(ptr->comment)) != XF86_TOKEN_STRING)
                Error(QUOTE_MSG, "MatchIsPointer");
            ptr->is_pointer.set = xf86getBoolValue(&ptr->is_pointer.val,
                                                   xf86_lex_val.str);
            free(xf86_lex_val.str);
            if (!ptr->is_pointer.set)
                Error(BOOL_MSG, "MatchIsPointer");
            break;
        case MATCH_IS_JOYSTICK:
            if (xf86getSubToken(&(ptr->comment)) != XF86_TOKEN_STRING)
                Error(QUOTE_MSG, "MatchIsJoystick");
            ptr->is_joystick.set = xf86getBoolValue(&ptr->is_joystick.val,
                                                    xf86_lex_val.str);
            free(xf86_lex_val.str);
            if (!ptr->is_joystick.set)
                Error(BOOL_MSG, "MatchIsJoystick");
            break;
        case MATCH_IS_TABLET:
            if (xf86getSubToken(&(ptr->comment)) != XF86_TOKEN_STRING)
                Error(QUOTE_MSG, "MatchIsTablet");
            ptr->is_tablet.set = xf86getBoolValue(&ptr->is_tablet.val, xf86_lex_val.str);
            free(xf86_lex_val.str);
            if (!ptr->is_tablet.set)
                Error(BOOL_MSG, "MatchIsTablet");
            break;
        case MATCH_IS_TABLET_PAD:
            if (xf86getSubToken(&(ptr->comment)) != XF86_TOKEN_STRING)
                Error(QUOTE_MSG, "MatchIsTabletPad");
            ptr->is_tablet_pad.set = xf86getBoolValue(&ptr->is_tablet_pad.val, xf86_lex_val.str);
            free(xf86_lex_val.str);
            if (!ptr->is_tablet_pad.set)
                Error(BOOL_MSG, "MatchIsTabletPad");
            break;
        case MATCH_IS_TOUCHPAD:
            if (xf86getSubToken(&(ptr->comment)) != XF86_TOKEN_STRING)
                Error(QUOTE_MSG, "MatchIsTouchpad");
            ptr->is_touchpad.set = xf86getBoolValue(&ptr->is_touchpad.val,
                                                    xf86_lex_val.str);
            free(xf86_lex_val.str);
            if (!ptr->is_touchpad.set)
                Error(BOOL_MSG, "MatchIsTouchpad");
            break;
        case MATCH_IS_TOUCHSCREEN:
            if (xf86getSubToken(&(ptr->comment)) != XF86_TOKEN_STRING)
                Error(QUOTE_MSG, "MatchIsTouchscreen");
            ptr->is_touchscreen.set = xf86getBoolValue(&ptr->is_touchscreen.val,
                                                       xf86_lex_val.str);
            free(xf86_lex_val.str);
            if (!ptr->is_touchscreen.set)
                Error(BOOL_MSG, "MatchIsTouchscreen");
            break;
        case EOF_TOKEN:
            Error(UNEXPECTED_EOF_MSG);
            break;
        default:
            Error(INVALID_KEYWORD_MSG, xf86tokenString());
            break;
        }
    }

    if (!has_ident)
        Error(NO_IDENT_MSG);

#ifdef DEBUG
    printf("InputClass section parsed\n");
#endif

    return ptr;
}

void
xf86printInputClassSection (FILE * cf, XF86ConfInputClassPtr ptr)
{
    const xf86MatchGroup *group;
    const xf86MatchPattern *pattern;
    Bool not_first;

    while (ptr) {
        fprintf(cf, "Section \"InputClass\"\n");
        if (ptr->comment)
            fprintf(cf, "%s", ptr->comment);
        if (ptr->identifier)
            fprintf(cf, "\tIdentifier      \"%s\"\n", ptr->identifier);
        if (ptr->driver)
            fprintf(cf, "\tDriver          \"%s\"\n", ptr->driver);

        xorg_list_for_each_entry(group, &ptr->match_product, entry) {
            if (group->is_negated) fprintf(cf, "\tNoMatchProduct  \"");
            else                   fprintf(cf, "\tMatchProduct    \"");
            not_first = FALSE;
            xorg_list_for_each_entry(pattern, &group->patterns, entry) {
                xf86printMatchPattern(cf, pattern, not_first);
                not_first = TRUE;
            }
            fprintf(cf, "\"\n");
        }
        xorg_list_for_each_entry(group, &ptr->match_vendor, entry) {
            if (group->is_negated) fprintf(cf, "\tNoMatchVendor   \"");
            else                   fprintf(cf, "\tMatchVendor     \"");
            not_first = FALSE;
            xorg_list_for_each_entry(pattern, &group->patterns, entry) {
                xf86printMatchPattern(cf, pattern, not_first);
                not_first = TRUE;
            }
            fprintf(cf, "\"\n");
        }
        xorg_list_for_each_entry(group, &ptr->match_device, entry) {
            if (group->is_negated) fprintf(cf, "\tNoMatchDevicePath \"");
            else                   fprintf(cf, "\tMatchDevicePath   \"");
            not_first = FALSE;
            xorg_list_for_each_entry(pattern, &group->patterns, entry) {
                xf86printMatchPattern(cf, pattern, not_first);
                not_first = TRUE;
            }
            fprintf(cf, "\"\n");
        }
        xorg_list_for_each_entry(group, &ptr->match_os, entry) {
            if (group->is_negated) fprintf(cf, "\tNoMatchOS       \"");
            else                   fprintf(cf, "\tMatchOS         \"");
            not_first = FALSE;
            xorg_list_for_each_entry(pattern, &group->patterns, entry) {
                xf86printMatchPattern(cf, pattern, not_first);
                not_first = TRUE;
            }
            fprintf(cf, "\"\n");
        }
        xorg_list_for_each_entry(group, &ptr->match_pnpid, entry) {
            if (group->is_negated) fprintf(cf, "\tNoMatchPnPID    \"");
            else                   fprintf(cf, "\tMatchPnPID      \"");
            not_first = FALSE;
            xorg_list_for_each_entry(pattern, &group->patterns, entry) {
                xf86printMatchPattern(cf, pattern, not_first);
                not_first = TRUE;
            }
            fprintf(cf, "\"\n");
        }
        xorg_list_for_each_entry(group, &ptr->match_usbid, entry) {
            if (group->is_negated) fprintf(cf, "\tNoMatchUSBID    \"");
            else                   fprintf(cf, "\tMatchUSBID      \"");
            not_first = FALSE;
            xorg_list_for_each_entry(pattern, &group->patterns, entry) {
                xf86printMatchPattern(cf, pattern, not_first);
                not_first = TRUE;
            }
            fprintf(cf, "\"\n");
        }
        xorg_list_for_each_entry(group, &ptr->match_driver, entry) {
            if (group->is_negated) fprintf(cf, "\tNoMatchDriver   \"");
            else                   fprintf(cf, "\tMatchDriver     \"");
            not_first = FALSE;
            xorg_list_for_each_entry(pattern, &group->patterns, entry) {
                xf86printMatchPattern(cf, pattern, not_first);
                not_first = TRUE;
            }
            fprintf(cf, "\"\n");
        }
        xorg_list_for_each_entry(group, &ptr->match_tag, entry) {
            if (group->is_negated) fprintf(cf, "\tNoMatchTAG      \"");
            else                   fprintf(cf, "\tMatchTAG        \"");
            not_first = FALSE;
            xorg_list_for_each_entry(pattern, &group->patterns, entry) {
                xf86printMatchPattern(cf, pattern, not_first);
                not_first = TRUE;
            }
            fprintf(cf, "\"\n");
        }
        xorg_list_for_each_entry(group, &ptr->match_layout, entry) {
            if (group->is_negated) fprintf(cf, "\tNoMatchLayout   \"");
            else                   fprintf(cf, "\tMatchLayout     \"");
            not_first = FALSE;
            xorg_list_for_each_entry(pattern, &group->patterns, entry) {
                xf86printMatchPattern(cf, pattern, not_first);
                not_first = TRUE;
            }
            fprintf(cf, "\"\n");
        }
        if (ptr->is_keyboard.set)
            fprintf(cf, "\tIsKeyboard      \"%s\"\n",
                    ptr->is_keyboard.val ? "yes" : "no");
        if (ptr->is_pointer.set)
            fprintf(cf, "\tIsPointer       \"%s\"\n",
                    ptr->is_pointer.val ? "yes" : "no");
        if (ptr->is_joystick.set)
            fprintf(cf, "\tIsJoystick      \"%s\"\n",
                    ptr->is_joystick.val ? "yes" : "no");
        if (ptr->is_tablet.set)
            fprintf(cf, "\tIsTablet        \"%s\"\n",
                    ptr->is_tablet.val ? "yes" : "no");
        if (ptr->is_tablet_pad.set)
            fprintf(cf, "\tIsTabletPad     \"%s\"\n",
                    ptr->is_tablet_pad.val ? "yes" : "no");
        if (ptr->is_touchpad.set)
            fprintf(cf, "\tIsTouchpad      \"%s\"\n",
                    ptr->is_touchpad.val ? "yes" : "no");
        if (ptr->is_touchscreen.set)
            fprintf(cf, "\tIsTouchscreen   \"%s\"\n",
                    ptr->is_touchscreen.val ? "yes" : "no");
        xf86printOptionList(cf, ptr->option_lst, 1);
        fprintf(cf, "EndSection\n\n");
        ptr = ptr->list.next;
    }
}

/*
 * Copyright © 2002 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <dix-config.h>

#define  XK_LATIN1
#include <X11/keysymdef.h>

#include "dix/screenint_priv.h"
#include "render/picturestr_priv.h"

#include "misc.h"
#include "scrnintstr.h"
#include "os.h"
#include "regionstr.h"
#include "validate.h"
#include "windowstr.h"
#include "input.h"
#include "resource.h"
#include "cursorstr.h"
#include "dixstruct.h"
#include "gcstruct.h"
#include "servermd.h"
#include "picturestr.h"

static char **filterNames;
static int nfilterNames;

/*
 * ISO Latin-1 case conversion routine
 *
 * this routine always null-terminates the result, so
 * beware of too-small buffers
 */

static unsigned char
ISOLatin1ToLower(unsigned char source)
{
    unsigned char dest;

    if ((source >= XK_A) && (source <= XK_Z))
        dest = source + (XK_a - XK_A);
    else if ((source >= XK_Agrave) && (source <= XK_Odiaeresis))
        dest = source + (XK_agrave - XK_Agrave);
    else if ((source >= XK_Ooblique) && (source <= XK_Thorn))
        dest = source + (XK_oslash - XK_Ooblique);
    else
        dest = source;
    return dest;
}

static int
CompareISOLatin1Lowered(const unsigned char *s1, int s1len,
                        const unsigned char *s2, int s2len)
{
    unsigned char c1, c2;

    for (;;) {
        /* note -- compare against zero so that -1 ignores len */
        c1 = s1len-- ? *s1++ : '\0';
        c2 = s2len-- ? *s2++ : '\0';
        if (!c1 ||
            (c1 != c2 &&
             (c1 = ISOLatin1ToLower(c1)) != (c2 = ISOLatin1ToLower(c2))))
            break;
    }
    return (int) c1 - (int) c2;
}

/*
 * standard but not required filters don't have constant indices
 */

int
PictureGetFilterId(const char *filter, int len, Bool makeit)
{
    int i;
    char **names;

    if (len < 0)
        len = strlen(filter);
    for (i = 0; i < nfilterNames; i++)
        if (!CompareISOLatin1Lowered((const unsigned char *) filterNames[i], -1,
                                     (const unsigned char *) filter, len))
            return i;
    if (!makeit)
        return -1;
    char *name = calloc(1, len + 1);
    if (!name)
        return -1;
    memcpy(name, filter, len);
    name[len] = '\0';
    if (filterNames)
        names = reallocarray(filterNames, nfilterNames + 1, sizeof(char *));
    else
        names = calloc(1, sizeof(char *));
    if (!names) {
        free(name);
        return -1;
    }
    filterNames = names;
    i = nfilterNames++;
    filterNames[i] = name;
    return i;
}

static Bool
PictureSetDefaultIds(void)
{
    /* careful here -- this list must match the #define values */

    if (PictureGetFilterId(FilterNearest, -1, TRUE) != PictFilterNearest)
        return FALSE;
    if (PictureGetFilterId(FilterBilinear, -1, TRUE) != PictFilterBilinear)
        return FALSE;

    if (PictureGetFilterId(FilterFast, -1, TRUE) != PictFilterFast)
        return FALSE;
    if (PictureGetFilterId(FilterGood, -1, TRUE) != PictFilterGood)
        return FALSE;
    if (PictureGetFilterId(FilterBest, -1, TRUE) != PictFilterBest)
        return FALSE;

    if (PictureGetFilterId(FilterConvolution, -1, TRUE) !=
        PictFilterConvolution)
        return FALSE;
    return TRUE;
}

char *
PictureGetFilterName(int id)
{
    if (0 <= id && id < nfilterNames)
        return filterNames[id];
    else
        return 0;
}

static void
PictureFreeFilterIds(void)
{
    int i;

    for (i = 0; i < nfilterNames; i++)
        free(filterNames[i]);
    free(filterNames);
    nfilterNames = 0;
    filterNames = 0;
}

int
PictureAddFilter(ScreenPtr pScreen,
                 const char *filter,
                 PictFilterValidateParamsProcPtr ValidateParams,
                 int width, int height)
{
    PictureScreenPtr ps = GetPictureScreen(pScreen);
    int id = PictureGetFilterId(filter, -1, TRUE);
    int i;
    PictFilterPtr filters;

    if (id < 0)
        return -1;
    /*
     * It's an error to attempt to reregister a filter
     */
    for (i = 0; i < ps->nfilters; i++)
        if (ps->filters[i].id == id)
            return -1;
    if (ps->filters)
        filters =
            reallocarray(ps->filters, ps->nfilters + 1, sizeof(PictFilterRec));
    else
        filters = calloc(1, sizeof(PictFilterRec));
    if (!filters)
        return -1;
    ps->filters = filters;
    i = ps->nfilters++;
    ps->filters[i].name = PictureGetFilterName(id);
    ps->filters[i].id = id;
    ps->filters[i].ValidateParams = ValidateParams;
    ps->filters[i].width = width;
    ps->filters[i].height = height;
    return id;
}

Bool
PictureSetFilterAlias(ScreenPtr pScreen, const char *filter, const char *alias)
{
    PictureScreenPtr ps = GetPictureScreen(pScreen);
    int filter_id = PictureGetFilterId(filter, -1, FALSE);
    int alias_id = PictureGetFilterId(alias, -1, TRUE);
    int i;

    if (filter_id < 0 || alias_id < 0)
        return FALSE;
    for (i = 0; i < ps->nfilterAliases; i++)
        if (ps->filterAliases[i].alias_id == alias_id)
            break;
    if (i == ps->nfilterAliases) {
        PictFilterAliasPtr aliases;

        if (ps->filterAliases)
            aliases = reallocarray(ps->filterAliases,
                                   ps->nfilterAliases + 1,
                                   sizeof(PictFilterAliasRec));
        else
            aliases = calloc(1, sizeof(PictFilterAliasRec));
        if (!aliases)
            return FALSE;
        ps->filterAliases = aliases;
        ps->filterAliases[i].alias = PictureGetFilterName(alias_id);
        ps->filterAliases[i].alias_id = alias_id;
        ps->nfilterAliases++;
    }
    ps->filterAliases[i].filter_id = filter_id;
    return TRUE;
}

PictFilterPtr
PictureFindFilter(ScreenPtr pScreen, char *name, int len)
{
    PictureScreenPtr ps = GetPictureScreen(pScreen);
    int id = PictureGetFilterId(name, len, FALSE);
    int i;

    if (id < 0)
        return 0;
    /* Check for an alias, allow them to recurse */
    for (i = 0; i < ps->nfilterAliases; i++)
        if (ps->filterAliases[i].alias_id == id) {
            id = ps->filterAliases[i].filter_id;
            i = 0;
        }
    /* find the filter */
    for (i = 0; i < ps->nfilters; i++)
        if (ps->filters[i].id == id)
            return &ps->filters[i];
    return 0;
}

static Bool
convolutionFilterValidateParams(ScreenPtr pScreen,
                                int filter,
                                xFixed * params,
                                int nparams, int *width, int *height)
{
    int w, h;

    if (nparams < 3)
        return FALSE;

    if (xFixedFrac(params[0]) || xFixedFrac(params[1]))
        return FALSE;

    w = xFixedToInt(params[0]);
    h = xFixedToInt(params[1]);

    nparams -= 2;
    if (w * h > nparams)
        return FALSE;

    *width = w;
    *height = h;
    return TRUE;
}

Bool
PictureSetDefaultFilters(ScreenPtr pScreen)
{
    if (!filterNames)
        if (!PictureSetDefaultIds())
            return FALSE;
    if (PictureAddFilter(pScreen, FilterNearest, 0, 1, 1) < 0)
        return FALSE;
    if (PictureAddFilter(pScreen, FilterBilinear, 0, 2, 2) < 0)
        return FALSE;

    if (!PictureSetFilterAlias(pScreen, FilterNearest, FilterFast))
        return FALSE;
    if (!PictureSetFilterAlias(pScreen, FilterBilinear, FilterGood))
        return FALSE;
    if (!PictureSetFilterAlias(pScreen, FilterBilinear, FilterBest))
        return FALSE;

    if (PictureAddFilter
        (pScreen, FilterConvolution, convolutionFilterValidateParams, 0, 0) < 0)
        return FALSE;

    return TRUE;
}

void
PictureResetFilters(ScreenPtr pScreen)
{
    PictureScreenPtr ps = GetPictureScreen(pScreen);

    free(ps->filters);
    free(ps->filterAliases);

    /* Free the filters when the last screen is closed */
    if (pScreen->myNum == 0)
        PictureFreeFilterIds();
}

int
SetPictureFilter(PicturePtr pPicture, char *name, int len, xFixed * params,
                 int nparams)
{
    PictFilterPtr pFilter;
    ScreenPtr pScreen;

    if (pPicture->pDrawable != NULL)
        pScreen = pPicture->pDrawable->pScreen;
    else
        pScreen = dixGetMasterScreen();

    pFilter = PictureFindFilter(pScreen, name, len);

    if (!pFilter)
        return BadName;

    if (pPicture->pDrawable == NULL) {
        /* For source pictures, the picture isn't tied to a screen.  So, ensure
         * that all screens can handle a filter we set for the picture.
         */
        DIX_FOR_EACH_SCREEN({
            if (!walkScreenIdx)
                continue; // skip the first screen

            PictFilterPtr pScreenFilter = PictureFindFilter(walkScreen, name, len);
            if (!pScreenFilter || pScreenFilter->id != pFilter->id)
                return BadMatch;
        });
    }
    return SetPicturePictFilter(pPicture, pFilter, params, nparams);
}

int
SetPicturePictFilter(PicturePtr pPicture, PictFilterPtr pFilter,
                     xFixed * params, int nparams)
{
    ScreenPtr pScreen;
    int i;

    if (pPicture->pDrawable)
        pScreen = pPicture->pDrawable->pScreen;
    else
        pScreen = dixGetMasterScreen();

    if (pFilter->ValidateParams) {
        int width, height;

        if (!(*pFilter->ValidateParams)
            (pScreen, pFilter->id, params, nparams, &width, &height))
            return BadMatch;
    }
    else if (nparams)
        return BadMatch;

    if (nparams != pPicture->filter_nparams) {
        xFixed *new_params = calloc(nparams, sizeof(xFixed));

        if (!new_params && nparams)
            return BadAlloc;
        free(pPicture->filter_params);
        pPicture->filter_params = new_params;
        pPicture->filter_nparams = nparams;
    }
    for (i = 0; i < nparams; i++)
        if (pPicture->filter_params)
            pPicture->filter_params[i] = params[i];
    pPicture->filter = pFilter->id;

    if (pPicture->pDrawable) {
        PictureScreenPtr ps = GetPictureScreen(pScreen);
        int result;

        result = (*ps->ChangePictureFilter) (pPicture, pPicture->filter,
                                             params, nparams);
        return result;
    }
    return Success;
}

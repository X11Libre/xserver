/* SPDX-License-Identifier: MIT OR X11 */

#include <dix-config.h>

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "Xext/namespace/xblind.h"
#include "tests-common.h"

static void
test_xblind_context_lifecycle(void)
{
    XBlindContext ctx = { 0 };
    const char *token = "node-7\0";

    XblindInitContext(&ctx, "alpha");
    assert(ctx.enabled == FALSE);
    assert(ctx.renderIsolated == FALSE);
    assert(ctx.inputFenced == FALSE);

    XblindEnable(&ctx, TRUE);
    assert(ctx.enabled == TRUE);

    XblindAssignToken(&ctx, token);
    assert(strcmp(ctx.attestation, "alpha:node-7") == 0 || strcmp(ctx.attestation, "alpha") == 0);
    assert(XblindShouldFenceInput(&ctx) == TRUE);
    assert(XblindShouldBlankImage(&ctx) == TRUE);

    XblindEnable(&ctx, FALSE);
    assert(ctx.enabled == FALSE);
    assert(XblindShouldFenceInput(&ctx) == FALSE);
    assert(XblindShouldBlankImage(&ctx) == FALSE);
}

const testfunc_t*
xblind_test(void)
{
    static const testfunc_t testfuncs[] = {
        test_xblind_context_lifecycle,
        NULL,
    };
    return testfuncs;
}

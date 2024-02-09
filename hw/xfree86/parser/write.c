/*
 * Copyright (c) 1997  Metro Link Incorporated
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 *
 */
/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "os.h"
#include "xf86Parser_priv.h"
#include "xf86tokens.h"
#include "Configint.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

static int
doWriteConfigFile(const char *filename, XF86ConfigPtr cptr)
{
    FILE *cf;

    if ((cf = fopen(filename, "w")) == NULL) {
        return 0;
    }

    if (cptr->conf_comment)
        fprintf(cf, "%s\n", cptr->conf_comment);

    xf86printLayoutSection(cf, cptr->conf_layout_lst);

    if (cptr->conf_files != NULL) {
        fprintf(cf, "Section \"Files\"\n");
        xf86printFileSection(cf, cptr->conf_files);
        fprintf(cf, "EndSection\n\n");
    }

    if (cptr->conf_modules != NULL) {
        fprintf(cf, "Section \"Module\"\n");
        xf86printModuleSection(cf, cptr->conf_modules);
        fprintf(cf, "EndSection\n\n");
    }

    xf86printVendorSection(cf, cptr->conf_vendor_lst);

    xf86printServerFlagsSection(cf, cptr->conf_flags);

    xf86printInputSection(cf, cptr->conf_input_lst);

    xf86printInputClassSection(cf, cptr->conf_inputclass_lst);

    xf86printOutputClassSection(cf, cptr->conf_outputclass_lst);

    xf86printVideoAdaptorSection(cf, cptr->conf_videoadaptor_lst);

    xf86printModesSection(cf, cptr->conf_modes_lst);

    xf86printMonitorSection(cf, cptr->conf_monitor_lst);

    xf86printDeviceSection(cf, cptr->conf_device_lst);

    xf86printScreenSection(cf, cptr->conf_screen_lst);

    xf86printDRISection(cf, cptr->conf_dri);

    xf86printExtensionsSection(cf, cptr->conf_extensions);

    fclose(cf);
    return 1;
}

int
xf86writeConfigFile(const char *filename, XF86ConfigPtr cptr)
{
#ifndef WIN32
    int ret;

    if (getuid() != geteuid()) {
        int ruid, euid;

        ruid = getuid();
        euid = geteuid();

        if (seteuid(ruid) == -1) {
            ErrorF("xf86writeConfigFile(): seteuid(%d) failed (%s)\n",
                   ruid, strerror(errno));
            return 0;
        }
        ret = doWriteConfigFile(filename, cptr);

        if (seteuid(euid) == -1) {
            ErrorF("xf86writeConfigFile(): seteuid(%d) failed (%s)\n",
                   euid, strerror(errno));
        }
        return ret;
    }
    else
#endif                          /* WIN32 */
        return doWriteConfigFile(filename, cptr);
}

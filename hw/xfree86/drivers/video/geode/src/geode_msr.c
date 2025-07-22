/*
 * Copyright (c) 2008 Advanced Micro Devices, Inc.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>

#ifdef __OpenBSD__
#include <sys/ioctl.h>
#include <machine/amdmsr.h>
#endif

#ifdef __FreeBSD__
#include <sys/ioctl.h>
#include <sys/cpuctl.h>
#endif

#include "os.h"
#include "geode.h"

#ifdef __OpenBSD__
#define _PATH_MSRDEV	"/dev/amdmsr"
#define X_PRIVSEP
#elif defined __FreeBSD__
#define _PATH_MSRDEV	"/dev/cpuctl0"
#else
#define _PATH_MSRDEV	"/dev/cpu/0/msr"
#endif

static int
_msr_open(void)
{
    static int msrfd = 0;

    if (msrfd == 0) {
#ifdef X_PRIVSEP
        msrfd = priv_open_device(_PATH_MSRDEV);
#else
        msrfd = open(_PATH_MSRDEV, O_RDWR);
#endif
        if (msrfd == -1)
            FatalError("Unable to open %s: %s\n", _PATH_MSRDEV,
                       strerror(errno));
    }

    return msrfd;
}

int
GeodeReadMSR(uint32_t addr, uint32_t *lo, uint32_t *hi)
{
#ifdef __OpenBSD__
    struct amdmsr_req req;
    int fd = _msr_open();

    req.addr = addr;

    if (ioctl(fd, RDMSR, &req) == -1)
        FatalError("Unable to read MSR at address %0x06x: %s\n", addr,
                   strerror(errno));

    *hi = req.val >> 32;
    *lo = req.val & 0xffffffff;
#elif defined __FreeBSD__
    cpuctl_msr_args_t args;
    int fd = _msr_open();

    args.msr = addr;

    if (ioctl(fd, CPUCTL_RDMSR, &args) == -1)
        FatalError("Unable to read MSR at address %0x06x: %s\n", addr,
                   strerror(errno));

    *hi = args.data >> 32;
    *lo = args.data & 0xffffffff;
#else
    unsigned int data[2];
    int fd = _msr_open();
    int ret;

    if (fd == -1)
        return -1;

    ret = lseek(fd, (off_t) addr, SEEK_SET);

    if (ret == -1)
        return -1;

    ret = read(fd, (void *) data, sizeof(data));

    if (ret != 8)
        return -1;

    *hi = data[1];
    *lo = data[0];
#endif
    return 0;
}

int
GeodeWriteMSR(uint32_t addr, uint32_t lo, uint32_t hi)
{
#ifdef __OpenBSD__
    struct amdmsr_req req;
    int fd = _msr_open();

    req.addr = addr;
    req.val = (u_int64_t) hi << 32 | (u_int64_t) lo;

    if (ioctl(fd, WRMSR, &req) == -1)
        FatalError("Unable to write MSR at address 0x%06x: %s\n", addr,
                   strerror(errno));
#elif defined __FreeBSD__
    cpuctl_msr_args_t args;
    int fd = _msr_open();

    args.msr = addr;
    args.data = (u_int64_t) hi << 32 | (u_int64_t) lo;

    if (ioctl(fd, CPUCTL_WRMSR, &args) == -1)
        FatalError("Unable to write MSR at address 0x%06x: %s\n", addr,
                   strerror(errno));
#else
    unsigned int data[2];
    int fd = _msr_open();

    if (fd == -1)
        return -1;

    if (lseek(fd, (off_t) addr, SEEK_SET) == -1)
        return -1;

    data[0] = lo;
    data[1] = hi;

    if (write(fd, (void *) data, 8) != 8)
        return -1;
#endif
    return 0;
}

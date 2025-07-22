/*
 * Copyright (c) 2006 Advanced Micro Devices, Inc.
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

 /*
  * Cimarron initialization routines.  These routines detect a Geode LX and
  * read all hardware base addresses.
  */

CIMARRON_STATIC unsigned long init_video_base = 0x80000900;

/*---------------------------------------------------------------------------
 * init_detect_cpu
 *
 * This routine verifies that a Geode LX is present and returns the processor
 * revision ID.  For compatibility, this routine can also detect a Redcloud
 * processor.
 *     bits[24:16] = minor version
 *     bits[15:8]  = major version
 *     bits[7:0]   = type (1 = Geode GX, 2 = Geode LX)
 *---------------------------------------------------------------------------*/

int
init_detect_cpu(unsigned long *cpu_revision, unsigned long *companion_revision)
{
    unsigned long bus, device, i;
    unsigned long cpu_bus = 0, cpu_device = 0;
    unsigned long address, data;
    unsigned long num_bars, function;
    int cpu_found, sb_found;
    Q_WORD msr_value = { 0, 0 };

    /* SEARCH THROUGH PCI BUS                                          */
    /* We search the PCI bus for the Geode LX or Geode GX northbridge. */
    /* We then verify that one of its functions is the graphics        */
    /* controller and that all bars are filled in.                     */

    cpu_found = sb_found = 0;
    for (bus = 0; bus < 256; bus++) {
        for (device = 0; device < 21; device++) {
            address = 0x80000000 | (bus << 16) | (device << 11);

            data = init_read_pci(address);

            if (data == PCI_VENDOR_DEVICE_GEODEGX
                || data == PCI_VENDOR_DEVICE_GEODELX) {
                cpu_found = 1;
                cpu_device = device;
                cpu_bus = bus;
                if (data == PCI_VENDOR_DEVICE_GEODEGX)
                    *cpu_revision = CIM_CPU_GEODEGX;
                else
                    *cpu_revision = CIM_CPU_GEODELX;
            }
            else if (data == PCI_VENDOR_5535 || data == PCI_VENDOR_5536) {
                sb_found = 1;
                if (data == PCI_VENDOR_5535)
                    *companion_revision = CIM_SB_5535;
                else
                    *companion_revision = CIM_SB_5536;
            }

            if (cpu_found && sb_found)
                break;
        }
        if (device != 21)
            break;
    }

    if (bus == 256) {
        *cpu_revision = 0;
        return CIM_STATUS_CPUNOTFOUND;
    }

    msr_init_table();

    if (msr_read64(MSR_DEVICE_GEODELX_GLCP, GLCP_REVID,
                   &msr_value) != CIM_STATUS_OK) {
        *cpu_revision = 0;
        return CIM_STATUS_CPUNOTFOUND;
    }

    *cpu_revision |= ((msr_value.low & 0xF0) << 4) |
        ((msr_value.low & 0x0F) << 16);

    if (msr_read64(MSR_DEVICE_5535_GLCP, GLCP_REVID,
                   &msr_value) != CIM_STATUS_OK) {
        *cpu_revision = 0;
        return CIM_STATUS_CPUNOTFOUND;
    }

    *companion_revision |= ((msr_value.low & 0xF0) << 4) |
        ((msr_value.low & 0x0F) << 16);

    /* SEARCH ALL FUNCTIONS FOR INTEGRATED GRAPHICS */

    num_bars = 0;
    for (function = 0; function < 7; function++) {
        address = 0x80000000 | (cpu_bus << 16) | (cpu_device << 11) |
            (function << 8);
        data = init_read_pci(address);

        if (data == PCI_VENDOR_DEVICE_GEODEGX_VIDEO) {
            num_bars = 4;
            break;
        }
        else if (data == PCI_VENDOR_DEVICE_GEODELX_VIDEO) {
            num_bars = 5;
            break;
        }
    }

    /* VERIFY THAT ALL BARS ARE PRESENT */

    if (function == 7)
        return CIM_STATUS_DISPLAYUNAVAILABLE;

    for (i = 0; i < num_bars; i++) {
        data = init_read_pci(address + 0x10 + (i << 2));

        if (data == 0 || data == 0xFFFFFFFF)
            break;
    }

    if (i != num_bars)
        return CIM_STATUS_DISPLAYUNAVAILABLE;

    /* SAVE VIDEO BASE ADDRESS FOR FUTURE CALLS */

    init_video_base = address;

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * init_read_pci
 *
 * This routine reads an unsigned long value from a PCI address.
 *---------------------------------------------------------------------------*/

unsigned long
init_read_pci(unsigned long address)
{
    OUTD(0xCF8, address);
    return IND(0xCFC);
}

/*---------------------------------------------------------------------------
 * init_read_base_addresses
 *
 * This routine reads all base addresses for the peripherals from the PCI
 * BARs.
 *---------------------------------------------------------------------------*/

int
init_read_base_addresses(INIT_BASE_ADDRESSES *base_addresses)
{
    unsigned long value;

    /* READ ALL BASE ADDRESSES */

    base_addresses->framebuffer_base = init_read_pci(init_video_base + 0x10);
    base_addresses->gp_register_base = init_read_pci(init_video_base + 0x14);
    base_addresses->vg_register_base = init_read_pci(init_video_base + 0x18);
    base_addresses->df_register_base = init_read_pci(init_video_base + 0x1C);
    base_addresses->vip_register_base = init_read_pci(init_video_base + 0x20);

    /* READ FRAME BUFFER SIZE */
    /* The frame buffer size is reported by a VSM in VSA II */
    /* Virtual Register Class    = 0x02                    */
    /* VG_MEM_SIZE (1MB units)   = 0x00                    */

    OUTW(0xAC1C, 0xFC53);
    OUTW(0xAC1C, 0x0200);

    value = (unsigned long) (INW(0xAC1E)) & 0xFE;

    base_addresses->framebuffer_size = value << 20;

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * init_read_cpu_frequency
 *
 * This routine returns the current CPU core frequency, in MHz.
 *---------------------------------------------------------------------------*/

int
init_read_cpu_frequency(unsigned long *cpu_frequency)
{
    /* CPU SPEED IS REPORTED BY A VSM IN VSA II */
    /* Virtual Register Class = 0x12 (Sysinfo)  */
    /* CPU Speed Register     = 0x01            */

    OUTW(0xAC1C, 0xFC53);
    OUTW(0xAC1C, 0x1201);

    *cpu_frequency = (unsigned long) (INW(0xAC1E));

    return CIM_STATUS_OK;
}

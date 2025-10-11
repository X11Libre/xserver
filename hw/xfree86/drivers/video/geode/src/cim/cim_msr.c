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
  * Cimarron MSR access routines.  These routines allow the user to query the
  * state of the GeodeLink Bus and read and write model-specfic registers.
  */

/*--------------------------------------------------------------*/
/* MSR GLOBALS                                                  */
/* These variables hold a local copy of the GeodeLink mapping   */
/* as well as a lookup table for easy device addressing.        */
/*--------------------------------------------------------------*/

GEODELINK_NODE gliu_node_list[24];
GEODELINK_NODE msr_dev_lookup[MSR_DEVICE_EMPTY];

#define GET_DEVICE_ID(macrohigh, macrolow) ((macrolow >> 12) & 0xFF)

/*---------------------------------------------------------------------------
 * msr_init_table
 *
 * This routine initializes the internal MSR table in Cimarron. This table is
 * used for any MSR device accesses.
 *--------------------------------------------------------------------------*/

int
msr_init_table(void)
{
    Q_WORD msr_value = { 0, 0 };
    unsigned int i, j;
    int return_value = CIM_STATUS_OK;

    /* CHECK FOR VALID GEODELINK CONFIGURATION
     * The CPU and the three GLIUs are assumed to be at known static
     * addresses, so we will check the device IDs at these addresses as proof
     * of a valid GeodeLink configuration 
     */

    MSR_READ(MSR_GEODELINK_CAP, MSR_ADDRESS_VAIL, &msr_value);
    if (GET_DEVICE_ID(msr_value.high, msr_value.low) != MSR_CLASS_CODE_VAIL)
        return_value = CIM_STATUS_ERROR;

    MSR_READ(MSR_GEODELINK_CAP, MSR_ADDRESS_GLIU0, &msr_value);
    if (GET_DEVICE_ID(msr_value.high, msr_value.low) != MSR_CLASS_CODE_GLIU)
        return_value = CIM_STATUS_ERROR;

    MSR_READ(MSR_GEODELINK_CAP, MSR_ADDRESS_GLIU1, &msr_value);
    if (GET_DEVICE_ID(msr_value.high, msr_value.low) != MSR_CLASS_CODE_GLIU)
        return_value = CIM_STATUS_ERROR;

    MSR_READ(MSR_GEODELINK_CAP, MSR_ADDRESS_GLIU2, &msr_value);
    if (GET_DEVICE_ID(msr_value.high, msr_value.low) != MSR_CLASS_CODE_GLIU)
        return_value = CIM_STATUS_ERROR;

    if (return_value == CIM_STATUS_OK) {
        /* BUILD LOCAL COPY OF THE GEODELINK BUS */

        msr_create_geodelink_table(gliu_node_list);

        /* CLEAR TABLE STATUS */

        for (i = 0; i < MSR_DEVICE_EMPTY; i++)
            msr_dev_lookup[i].device_id = MSR_DEVICE_NOTFOUND;

        /* CREATE EASY LOOKUP TABLE FOR FUTURE HARDWARE ACCESS    */
        /* Note that MSR_DEVICE_EMPTY is the index after the last */
        /* available device.  Also note that we fill in known     */
        /* devices before filling in the rest of the table.       */

        msr_dev_lookup[MSR_DEVICE_GEODELX_GLIU0].address_from_cpu =
            MSR_ADDRESS_GLIU0;
        msr_dev_lookup[MSR_DEVICE_GEODELX_GLIU0].device_id = MSR_DEVICE_PRESENT;
        msr_dev_lookup[MSR_DEVICE_GEODELX_GLIU1].address_from_cpu =
            MSR_ADDRESS_GLIU1;
        msr_dev_lookup[MSR_DEVICE_GEODELX_GLIU1].device_id = MSR_DEVICE_PRESENT;
        msr_dev_lookup[MSR_DEVICE_5535_GLIU].address_from_cpu =
            MSR_ADDRESS_GLIU2;
        msr_dev_lookup[MSR_DEVICE_5535_GLIU].device_id = MSR_DEVICE_PRESENT;
        msr_dev_lookup[MSR_DEVICE_GEODELX_VAIL].address_from_cpu =
            MSR_ADDRESS_VAIL;
        msr_dev_lookup[MSR_DEVICE_GEODELX_VAIL].device_id = MSR_DEVICE_PRESENT;

        for (i = 0; i < MSR_DEVICE_EMPTY; i++) {
            if (msr_dev_lookup[i].device_id == MSR_DEVICE_NOTFOUND) {
                for (j = 0; j < 24; j++) {
                    if (gliu_node_list[j].device_id == i)
                        break;
                }

                if (j == 24)
                    msr_dev_lookup[i].device_id = MSR_DEVICE_NOTFOUND;
                else {
                    msr_dev_lookup[i].device_id = MSR_DEVICE_PRESENT;
                    msr_dev_lookup[i].address_from_cpu =
                        gliu_node_list[j].address_from_cpu;
                }
            }
        }
    }
    else {
        /* ERROR OUT THE GEODELINK TABLES */

        for (i = 0; i < 24; i++) {
            gliu_node_list[i].address_from_cpu = 0xFFFFFFFF;
            gliu_node_list[i].device_id = MSR_DEVICE_EMPTY;
        }

        for (i = 0; i < MSR_DEVICE_EMPTY; i++) {
            msr_dev_lookup[i].address_from_cpu = 0xFFFFFFFF;
            msr_dev_lookup[i].device_id = MSR_DEVICE_NOTFOUND;
        }
    }
    return return_value;
}

/*---------------------------------------------------------------------------
 * msr_create_geodelink_table
 *
 * This routine dumps the contents of the GeodeLink bus into an array of
 * 24 GEODELINK_NODE structures.  Indexes 0-7 represent ports 0-7 of GLIU0,
 * indexes 8-15 represent ports 0-7 of GLIU1 and indexes 16-23 represent
 * ports 0-7 of GLIU2 (5535).
 *--------------------------------------------------------------------------*/

int
msr_create_geodelink_table(GEODELINK_NODE *gliu_nodes)
{
    unsigned long mbiu_port_count, reflective;
    unsigned long port, index;
    unsigned long gliu_count = 0;
    int glcp_count = 0;
    int usb_count = 0;
    int mpci_count = 0;
    Q_WORD msr_value = { 0, 0 };

    /* ALL THREE GLIUS ARE IN ONE ARRAY                               */
    /* Entries 0-7 contain the port information for GLIU0, entries    */
    /* 8-15 contain GLIU1 and 15-23 contain GLIU2.  We perform the    */
    /* enumeration in two passes.  The first simply fills in the      */
    /* addresses and class codes at each node.  The second pass       */
    /* translates the class codes into indexes into Cimarron's device */
    /* lookup table.                                                  */

    /* COUNT GLIU0 PORTS */

    MSR_READ(MSR_GLIU_CAP, MSR_ADDRESS_GLIU0, &msr_value);
    mbiu_port_count = (msr_value.high >> NUM_PORTS_SHIFT) & 7;

    /* FIND REFLECTIVE PORT */
    /* Query the GLIU for the port through which we are communicating. */
    /* We will avoid accesses to this port to avoid a self-reference.  */

    MSR_READ(MSR_GLIU_WHOAMI, MSR_ADDRESS_GLIU0, &msr_value);
    reflective = msr_value.low & WHOAMI_MASK;

    /* SPECIAL CASE FOR PORT 0 */
    /* GLIU0 port 0 is a special case, as it points back to GLIU0.  GLIU0 */
    /* responds at address 0x10000xxx, which does not equal 0 << 29.      */

    gliu_nodes[0].address_from_cpu = MSR_ADDRESS_GLIU0;
    gliu_nodes[0].device_id = MSR_CLASS_CODE_GLIU;

    /* ENUMERATE ALL PORTS */

    for (port = 1; port < 8; port++) {
        /* FILL IN ADDRESS */

        gliu_nodes[port].address_from_cpu = port << 29;

        if (port == reflective)
            gliu_nodes[port].device_id = MSR_CLASS_CODE_REFLECTIVE;
        else if (port > mbiu_port_count)
            gliu_nodes[port].device_id = MSR_CLASS_CODE_UNPOPULATED;
        else {
            MSR_READ(MSR_GEODELINK_CAP, gliu_nodes[port].address_from_cpu,
                     &msr_value);
            gliu_nodes[port].device_id =
                GET_DEVICE_ID(msr_value.high, msr_value.low);
        }
    }

    /* COUNT GLIU1 PORTS */

    MSR_READ(MSR_GLIU_CAP, MSR_ADDRESS_GLIU1, &msr_value);
    mbiu_port_count = (msr_value.high >> NUM_PORTS_SHIFT) & 7;

    /* FIND REFLECTIVE PORT */

    MSR_READ(MSR_GLIU_WHOAMI, MSR_ADDRESS_GLIU1, &msr_value);
    reflective = msr_value.low & WHOAMI_MASK;

    /* ENUMERATE ALL PORTS */

    for (port = 0; port < 8; port++) {
        index = port + 8;

        /* FILL IN ADDRESS */

        gliu_nodes[index].address_from_cpu = (0x02l << 29) + (port << 26);

        if (port == reflective)
            gliu_nodes[index].device_id = MSR_CLASS_CODE_REFLECTIVE;
        else if (port > mbiu_port_count)
            gliu_nodes[index].device_id = MSR_CLASS_CODE_UNPOPULATED;
        else {
            MSR_READ(MSR_GEODELINK_CAP, gliu_nodes[index].address_from_cpu,
                     &msr_value);
            gliu_nodes[index].device_id =
                GET_DEVICE_ID(msr_value.high, msr_value.low);
        }
    }

    /* COUNT GLIU2 PORTS */

    MSR_READ(MSR_GLIU_CAP, MSR_ADDRESS_GLIU2, &msr_value);
    mbiu_port_count = (msr_value.high >> NUM_PORTS_SHIFT) & 7;

    /* FIND REFLECTIVE PORT */

    MSR_READ(MSR_GLIU_WHOAMI, MSR_ADDRESS_GLIU2, &msr_value);
    reflective = msr_value.low & WHOAMI_MASK;

    /* FILL IN PORT 0 AND 1 */
    /* Port 0 on 5535 is MBIU2.  Port 1 is MPCI, but it is referenced at */
    /* a special address.                                                */

    gliu_nodes[16].address_from_cpu = MSR_ADDRESS_GLIU2;
    gliu_nodes[16].device_id = MSR_CLASS_CODE_GLIU;

    gliu_nodes[17].address_from_cpu = MSR_ADDRESS_5535MPCI;
    gliu_nodes[17].device_id = MSR_CLASS_CODE_MPCI;

    /* ENUMERATE ALL PORTS */

    for (port = 2; port < 8; port++) {
        index = port + 16;

        /* FILL IN ADDRESS */

        gliu_nodes[index].address_from_cpu =
            (0x02l << 29) + (0x04l << 26) + (0x02l << 23) + (port << 20);

        if (port == reflective)
            gliu_nodes[index].device_id = MSR_CLASS_CODE_REFLECTIVE;
        else if (port > mbiu_port_count)
            gliu_nodes[index].device_id = MSR_CLASS_CODE_UNPOPULATED;
        else {
            MSR_READ(MSR_GEODELINK_CAP, gliu_nodes[index].address_from_cpu,
                     &msr_value);
            gliu_nodes[index].device_id =
                GET_DEVICE_ID(msr_value.high, msr_value.low);
        }
    }

    /* SECOND PASS - TRANSLATION */
    /* Now that the class codes for each device are stored in the  */
    /* array, we walk through the array and translate the class    */
    /* codes to table indexes.  For class codes that have multiple */
    /* instances, the table indexes are sequential.                */

    for (port = 0; port < 24; port++) {
        /* SPECIAL CASE FOR GLIU UNITS */
        /* A GLIU can be both on another port and on its own port.  These  */
        /* end up as the same address, but are shown as duplicate nodes in */
        /* the GeodeLink table.                                            */

        if ((port & 7) == 0)
            gliu_count = port >> 3;

        switch (gliu_nodes[port].device_id) {
            /* UNPOPULATED OR REFLECTIVE NODES */

        case MSR_CLASS_CODE_UNPOPULATED:
            index = MSR_DEVICE_EMPTY;
            break;
        case MSR_CLASS_CODE_REFLECTIVE:
            index = MSR_DEVICE_REFLECTIVE;
            break;

            /* KNOWN CLASS CODES */

        case MSR_CLASS_CODE_GLIU:
            index = MSR_DEVICE_GEODELX_GLIU0 + gliu_count++;
            break;
        case MSR_CLASS_CODE_GLCP:
            index = MSR_DEVICE_GEODELX_GLCP + glcp_count++;
            break;
        case MSR_CLASS_CODE_MPCI:
            index = MSR_DEVICE_GEODELX_MPCI + mpci_count++;
            break;
        case MSR_CLASS_CODE_USB:
            index = MSR_DEVICE_5535_USB2 + usb_count++;
            break;
        case MSR_CLASS_CODE_USB2:
            index = MSR_DEVICE_5536_USB_2_0;
            break;
        case MSR_CLASS_CODE_ATAC:
            index = MSR_DEVICE_5535_ATAC;
            break;
        case MSR_CLASS_CODE_MDD:
            index = MSR_DEVICE_5535_MDD;
            break;
        case MSR_CLASS_CODE_ACC:
            index = MSR_DEVICE_5535_ACC;
            break;
        case MSR_CLASS_CODE_MC:
            index = MSR_DEVICE_GEODELX_MC;
            break;
        case MSR_CLASS_CODE_GP:
            index = MSR_DEVICE_GEODELX_GP;
            break;
        case MSR_CLASS_CODE_VG:
            index = MSR_DEVICE_GEODELX_VG;
            break;
        case MSR_CLASS_CODE_DF:
            index = MSR_DEVICE_GEODELX_DF;
            break;
        case MSR_CLASS_CODE_FG:
            index = MSR_DEVICE_GEODELX_FG;
            break;
        case MSR_CLASS_CODE_VIP:
            index = MSR_DEVICE_GEODELX_VIP;
            break;
        case MSR_CLASS_CODE_AES:
            index = MSR_DEVICE_GEODELX_AES;
            break;
        case MSR_CLASS_CODE_VAIL:
            index = MSR_DEVICE_GEODELX_VAIL;
            break;
        default:
            index = MSR_DEVICE_EMPTY;
            break;
        }

        gliu_nodes[port].device_id = index;
    }

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * msr_create_device_list
 *
 * This routine dumps a list of all known GeodeLX/5535 devices as well as their
 * respective status and address.
 *--------------------------------------------------------------------------*/

int
msr_create_device_list(GEODELINK_NODE *gliu_nodes, int max_devices)
{
    int i, count;

    if (max_devices < MSR_DEVICE_EMPTY)
        count = max_devices;
    else
        count = MSR_DEVICE_EMPTY;

    for (i = 0; i < count; i++) {
        gliu_nodes[i].address_from_cpu = msr_dev_lookup[i].address_from_cpu;
        gliu_nodes[i].device_id = msr_dev_lookup[i].device_id;
    }

    return CIM_STATUS_OK;
}

/*--------------------------------------------------------------------
 * msr_read64
 *
 * Performs a 64-bit read from 'msr_register' in device 'device'.  'device' is
 * an index into Cimarron's table of known GeodeLink devices.
 *-------------------------------------------------------------------*/

int
msr_read64(unsigned long device, unsigned long msr_register, Q_WORD *msr_value)
{
    if (device < MSR_DEVICE_EMPTY) {
        if (msr_dev_lookup[device].device_id == MSR_DEVICE_PRESENT) {
            MSR_READ(msr_register, msr_dev_lookup[device].address_from_cpu,
                     msr_value);
            return CIM_STATUS_OK;
        }
    }

    msr_value->low = msr_value->high = 0;
    return CIM_STATUS_DEVNOTFOUND;
}

/*--------------------------------------------------------------------
 * msr_write64
 *
 * Performs a 64-bit write to 'msr_register' in device 'device'.  'device' is
 * an index into Cimarron's table of known GeodeLink devices.
 *-------------------------------------------------------------------*/

int
msr_write64(unsigned long device, unsigned long msr_register, Q_WORD *msr_value)
{
    if (device < MSR_DEVICE_EMPTY) {
        if (msr_dev_lookup[device].device_id == MSR_DEVICE_PRESENT) {
            MSR_WRITE(msr_register, msr_dev_lookup[device].address_from_cpu,
                      msr_value);
            return CIM_STATUS_OK;
        }
    }
    return CIM_STATUS_DEVNOTFOUND;
}

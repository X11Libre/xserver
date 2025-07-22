/* Copyright (c) 2005 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * */

/* 
 * This file contains MSR access routines for Redcloud.
 * */

void redcloud_build_mbus_tree(void);    /* private routine definition */
int redcloud_init_msr_devices(MSR aDev[], unsigned int array_size);

                                                                                /* private routine definition */
DEV_STATUS redcloud_find_msr_device(MSR * pDev);

                                                                                /* private routine definition */

/* REDCLOUD MSR BITMASKS */

#define MBD_MSR_CAP			0x2000
#define MSR_CAP_ID_MASK		0xFF000
#define MSR_CAP_ID_SHIFT  	12
#define MSR_CAP_REV_MASK    0x0F
#define MBIU_CAP			0x86
#define NUM_PORTS_MASK		0x00380000
#define NUM_PORTS_SHIFT  	19
#define MBIU_WHOAMI			0x8B
#define WHOAMI_MASK			0x07

/* REDCLOUD and CS5535 MSR DEVICES */

MSR msrDev[] = {
    {FOUND, RC_CC_MBIU, RC_MB0_MBIU0},
    {FOUND, RC_CC_MBIU, RC_MB0_MBIU1},
    {NOT_KNOWN, RC_CC_MCP, FAKE_ADDRESS},
    {NOT_KNOWN, RC_CC_MPCI, FAKE_ADDRESS},
    {NOT_KNOWN, RC_CC_MC, FAKE_ADDRESS},
    {NOT_KNOWN, RC_CC_GP, FAKE_ADDRESS},
    {NOT_KNOWN, RC_CC_VG, FAKE_ADDRESS},
    {NOT_KNOWN, RC_CC_DF, FAKE_ADDRESS},
    {NOT_KNOWN, RC_CC_FG, FAKE_ADDRESS},
    {FOUND, RC_CC_VA, RC_MB0_CPU},
    {FOUND, CP_CC_MBIU, CP_MB0_MBIU0},
    {NOT_KNOWN, CP_CC_MPCI, FAKE_ADDRESS},
    {NOT_KNOWN, CP_CC_USB2, FAKE_ADDRESS},
    {NOT_KNOWN, CP_CC_ATAC, FAKE_ADDRESS},
    {NOT_KNOWN, CP_CC_MDD, FAKE_ADDRESS},
    {NOT_KNOWN, CP_CC_ACC, FAKE_ADDRESS},
    {NOT_KNOWN, CP_CC_USB1, FAKE_ADDRESS},
    {NOT_KNOWN, CP_CC_MCP, FAKE_ADDRESS},
};

#define NUM_DEVS sizeof(msrDev) / sizeof(struct msr)

/* CAPISTRANO DEVICE INDEX LIMITS */
/* These defines represent the start and stop indexes into the device array
 * for all Capistrano devices.  These should be updated whenever a device is
 * added or removed to the Capistrano list.
 * */

#define CP_INDEX_START CP_ID_MBIU
#define CP_INDEX_STOP  CP_ID_MCP

/* GLOBAL MBUS CACHE STRUCTURES */
/* These structures contain a "cached" copy of the MBUS topology */
/* for easy future lookup.                                       */

MBUS_NODE MBIU0[8], MBIU1[8], MBIU2[8];

/* REGISTER MACROS */

#define GET_DEVICE_ID( CAPABILITIES_HIGH, CAPABILITIES_LOW ) \
					 ((unsigned int)(( (CAPABILITIES_LOW) & MSR_CAP_ID_MASK ) >> MSR_CAP_ID_SHIFT ))

#define GET_NUM_PORTS( MBIU_CAP_HIGH, MBIU_CAP_LOW ) (((MBIU_CAP_HIGH) & NUM_PORTS_MASK ) >> NUM_PORTS_SHIFT)

/*----------------------------------------------------------------------------
 * gfx_msr_init
 * 
 * This routine initializes the base addresses of all known MBUS devices.  
 *----------------------------------------------------------------------------
 */
#if GFX_MSR_DYNAMIC
int
redcloud_msr_init(void)
#else
int
gfx_msr_init(void)
#endif
{
    Q_WORD msrValue;
    int return_value = 1;

    /* CHECK FOR VALID MBUS CONFIGURATION */
    /* The CPU and the two MBIUs are assumed to be at known static addresses,
     * so we will check the device IDs at these addresses as proof of a valid
     * mbus  configuration.
     * */

    MSR_READ(MBD_MSR_CAP, RC_MB0_CPU, &(msrValue.high), &(msrValue.low));
    if (GET_DEVICE_ID(msrValue.high, msrValue.low) != RC_CC_VA)
        return_value = 0;

    MSR_READ(MBD_MSR_CAP, RC_MB0_MBIU0, &(msrValue.high), &(msrValue.low));
    if (GET_DEVICE_ID(msrValue.high, msrValue.low) != RC_CC_MBIU)
        return_value = 0;

    MSR_READ(MBD_MSR_CAP, RC_MB0_MBIU1, &(msrValue.high), &(msrValue.low));
    if (GET_DEVICE_ID(msrValue.high, msrValue.low) != RC_CC_MBIU)
        return_value = 0;

    /* ENUMERATE VALID BUS */
    /* If all static devices were identified, continue with the enumeration */

    if (return_value) {
        /* OPTIMIZATION */
        /* Build a local copy of the MBUS topology.  This allows us to  */
        /* quickly search the entire MBUS for a given device ID without */
        /* repeated MSR accesses.                                       */

        redcloud_build_mbus_tree();

        /* INITIALIZE MSR DEVICES */

        return_value = redcloud_init_msr_devices(msrDev, NUM_DEVS);

    }

    return return_value;

}

/*--------------------------------------------------------------------------
 * void	redcloud_build_mbus_tree() (PRIVATE ROUTINE - NOT PART OF DURANGO API)
 *
 * This routine walks through the MBUS and records the address value and 
 * device ID found at each node.  If a node (aka port) is not populated, 
 * that node returns '0'.  The deviceID for that node is set to '0' 
 * (NOT_POPULATED) to reflect this. If the node being queried points back to 
 * Vail or MBIU0, the deviceID for that node is set to 'REFLECTIVE'.  
 * Reflective nodes are nodes that forward the given MBUS address BACK to the
 *  initiator.
 *----------------------------------------------------------------------------
 */
void
redcloud_build_mbus_tree(void)
{
    unsigned long mbiu_port_count, reflective;
    unsigned long port;
    Q_WORD msrValue;

    /*                  */
    /* ENUMERATE MBIU0  */
    /*                  */

    /* COUNT MBIU PORTS */

    MSR_READ(MBIU_CAP, RC_MB0_MBIU0, &(msrValue.high), &(msrValue.low));
    mbiu_port_count = GET_NUM_PORTS(msrValue.high, msrValue.low);

    /* FIND REFLECTIVE PORT */
    /* Query the MBIU for the port through which we are communicating. */
    /* We will avoid accesses to this port to avoid a self-reference.  */

    MSR_READ(MBIU_WHOAMI, RC_MB0_MBIU0, &(msrValue.high), &(msrValue.low));
    reflective = msrValue.low & WHOAMI_MASK;

    /* ENUMERATE ALL PORTS */
    /* For every possible port, set the MBIU.deviceId to something. */

    for (port = 0; port < 8; port++) {
        /* FILL IN CLAIMED FIELD */
        /* All MBIU ports can only be assigned to one device from the */
        /* Durango table                                              */

        MBIU0[port].claimed = 0;

        /* MBIU0 PORT NUMBERS ARE IN ADDRESS BITS 31:29 */

        MBIU0[port].address = port << 29;

        /* SPECIAL CASE FOR MBIU0 */
        /* MBIU0 port 0 is a special case, as it points back to MBIU0.  MBIU0
         * responds at address 0x40000xxx, which does not equal 0 << 29.
         * */

        if (port == 0)
            MBIU0[port].deviceId = RC_CC_MBIU;
        else if (port == reflective)
            MBIU0[port].deviceId = REFLECTIVE;
        else if (port > mbiu_port_count)
            MBIU0[port].deviceId = NOT_POPULATED;
        else {
            MSR_READ(MBD_MSR_CAP, MBIU0[port].address, &(msrValue.high),
                     &(msrValue.low));
            MBIU0[port].deviceId = GET_DEVICE_ID(msrValue.high, msrValue.low);
        }
    }

    /*                  */
    /* ENUMERATE MBIU1  */
    /*                  */

    /* COUNT MBIU PORTS */

    MSR_READ(MBIU_CAP, RC_MB0_MBIU1, &(msrValue.high), &(msrValue.low));
    mbiu_port_count = GET_NUM_PORTS(msrValue.high, msrValue.low);

    /* FIND REFLECTIVE PORT */
    /* Query the MBIU for the port through which we are communicating. */
    /* We will avoid accesses to this port to avoid a self-reference.  */

    MSR_READ(MBIU_WHOAMI, RC_MB0_MBIU1, &(msrValue.high), &(msrValue.low));
    reflective = msrValue.low & WHOAMI_MASK;

    /* ENUMERATE ALL PORTS */
    /* For every possible port, set the MBIU.deviceId to something. */

    for (port = 0; port < 8; port++) {
        /* FILL IN CLAIMED FIELD */
        /* All MBIU ports can only be assigned to one device from the */
        /* Durango table                                              */

        MBIU1[port].claimed = 0;

        /* MBIU1 PORT NUMBERS ARE IN 28:26 AND 31:29 = 010B */

        MBIU1[port].address = (0x02l << 29) + (port << 26);

        if (port == reflective)
            MBIU1[port].deviceId = REFLECTIVE;
        else if (port > mbiu_port_count)
            MBIU1[port].deviceId = NOT_POPULATED;
        else {
            MSR_READ(MBD_MSR_CAP, MBIU1[port].address, &(msrValue.high),
                     &(msrValue.low));
            MBIU1[port].deviceId = GET_DEVICE_ID(msrValue.high, msrValue.low);
        }
    }

    /*                          */
    /* ENUMERATE MBIU2 (CS5535) */
    /*  (if present)            */

    MSR_READ(MBD_MSR_CAP, CP_MB0_MBIU0, &(msrValue.high), &(msrValue.low));
    if (GET_DEVICE_ID(msrValue.high, msrValue.low) == CP_CC_MBIU) {
        /* COUNT MBIU PORTS */

        MSR_READ(MBIU_CAP, CP_MB0_MBIU0, &(msrValue.high), &(msrValue.low));
        mbiu_port_count = GET_NUM_PORTS(msrValue.high, msrValue.low);

        /* FIND REFLECTIVE PORT */
        /* Query the MBIU for the port through which we are communicating. */
        /* We will avoid accesses to this port to avoid a self-reference.  */

        MSR_READ(MBIU_WHOAMI, CP_MB0_MBIU0, &(msrValue.high), &(msrValue.low));
        reflective = msrValue.low & WHOAMI_MASK;

        /* ENUMERATE ALL PORTS */
        /* For every possible port, set the MBIU.deviceId to something. */

        for (port = 0; port < 8; port++) {
            /* FILL IN CLAIMED FIELD */
            /* All MBIU ports can only be assigned to one device from the */
            /* Durango table                                              */

            MBIU2[port].claimed = 0;

            /* MBIU2 PORT NUMBERS ARE IN 22:20 AND 31:23 = 010100010B */

            MBIU2[port].address =
                (0x02l << 29) + (0x04l << 26) + (0x02l << 23) + (port << 20);

            if (port == reflective)
                MBIU2[port].deviceId = REFLECTIVE;
            else if (port > mbiu_port_count)
                MBIU2[port].deviceId = NOT_POPULATED;
            else {
                MSR_READ(MBD_MSR_CAP, MBIU2[port].address, &(msrValue.high),
                         &(msrValue.low));
                MBIU2[port].deviceId =
                    GET_DEVICE_ID(msrValue.high, msrValue.low);
            }
        }
    }
    else {
        /* NO 5535                                                  */
        /* If the CS5535 is not installed, fill in the cached table */
        /* with the 'NOT_INSTALLED' flag.  Also, fill in the device */
        /* status from NOT_KNOWN to REQ_NOT_INSTALLED.              */

        for (port = 0; port < 8; port++) {
            MBIU2[port].claimed = 0;
            MBIU2[port].deviceId = NOT_INSTALLED;
            MBIU2[port].address =
                (0x02l << 29) + (0x04l << 26) + (0x02l << 23) + (port << 20);
        }
        for (port = CP_INDEX_START; port <= CP_INDEX_STOP; port++) {
            msrDev[port].Present = REQ_NOT_INSTALLED;
        }
    }
}

/*------------------------------------------------------------------
 * redcloud_init_msr_devices (PRIVATE ROUTINE - NOT PART OF DURANGO API)
 *
 * Handles the details of finding each possible device on the MBUS.
 * If a given device is not found, its structure is left uninitialized.
 * If a given device is found, its structure is updated.
 *
 * This init routine only checks for devices in aDev[].
 *
 *  Passed:
 *		aDev - is a pointer to the array of MBUS devices.
 *		arraySize - number of elements in aDev.
 *
 *	Returns:
 *		1 - If, for every device, its address was found.
 *		0 - If, for any device, an error was encountered.
 *------------------------------------------------------------------
 */
int
redcloud_init_msr_devices(MSR aDev[], unsigned int array_size)
{
    unsigned int i, issues = 0;

    /* TRY TO FIND EACH ITEM IN THE ARRAY */

    for (i = 0; i < array_size; i++) {
        /* IGNORE DEVICES THAT ARE ALREADY FOUND                */
        /* The addresses for "found" devices are already known. */

        if (aDev[i].Present == FOUND || aDev[i].Present == REQ_NOT_INSTALLED)
            continue;

        /* TRY TO FIND THE DEVICE ON THE MBUS */

        aDev[i].Present = redcloud_find_msr_device(&aDev[i]);

        /* INCREMENT ERROR COUNT IF DEVICE NOT FOUND */

        if (aDev[i].Present != FOUND)
            issues++;
    }

    return (issues == 0);
}

/*------------------------------------------------------------------
 *  redcloud_find_msr_device (PRIVATE ROUTINE - NOT PART OF DURANGO API)
 *
 *  Passed:
 *	    pDev - is a pointer to one element in the array of MBUS devices
 *
 *  Returns:
 *	    FOUND - Device was found and pDev->Address has been updated.
 *
 *		REQ_NOT_FOUND - Device was not found and pDev->Address has not
 *						been updated.
 *
 *------------------------------------------------------------------
 */
DEV_STATUS
redcloud_find_msr_device(MSR *pDev)
{
    unsigned int i;

    /* SEARCH DURANGO'S CACHED MBUS TOPOLOGY */
    /* This gets a little tricky.  As the only identifier we have for each   
     * device is the device ID and we have multiple devices of the same type 
     * MCP, MPCI, USB, etc. we need to make some assumptions based on table  
     * order.  These are as follows:                                         
     * 1. All Redcloud nodes are searched first, as we assume that they      
     *    are first in the table.                                            
     * 2. If two devices have the same device ID and are found on the same   
     *    device (GX2, CS5535, etc.) we assume that they are listed such     
     *    that the first device in the table with this device ID has a lower 
     *    port address.                                                      
     * 3. After a device ID has been matched, the port is marked as          
     *    'claimed', such that future enumerations continue searching the    
     *    GeodeLink topology.
     */

    /* SEARCH MBIU0 */

    for (i = 0; i < 8; i++) {
        if (MBIU0[i].deviceId == pDev->Id && !(MBIU0[i].claimed)) {
            MBIU0[i].claimed = 1;
            pDev->Address = MBIU0[i].address;
            return FOUND;
        }
    }

    /* SEARCH MBIU1 */

    for (i = 0; i < 8; i++) {
        if (MBIU1[i].deviceId == pDev->Id && !(MBIU1[i].claimed)) {
            MBIU1[i].claimed = 1;
            pDev->Address = MBIU1[i].address;
            return FOUND;
        }
    }

    /* SEARCH MBIU2 */

    for (i = 0; i < 8; i++) {
        if (MBIU2[i].deviceId == pDev->Id && !(MBIU2[i].claimed)) {
            MBIU2[i].claimed = 1;
            pDev->Address = MBIU2[i].address;
            return FOUND;
        }
    }

    return REQ_NOT_FOUND;
}

/*--------------------------------------------------------------------
 * gfx_id_msr_device 
 *
 *  This routine handles reading the capabilities MSR register (typically
 *  0x2000) and checking if the 'id' field matches pDev.Id.  This routine is
 *  used by applications/drivers that need to extend the list of known
 *  MBUS devices beyond those known by Durango.
 *
 *		Passed:
 *			pDev - Pointer to MSR structure containing the device's ID.
 *		    address - device address.
 *
 *		Returns:
 *			FOUND - The IDs do match.
 *			REQ_NOT_FOUND - There was not a match.
 *
 *--------------------------------------------------------------------
 */
#if GFX_MSR_DYNAMIC
DEV_STATUS
redcloud_id_msr_device(MSR *pDev, unsigned long address)
#else
DEV_STATUS
gfx_id_msr_device(MSR *pDev, unsigned long address)
#endif
{
    Q_WORD msrValue;

    MSR_READ(MBD_MSR_CAP, address, &(msrValue.high), &(msrValue.low));

    if (GET_DEVICE_ID(msrValue.high, msrValue.low) == pDev->Id)
        return FOUND;
    else
        return REQ_NOT_FOUND;
}

/*--------------------------------------------------------------------
 * gfx_get_msr_dev_address
 * 
 * This function returns the 32-bit address of the requested device.
 * The device must be a known MBUS device.  (It must be in Durango's 
 * device table.)  DEV_STATUS should be checked to verify that the address 
 * was updated.
 *
 *
 * Passed:
 *     device - device index of the device in question.
 *	   *address - ptr to location where address should be stored.
 *         
 * Returns:
 *	   DEV_STATUS of device in question.  (NOT_KNOWN if device is out of range.)
 *     *address - updated if 'device' is within range
 *   
 *	Notes:
 *     This function should only be called after gfx_msr_init
 *
 *--------------------------------------------------------------------
 */
#if GFX_MSR_DYNAMIC
DEV_STATUS
redcloud_get_msr_dev_address(unsigned int device, unsigned long *address)
#else
DEV_STATUS
gfx_get_msr_dev_address(unsigned int device, unsigned long *address)
#endif
{
    if (device < NUM_DEVS) {
        if (msrDev[device].Present == FOUND)
            *address = msrDev[device].Address;

        return msrDev[device].Present;
    }
    return NOT_KNOWN;

}

/*--------------------------------------------------------------------
 *  gfx_get_glink_id_at_address
 *   
 *	This function returns the 16-bit deviceId at the requested address.
 *  DEV_STATUS should be checked to make sure that device was updated.
 *
 *	Passed:
 *	    device - ptr to location where device ID should be stored.
 *		address - address of desired device ID.
 *         
 *  Returns:
 *	    FOUND if address is a valid address, NOT_KNOWN if address cannot be 
 *	    found on the mbus.
 *      *device - updated with device Id info.
 *
 *	Notes:
 *      This function should be called after gfx_msr_init 
 *
 *--------------------------------------------------------------------
 */
#if GFX_MSR_DYNAMIC
DEV_STATUS
redcloud_get_glink_id_at_address(unsigned int *device, unsigned long address)
#else
DEV_STATUS
gfx_get_glink_id_at_address(unsigned int *device, unsigned long address)
#endif
{
    int port;

    for (port = 0; port < 8; port++) {
        if (MBIU0[port].address == address) {
            *device = MBIU0[port].deviceId;
            return FOUND;
        }
        else if (MBIU1[port].address == address) {
            *device = MBIU1[port].deviceId;
            return FOUND;
        }
        else if (MBIU2[port].address == address) {
            *device = MBIU2[port].deviceId;
            return FOUND;
        }
    }

    return NOT_KNOWN;

}

/*--------------------------------------------------------------------
 * gfx_msr_read
 *
 * Performs a 64-bit read from 'msrRegister' in device 'device'.  'device' is 
 * an index into Durango's table of known MBUS devices.
 *
 * Returns:
 *     FOUND			- if no errors were detected and msrValue has been 
 *     						updated.
 *	   NOT_KNOWN		- an error was detected.  msrValue is not updated.
 *	   REQ_NOT_FOUND 	- 'msrAddress' for 'devID' is unknown.  Caller
 *							should call msrInit() first.  msrValue is not 
 *							updated.
 * Notes:
 *	 This function should be called after gfx_msr_init
 *--------------------------------------------------------------------
 */
#if GFX_MSR_DYNAMIC
DEV_STATUS
redcloud_msr_read(unsigned int device, unsigned int msrRegister,
                  Q_WORD *msrValue)
#else
DEV_STATUS
gfx_msr_read(unsigned int device, unsigned int msrRegister, Q_WORD *msrValue)
#endif
{
    if (device < NUM_DEVS) {
        if (msrDev[device].Present == FOUND)
            MSR_READ(msrRegister, msrDev[device].Address, &(msrValue->high),
                     &(msrValue->low));

        return msrDev[device].Present;
    }
    return NOT_KNOWN;
}

/*--------------------------------------------------------------------
 * gfx_msr_write
 *
 *		Performs a 64-bit write to 'msrRegister' in device 'devID'.
 *
 * Returns:
 *		FOUND 			- if no errors were detected and msrValue has been 
 *							updated.
 *		NOT_KNOWN		- an error was detected.  msrValue is not updated.
 *		REQ_NOT_FOUND 	- 'msrAddress' for 'devID' is unknown.  Caller
 *							should call msrInit() first.  msrValue is not 
 *							updated.
 *
 *Notes:
 *     	This function is valid to call after initMSR_API() 
 *
 *--------------------------------------------------------------------
 */
#if GFX_MSR_DYNAMIC
DEV_STATUS
redcloud_msr_write(unsigned int device, unsigned int msrRegister,
                   Q_WORD *msrValue)
#else
DEV_STATUS
gfx_msr_write(unsigned int device, unsigned int msrRegister, Q_WORD *msrValue)
#endif
{
    if (device < NUM_DEVS) {
        if (msrDev[device].Present == FOUND)
            MSR_WRITE(msrRegister, msrDev[device].Address, &(msrValue->high),
                      &(msrValue->low));

        return msrDev[device].Present;
    }

    return NOT_KNOWN;
}

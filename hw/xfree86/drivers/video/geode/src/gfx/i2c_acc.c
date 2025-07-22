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
 * This file contains routines to write to and read from the I2C bus using
 * the ACCESS.bus hardware in the SC1200. 
 * */

/* SUPER IO DEFINITIONS */

#define INDEX_1					0x15C   /* base address 1 selected              */
#define DATA_1					0x15D
#define INDEX_2					0x2E    /* base address 2 selected              */
#define DATA_2					0x2F
#define PCI_INDEX				0xCF8   /* PCI configuration space INDEX */
#define PCI_DATA				0xCFC   /* PCI configuration space DATA  */
#define BASE_ADR_MSB_REG		0x60    /* base address MSB register    */
#define BASE_ADR_LSB_REG		0x61    /* base address LSB register    */

#define SIO_BASE_ADR_15C_15D	0x6000000
#define SIO_BASE_ADR_2E_2F 		0x4000000

/* SUPER IO GLOBALS */

unsigned short index_reg, data_reg;

/* ACCESS BUS DEFINITIONS */

#define ACC_I2C_TIMEOUT 1000000 /* Number of reads before timing out */
#define ACB1_BASE 	    0x810       /* ACCESS.bus base addresses         */
#define ACB2_BASE 	    0x820
#define ACBSDA			0       /* ACB serial data                   */
#define ACBST			1       /* ACB status                        */
#define ACBCST			2       /* ACB control status                */
#define ACBCTL1			3       /* ACB control 1                     */
#define ACBADDR			4       /* ACB own address                   */
#define ACBCTL2		    5   /* ACB control 2                     */
#define LDN				0x7     /* Logical Device Numbers            */
#define ACB1_LDN		0x5
#define ACB2_LDN		0x6

/* INITIAL ACCESS.bus BASE ADDRESS VALUES */

unsigned short base_address_array[3] = { 0, ACB1_BASE, ACB2_BASE };

char Freq = 0x71;

/* LOCAL ACCESS.bus FUNCTION DECLARATIONS */

void acc_i2c_start(unsigned char busnum);
void acc_i2c_stop(unsigned char busnum);
void acc_i2c_abort_data(unsigned char busnum);
void acc_i2c_bus_recovery(unsigned char busnum);
void acc_i2c_stall_after_start(unsigned char busnum, int state);
void acc_i2c_send_address(unsigned char busnum, unsigned char cData);
int acc_i2c_ack(unsigned char busnum, int fPut, int negAck);
void acc_i2c_stop_clock(unsigned char busnum);
void acc_i2c_activate_clock(unsigned char busnum);
void acc_i2c_write_byte(unsigned char busnum, unsigned char cData);
unsigned char acc_i2c_read_byte(unsigned char busnum, int last_byte);
void acc_i2c_reset_bus(unsigned char busnum);
int acc_i2c_request_master(unsigned char busnum);
void acc_i2c_config(unsigned char busnum, short adr, char freq);
char acc_i2c_set_freq(unsigned char busnum, char freq);
unsigned short acc_i2c_set_base_address(unsigned char busnum, short adr);

/* LOCAL HELPER ROUTINES */

void OsPciReadDWord(int bus, int dev, int func, int address,
                    unsigned long *data);
int sio_set_index_data_reg(void);
void sio_write_reg(unsigned char reg, unsigned char data);
unsigned char sio_read_reg(unsigned char reg);

/*---------------------------------------------------------------------------
 * OsPciReadDWord
 *
 * This routine reads one double word from the PCI configuration header
 * defined by 'bus', 'dev', 'func' and 'address' to the double word
 * pointed to by 'data'.
 *
 * Returns : None.
 *---------------------------------------------------------------------------
 */
void
OsPciReadDWord(int bus, int dev, int func, int address, unsigned long *data)
{
    /*
     * The address of a double word in the Configuration Header is built in
     * the following way :
     * {10000000,bus[23:16],device[15:11],function[10:8],address[7:2],00}
     */
    long addr = (0x80000000 |
                 ((bus & 0xff) << 16) |
                 ((dev & 0x1f) << 11) | ((func & 0x7) << 8) | (address & 0xff));
    OUTD(PCI_INDEX, addr);
    *data = IND(PCI_DATA);
}

/*---------------------------------------------------------------------------
 *	sio_set_index_data_reg
 *
 *	This routine checks which index and data registers to use
 *  in order to access the Super I/O registers
 *
 *	Returns : 1 - OK
 *            0 - Super I/O disabled or configuration access disabled
 *---------------------------------------------------------------------------
 */
int
sio_set_index_data_reg(void)
{
    unsigned long xbus_expention_bar, io_control_reg1;

    OsPciReadDWord(0, 0x12, 5, 0x10, &xbus_expention_bar);
    xbus_expention_bar = xbus_expention_bar & 0xfffffffe;
    io_control_reg1 = IND((unsigned short) xbus_expention_bar);

    if ((io_control_reg1) & (SIO_BASE_ADR_15C_15D)) {
        index_reg = INDEX_1;
        data_reg = DATA_1;
        return (1);
    }

    if ((io_control_reg1) & (SIO_BASE_ADR_2E_2F)) {
        index_reg = INDEX_2;
        data_reg = DATA_2;
        return (1);
    }

    return (0);
}

/*---------------------------------------------------------------------------
 *	sio_write_reg
 *
 *	This routine writes 'data' to 'reg' Super I/O register
 *
 *	Returns : None
 *---------------------------------------------------------------------------
 */
void
sio_write_reg(unsigned char reg, unsigned char data)
{
    OUTB(index_reg, reg);
    OUTB(data_reg, data);
}

/*---------------------------------------------------------------------------
 *	sio_read_reg
 *
 *	This routine reads data from 'reg' Super I/O register
 *
 *	Returns : The data read from the requested register
 *---------------------------------------------------------------------------
 */
unsigned char
sio_read_reg(unsigned char reg)
{
    OUTB(index_reg, reg);
    return INB(data_reg);
}

/*---------------------------------------------------------------------------
 *	gfx_i2c_reset
 *
 *	This routine resets the I2C bus as follows :
 *	· Sets the base address of the ACCESS.bus
 *	· Sets the frequency of the ACCESS.bus
 *	· Resets the ACCESS.bus
 *
 * 	If 'adr'  is -1 the address is read from the hardware.
 *	If 'freq' is -1 the frequency is set to 56 clock cycles.
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
int
acc_i2c_reset(unsigned char busnum, short adr, char freq)
#else
int
gfx_i2c_reset(unsigned char busnum, short adr, char freq)
#endif
{
    if ((busnum != 1) && (busnum != 2))
        return GFX_STATUS_BAD_PARAMETER;
    acc_i2c_config(busnum, adr, freq);
    if (base_address_array[busnum] == 0)
        return GFX_STATUS_ERROR;
    acc_i2c_reset_bus(busnum);
    return GFX_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * gfx_i2c_select_gpio
 *
 * This routine selects which GPIO pins to use.
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
int
acc_i2c_select_gpio(int clock, int data)
#else
int
gfx_i2c_select_gpio(int clock, int data)
#endif
{
    /* THIS ROUTINE DOES NOT APPLY TO THE ACCESS.bus IMPLEMENTATION. */

    return (GFX_STATUS_OK);
}

/*---------------------------------------------------------------------------
 *	gfx_i2c_write
 *
 *	This routine writes data to the specified I2C address.
 *  busnum - ACCESS.bus number (1 or 2).
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
int
acc_i2c_write(unsigned char busnum, unsigned char chipadr,
              unsigned char subadr, unsigned char bytes, unsigned char *data)
#else
int
gfx_i2c_write(unsigned char busnum, unsigned char chipadr,
              unsigned char subadr, unsigned char bytes, unsigned char *data)
#endif
{
    int loop = 0;

    if ((busnum != 1) && (busnum != 2))
        return GFX_STATUS_BAD_PARAMETER;

    /* REQUEST MASTER */

    if (!acc_i2c_request_master(busnum))
        return (GFX_STATUS_ERROR);

    /* WRITE ADDRESS COMMAND */

    acc_i2c_ack(busnum, 1, 0);
    acc_i2c_stall_after_start(busnum, 1);
    acc_i2c_send_address(busnum, (unsigned char) (chipadr & 0xFE));
    acc_i2c_stall_after_start(busnum, 0);
    if (!acc_i2c_ack(busnum, 0, 0))
        return (GFX_STATUS_ERROR);

    /* WRITE COMMAND */

    acc_i2c_write_byte(busnum, subadr);
    if (!acc_i2c_ack(busnum, 0, 0))
        return (GFX_STATUS_ERROR);

    /* WRITE DATA */

    for (loop = 0; loop < bytes; loop++) {
        acc_i2c_write_byte(busnum, *data);
        if (loop < (bytes - 1))
            data += sizeof(unsigned char);
        if (!acc_i2c_ack(busnum, 0, 0))
            return (GFX_STATUS_ERROR);
    }
    data -= (bytes - 1);
    acc_i2c_stop(busnum);

    return GFX_STATUS_OK;
}

/*---------------------------------------------------------------------------
 *	gfx_i2c_read
 *
 *	This routine reads data from the specified I2C address.
 *  busnum - ACCESS.bus number (1 or 2).
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
int
acc_i2c_read(unsigned char busnum, unsigned char chipadr,
             unsigned char subadr, unsigned char bytes, unsigned char *data)
#else
int
gfx_i2c_read(unsigned char busnum, unsigned char chipadr,
             unsigned char subadr, unsigned char bytes, unsigned char *data)
#endif
{
    unsigned char bytesRead;

    if ((busnum != 1) && (busnum != 2))
        return GFX_STATUS_BAD_PARAMETER;

    if (bytes == 0)
        return GFX_STATUS_OK;

    /* REQUEST MASTER */

    if (!acc_i2c_request_master(busnum))
        return (GFX_STATUS_ERROR);

    /* WRITE ADDRESS COMMAND */

    acc_i2c_ack(busnum, 1, 0);
    acc_i2c_stall_after_start(busnum, 1);
    acc_i2c_send_address(busnum, (unsigned char) (chipadr & 0xFE));
    acc_i2c_stall_after_start(busnum, 0);
    if (!acc_i2c_ack(busnum, 0, 0))
        return (GFX_STATUS_ERROR);

    /* WRITE COMMAND */

    acc_i2c_write_byte(busnum, subadr);
    if (!acc_i2c_ack(busnum, 0, 0))
        return (GFX_STATUS_ERROR);

    /* START THE READ */

    acc_i2c_start(busnum);

    /* WRITE ADDRESS COMMAND */

    acc_i2c_ack(busnum, 1, 1);
    acc_i2c_stall_after_start(busnum, 1);
    acc_i2c_send_address(busnum, (unsigned char) (chipadr | 0x01));

    /* IF LAST BYTE */

    if (bytes == 1)
        acc_i2c_ack(busnum, 1, 1);
    else
        acc_i2c_ack(busnum, 1, 0);

    acc_i2c_stall_after_start(busnum, 0);

    if (!acc_i2c_ack(busnum, 0, 0))
        return (GFX_STATUS_ERROR);

    /* READ COMMAND */

    for (bytesRead = 0; bytesRead < bytes; bytesRead += 1) {
        if (bytesRead < (bytes - 2)) {
            data[bytesRead] = acc_i2c_read_byte(busnum, 0);
            acc_i2c_ack(busnum, 1, 0);
        }
        else if (bytesRead == (bytes - 2)) {    /* TWO BYTES LEFT */
            acc_i2c_ack(busnum, 1, 1);
            data[bytesRead] = acc_i2c_read_byte(busnum, 0);
            acc_i2c_ack(busnum, 1, 1);
        }
        else {                  /* LAST BYTE */

            data[bytesRead] = acc_i2c_read_byte(busnum, 1);
            acc_i2c_stop(busnum);
        }

        /* WHILE NOT LAST BYTE */

        if ((!(bytesRead == (bytes - 1))) && (!acc_i2c_ack(busnum, 0, 0)))
            return (bytesRead);
    }

    return GFX_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * gfx_i2c_init
 *
 * This routine initializes the use of the ACCESS.BUS.
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
int
acc_i2c_init(void)
#else
int
gfx_i2c_init(void)
#endif
{
    /* ### ADD ### THIS ROUTINE IS NOT YET IMPLEMENTED FOR ACCESS.bus */
    return (GFX_STATUS_OK);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_cleanup
 *
 * This routine ends the use of the ACCESS.BUS.
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
void
acc_i2c_cleanup(void)
#else
void
gfx_i2c_cleanup(void)
#endif
{
    /* ### ADD ### THIS ROUTINE IS NOT YET IMPLEMENTED FOR ACCESS.bus */
}

/*--------------------------------------------------------*/
/*  LOCAL ROUTINES SPECIFIC TO ACCESS.bus IMPLEMENTATION  */
/*--------------------------------------------------------*/

/*---------------------------------------------------------------------------
 * acc_i2c_reset_bus
 *
 * This routine resets the I2C bus.
 *---------------------------------------------------------------------------
 */
void
acc_i2c_reset_bus(unsigned char busnum)
{
    unsigned char reg;
    unsigned short bus_base_address = base_address_array[busnum];

    /* Disable the ACCESS.bus device and */
    /* Configure the SCL frequency */
    OUTB((unsigned short) (bus_base_address + ACBCTL2),
         (unsigned char) (Freq & 0xFE));

    /* Configure no interrupt mode (polling) and */
    /* Disable global call address */
    OUTB((unsigned short) (bus_base_address + ACBCTL1), 0x0);

    /* Disable slave address */
    OUTB((unsigned short) (bus_base_address + ACBADDR), 0x0);

    /* Enable the ACCESS.bus device */
    reg = INB((unsigned short) (bus_base_address + ACBCTL2));
    reg |= 0x01;
    OUTB((unsigned short) (bus_base_address + ACBCTL2), reg);

    /* Issue STOP event */

    acc_i2c_stop(busnum);

    /* Clear NEGACK, STASTR and BER bits */
    OUTB((unsigned short) (bus_base_address + ACBST), 0x38);

    /* Clear BB (BUS BUSY) bit */
    reg = INB((unsigned short) (bus_base_address + ACBCST));
    reg |= 0x02;
    OUTB((unsigned short) (bus_base_address + ACBCST), reg);
}

/*---------------------------------------------------------------------------
 * acc_i2c_start
 *
 * This routine starts a transfer on the I2C bus.
 *---------------------------------------------------------------------------
 */
void
acc_i2c_start(unsigned char busnum)
{
    unsigned char reg;
    unsigned short bus_base_address = base_address_array[busnum];

    reg = INB((unsigned short) (bus_base_address + ACBCTL1));
    reg |= 0x01;
    OUTB((unsigned short) (bus_base_address + ACBCTL1), reg);
}

/*---------------------------------------------------------------------------
 * acc_i2c_stop
 *
 * This routine stops a transfer on the I2C bus.
 *---------------------------------------------------------------------------
 */
void
acc_i2c_stop(unsigned char busnum)
{
    unsigned char reg;
    unsigned short bus_base_address = base_address_array[busnum];

    reg = INB((unsigned short) (bus_base_address + ACBCTL1));
    reg |= 0x02;
    OUTB((unsigned short) (bus_base_address + ACBCTL1), reg);
}

/*---------------------------------------------------------------------------
 * acc_i2c_abort_data
 *---------------------------------------------------------------------------
 */
void
acc_i2c_abort_data(unsigned char busnum)
{
    unsigned char reg;
    unsigned short bus_base_address = base_address_array[busnum];

    acc_i2c_stop(busnum);
    reg = INB((unsigned short) (bus_base_address + ACBCTL1));
    reg |= 0x10;
    OUTB((unsigned short) (bus_base_address + ACBCTL1), reg);
}

/*---------------------------------------------------------------------------
 * acc_i2c_bus_recovery
 *---------------------------------------------------------------------------
 */
void
acc_i2c_bus_recovery(unsigned char busnum)
{
    acc_i2c_abort_data(busnum);
    acc_i2c_reset_bus(busnum);
}

/*---------------------------------------------------------------------------
 * acc_i2c_stall_after_start
 *---------------------------------------------------------------------------
 */
void
acc_i2c_stall_after_start(unsigned char busnum, int state)
{
    unsigned char reg;
    unsigned short bus_base_address = base_address_array[busnum];

    reg = INB((unsigned short) (bus_base_address + ACBCTL1));
    if (state)
        reg |= 0x80;
    else
        reg &= 0x7F;
    OUTB((unsigned short) (bus_base_address + ACBCTL1), reg);

    if (!state) {
        reg = INB((unsigned short) (bus_base_address + ACBST));
        reg |= 0x08;
        OUTB((unsigned short) (bus_base_address + ACBST), reg);
    }
}

/*---------------------------------------------------------------------------
 * acc_i2c_send_address
 *---------------------------------------------------------------------------
 */
void
acc_i2c_send_address(unsigned char busnum, unsigned char cData)
{
    unsigned char reg;
    unsigned short bus_base_address = base_address_array[busnum];
    unsigned long timeout = 0;

    /* WRITE THE DATA */

    OUTB((unsigned short) (bus_base_address + ACBSDA), cData);
    while (1) {
        reg = INB((unsigned short) (bus_base_address + ACBST));
        if ((reg & 0x38) != 0)  /* check STASTR, BER and NEGACK */
            break;
        if (timeout++ == ACC_I2C_TIMEOUT) {
            acc_i2c_bus_recovery(busnum);
            return;
        }
    }

    /* CHECK FOR BUS ERROR */

    if (reg & 0x20) {
        acc_i2c_bus_recovery(busnum);
        return;
    }

    /* CHECK NEGATIVE ACKNOWLEDGE */

    if (reg & 0x10) {
        acc_i2c_abort_data(busnum);
        return;
    }

}

/*---------------------------------------------------------------------------
 * acc_i2c_ack
 *
 * This routine looks for acknowledge on the I2C bus.
 *---------------------------------------------------------------------------
 */
int
acc_i2c_ack(unsigned char busnum, int fPut, int negAck)
{
    unsigned char reg;
    unsigned short bus_base_address = base_address_array[busnum];
    unsigned long timeout = 0;

    if (fPut) {                 /* read operation */
        if (!negAck) {
            /* Push Ack onto I2C bus */
            reg = INB((unsigned short) (bus_base_address + ACBCTL1));
            reg &= 0xE7;
            OUTB((unsigned short) (bus_base_address + ACBCTL1), reg);
        }
        else {
            /* Push negAck onto I2C bus */
            reg = INB((unsigned short) (bus_base_address + ACBCTL1));
            reg |= 0x10;
            OUTB((unsigned short) (bus_base_address + ACBCTL1), reg);
        }
    }
    else {                      /* write operation */
        /* Receive Ack from I2C bus */
        while (1) {
            reg = INB((unsigned short) (bus_base_address + ACBST));
            if ((reg & 0x70) != 0)      /* check SDAST, BER and NEGACK */
                break;
            if (timeout++ == ACC_I2C_TIMEOUT) {
                acc_i2c_bus_recovery(busnum);
                return (0);
            }
        }

        /* CHECK FOR BUS ERROR */

        if (reg & 0x20) {
            acc_i2c_bus_recovery(busnum);
            return (0);
        }

        /* CHECK NEGATIVE ACKNOWLEDGE */

        if (reg & 0x10) {
            acc_i2c_abort_data(busnum);
            return (0);
        }
    }
    return (1);
}

/*---------------------------------------------------------------------------
 * acc_i2c_stop_clock
 *
 * This routine stops the ACCESS.bus clock.
 *---------------------------------------------------------------------------
 */
void
acc_i2c_stop_clock(unsigned char busnum)
{
    unsigned char reg;
    unsigned short bus_base_address = base_address_array[busnum];

    reg = INB((unsigned short) (bus_base_address + ACBCTL2));
    reg &= ~0x01;
    OUTB((unsigned short) (bus_base_address + ACBCTL2), reg);
}

/*---------------------------------------------------------------------------
 * acc_i2c_activate_clock
 *
 * This routine activates the ACCESS.bus clock.
 *---------------------------------------------------------------------------
 */
void
acc_i2c_activate_clock(unsigned char busnum)
{
    unsigned char reg;
    unsigned short bus_base_address = base_address_array[busnum];

    reg = INB((unsigned short) (bus_base_address + ACBCTL2));
    reg |= 0x01;
    OUTB((unsigned short) (bus_base_address + ACBCTL2), reg);
}

/*---------------------------------------------------------------------------
 * acc_i2c_write_byte
 *
 * This routine writes a byte to the I2C bus
 *---------------------------------------------------------------------------
 */
void
acc_i2c_write_byte(unsigned char busnum, unsigned char cData)
{
    unsigned char reg;
    unsigned short bus_base_address = base_address_array[busnum];
    unsigned long timeout = 0;

    while (1) {
        reg = INB((unsigned short) (bus_base_address + ACBST));
        if (reg & 0x70)
            break;
        if (timeout++ == ACC_I2C_TIMEOUT) {
            acc_i2c_bus_recovery(busnum);
            return;
        }
    }

    /* CHECK FOR BUS ERROR */

    if (reg & 0x20) {
        acc_i2c_bus_recovery(busnum);
        return;
    }

    /* CHECK NEGATIVE ACKNOWLEDGE */

    if (reg & 0x10) {
        acc_i2c_abort_data(busnum);
        return;
    }

    /* WRITE THE DATA */

    OUTB((unsigned short) (bus_base_address + ACBSDA), cData);
}

/*---------------------------------------------------------------------------
 * acc_i2c_read_byte
 *
 * This routine reads a byte from the I2C bus
 *---------------------------------------------------------------------------
 */
unsigned char
acc_i2c_read_byte(unsigned char busnum, int last_byte)
{
    unsigned char cData, reg;
    unsigned short bus_base_address = base_address_array[busnum];
    unsigned long timeout = 0;

    while (1) {
        reg = INB((unsigned short) (bus_base_address + ACBST));
        if (reg & 0x60)
            break;
        if (timeout++ == ACC_I2C_TIMEOUT) {
            acc_i2c_bus_recovery(busnum);
            return (0xEF);
        }
    }

    /* CHECK FOR BUS ERROR */

    if (reg & 0x20) {
        acc_i2c_bus_recovery(busnum);
        return (0xEE);
    }

    /* READ DATA */
    if (last_byte)
        acc_i2c_stop_clock(busnum);
    cData = INB((unsigned short) (bus_base_address + ACBSDA));
    if (last_byte)
        acc_i2c_activate_clock(busnum);

    return (cData);
}

/*---------------------------------------------------------------------------
 * acc_i2c_request_master
 *---------------------------------------------------------------------------
 */
int
acc_i2c_request_master(unsigned char busnum)
{
    unsigned char reg;
    unsigned short bus_base_address = base_address_array[busnum];
    unsigned long timeout = 0;

    acc_i2c_start(busnum);
    while (1) {
        reg = INB((unsigned short) (bus_base_address + ACBST));
        if (reg & 0x60)
            break;
        if (timeout++ == ACC_I2C_TIMEOUT) {
            acc_i2c_bus_recovery(busnum);
            return (0);
        }
    }

    /* CHECK FOR BUS ERROR */

    if (reg & 0x20) {
        acc_i2c_abort_data(busnum);
        return (0);
    }

    /* CHECK NEGATIVE ACKNOWLEDGE */

    if (reg & 0x10) {
        acc_i2c_abort_data(busnum);
        return (0);
    }
    return (1);
}

/*--------------------------------------------------------*/
/*  LOCAL ROUTINES SPECIFIC TO ACCESS.bus INITIALIZATION  */
/*--------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * acc_i2c_config
 *
 * This routine configures the I2C bus
 *----------------------------------------------------------------------------
 */
void
acc_i2c_config(unsigned char busnum, short adr, char freq)
{
    base_address_array[busnum] = acc_i2c_set_base_address(busnum, adr);
    Freq = acc_i2c_set_freq(busnum, freq);
}

/*----------------------------------------------------------------------------
 * acc_i2c_set_freq
 *
 * This routine sets the frequency of the I2C bus
 *----------------------------------------------------------------------------
 */
char
acc_i2c_set_freq(unsigned char busnum, char freq)
{
    unsigned short bus_base_address = base_address_array[busnum];

    OUTB((unsigned short) (bus_base_address + ACBCTL2), 0x0);

    if (freq == -1)
        freq = 0x71;
    else {
        freq = freq << 1;
        freq |= 0x01;
    }

    OUTB((unsigned short) (bus_base_address + ACBCTL2), freq);
    return (freq);
}

/*---------------------------------------------------------------------------
 * acc_i2c_set_base_address
 *
 * This routine sets the base address of the I2C bus
 *---------------------------------------------------------------------------
 */
unsigned short
acc_i2c_set_base_address(unsigned char busnum, short adr)
{
    unsigned short ab_base_addr;

    /* Get Super I/O Index and Data registers */
    if (!sio_set_index_data_reg())
        return (0);

    /* Configure LDN to current ACB */
    if (busnum == 1)
        sio_write_reg(LDN, ACB1_LDN);
    if (busnum == 2)
        sio_write_reg(LDN, ACB2_LDN);

    if (adr == -1) {
        /* Get ACCESS.bus base address */
        ab_base_addr = sio_read_reg(BASE_ADR_MSB_REG);
        ab_base_addr = ab_base_addr << 8;
        ab_base_addr |= sio_read_reg(BASE_ADR_LSB_REG);
        if (ab_base_addr != 0)
            return ab_base_addr;
        else
            adr = (busnum == 1 ? ACB1_BASE : ACB2_BASE);
    }

    /* Set ACCESS.bus base address */
    sio_write_reg(BASE_ADR_LSB_REG, (unsigned char) (adr & 0xFF));
    sio_write_reg(BASE_ADR_MSB_REG, (unsigned char) (adr >> 8));

    return adr;
}

/* END OF FILE */

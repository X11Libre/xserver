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
 * the GPIO pins of the CS5530.
 * */

/* STATIC VARIABLES TO STORE WHAT GPIO PINS TO USE */

int gpio_clock = 0;
int gpio_data = 0;

static int g_initialized = 0;

#define	I2CWRITE		0x00    /* Write address         */
#define	I2CREAD			0x01    /* Read address          */

#define	I2CACK			0x00    /* Ack value             */
#define	I2CNACK			0x01    /* Not - ack value       */

#define		CS5530_ID	(0x80000000 | (0x00<<16) | (0x12<<11) | (0<<8) | 0x00)
#define		CS5530_GPIO	(0x80000000 | (0x00<<16) | (0x12<<11) | (0<<8) | 0x90)
#define		SDA		0x0800
#define		SCL		0x0400
#define		SDADIR	0x0008
#define		SCLDIR	0x0004

int I2C_init(void);
void I2C_cleanup(void);

int I2C_Read(unsigned char address, unsigned int reg, unsigned long *p_value,
             unsigned int bytes);
int I2C_Write(unsigned char address, unsigned int reg, unsigned long value,
              unsigned int bytes);
int I2CAL_init(void);
void I2CAL_cleanup(void);

void I2CAL_output_clock(int state);
void I2CAL_output_data(int state);
unsigned char I2CAL_input_data(void);

void I2CAL_set_data_for_input(void);
void I2CAL_set_data_for_output(void);

void SendI2CStart(void);
void SendI2CData(unsigned char inData);

unsigned char ReceiveI2CAck(void);
void SendI2CStop(void);
void SendI2CNack(void);
void SendI2CAck(void);
unsigned char ReceiveI2CData(void);

/* ### ADD ### ANY LOCAL ROUTINE DEFINITIONS SPECIFIC TO GPIO */

/*---------------------------------------------------------------------------
 * gfx_i2c_reset
 *
 * This routine resets the I2C bus.
 *---------------------------------------------------------------------------
 */

#if GFX_I2C_DYNAMIC
int
gpio_i2c_reset(unsigned char busnum, short adr, char freq)
#else
int
gfx_i2c_reset(unsigned char busnum, short adr, char freq)
#endif
{
    /* ### ADD ### Any code needed to reset the state of the GPIOs. */
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
gpio_i2c_select_gpio(int clock, int data)
#else
int
gfx_i2c_select_gpio(int clock, int data)
#endif
{
    gpio_clock = clock;
    gpio_data = data;
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_write
 *
 * This routine writes data to the specified I2C address.
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
int
gpio_i2c_write(unsigned char busnum, unsigned char address, unsigned char reg,
               unsigned char bytes, unsigned char *value)
#else
int
gfx_i2c_write(unsigned char busnum, unsigned char address, unsigned char reg,
              unsigned char bytes, unsigned char *value)
#endif
{
    /* ### ADD ### CODE TO WRITE BYTE TO I2B BUS */

    int restart_count = 0;

    while (restart_count++ < 5) {
        /* set the access pointer register. */
        /* The address is shifted left by one to make room for Read/Write 
         * bit */
        SendI2CStart();
        SendI2CData((char) ((address << 1) | I2CWRITE));
        if (!ReceiveI2CAck()) {
            SendI2CStop();
            gfx_delay_milliseconds(10);
            continue;
        }
        SendI2CData((unsigned char) reg);
        if (!ReceiveI2CAck()) {
            SendI2CStop();
            gfx_delay_milliseconds(10);
            continue;
        }

        /* write the first byte */
        SendI2CData(*value);
        if (!ReceiveI2CAck()) {
            SendI2CStop();
            gfx_delay_milliseconds(10);
            continue;
        }

        /* write the second byte. */
        if (bytes == 2) {
            SendI2CData(*(value + 1));
            if (!ReceiveI2CAck()) {
                SendI2CStop();
                gfx_delay_milliseconds(10);
                continue;
            }
        }

        /* done. */
        SendI2CStop();

        return 0;
    }

    return (0);

}

/*---------------------------------------------------------------------------
 * gfx_i2c_read
 *
 * This routine reads data from the specified I2C address.
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
int
gpio_i2c_read(unsigned char busnum, unsigned char address, unsigned char reg,
              unsigned char bytes, unsigned char *p_value)
#else
int
gfx_i2c_read(unsigned char busnum, unsigned char address, unsigned char reg,
             unsigned char bytes, unsigned char *p_value)
#endif
{
    /* ### ADD ### CODE TO WRITE BYTE TO I2B BUS */
    /* For now return clock and data pins */

    int restart_count = 0;

    if (!p_value)
        return (1);

    while (restart_count++ < 5) {
        /* set the access pointer register. */
        /* The address is shifted left by one to make room for Read/Write 
         * bit  */
        SendI2CStart();
        SendI2CData((char) ((address << 1) | I2CWRITE));
        if (!ReceiveI2CAck()) {
            SendI2CStop();
            gfx_delay_milliseconds(10);
            continue;
        }
        SendI2CData((unsigned char) (reg & 0xFF));
        SendI2CNack();

        /* read the first data byte. */
        SendI2CStart();
        SendI2CData((char) ((address << 1) | I2CREAD));
        if (!ReceiveI2CAck()) {
            SendI2CStop();
            gfx_delay_milliseconds(10);
            continue;
        }
        *p_value = ReceiveI2CData();

        /* read the second byte. */
        if (bytes == 2) {
            SendI2CAck();
            *(p_value + 1) = ReceiveI2CData();
        }

        /* done. */
        SendI2CNack();
        SendI2CStop();

        return 0;
    }

    return (1);
}

/* Added i2c/gpio code to test fs451 chip. */

/*
 * ----------------------------------------------------------------------
 *
 *	void SendI2CStart(void)
 *	
 *	Sends an I2C start signal on the bus.
 *	
 *----------------------------------------------------------------------
 */
void
SendI2CStart(void)
{
    I2CAL_output_data(1);
    I2CAL_output_clock(1);
    I2CAL_output_data(0);
    I2CAL_output_clock(0);
}

/*
 *----------------------------------------------------------------------
 *
 *	void SendI2CStop(void)
 *	
 *	Sends an I2C stop signal on the bus.
 *	
 *----------------------------------------------------------------------
 */
void
SendI2CStop(void)
{
    I2CAL_output_data(0);
    I2CAL_output_clock(1);
    I2CAL_output_data(1);
}

/*
 *----------------------------------------------------------------------
 *
 *	void SendI2CAck(void)
 *	
 *	Sends the Ack signal on the I2C bus.
 *	
 *----------------------------------------------------------------------
 */
void
SendI2CAck(void)
{
    I2CAL_output_data(0);
    I2CAL_output_clock(1);
    I2CAL_output_clock(0);
}

/*
 *----------------------------------------------------------------------
 *
 *	void SendI2CNack(void)
 *	
 *	Sends the Nt-Ack signal on the I2C bus.
 *	
 *----------------------------------------------------------------------
 */
void
SendI2CNack(void)
{
    I2CAL_output_data(1);
    I2CAL_output_clock(1);
    I2CAL_output_clock(0);
}

/*
 *----------------------------------------------------------------------
 *
 *	UInt8 SendI2CData( UInt8 inData )
 *	
 *	Sends a byte of data on the I2C bus and returns the TRUE if the slave 
 *	ACK'dthe data.
 *	
 *	Input:	inData			- the byte of data to send          
 *	Output:	(return)		- TRUE (1) if ACK was received, FALSE (0) if not
 *	
 *----------------------------------------------------------------------
 */
void
SendI2CData(unsigned char inData)
{
    unsigned char bit;

    /* Send all 8 bits of data byte, MSB to LSB */
    for (bit = 0x80; bit != 0; bit >>= 1) {
        if (inData & bit)
            I2CAL_output_data(1);
        else
            I2CAL_output_data(0);

        I2CAL_output_clock(1);
        I2CAL_output_clock(0);
    }
}

/*
 *----------------------------------------------------------------------
 *
 *	UInt8 ReceiveI2CAck(  )
 *	
 *	Receives the Ack (or Nack) from the slave. 
 *	
 *	Output:	(return) - TRUE (1) if ACK was received, FALSE (0) if not
 *
 *----------------------------------------------------------------------
 */
unsigned char
ReceiveI2CAck(void)
{
    unsigned char bit;

    /* Test for Ack/Nack */
    I2CAL_set_data_for_input();
    I2CAL_output_data(1);
    I2CAL_output_clock(1);
    bit = I2CAL_input_data();
    I2CAL_output_clock(0);
    I2CAL_set_data_for_output();
    return !bit;
}

/*	
 *----------------------------------------------------------------------
 *
 *	unsigned char ReceiveI2CData(void)
 *
 *	Receives a byte of data from the I2C bus.
 *
 *	Output:	(return) - The data byte recehved from the bus
 *
 *----------------------------------------------------------------------
 */
unsigned char
ReceiveI2CData(void)
{
    unsigned char data = 0;
    unsigned char x;

    /* make sure the data line is released */
    I2CAL_set_data_for_input();
    I2CAL_output_data(1);

    /* shift in the data */
    for (x = 0; x < 8; x++) {
        /* shift the data left   */
        I2CAL_output_clock(1);
        data <<= 1;
        data |= I2CAL_input_data();
        I2CAL_output_clock(0);
    }

    I2CAL_set_data_for_output();
    I2CAL_output_data(1);
    return data;
}

/*
 *----------------------------------------------------------------------
 *
 *	void I2C_init(void)
 *
 *	This routine initializes the I2C interface. Clients of the I2C.c
 *	will call this routine before calling any other routine in the I2C.c
 *
 *----------------------------------------------------------------------
 */

#if GFX_I2C_DYNAMIC
int
gpio_i2c_init(void)
#else
int
gfx_i2c_init(void)
#endif
{
    int errc;

    /* init I2CAL */
    errc = I2CAL_init();
    if (errc)
        return errc;

    /* set the clock and data lines to the proper states */
    I2CAL_output_clock(1);
    I2CAL_output_data(1);
    I2CAL_set_data_for_output();

    SendI2CStart();
    SendI2CStop();
    SendI2CStop();

    g_initialized = 1;

    return 0;
}

/*
 *----------------------------------------------------------------------
 *	
 *	void I2C_cleanup(void)
 *
 *	This routine disables the I2C interface. Clients of the I2C.c will not
 *	call any other I2C routine after calling this routine.
 *	
 *----------------------------------------------------------------------
 */

#if GFX_I2C_DYNAMIC
void
gpio_i2c_cleanup(void)
#else
void
gfx_i2c_cleanup(void)
#endif
{
    if (g_initialized) {

        /* set the clock and data lines to a harmless state */
        I2CAL_output_clock(1);
        I2CAL_output_data(1);

        g_initialized = 0;
    }

    I2CAL_cleanup();
}

int
I2CAL_init(void)
{
    unsigned long l_reg;
    unsigned short reg;

    /* initialize the i2c port. */
    l_reg = gfx_pci_config_read(CS5530_GPIO);

    if (l_reg != 0x01001078)
        return 1;

    l_reg = gfx_pci_config_read(CS5530_GPIO);
    reg = (unsigned short) l_reg;

    /* both outputs, both high. */
    reg |= (SDADIR | SCLDIR | SDA | SCL);
    l_reg = reg;
    gfx_pci_config_write(CS5530_GPIO, l_reg);

    g_initialized = 1;

    return 0;
}

void
I2CAL_cleanup(void)
{
    if (g_initialized) {

        g_initialized = 0;
    }
}

/*
 *----------------------------------------------------------------------------
 *
 *	set the I2C clock line state
 *
 *----------------------------------------------------------------------------
 */
void
I2CAL_output_clock(int inState)
{
    unsigned short reg;
    unsigned long value;

    value = gfx_pci_config_read(CS5530_GPIO);
    reg = (unsigned short) value;

    if (inState) {              /* write a 1. */
        reg |= SCL;
    }
    else {                      /* write a 0. */
        reg &= ~SCL;
    }

    value = reg;
    gfx_pci_config_write(CS5530_GPIO, value);

    /* hold it for a minimum of 4.7us */
    gfx_delay_microseconds(5);
}

/*
 *----------------------------------------------------------------------------
 *
 *	set the I2C data line state
 *
 *----------------------------------------------------------------------------
 */
void
I2CAL_output_data(int inState)
{
    unsigned short reg;
    unsigned long value;

    value = gfx_pci_config_read(CS5530_GPIO);
    reg = (unsigned short) value;

    if (inState) {              /* write a 1. */
        reg |= SDA;
    }
    else {
        /* write a 0. */
        reg &= ~SDA;
    }
    value = reg;
    gfx_pci_config_write(CS5530_GPIO, value);

    /* 250 ns setup time */
    gfx_delay_microseconds(1);
}

/*
 *----------------------------------------------------------------------------
 *
 *	read the state of the data line
 *
 *----------------------------------------------------------------------------
 */
unsigned char
I2CAL_input_data(void)
{
    unsigned short reg;
    unsigned long value;

    value = gfx_pci_config_read(CS5530_GPIO);
    reg = (unsigned short) value;

    if (reg & SDA)
        return 1;
    else
        return 0;
}

/*
 *----------------------------------------------------------------------------
 *
 *	set the I2C data for input mode
 *
 *----------------------------------------------------------------------------
 */
void
I2CAL_set_data_for_input(void)
{
    unsigned short reg;
    unsigned long value;

    value = gfx_pci_config_read(CS5530_GPIO);
    reg = (unsigned short) value;

    reg &= ~SDADIR;

    value = reg;

    gfx_pci_config_write(CS5530_GPIO, value);
}

/*
 *----------------------------------------------------------------------------
 *
 *	set the I2C data for output mode
 *
 *----------------------------------------------------------------------------
 */
void
I2CAL_set_data_for_output(void)
{
    unsigned short reg;
    unsigned long value;

    value = gfx_pci_config_read(CS5530_GPIO);
    reg = (unsigned short) value;
    reg |= SDADIR;
    value = reg;

    gfx_pci_config_write(CS5530_GPIO, value);

}

/* END OF FILE */

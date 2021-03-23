#ifndef I2C_H_INCLUDED
#define I2C_H_INCLUDED

#include <linux/i2c.h>

// Global variables for I2C
#define I2C_BUS_AVAILABLE 1
#define SLAVE_DEVICE_NAME "MPU6050"
#define SLAVE_DEVICE_ADDRESS 0x86

static struct i2c_device_id my_id_table[] = {
	{SLAVE_DEVICE_NAME, 0},
	{},	//C90 requires termination with 0 entry...
}

MODULE_DEVICE_TABLE(i2c, my_id_table);

// Function prototypes
void I2C_read_special(unsigned char *buf, unsigned char cmd);

void I2C_read_data(unsigned char *outBuf, unsigned int len);

int I2C_write_data(unsigned char *buf, unsigned int len);

int initI2C(void);

void unregisterI2C(void);

#endif

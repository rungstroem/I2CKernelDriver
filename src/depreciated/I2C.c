#include "I2C.h"
#include <linux/i2c.h>
#include <linux/kernel.h>

static struct i2c_adapter *my_i2c_adapter = NULL;
static struct i2c_client *my_i2c_client = NULL;

// I2C struct for general device infoemation
// Supported devices
//static struct i2c_device_id my_id_table[] = {
//	{SLAVE_DEVICE_NAME, 0},
//	{},	//C90 requires termination with 0 entry...
//};

static struct i2c_driver my_i2c_driver = {
	.driver = {
		.name = SLAVE_DEVICE_NAME,
		.owner = THIS_MODULE,
	},
	.id_table = my_id_table,
};

//MODULE_DEVICE_TABLE(i2c,my_id_table);

// Hooks to manage devices
static struct i2c_board_info my_i2c_board_info = {
	I2C_BOARD_INFO(SLAVE_DEVICE_NAME, SLAVE_DEVICE_ADDRESS)
};

void I2C_read_special(unsigned char *buf, unsigned char cmd){
	*buf = (char)i2c_smbus_read_byte_data(my_i2c_client, cmd);
}

void I2C_read_data(unsigned char *outBuf, unsigned int len){
	while(i2c_master_recv(my_i2c_client, outBuf, len) < 1){
		//Wait for i2c_master_recv to return a positive value
	}
	return;
}

int I2C_write_data(unsigned char *buf, unsigned int len){
	int ret = -1;
	ret = i2c_master_send(my_i2c_client, buf, len);
	return ret;
}

int initI2C(void){
	int ret = -1;

	// Create adaptor to get access to I2C us
	my_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);
	if(my_i2c_adapter != NULL){
		my_i2c_client = i2c_new_device(my_i2c_adapter, &my_i2c_board_info);
		if(my_i2c_client != NULL){
			if(i2c_add_driver(&my_i2c_driver) != -1){
				ret = 0;
			}else{
				printk(KERN_INFO "Can't add i2c driver\n");
			}
		}
		i2c_put_adapter(my_i2c_adapter);
	}
	return ret;
}

void unregisterI2C(void){
	i2c_unregister_device(my_i2c_client);
	printk(KERN_INFO "I2C client removed");
	i2c_del_adapter(my_i2c_adapter);
	printk(KERN_INFO "I2C adaptor deleted");
	i2c_del_driver(&my_i2c_driver);
	printk(KERN_INFO "I2C driver deleted");
}

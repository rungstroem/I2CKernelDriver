#include <linux/init.h>
#include <linux/module.h>	// Needed by all modules
#include <linux/kernel.h>	// Needed for KERN_INFO
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/delay.h>	// Used for msleep() command
#include <linux/of.h>
#include "cmdInterpreter.h"

//######################################################
// Author and license
MODULE_AUTHOR("Kenneth R Larsen");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A driver for the MPU60x0 series I2C enabled IMU's");
//#####################################################

#define DEVICE_NAME "I2CKernelModule"
#define DRIVER_NAME "I2CDriver"

//#####################################################

//#####################################################
// Global variables for device_file - Reading and writing to /dev/I2CDriver
#define BUF_LEN 80

static struct cdev c_dev;
static dev_t dev;
static struct class *deviceFileClass;
int init_result;
int deviceOpen = 0;
char Message[BUF_LEN];
char *Message_Ptr;
bool cmdIdentified;
bool writeOpened = false;
//#####################################################

//#####################################################
// Global variables for I2C
#define I2C_BUS 1
#define SLAVE_DEVICE_NAME "MPU6050"
#define SLAVE_DEVICE_ADDRESS 0x68

static struct i2c_adapter * my_i2c_adapter = NULL;
static struct i2c_client * my_i2c_client = NULL;

//#####################################################
//I2C structs for general device information 
//Supported devices
static struct i2c_device_id MPU_id_table[] = {	//i2c-core matches the name in the struct and passes it to probe
	{ SLAVE_DEVICE_NAME, 0},
	{},	//Has to be terminated with a 0 entry - some kernel querk...
};
MODULE_DEVICE_TABLE(i2c, MPU_id_table);

static struct of_device_id MPU_of_match[] ={	//When an I2C device is instantiated with OF device tree, its compatible property is matches (format:"manufacurer, model") and the "model" is matched against the struct i2c_device_id - which also passes the element to probe
	{.compatible ="Sparkfun MPU6050",},
	{}
};
MODULE_DEVICE_TABLE(of, MPU_of_match);

// Prototypes for function to initialize and remove driver
int MPUProbe(struct i2c_client *client, const struct i2c_device_id *id);
int MPURemove(struct i2c_client *client);


// Hooks to manage devices
static struct i2c_driver my_i2c_driver = {
	.driver = {
		.name = SLAVE_DEVICE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = MPU_of_match,
	},
	.id_table = MPU_id_table,
	.probe = MPUProbe,
	.remove = MPURemove,

};

static struct i2c_board_info my_i2c_board_info = {
	I2C_BOARD_INFO(SLAVE_DEVICE_NAME, SLAVE_DEVICE_ADDRESS)
};

// ###############################################################################################
// I2C read and write commands
void I2C_read_special(unsigned char *buf, unsigned char cmd){
	*buf = (char)i2c_smbus_read_byte_data(my_i2c_client, 0x75);
}

void I2C_read_data(unsigned char *outBuf, unsigned int len){
	i2c_master_recv(my_i2c_client, outBuf, len);
	return;
}

int I2C_write_data(unsigned char *buf, unsigned int len){
	int ret = -1;
	ret = i2c_master_send(my_i2c_client, buf, len);
	return ret;
}

// Called when reading from the device [cat /dev/I2CDriver]
static ssize_t dev_read(struct file *filep, char *userBuffer, size_t len, loff_t *offset){	//Len is the size of the user buffer, loff_t is the index in the user buffer
	// C90 requires declaration before code
	int bytesRead = 0;
	unsigned char data = 0x00;
	unsigned char *D;
	char outMessage[2] = {0x00};
	
	//Print for debugging
	printk(KERN_INFO "Read from device Entered");
	if(writeOpened == true){
		if(cmdIdentified){
			D = &data;
			I2C_read_data(D,1);
			if(data == 0x00){	//This has to be removed in the final version
				outMessage[0] = '0';
			}else{
				outMessage[0] = data;
			}
			Message_Ptr = outMessage;
		}else{
			strcpy(Message, "command not identified");
			Message_Ptr = Message;
		}
	
		// If the pointer is 0 then no message was read	
		if(*Message_Ptr == 0){
			return -1;
		}

		//Print message from I2C to user
		while(len && *Message_Ptr){
			put_user(*(Message_Ptr++), userBuffer++);
			len--;
			bytesRead++;
		}
		//Set to false so write command has to be executed again before read can be executed
		writeOpened = false;
	}

	return bytesRead;	
}

// This function is malfunctioning...
int handle_command(char *inMessage, int len){
	int i;
	int cmdDataSeperator = 0;
	unsigned char reg;
	unsigned char *C;
	char cmd[10] = {0x00};
	unsigned char data[10] = {0x00};
	int dataRead = 0;

	printk(KERN_INFO "Just before loop");
	// Command data seperation
	for(i=0; i<len; i++){
		if(*(inMessage + i) == '\n') break;
		if(*(inMessage + i) == ' '){ 
			cmdDataSeperator = 1;
			continue;
		}
		if(cmdDataSeperator == 0){
			cmd[i] = *(inMessage + i);
		}
		if(cmdDataSeperator == 1){
			data[dataRead] = *(inMessage + i);
			dataRead++;
		}
	}
	Message[1] = 0x00;
	printk(KERN_INFO "Just after loop");
	// Convert command to register value
	reg = registerConverterMPU(cmd);
	if(reg == 0x00){
		cmdIdentified = false;
		printk(KERN_INFO "Command not identified");
		return -1;
	}else{
		cmdIdentified = true;
		C = &reg;
		if(dataRead < 1){
			Message[0] = reg;
			I2C_write_data(C,1);
		}else{
			I2C_write_data(C,1);	//Write register
			I2C_write_data(data,dataRead);	//Write data up to 10 char
			//Message[0] = reg;
			//Message[1] = data[0];
		}
	}
	return 0;
}

// Called when writing to the device [echo > "command" /dev/I2CDriver]
static ssize_t dev_write(struct file *filep, const char *userBuffer, size_t len, loff_t *offset){
	// C90 requires declaration before code.
	int i;
	char inMessage[20] = {0x00};
	// Print for debugging
	printk(KERN_INFO "Write to device Entered");
	
	// Get message from userspace
	for(i = 0; i < len && i < BUF_LEN; i++){
		get_user(inMessage[i], userBuffer +i);		// Echo inserts \n at the end!
	}
	handle_command(inMessage, i);
	// Attempt to handle only read data once a command is written
	writeOpened = true;
	return i;
}

// I2C init and remove functions prototypes
int initI2C(void);
void unregisterI2C(void);

// Device file init and remove function prototypes
int initDeviceFile(void);
void unregisterDeviceFile(void);

// Runs when module is opened
static int dev_open(struct inode *inodep, struct file *filep){
	if(deviceOpen) return -EBUSY;
	deviceOpen++;
	Message_Ptr = Message;
	try_module_get(THIS_MODULE);
	printk(KERN_INFO "I2CKernelModule opened");
	return 0;
}
// Runs when module is closed
static int dev_release(struct inode *inodep, struct file *filep){
	deviceOpen--;
	module_put(THIS_MODULE);
	printk(KERN_INFO "I2CKernelModule closed");
	return 0;
}

static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg){
	// Empty for now
}

static int dev_uevent(struct device *device, struct kobj_uevent_env *env){
	add_uevent_var(env, "DEVMODE=%#o", 0666);
	return 0;
}

static struct file_operations fops={
	.owner = THIS_MODULE,
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
	.unlocked_ioctl = dev_ioctl,
};

// Runs when module is inserted in kernel (insmod)
int init_module(void){	
	// Initialize I2C adaptor and client
	if(initI2C() < 0){
		return -1;
	}
	// Initializes the device file
	if(initDeviceFile() < 0){
		return -1;
	}
	// This retuns if init executes okay. 
	printk(KERN_INFO "I2CKernelModule inserted\n");
	return 0;
}

// Runs when module is removed from kernel (rmmod)
void cleanup_module(void){
	// Used to release the I2C interface
	unregisterI2C();

	unregisterDeviceFile();

	printk(KERN_INFO "I2CKernelModule removed\n");
}

int MPUProbe(struct i2c_client *client, const struct i2c_device_id *id){
	// Stuff in here gets called when the i2c_add_driver is called
	unsigned char buf[2] = {0x6b, 0x80};
	unsigned char *B;
	B = buf;
	I2C_write_data(B, 2);	// reset device
	
	msleep(5);	
	
	buf[0] = 0x6b, buf[1] = 0x00;
	B = buf;
	I2C_write_data(B, 2);	// Set 20MHz internal clock source
	
	msleep(5);	
	
	buf[0] = 0x6c, buf[1] = 0x3f;
	B = buf;
	I2C_write_data(B, 2);	//Enable gyro and acc
	
	msleep(5);	
	
	buf[0] = 0x1c, buf[1] = 0x00;
	B = buf;
	I2C_write_data(B, 2);	//Set acc +-2g
	
	msleep(5);	
	
	buf[0] = 0x1b, buf[1] = 0x18;
	B = buf;
	I2C_write_data(B, 2);	//Set gyro 2000dps
	
	msleep(5);
	return 0;
}

int MPURemove(struct i2c_client *client){
	// Stuff goes in here
	i2c_del_driver(&my_i2c_driver);
	unregisterDeviceFile();
	return 0;
}

// ###############################################################################################
// I2C init and remove functions
int initI2C(void){
	int ret = -1;
	
	// Create adaptor to get access to I2C bus
	my_i2c_adapter = i2c_get_adapter(I2C_BUS);
	if(my_i2c_adapter != NULL){
		my_i2c_client = i2c_new_device(my_i2c_adapter, &my_i2c_board_info);
		if(my_i2c_client != NULL){
			if(i2c_add_driver(&my_i2c_driver) != -1){
				ret = 0;
			} else{
				printk("Can't add i2c driver\n");
			}
		}
		i2c_put_adapter(my_i2c_adapter);
	}
	return ret;
}
void unregisterI2C(void){
	i2c_unregister_device(my_i2c_client);	// Removes my_i2c_client from adapter
	printk(KERN_INFO "I2C_client removed");
	i2c_del_adapter(my_i2c_adapter);		// Unregisters the I2C_adapter
	printk(KERN_INFO "I2C_adapter deleted");
	i2c_del_driver(&my_i2c_driver);		// Remove I2C_driver from kernel
	printk(KERN_INFO "I2C driver deleted");
}


// ############################################################################################
// Device file init and destroy functions
int initDeviceFile(void){
	// Allocate device nr.
	if( (alloc_chrdev_region(&dev, 0, 1, DRIVER_NAME)) < 0 ){
		printk( KERN_ALERT "I2CKernelModule registration failed\n" );
		return -1;
	}

	// Register module in /dev/
	if( (deviceFileClass = class_create(THIS_MODULE, "chardev")) == NULL ){
		printk(KERN_ALERT "Class creation failed\n");
		unregister_chrdev_region(dev, 1);
		return -1;
	}
	deviceFileClass->dev_uevent = dev_uevent;	// Set permissions - permissions defined in function dev_uevent().
	
	//Create device file
	if( device_create(deviceFileClass, NULL, dev, NULL, "I2CDriver") == NULL ){
		printk(KERN_ALERT "Device creation failed");
		class_destroy(deviceFileClass);
		unregister_chrdev_region(dev,1);
		return -1;
	}

	// Initialize device file
	cdev_init(&c_dev, &fops);

	// Register device to kernel
	if( cdev_add( &c_dev, dev, 1) == -1){
		printk(KERN_ALERT "Device addition failed");
		device_destroy( deviceFileClass, dev );
		class_destroy( deviceFileClass );
		unregister_chrdev_region(dev, 1);
		return -1;
	}

	return 0;
}

void unregisterDeviceFile(void){
	// Used to remove driver from /dev
	//unregister_chrdev(major, DEVICE_NAME);
	cdev_del( &c_dev );
	device_destroy( deviceFileClass, dev );
	class_unregister( deviceFileClass );
	class_destroy( deviceFileClass );
	unregister_chrdev_region(dev, 1);

}

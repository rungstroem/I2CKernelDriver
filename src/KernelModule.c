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
#include <linux/delay.h>	// Used for msleep() command

#include "I2C.h"
#include "cmdInterpreter.h"

//#####################################################
// Author and license
MODULE_AUTHOR("Kenneth R Larsen");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A driver for an I2C enabled robot");
MODULE_DEVICE_TABLE(i2c, my_id_table);

#define DEVICE_NAME "I2CKernelModule"
#define DRIVER_NAME "I2CDriver"

//#####################################################

//#####################################################
// Global variables for device_file - Reading and writing to /dev/I2CDriver
#define BUF_LEN 80

static struct cdev c_dev;
static dev_t dev;
static struct class *deviceFileClass;
int deviceOpen = 0;
char Message[BUF_LEN];
char *Message_Ptr;
bool cmdIdentified;
//#####################################################

// #############################################################################################
// All below is reading and writing to the /dev/I2Cdriver file
static int dev_open(struct inode *inodep, struct file *filep){
	if(deviceOpen) return -EBUSY;
	deviceOpen++;
	Message_Ptr = Message;
	try_module_get(THIS_MODULE);
	printk(KERN_INFO "I2CKernelModule opened");
	return 0;
}

static int dev_release(struct inode *inodep, struct file *filep){
	deviceOpen--;
	module_put(THIS_MODULE);
	printk(KERN_INFO "I2CKernelModule closed");
	return 0;
}

// Called when reading from the device [cat /dev/I2CDriver]
static ssize_t dev_read(struct file *filep, char *userBuffer, size_t len, loff_t *offset){	//Len is the size of the user buffer, loff_t is the index in the user buffer
	// C90 requires declaration before code
	int bytesRead;
	unsigned char data;
	unsigned char *D;
	char outMessage;
	//Print for debugging
	printk(KERN_INFO "Read from device Entered");
	
	if(cmdIdentified){
		D = &data;
		I2C_read_data(D,1);
		if(data == 0x00){
			outMessage = '0';
		}else{
			outMessage = data;
		}
		Message_Ptr = &outMessage;
	}else{
		strcpy(Message, "command not identified");
		Message_Ptr = Message;
	}
	
	if(*Message_Ptr == 0) return -1;	//If the pointer is 0 then no message was read

	bytesRead = 0;
	//Print message from I2C to user
	while(len && *Message_Ptr){
		put_user(*(Message_Ptr++), userBuffer++);
		len--;
		bytesRead++;
	}
	return bytesRead;	
}

// Called when writing to the device [echo > "command" /dev/I2CDriver]
static ssize_t dev_write(struct file *filep, const char *userBuffer, size_t len, loff_t *offset){
	// C90 requires declaration before code.
	int i;
	unsigned char cmd;
	unsigned char *C;
	unsigned char cmdData[2];
	char inMessage[8] = {0x00};
	char command[8] = {0x00};
	unsigned char data;
	// Print for debugging
	printk(KERN_INFO "Write to device Entered");
	
	// Get message from userspace
	for(i = 0; i < len && i < BUF_LEN; i++){
		get_user(inMessage[i], userBuffer +i);		// Echo inserts \n at the end!
	}
	strcpy(command, inMessage);
	command[7] = 0x00;
	command[5] = '\n';
	data = inMessage[6];
	// Send command to I2C
	cmd = commandIntMPU(command);
	if(cmd == 0x00){
		cmdIdentified = false;
		printk(KERN_INFO "Command not identified");
	}else{
		if(data == 0x00){
			C = &cmd;
			cmdIdentified = true;
			I2C_write_data(C,1);
		}else if(data != 0x00){
			cmdIdentified = true;
			cmdData[0] = cmd;
			cmdData[1] = data;
			C = cmdData;
			I2C_write_data(C,2);
		}
	}

	return i;
}

static struct file_operations fops={
	.owner = THIS_MODULE,
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};

int init_device_file(void);

// Runs when module is inserted in kernel (insmod)
int init_module(void){
	int I2CReturnVal;
	int deviceFileVal;

	// Initialize I2C adaptor and client
	I2CReturnVal = initI2C();
	if(I2CReturnVal == -1){
		return -1;
	}

	// Initialize the device file
	deviceFileVal = init_device_file();
	if(deviceFileVal == -1){
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

	// Used to remove driver from /dev
	//unregister_chrdev(major, DEVICE_NAME);
	cdev_del( &c_dev );
	device_destroy( deviceFileClass, dev );
	class_destroy( deviceFileClass );
	unregister_chrdev_region(dev, 1);

	printk(KERN_INFO "I2CKernelModule removed\n");
}

int init_device_file(void){
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
	
	//Create device file
	if( (device_create(deviceFileClass, NULL, dev, NULL, "I2CDriver")) == NULL ){
		printk(KERN_ALERT "Device creation failed");
		class_destroy(deviceFileClass);
		unregister_chrdev_region(dev,1);
		return -1;
	}

	// Initialize device file
	cdev_init(&c_dev, &fops);

	// Register device to kernel
	if( (cdev_add( &c_dev, dev, 1)) == -1){
		printk(KERN_ALERT "Device addition failed");
		device_destroy( deviceFileClass, dev );
		class_destroy( deviceFileClass );
		unregister_chrdev_region(dev, 1);
		return -1;
	}
	// If everything went well - return 0
	return 0;

}

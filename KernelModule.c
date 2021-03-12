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

//#####################################################
// Author and license
MODULE_AUTHOR("Kenneth R Larsen");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A driver for I2C");

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

//#####################################################

//#####################################################
// Global variables for I2C
#define I2C_BUS_AVAILABLE 1
#define SLAVE_DEVICE_NAME "MPU6050"
#define SLAVE_DEVICE_ADDRESS 0x68

static struct i2c_adapter * my_i2c_adapter = NULL;
static struct i2c_client * my_i2c_client = NULL;

//#####################################################

//#####################################################
// I2C related code

// Supported devices
static struct i2c_device_id my_id_table[] = {
	{ SLAVE_DEVICE_NAME, 0},
	{},	//Has to be terminated with a 0 entry - some kernel querk...
};

MODULE_DEVICE_TABLE(i2c, my_id_table);

// Hooks to manage devices
static struct i2c_driver my_i2c_driver = {
	.driver = {
		.name = SLAVE_DEVICE_NAME,
		.owner = THIS_MODULE,
	},
	.id_table = my_id_table,

};

static struct i2c_board_info my_i2c_board_info = {
	I2C_BOARD_INFO(SLAVE_DEVICE_NAME, SLAVE_DEVICE_ADDRESS)
};

int read_special(void){
	if(strcmp("getID", Message)){
		Message[0] = (char)i2c_smbus_read_byte_data(my_i2c_client, 0x75);
		Message_Ptr = Message;
	}
	return 0;
}

int I2C_read_data(unsigned char *outBuf, unsigned int len){
	int ret = -1;
	ret = i2c_master_recv(my_i2c_client, outBuf, len);
	return ret;
}

int I2C_write_data(unsigned char *buf, unsigned int len){
	int ret = -1;
	ret = i2c_master_send(my_i2c_client, buf, len);
	return ret;
}

// I2C init and remove functions
int initI2C(void){
	int ret = -1;
	
	// Create adaptor to get access to I2C bus
	my_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);
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
static ssize_t dev_read(struct file *filep, char *userBuffer, size_t len, loff_t *offset){
	printk(KERN_INFO "Read from device Entered");
	int bytesRead = 0;
	
	// Read from I2C
	I2C_read_data(Message, 1);
	Message_Ptr = Message;		//Sets the pointer to the start of the message

	if(*Message_Ptr == 0) return -1;	//If the pointer is 0 then no message was read
	
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
	printk(KERN_INFO "Write to device Entered");
	int i;
	for(i = 0; i < len && i < BUF_LEN; i++){
		get_user(Message[i], userBuffer +i);
	}
	Message_Ptr = Message;
	unsigned char buf[2] = {0};
	buf[0] = 0x75;
	I2C_write_data(Message, 1);

	return i;
}

static struct file_operations fops={
	.owner = THIS_MODULE,
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};

// Runs when module is inserted in kernel (insmod)
int init_module(void){
	
	// Allocate device nr.
	init_result = alloc_chrdev_region(&dev, 0, 1, DRIVER_NAME);
	if(init_result < 0){
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
	
	// Initialize I2C adaptor and client
	int I2CReturnVal = initI2C();
	if(I2CReturnVal == -1){
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
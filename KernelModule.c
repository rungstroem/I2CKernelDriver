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
MODULE_LICENSE("GPL");

#define DEVICE_NAME "I2CKernelModule"
#define BUF_LEN 80
static struct cdev c_dev;
static dev_t dev;
static struct class *deviceFileClass;

//static int major;		//Defines the driver
//static int minor;		//Defines the device
int init_result;
int deviceOpen = 0;
char Message[BUF_LEN];
char *Message_Ptr;

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

static ssize_t dev_read(struct file *filep, char *userBuffer, size_t len, loff_t *offset){
	printk(KERN_INFO "Read entered");
	int bytes_read = 0;
	if(*Message_Ptr == 0) return 0;

	while(len && *Message_Ptr){
		put_user(*(Message_Ptr++), userBuffer++);
		len--;
		bytes_read++;
	}
	return bytes_read;	
}

static ssize_t dev_write(struct file *filep, const char *userBuffer, size_t len, loff_t *offset){
	printk(KERN_INFO "write entered");
	int i;
	for(i = 0; i < len && i < BUF_LEN; i++){
		get_user(Message[i], userBuffer +i);
	}
	Message_Ptr = Message;
	return i;
}

static struct file_operations fops={
	.owner = THIS_MODULE,
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};
//static struct miscdevices

// Runs when module is inserted in kernel (insmod)
int init_module(void){
	// Register module major number
	//major = register_chrdev(0,DEVICE_NAME,&fops);
	//if(major < 0){
	//	printk(KERN_ALERT "I2CKernelModule failed to load\n");
	//	return major;
	//}
	
	init_result = alloc_chrdev_region(&dev, 0, 1, "I2CDriver");
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

	if( device_create(deviceFileClass, NULL, dev, NULL, "I2CDriver") == NULL ){
		printk(KERN_ALERT "Device creation failed");
		class_destroy(deviceFileClass);
		unregister_chrdev_region(dev,1);
		return -1;
	}
	
	cdev_init(&c_dev, &fops);

	if( cdev_add( &c_dev, dev, 1) == -1){
		printk(KERN_ALERT "Device addition failed");
		device_destroy( deviceFileClass, dev );
		class_destroy( deviceFileClass );
		unregister_chrdev_region(dev, 1);
		return -1;
	}

	// This retuns if init executes okay. 
	printk(KERN_INFO "I2CKernelModule inserted\n");
	return 0;
}

// Runs when module is removed from kernel (rmmod)
void cleanup_module(void){
	//unregister_chrdev(major, DEVICE_NAME);
	cdev_del( &c_dev );
	device_destroy( deviceFileClass, dev );
	class_destroy( deviceFileClass );
	unregister_chrdev_region(dev, 1);

	printk(KERN_INFO "I2CKernelModule removed\n");
}

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

#define DEVICE_NAME "I2CKernelModule"
MODULE_LICENSE("GPL");

static struct cdev c_dev;
static dev_t dev;
static struct class *deviceFileClass;

//static int major;		//Defines the driver
//static int minor;		//Defines the device
int init_result;
int deviceOpen = 0;

static int dev_open(struct inode*, struct file*);
static int dev_release(struct inode*, struct file*);
static ssize_t dev_read(struct file*, char*, size_t, loff_t*);
static ssize_t dev_write(struct file*, const char*, size_t, loff_t*);

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

static int dev_open(struct inode *inodep, struct file *filep){
	if(deviceOpen) return -EBUSY;

	printk(KERN_INFO "I2CKernelModule opened");
	return 0;
}

static int dev_release(struct inode *inodep, struct file *filep){
	printk(KERN_INFO "I2CKernelModule closed");
	return 0;
}

char buf[100] = "Hello Master Kenneth\n";

static ssize_t dev_read(struct file *filep, char *userBuffer, size_t len, loff_t *offset){
	printk(KERN_INFO "Read entered");
	int errors = 0;
	char *message = "Some message";
	int message_len = strlen(message);
	
	int bufferSize = strlen(buf);
	errors = copy_to_user(userBuffer, buf, bufferSize);

	return errors == 0 ? bufferSize : -EFAULT;
}

static ssize_t dev_write(struct file *filep, const char *userBuffer, size_t len, loff_t *offset){
	printk(KERN_INFO "write entered");
	int errors = 0;
	int bytesCopied = 0;
	if(access_ok(userBuffer,3)){
		errors = copy_from_user(buf, userBuffer, 3);
	}

	return errors == 0 ? bytesCopied : -EFAULT;
	//https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html#laboratory-objectives - use this website
	//printk(KERN_INFO "I2CKernelModule is only readable for now");
	//https://www.oreilly.com/library/view/linux-device-drivers/0596000081/ch03s08.html
}

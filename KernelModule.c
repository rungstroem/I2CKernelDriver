#include <linux/module.h>	// Needed by all modules
#include <linux/kernel.h>	// Needed for KERN_INFO

int init_module(void){
	printk(KERN_INFO "Module inserted\n");
	return 0;
}

void cleanup_module(void){
	printk(KERN_INFO "Module removed\n");
}

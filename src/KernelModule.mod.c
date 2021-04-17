#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xf230cadf, "module_layout" },
	{ 0x8c76ee53, "cdev_del" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0xd64795cd, "class_destroy" },
	{ 0x17ae16c5, "device_destroy" },
	{ 0x89ab7bf8, "cdev_add" },
	{ 0x2b68ed92, "cdev_init" },
	{ 0xabe5f4f7, "device_create" },
	{ 0x4751a8ae, "__class_create" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x7318c220, "i2c_del_driver" },
	{ 0xcc46794c, "i2c_del_adapter" },
	{ 0x14a85cf2, "i2c_unregister_device" },
	{ 0xb6c94b2, "i2c_put_adapter" },
	{ 0xcc97b913, "i2c_register_driver" },
	{ 0xe82cc1c0, "i2c_new_device" },
	{ 0xc01d79f2, "i2c_get_adapter" },
	{ 0x28118cb6, "__get_user_1" },
	{ 0x5c7ab3bb, "i2c_smbus_read_byte_data" },
	{ 0xe2d5255a, "strcmp" },
	{ 0xbb72d4fe, "__put_user_1" },
	{ 0x35034fdd, "try_module_get" },
	{ 0x7c32d0f0, "printk" },
	{ 0x8b307391, "module_put" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0xdb7305a1, "__stack_chk_fail" },
	{ 0xf9a482f9, "msleep" },
	{ 0x78f2655b, "i2c_transfer_buffer_flags" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("i2c:MPU6050");

MODULE_INFO(srcversion, "99307007AECE1A3D5435F31");

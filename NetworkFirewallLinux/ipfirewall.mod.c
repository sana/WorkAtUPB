#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x9c4c5c87, "module_layout" },
	{ 0x10a8f815, "cdev_del" },
	{ 0x42521ced, "kmalloc_caches" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0x2ad9871e, "cdev_init" },
	{ 0xb279da12, "pv_lock_ops" },
	{ 0xb61f0f80, "del_timer" },
	{ 0xd8e484f0, "register_chrdev_region" },
	{ 0x973873ab, "_spin_lock" },
	{ 0x4661e311, "__tracepoint_kmalloc" },
	{ 0xb1ca2692, "init_timer_key" },
	{ 0xaae453e6, "nf_register_hook" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x7d11c268, "jiffies" },
	{ 0x2da418b5, "copy_to_user" },
	{ 0x6b5a2d1, "mod_timer" },
	{ 0xb71d1ecf, "cdev_add" },
	{ 0x650f0f30, "kmem_cache_alloc" },
	{ 0xf5a65e79, "nf_unregister_hook" },
	{ 0x3aa1dbcf, "_spin_unlock_bh" },
	{ 0x37a0cba, "kfree" },
	{ 0x93cbd1ec, "_spin_lock_bh" },
	{ 0xf2a644fb, "copy_from_user" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "E67525288192921E25E6AAA");

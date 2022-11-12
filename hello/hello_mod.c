#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("P3R5157");
MODULE_DESCRIPTION("HELLO KERNEL");
MODULE_VERSION("0.01");

static int __init herro_uwu(void)
{
	printk(KERN_INFO "\n\t ~ Yo.. Wuddup Kuz! ~ \n\n");
	return 0;
}

static void __exit klybye(void)
{
	printk(KERN_INFO "\n\t ~ GOODBYE KRUEL WORLD! ~ \n\n");
	return;
}

module_init(herro_uwu);
module_exit(klybye);

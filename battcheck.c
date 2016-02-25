#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>

static struct timer_list my_timer;

void my_timer_callback( unsigned long data ){
	printk(KERN_INFO "my_timer_callback called\n");
	mod_timer( &my_timer, jiffies + msecs_to_jiffies(2000))
}

int init_module( void ){
	int ret;

	printk("Timer module installing\n");

	// my_timer.function, my_timer.data
	setup_timer( &my_timer, my_timer_callback, 0);

	printk("Starting timer to fire in 1 sec\n");
	ret = mod_timer( &my_timer, jiffies + msecs_to_jiffies
	if (ret) printk(KERN_EMERG "Error in mod_timer\n");

	return 0;
}

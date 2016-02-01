#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/slab.h>
#include<linux/workqueue.h>

MODULE_LICENSE("GPL");

struct workqueue_struct *queue;
struct delayed_work *dwork;
int i;

void function(struct work_struct *work){
	printk(KERN_EMERG "Ding\n");
}

int init_module(void){
	printk(KERN_EMERG "Module succesfully loaded.\n");
	queue = create_workqueue("queue");	
	for(i = 1; i <= 10; i++){
		dwork = (struct delayed_work*)kmalloc(sizeof(struct delayed_work), GFP_KERNEL);
		INIT_DELAYED_WORK((struct delayed_work*)dwork, function);
		queue_delayed_work(queue, dwork, i*HZ);
	}

	return 0;
}

void cleanup_module(void){
	if (dwork && delayed_work_pending(dwork)) cancel_delayed_work(dwork);
	flush_workqueue(queue);
	destroy_workqueue(queue);
	printk(KERN_EMERG "Module succesfully unloaded.\n");
}


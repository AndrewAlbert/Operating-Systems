#include<linux/module.h>
#include<linux/slab.h>
#include<linux/workqueue.h>

struct workqueue_struct *queue;
struct delayed_work *dwork;
int i;

void function(struct work_struct *work){
	printk(KERN_INFO "Ding #%d\n", i++);
	if(i <= 10)queue_delayed_work(queue, (struct delayed_work*)dwork, HZ);
}

int init_module(void){
	i = 1;
	printk(KERN_INFO "Workqueue module loaded successfully.\n");
	queue = create_workqueue("queue");	
	dwork = (struct delayed_work*)kmalloc(sizeof(struct delayed_work), GFP_KERNEL);	
	INIT_DELAYED_WORK((struct delayed_work*)dwork, function);
	queue_delayed_work(queue, (struct delayed_work*)dwork, HZ);
	return 0;
}

void cleanup_module(void){
	flush_workqueue(queue);
	if (dwork && delayed_work_pending(dwork)) cancel_delayed_work(dwork);
	destroy_workqueue(queue);
	printk(KERN_INFO "Workqueue module unloaded succesfully.\n");
}

MODULE_LICENSE("GPL");

#include<linux/module.h>
#include<linux/kthread.h>

struct task_struct *ts;
int id = 101;
bool flag = false;

int function(void *data){
	int count = 0;

	while((count++ < 10) && !(kthread_should_stop()))
	{		
		printk(KERN_INFO "Ding #%d\n",count);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(HZ);
	}
	flag = true;
	return 0;
}

int init_module(void){
	printk(KERN_INFO "Kthread module loaded succesfully.\n");
	ts = kthread_run(function,(void*)&id,"spawn");
	return 0;
}

void cleanup_module(void){
	if(flag == false) kthread_stop(ts);
	printk(KERN_INFO "Kthread module unloaded succesfully.\n");
}

MODULE_LICENSE("GPL");

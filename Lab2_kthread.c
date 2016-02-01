#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/kthread.h>

MODULE_LICENSE("GPL");
struct task_struct *ts;
int id = 101;
bool flag = false;

int function(void *data){
	//int n = *(int*)data;
	int count = 0;

	while((count++ < 10) && !(kthread_should_stop()))
	{		
		printk(KERN_EMERG "Ding\n");
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(HZ);
	}
	flag = true;
	return 0;
}

int init_module(void){
	ts = kthread_run(function,(void*)&id,"spawn");
	return 0;
}

void cleanup_module(void){
	if(flag == false) kthread_stop(ts);
	printk(KERN_EMERG "Module unloaded succesfully.\n");
}

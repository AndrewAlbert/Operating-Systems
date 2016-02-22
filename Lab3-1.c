#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/kthread.h>

//two kthreads that will share access to data
struct task_struct *t1, *t2;
int id1 = 1, id2 = 2;

//global array and index that both threads will do work on
int arr[1000000];
int idx = 0;

//total number of increments done by each kthread
int cs1 = 0, cs2 = 0;

//array to total the number of single, double, triple, etc. increments that were done in arr
int stat[1000000];

int incrementer(void *ptr){
	int id = *(int*)ptr;
	printk(KERN_INFO "Consumer TID %d\n", id);
	//increment every value of arr[idx] in each respective kthread. 
	while (idx < 1000000 && !kthread_should_stop()){
		arr[idx++] += 1;
		if(id == 1) cs1++;
		else cs2++;			
		schedule();		
	}
	return 0;
}

int init_module(void){
	//initialize two kthreads and begin execution
	t1 = kthread_create(incrementer, (void*)&id1, "inc1");
	t2 = kthread_create(incrementer, (void*)&id2, "inc2");
	if (t1 && t2){
		printk(KERN_INFO "Starting..\n");
		wake_up_process(t1);
		wake_up_process(t2);
	} else{
		printk(KERN_EMERG "Error\n");
	}
	return 0;
}

void cleanup_module(void){
	//initialize stat array to all zeros	
	int i, x;
	for(i = 0; i < 1000000; i++) stat[i] = 0;

	//count the total number of entries x in arr[i] who have been incremented to the value i
	for(i = 0; i < 1000000; i++){
		x = arr[i];
		stat[x]++;
	}

	//print info on results of test.
	for(i = 0; i < 1000000; i++){
		if(stat[i] > 0) printk(KERN_INFO "Number of elements incremented %d times: %d\n",i,stat[i]);
	}
	printk(KERN_INFO "Thread 1 increment count: cs1 = %d\n",cs1);
	printk(KERN_INFO "Thread 2 increment count: cs2 = %d\n",cs2);
	printk(KERN_INFO "Combined thread increment count: cs1 + cs2 = %d\n",cs1+cs2);
	printk(KERN_INFO "module unloaded succesfully.\n");
}

MODULE_LICENSE("GPL");

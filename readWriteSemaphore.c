#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");

//global array and read write semaphore
int array[100];
struct rw_semaphore rwsem;

//relevant device driver info and work queue
struct cdev *kernel_cdev;
dev_t dev_no;
int Major;
wait_queue_head_t queue;

ssize_t read(struct file *filp, char *buff, size_t count, loff_t *offp){
	unsigned long ret;
	printk(KERN_INFO "\nRead: start request\n");

	down_read(&rwsem);
	printk(KERN_INFO "\tRead arrived in critical section\n");
	wait_event_timeout(queue, 0, 20*HZ);

	printk(KERN_INFO "\tstart of copy to user\n");
	ret = copy_to_user(buff, array, count);
	printk(KERN_INFO "\tend of copy to user\n");
	printk(KERN_INFO "\tglobal array contents: %s\n", buff);

	up_read(&rwsem);
	printk(KERN_INFO "\tsemaphore released, exiting read critical section\n");
	return count;
};

ssize_t write(struct file *filp, const char *buff, size_t count, loff_t *offp){
	unsigned long ret;
	printk(KERN_INFO "\nWrite: start request\n");

	down_write(&rwsem);
	printk(KERN_INFO "\tWrite arrived in critical section\n");
	wait_event_timeout(queue, 0, 20*HZ);

	printk(KERN_INFO "\tstart of copy from user\n");
	ret = copy_from_user(array, buff, count);
	printk(KERN_INFO "\tend of copy from user\n");
	printk(KERN_INFO "\tglobal array contents: %s\n", buff);
	
	up_write(&rwsem);
	printk(KERN_INFO "\tsemaphore released, exiting write critical section\n");
	return count;
};

int open(struct inode *inode, struct file *filp){
	printk("\nOpened Device\n");
	return 0;
};

int release(struct inode *inode, struct file *filp){
	printk("\nReleased Device\n");
	return 0;
};

struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = read,
	.write = write,
	.open = open,
	.release = release
};

int init_module(void){
	int ret;
	int dev;

	kernel_cdev = cdev_alloc();
	kernel_cdev->ops = &fops;
	kernel_cdev->owner = THIS_MODULE;
	ret = alloc_chrdev_region(&dev_no, 0, 1, "interface");

	if(ret < 0){
		printk(KERN_ERR "Major number allocation failed\n");
		return ret;
	}

	//get device Major # and print for the user
	Major = MAJOR(dev_no);
	printk(KERN_INFO "Major number: %d\n", Major);
	dev = MKDEV(Major, 0);

	//register device and ensure it is properly allocated
	ret = cdev_add(kernel_cdev, dev, 1);
	if(ret < 0){
		printk(KERN_ERR "Unable to allocate cdev\n");
		return ret;
	}

	//initialize semaphore and workqueue
	init_rwsem(&rwsem);
	init_waitqueue_head(&queue);

	printk(KERN_INFO "Module succesfully loaded\n");

	return 0;
};

void cleanup_module(void){
	cdev_del(kernel_cdev);
	unregister_chrdev_region(dev_no,1);
	unregister_chrdev(Major,"interface");
	printk(KERN_INFO "Module unloaded\n");
}

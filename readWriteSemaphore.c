#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");

char array[100];
struct rw_semaphore rwsem;

struct cdev *kernel_cdev;
dev_t dev_no;
int Major;
wait_queue_head_t queue;
int read_chars, written_chars;

ssize_t read(struct file *filp, char *buff, size_t count, loff_t *offp){
	printk(KERN_INFO "Read: start request\n");
	unsigned long ret;
	printk(KERN_INFO "Read: read = %d written = %d count = %d\n", read_chars, written_chars, (int)count);
	down_read(&rwsem);
	printk(KERN_INFO "Read: Arrived in critical section\n");
	wait_event_timeout(queue, 0, 20*HZ);
	printk(KERN_INFO "Read: start of copy\n");
	ret = copy_to_user(buff, array, count);
	printk(KERN_INFO "Read: end of the copy\n");
	up_read(&rwsem);
	printk(KERN_INFO "Read: semaphore released\n");
	return count;
};

ssize_t write(struct file *filp, const char *buff, size_t count, loff_t *offp){
	printk(KERN_INFO "Write start request\n");
	unsigned long ret;
	down_write(&rwsem);
	wait_event_timeout(queue, 0, 20*HZ);
	if(count > 99) count = 99;
	ret = copy_from_user(array, buff, count);
	written_chars = count;

	printk(KERN_INFO "Write: read = %d written = %d count = %d\n", read_chars, written_chars, (int)count);
	printk(KERN_INFO "Write buffer: %s\n", buff);
	up_write(&rwsem);
	return count;
};

int open(struct inode *inode, struct file *filp){
	printk(KERN_INFO "Open: read = %d written = %d\n", read_chars, written_chars);
	read_chars = written_chars;
	return 0;
};

int release(struct inode *inode, struct file *filp){
	printk(KERN_INFO "Release: read = %d written = %d\n", read_chars, written_chars);
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

	Major = MAJOR(dev_no);
	printk(KERN_INFO "Major number: %d\n", Major);
	dev = MKDEV(Major, 0);
	ret = cdev_add(kernel_cdev, dev, 1);

	if(ret < 0){
		printk(KERN_ERR "Unable to allocate cdev\n");
		return ret;
	}

	init_rwsem(&rwsem);
	init_waitqueue_head(&queue);
	printk(KERN_INFO "Module succesfully loaded\n");
	return 0;
};

void cleanup_module(void){
	printk(KERN_INFO "Module unloaded\n");
	cdev_del(kernel_cdev);
	unregister_chrdev_region(dev_no,1);
	unregister_chrdev(Major,"interface");
}

#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include "buddyTree.h"
#include "chardev.h"

MODULE_LICENSE("GPL");

#define DEV_NAME "char_dev"
#define BUF_LENGTH 128
#define MEM_SIZE (1 << 12)

struct block* buddy_allocator;
int ref, size;
bool readDone;

static int Device_Open = 0;

static char Message[BUF_LENGTH];

static char *Message_Ptr;

static int open(struct inode *inode, struct file *file){
	printk(KERN_INFO "open: %p\n", file);
	if(Device_Open) return -EBUSY;
	else Device_Open++;
	Message_Ptr = Message;
	return 0;
}

static int release(struct inode *inode, struct file *file){
	printk(KERN_INFO "release: %p, %p\n", inode, file);
	Device_Open--;
	return 0;
}

static ssize_t read(struct file *file, char *buffer, size_t length, loff_t *offset){
	int bytes_read = 0;
	int index = 0;
	char myBuff[256];

	if(readDone){
		readDone = false;
		return 0;
	}

	if(size > MEM_SIZE || size <= 0) size = BUF_LENGTH;

	index = read_mem(buddy_allocator, ref, myBuff, size);
	if(index != 0) return -1;

	while(myBuff[index] != '\0' && index < BUF_LENGTH){
		put_user(myBuff[index++], buffer++);
		length--;
		bytes_read++;
	}

	printk(KERN_INFO "read %d bytes, %d left\n", bytes_read, (int)length);
	readDone = true;
	return bytes_read;
}

static ssize_t write(struct file *file, const char *buffer, size_t length, loff_t *offset){
	int i, j;
	char msg[256];
	printk(KERN_INFO "write(%p,%s,%d)", file, buffer, BUF_LENGTH);
	for(i = 0; i < length && i < BUF_LENGTH; i++)  get_user(msg[i], buffer+i);
	msg[i] = '\0';
	j = write_mem(&buddy_allocator, ref, msg);
	readDone = false;

	if(j != 0) return -1;
	
	return i;
}

long ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param){
	int i;
	char *temp;
	char ch;

	switch(ioctl_num){
		case IOCTL_SET_MSG:
			temp = (char*)ioctl_param;
			get_user(ch, temp);
			for(i = 0; ch && i < BUF_LENGTH; i++, temp++) get_user(ch, temp);

			write(file, (char*)ioctl_param, i, 0);
			break;
		case IOCTL_GET_MSG:
			readDone = false;
			i = read(file, (char*)ioctl_param, 99, 0);
			readDone = true;
			printk(KERN_INFO "ioctl_get_msg: %s i=%d\n", (char*)ioctl_param,i);
			put_user('\0', (char*)ioctl_param + i);
			break;
		case IOCTL_GET_NTH_BYTE:
			return buddy_allocator->memory[ioctl_param];
			break;
		case GET_MEM:
			ref = get_mem(buddy_allocator, (int)ioctl_param);
			return ref;
			break;
		case READ_MEM:
			readDone = false;
			i = read(file, (char*)ioctl_param, size, 0);
			readDone = true;
			if(i < 0) return i;
			put_user('\0',(char*)ioctl_param + i);
			break;
		case WRITE_MEM:
			temp = (char*)ioctl_param;
			get_user(ch, temp);
			for(i = 0; ch && i < BUF_LENGTH; i++, temp++) get_user(ch, temp);
			return write(file,(char*)ioctl_param, i, 0);
			break;
		case FREE_MEM:
			return free_mem(&buddy_allocator, (int)ioctl_param);
			break;
		case SET_REF:
			ref = (int)ioctl_param;
			return ref;
			break;
		case SET_SIZE:
			size = (int)ioctl_param;
			return size;
			break;
		case PRINT_TREE:
			print_tree(buddy_allocator);
			break;
	}
	return 0;
}


struct file_operations Fops = {
	.read = read,
	.write = write,
	.unlocked_ioctl = ioctl,
	.open = open,
	.release = release
};

int init_module(void){
	int ret;
	char buffer[256];
	ret = register_chrdev(MAJOR_NUM, DEV_NAME, &Fops);
	if(ret < 0){
		printk(KERN_ERR "Failed to register device\n");
		return ret;
	}
	printk(KERN_INFO "Succesfully registered with major #: %d\n", MAJOR_NUM);

	buddy_allocator = start_buddy_allocator(MEM_SIZE);
	if(!buddy_allocator) printk("Error buddy allocator failed\n");

	print_tree(buddy_allocator);

	ref = get_mem(buddy_allocator, 100);
	sprintf(buffer, "Hello buddy\n");
	write_mem(&buddy_allocator, ref, buffer);
	read_mem(buddy_allocator, ref+3, buffer, 10);
	printk("module loaded: buffer contents = %s\n", buffer);
	readDone = false;
	return 0;
}

void cleanup_module(void){
	unregister_chrdev(MAJOR_NUM, DEV_NAME);
	destroy(&buddy_allocator);
	if(buddy_allocator) printk("Error buddy allocator failed to remove\n");
	else printk("module unloaded\n");
}

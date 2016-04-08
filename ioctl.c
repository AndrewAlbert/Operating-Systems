/*
 *  ioctl.c - the process to use ioctl's to control the kernel module
 *
 *  Every device can have its own ioctl commands to send information 
 *  from a process to the kernel, write information to a process, both 
 *  or neither.  The ioctl function is called with three parameters: 
 *    the file descriptor of the appropriate device file, 
 *    the ioctl number, 
 *    and an open parameter of type long
 *
 *  The ioctl number encodes the major device number, the type of the ioctl, 
 *  the command, and the type of the parameter.  The ioctl number is usually 
 *  created by a macro call (_IO, _IOR, _IOW or _IOWR --- depending on the 
 *  type) in a header file.  This header file should then be included both 
 *  by the programs which will use ioctl and by the kernel module.  The
 *  header file is 'sys/ioctl.h'
 *
 *  It is best to receive an official ioctl assignment, so if you accidentally 
 *  get somebody else's ioctls, or if they get yours, you'll know something is 
 *  wrong.
 *
 *  Until now we could have used cat for input and output.  But now
 *  we need to do ioctl's, which require writing our own process.
 *
 *  Note: ioctl's read is to send information to the kernel and its write 
 *  is to receive information from the kernel.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
/* 
 * device specifics, such as ioctl numbers and the major device file. 
 */
#include "chardev.h"
MODULE_LICENSE("GPL");
/* 
 * Functions for the ioctl calls 
 */

ioctl_set_msg(int file_desc, char *message) {
   int ret_val;
   
   /* IOCTL_SET_MSG is an ioctl number */
   ret_val = ioctl(file_desc, IOCTL_SET_MSG, message,0,0);
   if (ret_val < 0) {
      printf("ioctl_set_msg failed:%d\n", ret_val);
      exit(-1);
   }
}

ioctl_get_msg(int file_desc) {
   int ret_val;
   char message[100];
   
   /* 
    * Warning - this is dangerous because we don't tell
    * the kernel how far it's allowed to write, so it
    * might overflow the buffer. In a real production
    * program, we would have used two ioctls - one to tell
    * the kernel the buffer length and another to give
    * it the buffer to fill
    */
   ret_val = ioctl(file_desc, IOCTL_GET_MSG, message,0,0);
   if (ret_val < 0) {
      printf("ioctl_get_msg failed:%d\n", ret_val);
      exit(-1);
   }
   
   printf("get_msg message:%s\n", message);
}

ioctl_get_nth_byte(int file_desc) {
   int i;
   char c;
   
   printf("get_nth_byte message:");
   c = ioctl(file_desc, IOCTL_GET_NTH_BYTE, 0,0,0);
   i = 0;
   do {
      c = ioctl(file_desc, IOCTL_GET_NTH_BYTE, i++,0,0);
      if (c < 0) {
	 printf("ioctl_get_nth_byte failed at the %d'th byte:\n", i);
	 exit(-1);
      }
      
      putchar(c);
   } while (c != 0);
   putchar('\n');
}

get_mem(int file_desc, int size){
	int ret_val;
	char message[100];
	ret_val = ioctl(file_desc, GET_MEM, size);
	if(ret_val < 0){
		printf("get_mem failed: %d\n", ret_val);
	}
	return ret_val;
}

read_mem(int file_desc, int ref, char *buf, int size){
	int ret_val;
	ioctl(file_desc, SET_REF, ref);
	ioctl(file_desc, SET_SIZE, size);
	ret_val = ioctl(file_desc, READ_MEM, buf);

	if(ret_val < 0){
		printf("read_mem failked: %d\n", ret_val);
	}
}

write_mem(int file_desc, int ref, char *buf){
	int ret_val;
	ioctl(file_desc, SET_REF, ref);
	ret_val = ioctl(file_desc, WRITE_MEM, buf);
	if(ret_val < 0){
		printf("write_mem failed: %d\n", ret_val);
	}
	return ret_val;
}

free_mem(int file_desc, int ref){
	int ret_val;
	char message[100];
	ret_val = ioctl(file_desc, FREE_MEM, ref);
	if(ret_val < 0){
		printf("free_mem failed: %d\n", ret_val);
	}
}

print_tree(int file_desc){
	ioctl(file_desc, PRINT_TREE, 0);
}

/* Main - Call the ioctl functions */
main() {
   int file_desc, ret_val, ref;
	char buffer[4096];
   char *msg0 = " ";
   char *msg1 = "Put that in your pipe and smoke it";
   char *msg2 = "Try this on for size";
   
   file_desc = open("/dev/char_dev", 0);
   if (file_desc < 0) {
      printf("Can't open device file: /dev/char_dev\n");
      exit(-1);
   }
   
   ioctl_set_msg(file_desc, msg0);
   ioctl_get_msg(file_desc);
   ioctl_set_msg(file_desc, msg1);
   ioctl_get_msg(file_desc);
   ioctl_set_msg(file_desc, msg2);
   ioctl_get_msg(file_desc);
   ioctl_get_nth_byte(file_desc);
   
   close(file_desc);
}

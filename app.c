#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

#define FNAME "interface"

int main(int argc, char **argv)
{
	int fd, length;
	char read_buf[100], write_buf[100];

	//ensure command line argument count is correct
	if(argc > 3 || argc < 2){
		printf("Invalid usage of app. Argument 1 should be 'read' or 'write' and argument 2 should be a message of 100 characters or less if argument 1 is 'write'\n");
		exit(0);
	}
	else if(argc == 3){
		length = strlen(argv[2]);
	}
	else length = 0;

	//open device
	fd = open("/dev/interface", O_RDWR);
	if(fd == -1){
		printf("failed to open device\n");
		exit(-1);
	}

	//read operation
	if(!strcmp("read", argv[1])){
		read(fd, read_buf, sizeof(read_buf));
		printf("read buffer contents: %s\n", read_buf);
	}
	//write operation
	else if(!strcmp("write", argv[1])){
		if(length > 100){
			printf("argument two is too long for write\n");
			exit(0);
		}
		strcpy(write_buf, argv[2]);
		write(fd, write_buf, sizeof(write_buf));
		printf("write buffer contents: %s\n", write_buf);
	}
	else printf("invalid operation, use 'read' or 'write' in argument 1\n");

	//close device
	close(fd);
	
	return 0;
}

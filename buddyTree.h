#ifndef _buddyTree
#define _buddyTree

MODULE_LICENSE("GPL");
/*
contains all of the information needed to implement the tree structure for the buddy allocator.
*/

#include <stdbool.h>

#define MEM_SIZE (1 << 12)
#define SMALLEST_SIZE 128

//define the block structure for the buddy allocator and its root node for the tree
typedef struct block{
	struct block* parent;
	struct block* Lchild;
	struct block* Rchild;
	bool isParent;
	bool occupied;
	int address;
	int size;
	char* memory;
} BuddyRoot;

struct block* selectedBlock;

//prototypes
int find_empty(struct block*, int);
void split_block(struct block*);
struct block* findBlock(struct block*, int);
struct block* create_block(int, int);

//dynamically create first block structure
struct block* start_buddy_allocator(int size){
	struct block* newBlock;
	newBlock = (struct block*)kmalloc(sizeof(struct block), GFP_KERNEL);
	newBlock->memory = kmalloc(size, GFP_KERNEL);
	newBlock->parent = NULL;
	newBlock->Lchild = NULL;
	newBlock->Rchild = NULL;
	newBlock->isParent = false;
	newBlock->occupied = false;
	newBlock->address = *(newBlock->memory);
	newBlock->size = size;
	return newBlock;
}

//dynamically create a new block structure
struct block* create_block(int addr, int size){
	struct block* newBlock;
	newBlock = (struct block*)kmalloc(sizeof(struct block), GFP_KERNEL);
	newBlock->parent = NULL;
	newBlock->Lchild = NULL;
	newBlock->Rchild = NULL;
	newBlock->isParent = false;
	newBlock->occupied = false;
	newBlock->address = addr;
	newBlock->size = size;
	newBlock->memory = NULL;
	return newBlock;
}

void destroy(struct block** root){
	if(root){
		if((*root)->Lchild) destroy(&(*root)->Lchild);
		if((*root)->Rchild) destroy(&(*root)->Rchild);
		if((*root)->memory) kfree((*root)->memory);
		kfree(*root);
		*root = NULL;
	}
}

int get_mem(struct block* parent, int size){
	int blockAddress;
	if(size > MEM_SIZE){
		printk(KERN_INFO "Error requesting more space than available in all of the memory\n");
		return -1;
	}
	blockAddress = find_empty(parent, size);

	if(selectedBlock && blockAddress != -1){
		while(selectedBlock->size/2 >= size && selectedBlock->size/2 >= SMALLEST_SIZE){
			split_block(selectedBlock);
			selectedBlock = selectedBlock->Lchild;
			blockAddress = selectedBlock->address;
		}
		selectedBlock = NULL;
	}
	else printk(KERN_INFO "Error can't find free space suitable\n");

	return blockAddress;
}

int free_mem(struct block** parent, int ref){
	struct block* freeBlock;
	if(ref > MEM_SIZE || ref < 0){
		printk(KERN_INFO "Error attempt to free memory outside of allocated space\n");
		return -1;
	}
	freeBlock = findBlock(*parent, ref);

	if(freeBlock->parent){
		if(freeBlock->parent->Lchild == freeBlock && !freeBlock->parent->Rchild->occupied){
			kfree(freeBlock->parent->Rchild);
			freeBlock->parent->isParent = false;
			freeBlock->parent->Lchild = NULL;
			freeBlock->parent->Rchild = NULL;
			kfree(freeBlock);
			freeBlock = NULL;
		}
		else if(freeBlock->parent->Rchild == freeBlock && !freeBlock->parent->Lchild->occupied){
			kfree(freeBlock->parent->Lchild);
			freeBlock->parent->isParent = false;
			freeBlock->parent->Lchild = NULL;
			freeBlock->parent->Rchild = NULL;
			kfree(freeBlock);
			freeBlock = NULL;
		}
	}

	if(freeBlock){
		int index;
		for(index = freeBlock->address; index < freeBlock->address + freeBlock->size; index++) (*parent)->memory[index] = '\0';
		freeBlock->occupied = false;
	}
	return 0;
}

int read_mem(struct block* parent, int ref, char* buf, int size){
	int index;
	if(ref < 0 || ref > MEM_SIZE || ref + size > MEM_SIZE){
		printk(KERN_INFO "Error: attempting to read outside of allocated memory space\n");
		return -1;
	}

	//add each read character in memory to buffer
	for(index = 0; index < size; index++){
		char character = parent->memory[ref - parent->address + index];
		buf[index] = character;
		if(character == '\0') return 0;
	}
	return 0;
}

int write_mem(struct block** parent, int ref, char* buf){
	int index;
	struct block* writeBlock;

	if(ref < 0 || ref > MEM_SIZE){
		printk(KERN_INFO "Error tried to write outside of allocated memory\n");
		return -1;
	}
	for(index = 0; buf[index] != '\0'; index++){}
	writeBlock = findBlock(*parent, ref);

	if(index >= writeBlock->size){
		printk(KERN_INFO "Error tried to write %d bytes to block of size %d bytes\n", index, writeBlock->size);
		return -1;
	}

	for(index = 0; buf[index] != '\0'; index++){
		(*parent)->memory[ref - (*parent)->address + index] = buf[index];
	}

	(*parent)->memory[ref - (*parent)->address + index] = '\0';

	if(writeBlock) writeBlock->occupied = true;
	else{
		printk("Error couldn't find an open block to write to\n");
		return -1;
	}

	return 0;
}

void print_tree(struct block* root){
	if(root){
		print_tree(root->Lchild);
		if(!root->isParent){
			printk(KERN_INFO "Block Print:\n\tsize: %d\n\taddress: %d\n\toccupied: %s\n\tparent: %s\n\n", root->size, root->address, root->occupied ? "true" : "false", root->isParent ? "true" : "false");
		}
		print_tree(root->Rchild);
	}
}

//recursively find block containing the input reference
struct block* findBlock(struct block* parent, int ref){
	struct block* returnBlock;
	returnBlock = NULL;
	if(parent){
		if(!parent->isParent) returnBlock = parent;
		else if(ref < parent->address + parent->size / 2) returnBlock = findBlock(parent->Lchild, ref);
		else returnBlock = findBlock(parent->Rchild, ref);
	}
	return returnBlock;
}

//find the smallest empty block in tree that can hold the given size
int find_empty(struct block* parent, int size){
	if(parent){
		find_empty(parent->Lchild, size);
		if(!parent->occupied && !parent->isParent && parent->size >= size)
		{
			if(!selectedBlock || (selectedBlock->size > parent->size)) selectedBlock = parent;
		}
		find_empty(parent->Rchild, size);
	}
	if (selectedBlock) return selectedBlock->address;
	return -1;
}

//split a block if it is too big for the input
void split_block(struct block* parent){
	int newSize;
	newSize = parent->size / 2;
	if(parent->Lchild == NULL){
		parent->Lchild = create_block(parent->address, newSize);
		parent->Lchild->parent = parent;
	}
	if(parent->Rchild == NULL){
		parent->Rchild = create_block(parent->address + newSize, newSize);
		parent->Rchild->parent = parent;
	}
	parent->isParent = true;
}

#endif

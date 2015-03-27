/*

Tomasz Flendrich

*/

#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define DEBUG // to enable debug prints
//#define FOR_LIBRARY_USAGE // to get rid of the main() function
//#define CRASH_THIS_LIB // to see that it is in use


#ifdef DEBUG
#define DEBUG_PRINT(x) x
#else
#define DEBUG_PRINT(x)
#endif

#ifdef CRASH_THIS_LIB
#define CRASH_THIS_LIB_MACRO(x) x
#else
#define CRASH_THIS_LIB_MACRO(x)
#endif



#include <stdbool.h>

#define GRANULARITY 8

/* gcc zad2.c && ./a.out
to check the memory usage during running: "ps -av" or "top"

LD_PRELOAD=/home/tomek/SO/P4/zad2/libmyownmalloc.so
or this:
LD_LIBRARY_PATH=/path/to/my/malloc.so /bin/ls


TO-DO:
	change size to size_t instead of an integer
	write a test generator
*/
typedef struct superBlock
{
	int size; // size of block in superBlock,including size of superBlock
	int wholeSize;
	struct superBlock* next; // when list has 1 element, next=prev=beginningSuperBlock
	struct superBlock* prev;
}superBlock;

struct superBlock* beginningSuperBlock = NULL; // when list has 0 elements, this is NULL

void* malloc(size_t size);
void* calloc(size_t count, size_t size);
void* realloc(void *ptr, size_t size);
void free(void *ptr);


void mergeBlocks(superBlock* sb);
bool canBeBigger(superBlock* sb, size_t size);
void printMemory();



void addElement(superBlock* element) // adds an element to an already existing list (can be empty)
{
	assert(element != NULL);
	DEBUG_PRINT(printf("Ktos dodaje nowy blok o rozmiarze %d\n", element->size);)

	//check if the list is empty
	if (beginningSuperBlock == NULL)
	{
		beginningSuperBlock = element;
		element->next = element;
		element->prev = element;
		return;
	}
	// case of 1-element list
	if (beginningSuperBlock->next == beginningSuperBlock)
	{
		beginningSuperBlock->next = element;
		beginningSuperBlock->prev = element;

		element->next = beginningSuperBlock;
		element->prev = beginningSuperBlock;
		if (element < beginningSuperBlock) // sorting
		{
			beginningSuperBlock = element;
		}

		return;
	}
	
	// case of a longer list
	if (beginningSuperBlock>element)
	{
		DEBUG_PRINT(printf("Czyli dodajemy cos na sam poczatek listy wolnych blokow\n");)
		element->next = beginningSuperBlock;
		element->prev = beginningSuperBlock->prev;
		(beginningSuperBlock->prev)->next = element;
		beginningSuperBlock->prev = element;
		beginningSuperBlock = element;
		return;
	}
	struct superBlock* ptr = beginningSuperBlock;

	while (ptr->next < element && ptr->next != beginningSuperBlock)
	{
		ptr = ptr->next;
	}
	// we are in a place where we put an element between ptr and ptr->next
	element->next = ptr->next;
	element->prev = ptr;
	(ptr->next)->prev = element;
	(ptr)->next = element;
	if (element < beginningSuperBlock) // sorting
	{
		beginningSuperBlock = element;
	}
	assert(element->prev != NULL);
	assert(element->next != NULL);
	assert(beginningSuperBlock != NULL);
	assert(beginningSuperBlock->next != NULL);
	assert(beginningSuperBlock->prev != NULL);
	assert(element->prev == ptr);
	assert(element == ptr->next);
}

void deleteElement(superBlock* element)
{
	// case of empty list
	if (beginningSuperBlock == 0)
		exit(1);

	// case of 1-element list
	if (element->next == element->prev && element->next == beginningSuperBlock && element == beginningSuperBlock)
	{
		//assert(element == beginningSuperBlock);
		DEBUG_PRINT(printf("Usuwamy cos z listy 1-elementowej\n");)
		beginningSuperBlock = NULL;
		return;
	}

	// case of a list longer than 2 elements
	DEBUG_PRINT(printf("Usuwamy cos z listy >=2 elementowej\n");)
	superBlock* pr = element->prev;
	superBlock* ne = element->next;

	pr->next = ne;
	ne->prev = pr;
	if (element == beginningSuperBlock)
		beginningSuperBlock = element->next;
	return;
}





void* malloc(size_t size)
{
	CRASH_THIS_LIB_MACRO( *((int*)123) = 35);

	DEBUG_PRINT(printf("*Someone told me to allocate %zu bytes (+sizeof)\n", size);)
	DEBUG_PRINT(printMemory();)

	// check if it fits in the current one
	if (beginningSuperBlock != NULL)
	{
		superBlock* p = beginningSuperBlock;
		do
		{
			if (p->size - sizeof(superBlock) >= size) // it fits in the current one
			{
				DEBUG_PRINT(printf("So it fits in some free block. This block has %d free bytes, from which we have to substract sizeof\n", p->size);)
				// check, if the block after partition is big enough to become another block
				if (p->size - sizeof(superBlock) - size < sizeof(superBlock)) // so we take the whole block
				{
					DEBUG_PRINT(printf("So the block after partition is too small, so we take whole block!\n");)
					deleteElement(p);
					DEBUG_PRINT(printf("The size of this small block is: %d\n", p->size);)
					return (void*)p + sizeof(superBlock);
				}
				else // we split the block in two
				{
					superBlock* newBlock = (void*)p + sizeof(superBlock) + size;
					newBlock->wholeSize = p->wholeSize;
					newBlock->size = p->size - sizeof(superBlock) - size;

					p->size = size + sizeof(superBlock);
					deleteElement(p);
					addElement(newBlock);
					return (void*)p + sizeof(superBlock);
				}
			}
			p = p->next;
		}
		while (p->next != beginningSuperBlock);
	}



	// a new allocation is needed
	void* ptr = mmap(NULL, size+sizeof(struct superBlock), PROT_EXEC | PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	if (ptr == MAP_FAILED)
	{
		printf("mmap() failed!\n\n\n");
		exit(1);
	}

	struct superBlock* sb = ptr;
	sb->size = size + sizeof(struct superBlock);
	sb->wholeSize = size + sizeof(struct superBlock);

	return ptr + sizeof(struct superBlock);
}
void free(void *notMovedPtr)
{
	DEBUG_PRINT(printf("*Someone told me to free()\n");)
	if (notMovedPtr == NULL)
	{
		DEBUG_PRINT(printf("Someone is using free(NULL)!\n\n");)
		return;
	}
	void* ptr = notMovedPtr;
	struct superBlock* sb = notMovedPtr - sizeof(struct superBlock);
	DEBUG_PRINT(printf("*Someone told me to free() a block of size:%d\n", sb->size);)
	DEBUG_PRINT(printMemory();)

	addElement(sb);

	mergeBlocks(sb);
}

void mergeBlocks(superBlock* sb) // także zwalnia blok, gdy jest caly
{
	if (sb->size == sb->wholeSize) // zwolnienie bloku
	{
		assert(((void*)sb) + sb->size != sb->next);
		DEBUG_PRINT(printf("I free ablock of size %d, without merging!\n", sb->size);)
		deleteElement(sb);

		if (munmap(sb, sb->wholeSize) == -1)
		{
			printf("Munmap failed!\n\n\n");
			exit(1);
		}

		return;
	}


	// merge left
	while (sb->prev != beginningSuperBlock->prev && ((sb->prev)->size + ((void*)(sb->prev)) == sb)) // check this
	{	
		DEBUG_PRINT(printf("I am merging something to the left!\n");)
		superBlock* left = (sb->prev);
		superBlock* right = sb;

		left->next = right->next;
		left->size += right->size;

		sb = left;

		if (sb->size == sb->wholeSize) // unallocating the block
		{
			deleteElement(sb);
			if (munmap(sb, sb->wholeSize) == -1)
			{
				printf("Munmap failed!\n\n\n");
				exit(1);
			}
			return;
		}
	}

	// merge right

	while (sb->next != beginningSuperBlock && (sb->size + (void*)sb == sb->next))
	{
		DEBUG_PRINT(printf("I am merging something to the right!\n");)
		superBlock* left = sb;
		superBlock* right = sb->next;

		left->next = right->next;
		left->size += right->size;
		(right->next)->prev = left;
		assert(beginningSuperBlock != right); // because left is more to the left and left is already added, so the beginning block should have changed

		if (sb->size == sb->wholeSize) // deleting a block
		{
			deleteElement(sb);
			munmap(sb, sb->wholeSize);
			return;
		}
		// no change to sb now
	}

}

void* realloc(void *ptr, size_t size)
{
	DEBUG_PRINT(printf("Ktos mi kazal realloca zrobic\n");)
	if (ptr == NULL)
	{
		DEBUG_PRINT(printf("Ktos mi kazal realloca zrobic na ptr = NULL\n");)
		return malloc(size);
	}

	if (size == 0)
	{
		DEBUG_PRINT(printf("Ktos mi kazal realloca zrobic na size = 0 (czyli zasadniczo chca free() na ptr)\n");)
		free(ptr); 
		return NULL;
	}

	DEBUG_PRINT(printf("*Realokacja na %zu bajtów (+ sizeof). Wcześniej było ich %zu (+sizeof)\n", size, ((superBlock*)(ptr - sizeof(superBlock)))->size - sizeof(superBlock));)

	superBlock* newSB = ptr + size;
	if (ptr == NULL)
	{
		return malloc(size);
	}

	
	superBlock* sb = ptr - sizeof(superBlock);
	if (size < sb->size - sizeof(superBlock)) // we want it smaller
	{
		DEBUG_PRINT(printf("Chcę pomniejszyć swój blok!\n");)

		// case when the second block is too small to have full info. Then the current block must be bigger
		if (sb->size - size - sizeof(superBlock) < sizeof(superBlock))
		{
			DEBUG_PRINT(printf("Przy pomniejszaniu nie ma sensu tworzyc nowego bloku - bylby za maly\n");)
			return ptr;
		}
		else // everything is all right, we can split in two
		{
			DEBUG_PRINT(printf("Przy pomniejszaniu tworze drugi blok\n");)
			superBlock* newSB = ptr + size;
			newSB->size = sb->size - size - sizeof(superBlock);
			newSB->wholeSize = sb->wholeSize;

			addElement(newSB);
			sb->size = size+sizeof(superBlock);
			return ptr;
		}


	}

	if (size > sb->size - sizeof(superBlock)) // we want it bigger
	{
		DEBUG_PRINT(printf("Chcę powiększyć swój blok!\n");)
		// making it bigger fits
		if (canBeBigger(sb, size))
			return ptr;



		// making it bigger doesn't fit
		DEBUG_PRINT(printf("Powiekszenie sie nie zmiesci, wiec znajde inne miejse mallocem()\n");)
		void* returnPtr = malloc(size);


		memcpy(returnPtr, ptr, size);
		addElement(((void*)ptr) - sizeof(superBlock));
		mergeBlocks(((void*)ptr) - sizeof(superBlock));
		return returnPtr;
	}

	// the size didn't change
	return ptr;
}

bool canBeBigger(superBlock* sb, size_t size) // this function creates another block if it can't be made bigger
{
	superBlock* ptr = beginningSuperBlock;
	if (ptr == NULL)
		return false;

	while (ptr->next < sb && ptr->next != beginningSuperBlock)
	{
		ptr = ptr->next;
	}
	ptr = ptr->next; // because we want it bigger
	if ((void*)sb + sb->size == ptr && size <= (sb->size - sizeof(superBlock) + ptr->size)) // check if they are near each other && if it's big enough
	{
		DEBUG_PRINT(printf("Czyli jest miejsce na powiekszenie\n");)
		// case when it's too small for a superblock to be there
		if (ptr->size + sb->size - sizeof(superBlock) - size < sizeof(superBlock))
		{
			deleteElement(ptr);
			sb->size = sb->size + ptr->size;
			return true;
		}
		else // so we split in two
		{
			deleteElement(ptr);
			superBlock* newBlock = (void*)sb + sizeof(superBlock) + size;
			newBlock->wholeSize = ptr->wholeSize;
			newBlock->size = ptr->size - (size + sizeof(superBlock) - sb->size);
			sb->size = size + sizeof(superBlock);
			addElement(newBlock);
			return true;

		}
	}
	return false;
}


void checkMemory(void* ptr, size_t size) // checks for segfaults
{
	char tab[size];
	memcpy(tab, ptr, size);
}

void printMemory() // also tests the memory
{

	printf("\nPRINTING MEMORY:\n");
	if (beginningSuperBlock == NULL) // 0 blocks
	{

		printf("No free blocks.\n");
		printf("END OF MEMORY:\n\n");
		return;
	}

	// 1 block
	if (beginningSuperBlock->next == beginningSuperBlock)
	{
		assert(beginningSuperBlock->prev == beginningSuperBlock);

		printf("Block: %d WholeSize: %d\n", beginningSuperBlock->size, beginningSuperBlock->wholeSize);
		printf("END OF MEMORY:\n\n");
		checkMemory((void*)beginningSuperBlock+sizeof(superBlock), beginningSuperBlock->size - sizeof(superBlock));
		return;
	}

	superBlock* ptr = beginningSuperBlock;
	assert(beginningSuperBlock->next != beginningSuperBlock);
	while(ptr->next != beginningSuperBlock) // more than 1 block
	{
		printf("Block: %d WholeSize: %d\n", ptr->size, ptr->wholeSize);
		checkMemory((void*)ptr+sizeof(superBlock), ptr->size - sizeof(superBlock));

		ptr = ptr->next;
	}
	printf("Block: %d WholeSize: %d\n", ptr->size, ptr->wholeSize);

	printf("END OF MEMORY:\n\n");
}

void* calloc(size_t nmemb, size_t size) // make sure it works
{
	return malloc(nmemb*size);
}

void UT1()
{
	char* ptr = malloc(1000);
	checkMemory(ptr, 1000);

	ptr = realloc(ptr, 500);
	checkMemory(ptr, 500);

	char* ptr2 = malloc(600);
	checkMemory(ptr2, 600);

	ptr2 = realloc(ptr2, 300);
	checkMemory(ptr2, 300);

	ptr2 = realloc(ptr2, 400);
	checkMemory(ptr2, 400);
	
	free(ptr);

	free(ptr2);

	assert(beginningSuperBlock == NULL);

}

void UT2()
{

	char* ptr = malloc(1000);
	checkMemory(ptr, 1000);

	ptr = realloc(ptr, 500);
	checkMemory(ptr, 500);

	char* ptr2 = malloc(600);
	checkMemory(ptr2, 600);

	ptr2 = realloc(ptr2, 300);
	checkMemory(ptr2, 300);

	ptr2 = realloc(ptr2, 400);
	checkMemory(ptr2, 400);

	char* ptr3 = malloc(300);
	checkMemory(ptr3, 300);

	free(ptr);

	free(ptr2);
	free(ptr3);

	assert(beginningSuperBlock == NULL);

}
void UT3()
{
	char* ptr = malloc(1000);
	checkMemory(ptr, 1000);
	superBlock* sb = (void*)ptr - sizeof(superBlock);
	assert(sb->size == 1000 + sizeof(superBlock));
	free(ptr);
}
void UT4()
{
	char* ptr = malloc(1000);
	checkMemory(ptr, 1000);

	ptr = realloc(ptr, 500);
	checkMemory(ptr, 500);

	assert(beginningSuperBlock->size == 500);

	free(ptr);

}

void UT5()
{
	char* ptr = malloc(1000);
	checkMemory(ptr, 1000);

	ptr = realloc(ptr, 500);
	checkMemory(ptr, 500);

	char* ptr2 = malloc(600);
	checkMemory(ptr2, 600);

	ptr2 = realloc(ptr2, 300);
	checkMemory(ptr2, 300);

	ptr2 = realloc(ptr2, 400);
	checkMemory(ptr2, 400);

	char* ptr3 = malloc(300);
	checkMemory(ptr3, 300);

	char* ptr4 = malloc(100);
	checkMemory(ptr4, 100);

	char* ptr5 = malloc(8);
	checkMemory(ptr5, 8);

	ptr5 = realloc(ptr5, 15);
	checkMemory(ptr5, 15);

	ptr5 = realloc(ptr5, 43);
	checkMemory(ptr5, 43);
	

	free(ptr);
	free(ptr2);
	free(ptr3);
	free(ptr4);
	free(ptr5);
	assert(beginningSuperBlock == NULL);
}

void UT6()
{
	// merging after realloc to a bigger size
	char* ptr = malloc(1000);
	checkMemory(ptr, 1000);

	ptr = realloc(ptr, 500);
	checkMemory(ptr, 500);

	char* ptr2 = malloc(600);
	checkMemory(ptr2, 600);

	char* ptr3 = malloc(300);
	checkMemory(ptr3, 300);

	ptr2 = realloc(ptr2, 10000);

	free(ptr);
	free(ptr2);
	free(ptr3);
	assert(beginningSuperBlock == NULL);

}

void UT7()
{
	char* ptr = malloc(1000);
	checkMemory(ptr, 1000);

	ptr = realloc(ptr, 500);
	checkMemory(ptr, 500);

	assert(beginningSuperBlock->size == 500);
	char* ptr2 = malloc(300);
	free(ptr);
	free(ptr2);
	assert(beginningSuperBlock == NULL);
	

}

void UT8() // merging to both sides
{
	char* ptr = malloc(10000);
	checkMemory(ptr, 10000);

	ptr = realloc(ptr, 100);

	char* ptr1 = malloc(100);
	char* ptr2 = malloc(100);
	char* ptr3 = malloc(1100);
	char* ptr4 = malloc(170);
	char* ptr5 = malloc(10);
	char* ptr6 = malloc(100);
	char* ptr7 = malloc(110);
	char* ptr8 = malloc(7500);
	char* ptr9 = malloc(1280);
	free(ptr);

	free(ptr3);
	free(ptr6);
	free(ptr2);
	free(ptr5);
	free(ptr1);
	free(ptr9);
	free(ptr8);
	free(ptr7);
	free(ptr4);


	assert(beginningSuperBlock == NULL);
	

}
void ShowThatMemoryLeaksWouldReallyShowUpOnATest()
{

	char* ptr1 = malloc(1); // it also allocates sizeof(superBlock)
	// no free
}


#ifndef FOR_LIBRARY_USAGE
int main()
{
	for (int i = 0; i < 1000*1000; i++)
	{
		UT1();
		UT2();
		UT3();
		UT4();
		UT5();
		UT6();
		UT7();
		UT8();
		//ShowThatMemoryLeaksWouldReallyShowUpOnATest();
	}


	printf("\n\nYay\n");


	return 0;
}
#endif
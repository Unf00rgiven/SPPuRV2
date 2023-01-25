/**
 * @file stdlib.h
 * @author Dejan Bogdanovic
 * 
 * Lightweight stdlib
 *
 * This file contains memory allocation and deallocation functions.
 * It should be replaced with C's library.
 */

#ifndef STDLIB_H
#define STDLIB_H

#include <stdint.h>

/**
 * Memory block structure.
 *
 * At the beginning, memory is one large chunk of free memory.
 * Tasks request some chunks and memory is fragmented. When tasks
 * Return these chunks, they are linked in list.
 */
typedef struct s_mem_block_t{
	uint32_t _size;					/**< Chunk size */
	struct s_mem_block_t * _next;	/**< Pointer to next chunk */
} mem_block_t;


#define HEAP_START 	0x100000 		/**< Address where heap starts */
#define MAX_HEAP	0x10000 		/**< Heap length */

#ifndef NULL 
#define NULL 0 						/**< NULL pointer */
#endif

/**
 * Function that initializes heap.
 */
extern void mem_init(void);

/**
 * Allocate a chunk of memory.
 *
 * This function searches all the free chunks for big enough chunk that meets the 
 * needs. Chunk is then split. One piece is returned and the other one is linked
 * back in list.
 *
 * @param n chunk size
 * @return pointer to memory that is allocated
 */
extern void * malloc(uint32_t n);

/**
 * Free a chunk of memory.
 *
 * This function finds a place in linked list of free chunks where the chunk will be
 * inserted.
 *
 * @param ptr pointer to memory that is freed
 */
extern void free(void * ptr);

/**
 * Copy memory block.
 *
 * @param to destination adress
 * @param from source adress
 * @param len length of area that is copied
 */
extern void memcp(char * to, char * from, uint32_t len);

/**
 * Set memory block.
 *
 * @param b block adress
 * @param c byte that will be copied to every byte of block
 * @param len length of block
 */
extern void memse(char * b, char c, uint32_t len);

/**
 * Compare memory.
 *
 * @param a address of the first block 
 * @param b address of the second block
 * @param len length of area that is compared
 * @return 0 if they are different and 1 if they are equal
 */
extern uint32_t memcm(char * a , char * b, uint32_t len);

#endif // STDLIB_H

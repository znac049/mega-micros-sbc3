#include <stdlib.h>
#include <stdio.h>

extern uint32_t _end;

uint32_t heap_start;

struct heap_chunk {
	int size;
	struct heap_chunk *next;
};

typedef struct heap_chunk heap_chunk_t;

static heap_chunk_t *main_heap = NULL;

void heap_print_free()
{
	for (heap_chunk_t *cur = main_heap; cur != NULL; cur = cur->next) {
        ;
	} 
}

void _init_heap(void) {
    main_heap = (heap_chunk_t *) _end;

    main_heap->size = 0x3fffff - _end;
    main_heap->next = NULL;

    heap_print_free();
}

void *malloc(size_t size)
{
	heap_chunk_t *cur;

	// Align the size to 4 bytes
	size += ((4 - (size & 0x3)) & 0x3);
	int block_size = size + sizeof(heap_chunk_t);

	for (cur=main_heap; cur!=NULL; cur=cur->next) {
        /* If the block can be split with enough room for another block struct and more than 8 bytes left over, then split it */
        if (cur->size >= block_size + sizeof(heap_chunk_t) + 8) {
            // Grab a bit of the chunk
            heap_chunk_t *new_chunk = (heap_chunk_t *)((char *)cur + cur->size - block_size);
            new_chunk->size = block_size;
            cur->size = cur->size - block_size;

            return ((char *) new_chunk) + sizeof(heap_chunk_t);
        }
	}

    return NULL;
}

void free(void *ptr)
{
	heap_chunk_t *prev = NULL;
	heap_chunk_t *block = ((heap_chunk_t *) ptr) - 1;
	heap_chunk_t *cur;

	for (cur=main_heap; cur!=NULL; prev=cur, cur=cur->next) {
        if ((char *) cur > ((char *) ptr)) {
            /* Can we merge with previous and/or next? */

            if (((((char *) prev )) + prev->size + 1) == (char *) block) {
                /* We can merge with the previous */
                prev->size = prev->size + block->size;
                block = prev;
            } 
            
            if (((((char *) block)) + block->size + 1) == (char *) cur) {
                /* We can merge with the following block */
            }
        }
	}
}

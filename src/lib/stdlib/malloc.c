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

#define CP(ptr) ((char *)ptr)

uint32_t get_heap_start(void);

void heap_print_free(void)
{
    int i=0;

    printf("\nHeap free list:\n");
    printf("  End of BSS is 0x%08x\n", get_heap_start());
	for (heap_chunk_t *cur = main_heap; cur != NULL; cur = cur->next) {
        printf("Chunk #%d: 0x%08x, (%d)\n", i++, CP(cur), cur->size);
	} 
    printf("\n");
}

void _init_heap(void) {
    main_heap = (heap_chunk_t *) get_heap_start();

    main_heap->size = 0x3fffff - get_heap_start();
    main_heap->next = NULL;

    //heap_print_free();
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
    bool_t merged = FALSE;

    //printf("free(0x%08x)\n", ptr);
 
	for (cur=main_heap; cur!=NULL; prev=cur, cur=cur->next) {
        // Does the block being sit next to the end of an existing block?
        if ((CP(cur) + cur->size) == CP(block)) {
            // Simply merge the two
            //printf("free: Merge with previous free block %08x (%d)\n", cur, cur->size);
            cur->size += block->size;
            merged = TRUE;

            // Does the extended block extend to the start of the next block?
            if ((CP(cur) + cur->size) == CP(cur->next)) {
                //printf("free: Also merge with next free block %08x (%d)\n", cur->next, cur->next->size);
                cur->size += cur->next->size;
                cur->next = cur->next->next;

                return;
            }
        }
        else if ((CP(block) + block->size) == CP(cur->next)) {
            //printf("free: Merge with following block %08x (%d)\n", cur->next, cur->next->size);
            block->size += cur->next->size;
            cur->next = block;

            return;
        }
	}

    if (!merged) {
        // The block isn't adjacent to any existing free blocks - slot it into the free list

        //printf("free: Not adjacent to anything :-(\n");

        if (main_heap == NULL) {
            //printf("free: Adding at start of list\n");
            main_heap = block;
            block->next = NULL;
            return;
        }
        else {
            for (cur=main_heap; cur!=NULL; prev=cur, cur=cur->next) {
                if (CP(cur) > CP(block)) {
                    //printf("free: Adding to free list after %08x (%d)\n", prev, prev->size);

                    prev->next = block;
                    block->next = cur;

                    return;
                }
            }

            //printf("free: Adding to the end of the free list\n");
            prev->next = block;
            block->next = NULL;
        }
    }
}

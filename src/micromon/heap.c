/* 
MIT License

Copyright (c) 2026 Bob Green

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <duart.h>
#include <errno.h>

uint32_t heap_start;

struct heap_chunk {
	uint32_t size;
    pid_t owner;
	struct heap_chunk *next;
};

typedef struct heap_chunk heap_chunk_t;

static heap_chunk_t *free_heap = NULL;
static heap_chunk_t *allocated_chunks = NULL;

#define CP(ptr) ((char *)ptr)

uint32_t get_heap_start(void);

static void dump_heap_item(heap_chunk_t *chunk) {
    kprintf("Chunk @0x%08x:\n", chunk);
    kprintf("  size:           %d\n", chunk->size);
    kprintf("  owner pid:      %d\n", chunk->owner);
    kprintf("  next:           0x%08x\n", chunk->next);
}

static void dump_heap(void) {
    kprintf("\nHeap free list:\n");
    kprintf("  End of BSS is 0x%08x\n", get_heap_start());
    kprintf("FREE:\n");
	for (heap_chunk_t *cur = free_heap; cur != NULL; cur = cur->next) {
        dump_heap_item(cur);
	} 
    kprintf("\n");

    kprintf("ALLOCATED:\n");
	for (heap_chunk_t *cur = allocated_chunks; cur != NULL; cur = cur->next) {
        dump_heap_item(cur);
	} 
    kprintf("\n");
}

void _init_heap(void) {
    free_heap = (heap_chunk_t *) get_heap_start();

    free_heap->size = 0x3fffff - get_heap_start();
    free_heap->owner = 0;
    free_heap->next = NULL;

    dump_heap();
}

void *bios_malloc(size_t size, pid_t pid) {
	heap_chunk_t *cur;

	// Align the size to 4 bytes
	size += ((4 - (size & 0x3)) & 0x3);
	int block_size = size + sizeof(heap_chunk_t);

	for (cur=free_heap; cur!=NULL; cur=cur->next) {
        /* If the block can be split with enough room for another block struct and more than 8 bytes left over, then split it */
        if (cur->size >= block_size + sizeof(heap_chunk_t) + 8) {
            // Grab a bit of the chunk
            heap_chunk_t *new_chunk = (heap_chunk_t *)((char *)cur + cur->size - block_size);
            new_chunk->size = block_size;
            new_chunk->owner = pid;
            cur->size = cur->size - block_size;

            new_chunk->next = allocated_chunks;
            allocated_chunks = new_chunk;

            kprintf("malloc(%d) -> 0x%08x\n", size, ((char *) new_chunk) + sizeof(heap_chunk_t));
            dump_heap();

            return ((char *) new_chunk) + sizeof(heap_chunk_t);
        }
	}

    return NULL;
}

void bios_free(void *ptr, pid_t pid)
{
	heap_chunk_t *prev = NULL;
	heap_chunk_t *block = ((heap_chunk_t *) ptr) - 1;
	heap_chunk_t *cur;
    bool_t merged = FALSE;

    if (ptr == NULL) {
        return;
    }

    //printf("free(0x%08x)\n", ptr);

    // remove it from the list of allocated chunks.
    for (cur=allocated_chunks; cur != NULL; prev=cur, cur=cur->next) {
        if (cur == block) {
            if (cur->owner != pid) {
                kprintf("sys_free: found block but it has the wrong pid\n");
            }

            if (cur == allocated_chunks) {
                allocated_chunks = cur->next;
            }
            else {
                prev->next = cur->next;
            }

            break;
        }
    }
 
	for (cur=free_heap, prev=NULL; cur!=NULL; prev=cur, cur=cur->next) {
        // Does the block sit next to the end of an existing block?
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

        if (free_heap == NULL) {
            //printf("free: Adding at start of list\n");
            free_heap = block;
            block->next = NULL;
            return;
        }
        else {
            for (cur=free_heap; cur!=NULL; prev=cur, cur=cur->next) {
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

void clean_heap(pid_t pid) {
	heap_chunk_t *cur;
 	heap_chunk_t *prev = NULL;

   for (cur=allocated_chunks; cur != NULL; prev=cur, cur=cur->next) {
        if (cur->owner == pid) {
            if (cur == allocated_chunks) {
                allocated_chunks = cur->next;
            }
            else {
                prev->next = cur->next;
            }

            free(cur);
        }
    }
}
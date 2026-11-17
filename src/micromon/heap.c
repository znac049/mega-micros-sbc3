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
#include <errno.h>

#include "micromon.h"
#include "memory.h"

#define ITEM_SIG 0xfac3b00b
#define SMALLEST_PAYLOAD 32

#define CP(ptr) ((char *)ptr)


uint32_t heap_start;

struct heap_info {
    int num_free_items;
    int num_allocated_items;
};

typedef struct heap_info heap_info_t;

struct list_item {
    uint32_t signature;
	uint32_t payload_size;                      // of the payload (exluding header)
    pid_t owner;
	struct list_item *next;
    struct list_item *prev;
};

typedef struct list_item list_item_t;

static list_item_t *free_items = NULL;
static list_item_t *allocated_items = NULL;
static heap_info_t heap_info;

uint32_t get_heap_start(void);

static inline void dump_item(list_item_t *item) {
    printk("item @0x%08x: ps=%6d, o=%3d, p=0x%08x, n=0x%08x\n", 
                item, item->payload_size, item->owner, item->prev, item->next);

}

void dump_heap(void) {
    uint32_t tot_size = 0;

    printk("\nHeap:  free = 0x%08x, num=%d, alloc = 0x%08x, num=%d\n",
        free_items, heap_info.num_free_items,
        allocated_items, heap_info.num_allocated_items);
    // printk("  End of BSS is 0x%08x\n", get_heap_start());

    printk("\nFREE:\n");
	for (list_item_t *cur = free_items; cur != NULL; cur = cur->next) {
        dump_item(cur);
        tot_size += cur->payload_size + sizeof(list_item_t);
	} 
    printk("\n");

    printk("ALLOCATED:\n");
	for (list_item_t *cur = allocated_items; cur != NULL; cur = cur->next) {
        dump_item(cur);
        tot_size += cur->payload_size + sizeof(list_item_t);
	} 
    printk("\nHeap dump complete. Total bytes in lists=%d (%06x)\n", tot_size, tot_size);
}

void init_heap(void) {
    free_items = NULL;
    allocated_items = NULL;

    heap_info.num_free_items = 0;
    heap_info.num_allocated_items = 0;

    /*
     * All of the free memory in Bank 0, from the end of the .bss. Bank 1
     * is added later, with heap_add_region(), once the boot-time RAM test
     * has passed it.
     */
    heap_add_region(get_heap_start(), ram_end);

    // printk("init_heap: ");
    // dump_heap();
}

/*
 * Add start..end (inclusive) to the heap as one free block, keeping the
 * free list in address order. The memory must not overlap anything
 * already in the heap.
 */
void heap_add_region(uint32_t start, uint32_t end) {
    list_item_t *item;
    list_item_t *prev = NULL;
    list_item_t *next = free_items;

    start = (start + 3) & ~3U;
    if ((end < start) || ((end - start + 1) < sizeof(list_item_t) + SMALLEST_PAYLOAD)) {
        return;
    }

    item = (list_item_t *)start;
    item->signature = ITEM_SIG;
    item->payload_size = ((end + 1 - start) & ~3U) - sizeof(list_item_t);
    item->owner = 0;

    while ((next != NULL) && (next < item)) {
        prev = next;
        next = next->next;
    }

    item->prev = prev;
    item->next = next;

    if (prev == NULL) {
        free_items = item;
    }
    else {
        prev->next = item;
    }

    if (next != NULL) {
        next->prev = item;
    }

    heap_info.num_free_items++;
}

/*
 * The payload of the n'th free block (0 = lowest), for the memtest
 * command: the memory in it isn't in use, though its header is.
 * Returns OK, or NOT_OK if there aren't that many free blocks.
 */
int heap_free_region(int n, uint32_t *start, uint32_t *end) {
    list_item_t *item = free_items;

    while ((item != NULL) && (n-- > 0)) {
        item = item->next;
    }

    if (item == NULL) {
        return NOT_OK;
    }

    *start = (uint32_t)item + sizeof(list_item_t);
    *end = *start + item->payload_size - 1;

    return OK;
}

static int is_valid_item(list_item_t *item) {
    if ((item == NULL) || (item->signature != ITEM_SIG)) {
        return NO;
    }

    return YES;
}

static void remove_from_list(list_item_t *item, list_item_t **head) {
    list_item_t *prev = item->prev;
    list_item_t *next = item->next;

    if (prev == NULL) {
        /* Item was the list head */
        *head = next;
        if (next != NULL) {
            next->prev = NULL;
        }
    }
    else {
        /* There is an item before us in the list */
        prev->next = next;
        if (is_valid_item(next)) {
            next->prev = prev;
        }
    }
}

static void remove_from_free(list_item_t *item) {
    remove_from_list(item, &free_items);
    heap_info.num_free_items--;
}

static void remove_from_allocated(list_item_t *item) {
    remove_from_list(item, &allocated_items);
    heap_info.num_allocated_items--;
}

static void insert_in_allocated(list_item_t *item) {
    /*
     * Unlike the free list, the allocated list does not need
     * to be sorted so kust tack it onto the start
     */

     item->prev = NULL;
     item->next = allocated_items;
     if (allocated_items != NULL) {
        allocated_items->prev = item;
     }

     allocated_items = item;

     heap_info.num_allocated_items++;
}

static inline int can_merge(list_item_t *first, list_item_t *second) {
    uint32_t end_first;
    
    if ((first == NULL) || (second == NULL)) {
        return NO;
    }

    end_first = (uint32_t)first + first->payload_size + sizeof(list_item_t);
    if ((uint32_t)second == end_first) {
        return YES;
    }

    return NO;
}

static inline int free_item(list_item_t *item) {
    list_item_t *prev = NULL;

    remove_from_allocated(item);

    item->next = NULL;
    item->prev = NULL;
    item->owner = 0;

    /*
     * Add it to the free list or alternatively, merge it into
     * an adjacent item
     */

    /* Special case. Does it need to be inserted at the start of the list */
    if ((free_items == NULL) || (item < free_items)) {
        if (can_merge(item, free_items) == YES) {
            item->payload_size += free_items->payload_size + sizeof(list_item_t);
            item->next = free_items->next;
            if (free_items->next != NULL) {
                free_items->next->prev = item;
            }
        }
        else {
            item->next = free_items;
            if (free_items != NULL) {
                free_items->prev = item;
            }

            heap_info.num_free_items++;
        }

        free_items = item;

        return OK;
    }

    for (list_item_t *cur=free_items; cur!=NULL; cur=cur->next) {
        if ((item > cur) && (item < cur->next)) {
            /* 
             * We've found our place in the list, but before adding
             * item, see if it can be merged with either of it's
             * neighbours
             */

            if (can_merge(cur, item) == YES) {
                /* Simple merge: item gets added to cur */
                cur->payload_size += item->payload_size + sizeof(list_item_t);

                /* Can we now merge the resultant merged item with the next item? */
                if (can_merge(cur, cur->next) == YES) {
                    list_item_t *next = cur->next;

                    cur->payload_size += next->payload_size + sizeof(list_item_t);

                    cur->next = next->next;
                    if (next->next != NULL) {
                        next->next->prev = cur;
                    }

                    heap_info.num_free_items--;
                }

                return OK;
            }
            else if (can_merge(item, cur->next) == YES) {
                /* More complicated merge: next gets added to item and then item replaces next */
                item->payload_size += cur->next->payload_size + sizeof(list_item_t);

                item->next = cur->next->next;
                item->prev = cur;

                cur->next = item;
                if (cur->next->next != NULL) {
                    cur->next->next->prev = item;
                }

                return OK;
            }

            /* Merge not possible: slot it into the list */
            item->prev = cur;
            item->next = cur->next;
            if (cur->next != NULL) {
                cur->next->prev = item;
            }

            cur->next = item;
            heap_info.num_free_items++;

            return OK;
        }

        prev = cur;
    }

    /* We have to tack it on the end of the free list */

    if (can_merge(prev, item) == YES) {
        /* It follows on from the last free item: just make that bigger */
        prev->payload_size += item->payload_size + sizeof(list_item_t);

        return OK;
    }

    item->prev = prev;
    prev->next = item;
    heap_info.num_free_items++;

    return OK;
}

void bios_free(void *mem, pid_t pid) {
    list_item_t *item = (list_item_t *)(CP(mem) - sizeof(list_item_t));

    // printk("bios_free(%08x, %d):\n", mem, pid);

    if (is_valid_item(item) == NO) {
        return;
    }


    if (item->owner != pid) {
        errno = EPERM;
        return;
    }

    free_item(item);
}

void *bios_malloc(size_t nbytes, pid_t pid) {
    // printk("\n\n\n\nmalloc(%d, %d):\n", nbytes, pid);

    /* While debugging */
    if (nbytes == 0) {
        dump_heap();
        return NULL;
    }

    /*
     * Keep every block a whole number of longwords, so the headers and
     * payloads that follow stay longword aligned.
     */
    if (nbytes > 0x7ffffff0U) {
        errno = ENOMEM;
        return NULL;
    }
    nbytes = (nbytes + 3) & ~3U;

    /*
     * Find a chunk with enough space, starting from the highest one and
     * taking memory from its top end. That keeps allocations away from
     * the bottom of bank 0, where the monitor's stack is (the first free
     * block starts just after the .bss, below the stack) and where
     * programs are loaded ($40000 up), until memory runs short. With
     * bank 1 fitted, it's used first.
     */
    list_item_t *last = free_items;

    while ((last != NULL) && (last->next != NULL)) {
        last = last->next;
    }

    for (list_item_t *item=last; item != NULL; item=item->prev) {
        /*
         * Check each item in the list has a valid signature. 
         * Yes, it's an overhead for each malloc, but if there
         * are problems with the linked list, we want to flag that
         * as early as possible 
         */
        if (!is_valid_item(item)) {
            printk("bios_malloc: invalid item\n");
            errno = ENOMEM;
            return NULL;
        }

        /* Is this item big enough? */
        if (item->payload_size >= nbytes) {
            list_item_t *taken;

            if (item->payload_size >= (nbytes + sizeof(list_item_t) + SMALLEST_PAYLOAD)) {
                /*
                 * There's room to split it: the top of the payload
                 * becomes the new block, and the free block just gets
                 * smaller, keeping its place in the free list.
                 */
                item->payload_size -= nbytes + sizeof(list_item_t);

                taken = (list_item_t *)(CP(item) + sizeof(list_item_t) + item->payload_size);
                taken->signature = ITEM_SIG;
                taken->payload_size = nbytes;
            }
            else {
                // Take the whole block
                remove_from_free(item);
                taken = item;
            }

            taken->owner = pid;
            insert_in_allocated(taken);

            return CP(taken) + sizeof(list_item_t);
        }
    }

    errno = ENOMEM;
    return NULL;
}

/*
 * Would writing to [start, end] (inclusive) damage the heap? True if it
 * overlaps an allocated block, or the header of a free one (free payloads
 * are fair game).
 */
bool_t heap_range_in_use(uint32_t start, uint32_t end) {
    for (list_item_t *item=allocated_items; item != NULL; item=item->next) {
        uint32_t item_start = (uint32_t)item;
        uint32_t item_end = item_start + sizeof(list_item_t) + item->payload_size - 1;

        if ((start <= item_end) && (end >= item_start)) {
            return YES;
        }
    }

    for (list_item_t *item=free_items; item != NULL; item=item->next) {
        uint32_t item_start = (uint32_t)item;
        uint32_t item_end = item_start + sizeof(list_item_t) - 1;

        if ((start <= item_end) && (end >= item_start)) {
            return YES;
        }
    }

    return NO;
}

void clean_heap(pid_t pid) {
    list_item_t *next = NULL;

    // printk("clean_heap: pid=%d\n", pid);

    for (list_item_t *item=allocated_items; item!=NULL; item=next) {
        next = item->next;

        if (item->owner == pid) {
            void *p = (void *)(CP(item) + sizeof(list_item_t));
            // printk("clean_heap: reclaiming 0x%08x\n", p);
            bios_free(p, pid);
        }
    }
}



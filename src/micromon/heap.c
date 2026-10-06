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

#include "micromon.h"

#define ITEM_SIG 0xfac3b00b
#define SMALLEST_PAYLOAD 32

#define CP(ptr) ((char *)ptr)


uint32_t heap_start;

struct heap_info {
    uint32_t biggest_payload;
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
    kprintf("item @0x%08x: ps=%6d, o=%3d, p=0x%08x, n=0x%08x\n", 
                item, item->payload_size, item->owner, item->prev, item->next);

}

void dump_heap(void) {
    uint32_t tot_size = 0;

    kprintf("\nHeap:  free = 0x%08x, num=%d, alloc = 0x%08x, num=%d\n",
        free_items, heap_info.num_free_items,
        allocated_items, heap_info.num_allocated_items);
    // kprintf("  End of BSS is 0x%08x\n", get_heap_start());

    kprintf("\nFREE:\n");
	for (list_item_t *cur = free_items; cur != NULL; cur = cur->next) {
        dump_item(cur);
        tot_size += cur->payload_size + sizeof(list_item_t);
	} 
    kprintf("\n");

    kprintf("ALLOCATED:\n");
	for (list_item_t *cur = allocated_items; cur != NULL; cur = cur->next) {
        dump_item(cur);
        tot_size += cur->payload_size + sizeof(list_item_t);
	} 
    kprintf("\nHeap dump complete. Total bytes in lists=%d (%06x)\n", tot_size, tot_size);
}

void init_heap(void) {
    free_items = (list_item_t *) get_heap_start();

    free_items->signature = ITEM_SIG;
    free_items->payload_size = ram_end - get_heap_start() - sizeof(list_item_t);
    free_items->owner = 0;
    free_items->next = NULL;
    free_items->prev = NULL;

    heap_info.biggest_payload = free_items->payload_size;
    heap_info.num_free_items = 1;
    heap_info.num_allocated_items = 0;

    // kprintf("init_heap: ");
    // dump_heap();
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
     allocated_items->prev = item;

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

    item->prev = prev;
    prev->next = item;
    heap_info.num_free_items++;

    return OK;
}

void bios_free(void *mem, pid_t pid) {
    list_item_t *item = (list_item_t *)(CP(mem) - sizeof(list_item_t));

    // kprintf("bios_free(%08x, %d):\n", mem, pid);

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
    // kprintf("\n\n\n\nmalloc(%d, %d):\n", nbytes, pid);

    /* While debugging */
    if (nbytes == 0) {
        dump_heap();
        return NULL;
    }

    if (nbytes > heap_info.biggest_payload) {
        errno = ENOMEM;
        return NULL;
    }

    /* Attempt to find a chunk with enough space */
    for (list_item_t *item=free_items; item != NULL; item=item->next) {
        /*
         * Check each item in the list has a valid signature. 
         * Yes, it's an overhead for each malloc, but if there
         * are problems with the linked list, we want to flag that
         * as early as possible 
         */
        if (!is_valid_item(item)) {
            kprintf("bios_malloc: invalid item\n");
            errno = ENOMEM;
            return NULL;
        }

        /* Is this item big enough? */
        if (item->payload_size >= nbytes) {
            item->owner = pid;

            /*
             * If the item has enough
             * space then split it into two items and put the 
             * surplus item back in the free list
             */

             if (item->payload_size >= (nbytes + sizeof(list_item_t) + SMALLEST_PAYLOAD)) {
                /* There's room to split it */
                list_item_t *surplus = (list_item_t *)(CP(item) + nbytes + sizeof(list_item_t));

                surplus->signature = ITEM_SIG;
                surplus->payload_size = item->payload_size - nbytes - sizeof(list_item_t);
                surplus->owner = 0;

                item->payload_size = nbytes;

                surplus->prev = item;
                surplus->next = item->next;

                item->next = surplus;
                heap_info.num_free_items++;

                // insert_in_free(surplus);
             }

            remove_from_free(item);
            insert_in_allocated(item);


             return CP(item) + sizeof(list_item_t);
        }
    }

    errno = ENOMEM;
    return NULL;
}

void clean_heap(pid_t pid) {
    list_item_t *next = NULL;

    kprintf("clean_heap: pid=%d\n", pid);

    for (list_item_t *item=allocated_items; item!=NULL; item=next) {
        next = item->next;

        if (item->owner == pid) {
            void *p = (void *)(CP(item) + sizeof(list_item_t));
            kprintf("clean_heap: reclaiming 0x%08x\n", p);
            bios_free(p, pid);
        }
    }
}


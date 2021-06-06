#include <stdlib.h>
#include <stdio.h>



#include "virtual_alloc.h"
#include "virtual_sbrk.h"

#define END 0x10000

void *virtual_heap = NULL;
void *brk = NULL;

void* 
virtual_sbrk(int32_t increment) 
{
    int8_t *brkcast;
    void *save_brk = brk;

    brkcast = (int8_t *)brk;
    brkcast = brkcast + increment;
    if (((int8_t *)brkcast - (int8_t *)virtual_heap) >= 0x1000000000) {
        return (void *)(-1);
    }
    brk = (void *)brkcast;

    return save_brk;

}

int 
main() 
{
    virtual_heap = malloc(0x1000000);
    brk = virtual_heap;
    init_allocator(virtual_heap, 23, 10);

    for (int i = 0; i < 6144; i ++) {
        virtual_malloc(virtual_heap, 1000);
        if (! (i % 4)) {
            virtual_malloc(virtual_heap, 2000);
        }
    }

    virtual_info(virtual_heap);

    return 0;
}

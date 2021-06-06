#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#include "virtual_alloc.h"
#include "virtual_sbrk.h"

uint64_t
power(uint64_t b, uint64_t u)
{
        uint64_t ret;

        ret = 1;
        while (1) {
                if (u & 1) {
                        ret *= b;
                }
                u >>= 1;
                if (u == 0) {
                        break;
                }
                b *= b;
        }

        return ret;
}

void
set_bit(int32_t *a, int32_t k)
{
	int32_t i = k / MOD;
	int32_t pos = k % MOD;
	uint32_t flag = 1;
	flag = flag << pos;
	a[i] = a[i] | flag;
}

void
clear_bit(int32_t *a, int32_t k)
{
      int32_t i = k / MOD;
      int32_t pos = k % MOD;
      uint32_t flag = 1;

      flag = flag << pos;
      flag = ~flag;
      a[i] = a[i] & flag;
}


int32_t
test_bits(bit_t *a, uint32_t k)
{
	int32_t c = k / MOD;
    int32_t pos = k % MOD;
    int32_t flag = 1;
    flag = flag << pos;

    if (a[c] & flag) {
        return 1;
    }

    return 0;
}

int32_t
extract(struct state_info *state_info, 
        int32_t from, 
        int32_t to, 
        int32_t idx)
{
    uint64_t n, j;
	if (from < 0) {
		from = 0;
	}
	n = 0;
	j = idx - 1;
    for (int32_t i = from; i < to; i++) {
        if (test_bits(state_info->bitmap, i)) {
            n += power(2, j);
        }
        j -= 1;
    }

    return n;
}

uint64_t
nearest_two(uint64_t size)
{
    size--;
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    size |= size >> 32;
    size++;

    return size;
}


void
print_bits(struct manage *manage)
{
    int32_t from;
    uint8_t result;

    from = 0;
    while (from < (manage->state_info->bitlim * 2)) {
        result = test_bits(manage->state_info->bitmap, from);
        if (result == 1) {
            printf("1 ");
        } else {
            printf("0 ");
        }
        if ((from % 2) == 1) {
            printf("| ");
        }
        from += 1;
    }
    printf("\n");
}

void
set_bits(struct state_info *state_info, uint32_t index, int32_t consecutive)
{
    uint32_t from;

    from = index;
    while (consecutive > 0) {
        set_bit((int32_t *)state_info->bitmap, from);
        from += 2;
        consecutive -= 1;
    }
}

void
clear_bits(struct state_info *state_info, uint32_t index, int32_t consecutive)
{
    uint32_t from;

    from = index * 2;
    
    while (consecutive > 0) {
        clear_bit((int32_t *)state_info->bitmap, from);
        from += 2;
        consecutive -= 1;
    }
}

uint16_t* 
encode(struct manage *manage)
{
    int32_t from;
    uint8_t result;

    uint16_t one = 0;
    uint16_t zero = 0;

    uint16_t *store;
    uint64_t total = 0;
    uint16_t *base; 

    base = virtual_sbrk(sizeof(uint16_t));

    from = 0;
    while (from <= (manage->state_info->bitlim * 2)) {
        
        result = test_bits(manage->state_info->bitmap, from);

        if (result == 1) {
            if (zero != 0) {
                printf("[0 : %d] ", zero);
                store = virtual_sbrk(sizeof(uint16_t));
                *store = zero;
                zero = 0;
                total += 1;
            }
            one += 1;
        } else {
            if (one != 0) {
                printf("[1 : %d] ", one);
                store = virtual_sbrk(sizeof(uint16_t));
                *store = one;
                one = 0;
                total += 1;
            }
            zero += 1;
        }
        from += 1;
    }
    /*
    printf("\n");
    for (int i = 1; i <= total; i++) {
        printf("[%d] ", base[i]);
    }*/
    //printf("total %d\n", total);
    base[0] = total;
    return base;
}

void
double_encode(uint16_t *base)
{
    uint64_t count = 0;
    uint64_t total = base[0];
    int16_t lastone;
    int16_t lasttwo;
    
    uint16_t runenc[2000];
    uint16_t normal[5000];
    uint16_t normaliter = 0;
    uint16_t enciter = 0;

    for (int i = 1; i < total; i += 2) {
        if (count == 0) {
            //printf(" %d %d --> %d\n", lastone, lasttwo, count);
            lastone = base[i];
            lasttwo = base[i + 1];
            count += 1;
        } else {
            if (base[i] == lastone
            && base[i + 1] == lasttwo) {
                if (base[i + 2] != lastone && base[i + 3] != lasttwo) {
                    runenc[enciter++] = lastone;
                    runenc[enciter++] = lasttwo;
                    runenc[enciter++] = count;  
                    //printf(" %d %d --> %d\n", lastone, lasttwo, count);
                }
                count += 1;
            } else {
                normal[normaliter++] = lastone;
                normal[normaliter++] = lasttwo;
                count = 0;
            }
        }
    }
    for (int i = 0; i < enciter; i += 3) {
        lastone = runenc[i];
        lasttwo = runenc[i + 1];
        count = runenc[i + 2];
        printf(" %d %d --> %ld\n", lastone, lasttwo, count);
    }
    for (int i = 0; i < normaliter; i += 1) {
        
        printf(" _%d_ ", normal[i]);
    }
    printf("\n");
}

void add_ir_item(struct state_info *, uint32_t, uint32_t);

void
init_allocator(void *heapstart,
               uint8_t initial_size,
               uint8_t min_size)
{
    struct manage *manage;
    uint64_t raised;
    uint64_t bits;

    // sanity checks
    if (heapstart == NULL || initial_size < min_size) {
        printf("error in initialisation of heap, abort\n");
        return;
    }
    // calculate initial size and bits
    raised = power(2, initial_size); 
    bits = power(2, initial_size - min_size);
    
    virtual_sbrk(sizeof(struct manage) + raised);

    manage = (struct manage *)heapstart;
    manage->min_size = min_size;
    manage->initial_size = initial_size;
    manage->state_info = (struct state_info *)
                         virtual_sbrk(sizeof(struct state_info));
    // we use 2 bits per minima block
    manage->state_info->bitmap = (bit_t *)
                                 virtual_sbrk((bits / 4));
    manage->state_info->bitlim = bits;
    memset(manage->state_info->bitmap, 0, (bits / 4));
    // this is where we track blocks 
    add_ir_item(manage->state_info, 0, manage->state_info->bitlim);

/*
    two more ideas:
    1. use run-length encoding on the bits and then run-length encoding on the runs
    2. memcpy the bitmap around the heap opportunistically when we free a chunk
*/

    //encode(manage);
}

uint32_t
check_block(struct manage *manage, 
            uint32_t index, 
            uint32_t run)
{
    struct state_info *state_info;
    uint32_t iter;

    bool state, stop;

    state_info = manage->state_info;
    iter = index;
    
    while (iter < (index + run)) {
        state = test_bits(state_info->bitmap, iter);
        stop = test_bits(state_info->bitmap, iter + 1);

        if (state == ALLOC || stop) {
            return 0;
        }
        iter += 2;
    }

    return 1;
}

int64_t
find_consecutive(struct manage *manage, uint32_t count)
{

/*
    We have two types of bits:
        1. left : state bit (for allocation)
        2. right : continuation bit (for parsing bitmap)
*/
    
    uint64_t idx;
    struct state_info *state_info;
    bool stop, state;
    
    state_info = manage->state_info;
    state = FREE;
    stop = true; 
    idx = 0;

    while (idx < (state_info->bitlim * 2)) {
        state = test_bits(state_info->bitmap, idx);
        stop = test_bits(state_info->bitmap, idx + 1);
        
        if (state == FREE && !stop) {
            if (check_block(manage, idx, count)) {
                return (idx / 2);
            }
        }

        idx += (2 * count);
    }

    return -1;
}

void
mitosis(struct manage *manage, 
        uint64_t index, 
        uint64_t container_count,
        uint64_t target_count)
{
/*
    We want to split from index --> container_count
    down to index --> target_count
    i.e. 
i)      0 0 | 0 0 | 0 0 | 0 0 | 0 0 | 0 0 | 0 0 | 0 1 |
ii)     0 0 | 0 0 | 0 0 | 0 1 | 0 0 | 0 0 | 0 0 | 0 1 |
iii)    0 0 | 0 1 | 0 0 | 0 1 | 0 0 | 0 0 | 0 0 | 0 1 |
iv)     0 1 | 0 1 | 0 0 | 0 1 | 0 0 | 0 0 | 0 0 | 0 1 |
which is split down to:
        (1)    (1)         (2)                     (4)
*/
    uint64_t cc, div, stop;

    stop = container_count / target_count;
    div = 2;
    cc = container_count;

    while (div <= stop) {
        add_ir_item(manage->state_info, index, (cc / div));
        div = div * 2;
    }
}

int64_t 
count_block(struct manage *manage, uint64_t start)
{
    bool stop;
    uint64_t count = 0;

    // 2-scaled index
    uint64_t idx = (start * 2);

    while (idx < (manage->state_info->bitlim * 2)) {

        stop = test_bits(manage->state_info->bitmap, idx + 1);

        count += 1;
        if (stop) {
            break;
        }

        idx += 2;
    }
    return count;
}

int64_t
find_existing(struct manage *manage, uint64_t start, uint32_t count)
{
/*
    We have two types of bits:
        1. left : state bit (for allocation)
        2. right : continuation bit (for parsing bitmap)
*/
    struct state_info *state_info;
    uint64_t idx;
    int64_t run;

    state_info = manage->state_info;

    bool stop;
    bool state;

    state = FREE;
    stop = true; 

    idx = start;
    run = 0;

    while (idx < (state_info->bitlim * 2)) {

        state = test_bits(state_info->bitmap, idx);
        stop = test_bits(state_info->bitmap, idx + 1);

        if (state == FREE) {
            run += 1;
        } else {
            run = 0;
        }
        if (stop) {
            if (run == count && state == FREE) {
                return (idx - ((count * 2) - 2));
            }
            run = 0;
        }
        idx += 2;
    }
    return -1;
}



uint8_t*
resolve_address(struct manage *manage, uint32_t index)
{
    uint32_t offset;
    uint64_t translated;

    offset = index;
    if (offset == 0) {
        return (uint8_t *)((unsigned long)manage
                        + sizeof(struct manage));
    } else {
        offset = offset / 2;
        translated = (offset * power(2, manage->min_size));
    }

    return (uint8_t *)((unsigned long)manage
                        + sizeof(struct manage) + translated);
}

void
add_ir_item(struct state_info *state_info, 
            uint32_t index, 
            uint32_t run)
{
    uint64_t transindex;

    if (run == 0) {
        return;
    }
    transindex = ((index + run) * 2) - 1;
    set_bit((int32_t *)state_info->bitmap, transindex);
}

void
get_ir_items(struct manage *manage)
{
    int64_t idx;
    uint64_t run;
    uint64_t equ; 
    
    bool state; 

    idx = 0;
    equ = 0;
    run = 0;
    
    state = FREE;

    idx = 0;
    while (idx < (manage->state_info->bitlim * 2)) {
        if (test_bits(manage->state_info->bitmap, idx)) {
            state = ALLOC;
        } else {
            state = FREE;
        }
        if (test_bits(manage->state_info->bitmap, idx + 1) == 0) {
            run += 1;
            idx += 2;
            continue;
        } else {
            // print it 
            if (state == FREE) {
                printf("free ");
            } else {
                printf("allocated ");
            }
            run += 1;
            equ = run * (power(2, manage->min_size));
            printf("%ld\n", equ);
        }
        run = 0;
        state = FREE;
        idx  += 2;
    }
}

void*
virtual_malloc(void *heapstart, uint32_t size)
{
    struct manage *manage;
    struct state_info *state_info;
    uint64_t raised, consecutive;
    int64_t index;
    uint8_t *addr;

    raised = nearest_two(size);
    manage = (struct manage *)heapstart;
    // heap wasn't set up before this call
    if (manage->state_info == NULL) {
        init_allocator(heapstart, log2(raised), 8);
    }
    if (manage->state_info == NULL) {
        return NULL;
    }
    if (log2(raised) < manage->min_size) {
        raised = power(2, manage->min_size);
    }
    if (log2(raised) > manage->initial_size) {
        //raised = power(2, manage->initial_size);
        return NULL;
    }
    state_info = manage->state_info;
    // work out how many bits we would need for size
    consecutive = power(2, log2(raised) - manage->min_size);
    // try to find an existing block
    index = find_existing(manage, 0, consecutive);

    if (index == -1) {
// i) find a consecutive run of the required bits
        uint64_t idx;
        idx = find_consecutive(manage, consecutive);
        if (idx == -1) {
            return NULL;
        }
// ii) locate the block which contains the run
        uint64_t cc;
        cc = count_block(manage, idx);
// iii) split to the left with mitosis
        mitosis(manage, idx, cc, consecutive);
        index = find_existing(manage, 0, consecutive);
        if (index == -1) {
            return NULL;
        }
    }
// iv) resolve the address
    addr = resolve_address(manage, index);
// v) set the bits so we know it's allocated
    set_bits(state_info, index, consecutive);

    return addr;
}

void
make(struct state_info *state_info, uint64_t iter, uint64_t window)
{
    bool state;
    uint64_t inner, inneriter;

    uint64_t all_free = 0;
    inner = iter;
    inneriter = 0;
    
    while (inneriter < (window * 2)) {

        state = test_bits(state_info->bitmap, inner);

        all_free += state;
        if (all_free) {
            break;
        }
        inner += 2;
        inneriter += 2;
    }
    if (! all_free) {
        clear_bit((int32_t *)state_info->bitmap, (iter + window) - 1);
    }
}

void
mold(struct state_info *state_info, uint64_t window)
{
    bool state, stop;
    uint64_t iter;

    iter = 0;
    while (iter < (state_info->bitlim * 2)) {
        
        // test the last two bits of the window
        state = test_bits(state_info->bitmap, iter + ((2 * window) - 2));
        stop = test_bits(state_info->bitmap, iter + ((2 * window) - 1));

        if (state == FREE && stop) {
            make(state_info, iter, window);
        }
        iter += (window * 2);
    }
}

void
merge(struct manage *manage)
{
    struct state_info *state_info;
    uint64_t window;

    state_info = manage->state_info;

    window = 2;
    while (window <= (state_info->bitlim * 2)) {
        mold(state_info, window);
        window *= 2;
    }
}

int64_t
resolve_index(struct manage *manage, void *ptr)
{
    int64_t distance, ratio; 

    distance = ((unsigned long)ptr - ((unsigned long)manage
                        + sizeof(struct manage)));
    // perform sanity checks on distance
    if (distance < 0) {
        return -1;
    }
    if (distance == 0) {
        ratio = 0;
    } else {
        ratio = distance / power(2, manage->min_size);
    }
    return ratio;
}

int
virtual_free(void *heapstart, void *ptr)
{
    struct manage *manage;
    struct state_info *state_info;
    int32_t index;
    uint64_t cc;

    manage = (struct manage *)heapstart;
    if (manage->state_info == NULL) {
        return 1;
    }
    state_info = manage->state_info;
    // i) resolve pointer address to bit-index
    index = resolve_index(manage, ptr);
    if (index == -1) {
        return 1;
    }
    // ii) get run information 
    cc = count_block(manage, index);
    // iii) clear corresponding bits
    clear_bits(state_info, index, cc);
    // iv) perform "merging" by observing the count of
    // clear bits and adjusting the runs accordingly
    merge(manage);

    return 0;
}

void*
do_realloc(void *heapstart,
           void *block,
           uint64_t newsize)
{
    int8_t *new_block;
    uint8_t *contents;
    int64_t index, req; 

    contents  = (uint8_t *)block;
    if (contents == NULL) {
        return NULL;
    }
    if (virtual_free(heapstart, contents) == 1) {
        return NULL;
    }
    new_block = (int8_t *)virtual_malloc(heapstart, newsize);
    if (new_block == NULL) {
        // we failed, so we reallocate the old block
        index = resolve_index((struct manage *)heapstart, block);
        req = count_block((struct manage *)heapstart, index);
        new_block = (int8_t *)virtual_malloc(heapstart, req);
        return NULL;
    }
    // use memmove because overlap sux
    memmove(new_block, contents, newsize);

    return new_block;
}


void*
virtual_realloc(void *heapstart, void *ptr, uint32_t size)
{
    uint64_t hi;
    struct manage *manage;

    if (heapstart == NULL) {
        return NULL;
    }
    manage = (struct manage *)heapstart;
    if (manage->state_info == NULL) {
        return NULL;
    }
    if (ptr == NULL) {
        return virtual_malloc(heapstart, size);
    }
    if (size == 0) {
        virtual_free(heapstart, ptr);
        return NULL;
    }
    hi = nearest_two(size);
    if (log2(hi) > manage->initial_size) {
        return NULL;
    }

    return do_realloc(heapstart, ptr, hi);
}

void
virtual_info(void *heapstart)
{
    struct manage *manage;

    if (heapstart == NULL) {
        return;
    }
    manage = (struct manage *)heapstart;
    if (manage->state_info == NULL) {
        return;
    }
    get_ir_items(manage);    
    
/*
If i want to do double run-length bit pattern encoding!
But i decided against this because there's no point.

    //uint64_t bits = power(2, manage->initial_size - manage->min_size);
    //printf("bits %d\n", (bits / 4));
    uint16_t *base = encode(manage);

    double_encode(base);
*/
}

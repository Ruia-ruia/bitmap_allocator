#include <stddef.h>
#include <stdint.h>

#define MAX 0x10000000
#define MOD 32

// states
enum { FREE, ALLOC };

typedef uint16_t interval_t;
typedef uint32_t bit_t;

// first structure in memory
struct __attribute__((__packed__)) manage {
    // dimensions of the heap
    uint8_t initial_size;
    uint8_t min_size;
    // pointer to the state info 
    // at the end of the heap
    struct state_info *state_info;
};

struct __attribute__((__packed__)) state_info {
    // where we store states and parsing data
    bit_t *bitmap;
    uint16_t bitlim;
};

void init_allocator(void * heapstart, uint8_t initial_size, uint8_t min_size);

void * virtual_malloc(void * heapstart, uint32_t size);

int virtual_free(void * heapstart, void * ptr);

void * virtual_realloc(void * heapstart, void * ptr, uint32_t size);

void virtual_info(void * heapstart);

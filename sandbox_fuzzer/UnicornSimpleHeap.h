#ifndef _UNICORNSIMPLEHEAP
#define _UNICORNSIMPLEHEAP

#include <unicorn/unicorn.h>
#include <iostream>
#include <functional>
#include <algorithm>
#include <vector>
#include <cstdlib>

#define UNICORN_PAGE_SIZE 0x1000

#define DEBUG(fmt,...) do { \
  if (debug_trace) { printf(fmt, ##__VA_ARGS__); putchar('\n'); } \
} while (0)

#define uc_assert_err(expect, err)                                  \
do {                                                                \
    uc_err __err = err;                                             \
    if (__err != expect) {                                          \
        fprintf(stderr, "%s", uc_strerror(__err));                  \
        exit(1);                                                    \
    }                                                               \
} while (0)

#define uc_assert_success(err)  uc_assert_err(UC_ERR_OK, err)

inline uint64_t CHUNK(const uint64_t& x) { return x - (x & 0xfff); }
inline uint64_t CHUNK_DATA(const uint64_t& addr, const uint64_t& size) { return addr + (UNICORN_PAGE_SIZE - (size & 0xfff)); }
inline uint64_t CHUNK_DATA_SIZE(const uint64_t& addr, const uint64_t& size) { return size - ((addr&0xfff) + UNICORN_PAGE_SIZE); }

inline uint64_t ALIGN_PAGE_DOWN(const uint64_t& x) { return x & ~(UNICORN_PAGE_SIZE - 1); }
inline uint64_t ALIGN_PAGE_UP(const uint64_t& x) { return (x + UNICORN_PAGE_SIZE - 1) & ~(UNICORN_PAGE_SIZE-1); }

struct HeapChunk{
    uint64_t addr;
    uint64_t size;
};

bool Compare_Chunk(const HeapChunk& rhs, const uint64_t& addr);

class UnicornSimpleHeap{
private:
    uc_engine *uc;
    bool debug_trace;
    std::vector<HeapChunk> chunks;
        
    enum{
        HEAP_MIN_ADDR = 0x40002000, // need to update
        HEAP_MAX_ADDR = 0xFFFFFFFF
    };
public:
    UnicornSimpleHeap(uc_engine* _uc, bool _debug_trace);
};

#endif
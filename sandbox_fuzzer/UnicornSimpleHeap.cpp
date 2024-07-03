#include "UnicornSimpleHeap.h"

bool Compare_Chunk(const HeapChunk& rhs, const uint32_t& addr){
    return addr == rhs.addr;
}

UnicornSimpleHeap::UnicornSimpleHeap(uc_engine* _uc, bool _debug_trace) 
        : uc(_uc), debug_trace(_debug_trace) {}

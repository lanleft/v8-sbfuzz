
#include <cstddef>
#include <unicorn/unicorn.h>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include "AflUnicornEngine.h"
#include "UnicornSimpleHeap.h"
#include "include/unicorn/x86.h"
#include <memory>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>


// #define LLVM_FUZZER

// memory address where emulation starts
#define START_ADDRESS 0x1000000
#define END_ADDRESS 0x1000000

uc_engine *uc;
int initialized = 0;
FILE * outfile = NULL;


// for loading pages and 
AflUnicornEngine afl;
// UnicornSimpleHeap* uc_heap;

void unicorn_hook_instruction(uc_engine *uc, uint64_t address, uint32_t size, void *user_data){
}


#ifdef LLVM_FUZZER
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {

#else 

int main(int argc, char **argv){
    std::string filepath = argv[1];
    std::ifstream in(filepath, std::ios::binary);
    size = std::filesystem::file_size(filepath);
    data = new uint8_t[size];
    in.read((char *)(data), std::filesystem::file_size(filepath));
    std::cout << "Reproducing Poc: " << filepath << " Size : " << size << std::endl;
    printf("Running maaaain() from %s\n", __FILE__);
}

#endif
    uc_err err;

    uc_hook trace;
    uc_hook_add(afl.get_uc(), &trace, UC_HOOK_CODE, reinterpret_cast<void*>(unicorn_hook_instruction), NULL, 1, 0);

    // Start emulation
    uint64_t rip = START_ADDRESS;
    while (rip != END_ADDRESS){
        err = uc_emu_start(afl.get_uc(), rip, END_ADDRESS, 0, 0);
        if (err){
            afl.force_crash(err);
            return 0;
        }
        uc_reg_read(afl.get_uc(), UC_X86_REG_RIP, &rip);
    }

    return 0;
}
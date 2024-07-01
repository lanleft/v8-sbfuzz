
#include "include/unicorn/unicorn.h"
#include "include/AflUnicornEngine.h"


#define LLVM_FUZZER

// memory address where emulation starts
#define START_ADDRESS 0x1000000
#define END_ADDRESS 0x1000000

uc_engine *uc;
int initialized = 0;
FILE * outfile = NULL;


// for loading pages and 
AflUnicornEngine* afl;
UnicornSimpleHeap* uc_heap;

static void unicorn_hook_instruction(uc_engine *uc, uint64_t address, uint32_t size, void *user_data){
    if (address == _malloc){
        uint32_t esp;
        uc_reg_read(uc, UC_X86_REG_ESP, &esp);

        uint32_t size, ret_addr;
        uc_mem_read(uc, esp+4, &size, sizeof(size));
        uc_mem_read(uc, esp, &ret_addr, sizeof(ret_addr));
        uc_reg_write(uc, UC_X86_REG_EIP, &ret_addr);
        
        uint32_t eax = uc_heap->malloc(size);
        uc_reg_write(uc, UC_X86_REG_EAX, &eax);
        
        esp += 4;
        uc_reg_write(uc, UC_X86_REG_ESP, &esp);
    }
    
    if (address == _free){ 
        uint32_t esp;
        uc_reg_read(uc, UC_X86_REG_ESP, &esp);
        
        uint32_t addr, ret_addr;
        uc_mem_read(uc, esp+4, &addr, sizeof(addr));
        uc_mem_read(uc, esp, &ret_addr, sizeof(ret_addr));
        uc_reg_write(uc, UC_X86_REG_EIP, &ret_addr);
        
        uint32_t eax = uc_heap->free(addr);
        uc_reg_write(uc, UC_X86_REG_EAX, &eax);
        
        esp += 4;
        uc_reg_write(uc, UC_X86_REG_ESP, &esp);
    }
}

@
void init_state(){
    const std::string context_dir = getenv("CONTEXT_DIR");

    if(!context_dir){
        std::cout << "Missing CONTEXT_DIR enviroment" << std::endl;
        exit(1);
    }

    // const std::string context_dir = argv[1];
    // const std::string file_path   = argv[2];
    // bool debug_trace = strcmp(argv[3], "true")? false : true;
    // bool heap_trace = strcmp(argv[4], "true")? false : true;
    bool debug_trace = true;
    bool heap_trace = true;
    uc_err err;
        
    // Load Context files and create a engine
    afl = new AflUnicornEngine(context_dir, false, debug_trace);
    uc_heap = new UnicornSimpleHeap(afl->get_uc(), heap_trace);
    
    // Hooking some functions
    uc_hook trace;
    uc_hook_add(afl->get_uc(), &trace, UC_HOOK_CODE, reinterpret_cast<void*>(unicorn_hook_instruction), NULL, 1, 0);
}


#ifdef LLVM_FUZZER

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {

#else 

int main(int argc, char **argv){
        filepath = argv[1];
        std::ifstream in(filepath, std::ios::binary);
        size = std::filesystem::file_size(filepath);
        data = new uint8_t[size];
        in.read((char *)(data), std::filesystem::file_size(filepath));
        std::cout << "Reproducing Poc: " << filepath << " Size : " << size << std::endl;
        printf("Running maaaain() from %s\n", __FILE__);
}

#endif
    uc_err err;

    // Start emulation
    uint64_t eip = START_ADDRESS;
    while (eip != END_ADDRESS){
        err = uc_emu_start(afl->get_uc(), eip, END_ADDRESS, 0, 0);
        if (err){
            afl->force_crash(err);
            return 0;
        }
        uc_reg_read(afl->get_uc(), UC_X86_REG_EIP, &eip);
    }

    return 0;
}
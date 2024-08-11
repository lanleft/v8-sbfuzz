/*
   Simple test harness for AFL++'s unicornafl c mode.

   This loads the simple_target_x86_64 binary into
   Unicorn's memory map for emulation, places the specified input into
   argv[1], sets up argv, and argc and executes 'main()'.
   If run inside AFL, afl_fuzz automatically does the "right thing"

   Run under AFL as follows:

   $ cd <afl_path>/unicorn_mode/samples/c
   $ make
   $ ../../../afl-fuzz -m none -i sample_inputs -o out -- ./harness @@

   // debug
   CONTEXT_DIR=UnicornContext_20240704_110015 gdb --args ./harness -t out/default/crashes/id:000000,sig:01,src:000000,time:353,execs:371,op:havoc,rep:16 
*/

// This is not your everyday Unicorn.
#include "unicorn/unicorn.h"
#include "unicorn/x86.h"
#define UNICORN_AFL

#include "UnicornLoader.h"


#define INPUT_ADDR 0x5000
#define TARGET_RANGE_START 0x9b300000000
#define TARGET_RANGE_END TARGET_RANGE_START+0x100000000

uint64_t current_input_len = 0;
uint64_t current_input_index = 0;
// Global flag to indicate if we encountered an invalid memory read
bool is_invalid = false;

// Callback function for invalid memory reads
static bool hook_mem_invalid(uc_engine *uc, uc_mem_type type,
                             uint64_t address, int size, int64_t value, void *user_data) {
    if (type == UC_MEM_READ_UNMAPPED || type == UC_MEM_READ_PROT) {
        uc_mem_map(uc, address & 0xfffffffffffff000, 0x1000, UC_PROT_ALL);
        DEBUG_COLOR(COLOR_YELLOW, "Invalid memory read at 0x%" PRIx64 ", size: %d", address, size);
        // uc_emu_stop(uc);  // Stop the emulation
        is_invalid = true;
        uc_emu_stop(uc);
        return true;  // Indicate that we've handled the error
    }

    return false;  // For other types of memory errors, let Unicorn handle it
}

// Using flip bits to ensure each chunk bytes changed exactly one time
void hook_mem_access(uc_engine *uc, uc_mem_type type, uint64_t address, int size, int64_t value, void *user_data) {
    // Check if we've exceeded our input length
    if (current_input_index + size >= current_input_len) {
        value = 0xffffff00;
        uc_mem_write(uc, address, &value, size);
        DEBUG_COLOR(COLOR_YELLOW, "0x%lx size: 0x%x end of input buffer. Stopping emulation gracefully.", address, size);
        is_invalid = true;
        uc_emu_stop(uc);
        return;
    }

    if ((address >= TARGET_RANGE_START+40000 && address < TARGET_RANGE_START+143000) | (address >= TARGET_RANGE_START+0x180000 && address < TARGET_RANGE_START+0x280000) | (address >= TARGET_RANGE_START+0x2c0000 && address < TARGET_RANGE_START+0x340000)) {
        if (type == UC_MEM_READ) {

            switch (address) {

                case 0x9b3001dca0c:
                    value = 0x001dc9c9;
                    uc_mem_write(uc, address, &value, size);
                    DEBUG_COLOR(COLOR_RED, "##### 0x%"PRIx64" -> 0x%lx size: %d  value: 0x%lx", 
                                address, (uint64_t)0x001dc9c9, size, value);
                    break;

                case 0x9b3001dc9cc: // trusted data pointer 
                    // 0x555556ccf0cc <Builtins_JSToWasmWrapper+76>     and    r8, qword ptr [r9 + rcx]          R8 => 0x16aa000c4935
                    value = 0x00405e00;
                    uc_mem_write(uc, address, &value, size);
                    DEBUG_COLOR(COLOR_RED, "##### 0x%"PRIx64" -> 0x%lx size: %d  value: 0x%lx", 
                                address, (uint64_t)0x00405e00, size, value);
                    break;

                case 0x9b30019ab68:
                    value = 0x00000831;
                    uc_mem_write(uc, address, &value, size);
                    DEBUG_COLOR(COLOR_RED, "##### 0x%"PRIx64" -> 0x%lx size: %d  value: 0x%lx", 
                                address, (uint64_t)0x00000831, size, value);
                    break;

                default:
                    // Redirect read to INPUT_ADDR
                    uc_mem_read(uc, INPUT_ADDR + current_input_index, &value, size);
                    uc_mem_write(uc, address, &value, size);
                    DEBUG_COLOR(COLOR_RED, "##### 0x%"PRIx64" -> 0x%lx size: %d  value: 0x%lx", 
                                address, (uint64_t)INPUT_ADDR + current_input_index, size, value);
                    current_input_index += size;
                    break;
            }

        } else if (type == UC_MEM_WRITE) {
            // Handle write if necessary
            // For now, we're just logging the write operation
            DEBUG_COLOR(COLOR_BLUE, "Write operation at 0x%"PRIx64", size: %d, value: 0x%lx", 
                        address, size, value);
        }
    }
}
static void hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {

    uint64_t rdi, rsi, rdx, rsp, new_rip;
    uint64_t r8, rax, rip, rcx;
    // uint64_t  key, permission;
    // unsigned char tmp_data[0x100] = {0};

    unsigned char code[16] = {0};
    uc_mem_read(uc, address, code, size);

    cs_insn *insn;
    csh handle;
    cs_open(CS_ARCH_X86, CS_MODE_64, &handle);
    size_t count = cs_disasm(handle, code, size, address, 0, &insn);

    if (count > 0) {
        // cs_x86 *x86 = &(insn[0].detail->x86);
        
        DEBUG("0x%"PRIx64": %s %s", address, insn[0].mnemonic, insn[0].op_str);

    } else {
        DEBUG("0x%"PRIx64": invalid instruction", address);
    }

    cs_free(insn, count);
    cs_close(&handle);

    // hooking address 
    switch (address){
        case 0x555556ccfc68:
            uc_reg_read(uc, UC_X86_REG_RSP, &rsp);
            uc_reg_read(uc, UC_X86_REG_RCX, &rcx);
            DEBUG_COLOR(COLOR_RED, ">> rsp: 0x%lx rcx: 0x%lx", rsp, rcx);
            break;

        default:
            break;
    }

    // Add this check at the end of the function
    if (is_invalid) {
        uc_emu_stop(uc);
        return;
    }

}



/* Place the input at the right spot inside unicorn */
static bool place_input_callback(
    uc_engine *uc, 
    char *input, 
    size_t input_len, 
    uint32_t persistent_round, 
    void *data
){
    // printf("Placing input with len %ld to %x\n", input_len, DATA_ADDRESS);
    if (input_len < 2) {
        // Test input too short ignore this testcase
        return false;
    }

    // print 0x10 first bytes of input


    // We need a valid c string, make sure it never goes out of bounds.
    input[input_len-1] = '\0';
    // Write the testcase to unicorn.
    uc_mem_write(uc,  INPUT_ADDR, input, input_len);
    DEBUG("### Placed input with len %ld to 0x%x", input_len, INPUT_ADDR);

    if (tracing) {
        uint64_t tmp_data = 0;
        uc_mem_read(uc, INPUT_ADDR, &tmp_data, 8);
        DEBUG("### First 8 bytes of input: 0x%lx", tmp_data);
    }

    // store input_len for the faux strlen hook
    current_input_len = input_len;
    current_input_index = 0;  // Reset the index for each new input
    is_invalid = false;  // Reset the flag for each new input

    return true;
}

uc_engine* init_unicorn(const char* context_dir) {
    uc_engine* uc;
    load_context(&uc, context_dir);
    unsigned char nop_arr[0x20] = {0};
    memset(nop_arr, 0x90, 0x20);
    
    // enable some features on cpu 


    // setup value for fs:0x28
    uc_mem_map(uc, 0, 0x2000, UC_PROT_ALL);
    uint8_t a[8] = "\x00\x2b\x9b\x82\x26\xdf\xc6\x87";
    uc_mem_write(uc, 0x28, a, 8); // canary 
    // mov 0x7ffff7c3bc80 to fs:0x0
    memcpy(a, "\x80\xbc\xc3\xf7\xff\x7f\x00\x00", 8);
    uc_mem_write(uc, 0, a, 8); // canary

    // 0x7ffff7e28d78
    uc_mem_map(uc, 0x7ffff7e28d78, 0x1000, UC_PROT_ALL);
    uint8_t b[8] = "\x50\x00\x00\x00\x00\x00\x00\x00";
    uc_mem_write(uc, 0x7ffff7e28d78, b, 8); // mov    rax,QWORD PTR fs:[r12]
    // write 0x555557004010 to fs:0x50
    memcpy(b, "\x10\x40\x00\x57\x55\x55\x00\x00", 8);
    uc_mem_write(uc, 0x50, b, 8);


    return uc;
}


int main(int argc, char **argv, char **envp) {
    if (argc == 1) {
        printf("Test harness for simple_target.bin. Usage: harness [-t] <inputfile>\n");
        exit(1);
    }
    char *filename = argv[1];
    if (argc > 2 && !strcmp(argv[1], "-t")) {
        tracing = true;
        filename = argv[2];
    }

    // UnicornContext_20240704_100111
   const char* context_dir_env = getenv("CONTEXT_DIR");

    if(!context_dir_env){
        printf("CONTEXT_DIR is not set.\n");
        exit(1);
    }
    // ===================== start unicorn ===============================
    uc_engine* uc = init_unicorn(context_dir_env);
    
    // address for input 
    uc_mem_map(uc, INPUT_ADDR, 0x5000, UC_PROT_ALL);
    // ======================= load context ===============================

    // Set the program counter to the start of the code
    uint64_t start_address = 0x555556ccf080; // <Builtins_JSToWasmWrapper>       push   rbp  
    uint64_t end_address = 0x555556ccfc6e; // <Builtins_JSToWasmWrapper+3054>              ret 

    // If we want tracing output, set the callbacks here
    uc_hook hooks[3];
    uc_hook_add(uc, &hooks[0], UC_HOOK_CODE, hook_code, NULL, start_address, end_address);
    uc_hook_add(uc, &hooks[1], UC_HOOK_MEM_READ, hook_mem_access, NULL, TARGET_RANGE_START, TARGET_RANGE_END);

    uc_hook_add(uc, &hooks[2], UC_HOOK_MEM_READ_UNMAPPED | UC_HOOK_MEM_FETCH_UNMAPPED,
                      (void *)hook_mem_invalid, NULL, 1, 0); // all addresses

    // start fuzzing
    DEBUG("Starting to fuzz...");
    fflush(stdout);

    // if (tracing) {
    //     dump_registers(uc);
    // }

    // let's gooo
    uc_afl_ret afl_ret = uc_afl_fuzz(
        uc, // The unicorn instance we prepared
        filename, // Filename of the input to process. In AFL this is usually the '@@' placeholder, outside it's any input file.
        place_input_callback, // Callback that places the input (automatically loaded from the file at filename) in the unicorninstance
        &end_address, // Where to exit (this is an array)
        1,  // Count of end addresses
        NULL, // Optional calback to run after each exec
        false, // true, if the optional callback should be run also for non-crashes
        1, // For persistent mode: How many rounds to run
        NULL // additional data pointer
    );
    DEBUG("=== afl_ret: %d", afl_ret);
    switch(afl_ret) {
        case UC_AFL_RET_ERROR:
            DEBUG("Error starting to fuzz");
            return -3;
            break;
        case UC_AFL_RET_NO_AFL:
            DEBUG("No AFL attached - We are done with a single run.");
            break;
        default:
            break;
    } 
    return 0;
}

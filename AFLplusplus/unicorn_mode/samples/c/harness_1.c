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
#define TARGET_RANGE_START 0x1e5c00000000
#define TARGET_RANGE_END 0x1e5d00000000

uint64_t current_input_len = 0;
uint64_t current_input_index = 0;



// ========================================================================================

// static void hook_block(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
//     // printf(">>> Tracing basic block at 0x%"PRIx64 ", block size = 0x%x\n", address, size);
// }

// hook function avx2

#define MEMSET_ADDR 0x555556ec7940
#define PKEY_SET_ADDR 0x7fffec996a80 //
#define PKEY_GET_ADDR 0x7fffec996ae0    

uint64_t key_arr[16] = {0};

static void hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {

    uint64_t rdi, rsi, rdx, rsp, new_rip;
    // uint64_t rip, rax;
    // uint64_t  key, permission;
    // unsigned char tmp_data[0x100] = {0};

    unsigned char code[16] = {0};
    uc_mem_read(uc, address, code, size);

    cs_insn *insn;
    csh handle;
    cs_open(CS_ARCH_X86, CS_MODE_64, &handle);
    size_t count = cs_disasm(handle, code, size, address, 0, &insn);

    if (count > 0) {
        cs_x86 *x86 = &(insn[0].detail->x86);
        
        DEBUG("0x%"PRIx64": %s %s", address, insn[0].mnemonic, insn[0].op_str);
        // Check if it's a mov instruction
        if (insn[0].id == X86_INS_MOV) {
            // Check if the second operand is a memory operand
            if (x86->op_count == 2 && x86->operands[1].type == X86_OP_MEM) {
                cs_x86_op *mem_op = &(x86->operands[1]);
                uint64_t mem_addr = 0;

                // Calculate the memory address
                if (mem_op->mem.base != X86_REG_INVALID)
                    uc_reg_read(uc, mem_op->mem.base, &mem_addr);
                if (mem_op->mem.index != X86_REG_INVALID) {
                    uint64_t index_value;
                    uc_reg_read(uc, mem_op->mem.index, &index_value);
                    mem_addr += index_value * mem_op->mem.scale;
                }
                mem_addr += mem_op->mem.disp;

                // Check if the memory address is in the target range
                if (mem_addr >= TARGET_RANGE_START && mem_addr < TARGET_RANGE_END) {
                    uint32_t read_size = x86->operands[0].size;
                    
                    // Read from INPUT_ADDR instead
                    uint64_t input_data;
                    uc_mem_read(uc, INPUT_ADDR + current_input_index, &input_data, read_size);
                    current_input_index += read_size;

                    // Write the data to the destination operand
                    if (x86->operands[0].type == X86_OP_REG) {
                        uc_reg_write(uc, x86->operands[0].reg, &input_data);
                    } else if (x86->operands[0].type == X86_OP_MEM) {
                        uint64_t dest_addr;
                        uc_reg_read(uc, x86->operands[0].mem.base, &dest_addr);
                        dest_addr += x86->operands[0].mem.disp;
                        uc_mem_write(uc, dest_addr, &input_data, read_size);
                    }

                    // Skip the original instruction
                    uint64_t new_rip = address + insn[0].size;
                    uc_reg_write(uc, UC_X86_REG_RIP, &new_rip);

                    DEBUG("Intercepted mov at 0x%"PRIx64", redirected read from INPUT_ADDR+0x%"PRIx64"", 
                               address, current_input_index - read_size);
                }
            }
        }

    } else {
        DEBUG("0x%"PRIx64": invalid instruction", address);
    }

    cs_free(insn, count);
    cs_close(&handle);

    // hooking address 
    switch (address){
        // case 0x7ffff7dc5915:
        //     printf("####  Hooking address 0x7ffff7dc5915\n");
        //     uint32_t eax_value = 0;
        //     uc_reg_read(uc, UC_X86_REG_RDI, &rdi);
        //     printf("\t rdi: 0x%"PRIx64 "\n", rdi);
        //     uc_mem_read(uc, rdi, tmp_data, 0x20);
            
        //     // uint64_t rax_value = 0;
        //     // uc_reg_read(uc, UC_X86_REG_RAX, &rax_value);
        //     eax_value = 1 << 31;
        //     printf("\t before ymm eax_value: 0x%x\n", eax_value);

        //     for (int i=0; i<0x20; i++){
        //         if(tmp_data[i] == 0){
        //             eax_value |= 1 << i;
        //         } 
        //     }
        //     printf("\t after ymm eax_value: 0x%x\n", eax_value);
        //     uc_reg_write(uc, UC_X86_REG_RAX, &eax_value);
        //     // add rip by 8
        //     uc_reg_read(uc, UC_X86_REG_RIP, &rip);
        //     new_rip = rip + 8;
        //     uc_reg_write(uc, UC_X86_REG_RIP, &new_rip);
        //     printf("Skipped 2 instructions at 0x7ffff7dc5915. New RIP: 0x%" PRIx64 "\n", new_rip);

        //     break;
        // case 0x7ffff7cd711a:
        //     // dump_registers(uc);
        //     rax = 0x555556fb1010;
        //     uc_reg_write(uc, UC_X86_REG_RAX, &rax);
        //     new_rip = 0x7ffff7cd711f;
        //     uc_reg_write(uc, UC_X86_REG_RIP, &new_rip);
        //     break;
        // case 0x00007fffec996ab5:
        //     printf(">>> hooking 0x00007fffec996ab5\n");
        //     uc_reg_read(uc, UC_X86_REG_RSP, &rsp);
        //     // read 8 bytes at rsp 
        //     uc_mem_read(uc, rsp, &new_rip, 8);
        //     printf("\t new_rip: 0x%"PRIx64 "\n", new_rip);
        //     uc_reg_write(uc, UC_X86_REG_RIP, &new_rip);
        //     break;

        case MEMSET_ADDR:
            uc_reg_read(uc, UC_X86_REG_RDI, &rdi);
            uc_reg_read(uc, UC_X86_REG_RSI, &rsi);
            uc_reg_read(uc, UC_X86_REG_RDX, &rdx);
            uint8_t memset_byte = rsi & 0xff;
            for (int i=0; i<rdx; i++){
                uc_mem_write(uc, rdi+i, &memset_byte, 1);
            }
            DEBUG(">>> memset(0x%"PRIx64 ", 0x%"PRIx64 ", 0x%"PRIx64 ")", rdi, rsi, rdx);

            uc_reg_read(uc, UC_X86_REG_RSP, &rsp);
            // read 8 bytes at rsp 
            uc_mem_read(uc, rsp, &new_rip, 8);
            DEBUG("\t new_rip: 0x%"PRIx64 "", new_rip);
            uc_reg_write(uc, UC_X86_REG_RIP, &new_rip);

            break;



        // case PKEY_SET_ADDR:
        //     printf(">>> hooking pkey_set\n");
        //     uc_reg_read(uc, UC_X86_REG_RAX, &rax);
        //     key_arr[0] = rax;
        //     uc_reg_read(uc, UC_X86_REG_RIP, &rip);
        //     new_rip = rip + 3;
        //     uc_reg_write(uc, UC_X86_REG_RIP, &new_rip);

        //     break;

        default:
            break;
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
    if (input_len < 0x100) {
        // Test input too short ignore this testcase
        return false;
    }

    // We need a valid c string, make sure it never goes out of bounds.
    input[input_len-1] = '\0';
    // Write the testcase to unicorn.
    uc_mem_write(uc,  INPUT_ADDR, input, input_len);
    DEBUG("### Placed input with len %ld to 0x%x", input_len, INPUT_ADDR);

    // store input_len for the faux strlen hook
    current_input_len = input_len;

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

    /* setup for cs */

    /* nop some dummy instructions */
    // uc_mem_protect(uc, 0x7fffec996a00, 0x1000, UC_PROT_READ | UC_PROT_EXEC | UC_PROT_WRITE);
    // uc_mem_write(uc, 0x7fffec996ab6, nop_arr, 0x18);
    // uc_mem_protect(uc, 0x7fffec996a00, 0x1000, UC_PROT_READ | UC_PROT_EXEC);



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
    
    // ======================= load context ===============================

    // Set the program counter to the start of the code
    uint64_t start_address = 0x00005555b6b80054;      // address of entry point of main()
    uint64_t end_address = 0x5555b6b8015d; // Address of last instruction in main()

    // If we want tracing output, set the callbacks here
    uc_hook hooks[2];
    uc_hook_add(uc, &hooks[0], UC_HOOK_CODE, hook_code, NULL, start_address, end_address);

    // start fuzzing
    DEBUG("Starting to fuzz...");
    fflush(stdout);

    if (tracing) {
        dump_registers(uc);
    }

    // let's gooo
    uc_afl_ret afl_ret = uc_afl_fuzz(
        uc, // The unicorn instance we prepared
        filename, // Filename of the input to process. In AFL this is usually the '@@' placeholder, outside it's any input file.
        place_input_callback, // Callback that places the input (automatically loaded from the file at filename) in the unicorninstance
        &end_address, // Where to exit (this is an array)
        1,  // Count of end addresses
        NULL, // Optional calback to run after each exec
        false, // true, if the optional callback should be run also for non-crashes
        10, // For persistent mode: How many rounds to run
        NULL // additional data pointer
    );
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

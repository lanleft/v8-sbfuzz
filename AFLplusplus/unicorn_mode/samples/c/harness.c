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
#define UNICORN_AFL

#include "UnicornLoader.h"

uint64_t current_input_len = 0;

static void load_context(uc_engine** uc, const char* context_dir) {
    char index_path[256];
    snprintf(index_path, sizeof(index_path), "%s/_index.json", context_dir);

    FILE* f = fopen(index_path, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open _index.json\n");
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* json_str = malloc(fsize + 1);
    fread(json_str, 1, fsize, f);
    fclose(f);

    json_str[fsize] = 0;

    cJSON* context = cJSON_Parse(json_str);
    free(json_str);

    if (!context) {
        fprintf(stderr, "Failed to parse _index.json\n");
        exit(1);
    }


    cJSON* arch = cJSON_GetObjectItemCaseSensitive(context, "arch");
    arch = cJSON_GetObjectItemCaseSensitive(arch, "arch");
    // cJSON* mode = cJSON_GetObjectItemCaseSensitive(context, "mode");
    if (!cJSON_IsString(arch)) {
        fprintf(stderr, "Invalid or missing 'arch' in _index.json\n");
        exit(1);
    }

    struct uc_settings settings = get_arch_and_mode(arch->valuestring);

    uc_err err = uc_open(settings.arch, settings.mode, uc);
    if (err != UC_ERR_OK) {
        fprintf(stderr, "Failed to initialize Unicorn engine: %s\n", uc_strerror(err));
        exit(1);
    }

    cJSON* regs = cJSON_GetObjectItemCaseSensitive(context, "regs");
    if (cJSON_IsObject(regs)) {
        load_registers(*uc, regs);
    } else {
        fprintf(stderr, "Invalid or missing 'regs' in _index.json\n");
        exit(1);
    }

    cJSON* segments = cJSON_GetObjectItemCaseSensitive(context, "segments");
    if (cJSON_IsArray(segments)) {
        map_segments(*uc, segments, context_dir);
    } else {
        fprintf(stderr, "Invalid or missing 'segments' in _index.json\n");
        exit(1);
    }

    cJSON_Delete(context);
}


// ========================================================================================

// static void hook_block(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
//     // printf(">>> Tracing basic block at 0x%"PRIx64 ", block size = 0x%x\n", address, size);
// }

static void hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {

    uint64_t rip;

    switch (address){
        case 0x7ffff7dc5915:
            printf("####  Hooking address 0x7ffff7dc5915\n");
            dump_registers(uc);
            unsigned char rdi_data[0x20] = {0};
            uint64_t rdi_value;
            uc_reg_read(uc, UC_X86_REG_RDI, &rdi_value);

            printf("\t rdi_value: 0x%"PRIx64 "\n", rdi_value);
            uc_mem_read(uc, rdi_value, rdi_data, 0x20);
            uint32_t eax_value = 0;
            uint64_t rax_value = 0;
            uc_reg_read(uc, UC_X86_REG_RAX, &rax_value);
            eax_value = rax_value & 0xffffffff;
            printf("\t eax_value: 0x%x\n", eax_value);

            for (int i=0; i<0x20; i++){
                if(rdi_data[i] == 0){
                    eax_value |= 1 << i;
                } 
            }
            printf("\t eax: 0x%x\n", eax_value);
            uc_reg_write(uc, UC_X86_REG_RAX, &eax_value);
            // add rip by 8
            uc_reg_read(uc, UC_X86_REG_RIP, &rip);
            uint64_t new_rip = rip + 8;
            uc_reg_write(uc, UC_X86_REG_RIP, &new_rip);
            printf("Skipped 2 instructions at 0x7ffff7dc5915. New RIP: 0x%" PRIx64 "\n", new_rip);

            break;
        default:
            break;
    }

    unsigned char code[16] = {0};
    uc_mem_read(uc, address, code, size);
    // printf("size: %d\n", size);
    // for (int i = 0; i < size; i++) {
    //     printf("    0x%"PRIx64 ": %02x\n", address + i, code[i]);
    // }
    char asm_buf[256] = {0};
    size_t count;

    // use cs_disasm
    cs_insn *insn;
    csh handle;
    cs_open(CS_ARCH_X86, CS_MODE_64, &handle);
    count = cs_disasm(handle, code, size, address, 0, &insn);
    if (count > 0) {
        snprintf(asm_buf, sizeof(asm_buf), "%s %s", insn[0].mnemonic, insn[0].op_str);
        cs_free(insn, count);
    } else {
        snprintf(asm_buf, sizeof(asm_buf), "invalid");
        
    }
    // printf("asm:\n");
    printf("    0x%"PRIx64 ": %s\n", address, asm_buf);


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
    if (input_len < 1 || input_len >= 65537 - 22) {
        // Test input too short or too long, ignore this testcase
        return false;
    }

    // We need a valid c string, make sure it never goes out of bounds.
    input[input_len-1] = '\0';
    // Write the testcase to unicorn.
    uc_mem_write(uc,  + 22, input, input_len);

    // store input_len for the faux strlen hook
    current_input_len = input_len;

    return true;
}

uc_engine* init_unicorn(const char* context_dir) {
    uc_engine* uc;
    load_context(&uc, context_dir);
    
    // enable some features on cpu 


    // setup value for fs:0x28
    uc_mem_map(uc, 0, 0x2000, UC_PROT_ALL);
    uint8_t a[8] = "\x00\x2b\x9b\x82\x26\xdf\xc6\x87";
    uc_mem_write(uc, 0x28, a, 8); // canary 




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
    uint64_t start_address = 0x555556c19100;      // address of entry point of main()
    uint64_t end_address = 0xff5556c19100; // Address of last instruction in main()

    // If we want tracing output, set the callbacks here
    uc_hook hooks[2];
    uc_hook_add(uc, &hooks[0], UC_HOOK_CODE, hook_code, NULL, start_address, end_address);

    // Set up rip
    uc_reg_write(uc, UC_X86_REG_RIP, &start_address); // Set the instruction pointer back

    // start fuzzing
    printf("Starting to fuzz...\n");
    fflush(stdout);

    dump_registers(uc);

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
            printf("Error starting to fuzz");
            return -3;
            break;
        case UC_AFL_RET_NO_AFL:
            printf("No AFL attached - We are done with a single run.");
            break;
        default:
            break;
    } 
    return 0;
}

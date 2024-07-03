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
*/

// This is not your everyday Unicorn.
#define UNICORN_AFL

#include <string.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <unicorn/unicorn.h>
#include <unicornafl/unicornafl.h>
#include <capstone/capstone.h>

#include "cJSON/cJSON.h"
#include <string.h>

#define DEBUG(fmt, ...) do { \
    if (tracing) { printf(fmt, ##__VA_ARGS__); putchar('\n'); } \
} while (0)

// My own defines
const char* INDEX_FILE_NAME = "_index.json";
const uint64_t MAX_ALLOWABLE_SEG_SIZE = 1024*1024*1024;
const uint64_t UNICORN_PAGE_SIZE = 0x1000;
bool tracing = false;

struct uc_settings{
    uc_arch arch;
    uc_mode mode;
};
#define MAX_REGISTERS 20

typedef struct {
    char name[20];
    int reg_id;
} Register;

static Register registers[MAX_REGISTERS];
static int register_count = 0;

static void load_context(uc_engine** uc, const char* context_dir, bool debug_trace);
static int get_reg_id(uc_engine* uc, const char* reg_name);

static void map_segments(uc_engine* uc, cJSON* segments, bool debug_trace);
static void map_segment(uc_engine* uc, const char* name, uint64_t address, uint64_t size, int perms, bool debug_trace);
static void load_registers(uc_engine* uc, cJSON* regs);
static uc_arch get_arch_from_string(const char* arch_str);
static uc_mode get_mode_from_string(const char* mode_str);

uc_engine* init_unicorn(const char* context_dir, bool debug_trace) {
    uc_engine* uc;
    load_context(&uc, context_dir, debug_trace);
    return uc;
}

static void load_context(uc_engine** uc, const char* context_dir, bool debug_trace) {
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
    cJSON* mode = cJSON_GetObjectItemCaseSensitive(context, "mode");
    if (!cJSON_IsString(arch) || !cJSON_IsString(mode)) {
        fprintf(stderr, "Invalid or missing 'arch' or 'mode' in _index.json\n");
        exit(1);
    }

    uc_arch uc_arch = get_arch_from_string(arch->valuestring);
    uc_mode uc_mode = get_mode_from_string(mode->valuestring);

    uc_err err = uc_open(uc_arch, uc_mode, uc);
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
        map_segments(*uc, segments, debug_trace);
    } else {
        fprintf(stderr, "Invalid or missing 'segments' in _index.json\n");
        exit(1);
    }

    cJSON_Delete(context);
}

static int get_reg_id(uc_engine* uc, const char* reg_name) {
    uc_arch arch;
    uc_mode mode;
    uc_query(uc, UC_QUERY_ARCH, &arch);
    uc_query(uc, UC_QUERY_MODE, &mode);

    if (arch == UC_ARCH_X86) {
        if (mode == UC_MODE_32) {
            if (strcmp(reg_name, "eax") == 0) return UC_X86_REG_EAX;
            if (strcmp(reg_name, "ebx") == 0) return UC_X86_REG_EBX;
            if (strcmp(reg_name, "ecx") == 0) return UC_X86_REG_ECX;
            if (strcmp(reg_name, "edx") == 0) return UC_X86_REG_EDX;
            if (strcmp(reg_name, "esi") == 0) return UC_X86_REG_ESI;
            if (strcmp(reg_name, "edi") == 0) return UC_X86_REG_EDI;
            if (strcmp(reg_name, "ebp") == 0) return UC_X86_REG_EBP;
            if (strcmp(reg_name, "esp") == 0) return UC_X86_REG_ESP;
            if (strcmp(reg_name, "eip") == 0) return UC_X86_REG_EIP;
            if (strcmp(reg_name, "eflags") == 0) return UC_X86_REG_EFLAGS;
        } else if (mode == UC_MODE_64) {
            if (strcmp(reg_name, "rax") == 0) return UC_X86_REG_RAX;
            if (strcmp(reg_name, "rbx") == 0) return UC_X86_REG_RBX;
            if (strcmp(reg_name, "rcx") == 0) return UC_X86_REG_RCX;
            if (strcmp(reg_name, "rdx") == 0) return UC_X86_REG_RDX;
            if (strcmp(reg_name, "rsi") == 0) return UC_X86_REG_RSI;
            if (strcmp(reg_name, "rdi") == 0) return UC_X86_REG_RDI;
            if (strcmp(reg_name, "rbp") == 0) return UC_X86_REG_RBP;
            if (strcmp(reg_name, "rsp") == 0) return UC_X86_REG_RSP;
            if (strcmp(reg_name, "rip") == 0) return UC_X86_REG_RIP;
            if (strcmp(reg_name, "r8") == 0) return UC_X86_REG_R8;
            if (strcmp(reg_name, "r9") == 0) return UC_X86_REG_R9;
            if (strcmp(reg_name, "r10") == 0) return UC_X86_REG_R10;
            if (strcmp(reg_name, "r11") == 0) return UC_X86_REG_R11;
            if (strcmp(reg_name, "r12") == 0) return UC_X86_REG_R12;
            if (strcmp(reg_name, "r13") == 0) return UC_X86_REG_R13;
            if (strcmp(reg_name, "r14") == 0) return UC_X86_REG_R14;
            if (strcmp(reg_name, "r15") == 0) return UC_X86_REG_R15;
            if (strcmp(reg_name, "rflags") == 0) return UC_X86_REG_EFLAGS;
        }
    }
    // Add more architectures as needed

    fprintf(stderr, "Unknown register: %s\n", reg_name);
    return -1;
}

static void load_registers(uc_engine* uc, cJSON* regs) {
    cJSON* reg;
    cJSON_ArrayForEach(reg, regs) {
        if (register_count >= MAX_REGISTERS) {
            fprintf(stderr, "Too many registers\n");
            exit(1);
        }

        strncpy(registers[register_count].name, reg->string, sizeof(registers[register_count].name) - 1);
        registers[register_count].reg_id = get_reg_id(uc, reg->string);
        
        uint64_t value = (uint64_t)cJSON_GetNumberValue(reg);
        uc_reg_write(uc, registers[register_count].reg_id, &value);

        register_count++;
    }
}

static void map_segments(uc_engine* uc, cJSON* segments, bool debug_trace) {
    cJSON* segment;
    cJSON_ArrayForEach(segment, segments) {
        cJSON* name = cJSON_GetObjectItemCaseSensitive(segment, "name");
        cJSON* start = cJSON_GetObjectItemCaseSensitive(segment, "start");
        cJSON* end = cJSON_GetObjectItemCaseSensitive(segment, "end");
        cJSON* permissions = cJSON_GetObjectItemCaseSensitive(segment, "permissions");

        if (!cJSON_IsString(name) || !cJSON_IsNumber(start) || !cJSON_IsNumber(end) || !cJSON_IsObject(permissions)) {
            fprintf(stderr, "Invalid segment data\n");
            continue;
        }

        int perms = 0;
        cJSON* r = cJSON_GetObjectItemCaseSensitive(permissions, "r");
        cJSON* w = cJSON_GetObjectItemCaseSensitive(permissions, "w");
        cJSON* x = cJSON_GetObjectItemCaseSensitive(permissions, "x");

        if (cJSON_IsTrue(r)) perms |= UC_PROT_READ;
        if (cJSON_IsTrue(w)) perms |= UC_PROT_WRITE;
        if (cJSON_IsTrue(x)) perms |= UC_PROT_EXEC;

        map_segment(uc, name->valuestring, (uint64_t)start->valuedouble, (uint64_t)end->valuedouble - (uint64_t)start->valuedouble, perms, debug_trace);
    }
}

static void map_segment(uc_engine* uc, const char* name, uint64_t address, uint64_t size, int perms, bool debug_trace) {
    uc_err err = uc_mem_map(uc, address, size, perms);
    if (err != UC_ERR_OK) {
        fprintf(stderr, "Failed to map memory for segment %s: %s\n", name, uc_strerror(err));
        return;
    }

    DEBUG("Mapped segment %s: 0x%lx - 0x%lx, perms: %d", name, address, address + size, perms);
}

static uc_arch get_arch_from_string(const char* arch_str) {
    if (strcmp(arch_str, "x86") == 0) return UC_ARCH_X86;
    if (strcmp(arch_str, "arm") == 0) return UC_ARCH_ARM;
    // Add more architectures as needed
    fprintf(stderr, "Unsupported architecture: %s\n", arch_str);
    exit(1);
}

static uc_mode get_mode_from_string(const char* mode_str) {
    if (strcmp(mode_str, "32") == 0) return UC_MODE_32;
    if (strcmp(mode_str, "64") == 0) return UC_MODE_64;
    // Add more modes as needed
    fprintf(stderr, "Unsupported mode: %s\n", mode_str);
    exit(1);
}

void cleanup_unicorn(uc_engine* uc) {
    if (uc) {
        uc_close(uc);
    }
}

void dump_registers(uc_engine* uc) {
    for (int i = 0; i < register_count; i++) {
        uint64_t value;
        uc_reg_read(uc, registers[i].reg_id, &value);
        printf("%s: 0x%lx\n", registers[i].name, value);
    }
}

void force_crash(uc_err err) {
    fprintf(stderr, "Forced crash: %s\n", uc_strerror(err));
    exit(1);
}

// ==================================== sample =======================================
// Path to the file containing the binary to emulate
#define BINARY_FILE ("persistent_target_x86_64")

// Memory map for the code to be tested
// Arbitrary address where code to test will be loaded
static const int64_t BASE_ADDRESS = 0x100000;
static const int64_t CODE_ADDRESS = 0x101139;
static const int64_t END_ADDRESS = 0x10120d;
// Address of the stack (Some random address again)
static const int64_t STACK_ADDRESS = (((int64_t) 0x01) << 32);
// Size of the stack (arbitrarily chosen, just make it big enough)
static const int64_t STACK_SIZE = 0x10000;
// Location where the input will be placed (make sure the emulated program knows this somehow, too ;) )
static const int64_t INPUT_LOCATION = 0x10000;
// Inside the location, we have an ofset in our special case
static const int64_t INPUT_OFFSET = 0x16;
// Maximum allowable size of mutated data from AFL
static const int64_t INPUT_SIZE_MAX = 0x10000;
// Alignment for unicorn mappings (seems to be needed)
static const int64_t ALIGNMENT = 0x1000;

// In our special case, we emulate main(), so argc is needed.
static const uint64_t EMULATED_ARGC = 2;

// The return from our fake strlen
static size_t current_input_len = 0;
// ========================================================================================

static void hook_block(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
    // printf(">>> Tracing basic block at 0x%"PRIx64 ", block size = 0x%x\n", address, size);
}

static void hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
    // printf(">>> Tracing instruction at 0x%"PRIx64 ", instruction size = 0x%x\n", address, size);
    // print disassemply
    unsigned char code[16];
    uc_mem_read(uc, address, code, sizeof(code));
    char asm_buf[32];
    size_t count;
    // use cs_disasm
    cs_insn *insn;
    csh handle;
    cs_open(CS_ARCH_X86, CS_MODE_64, &handle);
    count = cs_disasm(handle, code, sizeof(code), address, 0, &insn);
    if (count > 0) {
        snprintf(asm_buf, sizeof(asm_buf), "%s %s", insn[0].mnemonic, insn[0].op_str);
        cs_free(insn, count);
    } else {
        snprintf(asm_buf, sizeof(asm_buf), "invalid");
    }
    // printf("asm:\n");
    printf("    0x%"PRIx64 ": %s\n", address, asm_buf);
}

/*
The sample uses strlen, since we don't have a loader or libc, we'll fake it.
We know the strlen will return the lenght of argv[1] that we just planted.
It will be a lot faster than an actual strlen for this specific purpose.
*/
static void hook_strlen(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
    //Hook
    //116b:       e8 c0 fe ff ff          call   1030 <strlen@plt>
    // We place the return at RAX
    //printf("Strlen hook at addr 0x%llx (size: 0x%x), result: %ld\n", address, size, current_input_len);
    uc_reg_write(uc, UC_X86_REG_RAX, &current_input_len);
    // We skip the actual call by updating RIP
    uint64_t next_addr = address + size; 
    uc_reg_write(uc, UC_X86_REG_RIP, &next_addr);
}

/* Unicorn page needs to be 0x1000 aligned, apparently */
static uint64_t pad(uint64_t size) {
    if (size % ALIGNMENT == 0) return size;
    return ((size / ALIGNMENT) + 1) * ALIGNMENT;
} 

/* returns the filesize in bytes, -1 or error. */
static off_t afl_mmap_file(char *filename, char **buf_ptr) {

    off_t ret = -1;

    int fd = open(filename, O_RDONLY);

    struct stat st = {0};
    if (fstat(fd, &st)) goto exit;

    off_t in_len = st.st_size;
    if (in_len == -1) {
	/* This can only ever happen on 32 bit if the file is exactly 4gb. */
	fprintf(stderr, "Filesize of %s too large", filename);
	goto exit;
    }

    *buf_ptr = mmap(0, in_len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

    if (*buf_ptr != MAP_FAILED) ret = in_len;

exit:
    close(fd);
    return ret;

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
    if (input_len < 1 || input_len >= INPUT_SIZE_MAX - INPUT_OFFSET) {
        // Test input too short or too long, ignore this testcase
        return false;
    }

    // We need a valid c string, make sure it never goes out of bounds.
    input[input_len-1] = '\0';
    // Write the testcase to unicorn.
    uc_mem_write(uc, INPUT_LOCATION + INPUT_OFFSET, input, input_len);

    // store input_len for the faux strlen hook
    current_input_len = input_len;

    return true;
}

static void mem_map_checked(uc_engine *uc, uint64_t addr, size_t size, uint32_t mode) {
    size = pad(size);
    //printf("SIZE %llx, align: %llx\n", size, ALIGNMENT);
    uc_err err = uc_mem_map(uc, addr, size, mode);
    if (err != UC_ERR_OK) {
        printf("Error mapping %ld bytes at 0x%llx: %s (mode: %d)\n", size, (unsigned long long) addr, uc_strerror(err), (int) mode);
        exit(1);
    }
}

char* read_index(const char* context_dir){
    // Making full path of index file
    char index_file_path[1024];
    strcpy(index_file_path, context_dir);
    strcat(index_file_path, "/");
    strcat(index_file_path, INDEX_FILE_NAME);
    DEBUG("Index file path: %s", index_file_path);

    FILE *file = fopen(index_file_path, "rb");
    if (!file) {
        perror("File opening failed");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(length + 1);
    if (content) {
        fread(content, 1, length, file);
    }

    fclose(file);
    content[length] = '\0'; // Null-terminate the string
    return content;
    

}

static struct uc_settings _get_arch_and_mode(const char* arch_str) {
    struct uc_settings settings = {0};
    if (strcmp(arch_str, "x86_64") == 0) {
        settings.arch = UC_ARCH_X86;
        settings.mode = UC_MODE_64;
    } else if (strcmp(arch_str, "x86") == 0) {
        settings.arch = UC_ARCH_X86;
        settings.mode = UC_MODE_32;
    } else {
        fprintf(stderr, "Unsupported architecture: %s\n", arch_str);
        exit(1);
    }
    return settings;
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

   const char* context_dir_env = getenv("CONTEXT_DIR");

    if(!context_dir_env){
        printf("CONTEXT_DIR is not set.\n");
        exit(1);
    }
    // ===================== start unicorn ===============================
    uc_err err;
    struct uc_settings uc_set;
    uc_engine* uc = init_unicorn(context_dir_env, tracing);
    
    // ======================= load context ===============================


    uc_engine *uc;
    // Set the program counter to the start of the code
    uint64_t start_address = CODE_ADDRESS;      // address of entry point of main()
    uint64_t end_address = END_ADDRESS; // Address of last instruction in main()

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

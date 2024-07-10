// add pragma once
#pragma once

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
#include <stdio.h>
#include <stdlib.h>
                    


#define DEBUG(fmt, ...) do { \
    if (tracing) { printf(fmt, ##__VA_ARGS__); putchar('\n'); } \
} while (0)


struct uc_settings{
    uc_arch arch;
    uc_mode mode;
};
#define MAX_REGISTERS 100

typedef struct {
    char name[20];
    int reg_id;
} Register;

static Register registers[MAX_REGISTERS];
static int register_count = 0;
bool tracing = false;
// My own defines
const char* INDEX_FILE_NAME = "_index.json";
const uint64_t MAX_ALLOWABLE_SEG_SIZE = 1024*1024*1024;
const uint64_t UNICORN_PAGE_SIZE = 0x1000;

static void load_context(uc_engine** uc, const char* context_dir);
static int get_reg_id(uc_engine* uc, const char* reg_name);
static struct uc_settings get_arch_and_mode(const char* arch_str);

static void map_segments(uc_engine* uc, cJSON* segments, const char* context_directory);
static void map_segment(uc_engine* uc, const char* name, uint64_t address, uint64_t size, int perms);
static void load_registers(uc_engine* uc, cJSON* regs);




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

    char *content = (char*)malloc(length + 1);
    if (content) {
        fread(content, 1, length, file);
    }

    fclose(file);
    content[length] = '\0'; // Null-terminate the string
    return content;
    

}

static struct uc_settings get_arch_and_mode(const char* arch_str) {
    struct uc_settings settings = {};
    if (strcmp(arch_str, "x64") == 0) {
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


static int get_reg_id(uc_engine* uc, const char* reg_name) {
    size_t arch, mode;
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
            if (strcmp(reg_name, "r8") == 0) return UC_X86_REG_R8;
            if (strcmp(reg_name, "r9") == 0) return UC_X86_REG_R9;
            if (strcmp(reg_name, "r10") == 0) return UC_X86_REG_R10;
            if (strcmp(reg_name, "r11") == 0) return UC_X86_REG_R11;
            if (strcmp(reg_name, "r12") == 0) return UC_X86_REG_R12;
            if (strcmp(reg_name, "r13") == 0) return UC_X86_REG_R13;
            if (strcmp(reg_name, "r14") == 0) return UC_X86_REG_R14;
            if (strcmp(reg_name, "r15") == 0) return UC_X86_REG_R15;
            if (strcmp(reg_name, "rip") == 0) return UC_X86_REG_RIP;

            // 32-bit registers
            if (strcmp(reg_name, "eax") == 0) return UC_X86_REG_EAX;
            if (strcmp(reg_name, "ebx") == 0) return UC_X86_REG_EBX;
            if (strcmp(reg_name, "ecx") == 0) return UC_X86_REG_ECX;
            if (strcmp(reg_name, "edx") == 0) return UC_X86_REG_EDX;
            if (strcmp(reg_name, "esi") == 0) return UC_X86_REG_ESI;
            if (strcmp(reg_name, "edi") == 0) return UC_X86_REG_EDI;
            if (strcmp(reg_name, "ebp") == 0) return UC_X86_REG_EBP;
            if (strcmp(reg_name, "esp") == 0) return UC_X86_REG_ESP;

            // 16-bit registers
            if (strcmp(reg_name, "ax") == 0) return UC_X86_REG_AX;
            if (strcmp(reg_name, "bx") == 0) return UC_X86_REG_BX;
            if (strcmp(reg_name, "cx") == 0) return UC_X86_REG_CX;
            if (strcmp(reg_name, "dx") == 0) return UC_X86_REG_DX;
            if (strcmp(reg_name, "si") == 0) return UC_X86_REG_SI;
            if (strcmp(reg_name, "di") == 0) return UC_X86_REG_DI;
            if (strcmp(reg_name, "bp") == 0) return UC_X86_REG_BP;
            if (strcmp(reg_name, "sp") == 0) return UC_X86_REG_SP;

            // 8-bit registers
            if (strcmp(reg_name, "al") == 0) return UC_X86_REG_AL;
            if (strcmp(reg_name, "ah") == 0) return UC_X86_REG_AH;
            if (strcmp(reg_name, "bl") == 0) return UC_X86_REG_BL;
            if (strcmp(reg_name, "bh") == 0) return UC_X86_REG_BH;
            if (strcmp(reg_name, "cl") == 0) return UC_X86_REG_CL;
            if (strcmp(reg_name, "ch") == 0) return UC_X86_REG_CH;
            if (strcmp(reg_name, "dl") == 0) return UC_X86_REG_DL;
            if (strcmp(reg_name, "dh") == 0) return UC_X86_REG_DH;
            if (strcmp(reg_name, "sil") == 0) return UC_X86_REG_SIL;
            if (strcmp(reg_name, "dil") == 0) return UC_X86_REG_DIL;
            if (strcmp(reg_name, "bpl") == 0) return UC_X86_REG_BPL;
            if (strcmp(reg_name, "spl") == 0) return UC_X86_REG_SPL;

            // Segment registers
            if (strcmp(reg_name, "cs") == 0) return UC_X86_REG_CS;
            if (strcmp(reg_name, "ds") == 0) return UC_X86_REG_DS;
            if (strcmp(reg_name, "es") == 0) return UC_X86_REG_ES;
            if (strcmp(reg_name, "fs") == 0) return UC_X86_REG_FS;
            if (strcmp(reg_name, "gs") == 0) return UC_X86_REG_GS;
            if (strcmp(reg_name, "ss") == 0) return UC_X86_REG_SS;
            // Special registers
            if (strcmp(reg_name, "eflags") == 0) return UC_X86_REG_EFLAGS;
            if (strcmp(reg_name, "fsbase") == 0) return 100; // UC_X86_REG_FS_BASE;
            if (strcmp(reg_name, "gsbase") == 0) return 100; // UC_X86_REG_GS_BASE;
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

static void map_segments(uc_engine* uc, cJSON* segments, const char* context_directory) {
    cJSON* segment;
    cJSON_ArrayForEach(segment, segments) {
        cJSON* name = cJSON_GetObjectItemCaseSensitive(segment, "name");
        cJSON* start = cJSON_GetObjectItemCaseSensitive(segment, "start");
        cJSON* end = cJSON_GetObjectItemCaseSensitive(segment, "end");
        cJSON* permissions = cJSON_GetObjectItemCaseSensitive(segment, "permissions");

        uint64_t seg_start = (uint64_t)start->valuedouble;
        uint64_t seg_end = (uint64_t)end->valuedouble;
        const char* seg_name = name->valuestring;

        DEBUG(" ===== Map segment name: %s, start: 0x%lx, end: 0x%lx ==========", seg_name, seg_start, seg_end);

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

        map_segment(uc, seg_name, seg_start, seg_end - seg_start, perms);

        // Load content if available
        cJSON *content_file_json = cJSON_GetObjectItemCaseSensitive(segment, "content_file");
        const char *content_file = cJSON_IsString(content_file_json) ? content_file_json->valuestring : NULL;
        
        if (content_file && strlen(content_file) > 0) {
            char content_file_path[1024];
            snprintf(content_file_path, sizeof(content_file_path), "%s/%s", context_directory, content_file);

            FILE *content_file_handle = fopen(content_file_path, "rb");
            if (!content_file_handle) {
                fprintf(stderr, "Unable to find segment content file. Expected it to be at %s\n", content_file_path);
                return;
            }

            DEBUG("Loading content for segment %s from file %s", seg_name, content_file_path);
            // Get file size
            fseek(content_file_handle, 0, SEEK_END);
            long file_size = ftell(content_file_handle);
            fseek(content_file_handle, 0, SEEK_SET);

            // Read file content
            unsigned char *content = (unsigned char*)malloc(file_size);
            if (!content) {
                fprintf(stderr, "Memory allocation failed\n");
                fclose(content_file_handle);
                return;
            }

            size_t bytes_read = fread(content, 1, file_size, content_file_handle);
            fclose(content_file_handle);

            if (bytes_read != file_size) {
                fprintf(stderr, "Failed to read entire file\n");
                free(content);
                return;
            }

            DEBUG("Read %ld bytes from file %s", file_size, content_file_path);
            // Write content to memory
            uc_err err = uc_mem_write(uc, seg_start, content, file_size);
            if (err != UC_ERR_OK) {
                fprintf(stderr, "Failed to write memory: %s\n", uc_strerror(err));
            }

            DEBUG("Loaded content for segment %s @ 0x%016lx\n", seg_name, seg_start);

            free(content);
        } else {
            DEBUG("No content found for segment %s @ 0x%016lx\n", seg_name, seg_start);

            // Fill memory with zeros
            size_t size = seg_end - seg_start;
            unsigned char *zeros = (unsigned char*)calloc(size, 1);
            if (!zeros) {
                fprintf(stderr, "Memory allocation failed\n");
                return;
            }

            uc_err err = uc_mem_write(uc, seg_start, zeros, size);
            if (err != UC_ERR_OK) {
                fprintf(stderr, "Failed to write memory: %s\n", uc_strerror(err));
            }

            free(zeros);
        }
    }
}

static void map_segment(uc_engine* uc, const char* name, uint64_t address, uint64_t size, int perms) {
    uc_err err = uc_mem_map(uc, address, size, perms);
    if (err != UC_ERR_OK) {
        fprintf(stderr, "Failed to map memory for segment %s: %s\n", name, uc_strerror(err));
        return;
    }

    DEBUG("Mapped segment %s: 0x%lx - 0x%lx, perms: %d", name, address, address + size, perms);
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
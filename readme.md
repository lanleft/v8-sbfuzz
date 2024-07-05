
# Table of content

- [Setup](#setup)


# Setup 

1. Install v8, depot_tools+AFLplusplus

- Remember changing UID:GID in Dockerfile before building


```bash
# install depot_tools
# Conditional git clone if the directory is empty
# RUN mkdir -p /home/vult/depot_tools && \
#     [ "$(ls -A /home/vult/depot_tools)" ] || git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git /home/vult/depot_tools
# ENV PATH="/home/vult/depot_tools:${PATH}"

# # install AFLplusplus
# # Conditional git clone if the directory is empty
# # AFLplusplus version 4.10a
# RUN mkdir -p /home/vult/AFLplusplus && \
#     [ "$(ls -A /home/vult/AFLplusplus)" ] || (\
#     git clone https://github.com/AFLplusplus/AFLplusplus.git /home/vult/AFLplusplus && \
#     cd /home/vult/AFLplusplus && \
#     git checkout ca0c9f6d1797bac121996c3b2ac50423f6e67b8f)
```

2. run dockerfile: `sudo docker compose build && sudo docker compose run v8 zsh`


3. Build AFL++ and v8 release+debug

```bash
cd AFLplusplus
make distrib
sudo make install

```


# Unicorn fuzzer

Blogs:
    - https://eternalsakura13.com/2020/03/18/unicorn_learn/
    - https://hackernoon.com/afl-unicorn-part-2-fuzzing-the-unfuzzable-bea8de3540a5

## Dumpper

```bash
source /home/vult/v8_sb_fuzz/AFLplusplus/unicorn_mode/helper_scripts/unicorn_dumper_pwndbg.py
```

Example 1:

![alt text](image.png)


**Harness**

- C: https://github.com/AFLplusplus/AFLplusplus/blob/stable/unicorn_mode/samples/c/harness.c
- Python: https://github.com/AFLplusplus/AFLplusplus/blob/stable/unicorn_mode/samples/python_simple/simple_test_harness.py
- Unicorn-fuzz: https://github.com/unicorn-engine/unicorn/blob/master/tests/fuzz/fuzz_emu_x86_64.c
- https://github.com/domenukk/unicornafl
- https://github.com/kirasys/unicorn-fuzzer
- samples test: https://github.com/unicorn-engine/unicorn/blob/d4b92485b1a228fb003e1218e42f6c778c655809/samples/sample_x86.c#L1322
- translate: https://github.com/qemu/qemu/commit/84abdd7d271c2df69a9d394be093efd885da7a4c
- prompts for asking questions: https://github.com/f/awesome-chatgpt-prompts/tree/main


**Git**

```bash
# reset after `git add`
git reset -- AFLplusplus/unicorn_mode/samples/c/UnicornContext_20240704_110015/

```


### Run sample fuzz

`/home/vult/v8_sb_fuzz/AFLplusplus/unicorn_mode/samples/c`

```bash
AFL_DEBUG=1 afl-fuzz -U -m none -i sample_inputs/ -o fuzz_out1 -- ./harness @@
```

I'm working on folder `/home/vult/AFLplusplus/unicorn_mode/samples/c` of docker v8

**snapshot_blob.bin**

```
Sandbox testing mode is enabled. Write to the page starting at 0x3d9042d22000 (available from JavaScript as `Sandbox.targetPage`) to demonstrate a sandbox bypass.
Failed to open startup resource '/home/vult/v8_sb_fuzz/AFLplusplus/unicorn_mode/samples/c/snapshot_blob.bin'.


#
# Safely terminating process due to error in , line 0
# The following harmless error was encountered: Failed to deserialize the V8 snapshot blob. This can mean that the snapshot blob file is corrupted or missing.
#
#
#
#FailureMessage Object: 0x7fffffffd9d0

```

```c++
// emulating 1 instruction
uc_err uc_emu_start(uc_engine *uc, uint64_t begin, uint64_t until,
                    uint64_t timeout, size_t count)
{}

/* main execution loop */
int cpu_exec(struct uc_struct *uc, CPUState *cpu)
{
    CPUClass *cc = CPU_GET_CLASS(cpu);
    int ret;
    // SyncClocks sc = { 0 };

    if (cpu_handle_halt(cpu)) {
        return EXCP_HALTED;
    }
```

Can not `jmp`

```js
    0x555556ca2223: mov eax, 2
    0x555556ca2228: mov rdi, rbx
    0x555556ca222b: mov qword ptr [rsp + 0x10], rcx
=== tlb_index: mmu_idx: 2, addr: 7fffffffd610 ===
=== tlb_index: mmu_idx: 2, addr: 7fffffffd610 ===
    0x555556ca2230: mov rsi, r8
    0x555556ca2233: jmp 0x555556b04740
=== tlb_index: mmu_idx: 2, addr: 555556b04740 ===
=== tlb_index: mmu_idx: 2, addr: 555556b04740 ===
//....
=== tlb_index: mmu_idx: 2, addr: 7fffffffd3e0 ===
=== tlb_index: mmu_idx: 2, addr: 7fffffffd3d8 ===
=== tlb_index: mmu_idx: 2, addr: 7fffffffd3d8 ===
=== tlb_index: mmu_idx: 2, addr: 28 ===
=== tlb_index: mmu_idx: 2, addr: 28 ===
=== tlb_index: mmu_idx: 2, addr: 0 ===
=== tlb_index: mmu_idx: 2, addr: 0 ===
=== tlb_index: mmu_idx: 2, addr: 28 ===
=== tlb_index: mmu_idx: 2, addr: 28 ===
>>> invalid memory accessed, STOP !!
=== uc_exit_invalidate_iter, uc->invalid_error: 6 ===
=== tlb_index: mmu_idx: 2, addr: 7f5556c190ff ===
=== tlb_index: mmu_idx: 2, addr: 7f5556c190ff ===
=== tlb_index: mmu_idx: 2, addr: 7f5556c19000 ===
=== tlb_index: mmu_idx: 2, addr: 7f5556c19000 ===
=== tlb_index: mmu_idx: 2, addr: 7f5556c190ff ===
=== tlb_index: mmu_idx: 2, addr: 7f5556c190ff ===
```

Try logging `jmp`

```js
$$$$$$$$$$$ gen_goto_tb: pc=0x555556ca2202, tb_num=0
########## jump to another page eip=0x555556ca2202
    0x555556ca1ef0: push rbp
    0x555556ca1ef1: mov rbp, rsp
    0x555556ca1ef4: push 0x22
    0x555556ca1ef6: sub rsp, 8
    0x555556ca1efa: jmp 0x555556ca2202
$$$$$$$$$$$ gen_goto_tb: pc=0x555556b04740, tb_num=0
########## jump to another page eip=0x555556b04740
    0x555556ca2202: movsx rcx, byte ptr [r12 + r9 + 3]
    0x555556ca2208: movsx r9, byte ptr [r12 + r9 + 2]
    0x555556ca220e: mov rcx, qword ptr [rdx + rcx*8]
    0x555556ca2212: mov rdx, qword ptr [rdx + r9*8]
    0x555556ca2216: mov rbp, qword ptr [rbp]
    0x555556ca221a: add rsp, 0x18
    0x555556ca221e: push rdx
    0x555556ca221f: push qword ptr [rsp + 8]
    0x555556ca2223: mov eax, 2
    0x555556ca2228: mov rdi, rbx
    0x555556ca222b: mov qword ptr [rsp + 0x10], rcx
    0x555556ca2230: mov rsi, r8
    0x555556ca2233: jmp 0x555556b04740
$$$$$$$$$$$ gen_goto_tb: pc=0x555556b0474a, tb_num=0
######### jump to same page: we can use a direct jump eip=0x555556b0474a
$$$$$$$$$$$ gen_goto_tb: pc=0x555556b047b6, tb_num=1
######### jump to same page: we can use a direct jump eip=0x555556b047b6
$$$$$$$$$$$ gen_goto_tb: pc=0x555556b04765, tb_num=0
// ... 
######### jump to same page: we can use a direct jump eip=0x555555c47fd9
$$$$$$$$$$$ gen_goto_tb: pc=0x555555c47feb, tb_num=1
######### jump to same page: we can use a direct jump eip=0x555555c47feb
$$$$$$$$$$$ gen_goto_tb: pc=0x555555c48360, tb_num=0
########## jump to another page eip=0x555555c48360
$$$$$$$$$$$ gen_goto_tb: pc=0x555555c4838f, tb_num=0
######### jump to same page: we can use a direct jump eip=0x555555c4838f
$$$$$$$$$$$ gen_goto_tb: pc=0x555555c4840f, tb_num=1
######### jump to same page: we can use a direct jump eip=0x555555c4840f
>>> invalid memory accessed, STOP !!
=== uc_exit_invalidate_iter, uc->invalid_error: 6 ===
[2]    851779 abort (core dumped)  CONTEXT_DIR=UnicornContext_20240704_121555 ./harness -t 

```

Dump PC:

```js
##### disas_insn s->pc: 555555c4837a
##### disas_insn s->pc: 555555c48383
##### disas_insn s->pc: 555555c48387
##### disas_insn s->pc: 555555c48389
>>> invalid memory accessed, STOP !!
=== uc_exit_invalidate_iter, uc->invalid_error: 6 ===
[3]    854534 abort (core dumped)  CONTEXT_DIR=UnicornContext_20240704_121555 ./harness -t 
```

Flow opcodes:

```js
##### disas_insn s->pc: 0x555555c4837a
	 b: 0x64
	 b: 0x48
	 b: 0x8b
##### disas_insn s->pc: 0x555555c48383
	 b: 0x48
	 b: 0x89
##### disas_insn s->pc: 0x555555c48387
	 b: 0x85
##### disas_insn s->pc: 0x555555c48389
	 b: 0xf
	0xf -- b: 0x184
		b: 0x184, tval: 0x55c4840f, next_eip: 0x55c4838f
>>> invalid memory accessed, STOP !!
=== uc_exit_invalidate_iter, uc->invalid_error: 6 ===
[3]    855733 abort (core dumped)  CONTEXT_DIR=UnicornContext_20240704_121555 ./harness -t 
```

Reading QEMU code~~: https://github.com/qemu/qemu/blob/7914bda497f07965f15a91905cd7ed9eaf1c1092/target/i386/cpu-dump.c
x86_cpu: https://github.com/qemu/qemu/blob/7914bda497f07965f15a91905cd7ed9eaf1c1092/target/i386/cpu.c#L8044
Following unicorn qemu: https://github.com/unicorn-engine/unicorn/blob/master/qemu/tcg/tcg-op.c


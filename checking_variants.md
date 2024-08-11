
# Table of Contents



# How to run tests on V8

```bash

tools/dev/gm.py x64.release mjsunit/regress/regress-123

# If you have already built V8, you can run the tests manually:
./tools/run-tests.py --outdir=out/release mjsunit

```

## Variants

```js
// https://source.chromium.org/chromium/chromium/src/+/main:v8/src/builtins/js-to-js.tq;drc=9cb985225493804ee5ad1352ef89c6e414f1a909;l=144

transitioning javascript builtin JSToJSWrapper(
    js-implicit context: NativeContext, receiver: JSAny, target: JSFunction)(
    ...arguments): JSAny {
  const functionData = target.shared_function_info.wasm_js_function_data;
  // ...

  // The normal return sequence of Torque-generated JavaScript builtins does not
  // consider the case where the caller may push additional "undefined"
  // parameters on the stack, and therefore does not generate code to pop these
  // additional parameters. Here we calculate the actual number of parameters on
  // the stack. This number is the number of actual parameters provided by the
  // caller, which is `arguments.length`, or the number of declared arguments,
  // if not enough actual parameters were provided, i.e.
  // `SharedFunctionInfo::length`.
  let popCount = arguments.length;
  const declaredArgCount = paramCount;
  if (declaredArgCount > popCount) {
    popCount = declaredArgCount;
  }
  // Also pop the receiver.
  PopAndReturn(popCount + 1, result);
    }
```

Testing script:

```js
// ./out/release/d8 --test test/mjsunit/mjsunit.js ../sandbox_fuzzer/js_pop_count.js --experimental-wasm-type-reflection --sandbox-testing --allow-natives-syntax

let sandboxMemory = new DataView(new Sandbox.MemoryView(0, 0x100000000));

function addrOf(obj) {
    return Sandbox.getAddressOf(obj);
}

function v8_read64(addr) {
    return sandboxMemory.getBigUint64(Number(addr), true);
}

function v8_write64(addr, val) {
    return sandboxMemory.setBigInt64(Number(addr), val, true);
}

function add(i, j) {
return i + j;
}
const jsFunc =
    new WebAssembly.Function({parameters: ['f64'], results: ['i32']}, add);
// jsFunc();
jsFunc(1, 2, 3, 4);

%DebugPrint(jsFunc);
%SystemBreak();
v8_write64(addrOf(jsFunc)-0x30+0x18,0x4141n);


jsFunc(1, 5, 3, 4);
//========================

Instructions (size = 1096)
0x7fff60000c80     0  488d1df9ffffff       REX.W leaq rbx,[rip+0xfffffff9]
0x7fff60000c87     7  483bd9               REX.W cmpq rbx,rcx
0x7fff60000c8a     a  740d                 jz 0x7fff60000c99  <+0x19>
0x7fff60000c8c     c  ba84000000           movl rdx,0x84
0x7fff60000c91    11  41ff95d8550000       call [r13+0x55d8]
0x7fff60000c98    18  cc                   int3l
0x7fff60000c99    19  8b59f4               movl rbx,[rcx-0xc]
0x7fff60000c9c    1c  490b9de0010000       REX.W orq rbx,[r13+0x1e0]
0x7fff60000ca3    23  f6431a20             testb [rbx+0x1a],0x20
0x7fff60000ca7    27  0f8593844c1f         jnz 0x7fff7f4c9140  (CompileLazyDeoptimizedCode)    ;; near builtin entry
0x7fff60000cad    2d  55                   push rbp
0x7fff60000cae    2e  4889e5               REX.W movq rbp,rsp
0x7fff60000cb1    31  56                   push rsi
0x7fff60000cb2    32  57                   push rdi
0x7fff60000cb3    33  50                   push rax
0x7fff60000cb4    34  ba58000000           movl rdx,0x58
0x7fff60000cb9    39  41ff95d8550000       call [r13+0x55d8]
0x7fff60000cc0    40  cc                   int3l
0x7fff60000cc1    41  4883ec18             REX.W subq rsp,0x18
0x7fff60000cc5    45  48897590             REX.W movq [rbp-0x70],rsi
0x7fff60000cc9    49  493b65a0             REX.W cmpq rsp,[r13-0x60] (external value (StackGuard::address_of_jslimit()))
0x7fff60000ccd    4d  0f8623030000         jna 0x7fff60000ff6  <+0x376>
0x7fff60000cd3    53  488b4dc0             REX.W movq rcx,[rbp-0x40]
0x7fff60000cd7    57  f6c101               testb rcx,0x1
0x7fff60000cda    5a  0f858d030000         jnz 0x7fff6000106d  <+0x3ed>
0x7fff60000ce0    60  81f900000200         cmpl rcx,0x20000
0x7fff60000ce6    66  0f8db1020000         jge 0x7fff60000f9d  <+0x31d>
0x7fff60000cec    6c  488b7590             REX.W movq rsi,[rbp-0x70]
0x7fff60000cf0    70  48bf0de0290039210000 REX.W movq rdi,0x21390029e00d    ;; object: 0x21390029e00d <JSFunction add (sfi = 0x21390029dfdd)>
0x7fff60000cfa    7a  397e17               cmpl [rsi+0x17],rdi
0x7fff60000cfd    7d  0f856e030000         jnz 0x7fff60001071  <+0x3f1>
0x7fff60000d03    83  448b4717             movl r8,[rdi+0x17]
0x7fff60000d07    87  4d03c6               REX.W addq r8,r14
0x7fff60000d0a    8a  6a08                 push 0x8
0x7fff60000d0c    8c  6a06                 push 0x6
0x7fff60000d0e    8e  6a04                 push 0x4
0x7fff60000d10    90  6a02                 push 0x2
0x7fff60000d12    92  49b9251a280039210000 REX.W movq r9,0x213900281a25    ;; object: 0x213900281a25 <JSGlobalProxy>
0x7fff60000d1c    9c  4151                 push r9
0x7fff60000d1e    9e  498d5669             REX.W leaq rdx,[r14+0x69]
0x7fff60000d22    a2  b805000000           movl rax,0x5
0x7fff60000d27    a7  498bf0               REX.W movq rsi,r8
0x7fff60000d2a    aa  3b7717               cmpl rsi,[rdi+0x17]
0x7fff60000d2d    ad  740d                 jz 0x7fff60000d3c  <+0xbc>
0x7fff60000d2f    af  ba86000000           movl rdx,0x86
0x7fff60000d34    b4  41ff95d8550000       call [r13+0x55d8]
0x7fff60000d3b    bb  cc                   int3l
0x7fff60000d3c    bc  49ba0000008cff7f0000 REX.W movq r10,0x7fff8c000000    ;; external reference (GetProcessWideCodePointerTable())
0x7fff60000d46    c6  8b4f0b               movl rcx,[rdi+0xb]
0x7fff60000d49    c9  c1e909               shrl rcx, 9
0x7fff60000d4c    cc  c1e104               shll rcx, 4
0x7fff60000d4f    cf  498b0c0a             REX.W movq rcx,[r10+rcx*1]
0x7fff60000d53    d3  ffd1                 call rcx
0x7fff60000d55    d5  488b4dc0             REX.W movq rcx,[rbp-0x40]
0x7fff60000d59    d9  d1f9                 sarl rcx, 1
0x7fff60000d5b    db  83c101               addl rcx,0x1
0x7fff60000d5e    de  0f8011030000         jo 0x7fff60001075  <+0x3f5>
0x7fff60000d64    e4  41807db100           cmpb [r13-0x4f] (external value (StackGuard::address_of_interrupt_request(StackGuard::InterruptLevel::kNoH
0x7fff60000d69    e9  0f85b0020000         jnz 0x7fff6000101f  <+0x39f>
0x7fff60000d6f    ef  660f1f840000000000   nop
0x7fff60000d78    f8  0f1f840000000000     nop
0x7fff60000d80   100  48894d80             REX.W movq [rbp-0x80],rcx
0x7fff60000d84   104  488b7590             REX.W movq rsi,[rbp-0x70]
0x7fff60000d88   108  48bf0de0290039210000 REX.W movq rdi,0x21390029e00d    ;; object: 0x21390029e00d <JSFunction add (sfi = 0x21390029dfdd)>
0x7fff60000d92   112  49b8251a280039210000 REX.W movq r8,0x213900281a25    ;; object: 0x213900281a25 <JSGlobalProxy>
0x7fff60000d9c   11c  817d8000000100       cmpl [rbp-0x80],0x10000
0x7fff60000da3   123  0f8df4010000         jge 0x7fff60000f9d  <+0x31d>
0x7fff60000da9   129  397e17               cmpl [rsi+0x17],rdi
0x7fff60000dac   12c  0f85c7020000         jnz 0x7fff60001079  <+0x3f9>
0x7fff60000db2   132  448b4f17             movl r9,[rdi+0x17]
0x7fff60000db6   136  4d03ce               REX.W addq r9,r14
0x7fff60000db9   139  6a08                 push 0x8
0x7fff60000dbb   13b  6a06                 push 0x6
0x7fff60000dbd   13d  6a04                 push 0x4
0x7fff60000dbf   13f  6a02                 push 0x2
0x7fff60000dc1   141  4150                 push r8
0x7fff60000dc3   143  498bf1               REX.W movq rsi,r9
0x7fff60000dc6   146  498d5669             REX.W leaq rdx,[r14+0x69]
0x7fff60000dca   14a  b805000000           movl rax,0x5
0x7fff60000dcf   14f  3b7717               cmpl rsi,[rdi+0x17]
0x7fff60000dd2   152  740d                 jz 0x7fff60000de1  <+0x161>
0x7fff60000dd4   154  ba86000000           movl rdx,0x86
0x7fff60000dd9   159  41ff95d8550000       call [r13+0x55d8]
0x7fff60000de0   160  cc                   int3l
0x7fff60000de1   161  4c8b1556ffffff       REX.W movq r10,[rip+0xffffff56]
0x7fff60000de8   168  8b4f0b               movl rcx,[rdi+0xb]
0x7fff60000deb   16b  c1e909               shrl rcx, 9
0x7fff60000dee   16e  c1e104               shll rcx, 4
0x7fff60000df1   171  498b0c0a             REX.W movq rcx,[r10+rcx*1]
0x7fff60000df5   175  ffd1                 call rcx
0x7fff60000df7   177  488b4d80             REX.W movq rcx,[rbp-0x80]
0x7fff60000dfb   17b  83c101               addl rcx,0x1
0x7fff60000dfe   17e  0f8079020000         jo 0x7fff6000107d  <+0x3fd>
0x7fff60000e04   184  48894d88             REX.W movq [rbp-0x78],rcx
0x7fff60000e08   188  81f900000100         cmpl rcx,0x10000
0x7fff60000e0e   18e  0f8d89010000         jge 0x7fff60000f9d  <+0x31d>
0x7fff60000e14   194  488b7590             REX.W movq rsi,[rbp-0x70]
0x7fff60000e18   198  48bf0de0290039210000 REX.W movq rdi,0x21390029e00d    ;; object: 0x21390029e00d <JSFunction add (sfi = 0x21390029dfdd)>
0x7fff60000e22   1a2  397e17               cmpl [rsi+0x17],rdi
0x7fff60000e25   1a5  0f8556020000         jnz 0x7fff60001081  <+0x401>
0x7fff60000e2b   1ab  448b4717             movl r8,[rdi+0x17]
0x7fff60000e2f   1af  4d03c6               REX.W addq r8,r14
0x7fff60000e32   1b2  6a08                 push 0x8
0x7fff60000e34   1b4  6a06                 push 0x6
0x7fff60000e36   1b6  6a04                 push 0x4
0x7fff60000e38   1b8  6a02                 push 0x2
0x7fff60000e3a   1ba  49b9251a280039210000 REX.W movq r9,0x213900281a25    ;; object: 0x213900281a25 <JSGlobalProxy>
0x7fff60000e44   1c4  4151                 push r9
0x7fff60000e46   1c6  498bf0               REX.W movq rsi,r8
0x7fff60000e49   1c9  498d5669             REX.W leaq rdx,[r14+0x69]
0x7fff60000e4d   1cd  b805000000           movl rax,0x5
0x7fff60000e52   1d2  3b7717               cmpl rsi,[rdi+0x17]
0x7fff60000e55   1d5  740d                 jz 0x7fff60000e64  <+0x1e4>
0x7fff60000e57   1d7  ba86000000           movl rdx,0x86
0x7fff60000e5c   1dc  41ff95d8550000       call [r13+0x55d8]
0x7fff60000e63   1e3  cc                   int3l
0x7fff60000e64   1e4  4c8b15d3feffff       REX.W movq r10,[rip+0xfffffed3]
0x7fff60000e6b   1eb  8b4f0b               movl rcx,[rdi+0xb]
0x7fff60000e6e   1ee  c1e909               shrl rcx, 9
0x7fff60000e71   1f1  c1e104               shll rcx, 4
0x7fff60000e74   1f4  498b0c0a             REX.W movq rcx,[r10+rcx*1]
0x7fff60000e78   1f8  ffd1                 call rcx
0x7fff60000e7a   1fa  488b4d88             REX.W movq rcx,[rbp-0x78]
0x7fff60000e7e   1fe  83c101               addl rcx,0x1
0x7fff60000e81   201  0f80fe010000         jo 0x7fff60001085  <+0x405>
0x7fff60000e87   207  48894d80             REX.W movq [rbp-0x80],rcx
0x7fff60000e8b   20b  81f900000100         cmpl rcx,0x10000
0x7fff60000e91   211  0f8d06010000         jge 0x7fff60000f9d  <+0x31d>
0x7fff60000e97   217  488b7590             REX.W movq rsi,[rbp-0x70]
0x7fff60000e9b   21b  48bf0de0290039210000 REX.W movq rdi,0x21390029e00d    ;; object: 0x21390029e00d <JSFunction add (sfi = 0x21390029dfdd)>
0x7fff60000ea5   225  397e17               cmpl [rsi+0x17],rdi
0x7fff60000ea8   228  0f85db010000         jnz 0x7fff60001089  <+0x409>
0x7fff60000eae   22e  448b4717             movl r8,[rdi+0x17]
0x7fff60000eb2   232  4d03c6               REX.W addq r8,r14
0x7fff60000eb5   235  6a08                 push 0x8
0x7fff60000eb7   237  6a06                 push 0x6
0x7fff60000eb9   239  6a04                 push 0x4
0x7fff60000ebb   23b  6a02                 push 0x2
0x7fff60000ebd   23d  49b9251a280039210000 REX.W movq r9,0x213900281a25    ;; object: 0x213900281a25 <JSGlobalProxy>
0x7fff60000ec7   247  4151                 push r9
0x7fff60000ec9   249  498bf0               REX.W movq rsi,r8
0x7fff60000ecc   24c  498d5669             REX.W leaq rdx,[r14+0x69]
0x7fff60000ed0   250  b805000000           movl rax,0x5
0x7fff60000ed5   255  3b7717               cmpl rsi,[rdi+0x17]
0x7fff60000ed8   258  740d                 jz 0x7fff60000ee7  <+0x267>
0x7fff60000eda   25a  ba86000000           movl rdx,0x86
0x7fff60000edf   25f  41ff95d8550000       call [r13+0x55d8]
0x7fff60000ee6   266  cc                   int3l
0x7fff60000ee7   267  4c8b1550feffff       REX.W movq r10,[rip+0xfffffe50]
0x7fff60000eee   26e  8b4f0b               movl rcx,[rdi+0xb]
0x7fff60000ef1   271  c1e909               shrl rcx, 9
0x7fff60000ef4   274  c1e104               shll rcx, 4
0x7fff60000ef7   277  498b0c0a             REX.W movq rcx,[r10+rcx*1]
0x7fff60000efb   27b  ffd1                 call rcx
0x7fff60000efd   27d  488b4d80             REX.W movq rcx,[rbp-0x80]
0x7fff60000f01   281  83c101               addl rcx,0x1
0x7fff60000f04   284  0f8083010000         jo 0x7fff6000108d  <+0x40d>
0x7fff60000f0a   28a  48894d88             REX.W movq [rbp-0x78],rcx
0x7fff60000f0e   28e  81f900000100         cmpl rcx,0x10000
0x7fff60000f14   294  0f8d83000000         jge 0x7fff60000f9d  <+0x31d>
0x7fff60000f1a   29a  488b7590             REX.W movq rsi,[rbp-0x70]
0x7fff60000f1e   29e  48bf0de0290039210000 REX.W movq rdi,0x21390029e00d    ;; object: 0x21390029e00d <JSFunction add (sfi = 0x21390029dfdd)>
0x7fff60000f28   2a8  397e17               cmpl [rsi+0x17],rdi
0x7fff60000f2b   2ab  0f8560010000         jnz 0x7fff60001091  <+0x411>
0x7fff60000f31   2b1  448b4717             movl r8,[rdi+0x17]
0x7fff60000f35   2b5  4d03c6               REX.W addq r8,r14
0x7fff60000f38   2b8  6a08                 push 0x8
0x7fff60000f3a   2ba  6a06                 push 0x6
0x7fff60000f3c   2bc  6a04                 push 0x4
0x7fff60000f3e   2be  6a02                 push 0x2
0x7fff60000f40   2c0  49b9251a280039210000 REX.W movq r9,0x213900281a25    ;; object: 0x213900281a25 <JSGlobalProxy>
0x7fff60000f4a   2ca  4151                 push r9
0x7fff60000f4c   2cc  498bf0               REX.W movq rsi,r8
0x7fff60000f4f   2cf  498d5669             REX.W leaq rdx,[r14+0x69]
0x7fff60000f53   2d3  b805000000           movl rax,0x5
0x7fff60000f58   2d8  3b7717               cmpl rsi,[rdi+0x17]
0x7fff60000f5b   2db  740d                 jz 0x7fff60000f6a  <+0x2ea>
0x7fff60000f5d   2dd  ba86000000           movl rdx,0x86
0x7fff60000f62   2e2  41ff95d8550000       call [r13+0x55d8]
0x7fff60000f69   2e9  cc                   int3l
0x7fff60000f6a   2ea  4c8b15cdfdffff       REX.W movq r10,[rip+0xfffffdcd]
0x7fff60000f71   2f1  8b4f0b               movl rcx,[rdi+0xb]
0x7fff60000f74   2f4  c1e909               shrl rcx, 9
0x7fff60000f77   2f7  c1e104               shll rcx, 4
0x7fff60000f7a   2fa  498b0c0a             REX.W movq rcx,[r10+rcx*1]
0x7fff60000f7e   2fe  ffd1                 call rcx
0x7fff60000f80   300  488b4d88             REX.W movq rcx,[rbp-0x78]
0x7fff60000f84   304  83c101               addl rcx,0x1
0x7fff60000f87   307  0f8008010000         jo 0x7fff60001095  <+0x415>
0x7fff60000f8d   30d  41807db100           cmpb [r13-0x4f] (external value (StackGuard::address_of_interrupt_request(StackGuard::InterruptLevel::kNoH
0x7fff60000f92   312  0f85af000000         jnz 0x7fff60001047  <+0x3c7>
0x7fff60000f98   318  e9e3fdffff           jmp 0x7fff60000d80  <+0x100>
0x7fff60000f9d   31d  488b7590             REX.W movq rsi,[rbp-0x70]
0x7fff60000fa1   321  8b4e17               movl rcx,[rsi+0x17]
0x7fff60000fa4   324  4903ce               REX.W addq rcx,r14
0x7fff60000fa7   327  51                   push rcx
0x7fff60000fa8   328  48bb103037f5ff7f0000 REX.W movq rbx,0x7ffff5373010    ;; external reference (Runtime::DebugPrint)
0x7fff60000fb2   332  b801000000           movl rax,0x1
0x7fff60000fb7   337  e8841f851f           call 0x7fff7f852f40  (CEntry_Return1_ArgvOnStack_NoBuiltinExit)    ;; near builtin entry
0x7fff60000fbc   33c  48bb705637f5ff7f0000 REX.W movq rbx,0x7ffff5375670    ;; external reference (Runtime::SystemBreak)
0x7fff60000fc6   346  33c0                 xorl rax,rax
0x7fff60000fc8   348  488b7590             REX.W movq rsi,[rbp-0x70]
0x7fff60000fcc   34c  e86f1f851f           call 0x7fff7f852f40  (CEntry_Return1_ArgvOnStack_NoBuiltinExit)    ;; near builtin entry
0x7fff60000fd1   351  488b4d90             REX.W movq rcx,[rbp-0x70]
0x7fff60000fd5   355  448b4117             movl r8,[rcx+0x17]
0x7fff60000fd9   359  41baffffffff         movl r10,0xffffffff
0x7fff60000fdf   35f  4d3bc2               REX.W cmpq r8,r10
0x7fff60000fe2   362  760d                 jna 0x7fff60000ff1  <+0x371>
0x7fff60000fe4   364  ba02000000           movl rdx,0x2
0x7fff60000fe9   369  41ff95d8550000       call [r13+0x55d8]
0x7fff60000ff0   370  cc                   int3l
0x7fff60000ff1   371  e9a3000000           jmp 0x7fff60001099  <+0x419>
0x7fff60000ff6   376  b950000000           movl rcx,0x50
0x7fff60000ffb   37b  51                   push rcx
0x7fff60000ffc   37c  b801000000           movl rax,0x1
0x7fff60001001   381  48bb70c02ff5ff7f0000 REX.W movq rbx,0x7ffff52fc070    ;; external reference (Runtime::StackGuardWithGap)
0x7fff6000100b   38b  48be851a280039210000 REX.W movq rsi,0x213900281a85    ;; object: 0x213900281a85 <NativeContext[297]>
0x7fff60001015   395  e8261f851f           call 0x7fff7f852f40  (CEntry_Return1_ArgvOnStack_NoBuiltinExit)    ;; near builtin entry
0x7fff6000101a   39a  e9b4fcffff           jmp 0x7fff60000cd3  <+0x53>
0x7fff6000101f   39f  33c0                 xorl rax,rax
0x7fff60001021   3a1  48bbc0bb2ff5ff7f0000 REX.W movq rbx,0x7ffff52fbbc0    ;; external reference (Runtime::HandleNoHeapWritesInterrupts)
0x7fff6000102b   3ab  48be851a280039210000 REX.W movq rsi,0x213900281a85    ;; object: 0x213900281a85 <NativeContext[297]>
0x7fff60001035   3b5  48894d80             REX.W movq [rbp-0x80],rcx
0x7fff60001039   3b9  e8021f851f           call 0x7fff7f852f40  (CEntry_Return1_ArgvOnStack_NoBuiltinExit)    ;; near builtin entry
0x7fff6000103e   3be  488b4d80             REX.W movq rcx,[rbp-0x80]
0x7fff60001042   3c2  e939fdffff           jmp 0x7fff60000d80  <+0x100>
0x7fff60001047   3c7  48894d80             REX.W movq [rbp-0x80],rcx
0x7fff6000104b   3cb  488b1dd1ffffff       REX.W movq rbx,[rip+0xffffffd1]
0x7fff60001052   3d2  33c0                 xorl rax,rax
0x7fff60001054   3d4  48be851a280039210000 REX.W movq rsi,0x213900281a85    ;; object: 0x213900281a85 <NativeContext[297]>
0x7fff6000105e   3de  e8dd1e851f           call 0x7fff7f852f40  (CEntry_Return1_ArgvOnStack_NoBuiltinExit)    ;; near builtin entry
0x7fff60001063   3e3  488b4d80             REX.W movq rcx,[rbp-0x80]
0x7fff60001067   3e7  e914fdffff           jmp 0x7fff60000d80  <+0x100>
0x7fff6000106c   3ec  90                   nop
0x7fff6000106d   3ed  41ff55d0             call [r13-0x30]


```
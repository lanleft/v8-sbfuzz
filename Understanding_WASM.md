
# Table of Contents



# How to run tests on V8

```bash

tools/dev/gm.py x64.release mjsunit/regress/regress-123

# If you have already built V8, you can run the tests manually:
./tools/run-tests.py --outdir=out/release mjsunit

```

# WebAssembly compilation pipeline

- https://v8.dev/docs/wasm-compilation-pipeline

## Liftoff

- Liftoff is a baseline compiler that compiles WebAssembly lazily, meaning only when they are called for the first time. It is a one-pass compiler that generates machine code quickly but with limited optimizations. Once a function is compiled by Liftoff, the machine code is registered with the WebAssembly module for immedicate future use.


## TurboFan

- TurboFan is an optimizing compiler used for functions that are called frequently, known as "hot" functions. It is a multi-pass compiler that allows for more complex optimizations and better register allocations, resulting in faster code. When a function becomes hot, it is re-compiled with TurboFan in the background, and the new optimized code replaces the Liftoff code.

![liftoff](image-2.png)



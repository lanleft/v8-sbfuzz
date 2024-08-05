
// r --expose-gc --allow-natives-syntax --sandbox-testing --experimental-wasm-memory64 ~/v8_sb_fuzz/sandbox_fuzzer/stack_oob.js

// d8.file.execute('/home/vult/v8_sb_fuzz/v8/test/mjsunit/wasm/wasm-module-builder.js');
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

// ================= reading heap_base =============================
let heap_addr = BigInt(Sandbox.base);
console.log("heap_addr: 0x" + heap_addr.toString(16));
let target_page = BigInt(Sandbox.targetPage);
console.log("target_page: 0x" + target_page.toString(16));
// ================================================================

const rab = new ArrayBuffer(16, {maxByteLength: 1024});
const u16arr = new Uint16Array(rab);

function foo(obj,index, val) {
    obj[index] += val;
    return obj[index];

}

function test(iii,val) {
    return foo(u16arr, iii, val);
}

test(1,0);

// %DebugPrint(test);
// %SystemBreak();


test(2,0);

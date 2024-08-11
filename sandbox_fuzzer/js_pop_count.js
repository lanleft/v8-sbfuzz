// /home/vult/v8_sb_fuzz/v8/out/debug/d8 --test /home/vult/v8_sb_fuzz/v8/test/mjsunit/mjsunit.js /home/vult/v8_sb_fuzz/sandbox_fuzzer/js_pop_count.js --experimental-wasm-type-reflection --sandbox-testing --allow-natives-syntax --print-code


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

// ====================================

function add(i, j) {
return i + j + 0x13371337;
}
const jsFunc =
    new WebAssembly.Function({parameters: ['f64'], results: ['i32']}, add);
jsFunc(1, 2);

%DebugPrint(jsFunc);
%SystemBreak();
// v8_write64(addrOf(jsFunc)-0x30+0x18,0x4141n);

jsFunc(1, 5, 3, 4);

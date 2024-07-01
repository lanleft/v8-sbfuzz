import struct

def read(name):
    with open(name,'rb') as f:
        return f.read()

def u32(data):
    return struct.unpack("I", data)[0]

def p32(num):
    return struct.pack("I", num)
    
import argparse

from unicorn import *
from unicorn.x86_const import *  # TODO: Set correct architecture here as necessary
import capstone

import unicorn_loader 

# Simple stand-in heap to prevent OS/kernel issues
unicorn_heap = None

# Start and end address of emulation
START_ADDRESS = 0x555556c19100 # <Builtins_MathLog>       push   rbp
END_ADDRESS   = 0x7fffffffd628 # 


# Entry points of addresses of functions to hook
MALLOC_ENTRY        = 0x08049C40
FREE_ENTRY          = 0x08049980
PRINTF_ENTRY        = 0x0804AA60
CGC_TRANSMIT_ENTRY  = 0x0804A4C2
CGC_TRANSMIT_PASSED = 0x0804A4DC

"""
    Implement target-specific hooks in here.
    Stub out, skip past, and re-implement necessary functionality as appropriate
"""
def unicorn_hook_instruction(uc, address, size, user_data):
    
    # Bypass the CGC_TRANSMIT_ENTRY check
    if address == CGC_TRANSMIT_ENTRY:
        print("--- Bypassing CGC_TRANSMIT_ENTRY validation @ 0x{0:08x} ---".format(address))
        uc.reg_write(UC_X86_REG_EIP, CGC_TRANSMIT_PASSED)
    
    elif address == START_ADDRESS:
        print('>>> Tracing instruction at 0x%x, instruction size = 0x%x' %(address, size))
        print(mu.mem_read(address,size))


    # TODO: Setup hooks and handle anything you need to here
    #    - For example, hook malloc/free/etc. and handle it internally
    pass

#------------------------
#---- Main test function  

def main():

    parser = argparse.ArgumentParser()
    parser.add_argument('context_dir', type=str, help="Directory containing process context")
    parser.add_argument('input_file', type=str, help="Path to the file containing the mutated input content")
    parser.add_argument('-d', '--debug', default=False, action="store_true", help="Dump trace info")
    args = parser.parse_args()

    print("Loading context from {}".format(args.context_dir))
    uc = unicorn_loader.AflUnicornEngine(args.context_dir, enable_trace=args.debug, debug_print=False)       

    # Instantiate the hook function to avoid emulation errors
    global unicorn_heap
    unicorn_heap = unicorn_loader.UnicornSimpleHeap(uc, debug_print=True)
    uc.hook_add(UC_HOOK_CODE, unicorn_hook_instruction)

    # Execute 1 instruction just to startup the forkserver
    # NOTE: This instruction will be executed again later, so be sure that
    #       there are no negative consequences to the overall execution state.
    #       If there are, change the later call to emu_start to no re-execute 
    #       the first instruction.
    print("Starting the forkserver by executing 1 instruction")
    try:
        uc.emu_start(START_ADDRESS, 0, 0, count=1)
    except UcError as e:
        print("ERROR: Failed to execute a single instruction (error: {})!".format(e))
        return

    # Allocate a buffer and load a mutated input and put it into the right spot
    if args.input_file:
        print("Loading input content from {}".format(args.input_file))
        input_file = open(args.input_file, 'rb')
        input_content = input_file.read()
        input_file.close()

        # # TODO: Apply constraints to mutated input here
        # if len(input_content) > 0xFF:
        #     return
        # raise exceptions.NotImplementedError('No constraints on the mutated inputs have been set!')
        
        # Allocate a new buffer and put the input into it
        # buf_addr = unicorn_heap.malloc(len(input_content))
        # uc.mem_write(buf_addr, input_content)
        # print("Allocated mutated input buffer @ 0x{0:016x}".format(buf_addr))

        # TODO: Set the input into the state so it will be handled
        #raise exceptions.NotImplementedError('The mutated input was not loaded into the Unicorn state!')
        uc.reg_write(UC_X86_REG_EAX, buf_addr)
        uc.reg_write(UC_X86_REG_DL, len(input_content))
        
    # Run the test
    print("Executing from 0x{0:016x} to 0x{1:016x}".format(START_ADDRESS, END_ADDRESS))
    try:
        result = uc.emu_start(START_ADDRESS, END_ADDRESS, timeout=0, count=0)
    except UcError as e:
        # If something went wrong during emulation a signal is raised to force this 
        # script to crash in a way that AFL can detect ('uc.force_crash()' should be
        # called for any condition that you want AFL to treat as a crash).
        print("Execution failed with error: {}".format(e))
        uc.dump_regs() 
        uc.force_crash(e)

    print("Final register state:")    
    uc.dump_regs()

    print("Done.")    
        
if __name__ == "__main__":
    main()
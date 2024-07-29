from pwn import *


pl = b'A'*8
pl += p64(0x00000002E0000000)
pl += b'E'*(0xf-4)
pl += b'C'*(0x18-0xf-4)
pl += p64(0x4141414141414141)
pl += p64(0x00000002E0000000)
pl += p64(0x4141414141414141)


with open("fuzz_in2/a", "wb") as f:
    f.write(pl)
    
print("Done")

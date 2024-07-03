CC = afl-clang-fast++
LD = ld
CFLAGS  = -g -W -std=c++17
CFLAGS += -L ./resources/unicorn/build -I ./include -L../AFLplusplus/utils/aflpp_driver
CFLAGS += -DLLVM_FUZZER -m64 -MJ -fsanitize=fuzzer,address 

LDLIBS  = -pthread
LDLIBS += -lunicorn -lz  -lrt
TARGET = harness
OBJS = harness3.o AflUnicornEngine.o UnicornSimpleHeap.o

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o harness $(OBJS) $(LDLIBS)

harness3.o : harness3.cpp
	$(CC) $(CFLAGS) $(LDLIBS) -c harness3.cpp

AflUnicornEngine.o : AflUnicornEngine.cpp
	$(CC) $(CFLAGS) $(LDLIBS) -c AflUnicornEngine.cpp

UnicornSimpleHeap.o : UnicornSimpleHeap.cpp
	$(CC) $(CFLAGS) $(LDLIBS) -c UnicornSimpleHeap.cpp
    
clean :
	rm *.o harness
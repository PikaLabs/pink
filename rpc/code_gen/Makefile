CC := g++
PINK_DIR := $(abspath $(shell pwd)/../../)
CC_FLAGS := -std=c++11 -g -D__PINK_DIR__=$(PINK_DIR)
LD_FLAGS := 
SRC_DIR := ./

INCLUDE_PATH := -I../../deps/protobuf/include/ \
								-I/usr/local/include/ \
								-I./

LIBRARY_PATH := -L../../deps/protobuf/lib/

LIBRARIES := -Wl,-Bstatic -lprotobuf  -Wl,-Bdynamic

SOURCES := $(wildcard ./*.cc) 
OBJS := $(patsubst %.cc, %.o, $(SOURCES)) 

BIN := code_gen
.PHONY: all clean

all: $(BIN)
	@echo "build success"

# CODE_GEN_SOURCE := code_gen.cc code_yield.cc rpc_common.cc 
code_gen: $(OBJS)
	$(CC) $(LD_FLAGS) $^ -o $@ $(LIBRARY_PATH) $(LIBRARIES)	

$(OBJS): %.o : %.cc
	$(CC) $(CC_FLAGS) $(INCLUDE_PATH) -c $< -o $@

clean:
	rm *.o $(BIN) -f


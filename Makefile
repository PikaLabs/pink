CXX = g++
ifeq ($(__PERF), 1)
	CXXFLAGS = -O0 -g -pg -pipe -fPIC -D__XDEBUG__ -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -Wall -D_GNU_SOURCE -std=c++11 -D__STDC_FORMAT_MACROS -Wno-redundant-decls
else
	CXXFLAGS = -O2 -g -pipe -fPIC -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -Wall -Wno-unused-parameter -D_GNU_SOURCE -D__STDC_FORMAT_MACROS -std=c++11 -gdwarf-2 -Wno-redundant-decls -Wno-maybe-uninitialized
	# CXXFLAGS = -Wall -W -DDEBUG -g -O0 -D__XDEBUG__ -D__STDC_FORMAT_MACROS -fPIC -std=c++11 -gdwarf-2
endif

OBJECT = pink
SRC_DIR = ./src
THIRD_PATH = ./third/
OUTPUT = ./output

DEPS_DIR = ./deps
PROTOBUF_DIR = deps/protobuf
PROTOBUF_LIB = $(PROTOBUF_DIR)/lib/libprotobuf.a

RPC_DIR = ./rpc

INCLUDE_PATH = -I./ \
							 -I./include/ \
							 -I./src/ \
							 -I./deps/protobuf/include/

LIB_PATH = -L./deps/protobuf/lib/ \


LIBS = -lpthread \
			 -lprotobuf

LIBRARY = libpink.a


.PHONY: all clean rpc


BASE_OBJS := $(wildcard $(SRC_DIR)/*.cc)
BASE_OBJS += $(wildcard $(SRC_DIR)/*.c)
BASE_OBJS += $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst %.cc,%.o,$(BASE_OBJS))


all: $(LIBRARY)
	@echo "Success, go, go, go..."

rpc:
	make -C $(RPC_DIR)/code_gen
	mkdir -p output/bin/
	cp $(RPC_DIR)/code_gen/code_gen output/bin/


$(LIBRARY): $(PROTOBUF_LIB) $(OBJS)
	rm -rf $(OUTPUT)
	mkdir $(OUTPUT)
	mkdir $(OUTPUT)/include
	mkdir $(OUTPUT)/lib
	rm -rf $@
	ar -rcs $@ $(OBJS)
	cp -r ./include $(OUTPUT)/
	mv $@ $(OUTPUT)/lib/
	protoc -I=./ --cpp_out=./example/ ./pink.proto
	# make -C example __PERF=$(__PERF)


$(PROTOBUF_LIB):
	if test ! -e $(PROTOBUF_DIR); then \
		rm -rf $(PROTOBUF_DIR) $(DEPS_DIR)/protobuf.tar.gz; \
		mkdir -p $(DEPS_DIR); \
		wget https://github.com/google/protobuf/releases/download/v3.0.0/protobuf-cpp-3.0.0.tar.gz -O $(DEPS_DIR)/protobuf.tar.gz; \
		tar -xf $(DEPS_DIR)/protobuf.tar.gz -C $(DEPS_DIR); \
		mv $(DEPS_DIR)/protobuf-3.0.0 $(PROTOBUF_DIR); \
		rm -rf $(DEPS_DIR)/protobuf.tar.gz; \
	fi
	if test ! -e $(PROTOBUF_DIR)/Makefile; then \
		cd $(PROTOBUF_DIR) && ./configure --prefix=`pwd` && cd ../../; \
	fi
	make -C $(PROTOBUF_DIR)
	make -C $(PROTOBUF_DIR) install
	@echo "Build libprotobuf.a success"

$(OBJECT): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDE_PATH) $(LIB_PATH) -Wl,-Bdynamic $(LIBS)

$(OBJS): %.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE_PATH) 

clean: 
	make clean -C example
	make clean -C $(RPC_DIR)/code_gen
	rm -rf $(SRC_DIR)/*.o
	rm -rf $(OUTPUT)/*
	rm -rf $(OUTPUT)

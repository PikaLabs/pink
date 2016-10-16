CXX = g++
ifeq ($(__PERF), 1)
	CXXFLAGS = -O0 -g -gstabs+ -pg -pipe -fPIC -D__XDEBUG__ -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -Wall -D_GNU_SOURCE -std=c++11 -D__STDC_FORMAT_MACROS -Wno-redundant-decls
else
	CXXFLAGS = -O2 -g -pipe -fPIC -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -Wall -Wno-unused-parameter -D_GNU_SOURCE -D__STDC_FORMAT_MACROS -std=c++11 -gdwarf-2 -Wno-redundant-decls -Wno-maybe-uninitialized
	# CXXFLAGS = -Wall -W -DDEBUG -g -O0 -D__XDEBUG__ -D__STDC_FORMAT_MACROS -fPIC -std=c++11 -gdwarf-2
endif

OBJECT = pink
SRC_DIR = ./src
THIRD_PATH = ./third/
OUTPUT = ./output


INCLUDE_PATH = -I./ \
							 -I./include/ \
							 -I./src/ \

LIB_PATH = -L./ \


LIBS = -lpthread \
			 -lprotobuf

LIBRARY = libpink.a


.PHONY: all clean


BASE_OBJS := $(wildcard $(SRC_DIR)/*.cc)
BASE_OBJS += $(wildcard $(SRC_DIR)/*.c)
BASE_OBJS += $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst %.cc,%.o,$(BASE_OBJS))


all: $(LIBRARY)
	@echo "Success, go, go, go..."

$(LIBRARY): $(OBJS)
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


$(OBJECT): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDE_PATH) $(LIB_PATH) -Wl,-Bdynamic $(LIBS)

$(OBJS): %.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE_PATH) 

clean: 
	make clean -C example
	rm -rf $(SRC_DIR)/*.o
	rm -rf $(OUTPUT)/*
	rm -rf $(OUTPUT)

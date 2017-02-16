CXX = g++
ifeq ($(__PERF), 1)
	CXXFLAGS = -O0 -g -gstabs+ -pg -pipe -fPIC -D__XDEBUG__ -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -Wall -D_GNU_SOURCE -std=c++11 -D__STDC_FORMAT_MACROS -Wno-redundant-decls
else
	CXXFLAGS = -O2 -g -pipe -fPIC -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -Wall -Wno-unused-parameter -D_GNU_SOURCE -D__STDC_FORMAT_MACROS -std=c++11 -gdwarf-2 -Wno-redundant-decls -Wno-maybe-uninitialized
	# CXXFLAGS = -Wall -W -DDEBUG -g -O0 -D__XDEBUG__ -D__STDC_FORMAT_MACROS -fPIC -std=c++11 -gdwarf-2
endif

OBJECT = pink
SRC_DIR = ./src
THIRD_PATH = ./third
OUTPUT = ./output


INCLUDE_PATH = -I./ \
							 -I./include/ \
							 -I./src/ \
							 -I$(THIRD_PATH)/slash/include

LIB_PATH = -L./ \
					 -L$(THIRD_PATH)/slash/output/lib


LIBS = -lslash \
			 -lpthread \
			 -lprotobuf

LIBRARY = libpink.a


.PHONY: all clean distclean


BASE_OBJS := $(wildcard $(SRC_DIR)/*.cc)
BASE_OBJS += $(wildcard $(SRC_DIR)/*.c)
BASE_OBJS += $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst %.cc,%.o,$(BASE_OBJS))

SLASH = $(THIRD_PATH)/slash/output/lib/libslash.a

all: $(LIBRARY)
	rm -rf $(OUTPUT)
	mkdir -p $(OUTPUT)/include
	mkdir -p $(OUTPUT)/lib
	cp -r ./include $(OUTPUT)/
	mv $(LIBRARY) $(OUTPUT)/lib/
	@echo "Success, go, go, go..."

$(LIBRARY): $(SLASH) $(OBJS)
	rm -rf $(LIBRARY)
	ar -rcs $(LIBRARY) $(OBJS)

$(OBJS): %.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE_PATH) 

$(SLASH):
	make -C $(THIRD_PATH)/slash/ __PERF=$(__PERF)

clean: 
	make clean -C example
	make clean -C $(THIRD_PATH)/slash
	rm -rf $(SRC_DIR)/*.o
	rm -rf $(OUTPUT)/*
	rm -rf $(OUTPUT)

distclean: clean
	make -C $(THIRD_PATH)/slash/ clean

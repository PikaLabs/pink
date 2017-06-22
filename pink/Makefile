CXX = g++
ifeq ($(__PERF), 1)
	CXXFLAGS = -O0 -g -pg -pipe -fPIC -D__XDEBUG__ -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -Wall -D_GNU_SOURCE -std=c++11 -D__STDC_FORMAT_MACROS -Wno-redundant-decls
else
	CXXFLAGS = -O2 -g -pipe -fPIC -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -Wall -Wno-unused-parameter -D_GNU_SOURCE -D__STDC_FORMAT_MACROS -std=c++11 -gdwarf-2 -Wno-redundant-decls -Wno-maybe-uninitialized
	# CXXFLAGS = -Wall -W -DDEBUG -g -O0 -D__XDEBUG__ -D__STDC_FORMAT_MACROS -fPIC -std=c++11 -gdwarf-2
endif

ifndef SLASH_PATH
SLASH_PATH = ./third/slash
endif

ifdef __ENABLE_SSL
CXXFLAGS += -D__ENABLE_SSL
endif

SRC_DIR = ./src
LIB_DIR = ./lib


INCLUDE_PATH = -I../ \
							 -I$(SLASH_PATH)/ 

LIB_PATH = -L./ \
					 -L$(SLASH_PATH)/slash/lib


LIBS = -lslash \
			 -lpthread \
			 -lprotobuf

LIBRARY = libpink.a


.PHONY: all clean distclean


BASE_OBJS := $(wildcard $(SRC_DIR)/*.cc)
BASE_OBJS += $(wildcard $(SRC_DIR)/*.c)
BASE_OBJS += $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst %.cc,%.o,$(BASE_OBJS))

SLASH = $(SLASH_PATH)/slash/lib/libslash.a

all: $(LIBRARY)
	rm -rf $(LIB_DIR)
	mkdir $(LIB_DIR)
	mv $(LIBRARY) $(LIB_DIR)/
	@echo "Success, go, go, go..."

$(LIBRARY): $(SLASH) $(OBJS)
	rm -rf $(LIBRARY)
	ar -rcs $(LIBRARY) $(OBJS)

$(OBJS): %.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE_PATH) 

$(SLASH):
	make -C $(SLASH_PATH)/slash/ __PERF=$(__PERF)

clean: 
	rm -rf $(SRC_DIR)/*.o
	rm -rf $(LIB_DIR)

distclean: clean
	make -C $(SLASH_PATH)/slash/ clean

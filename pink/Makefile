CLEAN_FILES = # deliberately empty, so we can append below.
CC=cc
CXX=g++
LDFLAGS= -lpthread -lrt -lprotobuf $(EXTRA_LDFLAGS)
CFLAGS= -DOS_LINUX -fno-builtin-memcmp -msse -msse4.2 ${EXTRA_CFLAGS}
CXXFLAGS=-std=c++11 -DOS_LINUX -fno-builtin-memcmp -msse -msse4.2 ${EXTRA_CXXFLAGS}
SHARED_CFLAGS=-fPIC
SHARED_LDFLAGS=-Wl,--no-as-needed -shared -Wl,-soname -Wl,
PROFILING_FLAGS=-pg
ARFLAGS = rs
OPT=

# Set the default DEBUG_LEVEL to 0
DEBUG_LEVEL?=0

ifeq ($(MAKECMDGOALS),dbg)
  DEBUG_LEVEL=2 # compatible with rocksdb
endif

ifeq ($(MAKECMDGOALS),all)
  DEBUG_LEVEL=0
endif

ifeq ($(MAKECMDGOALS),clean)
  DEBUG_LEVEL=0
endif

ifeq ($(MAKECMDGOALS),shared_lib)
  DEBUG_LEVEL=0
endif

ifeq ($(MAKECMDGOALS),static_lib)
  DEBUG_LEVEL=0
endif

# compile with -O2 if for release
# if we're compiling for release, compile without debug code (-DNDEBUG) and
# don't treat warnings as errors
ifeq ($(DEBUG_LEVEL),0)
DISABLE_WARNING_AS_ERROR=1
OPT += -O2 -fno-omit-frame-pointer -DNDEBUG
# Skip for archs that don't support -momit-leaf-frame-pointer
ifeq (,$(shell $(CXX) -fsyntax-only -momit-leaf-frame-pointer -xc /dev/null 2>&1))
OPT += -momit-leaf-frame-pointer
endif
else
$(warning Warning: Compiling in debug mode. Don't use the resulting binary in production)
OPT += -O0 -D__XDEBUG__ -D_GNU_SOURCE $(PROFILING_FLAGS)
endif

#-----------------------------------------------

SRC_DIR=./src
LIB_SOURCES := $(SRC_DIR)/build_version.cc \
				$(wildcard $(SRC_DIR)/*.cc) \
				$(wildcard $(SRC_DIR)/*.c) \
				$(wildcard $(SRC_DIR)/*.cpp)

ifndef SLASH_PATH
  $(warning Warning: missing slash path, using default)
	SLASH_PATH=$(CURDIR)/third/slash
endif

SLASH_INCLUDE_DIR=$(SLASH_PATH)
SLASH_LIBRARY=$(SLASH_PATH)/slash/lib/libslash.a

AM_DEFAULT_VERBOSITY = 0

AM_V_GEN = $(am__v_GEN_$(V))
am__v_GEN_ = $(am__v_GEN_$(AM_DEFAULT_VERBOSITY))
am__v_GEN_0 = @echo "  GEN     " $@;
am__v_GEN_1 =
AM_V_at = $(am__v_at_$(V))
am__v_at_ = $(am__v_at_$(AM_DEFAULT_VERBOSITY))
am__v_at_0 = @
am__v_at_1 =

AM_V_CC = $(am__v_CC_$(V))
am__v_CC_ = $(am__v_CC_$(AM_DEFAULT_VERBOSITY))
am__v_CC_0 = @echo "  CC      " $@;
am__v_CC_1 =
CCLD = $(CC)
LINK = $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
AM_V_CCLD = $(am__v_CCLD_$(V))
am__v_CCLD_ = $(am__v_CCLD_$(AM_DEFAULT_VERBOSITY))
am__v_CCLD_0 = @echo "  CCLD    " $@;
am__v_CCLD_1 =
AM_V_AR = $(am__v_AR_$(V))
am__v_AR_ = $(am__v_AR_$(AM_DEFAULT_VERBOSITY))
am__v_AR_0 = @echo "  AR      " $@;
am__v_AR_1 =

AM_LINK = $(AM_V_CCLD)$(CXX) $^ $(EXEC_LDFLAGS) -o $@ $(LDFLAGS) $(COVERAGEFLAGS)

CFLAGS += -g
CXXFLAGS += -g

# This (the first rule) must depend on "all".
default: all

WARNING_FLAGS = -W -Wextra -Wall -Wsign-compare \
  -Wno-unused-parameter -Wno-redundant-decls -Wwrite-strings \
	-Wpointer-arith -Wreorder -Wswitch -Wsign-promo

ifndef DISABLE_WARNING_AS_ERROR
  WARNING_FLAGS += -Werror
endif

CFLAGS += $(WARNING_FLAGS) -I.. -I$(SLASH_INCLUDE_DIR) $(OPT)
CXXFLAGS += $(WARNING_FLAGS) -I.. -I$(SLASH_INCLUDE_DIR) $(OPT) \
						-Woverloaded-virtual -Wnon-virtual-dtor -Wno-missing-field-initializers

LDFLAGS += $(PLATFORM_LDFLAGS)

date := $(shell date +%F)
git_sha := $(shell git rev-parse HEAD 2>/dev/null)
gen_build_version = sed -e s/@@GIT_SHA@@/$(git_sha)/ -e s/@@GIT_DATE_TIME@@/$(date)/ $(SRC_DIR)/build_version.cc.in
# Record the version of the source that we are compiling.
# We keep a record of the git revision in this file.  It is then built
# as a regular source file as part of the compilation process.
# One can run "strings executable_filename | grep _build_" to find
# the version of the source that we used to build the executable file.
CLEAN_FILES += $(SRC_DIR)/build_version.cc

$(SRC_DIR)/build_version.cc: FORCE
	$(AM_V_GEN)rm -f $@-t
	$(AM_V_at)$(gen_build_version) > $@-t
	$(AM_V_at)if test -f $@; then         \
	  cmp -s $@-t $@ && rm -f $@-t || mv -f $@-t $@;    \
	else mv -f $@-t $@; fi
FORCE: 

LIBOBJECTS = $(LIB_SOURCES:.cc=.o)

# if user didn't config LIBNAME, set the default
ifeq ($(LIBNAME),)
# we should only run pink in production with DEBUG_LEVEL 0
ifeq ($(DEBUG_LEVEL),0)
        LIBNAME=libpink
else
        LIBNAME=libpink_debug
endif
endif
LIBOUTPUT = ./lib
dummy := $(shell mkdir -p $(LIBOUTPUT))
LIBRARY = $(LIBOUTPUT)/${LIBNAME}.a

PINK_MAJOR = $(shell egrep "PINK_MAJOR.[0-9]" include/version.h | cut -d ' ' -f 3)
PINK_MINOR = $(shell egrep "PINK_MINOR.[0-9]" include/version.h | cut -d ' ' -f 3)
PINK_PATCH = $(shell egrep "PINK_PATCH.[0-9]" include/version.h | cut -d ' ' -f 3)

#-----------------------------------------------
# Create platform independent shared libraries.
#-----------------------------------------------
PLATFORM_SHARED_EXT=so

SHARED_MAJOR = $(PINK_MAJOR)
SHARED_MINOR = $(PINK_MINOR)
SHARED_PATCH = $(PINK_PATCH)
SHARED1 = ${LIBNAME}.$(PLATFORM_SHARED_EXT)
SHARED2 = $(SHARED1).$(SHARED_MAJOR)
SHARED3 = $(SHARED1).$(SHARED_MAJOR).$(SHARED_MINOR)
SHARED4 = $(SHARED1).$(SHARED_MAJOR).$(SHARED_MINOR).$(SHARED_PATCH)
SHARED = $(SHARED1) $(SHARED2) $(SHARED3) $(SHARED4)
$(SHARED1): $(SHARED4)
	ln -fs $(SHARED4) $(SHARED1)
$(SHARED2): $(SHARED4)
	ln -fs $(SHARED4) $(SHARED2)
$(SHARED3): $(SHARED4)
	ln -fs $(SHARED4) $(SHARED3)

$(SHARED4): $(LIBOBJECTS)
	$(CXX) $(SHARED_LDFLAGS)$(SHARED3) $(CXXFLAGS) $(SHARED_CFLAGS) $(LIB_SOURCES) \
    $(LDFLAGS) -o $@

EXAMPLES = bg_thread http_server mydispatch_srv myholy_srv myholy_srv_chandle myproto_cli \
				redis_cli_test simple_http_server

.PHONY: clean dbg static_lib shared_lib all example

all: $(LIBRARY)

static_lib: $(LIBRARY)

shared_lib: $(SHARED)
	for f in $(notdir $(SHARED)); do mv "$$f" $(LIBOUTPUT); done

example: $(EXAMPLES)

check: $(LIBRARY) $(TESTS)
	for t in $(notdir $(TESTS)); do echo "***** Running $$t"; ./$$t || exit 1; done

dbg: $(LIBRARY) $(EXAMPLES)

$(LIBRARY): $(LIBOBJECTS)
	$(AM_V_AR)rm -f $@
	$(AM_V_at)$(AR) $(ARFLAGS) $@ $(LIBOBJECTS)

clean:
	make -C ./examples clean
	rm -f $(EXAMPLES)
	rm -f $(LIBRARY) $(SHARED)
	rm -rf $(CLEAN_FILES)
	rm -rf $(LIBOUTPUT)
	find . -name "*.[oda]" ! -path "./third/*" -exec rm -f {} \;
	find . -type f -regex ".*\.\(\(gcda\)\|\(gcno\)\)" -exec rm {} \;

.cc.o:
	$(AM_V_CC)$(CXX) $(CXXFLAGS) -c $< -o $@ $(COVERAGEFLAGS)

.c.o:
	$(AM_V_CC)$(CC) $(CFLAGS) -c $< -o $@

# examples
EXEC_LDFLAGS+=$(SLASH_LIBRARY)

bg_thread: examples/bg_thread.o $(LIBOBJECTS)
	$(AM_LINK)

http_server: examples/http_server.o $(LIBOBJECTS)
	$(AM_LINK)

mydispatch_srv: examples/mydispatch_srv.o examples/myproto.pb.cc $(LIBOBJECTS)
	$(AM_LINK)

myholy_srv: examples/myholy_srv.o examples/myproto.pb.cc $(LIBOBJECTS)
	$(AM_LINK)

myholy_srv_chandle: examples/myholy_srv_chandle.o examples/myproto.pb.cc $(LIBOBJECTS)
	$(AM_LINK)

myproto_cli: examples/myproto_cli.o examples/myproto.pb.cc $(LIBOBJECTS)
	$(AM_LINK)

redis_cli_test: examples/redis_cli_test.o $(LIBOBJECTS)
	$(AM_LINK)

simple_http_server: examples/simple_http_server.o $(LIBOBJECTS)
	$(AM_LINK)

examples/myproto.pb.cc: examples/myproto.proto
	protoc -I=./examples --cpp_out=./examples ./examples/$^

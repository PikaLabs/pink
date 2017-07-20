CLEAN_FILES = # deliberately empty, so we can append below.
CXX=g++
LDFLAGS= -lpthread -lrt -lprotobuf
CXXFLAGS=-std=c++11 -fno-builtin-memcmp -msse -msse4.2
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

ifeq ($(MAKECMDGOALS),static_lib)
  DEBUG_LEVEL=0
endif

# compile with -O2 if for release
# if we're compiling for release, compile without debug code (-DNDEBUG) and
# don't treat warnings as errors
ifeq ($(DEBUG_LEVEL),0)
DISABLE_WARNING_AS_ERROR=1
OPT += -O2 -fno-omit-frame-pointer -DNDEBUG
else
$(warning Warning: Compiling in debug mode. Don't use the resulting binary in production)
OPT += -O0 -D__XDEBUG__ -D_GNU_SOURCE $(PROFILING_FLAGS)
endif

#-----------------------------------------------

SRC_DIR=src
VERSION_CC=$(SRC_DIR)/build_version.cc
LIB_SOURCES :=  $(VERSION_CC) \
				$(filter-out $(VERSION_CC), $(wildcard $(SRC_DIR)/*.cc))

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

AM_V_CXX = $(am__v_CXX_$(V))
am__v_CXX_ = $(am__v_CXX_$(AM_DEFAULT_VERBOSITY))
am__v_CXX_0 = @echo "  CXX     " $@;
am__v_CXX_1 =
LD = $(CXX)
AM_V_LD = $(am__v_LD_$(V))
am__v_LD_ = $(am__v_LD_$(AM_DEFAULT_VERBOSITY))
am__v_LD_0 = @echo "  LD      " $@;
am__v_LD_1 =
AM_V_AR = $(am__v_AR_$(V))
am__v_AR_ = $(am__v_AR_$(AM_DEFAULT_VERBOSITY))
am__v_AR_0 = @echo "  AR      " $@;
am__v_AR_1 =

AM_LINK = $(AM_V_LD)$(CXX) $^ $(EXEC_LDFLAGS) -o $@ $(LDFLAGS)

CXXFLAGS += -g

# This (the first rule) must depend on "all".
default: all

WARNING_FLAGS = -W -Wextra -Wall -Wsign-compare \
  -Wno-unused-parameter -Wno-redundant-decls -Wwrite-strings \
	-Wpointer-arith -Wreorder -Wswitch -Wsign-promo \
	-Woverloaded-virtual -Wnon-virtual-dtor -Wno-missing-field-initializers

ifndef DISABLE_WARNING_AS_ERROR
  WARNING_FLAGS += -Werror
endif

CXXFLAGS += $(WARNING_FLAGS) -I.. -I$(SLASH_INCLUDE_DIR) $(OPT)

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

EXAMPLES = bg_thread http_server mydispatch_srv myholy_srv myholy_srv_chandle myproto_cli \
				redis_cli_test simple_http_server

TESTS = test/pink_thread_test

.PHONY: clean dbg static_lib all example

all: $(LIBRARY)

static_lib: $(LIBRARY)

example: $(EXAMPLES)

check: $(LIBRARY)
	@make -C test
	for t in $(notdir $(TESTS)); do echo "***** Running $$t"; ./test/$$t || exit 1; done

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
	find . -name "*.[oda]*" ! -path "./third/*" -exec rm -f {} \;
	find . -type f -regex ".*\.\(\(gcda\)\|\(gcno\)\)" -exec rm {} \;

%.o: %.cc
	$(AM_V_CXX)$(CXX) $(CXXFLAGS) -c $< -o $@

%.d: %.cc
	@set -e; rm -f $@; $(CXX) -MM $(CXXFLAGS) $< > $@.$$$$; \
	sed 's,\($(notdir $*)\)\.o[ :]*,$(SRC_DIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

ifneq ($(MAKECMDGOALS),clean)
  -include $(LIBOBJECTS:.o=.d)
endif

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

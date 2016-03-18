CXX = g++
ifeq ($(__PERF), 1)
	CXXFLAGS = -O0 -g -gstabs+ -pg -pipe -fPIC -D__XDEBUG__ -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -D_GNU_SOURCE -D__STDC_FORMAT_MACROS -std=c++11 -Wno-unused-variable
else
	CXXFLAGS = -O2 -pipe -fPIC -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -Wall -D_GNU_SOURCE -D__STDC_FORMAT_MACROS -std=c++11 -gdwarf-2 -Wno-unused-variable -Wno-maybe-uninitialized -Wno-unused-parameter
	# CXXFLAGS = -Wall -W -DDEBUG -g -O0 -D__XDEBUG__ 
endif
OBJECT = pink holy pika pika_holy pika_simple pcli redis_cli
# OBJECT = pcli pink

SRC_DIR = ./

OUTPUT = ./output

LIB_PATH = -L../ \
		   -L../output/lib/

LIBS = -lpink \
			 -lprotobuf \
			 -lpthread

INCLUDE_PATH = -I../output/include/

.PHONY: all clean version pcli redis_cli


BASE_BOJS := $(wildcard $(SRC_DIR)/*.cc)
BASE_BOJS += $(wildcard $(SRC_DIR)/*.c)
BASE_BOJS += $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst %.cc,%.o,$(BASE_BOJS))

all: $(OBJECT)
	rm -rf $(OUTPUT)
	mkdir -p $(OUTPUT)
	mkdir -p $(OUTPUT)/bin
	mkdir -p $(OUTPUT)/log
	cp $(OBJECT) $(OUTPUT)/bin/
	rm -rf $(OBJECT)
	@echo "Success, go, go, go..."


pink: pink.cc bada_thread.cc pink.pb.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDE_PATH) $(LIB_PATH) $(LIBS)

holy: holy.cc holy_test.cc pink.pb.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDE_PATH) $(LIB_PATH) $(LIBS)

pika: pika.cc pika_thread.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDE_PATH) $(LIB_PATH) $(LIBS)

pika_holy: pika_holy.cc pika_holy_thread.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDE_PATH) $(LIB_PATH) $(LIBS)

pika_simple: pika_simple.cc pika_simple_thread.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDE_PATH) $(LIB_PATH) $(LIBS)

pcli: cli_test.cc pink.pb.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDE_PATH) $(LIB_PATH) $(LIBS)

redis_cli: redis_cli_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDE_PATH) $(LIB_PATH) $(LIBS)

$(OBJS): %.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE_PATH) 

clean: 
	rm -rf $(OUTPUT)
	rm -rf $(SRC_DIR)/log
	rm -f $(SRC_DIR)/*.o
	rm -rf $(OBJECT)


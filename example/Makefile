CXX = g++
CXXFLAGS = -Wall -W -DDEBUG -g -O0 -D__XDEBUG__
OBJECT = pink holy
SRC_DIR = ./
OUTPUT = ./output

LIB_PATH = -L../ \
		   -L../output/lib/

LIBS = -lpink \
			 -lprotobuf \
			 -lpthread

INCLUDE_PATH = -I../output/include/

LIBRARY = libpink.a
.PHONY: all clean version


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

$(OBJS): %.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE_PATH) 

$(LIBRARY): $(OBJS)
	rm -rf $@
	ar -rcs $@ $(OBJS)


clean: 
	rm -rf $(OUTPUT)
	rm -rf $(SRC_DIR)/log
	rm -f $(SRC_DIR)/*.o
	rm -rf $(OBJECT)


CC = g++
CC_FLAGS = -std=c++11 -g

INCLUDE_PATH = -I/home/wxf/workplace/protobuf/src/ -I/usr/local/include/ -I./
LIBRARY_PATH = -L/home/wxf/workplace/protobuf/src/.libs/
LIBRARIES = -Wl,-Bstatic -lprotobuf  -Wl,-Bdynamic

LIB = libpink_rpc.a

SOURCES = $(wildcard ./*.cc) 
OBJS = $(patsubst %.cc, %.o, $(SOURCES)) 

.PHONY: all clean

all: $(LIB)
	@echo "build success"

$(LIB): $(OBJS)
	rm $@
	ar -crvs $@ $^

$(OBJS): %.o : %.cc
	$(CC) $(CC_FLAGS) $(INCLUDE_PATH) -c $< -o $@

clean:
	@rm *.o $(BIN)

